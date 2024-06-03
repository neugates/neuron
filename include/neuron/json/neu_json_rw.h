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

#ifndef _NEU_JSON_API_NEU_JSON_RW_H_
#define _NEU_JSON_API_NEU_JSON_RW_H_

#include "json/json.h"

#include "tag.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *   group;
    char *   node;
    uint64_t timestamp;
} neu_json_read_periodic_t;

typedef struct {
    char *               name;
    enum neu_json_type   t;
    union neu_json_value value;
} neu_json_tag_meta_t;

typedef struct {
    int64_t              error;
    char *               name;
    enum neu_json_type   t;
    union neu_json_value value;
    uint8_t              precision;
    int                  n_meta;
    neu_json_tag_meta_t *metas;
} neu_json_read_resp_tag_t;

typedef struct {
    int                       n_tag;
    neu_json_read_resp_tag_t *tags;
} neu_json_read_resp_t;

int neu_json_encode_read_resp(void *json_object, void *param);
int neu_json_encode_read_resp1(void *json_object, void *param);
int neu_json_encode_read_paginate_resp(void *json_object, void *param);

typedef struct {
    int64_t              error;
    char *               name;
    enum neu_json_type   t;
    union neu_json_value value;
    uint8_t              precision;
    int                  n_meta;
    neu_json_tag_meta_t *metas;
    neu_datatag_t        datatag;
} neu_json_read_paginate_resp_tag_t;

typedef struct {
    unsigned int current_page;
    unsigned int page_size;
    unsigned int total;
} neu_json_read_meta_resp_t;

typedef struct {
    neu_json_read_meta_resp_t          meta;
    int                                n_tag;
    neu_json_read_paginate_resp_tag_t *tags;
} neu_json_read_paginate_resp_t;

typedef struct {
    char *               group;
    char *               node;
    char *               tag;
    enum neu_json_type   t;
    union neu_json_value value;
} neu_json_write_req_t;

int  neu_json_decode_write_req(char *buf, neu_json_write_req_t **result);
int  neu_json_decode_write_req_json(void *                 json_obj,
                                    neu_json_write_req_t **result);
void neu_json_decode_write_req_free(neu_json_write_req_t *req);

typedef struct {
    char *               tag;
    enum neu_json_type   t;
    union neu_json_value value;
} neu_json_write_tags_elem_t;

typedef struct {
    char *                      group;
    char *                      node;
    int                         n_tag;
    neu_json_write_tags_elem_t *tags;
} neu_json_write_tags_req_t;
int  neu_json_decode_write_tags_req(char *                      buf,
                                    neu_json_write_tags_req_t **result);
int  neu_json_decode_write_tags_req_json(void *                      json_obj,
                                         neu_json_write_tags_req_t **result);
void neu_json_decode_write_tags_req_free(neu_json_write_tags_req_t *req);

typedef struct {
    char *                      group;
    int                         n_tag;
    neu_json_write_tags_elem_t *tags;
} neu_json_write_gtags_elem_t;
typedef struct {
    char *node;

    int                          n_group;
    neu_json_write_gtags_elem_t *groups;
} neu_json_write_gtags_req_t;

int  neu_json_decode_write_gtags_req(char *                       buf,
                                     neu_json_write_gtags_req_t **result);
int  neu_json_decode_write_gtags_req_json(void *                       json_obj,
                                          neu_json_write_gtags_req_t **result);
void neu_json_decode_write_gtags_req_free(neu_json_write_gtags_req_t *req);

typedef struct {
    union {
        neu_json_write_req_t      single;
        neu_json_write_tags_req_t plural;
    };
    bool singular;
} neu_json_write_t;

int  neu_json_decode_write(char *buf, neu_json_write_t **result);
void neu_json_decode_write_free(neu_json_write_t *req);

typedef struct {
    char *group;
    char *node;
    char *name;
    char *desc;
    bool  sync;
} neu_json_read_req_t;

int  neu_json_decode_read_req(char *buf, neu_json_read_req_t **result);
void neu_json_decode_read_req_free(neu_json_read_req_t *req);

typedef struct {
    char *group;
    char *node;
    char *name;
    char *desc;
    bool  sync;
    int   current_page;
    int   page_size;
    bool  is_error;
} neu_json_read_paginate_req_t;

int  neu_json_decode_read_paginate_req(char *                         buf,
                                       neu_json_read_paginate_req_t **result);
void neu_json_decode_read_paginate_req_free(neu_json_read_paginate_req_t *req);

int neu_json_encode_read_periodic_resp(void *json_object, void *param);

void neu_json_metas_to_json(neu_tag_meta_t *metas, int n_meta,
                            neu_json_read_resp_tag_t *json_tag);
void neu_json_metas_to_json_paginate(
    neu_tag_meta_t *metas, int n_meta,
    neu_json_read_paginate_resp_tag_t *json_tag);

#ifdef __cplusplus
}
#endif

#endif
