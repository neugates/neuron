/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2023 EMQ Technologies Co., Ltd All rights reserved.
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

#ifndef NEURON_MSG_INTERNAL_H
#define NEURON_MSG_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include "msg.h"

#define NEU_REQRESP_TYPE_MAP(XX)                                     \
    XX(NEU_RESP_ERROR, neu_resp_error_t)                             \
    XX(NEU_REQ_READ_GROUP, neu_req_read_group_t)                     \
    XX(NEU_RESP_READ_GROUP, neu_resp_read_group_t)                   \
    XX(NEU_REQ_READ_GROUP_PAGINATE, neu_req_read_group_paginate_t)   \
    XX(NEU_RESP_READ_GROUP_PAGINATE, neu_resp_read_group_paginate_t) \
    XX(NEU_REQ_TEST_READ_TAG, neu_req_test_read_tag_t)               \
    XX(NEU_RESP_TEST_READ_TAG, neu_resp_test_read_tag_t)             \
    XX(NEU_REQ_WRITE_TAG, neu_req_write_tag_t)                       \
    XX(NEU_REQ_WRITE_TAGS, neu_req_write_tags_t)                     \
    XX(NEU_REQ_WRITE_GTAGS, neu_req_write_gtags_t)                   \
    XX(NEU_REQ_SUBSCRIBE_GROUP, neu_req_subscribe_t)                 \
    XX(NEU_REQ_UNSUBSCRIBE_GROUP, neu_req_unsubscribe_t)             \
    XX(NEU_REQ_UPDATE_SUBSCRIBE_GROUP, neu_req_subscribe_t)          \
    XX(NEU_REQ_SUBSCRIBE_GROUPS, neu_req_subscribe_groups_t)         \
    XX(NEU_REQ_GET_SUBSCRIBE_GROUP, neu_req_get_subscribe_group_t)   \
    XX(NEU_RESP_GET_SUBSCRIBE_GROUP, neu_resp_get_subscribe_group_t) \
    XX(NEU_REQ_GET_SUB_DRIVER_TAGS, neu_req_get_sub_driver_tags_t)   \
    XX(NEU_RESP_GET_SUB_DRIVER_TAGS, neu_resp_get_sub_driver_tags_t) \
    XX(NEU_REQ_NODE_INIT, neu_req_node_init_t)                       \
    XX(NEU_REQ_NODE_UNINIT, neu_req_node_uninit_t)                   \
    XX(NEU_RESP_NODE_UNINIT, neu_resp_node_uninit_t)                 \
    XX(NEU_REQ_ADD_NODE, neu_req_add_node_t)                         \
    XX(NEU_REQ_UPDATE_NODE, neu_req_update_node_t)                   \
    XX(NEU_REQ_DEL_NODE, neu_req_del_node_t)                         \
    XX(NEU_REQ_GET_NODE, neu_req_get_node_t)                         \
    XX(NEU_RESP_GET_NODE, neu_resp_get_node_t)                       \
    XX(NEU_REQ_NODE_SETTING, neu_req_node_setting_t)                 \
    XX(NEU_REQ_GET_NODE_SETTING, neu_req_get_node_setting_t)         \
    XX(NEU_RESP_GET_NODE_SETTING, neu_resp_get_node_setting_t)       \
    XX(NEU_REQ_GET_NODE_STATE, neu_req_get_node_state_t)             \
    XX(NEU_RESP_GET_NODE_STATE, neu_resp_get_node_state_t)           \
    XX(NEU_REQ_GET_NODES_STATE, neu_req_get_nodes_state_t)           \
    XX(NEU_RESP_GET_NODES_STATE, neu_resp_get_nodes_state_t)         \
    XX(NEU_REQ_NODE_CTL, neu_req_node_ctl_t)                         \
    XX(NEU_REQ_NODE_RENAME, neu_req_node_rename_t)                   \
    XX(NEU_RESP_NODE_RENAME, neu_resp_node_rename_t)                 \
    XX(NEU_REQ_ADD_GROUP, neu_req_add_group_t)                       \
    XX(NEU_REQ_DEL_GROUP, neu_req_del_group_t)                       \
    XX(NEU_REQ_UPDATE_GROUP, neu_req_update_group_t)                 \
    XX(NEU_REQ_UPDATE_DRIVER_GROUP, neu_req_update_group_t)          \
    XX(NEU_RESP_UPDATE_DRIVER_GROUP, neu_resp_update_group_t)        \
    XX(NEU_REQ_GET_GROUP, neu_req_get_group_t)                       \
    XX(NEU_RESP_GET_GROUP, neu_resp_get_group_t)                     \
    XX(NEU_REQ_GET_DRIVER_GROUP, neu_req_get_group_t)                \
    XX(NEU_RESP_GET_DRIVER_GROUP, neu_resp_get_driver_group_t)       \
    XX(NEU_REQ_ADD_TAG, neu_req_add_tag_t)                           \
    XX(NEU_RESP_ADD_TAG, neu_resp_add_tag_t)                         \
    XX(NEU_REQ_ADD_GTAG, neu_req_add_gtag_t)                         \
    XX(NEU_RESP_ADD_GTAG, neu_resp_add_tag_t)                        \
    XX(NEU_REQ_DEL_TAG, neu_req_del_tag_t)                           \
    XX(NEU_REQ_UPDATE_TAG, neu_req_update_tag_t)                     \
    XX(NEU_RESP_UPDATE_TAG, neu_resp_update_tag_t)                   \
    XX(NEU_REQ_GET_TAG, neu_req_get_tag_t)                           \
    XX(NEU_RESP_GET_TAG, neu_resp_get_tag_t)                         \
    XX(NEU_REQ_ADD_PLUGIN, neu_req_add_plugin_t)                     \
    XX(NEU_REQ_DEL_PLUGIN, neu_req_del_plugin_t)                     \
    XX(NEU_REQ_UPDATE_PLUGIN, neu_req_update_plugin_t)               \
    XX(NEU_REQ_GET_PLUGIN, neu_req_get_plugin_t)                     \
    XX(NEU_RESP_GET_PLUGIN, neu_resp_get_plugin_t)                   \
    XX(NEU_REQRESP_TRANS_DATA, neu_reqresp_trans_data_t)             \
    XX(NEU_REQRESP_NODES_STATE, neu_reqresp_nodes_state_t)           \
    XX(NEU_REQRESP_NODE_DELETED, neu_reqresp_node_deleted_t)         \
    XX(NEU_REQ_ADD_DRIVERS, neu_req_driver_array_t)                  \
    XX(NEU_REQ_UPDATE_LOG_LEVEL, neu_req_update_log_level_t)         \
    XX(NEU_REQ_PRGFILE_UPLOAD, neu_req_prgfile_upload_t)             \
    XX(NEU_REQ_PRGFILE_PROCESS, neu_req_prgfile_process_t)           \
    XX(NEU_RESP_PRGFILE_PROCESS, neu_resp_prgfile_process_t)         \
    XX(NEU_REQ_SCAN_TAGS, neu_req_scan_tags_t)                       \
    XX(NEU_RESP_SCAN_TAGS, neu_resp_scan_tags_t)                     \
    XX(NEU_REQ_ADD_NODE_EVENT, neu_req_add_node_t)                   \
    XX(NEU_REQ_DEL_NODE_EVENT, neu_req_del_node_t)                   \
    XX(NEU_REQ_NODE_CTL_EVENT, neu_req_node_ctl_t)                   \
    XX(NEU_REQ_NODE_SETTING_EVENT, neu_req_node_setting_t)           \
    XX(NEU_REQ_ADD_GROUP_EVENT, neu_req_add_group_t)                 \
    XX(NEU_REQ_DEL_GROUP_EVENT, neu_req_del_group_t)                 \
    XX(NEU_REQ_UPDATE_GROUP_EVENT, neu_req_update_group_t)           \
    XX(NEU_REQ_ADD_TAG_EVENT, neu_req_add_tag_t)                     \
    XX(NEU_REQ_DEL_TAG_EVENT, neu_req_del_tag_t)                     \
    XX(NEU_REQ_UPDATE_TAG_EVENT, neu_req_update_tag_t)               \
    XX(NEU_REQ_ADD_GTAG_EVENT, neu_req_add_gtag_t)                   \
    XX(NEU_REQ_ADD_PLUGIN_EVENT, neu_req_add_plugin_t)               \
    XX(NEU_REQ_DEL_PLUGIN_EVENT, neu_req_del_plugin_t)               \
    XX(NEU_REQ_SUBSCRIBE_GROUP_EVENT, neu_req_subscribe_t)           \
    XX(NEU_REQ_UNSUBSCRIBE_GROUP_EVENT, neu_req_unsubscribe_t)       \
    XX(NEU_REQ_UPDATE_SUBSCRIBE_GROUP_EVENT, neu_req_subscribe_t)    \
    XX(NEU_REQ_SUBSCRIBE_GROUPS_EVENT, neu_req_subscribe_groups_t)

static inline size_t neu_reqresp_size(neu_reqresp_type_e t)
{
    switch (t) {
#define XX(type, structure) \
    case type:              \
        return sizeof(structure);
        NEU_REQRESP_TYPE_MAP(XX)
#undef XX
    default:
        assert(false);
    }

    return 0;
}

#define XX(type, structure) structure type##_data;
union neu_reqresp_u {
    NEU_REQRESP_TYPE_MAP(XX)
};
#define NEU_REQRESP_MAX_SIZE sizeof(union neu_reqresp_u)
#undef XX

struct neu_msg_s {
    neu_reqresp_head_t head;
    //  NOTE: we keep the data layout intact only to save refactor efforts.
    // FIXME: potential alignment problem here.
    neu_reqresp_head_t body[];
};

typedef struct neu_msg_s neu_msg_t;

static inline neu_msg_t *neu_msg_new(neu_reqresp_type_e t, void *ctx,
                                     void *data)
{
    size_t data_size = neu_reqresp_size(t);

    // NOTE: ensure enough space to reuse message
    size_t body_size = 0;
    switch (t) {
    case NEU_REQ_GET_PLUGIN:
        body_size = neu_reqresp_size(NEU_RESP_GET_PLUGIN);
        break;
    case NEU_REQ_UPDATE_GROUP:
    case NEU_REQ_UPDATE_DRIVER_GROUP:
        body_size = neu_reqresp_size(NEU_RESP_UPDATE_DRIVER_GROUP);
        break;
    case NEU_REQ_UPDATE_NODE:
    case NEU_REQ_NODE_RENAME:
        body_size = neu_reqresp_size(NEU_RESP_NODE_RENAME);
        break;
    case NEU_REQ_DEL_NODE:
        body_size = neu_reqresp_size(NEU_RESP_NODE_UNINIT);
        break;
    case NEU_REQ_GET_NODE_SETTING:
        body_size = neu_reqresp_size(NEU_RESP_GET_NODE_SETTING);
        break;
    case NEU_REQ_GET_NODES_STATE:
        body_size = neu_reqresp_size(NEU_RESP_GET_NODES_STATE);
        break;
    default:
        body_size = data_size;
    }

    size_t     total = sizeof(neu_msg_t) + body_size;
    neu_msg_t *msg   = calloc(1, total);
    if (msg) {
        msg->head.type = t;
        msg->head.len  = total;
        msg->head.ctx  = ctx;
        if (data) {
            memcpy(msg->body, data, data_size);
        }
    }
    return msg;
}

static inline neu_msg_t *neu_msg_copy(const neu_msg_t *other)
{
    neu_msg_t *msg = calloc(1, other->head.len);
    if (msg) {
        memcpy(msg, other, other->head.len);
    }
    return msg;
}

static inline void neu_msg_free(neu_msg_t *msg)
{
    if (msg) {
        free(msg);
    }
}

static inline size_t neu_msg_size(neu_msg_t *msg)
{
    return msg->head.len;
}

static inline size_t neu_msg_body_size(neu_msg_t *msg)
{
    return msg->head.len - sizeof(msg->head);
}

static inline void *neu_msg_get_header(neu_msg_t *msg)
{
    return &msg->head;
}

static inline void *neu_msg_get_body(neu_msg_t *msg)
{
    return &msg->body;
}

inline static int neu_send_msg(int fd, neu_msg_t *msg)
{
    int ret = send(fd, &msg, sizeof(neu_msg_t *), 0);
    return sizeof(neu_msg_t *) == ret ? 0 : ret;
}

inline static int neu_recv_msg(int fd, neu_msg_t **msg_p)
{
    neu_msg_t *msg = NULL;
    int        ret = recv(fd, &msg, sizeof(neu_msg_t *), 0);
    if (sizeof(neu_msg_t *) != ret) {
        // recv may return 0 bytes
        return 0 == ret ? -1 : ret;
    }
    *msg_p = msg;
    return 0;
}

inline static int neu_send_msg_to(int fd, struct sockaddr_un *addr,
                                  neu_msg_t *msg)
{
    int ret = sendto(fd, &msg, sizeof(neu_msg_t *), 0, (struct sockaddr *) addr,
                     sizeof(*addr));
    return sizeof(neu_msg_t *) == ret ? 0 : ret;
}

inline static int neu_recv_msg_from(int fd, struct sockaddr_un *addr,
                                    neu_msg_t **msg_p)
{
    neu_msg_t *msg      = NULL;
    socklen_t  addr_len = sizeof(struct sockaddr_un);
    int        ret      = recvfrom(fd, &msg, sizeof(neu_msg_t *), 0,
                       (struct sockaddr *) addr, &addr_len);
    if (sizeof(neu_msg_t *) != ret) {
        // recvfrom may return 0 bytes
        return 0 == ret ? -1 : ret;
    }
    *msg_p = msg;
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif
