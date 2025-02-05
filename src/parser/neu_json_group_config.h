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

/*
 * DO NOT EDIT THIS FILE MANUALLY!
 * It was automatically generated by `json-autotype`.
 */

#ifndef _NEU_JSON_API_NEU_JSON_GROUP_CONFIG_H_
#define _NEU_JSON_API_NEU_JSON_GROUP_CONFIG_H_

#include "json/json.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *  group;
    char *  node;
    int64_t interval;
} neu_json_add_group_config_req_t, neu_json_update_group_req_t;

int neu_json_encode_add_group_config_req(void *json_object, void *param);
int neu_json_decode_add_group_config_req(
    char *buf, neu_json_add_group_config_req_t **result);
void neu_json_decode_add_group_config_req_free(
    neu_json_add_group_config_req_t *req);

typedef struct {
    char *  node;
    char *  group;
    char *  new_name;
    bool    set_interval;
    int64_t interval;
} neu_json_update_group_config_req_t;

int neu_json_encode_update_group_config_req(void *json_object, void *param);
int neu_json_decode_update_group_config_req(
    char *buf, neu_json_update_group_config_req_t **result);
void neu_json_decode_update_group_config_req_free(
    neu_json_update_group_config_req_t *req);

typedef struct {
    char *group;
    char *node;
} neu_json_del_group_config_req_t;

int neu_json_encode_del_group_config_req(void *json_object, void *param);
int neu_json_decode_del_group_config_req(
    char *buf, neu_json_del_group_config_req_t **result);
void neu_json_decode_del_group_config_req_free(
    neu_json_del_group_config_req_t *req);

typedef struct {
    char *  name;
    int64_t interval;
    int64_t tag_count;
} neu_json_get_group_config_resp_group_config_t;

typedef struct {
    int                                            n_group_config;
    neu_json_get_group_config_resp_group_config_t *group_configs;
} neu_json_get_group_config_resp_t;

int neu_json_encode_get_group_config_resp(void *json_object, void *param);

typedef struct {
    char *  driver;
    char *  group;
    int64_t interval;
    int64_t tag_count;
} neu_json_get_driver_group_resp_group_t;

typedef struct {
    int                                     n_group;
    neu_json_get_driver_group_resp_group_t *groups;
} neu_json_get_driver_group_resp_t;

int neu_json_encode_get_driver_group_resp(void *json_object, void *param);
int neu_json_decode_get_driver_group_resp_json(
    void *json_obj, neu_json_get_driver_group_resp_t **result);
void neu_json_decode_get_driver_group_resp_free(
    neu_json_get_driver_group_resp_t *resp);

typedef struct {
    char *group;
    char *driver;
    char *params;
} neu_json_get_subscribe_resp_group_t;

typedef struct {
    int                                  n_group;
    neu_json_get_subscribe_resp_group_t *groups;
} neu_json_get_subscribe_resp_t;

int neu_json_encode_get_subscribe_resp(void *json_object, void *param);

typedef struct {
    char *group;
    char *app;
    char *driver;
    char *params;
} neu_json_subscribe_req_t;

int neu_json_encode_subscribe_req(void *json_object, void *param);
int neu_json_decode_subscribe_req(char *buf, neu_json_subscribe_req_t **result);
void neu_json_decode_subscribe_req_free(neu_json_subscribe_req_t *req);

typedef struct {
    char *group;
    char *app;
    char *driver;
} neu_json_unsubscribe_req_t;

int neu_json_encode_unsubscribe_req(void *json_object, void *param);
int  neu_json_decode_unsubscribe_req(char *                       buf,
                                     neu_json_unsubscribe_req_t **result);
void neu_json_decode_unsubscribe_req_free(neu_json_unsubscribe_req_t *req);

typedef struct {
    char *   driver;
    char *   group;
    uint16_t port;
    char *   params;
} neu_json_subscribe_groups_info_t;

typedef struct {
    char *                            app;
    int                               n_group;
    neu_json_subscribe_groups_info_t *groups;
} neu_json_subscribe_groups_req_t;

int neu_json_decode_subscribe_groups_req(
    char *buf, neu_json_subscribe_groups_req_t **result);
void neu_json_decode_subscribe_groups_req_free(
    neu_json_subscribe_groups_req_t *req);

#ifdef __cplusplus
}
#endif

#endif
