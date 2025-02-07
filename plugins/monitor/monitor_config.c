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

#include "json/json.h"
#include "json/neu_json_param.h"

#include "utils/asprintf.h"
#include "utils/base64.h"
#include "utils/log.h"

#include "monitor.h"
#include "monitor_config.h"

#define MB 1000000

static inline int decode_b64_param(neu_plugin_t *plugin, neu_json_elem_t *el)
{
    int   len = 0;
    char *s   = (char *) neu_decode64(&len, el->v.val_str);

    if (NULL == s) {
        plog_error(plugin, "setting %s invalid base64", el->name);
        return -1;
    }

    if (0 == len) {
        plog_error(plugin, "setting empty %s", el->name);
        free(s);
        return -1;
    }

    free(el->v.val_str);
    el->v.val_str = s;
    return 0;
}

static inline int parse_b64_param(neu_plugin_t *plugin, const char *setting,
                                  neu_json_elem_t *el)
{
    if (0 != neu_parse_param(setting, NULL, 1, el)) {
        plog_error(plugin, "setting no %s", el->name);
        return -1;
    }

    if (0 != decode_b64_param(plugin, el)) {
        free(el->v.val_str);
        el->v.val_str = NULL;
        return -1;
    }

    return 0;
}

static inline int parse_ssl_params(neu_plugin_t *plugin, const char *setting,
                                   neu_json_elem_t *ssl, neu_json_elem_t *ca,
                                   neu_json_elem_t *cert, neu_json_elem_t *key,
                                   neu_json_elem_t *keypass)
{
    // ssl, optional
    int ret = neu_parse_param(setting, NULL, 1, ssl);
    if (0 != ret) {
        plog_notice(plugin, "setting no ssl");
        return 0;
    }

    if (false == ssl->v.val_bool) {
        plog_notice(plugin, "setting ssl disabled");
        return 0;
    }

    // ca, optional
    ret = neu_parse_param(setting, NULL, 1, ca);
    if (0 != ret) {
        plog_notice(plugin, "setting no ca");
        return 0;
    }

    if (0 != decode_b64_param(plugin, ca)) {
        return -1;
    }

    // cert, optional
    ret = neu_parse_param(setting, NULL, 1, cert);
    if (0 != ret) {
        plog_notice(plugin, "setting no cert");
        return 0;
    }

    if (0 != decode_b64_param(plugin, cert)) {
        return -1;
    }

    // key, required if cert enable
    ret = parse_b64_param(plugin, setting, key);
    if (0 != ret) {
        return -1;
    }

    // keypass, optional
    ret = neu_parse_param(setting, NULL, 1, keypass);
    if (0 != ret) {
        plog_notice(plugin, "setting no keypass");
    } else if (0 == strlen(keypass->v.val_str)) {
        plog_error(plugin, "setting invalid keypass");
        return -1;
    }

    return 0;
}

static int make_event_topics(monitor_config_t *config, const char *prefix)
{
    char *node_add_topic               = NULL;
    char *node_del_topic               = NULL;
    char *node_ctl_topic               = NULL;
    char *node_setting_topic           = NULL;
    char *group_add_topic              = NULL;
    char *group_del_topic              = NULL;
    char *group_update_topic           = NULL;
    char *tag_add_topic                = NULL;
    char *tag_del_topic                = NULL;
    char *tag_update_topic             = NULL;
    char *gtag_add_topic               = NULL;
    char *plugin_add_topic             = NULL;
    char *plugin_del_topic             = NULL;
    char *group_subscribe_topic        = NULL;
    char *group_unsubscribe_topic      = NULL;
    char *group_update_subscribe_topic = NULL;
    char *groups_subscribe_topic       = NULL;

    neu_asprintf(&node_add_topic, "%s/node/add", prefix);
    if (NULL == node_add_topic) {
        goto error;
    }

    neu_asprintf(&node_del_topic, "%s/node/delete", prefix);
    if (NULL == node_del_topic) {
        goto error;
    }

    neu_asprintf(&node_ctl_topic, "%s/node/ctl", prefix);
    if (NULL == node_ctl_topic) {
        goto error;
    }

    neu_asprintf(&node_setting_topic, "%s/node/setting", prefix);
    if (NULL == node_setting_topic) {
        goto error;
    }

    neu_asprintf(&group_add_topic, "%s/group/add", prefix);
    if (NULL == group_add_topic) {
        goto error;
    }

    neu_asprintf(&group_del_topic, "%s/group/delete", prefix);
    if (NULL == group_del_topic) {
        goto error;
    }

    neu_asprintf(&group_update_topic, "%s/group/update", prefix);
    if (NULL == group_update_topic) {
        goto error;
    }

    neu_asprintf(&tag_add_topic, "%s/tag/add", prefix);
    if (NULL == tag_add_topic) {
        goto error;
    }

    neu_asprintf(&tag_del_topic, "%s/tag/delete", prefix);
    if (NULL == tag_del_topic) {
        goto error;
    }

    neu_asprintf(&tag_update_topic, "%s/tag/update", prefix);
    if (NULL == tag_update_topic) {
        goto error;
    }

    neu_asprintf(&gtag_add_topic, "%s/gtag/add", prefix);
    if (NULL == gtag_add_topic) {
        goto error;
    }

    neu_asprintf(&plugin_add_topic, "%s/plugin/add", prefix);
    if (NULL == plugin_add_topic) {
        goto error;
    }

    neu_asprintf(&plugin_del_topic, "%s/plugin/delete", prefix);
    if (NULL == plugin_del_topic) {
        goto error;
    }


    neu_asprintf(&group_subscribe_topic, "%s/group/subscribe", prefix);
    if (NULL == group_subscribe_topic) {
        goto error;
    }

    neu_asprintf(&group_unsubscribe_topic, "%s/group/unsubscribe", prefix);
    if (NULL == group_unsubscribe_topic) {
        goto error;
    }

    neu_asprintf(&group_update_subscribe_topic, "%s/group/update/subscribe", prefix);
    if (NULL == group_update_subscribe_topic) {
        goto error;
    }

    neu_asprintf(&groups_subscribe_topic, "%s/groups/subscribe", prefix);
    if (NULL == groups_subscribe_topic) {
        goto error;
    }

    config->node_add_topic               = node_add_topic;
    config->node_del_topic               = node_del_topic;
    config->node_ctl_topic               = node_ctl_topic;
    config->node_setting_topic           = node_setting_topic;
    config->group_add_topic              = group_add_topic;
    config->group_del_topic              = group_del_topic;
    config->group_update_topic           = group_update_topic;
    config->tag_add_topic                = tag_add_topic;
    config->tag_del_topic                = tag_del_topic;
    config->tag_update_topic             = tag_update_topic;
    config->gtag_add_topic               = gtag_add_topic;
    config->plugin_add_topic             = plugin_add_topic;
    config->plugin_del_topic             = plugin_del_topic;
    config->group_subscribe_topic        = group_subscribe_topic;
    config->group_unsubscribe_topic      = group_unsubscribe_topic;
    config->group_update_subscribe_topic = group_update_subscribe_topic;
    config->groups_subscribe_topic       = groups_subscribe_topic;

    return 0;

error:
    free(tag_update_topic);
    free(tag_del_topic);
    free(tag_add_topic);
    free(group_update_topic);
    free(group_del_topic);
    free(group_add_topic);
    free(node_setting_topic);
    free(node_ctl_topic);
    free(node_del_topic);
    free(node_add_topic);
    return -1;
}

int monitor_config_parse(neu_plugin_t *plugin, const char *setting,
                         monitor_config_t *config)
{
    int         ret         = 0;
    char *      err_param   = NULL;
    const char *placeholder = "********";

    neu_json_elem_t client_id = { .name = "client-id", .t = NEU_JSON_STR };
    neu_json_elem_t event_topic_prefix = { .name = "event-topic-prefix",
                                           .t    = NEU_JSON_STR };
    neu_json_elem_t heartbeat_interval = { .name = "heartbeat-interval",
                                           .t    = NEU_JSON_INT };
    neu_json_elem_t heartbeat_topic    = { .name = "heartbeat-topic",
                                        .t    = NEU_JSON_STR };
    neu_json_elem_t host               = { .name = "host", .t = NEU_JSON_STR };
    neu_json_elem_t port               = { .name = "port", .t = NEU_JSON_INT };
    neu_json_elem_t username = { .name = "username", .t = NEU_JSON_STR };
    neu_json_elem_t password = { .name = "password", .t = NEU_JSON_STR };
    neu_json_elem_t ssl      = { .name = "ssl", .t = NEU_JSON_BOOL };
    neu_json_elem_t ca       = { .name = "ca", .t = NEU_JSON_STR };
    neu_json_elem_t cert     = { .name = "cert", .t = NEU_JSON_STR };
    neu_json_elem_t key      = { .name = "key", .t = NEU_JSON_STR };
    neu_json_elem_t keypass  = { .name = "keypass", .t = NEU_JSON_STR };

    if (NULL == setting || NULL == config) {
        plog_error(plugin, "invalid argument, null pointer");
        return -1;
    }

    ret =
        neu_parse_param(setting, &err_param, 6, &client_id, &event_topic_prefix,
                        &heartbeat_interval, &heartbeat_topic, &host, &port);
    if (0 != ret) {
        plog_error(plugin, "parsing setting fail, key: `%s`", err_param);
        goto error;
    }

    // client-id, required
    if (0 == strlen(client_id.v.val_str)) {
        plog_error(plugin, "setting empty client-id");
        goto error;
    }

    // heartbeat-interval, required
    if (0 > heartbeat_interval.v.val_int) {
        plog_error(plugin, "setting negative heartbeat-interval:%" PRIi64,
                   heartbeat_interval.v.val_int);
        goto error;
    }

    // heartbeat-topic, required
    if (0 == strlen(heartbeat_topic.v.val_str)) {
        plog_error(plugin, "setting empty heartbeat-topic");
        goto error;
    }

    // host, required
    if (0 == strlen(host.v.val_str)) {
        plog_error(plugin, "setting invalid host: `%s`", host.v.val_str);
        goto error;
    }

    // port, required
    if (0 == port.v.val_int || port.v.val_int > 65535) {
        plog_error(plugin, "setting invalid port: %" PRIi64, port.v.val_int);
        goto error;
    }

    // username, optional
    ret = neu_parse_param(setting, NULL, 1, &username);
    if (0 != ret) {
        plog_notice(plugin, "setting no username");
    }

    // password, optional
    ret = neu_parse_param(setting, NULL, 1, &password);
    if (0 != ret) {
        plog_notice(plugin, "setting no password");
    }

    ret = parse_ssl_params(plugin, setting, &ssl, &ca, &cert, &key, &keypass);
    if (0 != ret) {
        goto error;
    }

    ret = make_event_topics(config, event_topic_prefix.v.val_str);
    if (0 != ret) {
        plog_error(plugin, "make event topics fail");
        goto error;
    }

    config->client_id          = client_id.v.val_str;
    config->event_topic_prefix = event_topic_prefix.v.val_str;
    config->heartbeat_interval = heartbeat_interval.v.val_int;
    config->heartbeat_topic    = heartbeat_topic.v.val_str;
    config->host               = host.v.val_str;
    config->port               = port.v.val_int;
    config->username           = username.v.val_str;
    config->password           = password.v.val_str;
    config->ssl                = ssl.v.val_bool;
    config->ca                 = ca.v.val_str;
    config->cert               = cert.v.val_str;
    config->key                = key.v.val_str;
    config->keypass            = keypass.v.val_str;

    plog_notice(plugin, "config client-id         : %s", config->client_id);
    plog_notice(plugin, "config event-topic-prefix: %s",
                config->event_topic_prefix);
    plog_notice(plugin, "config heartbeat-interval: %" PRIu64,
                config->heartbeat_interval);
    plog_notice(plugin, "config heartbeat-topic   : %s",
                config->heartbeat_topic);
    plog_notice(plugin, "config host              : %s", config->host);
    plog_notice(plugin, "config port              : %" PRIu16, config->port);

    if (config->username) {
        plog_notice(plugin, "config username          : %s", config->username);
    }
    if (config->password) {
        plog_notice(plugin, "config password          : %s",
                    0 == strlen(config->password) ? "" : placeholder);
    }

    plog_notice(plugin, "config ssl               : %d", config->ssl);
    if (config->ca) {
        plog_notice(plugin, "config ca                : %s", placeholder);
    }
    if (config->cert) {
        plog_notice(plugin, "config cert              : %s", placeholder);
    }
    if (config->key) {
        plog_notice(plugin, "config key               : %s", placeholder);
    }
    if (config->keypass) {
        plog_notice(plugin, "config keypass           : %s", placeholder);
    }

    plog_notice(plugin, "node add event topic     : %s",
                config->node_add_topic);
    plog_notice(plugin, "node del event topic     : %s",
                config->node_del_topic);
    plog_notice(plugin, "node ctl event topic     : %s",
                config->node_ctl_topic);
    plog_notice(plugin, "node setting event topic : %s",
                config->node_setting_topic);
    plog_notice(plugin, "group add topic          : %s",
                config->group_add_topic);
    plog_notice(plugin, "group del topic          : %s",
                config->group_del_topic);
    plog_notice(plugin, "group update topic       : %s",
                config->group_update_topic);
    plog_notice(plugin, "tag add topic            : %s", config->tag_add_topic);
    plog_notice(plugin, "tag del topic            : %s", config->tag_del_topic);
    plog_notice(plugin, "tag update topic         : %s",
                config->tag_update_topic);

    return 0;

error:
    free(err_param);
    free(event_topic_prefix.v.val_str);
    free(heartbeat_topic.v.val_str);
    free(host.v.val_str);
    free(username.v.val_str);
    free(password.v.val_str);
    free(ca.v.val_str);
    free(cert.v.val_str);
    free(key.v.val_str);
    free(keypass.v.val_str);
    return -1;
}

void monitor_config_fini(monitor_config_t *config)
{
    if (NULL == config) {
        return;
    }

    free(config->client_id);
    free(config->event_topic_prefix);
    free(config->heartbeat_topic);
    free(config->host);
    free(config->username);
    free(config->password);
    free(config->ca);
    free(config->cert);
    free(config->key);
    free(config->keypass);

    free(config->node_add_topic);
    free(config->node_del_topic);
    free(config->node_ctl_topic);
    free(config->node_setting_topic);
    free(config->group_add_topic);
    free(config->group_del_topic);
    free(config->group_update_topic);
    free(config->tag_add_topic);
    free(config->tag_del_topic);
    free(config->tag_update_topic);

    memset(config, 0, sizeof(*config));
}
