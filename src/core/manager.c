/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2022 EMQ Technologies Co., Ltd All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 **/
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#include <arpa/inet.h>

#include "event/event.h"
#include "persist/persist.h"
#include "utils/base64.h"
#include "utils/http.h"
#include "utils/log.h"
#include "utils/time.h"

#include "adapter.h"
#include "adapter/adapter_internal.h"
#include "adapter/driver/driver_internal.h"
#include "adapter/storage.h"
#include "argparse.h"
#include "base/msg_internal.h"
#include "errcodes.h"
#include "otel/otel_manager.h"

#include "node_manager.h"
#include "plugin_manager.h"
#include "storage.h"
#include "subscribe.h"

#include "manager.h"
#include "manager_internal.h"

// definition for adapter names
#define DEFAULT_DASHBOARD_ADAPTER_NAME DEFAULT_DASHBOARD_PLUGIN_NAME

static int manager_loop(enum neu_event_io_type type, int fd, void *usr_data);

inline static void reply(neu_manager_t *manager, neu_reqresp_head_t *header,
                         void *data);
inline static void forward_msg(neu_manager_t *     manager,
                               neu_reqresp_head_t *header, const char *node);
inline static void forward_msg_copy(neu_manager_t *     manager,
                                    neu_reqresp_head_t *header,
                                    const char *        node);

inline static void notify_monitors(neu_manager_t *    manager,
                                   neu_reqresp_type_e event, void *data);
static void start_static_adapter(neu_manager_t *manager, const char *name);
static int  update_timestamp(void *usr_data);
static void start_single_adapter(neu_manager_t *manager, const char *name,
                                 const char *plugin_name, bool display);

static char *file_save_tmp(const char *data, const char *suffix);
static bool  mv_tmp_library_file(neu_plugin_kind_e kind, const char *tmp_path,
                                 const char *library);
static bool  mv_tmp_schema_file(neu_plugin_kind_e kind, const char *tmp_path,
                                const char *schema);

uint16_t neu_manager_get_port()
{
    static uint16_t port = 10000;
    return port++;
}

neu_manager_t *neu_manager_create()
{
    int                  rv      = 0;
    neu_manager_t *      manager = calloc(1, sizeof(neu_manager_t));
    neu_event_io_param_t param   = {
        .usr_data = (void *) manager,
        .cb       = manager_loop,
    };

    neu_event_timer_param_t timestamp_timer_param = {
        .second      = 0,
        .millisecond = 10,
        .cb          = update_timestamp,
        .type        = NEU_EVENT_TIMER_NOBLOCK,
    };

    manager->events            = neu_event_new();
    manager->plugin_manager    = neu_plugin_manager_create();
    manager->node_manager      = neu_node_manager_create();
    manager->subscribe_manager = neu_subscribe_manager_create();
    manager->log_level         = default_log_level;

    manager->server_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    assert(manager->server_fd > 0);

    struct timeval sock_timeout = {
        .tv_sec  = 1,
        .tv_usec = 0,
    };
    if (setsockopt(manager->server_fd, SOL_SOCKET, SO_SNDTIMEO, &sock_timeout,
                   sizeof(sock_timeout)) < 0 ||
        setsockopt(manager->server_fd, SOL_SOCKET, SO_RCVTIMEO, &sock_timeout,
                   sizeof(sock_timeout)) < 0) {
        assert(!"fail to set manager sock timeout");
    }

    // abstract domain socket is a Linux extension, thus not portable.
    // use abstract domain socket here to avoid polluting the file system
    struct sockaddr_un local = {
        .sun_family = AF_UNIX,
        .sun_path   = "#neuron-manager",
    };
    local.sun_path[0] = '\0';
    rv = bind(manager->server_fd, (struct sockaddr *) &local, sizeof(local));
    assert(rv == 0);

    param.fd      = manager->server_fd;
    manager->loop = neu_event_add_io(manager->events, param);

    manager->timestamp_lev_manager = 0;

    strncpy(g_status, "loading", sizeof(g_status));

    neu_metrics_init();
    start_static_adapter(manager, DEFAULT_DASHBOARD_PLUGIN_NAME);

    if (manager_load_plugin(manager) != 0) {
        nlog_warn("load plugin error");
    }

    UT_array *single_plugins =
        neu_plugin_manager_get_single(manager->plugin_manager);

    utarray_foreach(single_plugins, neu_resp_plugin_info_t *, plugin)
    {
        start_single_adapter(manager, plugin->single_name, plugin->name,
                             plugin->display);
    }
    utarray_free(single_plugins);

    manager_load_node(manager);
    while (neu_node_manager_exist_uninit(manager->node_manager)) {
        usleep(1000 * 100);
    }

    manager_load_subscribe(manager);

    timestamp_timer_param.usr_data = (void *) manager;
    manager->timer_timestamp =
        neu_event_add_timer(manager->events, timestamp_timer_param);

    strncpy(g_status, "ready", sizeof(g_status));
    nlog_notice("manager start");
    return manager;
}

void neu_manager_destroy(neu_manager_t *manager)
{
    neu_req_node_init_t uninit           = { 0 };
    int                 send_msg_success = 1;

    UT_array *addrs = neu_node_manager_get_addrs_all(manager->node_manager);

    neu_event_del_timer(manager->events, manager->timer_timestamp);

    utarray_foreach(addrs, struct sockaddr_un *, addr)
    {
        neu_msg_t *msg = neu_msg_new(NEU_REQ_NODE_UNINIT, NULL, &uninit);
        neu_reqresp_head_t *header = neu_msg_get_header(msg);
        strcpy(header->sender, "manager");
        if (0 != neu_send_msg_to(manager->server_fd, addr, msg)) {
            nlog_error("manager -> %s uninit msg send fail",
                       &addr->sun_path[1]);
            send_msg_success = 0;
            neu_msg_free(msg);
            break;
        }
    }
    utarray_free(addrs);

    if (send_msg_success) {
        while (1) {
            usleep(1000 * 100);
            if (neu_node_manager_size(manager->node_manager) == 0) {
                break;
            }
        }
    }

    neu_subscribe_manager_destroy(manager->subscribe_manager);
    neu_node_manager_destroy(manager->node_manager);
    neu_plugin_manager_destroy(manager->plugin_manager);

    close(manager->server_fd);
    neu_event_del_io(manager->events, manager->loop);
    neu_event_close(manager->events);

    free(manager);
    nlog_notice("manager exit");
}

static int manager_loop(enum neu_event_io_type type, int fd, void *usr_data)
{
    int                 rv       = 0;
    neu_manager_t *     manager  = (neu_manager_t *) usr_data;
    struct sockaddr_un  src_addr = { 0 };
    neu_msg_t *         msg      = NULL;
    neu_reqresp_head_t *header   = NULL;

    if (type == NEU_EVENT_IO_CLOSED || type == NEU_EVENT_IO_HUP) {
        nlog_warn("manager socket(%d) recv closed or hup %d.", fd, type);
        return 0;
    }

    rv = neu_recv_msg_from(manager->server_fd, &src_addr, &msg);
    if (rv == -1) {
        nlog_warn("manager recv msg error: %s(%d)", strerror(errno), errno);
        return 0;
    }

    header = neu_msg_get_header(msg);

    nlog_info("manager recv msg from: %s to %s, type: %s", header->sender,
              header->receiver, neu_reqresp_type_string(header->type));
    switch (header->type) {
    case NEU_REQ_NODE_INIT: {
        neu_req_node_init_t *init = (neu_req_node_init_t *) &header[1];

        if (0 !=
            neu_node_manager_update(manager->node_manager, init->node,
                                    src_addr)) {
            nlog_warn("bind node %s to src addr(%s) fail", init->node,
                      &src_addr.sun_path[1]);
            neu_msg_free(msg);
            break;
        }

        if (init->state == NEU_NODE_RUNNING_STATE_READY ||
            init->state == NEU_NODE_RUNNING_STATE_RUNNING ||
            init->state == NEU_NODE_RUNNING_STATE_STOPPED) {
            neu_adapter_t *adapter =
                neu_node_manager_find(manager->node_manager, init->node);
            neu_adapter_start(adapter);
            if (init->state == NEU_NODE_RUNNING_STATE_STOPPED) {
                neu_adapter_stop(adapter);
            }
        }

        nlog_notice("bind node %s to src addr(%s)", init->node,
                    &src_addr.sun_path[1]);
        neu_msg_free(msg);
        break;
    }
    case NEU_REQ_ADD_PLUGIN: {
        neu_req_add_plugin_t *cmd        = (neu_req_add_plugin_t *) &header[1];
        neu_resp_error_t      e          = { 0 };
        neu_plugin_kind_e     kind       = -1;
        char                  schema[64] = { 0 };
        char                  buffer[65] = { 0 };

        if (sscanf(cmd->library, "libplugin-%64s.so", buffer) != 1) {
            nlog_warn("library %s no conform", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_NAME_NOT_CONFORM;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (neu_persister_library_exists(cmd->library)) {
            nlog_warn("library %s had exited", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_NAME_CONFLICT;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        char *so_tmp_path = file_save_tmp(cmd->so_file, "so");

        if (so_tmp_path == NULL) {
            nlog_warn("library %s so file save tmp fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_BODY_IS_WRONG;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        neu_plugin_instance_t ins   = { 0 };
        int                   error = NEU_ERR_SUCCESS;

        if (neu_plugin_manager_create_instance_by_path(
                manager->plugin_manager, so_tmp_path, &ins, &error)) {
            char module_name[64] = { 0 };
            kind                 = ins.module->kind;
            strncpy(module_name, ins.module->module_name, sizeof(module_name));
            strncpy(schema, ins.module->schema, sizeof(schema));

            nlog_debug("library %s, module_name %s, schema:%s", cmd->library,
                       module_name, schema);

            uint32_t major = NEU_GET_VERSION_MAJOR(ins.module->version);
            uint32_t minor = NEU_GET_VERSION_MINOR(ins.module->version);

            neu_plugin_manager_destroy_instance(manager->plugin_manager, &ins);

            if (NEU_VERSION_MAJOR != major || NEU_VERSION_MINOR != minor) {
                nlog_warn("library %s plugin version error, major:%d minor:%d",
                          module_name, major, minor);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_MODULE_VERSION_NOT_MATCH;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }

            if (neu_plugin_manager_exists(manager->plugin_manager,
                                          module_name)) {

                nlog_warn("%s module name had existed", module_name);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_MODULE_ALREADY_EXIST;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }

        } else {
            nlog_warn("library %s so file is not a vaild file", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = error;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (kind != NEU_PLUGIN_KIND_CUSTOM && kind != NEU_PLUGIN_KIND_SYSTEM) {
            nlog_warn("library %s kind no support", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_MODULE_KIND_NOT_SUPPORT;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        char *schema_tmp_path = file_save_tmp(cmd->schema_file, "json");

        if (schema_tmp_path == NULL) {
            nlog_warn("library %s schema file save tmp fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_BODY_IS_WRONG;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (!mv_tmp_library_file(kind, so_tmp_path, cmd->library)) {
            nlog_warn("library %s mv library tmp file fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            free(schema_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_ADD_FAIL;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (!mv_tmp_schema_file(kind, schema_tmp_path, schema)) {
            nlog_warn("library %s schema file save schema fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            free(schema_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_ADD_FAIL;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        error = neu_manager_add_plugin(manager, cmd->library);

        if (error == NEU_ERR_SUCCESS) {
            manager_strorage_plugin(manager);
            // notify_monitors(manager, NEU_REQ_ADD_PLUGIN_EVENT, cmd);
        }

        free(so_tmp_path);
        free(cmd->so_file);
        free(cmd->schema_file);
        free(schema_tmp_path);
        header->type = NEU_RESP_ERROR;
        e.error      = error;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &e);
        break;
    }
    case NEU_REQ_DEL_PLUGIN: {
        neu_req_del_plugin_t *cmd = (neu_req_del_plugin_t *) &header[1];
        neu_resp_error_t      e   = { 0 };

        UT_array *nodes = neu_manager_get_nodes(
            manager, NEU_NA_TYPE_DRIVER | NEU_NA_TYPE_APP, cmd->plugin, "");

        if (nodes != NULL) {
            if (utarray_len(nodes) > 0) {
                utarray_free(nodes);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_IN_USE;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }
            utarray_free(nodes);
        }

        int error = neu_manager_del_plugin(manager, cmd->plugin);
        e.error   = error;

        if (error == NEU_ERR_SUCCESS) {
            manager_strorage_plugin(manager);
            // notify_monitors(manager, NEU_REQ_DEL_PLUGIN_EVENT, cmd);
        }

        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &e);
        break;
    }
    case NEU_REQ_UPDATE_PLUGIN: {
        neu_req_update_plugin_t *cmd  = (neu_req_update_plugin_t *) &header[1];
        neu_resp_error_t         e    = { 0 };
        neu_plugin_kind_e        kind = -1;
        char                     module_name[64] = { 0 };
        char                     schema[64]      = { 0 };
        char                     buffer[65]      = { 0 };

        if (sscanf(cmd->library, "libplugin-%64s.so", buffer) != 1) {
            nlog_warn("library %s no conform", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_NAME_NOT_CONFORM;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (!neu_persister_library_exists(cmd->library)) {
            nlog_warn("library %s no exited", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_NOT_FOUND;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        char *so_tmp_path = file_save_tmp(cmd->so_file, "so");

        if (so_tmp_path == NULL) {
            nlog_warn("library %s so file save tmp fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_BODY_IS_WRONG;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        neu_plugin_instance_t ins   = { 0 };
        int                   error = NEU_ERR_SUCCESS;

        if (neu_plugin_manager_create_instance_by_path(
                manager->plugin_manager, so_tmp_path, &ins, &error)) {
            kind = ins.module->kind;
            strncpy(module_name, ins.module->module_name, sizeof(module_name));
            strncpy(schema, ins.module->schema, sizeof(schema));

            nlog_debug("library %s, module_name %s, schema:%s", cmd->library,
                       module_name, schema);

            uint32_t major = NEU_GET_VERSION_MAJOR(ins.module->version);
            uint32_t minor = NEU_GET_VERSION_MINOR(ins.module->version);

            neu_plugin_manager_destroy_instance(manager->plugin_manager, &ins);

            if (NEU_VERSION_MAJOR != major || NEU_VERSION_MINOR != minor) {
                nlog_warn("library %s plugin version error, major:%d minor:%d",
                          module_name, major, minor);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_MODULE_VERSION_NOT_MATCH;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }

            if (!neu_plugin_manager_create_instance_by_lib_name(
                    manager->plugin_manager, cmd->library, &ins)) {
                nlog_warn("library %s not instance", cmd->library);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_UPDATE_FAIL;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }

            if (strcmp(module_name, ins.module->module_name) != 0) {
                nlog_warn("library %s module name mismatch!", cmd->library);
                neu_plugin_manager_destroy_instance(manager->plugin_manager,
                                                    &ins);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_UPDATE_FAIL;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }

            neu_plugin_manager_destroy_instance(manager->plugin_manager, &ins);

            if (!neu_plugin_manager_exists(manager->plugin_manager,
                                           module_name)) {

                nlog_warn("library %s plugin name no existed", module_name);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_MODULE_NOT_EXISTS;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }

        } else {
            nlog_warn("library %s so file is not a vaild file", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = error;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (kind != NEU_PLUGIN_KIND_CUSTOM && kind != NEU_PLUGIN_KIND_SYSTEM) {
            nlog_warn("library %s kind no support", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_MODULE_KIND_NOT_SUPPORT;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        char *schema_tmp_path = file_save_tmp(cmd->schema_file, "json");

        if (schema_tmp_path == NULL) {
            nlog_warn("library %s schema file save tmp fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_BODY_IS_WRONG;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        UT_array *nodes = neu_manager_get_nodes(
            manager, NEU_NA_TYPE_DRIVER | NEU_NA_TYPE_APP, module_name, "");

        if (nodes != NULL) {
            if (utarray_len(nodes) > 0) {
                utarray_free(nodes);
                free(cmd->so_file);
                free(cmd->schema_file);
                free(so_tmp_path);
                free(schema_tmp_path);
                nlog_warn("library %s is using", cmd->library);
                header->type = NEU_RESP_ERROR;
                e.error      = NEU_ERR_LIBRARY_IN_USE;
                strcpy(header->receiver, header->sender);
                reply(manager, header, &e);
                break;
            }
            utarray_free(nodes);
        }

        if (!neu_plugin_manager_remove_library(manager->plugin_manager,
                                               cmd->library)) {
            nlog_warn("library %s src file remove fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            free(schema_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_UPDATE_FAIL;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (!mv_tmp_library_file(kind, so_tmp_path, cmd->library)) {
            nlog_warn("library %s mv library tmp file fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            free(schema_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_UPDATE_FAIL;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (!mv_tmp_schema_file(kind, schema_tmp_path, schema)) {
            nlog_warn("library %s schema file save schema fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            free(schema_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_UPDATE_FAIL;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        if (neu_plugin_manager_update(manager->plugin_manager, cmd->library) !=
            NEU_ERR_SUCCESS) {
            nlog_warn("update library %s fail", cmd->library);
            free(cmd->so_file);
            free(cmd->schema_file);
            free(so_tmp_path);
            free(schema_tmp_path);
            header->type = NEU_RESP_ERROR;
            e.error      = NEU_ERR_LIBRARY_UPDATE_FAIL;
            strcpy(header->receiver, header->sender);
            reply(manager, header, &e);
            break;
        }

        free(cmd->so_file);
        free(cmd->schema_file);
        free(so_tmp_path);
        free(schema_tmp_path);
        header->type = NEU_RESP_ERROR;
        e.error      = NEU_ERR_SUCCESS;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &e);

        break;
    }
    case NEU_REQ_GET_PLUGIN: {
        UT_array *            plugins = neu_manager_get_plugins(manager);
        neu_resp_get_plugin_t resp    = { .plugins = plugins };

        header->type = NEU_RESP_GET_PLUGIN;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &resp);
        break;
    }
    case NEU_REQ_ADD_NODE: {
        neu_req_add_node_t *cmd = (neu_req_add_node_t *) &header[1];
        nlog_notice("add node name:%s plugin:%s", cmd->node, cmd->plugin);
        int error =
            neu_manager_add_node(manager, cmd->node, cmd->plugin, cmd->setting,
                                 NEU_NODE_RUNNING_STATE_INIT, false);

        neu_resp_error_t e = { .error = error };

        if (error == NEU_ERR_SUCCESS) {
            manager_storage_add_node(manager, cmd->node);
            if (cmd->setting) {
                adapter_storage_setting(cmd->node, cmd->setting);
            }
            notify_monitors(manager, NEU_REQ_ADD_NODE_EVENT, cmd);
        }

        neu_req_add_node_fini(cmd);
        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &e);
        break;
    }
    case NEU_REQ_UPDATE_NODE: {
        neu_req_update_node_t *cmd = (neu_req_update_node_t *) &header[1];
        neu_resp_error_t       e   = { 0 };
        nlog_notice("update node name:%s new_name:%s", cmd->node,
                    cmd->new_name);
        if (NULL == neu_node_manager_find(manager->node_manager, cmd->node)) {
            e.error = NEU_ERR_NODE_NOT_EXIST;
        } else if (NULL !=
                   neu_node_manager_find(manager->node_manager,
                                         cmd->new_name)) {
            // this also makes renaming to the original name an error
            e.error = NEU_ERR_NODE_EXIST;
        }

        if (0 == e.error) {
            header->type = NEU_REQ_NODE_RENAME;
            forward_msg(manager, header, header->receiver);
        } else {
            header->type = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        }

        break;
    }
    case NEU_REQ_DEL_NODE: {
        neu_req_del_node_t *cmd   = (neu_req_del_node_t *) &header[1];
        neu_resp_error_t    error = { 0 };
        nlog_notice("del node name:%s", cmd->node);
        neu_adapter_t *adapter =
            neu_node_manager_find(manager->node_manager, cmd->node);
        bool single =
            neu_node_manager_is_single(manager->node_manager, cmd->node);

        strcpy(header->receiver, cmd->node);
        if (adapter == NULL) {
            error.error  = NEU_ERR_NODE_NOT_EXIST;
            header->type = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &error);
            break;
        }

        if (single) {
            error.error  = NEU_ERR_NODE_NOT_ALLOW_DELETE;
            header->type = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &error);
            break;
        }

        manager_storage_del_node(manager, cmd->node);
        if (neu_adapter_get_type(adapter) == NEU_NA_TYPE_APP) {
            UT_array *subscriptions = neu_subscribe_manager_get(
                manager->subscribe_manager, cmd->node, NULL, NULL);
            neu_subscribe_manager_unsub_all(manager->subscribe_manager,
                                            cmd->node);

            utarray_foreach(subscriptions, neu_resp_subscribe_info_t *, sub)
            {
                // NOTE: neu_req_unsubscribe_t and neu_resp_subscribe_info_t
                //       have compatible memory layout
                msg = neu_msg_new(NEU_REQ_UNSUBSCRIBE_GROUP, NULL, sub);
                if (NULL == msg) {
                    break;
                }
                neu_reqresp_head_t *hd = neu_msg_get_header(msg);
                strcpy(hd->receiver, sub->driver);
                strcpy(hd->sender, "manager");
                forward_msg(manager, hd, hd->receiver);
            }
            utarray_free(subscriptions);
        }

        notify_monitors(manager, NEU_REQ_DEL_NODE_EVENT, cmd);
        neu_reqresp_node_deleted_t resp = { 0 };
        strcpy(resp.node, header->receiver);
        // notify MQTT about node removal
        if (0 == strcmp(adapter->module->module_name, "MQTT")) {
            msg = neu_msg_new(NEU_REQRESP_NODE_DELETED, NULL, &resp);
            if (NULL != msg) {
                neu_reqresp_head_t *hd = neu_msg_get_header(msg);
                strcpy(hd->receiver, cmd->node);
                strcpy(hd->sender, "manager");
                reply(manager, hd, &resp);
            }
        }
        header->type = NEU_REQ_NODE_UNINIT;
        forward_msg(manager, header, header->receiver);

        if (neu_adapter_get_type(adapter) == NEU_NA_TYPE_DRIVER) {
            UT_array *apps = neu_subscribe_manager_find_by_driver(
                manager->subscribe_manager, resp.node);

            utarray_foreach(apps, neu_app_subscribe_t *, app)
            {
                msg = neu_msg_new(NEU_REQRESP_NODE_DELETED, NULL, &resp);
                if (NULL == msg) {
                    break;
                }
                header = neu_msg_get_header(msg);
                strcpy(header->receiver, app->app_name);
                strcpy(header->sender, "manager");
                reply(manager, header, &resp);
            }
            utarray_free(apps);
        }

        break;
    }
    case NEU_RESP_NODE_UNINIT: {
        neu_resp_node_uninit_t *cmd = (neu_resp_node_uninit_t *) &header[1];

        neu_manager_del_node(manager, cmd->node);
        if (strlen(header->receiver) > 0 &&
            strcmp(header->receiver, "manager") != 0) {
            neu_resp_error_t error = { 0 };
            header->type           = NEU_RESP_ERROR;
            reply(manager, header, &error);
        } else {
            neu_msg_free(msg);
        }
        break;
    }
    case NEU_REQ_GET_NODE: {
        neu_req_get_node_t *cmd = (neu_req_get_node_t *) &header[1];
        UT_array *          nodes =
            neu_manager_get_nodes(manager, cmd->type, cmd->plugin, cmd->node);
        neu_resp_get_node_t resp = { .nodes = nodes };

        header->type = NEU_RESP_GET_NODE;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &resp);
        break;
    }
    case NEU_REQ_SUBSCRIBE_GROUP: {
        neu_req_subscribe_t *cmd      = (neu_req_subscribe_t *) &header[1];
        neu_resp_error_t     error    = { 0 };
        uint16_t             app_port = 0;

        nlog_notice("add sub app_name:%s driver_node:%s group:%s params:%s",
                    cmd->app, cmd->driver, cmd->group,
                    cmd->params != NULL ? cmd->params : "");

        error.error = neu_manager_subscribe(manager, cmd->app, cmd->driver,
                                            cmd->group, cmd->params, &app_port);

        if (error.error == NEU_ERR_SUCCESS) {
            cmd->port = app_port;
            forward_msg_copy(manager, header, cmd->app);
            forward_msg_copy(manager, header, cmd->driver);
            manager_storage_subscribe(manager, cmd->app, cmd->driver,
                                      cmd->group, cmd->params);
        } else {
            free(cmd->params);
        }

        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &error);
        break;
    }
    case NEU_REQ_SUBSCRIBE_GROUPS: {
        neu_req_subscribe_groups_t *cmd =
            (neu_req_subscribe_groups_t *) &header[1];
        neu_resp_error_t error = { 0 };

        for (uint16_t i = 0; i < cmd->n_group; ++i) {
            neu_req_subscribe_group_info_t *info = &cmd->groups[i];
            nlog_notice("add sub app_name:%s driver_node:%s group:%s params:%s",
                        cmd->app, info->driver, info->group,
                        info->params != NULL ? info->params : "");
            error.error =
                neu_manager_subscribe(manager, cmd->app, info->driver,
                                      info->group, info->params, &info->port);
            if (0 != error.error) {
                break;
            }

            error.error = neu_manager_send_subscribe(manager, cmd->app,
                                                     info->driver, info->group,
                                                     info->port, info->params);
            if (0 != error.error) {
                break;
            }

            manager_storage_subscribe(manager, cmd->app, info->driver,
                                      info->group, info->params);
        }

        neu_req_subscribe_groups_fini(cmd);
        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &error);
        break;
    }
    case NEU_REQ_UPDATE_SUBSCRIBE_GROUP: {
        neu_req_subscribe_t *cmd   = (neu_req_subscribe_t *) &header[1];
        neu_resp_error_t     error = { 0 };
        nlog_notice("update sub app_name:%s driver_node:%s group:%s params:%s",
                    cmd->app, cmd->driver, cmd->group,
                    cmd->params != NULL ? cmd->params : "");
        error.error = neu_manager_update_subscribe(
            manager, cmd->app, cmd->driver, cmd->group, cmd->params);

        if (error.error == NEU_ERR_SUCCESS) {
            forward_msg_copy(manager, header, cmd->app);
            manager_storage_update_subscribe(manager, cmd->app, cmd->driver,
                                             cmd->group, cmd->params);
            notify_monitors(manager, NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT, cmd);
        } else {
            free(cmd->params);
        }

        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &error);
        break;
    }
    case NEU_REQ_UNSUBSCRIBE_GROUP: {
        neu_req_unsubscribe_t *cmd   = (neu_req_unsubscribe_t *) &header[1];
        neu_resp_error_t       error = { 0 };
        nlog_notice("del sub app_name:%s driver_node:%s group:%s", cmd->app,
                    cmd->driver, cmd->group);
        error.error =
            neu_manager_unsubscribe(manager, cmd->app, cmd->driver, cmd->group);

        if (error.error == NEU_ERR_SUCCESS) {
            forward_msg_copy(manager, header, cmd->app);
            forward_msg_copy(manager, header, cmd->driver);
            manager_storage_unsubscribe(manager, cmd->app, cmd->driver,
                                        cmd->group);
            notify_monitors(manager, NEU_REQ_UNSUBSCRIBE_GROUP_EVENT, cmd);
        }

        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &error);
        break;
    }
    case NEU_REQ_GET_SUBSCRIBE_GROUP: {
        neu_req_get_subscribe_group_t *cmd =
            (neu_req_get_subscribe_group_t *) &header[1];
        UT_array *groups = neu_manager_get_sub_group_deep_copy(
            manager, cmd->app, cmd->driver, cmd->group);
        neu_resp_get_subscribe_group_t resp = { .groups = groups };

        strcpy(header->receiver, header->sender);
        header->type = NEU_RESP_GET_SUBSCRIBE_GROUP;
        reply(manager, header, &resp);
        break;
    }
    case NEU_REQ_GET_SUB_DRIVER_TAGS: {
        neu_req_get_sub_driver_tags_t *cmd =
            (neu_req_get_sub_driver_tags_t *) &header[1];
        neu_resp_get_sub_driver_tags_t resp = { 0 };
        UT_array *groups = neu_manager_get_sub_group(manager, cmd->app);

        utarray_new(resp.infos, neu_resp_get_sub_driver_tags_info_icd());
        utarray_foreach(groups, neu_resp_subscribe_info_t *, info)
        {
            neu_resp_get_sub_driver_tags_info_t in = { 0 };
            neu_adapter_t *                     driver =
                neu_node_manager_find(manager->node_manager, info->driver);
            assert(driver != NULL);

            strcpy(in.driver, info->driver);
            strcpy(in.group, info->group);
            neu_adapter_driver_get_value_tag((neu_adapter_driver_t *) driver,
                                             info->group, &in.tags);

            utarray_push_back(resp.infos, &in);
        }
        utarray_free(groups);

        strcpy(header->receiver, header->sender);
        header->type = NEU_RESP_GET_SUB_DRIVER_TAGS;
        reply(manager, header, &resp);

        break;
    }
    case NEU_REQ_GET_NODES_STATE: {
        neu_resp_get_nodes_state_t resp = { 0 };
        UT_array *states = neu_node_manager_get_state(manager->node_manager);
        neu_nodes_state_t *p_state = NULL;
        while (
            (p_state = (neu_nodes_state_t *) utarray_next(states, p_state))) {
            p_state->sub_group_count = neu_subscribe_manager_group_count(
                manager->subscribe_manager, p_state->node);
        }
        resp.states     = utarray_clone(states);
        resp.core_level = manager->log_level;

        strcpy(header->receiver, header->sender);
        strcpy(header->sender, "manager");
        header->type = NEU_RESP_GET_NODES_STATE;
        reply(manager, header, &resp);

        utarray_free(states);
        break;
    }
    case NEU_REQ_GET_DRIVER_GROUP: {
        neu_resp_get_driver_group_t resp = { 0 };

        resp.groups = neu_manager_get_driver_group(manager);

        strcpy(header->receiver, header->sender);
        strcpy(header->sender, "manager");
        header->type = NEU_RESP_GET_DRIVER_GROUP;
        reply(manager, header, &resp);

        break;
    }

    case NEU_REQ_GET_GROUP:
    case NEU_REQ_GET_NODE_SETTING:
    case NEU_REQ_READ_GROUP:
    case NEU_REQ_READ_GROUP_PAGINATE:
    case NEU_REQ_WRITE_TAG:
    case NEU_REQ_WRITE_TAGS:
    case NEU_REQ_WRITE_GTAGS:
    case NEU_REQ_GET_TAG:
    case NEU_REQ_NODE_CTL:
    case NEU_REQ_ADD_GROUP: {
        neu_otel_trace_ctx trace = NULL;
        neu_otel_scope_ctx scope = NULL;
        if (neu_otel_control_is_started() &&
            (NEU_REQ_WRITE_TAG == header->type ||
             NEU_REQ_WRITE_TAGS == header->type ||
             NEU_REQ_WRITE_GTAGS == header->type)) {
            trace = neu_otel_find_trace(header->ctx);
            if (trace) {
                scope = neu_otel_add_span(trace);
                if (NEU_REQ_WRITE_TAG == header->type) {
                    neu_otel_scope_set_span_name(scope, "manager write tag");
                } else if (NEU_REQ_WRITE_TAGS == header->type) {
                    neu_otel_scope_set_span_name(scope, "manager write tags");
                } else if (NEU_REQ_WRITE_GTAGS == header->type) {
                    neu_otel_scope_set_span_name(scope, "manager write gtags");
                }
                char new_span_id[36] = { 0 };
                neu_otel_new_span_id(new_span_id);
                neu_otel_scope_set_span_id(scope, new_span_id);
                uint8_t *p_sp_id = neu_otel_scope_get_pre_span_id(scope);
                if (p_sp_id) {
                    neu_otel_scope_set_parent_span_id2(scope, p_sp_id, 8);
                }
                neu_otel_scope_add_span_attr_int(scope, "thread id",
                                                 (int64_t) pthread_self());
                neu_otel_scope_set_span_start_time(scope, neu_time_ns());
            }
        }

        bool re_flag = false;

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            if (NEU_REQ_READ_GROUP == header->type) {
                neu_req_read_group_fini((neu_req_read_group_t *) &header[1]);
            } else if (NEU_REQ_READ_GROUP_PAGINATE == header->type) {
                neu_req_read_group_paginate_fini(
                    (neu_req_read_group_paginate_t *) &header[1]);
            } else if (NEU_REQ_WRITE_TAG == header->type) {
                neu_req_write_tag_fini((neu_req_write_tag_t *) &header[1]);
            } else if (NEU_REQ_WRITE_TAGS == header->type) {
                neu_req_write_tags_fini((neu_req_write_tags_t *) &header[1]);
            } else if (NEU_REQ_WRITE_GTAGS == header->type) {
                neu_req_write_gtags_fini((neu_req_write_gtags_t *) &header[1]);
            }
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
            if (neu_otel_control_is_started() && trace) {
                neu_otel_scope_set_status_code2(scope, NEU_OTEL_STATUS_ERROR,
                                                NEU_ERR_NODE_NOT_EXIST);
            }
        } else {
            forward_msg(manager, header, header->receiver);
            re_flag = true;
        }

        if (neu_otel_control_is_started() && trace) {
            neu_otel_scope_set_span_end_time(scope, neu_time_ns());
            if (!re_flag) {
                neu_otel_trace_set_final(trace);
            }
        }

        break;
    }
    case NEU_REQ_PRGFILE_PROCESS:
    case NEU_REQ_PRGFILE_UPLOAD:
    case NEU_REQ_SCAN_TAGS:
    case NEU_REQ_TEST_READ_TAG:
    case NEU_REQ_GET_NODE_STATE: {
        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        } else {
            forward_msg(manager, header, header->receiver);
        }

        break;
    }

    case NEU_RESP_NODE_RENAME: {
        neu_resp_node_rename_t *resp = (neu_resp_node_rename_t *) &header[1];
        if (0 == resp->error) {
            neu_manager_update_node_name(manager, resp->node, resp->new_name);
            manager_storage_update_node(manager, resp->node, resp->new_name);

            if (neu_node_manager_is_driver(manager->node_manager,
                                           resp->new_name)) {
                UT_array *apps = neu_subscribe_manager_find_by_driver(
                    manager->subscribe_manager, resp->new_name);

                // notify app node about driver renaming
                utarray_foreach(apps, neu_app_subscribe_t *, app)
                {
                    header->type = NEU_REQ_UPDATE_NODE;
                    forward_msg_copy(manager, header, app->app_name);
                }

                utarray_free(apps);
            }
        }

        neu_resp_error_t e = { .error = resp->error };
        header->type       = NEU_RESP_ERROR;
        reply(manager, header, &e);
        break;
    }

    case NEU_RESP_UPDATE_DRIVER_GROUP: {
        neu_resp_update_group_t *resp = (neu_resp_update_group_t *) &header[1];
        if (0 == resp->error) {
            neu_manager_update_group_name(manager, resp->driver, resp->group,
                                          resp->new_name);

            UT_array *apps = neu_subscribe_manager_find(
                manager->subscribe_manager, resp->driver, resp->new_name);

            if (NULL != apps) {
                // notify app node about group renaming
                utarray_foreach(apps, neu_app_subscribe_t *, app)
                {
                    header->type = NEU_REQ_UPDATE_GROUP;
                    forward_msg_copy(manager, header, app->app_name);
                }

                utarray_free(apps);
            }
        }

        neu_resp_error_t e = { .error = resp->error };
        header->type       = NEU_RESP_ERROR;
        reply(manager, header, &e);
        break;
    }

    case NEU_REQ_UPDATE_GROUP: {
        neu_resp_error_t e = { 0 };

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            e.error = NEU_ERR_NODE_NOT_EXIST;
        } else if (!neu_node_manager_is_driver(manager->node_manager,
                                               header->receiver)) {
            e.error = NEU_ERR_GROUP_NOT_ALLOW;
        } else {
            header->type = NEU_REQ_UPDATE_DRIVER_GROUP;
            forward_msg(manager, header, header->receiver);
        }

        if (e.error) {
            header->type = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        }

        break;
    }

    case NEU_REQ_DEL_GROUP: {
        neu_req_del_group_t *cmd = (neu_req_del_group_t *) &header[1];

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        } else {
            UT_array *apps = neu_subscribe_manager_find(
                manager->subscribe_manager, cmd->driver, cmd->group);

            forward_msg(manager, header, header->receiver);
            neu_subscribe_manager_remove(manager->subscribe_manager,
                                         cmd->driver, cmd->group);

            if (NULL == apps) {
                break;
            }

            // notify app node about group deletion
            utarray_foreach(apps, neu_app_subscribe_t *, app)
            {
                forward_msg_copy(manager, header, app->app_name);
            }

            utarray_free(apps);
        }
        break;
    }
    case NEU_REQ_DEL_TAG: {
        neu_req_del_tag_t *cmd = (neu_req_del_tag_t *) &header[1];

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            for (int i = 0; i < cmd->n_tag; i++) {
                free(cmd->tags[i]);
            }
            free(cmd->tags);
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        } else {
            forward_msg(manager, header, header->receiver);
        }

        break;
    }
    case NEU_REQ_UPDATE_TAG:
    case NEU_REQ_ADD_TAG: {
        neu_req_add_tag_t *cmd = (neu_req_add_tag_t *) &header[1];

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            for (int i = 0; i < cmd->n_tag; i++) {
                neu_tag_fini(&cmd->tags[i]);
            }
            free(cmd->tags);
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        } else {
            forward_msg(manager, header, header->receiver);
        }

        break;
    }
    case NEU_REQ_ADD_GTAG: {
        neu_req_add_gtag_t *cmd = (neu_req_add_gtag_t *) &header[1];

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            for (int i = 0; i < cmd->n_group; i++) {
                for (int j = 0; j < cmd->groups[i].n_tag; j++) {
                    neu_tag_fini(&cmd->groups[i].tags[j]);
                }
                free(cmd->groups[i].tags);
            }
            free(cmd->groups);
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        } else {
            forward_msg(manager, header, header->receiver);
        }

        break;
    }
    case NEU_REQ_NODE_SETTING: {
        neu_req_node_setting_t *cmd = (neu_req_node_setting_t *) &header[1];

        if (neu_node_manager_find(manager->node_manager, header->receiver) ==
            NULL) {
            free(cmd->setting);
            neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
            header->type       = NEU_RESP_ERROR;
            neu_msg_exchange(header);
            reply(manager, header, &e);
        } else {
            forward_msg(manager, header, header->receiver);
        }
        break;
    }

    case NEU_RESP_ADD_TAG:
    case NEU_RESP_ADD_GTAG:
    case NEU_RESP_UPDATE_TAG:
    case NEU_RESP_GET_TAG:
    case NEU_RESP_GET_GROUP:
    case NEU_RESP_GET_NODE_SETTING:
    case NEU_RESP_ERROR:
    case NEU_RESP_READ_GROUP:
    case NEU_RESP_READ_GROUP_PAGINATE:
    case NEU_RESP_TEST_READ_TAG:
    case NEU_RESP_PRGFILE_PROCESS:
    case NEU_RESP_SCAN_TAGS:
        forward_msg(manager, header, header->receiver);
        break;

    case NEU_RESP_GET_NODE_STATE: {
        neu_resp_get_node_state_t *resp =
            (neu_resp_get_node_state_t *) &header[1];

        neu_adapter_t *adapter_node =
            neu_node_manager_find(manager->node_manager, header->sender);
        resp->sub_group_count = neu_subscribe_manager_group_count(
            manager->subscribe_manager, header->sender);
        resp->is_driver =
            (adapter_node->module->type == NEU_NA_TYPE_DRIVER) ? true : false;
        resp->core_level = manager->log_level;

        forward_msg(manager, header, header->receiver);
        break;
    }

    case NEU_REQ_ADD_DRIVERS: {
        neu_req_driver_array_t *cmd = (neu_req_driver_array_t *) &header[1];
        neu_resp_error_t        e   = { 0 };

        e.error = neu_manager_add_drivers(manager, cmd);
        if (NEU_ERR_SUCCESS == e.error) {
            for (uint16_t i = 0; i < cmd->n_driver; ++i) {
                neu_req_driver_t *driver = &cmd->drivers[i];
                manager_storage_add_node(manager, driver->node);
                adapter_storage_setting(driver->node, driver->setting);
                for (uint16_t j = 0; j < driver->n_group; j++) {
                    adapter_storage_add_group(driver->node,
                                              driver->groups[j].group,
                                              driver->groups[j].interval);
                    adapter_storage_add_tags(
                        driver->node, driver->groups[j].group,
                        driver->groups[j].tags, driver->groups[j].n_tag);
                }
            }
        }

        neu_req_driver_array_fini(cmd);
        header->type = NEU_RESP_ERROR;
        strcpy(header->receiver, header->sender);
        reply(manager, header, &e);
        break;
    }

    case NEU_REQ_UPDATE_LOG_LEVEL: {
        neu_req_update_log_level_t *cmd =
            (neu_req_update_log_level_t *) &header[1];

        if (cmd->core) {
            manager->log_level = cmd->log_level;
            nlog_level_change(manager->log_level);
        }

        if (strlen(cmd->node) > 0) {
            if (neu_node_manager_find(manager->node_manager,
                                      header->receiver) == NULL) {
                neu_resp_error_t e = { .error = NEU_ERR_NODE_NOT_EXIST };
                header->type       = NEU_RESP_ERROR;
                neu_msg_exchange(header);
                reply(manager, header, &e);
            } else {
                forward_msg(manager, header, header->receiver);
            }
        } else {
            if (cmd->core) {
                neu_resp_error_t e = { .error = NEU_ERR_SUCCESS };
                header->type       = NEU_RESP_ERROR;
                neu_msg_exchange(header);
                reply(manager, header, &e);
            } else {
                neu_resp_error_t e = { .error = NEU_ERR_PARAM_IS_WRONG };
                header->type       = NEU_RESP_ERROR;
                neu_msg_exchange(header);
                reply(manager, header, &e);
            }
        }

        break;
    }
    case NEU_REQ_ADD_NODE_EVENT:
    case NEU_REQ_DEL_NODE_EVENT:
    case NEU_REQ_NODE_CTL_EVENT:
    case NEU_REQ_NODE_SETTING_EVENT:
    case NEU_REQ_ADD_GROUP_EVENT:
    case NEU_REQ_DEL_GROUP_EVENT:
    case NEU_REQ_UPDATE_GROUP_EVENT:
    case NEU_REQ_ADD_TAG_EVENT:
    case NEU_REQ_DEL_TAG_EVENT:
    case NEU_REQ_UPDATE_TAG_EVENT: 
    case NEU_REQ_ADD_GTAG_EVENT:
    case NEU_REQ_ADD_PLUGIN_EVENT:
    case NEU_REQ_DEL_PLUGIN_EVENT:
    case NEU_REQ_SUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_UNSUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_SUBSCRIBE_GROUPS_EVENT: {
        notify_monitors(manager, header->type, &header[1]);
        neu_msg_free(msg);
        break;
    }
    default:
        assert(false);
        break;
    }

    return 0;
}

inline static void forward_msg_copy(neu_manager_t *     manager,
                                    neu_reqresp_head_t *header,
                                    const char *        node)
{
    neu_msg_t *msg = neu_msg_copy((neu_msg_t *) header);
    forward_msg(manager, neu_msg_get_header(msg), node);
}

static void start_static_adapter(neu_manager_t *manager, const char *name)
{
    neu_adapter_t *       adapter      = NULL;
    neu_plugin_instance_t instance     = { 0 };
    neu_adapter_info_t    adapter_info = {
        .name = name,
    };

    neu_plugin_manager_load_static(manager->plugin_manager, name, &instance);
    adapter_info.handle = instance.handle;
    adapter_info.module = instance.module;

    adapter = neu_adapter_create(&adapter_info, true);
    neu_node_manager_add_static(manager->node_manager, adapter);
    neu_adapter_init(adapter, false);
    neu_adapter_start(adapter);
}

static void start_single_adapter(neu_manager_t *manager, const char *name,
                                 const char *plugin_name, bool display)
{
    neu_adapter_t *       adapter      = NULL;
    neu_plugin_instance_t instance     = { 0 };
    neu_adapter_info_t    adapter_info = {
        .name = name,
    };

    if (0 !=
        neu_plugin_manager_create_instance(manager->plugin_manager, plugin_name,
                                           &instance)) {
        return;
    }

    adapter_info.handle = instance.handle;
    adapter_info.module = instance.module;
    adapter             = neu_adapter_create(&adapter_info, true);

    neu_node_manager_add_single(manager->node_manager, adapter, display);
    if (display) {
        manager_storage_add_node(manager, name);
    }

    neu_adapter_init(adapter, false);
    neu_adapter_start_single(adapter);
}

inline static void reply(neu_manager_t *manager, neu_reqresp_head_t *header,
                         void *data)
{
    neu_msg_gen(header, data);
    struct sockaddr_un addr =
        neu_node_manager_get_addr(manager->node_manager, header->receiver);

    neu_reqresp_type_e t                           = header->type;
    void *             ctx                         = header->ctx;
    char               receiver[NEU_NODE_NAME_LEN] = { 0 };
    strncpy(receiver, header->receiver, sizeof(receiver));

    neu_msg_t *msg = (neu_msg_t *) header;
    int        ret = neu_send_msg_to(manager->server_fd, &addr, msg);
    if (0 == ret) {
        nlog_notice("reply %s to %s(%s) %p", neu_reqresp_type_string(t),
                    receiver, &addr.sun_path[1], ctx);
    } else {
        nlog_warn("reply %s to %s, error: %d", neu_reqresp_type_string(t),
                  receiver, ret);
        neu_msg_free(msg);
    }
}

static int update_timestamp(void *usr_data)
{
    (void) usr_data;
    global_timestamp = neu_time_ms();
    return 0;
}

static char *file_save_tmp(const char *data, const char *suffix)
{
    int   d_len = 0;
    char *file  = (char *) neu_decode64(&d_len, data);

    if (file == NULL) {
        nlog_warn("library so file decode64 ret is NULL");
        return NULL;
    }

    if (d_len == 0) {
        nlog_warn("library so file decode64 len is 0");
        free(file);
        return NULL;
    }

    char *tmp_path = neu_persister_save_file_tmp(file, d_len, suffix);

    if (tmp_path == NULL) {
        free(file);
        return NULL;
    }

    free(file);

    return tmp_path;
}

static int copy_file(const char *src, const char *dst)
{
    FILE * source, *destination;
    char   buffer[1024];
    size_t bytes_read;

    source = fopen(src, "rb");
    if (source == NULL) {
        nlog_error("failed to open source file!");
        return -1;
    }

    destination = fopen(dst, "wb");
    if (destination == NULL) {
        nlog_error("failed to open destination file!");
        fclose(source);
        return -1;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        if (fwrite(buffer, 1, bytes_read, destination) != bytes_read) {
            nlog_error("failed to write to destination file!");
            fclose(source);
            fclose(destination);
            return -1;
        }
    }

    if (ferror(source)) {
        nlog_error("error reading source file");
        fclose(source);
        fclose(destination);
        return -1;
    }

    fclose(source);
    fclose(destination);

    return 0;
}

static bool mv_tmp_library_file(neu_plugin_kind_e kind, const char *tmp_path,
                                const char *library)
{
    char file_name[128] = { 0 };
    char path_name[128] = { 0 };
    if (kind == NEU_PLUGIN_KIND_CUSTOM) {
        snprintf(file_name, sizeof(file_name), "%s/custom/%s", g_plugin_dir,
                 library);
        snprintf(path_name, sizeof(path_name), "%s/custom", g_plugin_dir);
    } else if (kind == NEU_PLUGIN_KIND_SYSTEM) {
        snprintf(file_name, sizeof(file_name), "%s/system/%s", g_plugin_dir,
                 library);
        snprintf(path_name, sizeof(path_name), "%s/system", g_plugin_dir);
    } else {
        return false;
    }

    struct stat st;

    if (stat(path_name, &st) == -1) {
        if (mkdir(path_name, 0700) == -1) {
            nlog_error("%s mkdir fail", path_name);
            return false;
        }
    }

    if (copy_file(tmp_path, file_name) != 0) {
        nlog_error("%s copy_file %s fail!", tmp_path, file_name);
        return false;
    } else {
        return true;
    }

    return false;
}

static bool mv_tmp_schema_file(neu_plugin_kind_e kind, const char *tmp_path,
                               const char *schema)
{

    char file_name[128] = { 0 };
    char path_name[128] = { 0 };
    if (kind == NEU_PLUGIN_KIND_CUSTOM) {
        snprintf(file_name, sizeof(file_name), "%s/custom/schema/%s.json",
                 g_plugin_dir, schema);
        snprintf(path_name, sizeof(path_name), "%s/custom/schema",
                 g_plugin_dir);
    } else if (kind == NEU_PLUGIN_KIND_SYSTEM) {
        snprintf(file_name, sizeof(file_name), "%s/system/schema/%s.json",
                 g_plugin_dir, schema);
        snprintf(path_name, sizeof(path_name), "%s/system/schema",
                 g_plugin_dir);
    } else {
        return false;
    }

    struct stat st;

    if (stat(path_name, &st) == -1) {
        if (mkdir(path_name, 0700) == -1) {
            nlog_error("%s mkdir fail", path_name);
            return false;
        }
    }

    if (copy_file(tmp_path, file_name) != 0) {
        nlog_error("%s copy_file %s fail!", tmp_path, file_name);
        return false;
    } else {
        return true;
    }

    return false;
}

struct notify_monitor_ctx {
    neu_manager_t *    manager;
    neu_reqresp_type_e event;
    void *             data;
};

static int notify_monitor(const char *name, struct sockaddr_un addr, void *arg)
{
    struct notify_monitor_ctx *ctx     = arg;
    // monitor not ready, ignore
    if (0 == addr.sun_path) {
        return 0;
    }
    // message header
    neu_msg_t *msg = neu_msg_new(ctx->event, NULL, ctx->data);
    if (NULL == msg) {
        return -1;
    }
    neu_reqresp_head_t *header = neu_msg_get_header(msg);
    strcpy(header->sender, "manager");
    strcpy(header->receiver, name);

    int ret = neu_send_msg_to(ctx->manager->server_fd, &addr, msg);
    if (0 != ret) {
        nlog_warn("notify %s of %s, error: %s(%d)", header->receiver,
                  neu_reqresp_type_string(header->type), strerror(errno),
                  errno);
        neu_msg_free(msg);
    }
    return 0;
}
inline static void notify_monitors(neu_manager_t *    manager,
                                   neu_reqresp_type_e event, void *data)
{
    struct notify_monitor_ctx ctx = {
        .manager = manager,
        .event   = event,
        .data    = data,
    };
    neu_node_manager_for_each_monitor(manager->node_manager, notify_monitor,
                                      &ctx);
}