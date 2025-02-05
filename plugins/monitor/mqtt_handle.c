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

#include "connection/mqtt_client.h"
#include "errcodes.h"
#include "version.h"
#include "json/neu_json_mqtt.h"
#include "json/neu_json_rw.h"

#include "parser/neu_json_group_config.h"
#include "parser/neu_json_node.h"
#include "parser/neu_json_tag.h"
#include "parser/neu_json_plugin.h"

#include "monitor.h"
#include "mqtt_handle.h"

static char *generate_heartbeat_json(neu_plugin_t *plugin, UT_array *states)
{
    (void) plugin;
    char *                 version  = NEURON_VERSION;
    neu_json_states_head_t header   = { .version   = version,
                                      .timpstamp = global_timestamp };
    neu_json_states_t      json     = { 0 };
    char *                 json_str = NULL;

    json.n_state = utarray_len(states);
    json.states  = calloc(json.n_state, sizeof(neu_json_node_state_t));
    if (NULL == json.states) {
        return NULL;
    }

    utarray_foreach(states, neu_nodes_state_t *, state)
    {
        int index                  = utarray_eltidx(states, state);
        json.states[index].node    = state->node;
        json.states[index].link    = state->state.link;
        json.states[index].running = state->state.running;
    }

    neu_json_encode_with_mqtt(&json, neu_json_encode_states_resp, &header,
                              neu_json_encode_state_header_resp, &json_str);

    free(json.states);
    return json_str;
}

static char *generate_event_json(neu_plugin_t *plugin, neu_reqresp_type_e event,
                                 void *data, char **topic_p)
{
    char *json_str = NULL;
    union {
        neu_json_add_node_req_t            add_node;
        neu_json_del_node_req_t            del_node;
        neu_json_node_ctl_req_t            node_ctl;
        neu_json_node_setting_req_t        node_setting;
        neu_json_add_group_config_req_t    add_grp;
        neu_json_del_group_config_req_t    del_grp;
        neu_json_update_group_config_req_t update_grp;
        neu_json_add_tags_req_t            add_tags;
        neu_json_del_tags_req_t            del_tags;
        neu_json_subscribe_req_t           sub_grp;
        neu_json_unsubscribe_req_t         unsub_grp;
        neu_json_subscribe_groups_req_t    sub_grps;
        neu_json_add_plugin_req_t          add_plugin;
        neu_json_del_plugin_req_t          del_plugin;
        neu_json_add_gtags_req_t           add_gtag;
    } json_req = {};

    switch (event) {
    case NEU_REQ_ADD_NODE_EVENT: {
        neu_req_add_node_t *add_node = data;
        json_req.add_node.name       = add_node->node;
        json_req.add_node.plugin     = add_node->plugin;
        *topic_p                     = plugin->config->node_add_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_add_node_req,
                              &json_str);
        break;
    }
    case NEU_REQ_DEL_NODE_EVENT: {
        neu_req_del_node_t *del_node = data;
        json_req.del_node.name       = del_node->node;
        *topic_p                     = plugin->config->node_del_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_del_node_req,
                              &json_str);
        break;
    }
    case NEU_REQ_NODE_CTL_EVENT: {
        neu_req_node_ctl_t *node_ctl = data;
        json_req.node_ctl.node       = node_ctl->node;
        json_req.node_ctl.cmd        = node_ctl->ctl;
        *topic_p                     = plugin->config->node_ctl_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_node_ctl_req,
                              &json_str);
        break;
    }
    case NEU_REQ_NODE_SETTING_EVENT: {
        neu_req_node_setting_t *node_setting = data;
        json_req.node_setting.node           = node_setting->node;
        json_req.node_setting.setting        = node_setting->setting;
        *topic_p = plugin->config->node_setting_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_node_setting_req,
                              &json_str);
        break;
    }
    case NEU_REQ_ADD_GROUP_EVENT: {
        neu_req_add_group_t *add_grp = data;
        json_req.add_grp.node        = add_grp->driver;
        json_req.add_grp.group       = add_grp->group;
        json_req.add_grp.interval    = add_grp->interval;
        *topic_p                     = plugin->config->group_add_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_add_group_config_req,
                              &json_str);
        break;
    }
    case NEU_REQ_DEL_GROUP_EVENT: {
        neu_req_del_group_t *del_grp = data;
        json_req.del_grp.node        = del_grp->driver;
        json_req.del_grp.group       = del_grp->group;
        *topic_p                     = plugin->config->group_del_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_del_group_config_req,
                              &json_str);
        break;
    }
    case NEU_REQ_UPDATE_GROUP_EVENT: {
        neu_req_update_group_t *update_grp = data;
        json_req.update_grp.node           = update_grp->driver;
        json_req.update_grp.group          = update_grp->group;
        json_req.update_grp.new_name       = update_grp->new_name;
        json_req.update_grp.set_interval =
            update_grp->interval >= NEU_GROUP_INTERVAL_LIMIT;
        json_req.update_grp.interval = update_grp->interval;
        *topic_p                     = plugin->config->group_update_topic;
        neu_json_encode_by_fn(
            &json_req, neu_json_encode_update_group_config_req, &json_str);
        break;
    }
    case NEU_REQ_ADD_TAG_EVENT:
    case NEU_REQ_UPDATE_TAG_EVENT: {
        neu_req_add_tag_t *add_tag = data;
        json_req.add_tags.node     = add_tag->driver;
        json_req.add_tags.group    = add_tag->group;
        json_req.add_tags.n_tag    = add_tag->n_tag;
        json_req.add_tags.tags =
            calloc(add_tag->n_tag, sizeof(*json_req.add_tags.tags));
        if (NULL == json_req.add_tags.tags) {
            plog_error(plugin, "calloc fail");
            break;
        }
        for (uint16_t i = 0; i < add_tag->n_tag; ++i) {
            json_req.add_tags.tags[i].type      = add_tag->tags[i].type;
            json_req.add_tags.tags[i].name      = add_tag->tags[i].name;
            json_req.add_tags.tags[i].attribute = add_tag->tags[i].attribute;
            json_req.add_tags.tags[i].address   = add_tag->tags[i].address;
            json_req.add_tags.tags[i].decimal   = add_tag->tags[i].decimal;
            json_req.add_tags.tags[i].precision = add_tag->tags[i].precision;
            json_req.add_tags.tags[i].description =
                add_tag->tags[i].description;
            if (neu_tag_attribute_test(&add_tag->tags[i],
                                       NEU_ATTRIBUTE_STATIC)) {
                neu_tag_get_static_value_json(&add_tag->tags[i],
                                              &json_req.add_tags.tags[i].t,
                                              &json_req.add_tags.tags[i].value);
            } else {
                json_req.add_tags.tags[i].t = NEU_JSON_UNDEFINE;
            }
        }
        *topic_p = NEU_REQ_ADD_TAG_EVENT == event
            ? plugin->config->tag_add_topic
            : plugin->config->tag_update_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_add_tags_req,
                              &json_str);

        free(json_req.add_tags.tags);
        break;
    }
    case NEU_REQ_DEL_TAG_EVENT: {
        neu_req_del_tag_t *del_tag = data;
        json_req.del_tags.node     = del_tag->driver;
        json_req.del_tags.group    = del_tag->group;
        json_req.del_tags.n_tags   = del_tag->n_tag;
        json_req.del_tags.tags =
            calloc(del_tag->n_tag, sizeof(*json_req.del_tags.tags));
        if (NULL == json_req.del_tags.tags) {
            plog_error(plugin, "calloc fail");
            break;
        }
        for (uint16_t i = 0; i < del_tag->n_tag; ++i) {
            json_req.del_tags.tags[i] = del_tag->tags[i];
        }
        *topic_p = plugin->config->tag_del_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_del_tags_req,
                              &json_str);

        free(json_req.del_tags.tags);
        break;
    }
    case NEU_REQ_ADD_GTAG_EVENT: {
        neu_req_add_gtag_t *add_gtag = data;
        json_req.add_gtag.node       = add_gtag->driver;
        json_req.add_gtag.n_group    = add_gtag->n_group;
        json_req.add_gtag.groups     = 
            calloc(add_gtag->n_group, sizeof(*json_req.add_gtag.groups));
        for (u_int16_t i = 0; i < add_gtag->n_group; i++) {
            json_req.add_gtag.groups[i].group = strdup(add_gtag->groups[i].group);
            json_req.add_gtag.groups[i].n_tag = add_gtag->groups[i].n_tag;
            json_req.add_gtag.groups[i].interval = add_gtag->groups[i].interval;
            json_req.add_gtag.groups[i].tags =
                        calloc(add_gtag->groups[i].n_tag, sizeof(*json_req.add_gtag.groups->tags));
            for (u_int16_t j = 0; j < add_gtag->groups[i].n_tag; j++) {
                json_req.add_gtag.groups[i].tags[j].type = add_gtag->groups[i].tags[j].type;
                json_req.add_gtag.groups[i].tags[j].name = add_gtag->groups[i].tags[j].name;
                json_req.add_gtag.groups[i].tags[j].attribute = add_gtag->groups[i].tags[j].attribute;
                json_req.add_gtag.groups[i].tags[j].address = add_gtag->groups[i].tags[j].address;
                json_req.add_gtag.groups[i].tags[j].decimal = add_gtag->groups[i].tags[j].decimal;
                json_req.add_gtag.groups[i].tags[j].precision = add_gtag->groups[i].tags[j].precision;
                json_req.add_gtag.groups[i].tags[j].description = add_gtag->groups[i].tags[j].description;
            }
        }
        *topic_p = plugin->config->gtag_add_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_add_gtags_req,
                              &json_str);

        free(json_req.add_tags.tags);
        break;
    }
    case NEU_REQ_ADD_PLUGIN_EVENT: {
        neu_req_add_plugin_t *add_plugin = data;
        json_req.add_plugin.library      = add_plugin->library;
        json_req.add_plugin.schema_file  = add_plugin->schema_file;
        json_req.add_plugin.so_file      = add_plugin->so_file;
        *topic_p = plugin->config->plugin_add_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_add_plugin_req,
                              &json_str);
        break;
    }
    case NEU_REQ_DEL_PLUGIN_EVENT: {
        neu_req_del_plugin_t *del_plugin = data;
        json_req.del_plugin.plugin      = del_plugin->plugin;
        *topic_p = plugin->config->plugin_del_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_del_plugin_req,
                              &json_str);
        break;
    }
    case NEU_REQ_SUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT: {
        neu_req_subscribe_t *subscribe_grp = data;
        json_req.sub_grp.group             = subscribe_grp->group;
        json_req.sub_grp.app               = subscribe_grp->app;
        json_req.sub_grp.driver            = subscribe_grp->driver;
        json_req.sub_grp.params            = subscribe_grp->params;
        *topic_p = NEU_REQ_SUBSCRIBE_GROUP_EVENT == event
            ? plugin->config->group_subscribe_topic
            : plugin->config->group_update_subscribe_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_subscribe_req,
                              &json_str);
        break;
    }
    case NEU_REQ_UNSUBSCRIBE_GROUP_EVENT: {
        neu_req_unsubscribe_t *unsubscribe_grp = data;
        json_req.unsub_grp.group             = unsubscribe_grp->group;
        json_req.unsub_grp.app               = unsubscribe_grp->app;
        json_req.unsub_grp.driver            = unsubscribe_grp->driver;
        *topic_p = plugin->config->group_unsubscribe_topic;
        neu_json_encode_by_fn(&json_req, neu_json_encode_unsubscribe_req,
                              &json_str);
        break;
    }
    case NEU_REQ_SUBSCRIBE_GROUPS_EVENT: {
        break;
    }
    default: {
        plog_error(plugin, "unsupported event:%s",
                   neu_reqresp_type_string(event));
        return NULL;
    }
    }

    return json_str;
}

static void publish_cb(int errcode, neu_mqtt_qos_e qos, char *topic,
                       uint8_t *payload, uint32_t len, void *data)
{
    (void) qos;
    (void) topic;
    (void) len;

    neu_plugin_t *plugin = data;

    neu_adapter_update_metric_cb_t update_metric =
        plugin->common.adapter_callbacks->update_metric;

    if (0 == errcode) {
        update_metric(plugin->common.adapter, NEU_METRIC_SEND_MSGS_TOTAL, 1,
                      NULL);
    } else {
        update_metric(plugin->common.adapter, NEU_METRIC_SEND_MSG_ERRORS_TOTAL,
                      1, NULL);
    }

    free(payload);
}

static inline int publish(neu_plugin_t *plugin, neu_mqtt_qos_e qos, char *topic,
                          char *payload, size_t payload_len)
{
    neu_adapter_update_metric_cb_t update_metric =
        plugin->common.adapter_callbacks->update_metric;

    int rv = neu_mqtt_client_publish(
        plugin->mqtt_client, qos, topic, (uint8_t *) payload,
        (uint32_t) payload_len, plugin, publish_cb);
    if (0 != rv) {
        plog_error(plugin, "pub [%s, QoS%d] fail", topic, qos);
        update_metric(plugin->common.adapter, NEU_METRIC_SEND_MSG_ERRORS_TOTAL,
                      1, NULL);
        free(payload);
        rv = NEU_ERR_MQTT_PUBLISH_FAILURE;
    }

    return rv;
}

int handle_nodes_state(neu_plugin_t *plugin, neu_reqresp_nodes_state_t *states)
{
    int   rv       = 0;
    char *json_str = NULL;

    if (NULL == plugin->mqtt_client) {
        rv = NEU_ERR_MQTT_IS_NULL;
        goto end;
    }

    if (!neu_mqtt_client_is_connected(plugin->mqtt_client)) {
        // cache disable and we are disconnected
        rv = NEU_ERR_MQTT_FAILURE;
        goto end;
    }

    json_str = generate_heartbeat_json(plugin, states->states);
    if (NULL == json_str) {
        plog_error(plugin, "generate heartbeat json fail");
        rv = NEU_ERR_EINTERNAL;
        goto end;
    }

    char *         topic = plugin->config->heartbeat_topic;
    neu_mqtt_qos_e qos   = NEU_MQTT_QOS0;
    rv       = publish(plugin, qos, topic, json_str, strlen(json_str));
    json_str = NULL;

end:
    utarray_free(states->states);

    return rv;
}

int handle_events(neu_plugin_t *plugin, neu_reqresp_type_e event, void *data)
{
    int   rv       = 0;
    char *json_str = NULL;
    char *topic    = NULL;

    if (NULL == plugin->mqtt_client) {
        rv = NEU_ERR_MQTT_IS_NULL;
        goto end;
    }

    if (!neu_mqtt_client_is_connected(plugin->mqtt_client)) {
        // cache disable and we are disconnected
        rv = NEU_ERR_MQTT_FAILURE;
        goto end;
    }

    json_str = generate_event_json(plugin, event, data, &topic);
    if (NULL == json_str) {
        plog_error(plugin, "generate event:%s json fail",
                   neu_reqresp_type_string(event));
        rv = NEU_ERR_EINTERNAL;
        goto end;
    }

    neu_mqtt_qos_e qos = NEU_MQTT_QOS0;
    rv       = publish(plugin, qos, topic, json_str, strlen(json_str));
    json_str = NULL;

end:
    switch (event) {
    case NEU_REQ_NODE_SETTING_EVENT:
        neu_req_node_setting_fini(data);
        break;
    case NEU_REQ_ADD_TAG_EVENT:
    case NEU_REQ_UPDATE_TAG_EVENT:
        neu_req_add_tag_fini(data);
        break;
    case NEU_REQ_DEL_TAG_EVENT:
        neu_req_del_tag_fini(data);
        break;
    case NEU_REQ_SUBSCRIBE_GROUP_EVENT:
    case NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT:
        break;
    case NEU_REQ_SUBSCRIBE_GROUPS_EVENT:
    neu_req_subscribe_groups_fini(data);
        break;
    default:
        break;
    }
    return rv;
}
