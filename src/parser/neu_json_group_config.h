/**
 * NEURON IIoT System for Industry 4.0
 * Copyright (C) 2020-2021 EMQ Technologies Co., Ltd All rights reserved.
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
#ifndef _NEU_JSON_API_GROUP_CONFIG_H_
#define _NEU_JSON_API_GROUP_CONFIG_H_

#include <stdint.h>

#include "neu_json_parser.h"

struct neu_paser_get_group_config_req {
    enum neu_parse_function function;
    char *                  uuid;
    char *                  config;
    int                     node_id;
};

struct neu_paser_get_group_config_res_row {
    char *name;
    char *group;
    int   node_id;
    int   read_interval;
    int   pipe_count;
    int   tag_count;
};

struct neu_paser_get_group_config_res {
    enum neu_parse_function                    function;
    char *                                     uuid;
    int                                        error;
    int                                        n_config;
    struct neu_paser_get_group_config_res_row *rows;
};

struct neu_paser_group_config_row {
    char *name;
    int   type;
    int   decimal;
    char *address;
    int   flag;
};

struct neu_paser_add_group_config_req {
    enum neu_parse_function function;
    char *                  uuid;
    char *                  group;
    char *                  config;
    uint32_t                src_node_id;
    uint32_t                dst_node_id;
    int                     read_interval;
};

struct neu_paser_update_group_config_req {
    enum neu_parse_function function;
    char *                  uuid;
    char *                  group;
    char *                  config;
    uint32_t                src_node_id;
    uint32_t                dst_node_id;
    int                     read_interval;
};

struct neu_paser_delete_group_config_req {
    enum neu_parse_function function;
    char *                  uuid;
    char *                  group;
    char *                  config;
    uint32_t                node_id;
};

struct neu_paser_group_config_res {
    enum neu_parse_function function;
    char *                  uuid;
    int                     error;
};

int neu_paser_decode_get_group_config_req(
    char *buf, struct neu_paser_get_group_config_req **result);
int neu_parse_encode_get_group_config_res(
    struct neu_paser_get_group_config_res *res, char **buf);

int neu_paser_decode_add_group_config_req(
    char *buf, struct neu_paser_add_group_config_req **result);
int neu_paser_decode_update_group_config_req(
    char *buf, struct neu_paser_update_group_config_req **result);
int neu_paser_decode_delete_group_config_req(
    char *buf, struct neu_paser_delete_group_config_req **result);

int neu_parse_encode_add_group_config_res(
    struct neu_paser_group_config_res *res, char **buf);
int neu_parse_encode_update_group_config_res(
    struct neu_paser_group_config_res *res, char **buf);
int neu_parse_encode_delete_group_config_res(
    struct neu_paser_group_config_res *res, char **buf);

#endif