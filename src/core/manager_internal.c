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
#include <dlfcn.h>

#include "utils/log.h"
#include "json/neu_json_param.h"

#include "adapter.h"
#include "errcodes.h"

#include "adapter/adapter_internal.h"
#include "adapter/driver/driver_internal.h"
#include "base/msg_internal.h"

#include "manager_internal.h"
#include "storage.h"

int neu_manager_add_plugin(neu_manager_t *manager, const char *library)
{
    return neu_plugin_manager_add(manager->plugin_manager, library);
}

int neu_manager_del_plugin(neu_manager_t *manager, const char *plugin)
{
    return neu_plugin_manager_del(manager->plugin_manager, plugin);
}

UT_array *neu_manager_get_plugins(neu_manager_t *manager)
{
    return neu_plugin_manager_get(manager->plugin_manager);
}

int neu_manager_add_node(neu_manager_t *manager, const char *node_name,
                         const char *plugin_name, const char *setting,
                         neu_node_running_state_e state, bool load)
{
    neu_adapter_t *       adapter      = NULL;
    neu_plugin_instance_t instance     = { 0 };
    neu_adapter_info_t    adapter_info = {
        .name = node_name,
    };
    neu_resp_plugin_info_t info = { 0 };
    int                    ret =
        neu_plugin_manager_find(manager->plugin_manager, plugin_name, &info);

    if (ret != 0) {
        return NEU_ERR_LIBRARY_NOT_FOUND;
    }

    if (info.single) {
        return NEU_ERR_LIBRARY_NOT_ALLOW_CREATE_INSTANCE;
    }

    adapter = neu_node_manager_find(manager->node_manager, node_name);
    if (adapter != NULL) {
        return NEU_ERR_NODE_EXIST;
    }

    ret = neu_plugin_manager_create_instance(manager->plugin_manager, info.name,
                                             &instance);
    if (ret != 0) {
        return NEU_ERR_LIBRARY_FAILED_TO_OPEN;
    }
    adapter_info.handle = instance.handle;
    adapter_info.module = instance.module;

    adapter = neu_adapter_create(&adapter_info, load);
    if (adapter == NULL) {
        return neu_adapter_error();
    }
    neu_node_manager_add(manager->node_manager, adapter);
    neu_adapter_init(adapter, state);

    if (NULL != setting &&
        0 != (ret = neu_adapter_set_setting(adapter, setting))) {
        neu_node_manager_del(manager->node_manager, node_name);
        neu_adapter_uninit(adapter);
        neu_adapter_destroy(adapter);
        return ret;
    }

    return NEU_ERR_SUCCESS;
}

int neu_manager_del_node(neu_manager_t *manager, const char *node_name)
{
    neu_adapter_t *adapter =
        neu_node_manager_find(manager->node_manager, node_name);

    if (adapter == NULL) {
        return NEU_ERR_NODE_NOT_EXIST;
    }

    neu_adapter_destroy(adapter);
    neu_subscribe_manager_remove(manager->subscribe_manager, node_name, NULL);
    neu_node_manager_del(manager->node_manager, node_name);
    return NEU_ERR_SUCCESS;
}

UT_array *neu_manager_get_nodes(neu_manager_t *manager, int type,
                                const char *plugin, const char *node)
{
    return neu_node_manager_filter(manager->node_manager, type, plugin, node);
}

int neu_manager_update_node_name(neu_manager_t *manager, const char *node,
                                 const char *new_name)
{
    int ret = 0;
    if (neu_node_manager_is_driver(manager->node_manager, node)) {
        ret = neu_subscribe_manager_update_driver_name(
            manager->subscribe_manager, node, new_name);
    } else {
        ret = neu_subscribe_manager_update_app_name(manager->subscribe_manager,
                                                    node, new_name);
    }
    if (0 == ret) {
        ret =
            neu_node_manager_update_name(manager->node_manager, node, new_name);
    }
    return ret;
}

int neu_manager_update_group_name(neu_manager_t *manager, const char *driver,
                                  const char *group, const char *new_name)
{
    return neu_subscribe_manager_update_group_name(manager->subscribe_manager,
                                                   driver, group, new_name);
}

static inline neu_plugin_instance_t *
new_plugin_instance(neu_plugin_manager_t *plugin_manager, const char *plugin)
{
    neu_plugin_instance_t *inst = calloc(1, sizeof(*inst));
    if (NULL == inst) {
        return NULL;
    }

    if (0 != neu_plugin_manager_create_instance(plugin_manager, plugin, inst)) {
        free(inst);
        return NULL;
    }

    return inst;
}

static inline void free_plugin_instance(neu_plugin_instance_t *inst)
{
    if (inst) {
        dlclose(inst->handle);
        free(inst);
    }
}

UT_array *neu_manager_get_driver_group(neu_manager_t *manager)
{
    UT_array *drivers =
        neu_node_manager_get(manager->node_manager, NEU_NA_TYPE_DRIVER);
    UT_array *driver_groups = NULL;
    UT_icd    icd = { sizeof(neu_resp_driver_group_info_t), NULL, NULL, NULL };

    utarray_new(driver_groups, &icd);

    utarray_foreach(drivers, neu_resp_node_info_t *, driver)
    {
        neu_adapter_t *adapter =
            neu_node_manager_find(manager->node_manager, driver->node);
        UT_array *groups =
            neu_adapter_driver_get_group((neu_adapter_driver_t *) adapter);

        utarray_foreach(groups, neu_resp_group_info_t *, g)
        {
            neu_resp_driver_group_info_t dg = { 0 };

            strcpy(dg.driver, driver->node);
            strcpy(dg.group, g->name);
            dg.interval  = g->interval;
            dg.tag_count = g->tag_count;

            utarray_push_back(driver_groups, &dg);
        }

        utarray_free(groups);
    }

    utarray_free(drivers);

    return driver_groups;
}

static inline int manager_subscribe(neu_manager_t *manager, const char *app,
                                    const char *driver, const char *group,
                                    const char *params)
{
    int                ret  = NEU_ERR_SUCCESS;
    struct sockaddr_un addr = { 0 };
    neu_adapter_t *    adapter =
        neu_node_manager_find(manager->node_manager, driver);

    if (adapter == NULL) {
        return NEU_ERR_NODE_NOT_EXIST;
    }

    ret =
        neu_adapter_driver_group_exist((neu_adapter_driver_t *) adapter, group);
    if (ret != NEU_ERR_SUCCESS) {
        return ret;
    }

    addr = neu_node_manager_get_addr(manager->node_manager, app);
    return neu_subscribe_manager_sub(manager->subscribe_manager, driver, app,
                                     group, params, addr);
}

int neu_manager_subscribe(neu_manager_t *manager, const char *app,
                          const char *driver, const char *group,
                          const char *params, uint16_t *app_port)
{
    if (neu_node_manager_is_monitor(manager->node_manager, app)) {
        return NEU_ERR_NODE_NOT_ALLOW_SUBSCRIBE;
    }

    neu_adapter_t *adapter = neu_node_manager_find(manager->node_manager, app);

    if (adapter == NULL) {
        return NEU_ERR_NODE_NOT_EXIST;
    }

    *app_port = neu_adapter_trans_data_port(adapter);

    // guard against empty mqtt topic parameter
    // this is not an elegant solution due to the current architecture
    if (params && 0 == strcmp(adapter->module->module_name, "MQTT")) {
        neu_json_elem_t elem = { .name      = "topic",
                                 .t         = NEU_JSON_STR,
                                 .v.val_str = NULL };
        int             ret  = neu_parse_param(params, NULL, 1, &elem);
        if (ret != 0 || (elem.v.val_str && 0 == strlen(elem.v.val_str))) {
            free(elem.v.val_str);
            return NEU_ERR_MQTT_SUBSCRIBE_FAILURE;
        }
        free(elem.v.val_str);
    }

    if (NEU_NA_TYPE_APP != neu_adapter_get_type(adapter)) {
        return NEU_ERR_NODE_NOT_ALLOW_SUBSCRIBE;
    }

    return manager_subscribe(manager, app, driver, group, params);
}

int neu_manager_update_subscribe(neu_manager_t *manager, const char *app,
                                 const char *driver, const char *group,
                                 const char *params)
{
    return neu_subscribe_manager_update_params(manager->subscribe_manager, app,
                                               driver, group, params);
}

int neu_manager_send_subscribe(neu_manager_t *manager, const char *app,
                               const char *driver, const char *group,
                               uint16_t app_port, const char *params)
{
    neu_req_subscribe_t cmd = { 0 };
    strcpy(cmd.app, app);
    strcpy(cmd.driver, driver);
    strcpy(cmd.group, group);
    cmd.port = app_port;

    if (params && NULL == (cmd.params = strdup(params))) {
        return NEU_ERR_EINTERNAL;
    }

    neu_msg_t *msg = neu_msg_new(NEU_REQ_SUBSCRIBE_GROUP, NULL, &cmd);
    if (NULL == msg) {
        free(cmd.params);
        return NEU_ERR_EINTERNAL;
    }
    neu_reqresp_head_t *header = neu_msg_get_header(msg);
    strcpy(header->sender, "manager");
    strcpy(header->receiver, app);

    struct sockaddr_un addr =
        neu_node_manager_get_addr(manager->node_manager, app);

    int ret = neu_send_msg_to(manager->server_fd, &addr, msg);
    if (0 != ret) {
        nlog_warn("send %s to %s app failed",
                  neu_reqresp_type_string(NEU_REQ_SUBSCRIBE_GROUP), app);
        free(cmd.params);
        neu_msg_free(msg);
    } else {
        nlog_notice("send %s to %s app",
                    neu_reqresp_type_string(NEU_REQ_SUBSCRIBE_GROUP), app);
    }
    cmd.params = NULL;

    msg = neu_msg_new(NEU_REQ_SUBSCRIBE_GROUP, NULL, &cmd);
    if (NULL == msg) {
        return NEU_ERR_EINTERNAL;
    }
    header = neu_msg_get_header(msg);
    strcpy(header->sender, "manager");
    strcpy(header->receiver, driver);
    addr = neu_node_manager_get_addr(manager->node_manager, driver);

    ret = neu_send_msg_to(manager->server_fd, &addr, msg);
    if (0 != ret) {
        nlog_warn("send %s to %s driver failed",
                  neu_reqresp_type_string(NEU_REQ_SUBSCRIBE_GROUP), driver);
        neu_msg_free(msg);
    } else {
        nlog_notice("send %s to %s driver",
                    neu_reqresp_type_string(NEU_REQ_SUBSCRIBE_GROUP), driver);
    }

    return 0;
}

int neu_manager_unsubscribe(neu_manager_t *manager, const char *app,
                            const char *driver, const char *group)
{
    return neu_subscribe_manager_unsub(manager->subscribe_manager, driver, app,
                                       group);
}

UT_array *neu_manager_get_sub_group(neu_manager_t *manager, const char *app)
{
    return neu_subscribe_manager_get(manager->subscribe_manager, app, NULL,
                                     NULL);
}

UT_array *neu_manager_get_sub_group_deep_copy(neu_manager_t *manager,
                                              const char *   app,
                                              const char *   driver,
                                              const char *   group)
{
    UT_array *subs = neu_subscribe_manager_get(manager->subscribe_manager, app,
                                               driver, group);

    utarray_foreach(subs, neu_resp_subscribe_info_t *, sub)
    {
        if (sub->params) {
            sub->params = strdup(sub->params);
        }
    }

    // set vector element destructor
    subs->icd.dtor = (void (*)(void *)) neu_resp_subscribe_info_fini;

    return subs;
}

int neu_manager_get_node_info(neu_manager_t *manager, const char *name,
                              neu_persist_node_info_t *info)
{
    neu_adapter_t *adapter = neu_node_manager_find(manager->node_manager, name);

    if (adapter != NULL) {
        info->name        = strdup(name);
        info->type        = adapter->module->type;
        info->plugin_name = strdup(adapter->module->module_name);
        info->state       = adapter->state;
        return 0;
    }

    return -1;
}

static int del_node(neu_manager_t *manager, const char *node)
{
    neu_adapter_t *adapter = neu_node_manager_find(manager->node_manager, node);
    if (NULL == adapter) {
        return 0;
    }

    if (neu_node_manager_is_single(manager->node_manager, node)) {
        return NEU_ERR_NODE_NOT_ALLOW_DELETE;
    }

    if (neu_adapter_get_type(adapter) == NEU_NA_TYPE_APP) {
        UT_array *subscriptions = neu_subscribe_manager_get(
            manager->subscribe_manager, node, NULL, NULL);
        neu_subscribe_manager_unsub_all(manager->subscribe_manager, node);

        utarray_foreach(subscriptions, neu_resp_subscribe_info_t *, sub)
        {
            // NOTE: neu_req_unsubscribe_t and neu_resp_subscribe_info_t
            //       have compatible memory layout
            neu_msg_t *msg = neu_msg_new(NEU_REQ_UNSUBSCRIBE_GROUP, NULL, sub);
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

    if (neu_adapter_get_type(adapter) == NEU_NA_TYPE_DRIVER) {
        neu_reqresp_node_deleted_t resp = { 0 };
        strcpy(resp.node, node);

        UT_array *apps = neu_subscribe_manager_find_by_driver(
            manager->subscribe_manager, node);
        utarray_foreach(apps, neu_app_subscribe_t *, app)
        {
            neu_msg_t *msg = neu_msg_new(NEU_REQRESP_NODE_DELETED, NULL, &resp);
            if (NULL == msg) {
                break;
            }
            neu_reqresp_head_t *hd = neu_msg_get_header(msg);
            strcpy(hd->receiver, app->app_name);
            strcpy(hd->sender, "manager");
            forward_msg(manager, hd, hd->receiver);
        }
        utarray_free(apps);
    }

    neu_adapter_uninit(adapter);
    neu_manager_del_node(manager, node);
    manager_storage_del_node(manager, node);
    return 0;
}

static inline int add_driver(neu_manager_t *manager, neu_req_driver_t *driver)
{
    int ret = del_node(manager, driver->node);
    if (0 != ret) {
        return ret;
    }

    ret = neu_manager_add_node(manager, driver->node, driver->plugin,
                               driver->setting, false, false);
    if (0 != ret) {
        return ret;
    }

    neu_adapter_t *adapter =
        neu_node_manager_find(manager->node_manager, driver->node);

    neu_resp_add_tag_t resp = { 0 };
    neu_req_add_gtag_t cmd  = {
        .groups  = driver->groups,
        .n_group = driver->n_group,
    };

    if (0 != neu_adapter_validate_gtags(adapter, &cmd, &resp) ||
        0 != neu_adapter_try_add_gtags(adapter, &cmd, &resp) ||
        0 != neu_adapter_add_gtags(adapter, &cmd, &resp)) {
        neu_adapter_uninit(adapter);
        neu_manager_del_node(manager, driver->node);
    }

    return resp.error;
}

int neu_manager_add_drivers(neu_manager_t *manager, neu_req_driver_array_t *req)
{
    int ret = 0;

    // fast check
    for (uint16_t i = 0; i < req->n_driver; ++i) {
        neu_resp_plugin_info_t info   = { 0 };
        neu_req_driver_t *     driver = &req->drivers[i];

        ret = neu_plugin_manager_find(manager->plugin_manager, driver->plugin,
                                      &info);

        if (ret != 0) {
            return NEU_ERR_LIBRARY_NOT_FOUND;
        }

        if (info.single) {
            return NEU_ERR_LIBRARY_NOT_ALLOW_CREATE_INSTANCE;
        }

        if (NEU_NA_TYPE_DRIVER != info.type) {
            return NEU_ERR_PLUGIN_TYPE_NOT_SUPPORT;
        }

        if (driver->n_group > NEU_GROUP_MAX_PER_NODE) {
            return NEU_ERR_GROUP_MAX_GROUPS;
        }
    }

    for (uint16_t i = 0; i < req->n_driver; ++i) {
        ret = add_driver(manager, &req->drivers[i]);
        if (0 != ret) {
            nlog_notice("add i:%" PRIu16 " driver:%s fail", i,
                        req->drivers[i].node);
            while (i-- > 0) {
                nlog_notice("rollback i:%" PRIu16 " driver:%s", i,
                            req->drivers[i].node);
                neu_adapter_t *adapter = neu_node_manager_find(
                    manager->node_manager, req->drivers[i].node);
                neu_adapter_uninit(adapter);
                neu_manager_del_node(manager, req->drivers[i].node);
            }
            nlog_error("fail to add %" PRIu16 " drivers", req->n_driver);
            break;
        }
        nlog_notice("add i:%" PRIu16 " driver:%s success", i,
                    req->drivers[i].node);
    }

    return ret;
}
