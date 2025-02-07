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

#ifndef NEURON_MSG_H
#define NEURON_MSG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <pthread.h>
#include <stdint.h>

#include "json/neu_json_rw.h"

#include "define.h"
#include "errcodes.h"
#include "tag.h"
#include "type.h"
#include <math.h>

typedef struct {
    neu_node_running_state_e running;
    neu_node_link_state_e    link;
    int                      log_level;
} neu_node_state_t;

typedef enum neu_reqresp_type {
    NEU_RESP_ERROR,

    NEU_REQ_READ_GROUP,
    NEU_RESP_READ_GROUP,
    NEU_REQ_READ_GROUP_PAGINATE,
    NEU_RESP_READ_GROUP_PAGINATE,
    NEU_REQ_TEST_READ_TAG,
    NEU_RESP_TEST_READ_TAG,
    NEU_REQ_WRITE_TAG,
    NEU_REQ_WRITE_TAGS,
    NEU_REQ_WRITE_GTAGS,

    NEU_REQ_SUBSCRIBE_GROUP,
    NEU_REQ_UNSUBSCRIBE_GROUP,
    NEU_REQ_UPDATE_SUBSCRIBE_GROUP,
    NEU_REQ_SUBSCRIBE_GROUPS,
    NEU_REQ_GET_SUBSCRIBE_GROUP,
    NEU_RESP_GET_SUBSCRIBE_GROUP,
    NEU_REQ_GET_SUB_DRIVER_TAGS,
    NEU_RESP_GET_SUB_DRIVER_TAGS,

    NEU_REQ_NODE_INIT,
    NEU_REQ_NODE_UNINIT,
    NEU_RESP_NODE_UNINIT,
    NEU_REQ_ADD_NODE,
    NEU_REQ_UPDATE_NODE,
    NEU_REQ_DEL_NODE,
    NEU_REQ_GET_NODE,
    NEU_RESP_GET_NODE,
    NEU_REQ_NODE_SETTING,
    NEU_REQ_GET_NODE_SETTING,
    NEU_RESP_GET_NODE_SETTING,
    NEU_REQ_GET_NODE_STATE,
    NEU_RESP_GET_NODE_STATE,
    NEU_REQ_GET_NODES_STATE,
    NEU_RESP_GET_NODES_STATE,
    NEU_REQ_NODE_CTL,
    NEU_REQ_NODE_RENAME,
    NEU_RESP_NODE_RENAME,

    NEU_REQ_ADD_GROUP,
    NEU_REQ_DEL_GROUP,
    NEU_REQ_UPDATE_GROUP,
    NEU_REQ_UPDATE_DRIVER_GROUP,
    NEU_RESP_UPDATE_DRIVER_GROUP,
    NEU_REQ_GET_GROUP,
    NEU_RESP_GET_GROUP,
    NEU_REQ_GET_DRIVER_GROUP,
    NEU_RESP_GET_DRIVER_GROUP,

    NEU_REQ_ADD_TAG,
    NEU_RESP_ADD_TAG,
    NEU_REQ_ADD_GTAG,
    NEU_RESP_ADD_GTAG,
    NEU_REQ_DEL_TAG,
    NEU_REQ_UPDATE_TAG,
    NEU_RESP_UPDATE_TAG,
    NEU_REQ_GET_TAG,
    NEU_RESP_GET_TAG,

    NEU_REQ_ADD_PLUGIN,
    NEU_REQ_DEL_PLUGIN,
    NEU_REQ_UPDATE_PLUGIN,
    NEU_REQ_GET_PLUGIN,
    NEU_RESP_GET_PLUGIN,

    NEU_REQRESP_TRANS_DATA,
    NEU_REQRESP_NODES_STATE,
    NEU_REQRESP_NODE_DELETED,

    NEU_REQ_ADD_DRIVERS,

    NEU_REQ_UPDATE_LOG_LEVEL,

    NEU_REQ_PRGFILE_UPLOAD,
    NEU_REQ_PRGFILE_PROCESS,
    NEU_RESP_PRGFILE_PROCESS,

    NEU_REQ_SCAN_TAGS,
    NEU_RESP_SCAN_TAGS,

    NEU_REQ_ADD_NODE_EVENT,
    NEU_REQ_DEL_NODE_EVENT,
    NEU_REQ_NODE_CTL_EVENT,
    NEU_REQ_NODE_SETTING_EVENT,
    NEU_REQ_ADD_GROUP_EVENT,
    NEU_REQ_DEL_GROUP_EVENT,
    NEU_REQ_UPDATE_GROUP_EVENT,
    NEU_REQ_ADD_TAG_EVENT,
    NEU_REQ_DEL_TAG_EVENT,
    NEU_REQ_UPDATE_TAG_EVENT,

    NEU_REQ_ADD_GTAG_EVENT,

    NEU_REQ_ADD_PLUGIN_EVENT,
    NEU_REQ_DEL_PLUGIN_EVENT,

    NEU_REQ_SUBSCRIBE_GROUP_EVENT,
    NEU_REQ_UNSUBSCRIBE_GROUP_EVENT,
    NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT,
    NEU_REQ_SUBSCRIBE_GROUPS_EVENT,
} neu_reqresp_type_e;

static const char *neu_reqresp_type_string_t[] = {
    [NEU_RESP_ERROR] = "NEU_RESP_ERROR",

    [NEU_REQ_READ_GROUP]           = "NEU_REQ_READ_GROUP",
    [NEU_RESP_READ_GROUP]          = "NEU_RESP_READ_GROUP",
    [NEU_REQ_READ_GROUP_PAGINATE]  = "NEU_REQ_READ_GROUP_PAGINATE",
    [NEU_RESP_READ_GROUP_PAGINATE] = "NEU_RESP_READ_GROUP_PAGINATE",
    [NEU_REQ_TEST_READ_TAG]        = "NEU_REQ_TEST_READ_TAG",
    [NEU_RESP_TEST_READ_TAG]       = "NEU_RESP_TEST_READ_TAG",
    [NEU_REQ_WRITE_TAG]            = "NEU_REQ_WRITE_TAG",
    [NEU_REQ_WRITE_TAGS]           = "NEU_REQ_WRITE_TAGS",
    [NEU_REQ_WRITE_GTAGS]          = "NEU_REQ_WRITE_GTAGS",

    [NEU_REQ_SUBSCRIBE_GROUP]        = "NEU_REQ_SUBSCRIBE_GROUP",
    [NEU_REQ_UNSUBSCRIBE_GROUP]      = "NEU_REQ_UNSUBSCRIBE_GROUP",
    [NEU_REQ_UPDATE_SUBSCRIBE_GROUP] = "NEU_REQ_UPDATE_SUBSCRIBE_GROUP",
    [NEU_REQ_SUBSCRIBE_GROUPS]       = "NEU_REQ_SUBSCRIBE_GROUPS",
    [NEU_REQ_GET_SUBSCRIBE_GROUP]    = "NEU_REQ_GET_SUBSCRIBE_GROUP",
    [NEU_RESP_GET_SUBSCRIBE_GROUP]   = "NEU_RESP_GET_SUBSCRIBE_GROUP",
    [NEU_REQ_GET_SUB_DRIVER_TAGS]    = "NEU_REQ_GET_SUB_DRIVER_TAGS",
    [NEU_RESP_GET_SUB_DRIVER_TAGS]   = "NEU_RESP_GET_SUB_DRIVER_TAGS",

    [NEU_REQ_NODE_INIT]         = "NEU_REQ_NODE_INIT",
    [NEU_REQ_NODE_UNINIT]       = "NEU_REQ_NODE_UNINIT",
    [NEU_RESP_NODE_UNINIT]      = "NEU_RESP_NODE_UNINIT",
    [NEU_REQ_ADD_NODE]          = "NEU_REQ_ADD_NODE",
    [NEU_REQ_UPDATE_NODE]       = "NEU_REQ_UPDATE_NODE",
    [NEU_REQ_DEL_NODE]          = "NEU_REQ_DEL_NODE",
    [NEU_REQ_GET_NODE]          = "NEU_REQ_GET_NODE",
    [NEU_RESP_GET_NODE]         = "NEU_RESP_GET_NODE",
    [NEU_REQ_NODE_SETTING]      = "NEU_REQ_NODE_SETTING",
    [NEU_REQ_GET_NODE_SETTING]  = "NEU_REQ_GET_NODE_SETTING",
    [NEU_RESP_GET_NODE_SETTING] = "NEU_RESP_GET_NODE_SETTING",
    [NEU_REQ_GET_NODE_STATE]    = "NEU_REQ_GET_NODE_STATE",
    [NEU_RESP_GET_NODE_STATE]   = "NEU_RESP_GET_NODE_STATE",
    [NEU_REQ_GET_NODES_STATE]   = "NEU_REQ_GET_NODES_STATE",
    [NEU_RESP_GET_NODES_STATE]  = "NEU_RESP_GET_NODES_STATE",
    [NEU_REQ_NODE_CTL]          = "NEU_REQ_NODE_CTL",
    [NEU_REQ_NODE_RENAME]       = "NEU_REQ_NODE_RENAME",
    [NEU_RESP_NODE_RENAME]      = "NEU_RESP_NODE_RENAME",

    [NEU_REQ_ADD_GROUP]            = "NEU_REQ_ADD_GROUP",
    [NEU_REQ_DEL_GROUP]            = "NEU_REQ_DEL_GROUP",
    [NEU_REQ_UPDATE_GROUP]         = "NEU_REQ_UPDATE_GROUP",
    [NEU_REQ_UPDATE_DRIVER_GROUP]  = "NEU_REQ_UPDATE_DRIVER_GROUP",
    [NEU_RESP_UPDATE_DRIVER_GROUP] = "NEU_RESP_UPDATE_DRIVER_GROUP",
    [NEU_REQ_GET_GROUP]            = "NEU_REQ_GET_GROUP",
    [NEU_RESP_GET_GROUP]           = "NEU_RESP_GET_GROUP",
    [NEU_REQ_GET_DRIVER_GROUP]     = "NEU_REQ_GET_DRIVER_GROUP",
    [NEU_RESP_GET_DRIVER_GROUP]    = "NEU_RESP_GET_DRIVER_GROUP",

    [NEU_REQ_ADD_TAG]     = "NEU_REQ_ADD_TAG",
    [NEU_RESP_ADD_TAG]    = "NEU_RESP_ADD_TAG",
    [NEU_REQ_ADD_GTAG]    = "NEU_REQ_ADD_GTAG",
    [NEU_RESP_ADD_GTAG]   = "NEU_RESP_ADD_GTAG",
    [NEU_REQ_DEL_TAG]     = "NEU_REQ_DEL_TAG",
    [NEU_REQ_UPDATE_TAG]  = "NEU_REQ_UPDATE_TAG",
    [NEU_RESP_UPDATE_TAG] = "NEU_RESP_UPDATE_TAG",
    [NEU_REQ_GET_TAG]     = "NEU_REQ_GET_TAG",
    [NEU_RESP_GET_TAG]    = "NEU_RESP_GET_TAG",

    [NEU_REQ_ADD_PLUGIN]    = "NEU_REQ_ADD_PLUGIN",
    [NEU_REQ_DEL_PLUGIN]    = "NEU_REQ_DEL_PLUGIN",
    [NEU_REQ_UPDATE_PLUGIN] = "NEU_REQ_UPDATE_PLUGIN",
    [NEU_REQ_GET_PLUGIN]    = "NEU_REQ_GET_PLUGIN",
    [NEU_RESP_GET_PLUGIN]   = "NEU_RESP_GET_PLUGIN",

    [NEU_REQRESP_TRANS_DATA]   = "NEU_REQRESP_TRANS_DATA",
    [NEU_REQRESP_NODES_STATE]  = "NEU_REQRESP_NODES_STATE",
    [NEU_REQRESP_NODE_DELETED] = "NEU_REQRESP_NODE_DELETED",

    [NEU_REQ_ADD_DRIVERS] = "NEU_REQ_ADD_DRIVERS",

    [NEU_REQ_UPDATE_LOG_LEVEL] = "NEU_REQ_UPDATE_LOG_LEVEL",
    [NEU_REQ_PRGFILE_UPLOAD]   = "NEU_REQ_PRGFILE_UPLOAD",
    [NEU_REQ_PRGFILE_PROCESS]  = "NEU_REQ_PRGFILE_PROCESS",
    [NEU_RESP_PRGFILE_PROCESS] = "NEU_RESP_PRGFILE_PROCESS",

    [NEU_REQ_SCAN_TAGS]  = "NEU_REQ_SCAN_TAGS",
    [NEU_RESP_SCAN_TAGS] = "NEU_RESP_SCAN_TAGS",

    [NEU_REQ_ADD_NODE_EVENT]               = "NEU_REQ_ADD_NODE_EVENT",
    [NEU_REQ_DEL_NODE_EVENT]               = "NEU_REQ_DEL_NODE_EVENT",
    [NEU_REQ_NODE_CTL_EVENT]               = "NEU_REQ_NODE_CTL_EVENT",
    [NEU_REQ_NODE_SETTING_EVENT]           = "NEU_REQ_NODE_SETTING_EVENT",
    [NEU_REQ_ADD_GROUP_EVENT]              = "NEU_REQ_ADD_GROUP_EVENT",
    [NEU_REQ_DEL_GROUP_EVENT]              = "NEU_REQ_DEL_GROUP_EVENT",
    [NEU_REQ_UPDATE_GROUP_EVENT]           = "NEU_REQ_UPDATE_GROUP_EVENT",
    [NEU_REQ_ADD_TAG_EVENT]                = "NEU_REQ_ADD_TAG_EVENT",
    [NEU_REQ_DEL_TAG_EVENT]                = "NEU_REQ_DEL_TAG_EVENT",
    [NEU_REQ_UPDATE_TAG_EVENT]             = "NEU_REQ_UPDATE_TAG_EVENT",
    [NEU_REQ_ADD_GTAG_EVENT]               = "NEU_REQ_ADD_GTAG_EVENT",
    [NEU_REQ_ADD_PLUGIN_EVENT]             = "NEU_REQ_ADD_PLUGIN_EVENT",
    [NEU_REQ_DEL_PLUGIN_EVENT]             = "NEU_REQ_DEL_PLUGIN_EVENT",
    [NEU_REQ_SUBSCRIBE_GROUP_EVENT]        = "NEU_REQ_SUBSCRIBE_GROUP_EVENT",
    [NEU_REQ_UNSUBSCRIBE_GROUP_EVENT]      = "NEU_REQ_UNSUBSCRIBE_GROUP_EVENT",
    [NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT] = "NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT",
    [NEU_REQ_SUBSCRIBE_GROUPS_EVENT]       = "NEU_REQ_SUBSCRIBE_GROUPS_EVENT",
};

inline static const char *neu_reqresp_type_string(neu_reqresp_type_e type)
{
    return neu_reqresp_type_string_t[type];
}

typedef enum {
    NEU_OTEL_TRACE_TYPE_UNSET = 0,
    NEU_OTEL_TRACE_TYPE_REST_COMM,
    NEU_OTEL_TRACE_TYPE_REST_SPEC,
    NEU_OTEL_TRACE_TYPE_MQTT,
    NEU_OTEL_TRACE_TYPE_EKUIPER,
} neu_otel_trace_type_e;

typedef struct neu_reqresp_head {
    neu_reqresp_type_e type;
    void *             ctx;
    char               sender[NEU_NODE_NAME_LEN];
    char               receiver[NEU_NODE_NAME_LEN];
    uint8_t            otel_trace_type;
    uint32_t           len;
} neu_reqresp_head_t;

typedef struct neu_resp_error {
    int error;
} neu_resp_error_t;

typedef struct {
    char                     node[NEU_NODE_NAME_LEN];
    neu_node_running_state_e state;
} neu_req_node_init_t, neu_req_node_uninit_t, neu_resp_node_uninit_t;

typedef struct neu_req_add_plugin {
    char  library[NEU_PLUGIN_LIBRARY_LEN];
    char *schema_file;
    char *so_file;
} neu_req_add_plugin_t;

typedef neu_req_add_plugin_t neu_req_update_plugin_t;

typedef struct neu_req_del_plugin {
    char plugin[NEU_PLUGIN_NAME_LEN];
} neu_req_del_plugin_t;

typedef struct neu_req_get_plugin {
} neu_req_get_plugin_t;

typedef struct neu_resp_plugin_info {
    char schema[NEU_PLUGIN_NAME_LEN];
    char name[NEU_PLUGIN_NAME_LEN];
    char description[NEU_PLUGIN_DESCRIPTION_LEN];
    char description_zh[NEU_PLUGIN_DESCRIPTION_LEN];
    char library[NEU_PLUGIN_LIBRARY_LEN];

    neu_plugin_kind_e kind;
    neu_node_type_e   type;
    uint32_t          version;

    bool single;
    bool display;
    char single_name[NEU_NODE_NAME_LEN];
} neu_resp_plugin_info_t;

typedef struct neu_resp_get_plugin {
    UT_array *plugins; // array neu_resp_plugin_info_t
} neu_resp_get_plugin_t;

typedef struct neu_req_add_node {
    char  node[NEU_NODE_NAME_LEN];
    char  plugin[NEU_PLUGIN_NAME_LEN];
    char *setting;
} neu_req_add_node_t;

static inline void neu_req_add_node_fini(neu_req_add_node_t *req)
{
    free(req->setting);
}

typedef struct neu_req_update_node {
    char node[NEU_NODE_NAME_LEN];
    char new_name[NEU_NODE_NAME_LEN];
} neu_req_update_node_t;

typedef struct neu_req_del_node {
    char node[NEU_NODE_NAME_LEN];
} neu_req_del_node_t;

typedef struct neu_req_get_node {
    neu_node_type_e type;
    char            plugin[NEU_PLUGIN_NAME_LEN];
    char            node[NEU_NODE_NAME_LEN];
} neu_req_get_node_t;

typedef struct neu_resp_node_info {
    char node[NEU_NODE_NAME_LEN];
    char plugin[NEU_PLUGIN_NAME_LEN];
} neu_resp_node_info_t;

typedef struct neu_resp_get_node {
    UT_array *nodes; // array neu_resp_node_info_t
} neu_resp_get_node_t;

typedef struct {
    char     driver[NEU_NODE_NAME_LEN];
    char     group[NEU_GROUP_NAME_LEN];
    uint32_t interval;
} neu_req_add_group_t;

typedef struct {
    char     driver[NEU_NODE_NAME_LEN];
    char     group[NEU_GROUP_NAME_LEN];
    char     new_name[NEU_GROUP_NAME_LEN];
    uint32_t interval;
} neu_req_update_group_t;

typedef struct {
    char driver[NEU_NODE_NAME_LEN];
    char group[NEU_GROUP_NAME_LEN];
    char new_name[NEU_GROUP_NAME_LEN];
    struct {
        int      error;
        uint32_t unused; // padding for memory layout compatible with
                         // neu_req_update_group_t
    };
} neu_resp_update_group_t;

typedef struct neu_req_del_group {
    char driver[NEU_NODE_NAME_LEN];
    char group[NEU_GROUP_NAME_LEN];
} neu_req_del_group_t;

typedef struct neu_req_get_group {
    char driver[NEU_NODE_NAME_LEN];
} neu_req_get_group_t;

typedef struct neu_resp_group_info {
    char     name[NEU_GROUP_NAME_LEN];
    uint16_t tag_count;
    uint32_t interval;
} neu_resp_group_info_t;

typedef struct neu_resp_get_group {
    UT_array *groups; // array neu_resp_group_info_t
} neu_resp_get_group_t;

typedef struct {
    char           group[NEU_GROUP_NAME_LEN];
    int            n_tag;
    int            interval;
    neu_datatag_t *tags;
} neu_gdatatag_t;

typedef struct {
    char            driver[NEU_NODE_NAME_LEN];
    uint16_t        n_group;
    neu_gdatatag_t *groups;
} neu_req_add_gtag_t;

typedef struct {
    char     driver[NEU_NODE_NAME_LEN];
    char     group[NEU_GROUP_NAME_LEN];
    uint16_t tag_count;
    uint32_t interval;
} neu_resp_driver_group_info_t;

typedef struct {
    UT_array *groups; // array neu_resp_driver_group_info_t
} neu_resp_get_driver_group_t;

typedef struct {
    char           driver[NEU_NODE_NAME_LEN];
    char           group[NEU_GROUP_NAME_LEN];
    uint16_t       n_tag;
    neu_datatag_t *tags;
} neu_req_add_tag_t, neu_req_update_tag_t;

static inline void neu_req_add_tag_fini(neu_req_add_tag_t *req)
{
    for (uint16_t i = 0; i < req->n_tag; i++) {
        neu_tag_fini(&req->tags[i]);
    }
    free(req->tags);
}

static inline int neu_req_add_tag_copy(neu_req_add_tag_t *dst,
                                       neu_req_add_tag_t *src)
{
    strcpy(dst->driver, src->driver);
    strcpy(dst->group, src->group);
    dst->tags = (neu_datatag_t *) calloc(src->n_tag, sizeof(src->tags[0]));
    if (NULL == dst->tags) {
        return -1;
    }
    dst->n_tag = src->n_tag;
    for (uint16_t i = 0; i < src->n_tag; i++) {
        neu_tag_copy(&dst->tags[i], &src->tags[i]);
    }
    return 0;
}

typedef struct {
    uint16_t index;
    int      error;
} neu_resp_add_tag_t, neu_resp_update_tag_t;

typedef struct neu_req_del_tag {
    char     driver[NEU_NODE_NAME_LEN];
    char     group[NEU_GROUP_NAME_LEN];
    uint16_t n_tag;
    char **  tags;
} neu_req_del_tag_t;

static inline void neu_req_del_tag_fini(neu_req_del_tag_t *req)
{
    for (uint16_t i = 0; i < req->n_tag; i++) {
        free(req->tags[i]);
    }
    free(req->tags);
}

static inline int neu_req_del_tag_copy(neu_req_del_tag_t *dst,
                                       neu_req_del_tag_t *src)
{
    strcpy(dst->driver, src->driver);
    strcpy(dst->group, src->group);
    dst->tags = (char **) calloc(src->n_tag, sizeof(src->tags[0]));
    if (NULL == dst->tags) {
        return -1;
    }
    dst->n_tag = src->n_tag;
    for (uint16_t i = 0; i < src->n_tag; ++i) {
        dst->tags[i] = strdup(src->tags[i]);
        if (NULL == dst->tags[i]) {
            while (i-- > 0) {
                free(dst->tags[i]);
            }
            free(dst->tags);
        }
    }
    return 0;
}

typedef struct neu_req_get_tag {
    char driver[NEU_NODE_NAME_LEN];
    char group[NEU_GROUP_NAME_LEN];
    char name[NEU_TAG_NAME_LEN];
} neu_req_get_tag_t;

typedef struct neu_resp_get_tag {
    UT_array *tags; // array neu_datatag_t
} neu_resp_get_tag_t;

typedef struct {
    char     app[NEU_NODE_NAME_LEN];
    char     driver[NEU_NODE_NAME_LEN];
    char     group[NEU_GROUP_NAME_LEN];
    uint16_t port;
    char *   params;
} neu_req_subscribe_t;

typedef struct {
    char app[NEU_NODE_NAME_LEN];
    char driver[NEU_NODE_NAME_LEN];
    char group[NEU_GROUP_NAME_LEN];
} neu_req_unsubscribe_t;

typedef struct {
    char *   driver;
    char *   group;
    uint16_t port;
    char *   params;
} neu_req_subscribe_group_info_t;

typedef struct {
    char *                          app;
    uint16_t                        n_group;
    neu_req_subscribe_group_info_t *groups;
} neu_req_subscribe_groups_t;

static inline void
neu_req_subscribe_groups_fini(neu_req_subscribe_groups_t *req)
{
    for (uint16_t i = 0; i < req->n_group; ++i) {
        free(req->groups[i].driver);
        free(req->groups[i].group);
        free(req->groups[i].params);
    }
    free(req->groups);
    free(req->app);
}

typedef struct neu_req_get_subscribe_group {
    char app[NEU_NODE_NAME_LEN];
    char driver[NEU_NODE_NAME_LEN];
    char group[NEU_GROUP_NAME_LEN];
} neu_req_get_subscribe_group_t;

typedef struct {
    char app[NEU_NODE_NAME_LEN];
} neu_req_get_sub_driver_tags_t;

typedef struct neu_resp_subscribe_info {
    char  app[NEU_NODE_NAME_LEN];
    char  driver[NEU_NODE_NAME_LEN];
    char  group[NEU_GROUP_NAME_LEN];
    char *params;
} neu_resp_subscribe_info_t;

static inline void neu_resp_subscribe_info_fini(neu_resp_subscribe_info_t *info)
{
    free(info->params);
}

typedef struct {
    char      driver[NEU_NODE_NAME_LEN];
    char      group[NEU_GROUP_NAME_LEN];
    UT_array *tags;
} neu_resp_get_sub_driver_tags_info_t;
inline static UT_icd *neu_resp_get_sub_driver_tags_info_icd()
{
    static UT_icd icd = { sizeof(neu_resp_get_sub_driver_tags_info_t), NULL,
                          NULL, NULL };
    return &icd;
}

typedef struct {
    UT_array *infos; // array neu_resp_get_sub_driver_tags_info_t
} neu_resp_get_sub_driver_tags_t;

typedef struct neu_resp_get_subscribe_group {
    UT_array *groups; // array neu_resp_subscribe_info_t
} neu_resp_get_subscribe_group_t;

typedef struct neu_req_node_setting {
    char  node[NEU_NODE_NAME_LEN];
    char *setting;
} neu_req_node_setting_t;

static inline void neu_req_node_setting_fini(neu_req_node_setting_t *req)
{
    free(req->setting);
}

static inline int neu_req_node_setting_copy(neu_req_node_setting_t *dst,
                                            neu_req_node_setting_t *src)
{
    strcpy(dst->node, src->node);
    dst->setting = strdup(src->setting);
    return dst->setting ? 0 : -1;
}

typedef struct neu_req_get_node_setting {
    char node[NEU_NODE_NAME_LEN];
} neu_req_get_node_setting_t;

typedef struct neu_resp_get_node_setting {
    char  node[NEU_NODE_NAME_LEN];
    char *setting;
} neu_resp_get_node_setting_t;

typedef enum neu_adapter_ctl {
    NEU_ADAPTER_CTL_START = 0,
    NEU_ADAPTER_CTL_STOP,
} neu_adapter_ctl_e;

typedef struct neu_req_node_ctl {
    char              node[NEU_NODE_NAME_LEN];
    neu_adapter_ctl_e ctl;
} neu_req_node_ctl_t;

typedef struct {
    char node[NEU_NODE_NAME_LEN];
    char new_name[NEU_NODE_NAME_LEN];
} neu_req_node_rename_t;

typedef struct {
    char node[NEU_NODE_NAME_LEN];
    char new_name[NEU_NODE_NAME_LEN];
    int  error;
} neu_resp_node_rename_t;

typedef struct neu_req_get_node_state {
    char node[NEU_NODE_NAME_LEN];
} neu_req_get_node_state_t;

typedef struct neu_resp_get_node_state {
    neu_node_state_t state;
    uint16_t         rtt; // round trip time in milliseconds
    int              core_level;
    uint16_t         sub_group_count;
    bool             is_driver;
} neu_resp_get_node_state_t;

typedef struct neu_req_get_nodes_state {
} neu_req_get_nodes_state_t;

typedef struct {
    char             node[NEU_NODE_NAME_LEN];
    neu_node_state_t state;
    uint16_t         rtt; // round trip time in milliseconds
    uint16_t         sub_group_count;
    bool             is_driver;
} neu_nodes_state_t;

inline static UT_icd neu_nodes_state_t_icd()
{
    UT_icd icd = { sizeof(neu_nodes_state_t), NULL, NULL, NULL };

    return icd;
}
typedef struct {
    UT_array *states; // array of neu_nodes_state_t
    int       core_level;
} neu_resp_get_nodes_state_t, neu_reqresp_nodes_state_t;

typedef struct neu_req_read_group {
    char *driver;
    char *group;
    char *name;
    char *desc;
    bool  sync;
} neu_req_read_group_t;

static inline void neu_req_read_group_fini(neu_req_read_group_t *req)
{
    free(req->driver);
    free(req->group);
    free(req->name);
    free(req->desc);
}

typedef struct neu_req_read_group_paginate {
    char *driver;
    char *group;
    char *name;
    char *desc;
    bool  sync;
    int   current_page;
    int   page_size;
    bool  is_error;
} neu_req_read_group_paginate_t;

static inline void
neu_req_read_group_paginate_fini(neu_req_read_group_paginate_t *req)
{
    free(req->driver);
    free(req->group);
    free(req->name);
    free(req->desc);
}

typedef struct neu_req_write_tag {
    char *       driver;
    char *       group;
    char *       tag;
    neu_dvalue_t value;
} neu_req_write_tag_t;

static inline void neu_req_write_tag_fini(neu_req_write_tag_t *req)
{
    free(req->driver);
    free(req->group);
    free(req->tag);
}

typedef struct neu_resp_tag_value {
    char         tag[NEU_TAG_NAME_LEN];
    neu_dvalue_t value;
} neu_resp_tag_value_t;

typedef neu_resp_tag_value_t neu_tag_value_t;

typedef struct neu_resp_tag_value_meta {
    char           tag[NEU_TAG_NAME_LEN];
    neu_dvalue_t   value;
    neu_tag_meta_t metas[NEU_TAG_META_SIZE];
    neu_datatag_t  datatag;
} neu_resp_tag_value_meta_t;

static inline UT_icd *neu_resp_tag_value_meta_icd()
{
    static UT_icd icd = { sizeof(neu_resp_tag_value_meta_t), NULL, NULL, NULL };
    return &icd;
}

typedef struct neu_req_write_tags {
    char *driver;
    char *group;

    int                   n_tag;
    neu_resp_tag_value_t *tags;
} neu_req_write_tags_t;

static inline void neu_req_write_tags_fini(neu_req_write_tags_t *req)
{
    free(req->driver);
    free(req->group);
    free(req->tags);
}

typedef struct {
    char *group;

    int                   n_tag;
    neu_resp_tag_value_t *tags;
} neu_req_gtag_group_t;

static inline void neu_req_gtag_group_fini(neu_req_gtag_group_t *req)
{
    free(req->group);
    free(req->tags);
}

typedef struct {
    char *driver;

    int                   n_group;
    neu_req_gtag_group_t *groups;
} neu_req_write_gtags_t;

static inline void neu_req_write_gtags_fini(neu_req_write_gtags_t *req)
{
    free(req->driver);
    for (int i = 0; i < req->n_group; ++i) {
        neu_req_gtag_group_fini(&req->groups[i]);
    }
    free(req->groups);
}

typedef struct {
    char *driver;
    char *group;

    UT_array *tags; // neu_resp_tag_value_meta_t
} neu_resp_read_group_t;

typedef struct neu_resp_tag_value_meta_paginate {
    char           tag[NEU_TAG_NAME_LEN];
    neu_dvalue_t   value;
    neu_tag_meta_t metas[NEU_TAG_META_SIZE];
    neu_datatag_t  datatag;
} neu_resp_tag_value_meta_paginate_t;

static inline UT_icd *neu_resp_tag_value_meta_paginate_icd()
{
    static UT_icd icd = { sizeof(neu_resp_tag_value_meta_paginate_t), NULL,
                          NULL, NULL };
    return &icd;
}

typedef struct {
    char *driver;
    char *group;
    int   current_page;
    int   page_size;
    bool  is_error;
    int   total_count; // tags count without pagination

    UT_array *tags; // neu_resp_tag_value_meta_paginate_t
} neu_resp_read_group_paginate_t;

static inline void neu_resp_read_free(neu_resp_read_group_t *resp)
{
    utarray_foreach(resp->tags, neu_resp_tag_value_meta_t *, tag_value)
    {
        if (tag_value->value.type == NEU_TYPE_PTR) {
            free(tag_value->value.value.ptr.ptr);
        }
    }
    free(resp->driver);
    free(resp->group);
    utarray_free(resp->tags);
}

static inline void
neu_resp_read_paginate_free(neu_resp_read_group_paginate_t *resp)
{
    utarray_foreach(resp->tags, neu_resp_tag_value_meta_t *, tag_value)
    {
        if (tag_value->value.type == NEU_TYPE_PTR) {
            free(tag_value->value.value.ptr.ptr);
        }
    }
    free(resp->driver);
    free(resp->group);
    utarray_free(resp->tags);
}

typedef struct {
    uint16_t        index;
    pthread_mutex_t mtx;
} neu_reqresp_trans_data_ctx_t;

typedef struct {
    char *driver;
    char *group;
    void *trace_ctx;

    neu_reqresp_trans_data_ctx_t *ctx;
    UT_array *                    tags; // neu_resp_tag_value_meta_t
} neu_reqresp_trans_data_t;

typedef struct {
    char *          node;
    char *          plugin;
    char *          setting;
    uint16_t        n_group;
    neu_gdatatag_t *groups;
} neu_req_driver_t;

static inline void neu_req_driver_fini(neu_req_driver_t *req)
{
    free(req->node);
    free(req->plugin);
    free(req->setting);
    for (uint16_t i = 0; i < req->n_group; i++) {
        for (int j = 0; j < req->groups[i].n_tag; j++) {
            neu_tag_fini(&req->groups[i].tags[j]);
        }
        free(req->groups[i].tags);
    }
    free(req->groups);
}

typedef struct {
    uint16_t          n_driver;
    neu_req_driver_t *drivers;
} neu_req_driver_array_t;

static inline void neu_req_driver_array_fini(neu_req_driver_array_t *req)
{
    for (uint16_t i = 0; i < req->n_driver; ++i) {
        neu_req_driver_fini(&req->drivers[i]);
    }
    free(req->drivers);
}

static inline void neu_trans_data_free(neu_reqresp_trans_data_t *data)
{
    pthread_mutex_lock(&data->ctx->mtx);
    if (data->ctx->index > 0) {
        data->ctx->index -= 1;
    }

    if (data->ctx->index == 0) {
        utarray_foreach(data->tags, neu_resp_tag_value_meta_t *, tag_value)
        {
            if (tag_value->value.type == NEU_TYPE_PTR) {
                free(tag_value->value.value.ptr.ptr);
            }
        }
        utarray_free(data->tags);
        free(data->group);
        free(data->driver);
        pthread_mutex_unlock(&data->ctx->mtx);
        pthread_mutex_destroy(&data->ctx->mtx);
        free(data->ctx);
    } else {
        pthread_mutex_unlock(&data->ctx->mtx);
    }
}

static inline void neu_tag_value_to_json(neu_resp_tag_value_meta_t *tag_value,
                                         neu_json_read_resp_tag_t * tag_json)
{
    tag_json->name  = tag_value->tag;
    tag_json->error = 0;

    for (int k = 0; k < NEU_TAG_META_SIZE; k++) {
        if (strlen(tag_value->metas[k].name) > 0) {
            tag_json->n_meta++;
        } else {
            break;
        }
    }
    if (tag_json->n_meta > 0) {
        tag_json->metas = (neu_json_tag_meta_t *) calloc(
            tag_json->n_meta, sizeof(neu_json_tag_meta_t));
    }
    neu_json_metas_to_json(tag_value->metas, NEU_TAG_META_SIZE, tag_json);

    tag_json->datatag.bias = tag_value->datatag.bias;

    switch (tag_value->value.type) {
    case NEU_TYPE_ERROR:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i32;
        tag_json->error         = tag_value->value.value.i32;
        break;
    case NEU_TYPE_UINT8:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u8;
        break;
    case NEU_TYPE_INT8:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i8;
        break;
    case NEU_TYPE_INT16:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i16;
        break;
    case NEU_TYPE_INT32:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i32;
        break;
    case NEU_TYPE_INT64:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i64;
        break;
    case NEU_TYPE_WORD:
    case NEU_TYPE_UINT16:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u16;
        break;
    case NEU_TYPE_DWORD:
    case NEU_TYPE_UINT32:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u32;
        break;
    case NEU_TYPE_LWORD:
    case NEU_TYPE_UINT64:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u64;
        break;
    case NEU_TYPE_FLOAT:
        if (isnan(tag_value->value.value.f32)) {
            tag_json->t               = NEU_JSON_FLOAT;
            tag_json->value.val_float = tag_value->value.value.f32;
            tag_json->error           = NEU_ERR_PLUGIN_TAG_VALUE_EXPIRED;
        } else {
            tag_json->t               = NEU_JSON_FLOAT;
            tag_json->value.val_float = tag_value->value.value.f32;
            tag_json->precision       = tag_value->value.precision;
        }
        break;
    case NEU_TYPE_DOUBLE:
        if (isnan(tag_value->value.value.d64)) {
            tag_json->t                = NEU_JSON_DOUBLE;
            tag_json->value.val_double = tag_value->value.value.d64;
            tag_json->error            = NEU_ERR_PLUGIN_TAG_VALUE_EXPIRED;
        } else {
            tag_json->t                = NEU_JSON_DOUBLE;
            tag_json->value.val_double = tag_value->value.value.d64;
            tag_json->precision        = tag_value->value.precision;
        }
        break;
    case NEU_TYPE_BOOL:
        tag_json->t              = NEU_JSON_BOOL;
        tag_json->value.val_bool = tag_value->value.value.boolean;
        break;
    case NEU_TYPE_BIT:
        tag_json->t             = NEU_JSON_BIT;
        tag_json->value.val_bit = tag_value->value.value.u8;
        break;
    case NEU_TYPE_STRING:
    case NEU_TYPE_TIME:
    case NEU_TYPE_DATA_AND_TIME:
    case NEU_TYPE_ARRAY_CHAR:
        tag_json->t             = NEU_JSON_STR;
        tag_json->value.val_str = tag_value->value.value.str;
        break;
    case NEU_TYPE_PTR:
        tag_json->t             = NEU_JSON_STR;
        tag_json->value.val_str = (char *) tag_value->value.value.ptr.ptr;
        break;
    case NEU_TYPE_BYTES:
        tag_json->t = NEU_JSON_ARRAY_UINT8;
        tag_json->value.val_array_uint8.length =
            tag_value->value.value.bytes.length;
        tag_json->value.val_array_uint8.u8s =
            tag_value->value.value.bytes.bytes;
        break;
    case NEU_TYPE_ARRAY_BOOL:
        tag_json->t = NEU_JSON_ARRAY_BOOL;
        tag_json->value.val_array_bool.length =
            tag_value->value.value.bools.length;
        tag_json->value.val_array_bool.bools =
            tag_value->value.value.bools.bools;
        break;
    case NEU_TYPE_ARRAY_INT8:
        tag_json->t = NEU_JSON_ARRAY_INT8;
        tag_json->value.val_array_int8.length =
            tag_value->value.value.i8s.length;
        tag_json->value.val_array_int8.i8s = tag_value->value.value.i8s.i8s;
        break;
    case NEU_TYPE_ARRAY_UINT8:
        tag_json->t = NEU_JSON_ARRAY_UINT8;
        tag_json->value.val_array_uint8.length =
            tag_value->value.value.u8s.length;
        tag_json->value.val_array_uint8.u8s = tag_value->value.value.u8s.u8s;
        break;
    case NEU_TYPE_ARRAY_INT16:
        tag_json->t = NEU_JSON_ARRAY_INT16;
        tag_json->value.val_array_int16.length =
            tag_value->value.value.i16s.length;
        tag_json->value.val_array_int16.i16s = tag_value->value.value.i16s.i16s;
        break;
    case NEU_TYPE_ARRAY_UINT16:
        tag_json->t = NEU_JSON_ARRAY_UINT16;
        tag_json->value.val_array_uint16.length =
            tag_value->value.value.u16s.length;
        tag_json->value.val_array_uint16.u16s =
            tag_value->value.value.u16s.u16s;
        break;
    case NEU_TYPE_ARRAY_INT32:
        tag_json->t = NEU_JSON_ARRAY_INT32;
        tag_json->value.val_array_int32.length =
            tag_value->value.value.i32s.length;
        tag_json->value.val_array_int32.i32s = tag_value->value.value.i32s.i32s;
        break;
    case NEU_TYPE_ARRAY_UINT32:
        tag_json->t = NEU_JSON_ARRAY_UINT32;
        tag_json->value.val_array_uint32.length =
            tag_value->value.value.u32s.length;
        tag_json->value.val_array_uint32.u32s =
            tag_value->value.value.u32s.u32s;
        break;
    case NEU_TYPE_ARRAY_INT64:
        tag_json->t = NEU_JSON_ARRAY_INT64;
        tag_json->value.val_array_int64.length =
            tag_value->value.value.i64s.length;
        tag_json->value.val_array_int64.i64s = tag_value->value.value.i64s.i64s;
        break;
    case NEU_TYPE_ARRAY_UINT64:
        tag_json->t = NEU_JSON_ARRAY_UINT64;
        tag_json->value.val_array_uint64.length =
            tag_value->value.value.u64s.length;
        tag_json->value.val_array_uint64.u64s =
            tag_value->value.value.u64s.u64s;
        break;
    case NEU_TYPE_ARRAY_FLOAT:
        tag_json->t = NEU_JSON_ARRAY_FLOAT;
        tag_json->value.val_array_float.length =
            tag_value->value.value.f32s.length;
        tag_json->value.val_array_float.f32s = tag_value->value.value.f32s.f32s;
        break;
    case NEU_TYPE_ARRAY_DOUBLE:
        tag_json->t = NEU_JSON_ARRAY_DOUBLE;
        tag_json->value.val_array_double.length =
            tag_value->value.value.f64s.length;
        tag_json->value.val_array_double.f64s =
            tag_value->value.value.f64s.f64s;
        break;
    default:
        break;
    }
}

static inline void
neu_tag_value_to_json_paginate(neu_resp_tag_value_meta_paginate_t *tag_value,
                               neu_json_read_paginate_resp_tag_t * tag_json)
{
    tag_json->name  = tag_value->tag;
    tag_json->error = 0;

    tag_json->datatag.name        = strdup(tag_value->datatag.name);
    tag_json->datatag.address     = strdup(tag_value->datatag.address);
    tag_json->datatag.attribute   = tag_value->datatag.attribute;
    tag_json->datatag.type        = tag_value->datatag.type;
    tag_json->datatag.precision   = tag_value->datatag.precision;
    tag_json->datatag.decimal     = tag_value->datatag.decimal;
    tag_json->datatag.bias        = tag_value->datatag.bias;
    tag_json->datatag.description = strdup(tag_value->datatag.description);
    tag_json->datatag.option      = tag_value->datatag.option;
    memcpy(tag_json->datatag.meta, tag_value->datatag.meta,
           NEU_TAG_META_LENGTH);

    for (int k = 0; k < NEU_TAG_META_SIZE; k++) {
        if (strlen(tag_value->metas[k].name) > 0) {
            tag_json->n_meta++;
        } else {
            break;
        }
    }
    if (tag_json->n_meta > 0) {
        tag_json->metas = (neu_json_tag_meta_t *) calloc(
            tag_json->n_meta, sizeof(neu_json_tag_meta_t));
    }
    neu_json_metas_to_json_paginate(tag_value->metas, NEU_TAG_META_SIZE,
                                    tag_json);

    switch (tag_value->value.type) {
    case NEU_TYPE_ERROR:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i32;
        tag_json->error         = tag_value->value.value.i32;
        break;
    case NEU_TYPE_UINT8:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u8;
        break;
    case NEU_TYPE_INT8:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i8;
        break;
    case NEU_TYPE_INT16:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i16;
        break;
    case NEU_TYPE_INT32:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i32;
        break;
    case NEU_TYPE_INT64:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.i64;
        break;
    case NEU_TYPE_WORD:
    case NEU_TYPE_UINT16:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u16;
        break;
    case NEU_TYPE_DWORD:
    case NEU_TYPE_UINT32:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u32;
        break;
    case NEU_TYPE_LWORD:
    case NEU_TYPE_UINT64:
        tag_json->t             = NEU_JSON_INT;
        tag_json->value.val_int = tag_value->value.value.u64;
        break;
    case NEU_TYPE_FLOAT:
        if (isnan(tag_value->value.value.f32)) {
            tag_json->t               = NEU_JSON_FLOAT;
            tag_json->value.val_float = tag_value->value.value.f32;
            tag_json->error           = NEU_ERR_PLUGIN_TAG_VALUE_EXPIRED;
        } else {
            tag_json->t               = NEU_JSON_FLOAT;
            tag_json->value.val_float = tag_value->value.value.f32;
            tag_json->precision       = tag_value->value.precision;
        }
        break;
    case NEU_TYPE_DOUBLE:
        if (isnan(tag_value->value.value.d64)) {
            tag_json->t                = NEU_JSON_DOUBLE;
            tag_json->value.val_double = tag_value->value.value.d64;
            tag_json->error            = NEU_ERR_PLUGIN_TAG_VALUE_EXPIRED;
        } else {
            tag_json->t                = NEU_JSON_DOUBLE;
            tag_json->value.val_double = tag_value->value.value.d64;
            tag_json->precision        = tag_value->value.precision;
        }
        break;
    case NEU_TYPE_BOOL:
        tag_json->t              = NEU_JSON_BOOL;
        tag_json->value.val_bool = tag_value->value.value.boolean;
        break;
    case NEU_TYPE_BIT:
        tag_json->t             = NEU_JSON_BIT;
        tag_json->value.val_bit = tag_value->value.value.u8;
        break;
    case NEU_TYPE_STRING:
    case NEU_TYPE_TIME:
    case NEU_TYPE_DATA_AND_TIME:
    case NEU_TYPE_ARRAY_CHAR:
        tag_json->t             = NEU_JSON_STR;
        tag_json->value.val_str = tag_value->value.value.str;
        break;
    case NEU_TYPE_PTR:
        tag_json->t             = NEU_JSON_STR;
        tag_json->value.val_str = (char *) tag_value->value.value.ptr.ptr;
        break;
    case NEU_TYPE_BYTES:
        tag_json->t = NEU_JSON_ARRAY_UINT8;
        tag_json->value.val_array_uint8.length =
            tag_value->value.value.bytes.length;
        tag_json->value.val_array_uint8.u8s =
            tag_value->value.value.bytes.bytes;
        break;
    case NEU_TYPE_ARRAY_BOOL:
        tag_json->t = NEU_JSON_ARRAY_BOOL;
        tag_json->value.val_array_bool.length =
            tag_value->value.value.bools.length;
        tag_json->value.val_array_bool.bools =
            tag_value->value.value.bools.bools;
        break;
    case NEU_TYPE_ARRAY_INT8:
        tag_json->t = NEU_JSON_ARRAY_INT8;
        tag_json->value.val_array_int8.length =
            tag_value->value.value.i8s.length;
        tag_json->value.val_array_int8.i8s = tag_value->value.value.i8s.i8s;
        break;
    case NEU_TYPE_ARRAY_UINT8:
        tag_json->t = NEU_JSON_ARRAY_UINT8;
        tag_json->value.val_array_uint8.length =
            tag_value->value.value.u8s.length;
        tag_json->value.val_array_uint8.u8s = tag_value->value.value.u8s.u8s;
        break;
    case NEU_TYPE_ARRAY_INT16:
        tag_json->t = NEU_JSON_ARRAY_INT16;
        tag_json->value.val_array_int16.length =
            tag_value->value.value.i16s.length;
        tag_json->value.val_array_int16.i16s = tag_value->value.value.i16s.i16s;
        break;
    case NEU_TYPE_ARRAY_UINT16:
        tag_json->t = NEU_JSON_ARRAY_UINT16;
        tag_json->value.val_array_uint16.length =
            tag_value->value.value.u16s.length;
        tag_json->value.val_array_uint16.u16s =
            tag_value->value.value.u16s.u16s;
        break;
    case NEU_TYPE_ARRAY_INT32:
        tag_json->t = NEU_JSON_ARRAY_INT32;
        tag_json->value.val_array_int32.length =
            tag_value->value.value.i32s.length;
        tag_json->value.val_array_int32.i32s = tag_value->value.value.i32s.i32s;
        break;
    case NEU_TYPE_ARRAY_UINT32:
        tag_json->t = NEU_JSON_ARRAY_UINT32;
        tag_json->value.val_array_uint32.length =
            tag_value->value.value.u32s.length;
        tag_json->value.val_array_uint32.u32s =
            tag_value->value.value.u32s.u32s;
        break;
    case NEU_TYPE_ARRAY_INT64:
        tag_json->t = NEU_JSON_ARRAY_INT64;
        tag_json->value.val_array_int64.length =
            tag_value->value.value.i64s.length;
        tag_json->value.val_array_int64.i64s = tag_value->value.value.i64s.i64s;
        break;
    case NEU_TYPE_ARRAY_UINT64:
        tag_json->t = NEU_JSON_ARRAY_UINT64;
        tag_json->value.val_array_uint64.length =
            tag_value->value.value.u64s.length;
        tag_json->value.val_array_uint64.u64s =
            tag_value->value.value.u64s.u64s;
        break;
    case NEU_TYPE_ARRAY_FLOAT:
        tag_json->t = NEU_JSON_ARRAY_FLOAT;
        tag_json->value.val_array_float.length =
            tag_value->value.value.f32s.length;
        tag_json->value.val_array_float.f32s = tag_value->value.value.f32s.f32s;
        break;
    case NEU_TYPE_ARRAY_DOUBLE:
        tag_json->t = NEU_JSON_ARRAY_DOUBLE;
        tag_json->value.val_array_double.length =
            tag_value->value.value.f64s.length;
        tag_json->value.val_array_double.f64s =
            tag_value->value.value.f64s.f64s;
        break;
    default:
        break;
    }
}

typedef struct {
    char node[NEU_NODE_NAME_LEN];
} neu_reqresp_node_deleted_t;

typedef struct neu_req_update_log_level {
    char node[NEU_NODE_NAME_LEN];
    int  log_level;
    bool core;
} neu_req_update_log_level_t;

void neu_msg_gen(neu_reqresp_head_t *header, void *data);

inline static void neu_msg_exchange(neu_reqresp_head_t *header)
{
    char tmp[NEU_NODE_NAME_LEN] = { 0 };

    strcpy(tmp, header->sender);

    memset(header->sender, 0, sizeof(header->sender));
    strcpy(header->sender, header->receiver);

    memset(header->receiver, 0, sizeof(header->receiver));
    strcpy(header->receiver, tmp);
}

typedef struct {
    char driver[NEU_NODE_NAME_LEN];
    char path[128];
    bool del_flag;
    char prg_str_param1[128];
    char prg_str_param2[128];
    char prg_str_param3[128];
    char prg_str_param4[128];
} neu_req_prgfile_upload_t;

typedef struct {
    char driver[NEU_NODE_NAME_LEN];
} neu_req_prgfile_process_t;

typedef struct {
    char path[128];
    int  state;
    char reason[256];
} neu_resp_prgfile_process_t;

typedef struct neu_req_scan_tags {
    char driver[NEU_NODE_NAME_LEN];
    char id[NEU_TAG_ADDRESS_LEN];
    char ctx[NEU_VALUE_SIZE];
} neu_req_scan_tags_t;

typedef struct {
    char    name[NEU_TAG_NAME_LEN];
    char    id[5 + 1 + NEU_TAG_ADDRESS_LEN]; // ns + ! + address
    uint8_t tag;
} neu_scan_tag_t;

typedef struct {
    UT_array * scan_tags;
    int32_t    error;
    neu_type_e type;
    bool       is_array;
    char       ctx[NEU_VALUE_SIZE];
} neu_resp_scan_tags_t;

typedef struct {
    char       name[NEU_TAG_NAME_LEN];
    char       id[5 + 1 + NEU_TAG_ADDRESS_LEN];
    neu_type_e type;
} neu_scan_tag_attribute_t;

typedef struct neu_req_test_rea_tag {
    char                      driver[NEU_NODE_NAME_LEN];
    char                      group[NEU_GROUP_NAME_LEN];
    char                      tag[NEU_TAG_NAME_LEN];
    char                      address[NEU_TAG_ADDRESS_LEN];
    neu_attribute_e           attribute;
    neu_type_e                type;
    uint8_t                   precision;
    double                    decimal;
    double                    bias;
    neu_datatag_addr_option_u option;
} neu_req_test_read_tag_t;

typedef struct {
    neu_type_e       type;
    neu_json_type_e  t;
    neu_json_value_u value;
    int64_t          error;
} neu_resp_test_read_tag_t;

#ifdef __cplusplus
}
#endif

#endif
