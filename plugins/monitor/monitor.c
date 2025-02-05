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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errcodes.h"
#include "event/event.h"
#include "monitor.h"
#include "mqtt_handle.h"
#include "utils/http_handler.h"
#include "utils/log.h"

extern const neu_plugin_module_t neu_plugin_module;

static struct neu_plugin *g_monitor_plugin_;

static void connect_cb(void *data)
{
    neu_plugin_t *plugin      = data;
    plugin->common.link_state = NEU_NODE_LINK_STATE_CONNECTED;
    plog_notice(plugin, "plugin `%s` connected", neu_plugin_module.module_name);
}

static void disconnect_cb(void *data)
{
    neu_plugin_t *plugin      = data;
    plugin->common.link_state = NEU_NODE_LINK_STATE_DISCONNECTED;
    plog_notice(plugin, "plugin `%s` disconnected",
                neu_plugin_module.module_name);
}

static int config_mqtt_client(neu_plugin_t *plugin, neu_mqtt_client_t *client,
                              const monitor_config_t *config)
{
    int rv = 0;

    if (NULL == client) {
        return 0;
    }

    // set log category as soon as possible to ease debugging
    rv = neu_mqtt_client_set_zlog_category(client, plugin->common.log);
    if (0 != rv) {
        plog_error(plugin, "neu_mqtt_client_set_zlog_category fail");
        return -1;
    }

    rv = neu_mqtt_client_set_addr(client, config->host, config->port);
    if (0 != rv) {
        plog_error(plugin, "neu_mqtt_client_set_host fail");
        return -1;
    }

    rv = neu_mqtt_client_set_id(client, config->client_id);
    if (0 != rv) {
        plog_error(plugin, "neu_mqtt_client_set_id fail");
        return -1;
    }

    rv = neu_mqtt_client_set_connect_cb(client, connect_cb, plugin);
    if (0 != rv) {
        plog_error(plugin, "neu_mqtt_client_set_connect_cb fail");
        return -1;
    }

    rv = neu_mqtt_client_set_disconnect_cb(client, disconnect_cb, plugin);
    if (0 != rv) {
        plog_error(plugin, "neu_mqtt_client_set_disconnect_cb fail");
        return -1;
    }

    if (NULL != config->username) {
        rv = neu_mqtt_client_set_user(client, config->username,
                                      config->password);
        if (0 != rv) {
            plog_error(plugin, "neu_mqtt_client_set_user fail");
        }
    }

    rv = neu_mqtt_client_set_tls(client, config->ssl, config->ca, config->cert,
                                 config->key, config->keypass);
    if (0 != rv) {
        plog_error(plugin, "neu_mqtt_client_set_tsl fail");
        return -1;
    }

    return rv;
}

static int heartbeat_timer_cb(void *data)
{
    neu_plugin_t *plugin = data;

    neu_reqresp_head_t header = {
        .type = NEU_REQ_GET_NODES_STATE,
        .ctx  = plugin,
    };
    neu_req_get_nodes_state_t cmd = {};

    return neu_plugin_op(plugin, header, &cmd);
}

static inline void stop_heartbeart_timer(neu_plugin_t *plugin)
{
    if (plugin->heartbeat_timer) {
        neu_event_del_timer(plugin->events, plugin->heartbeat_timer);
        plugin->heartbeat_timer = NULL;
        plog_notice(plugin, "heartbeat timer stopped");
    }
}

static int start_hearbeat_timer(neu_plugin_t *plugin, uint64_t interval)
{
    neu_event_timer_t *timer = NULL;

    if (0 == interval) {
        plog_info(plugin, "heartbeat disabled");
        goto end;
    }

    if (NULL == plugin->events) {
        plugin->events = neu_event_new();
        if (NULL == plugin->events) {
            plog_error(plugin, "neu_event_new fail");
            return NEU_ERR_EINTERNAL;
        }
    }

    neu_event_timer_param_t param = {
        .second      = interval,
        .millisecond = 0,
        .cb          = heartbeat_timer_cb,
        .usr_data    = plugin,
    };

    timer = neu_event_add_timer(plugin->events, param);
    if (NULL == timer) {
        plog_error(plugin, "neu_event_add_timer fail");
        return NEU_ERR_EINTERNAL;
    }

end:
    if (plugin->heartbeat_timer) {
        neu_event_del_timer(plugin->events, plugin->heartbeat_timer);
    }
    plugin->heartbeat_timer = timer;

    plog_notice(plugin, "start_hearbeat_timer interval: %" PRIu64, interval);

    return 0;
}

static void uninit_timers_and_mqtt(neu_plugin_t *plugin)
{
    stop_heartbeart_timer(plugin);

    if (NULL != plugin->events) {
        neu_event_close(plugin->events);
        plugin->events = NULL;
    }

    if (NULL != plugin->config) {
        monitor_config_fini(plugin->config);
        free(plugin->config);
        plugin->config = NULL;
    }

    if (NULL != plugin->mqtt_client) {
        neu_mqtt_client_close(plugin->mqtt_client);
        neu_mqtt_client_free(plugin->mqtt_client);
        plugin->mqtt_client = NULL;
    }
}

static neu_plugin_t *monitor_plugin_open(void)
{
    neu_plugin_t *plugin = calloc(1, sizeof(neu_plugin_t));
    if (NULL == plugin) {
        return NULL;
    }

    neu_plugin_common_init(&plugin->common);

    g_monitor_plugin_ = plugin;
    return plugin;
}

static int monitor_plugin_close(neu_plugin_t *plugin)
{
    int rv = 0;

    plog_notice(plugin, "Success to free plugin: %s",
                neu_plugin_module.module_name);
    free(plugin);

    return rv;
}

static int monitor_plugin_init(neu_plugin_t *plugin, bool load)
{
    int rv = 0;
    (void) plugin;
    (void) load;

    plog_notice(plugin, "Initialize plugin: %s", neu_plugin_module.module_name);
    return rv;
}

static int monitor_plugin_uninit(neu_plugin_t *plugin)
{
    int rv = 0;

    uninit_timers_and_mqtt(plugin);

    plog_notice(plugin, "Uninitialize plugin: %s",
                neu_plugin_module.module_name);
    return rv;
}

static int monitor_plugin_config(neu_plugin_t *plugin, const char *setting)
{
    int              rv          = 0;
    const char *     plugin_name = neu_plugin_module.module_name;
    monitor_config_t config      = { 0 };

    rv = monitor_config_parse(plugin, setting, &config);
    if (0 != rv) {
        plog_error(plugin, "monitor_config_parse fail");
        return NEU_ERR_NODE_SETTING_INVALID;
    }

    if (NULL == plugin->config) {
        plugin->config = calloc(1, sizeof(*plugin->config));
        if (NULL == plugin->config) {
            plog_error(plugin, "calloc monitor config fail");
            rv = NEU_ERR_EINTERNAL;
            goto error;
        }
    }

    if (NULL == plugin->mqtt_client) {
        plugin->mqtt_client = neu_mqtt_client_new(NEU_MQTT_VERSION_V311);
        if (NULL == plugin->mqtt_client) {
            plog_error(plugin, "neu_mqtt_client_new fail");
            rv = NEU_ERR_EINTERNAL;
            goto error;
        }
    } else if (neu_mqtt_client_is_open(plugin->mqtt_client)) {
        rv = neu_mqtt_client_close(plugin->mqtt_client);
        if (0 != rv) {
            plog_error(plugin, "neu_mqtt_client_close fail");
            rv = NEU_ERR_EINTERNAL;
            goto error;
        }
    }

    rv = config_mqtt_client(plugin, plugin->mqtt_client, &config);
    if (0 != rv) {
        rv = NEU_ERR_MQTT_INIT_FAILURE;
        goto error;
    }

    if (plugin->started && 0 != neu_mqtt_client_open(plugin->mqtt_client)) {
        plog_error(plugin, "neu_mqtt_client_open fail");
        rv = NEU_ERR_MQTT_CONNECT_FAILURE;
        goto error;
    }

    if (plugin->started &&
        0 != start_hearbeat_timer(plugin, config.heartbeat_interval)) {
        plog_error(plugin, "start_hearbeat_timer fail");
        rv = NEU_ERR_EINTERNAL;
        goto error;
    }

    monitor_config_fini(plugin->config);
    memmove(plugin->config, &config, sizeof(config));
    // `config` moved, do not call monitor_config_fini

    plog_notice(plugin, "config plugin `%s` success", plugin_name);
    return 0;

error:
    plog_error(plugin, "config plugin `%s` fail", plugin_name);
    monitor_config_fini(&config);
    return rv;
}

static int monitor_plugin_request(neu_plugin_t *      plugin,
                                  neu_reqresp_head_t *header, void *data)
{
    (void) plugin;

    switch (header->type) {
    case NEU_RESP_ERROR: {
        neu_resp_error_t *resp_err = (neu_resp_error_t *) data;
        nlog_warn("recv error code: %d", resp_err->error);
        break;
    }
    case NEU_RESP_GET_NODES_STATE: {
        handle_nodes_state(header->ctx, (neu_resp_get_nodes_state_t *) data);
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
    case NEU_REQ_UNSUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_SUBSCRIBE_GROUPS_EVENT: {
        handle_events(plugin, header->type, data);
        break;
    }
    default:
        nlog_warn("recv unsupported msg: %s",
                  neu_reqresp_type_string(header->type));
        break;
    }
    return 0;
}

static int monitor_plugin_start(neu_plugin_t *plugin)
{
    int         rv          = 0;
    const char *plugin_name = neu_plugin_module.module_name;

    if (NULL == plugin->mqtt_client) {
        plog_notice(plugin, "mqtt client is NULL");
        goto end;
    }

    if (0 != neu_mqtt_client_open(plugin->mqtt_client)) {
        plog_error(plugin, "neu_mqtt_client_open fail");
        rv = NEU_ERR_MQTT_CONNECT_FAILURE;
        goto end;
    }

    if (0 != start_hearbeat_timer(plugin, plugin->config->heartbeat_interval)) {
        plog_error(plugin, "start_hearbeat_timer fail");
        neu_mqtt_client_close(plugin->mqtt_client);
        rv = NEU_ERR_EINTERNAL;
        goto end;
    }

end:
    if (0 == rv) {
        plugin->started = true;
        plog_notice(plugin, "start plugin `%s` success", plugin_name);
    } else {
        plog_error(plugin, "start plugin `%s` failed, error %d", plugin_name,
                   rv);
    }

    return 0;
}

static int monitor_plugin_stop(neu_plugin_t *plugin)
{
    if (plugin->mqtt_client) {
        neu_mqtt_client_close(plugin->mqtt_client);
        plog_notice(plugin, "mqtt client closed");
    }

    stop_heartbeart_timer(plugin);
    plugin->started = false;
    plog_notice(plugin, "stop plugin `%s` success",
                neu_plugin_module.module_name);
    return 0;
}

static const neu_plugin_intf_funs_t plugin_intf_funs = {
    .open    = monitor_plugin_open,
    .close   = monitor_plugin_close,
    .init    = monitor_plugin_init,
    .uninit  = monitor_plugin_uninit,
    .start   = monitor_plugin_start,
    .stop    = monitor_plugin_stop,
    .setting = monitor_plugin_config,
    .request = monitor_plugin_request,
};

#define DEFAULT_MONITOR_PLUGIN_DESCR \
    "Builtin plugin for Neuron monitoring and alerting"
#define DEFAULT_MONITOR_PLUGIN_DESCR_ZH "内置监控与告警插件"

const neu_plugin_module_t neu_plugin_module = {
    .version         = NEURON_PLUGIN_VER_1_0,
    .schema          = "monitor",
    .module_name     = "Monitor",
    .module_descr    = DEFAULT_MONITOR_PLUGIN_DESCR,
    .module_descr_zh = DEFAULT_MONITOR_PLUGIN_DESCR_ZH,
    .intf_funs       = &plugin_intf_funs,
    .kind            = NEU_PLUGIN_KIND_SYSTEM,
    .type            = NEU_NA_TYPE_APP,
    .display         = true,
    .single          = false,
    .single_name     = "monitor",
};

neu_plugin_t *neu_monitor_get_plugin()
{
    return g_monitor_plugin_;
}
