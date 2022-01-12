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

/*
 * DO NOT EDIT THIS FILE MANUALLY!
 * It was automatically generated by `json-autotype`.
 */

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

#include "json/json.h"

#include "persist/json/json_datatags.h"

int neu_json_decode_datatag_req(char *buf, neu_json_datatag_req_t **result)
{
    int ret = 0;

    neu_json_datatag_req_t *req = calloc(1, sizeof(neu_json_datatag_req_t));
    // neu_json_elem_t         req_elems[] = { {} };
    // ret = neu_json_decode(buf, NEU_JSON_ELEM_SIZE(req_elems), req_elems);
    // if (ret != 0) {
    //     goto decode_fail;
    // }

    req->n_tag = neu_json_decode_array_size(buf, "tags");
    req->tags  = calloc(req->n_tag, sizeof(neu_json_datatag_req_tag_t));
    neu_json_datatag_req_tag_t *p_tag = req->tags;
    for (int i = 0; i < req->n_tag; i++) {
        neu_json_elem_t tag_elems[] = { {
                                            .name = "type",
                                            .t    = NEU_JSON_INT,
                                        },
                                        {
                                            .name = "name",
                                            .t    = NEU_JSON_STR,
                                        },
                                        {
                                            .name = "attribute",
                                            .t    = NEU_JSON_INT,
                                        },
                                        {
                                            .name = "address",
                                            .t    = NEU_JSON_STR,
                                        } };
        ret                         = neu_json_decode_array(buf, "tags", i,
                                    NEU_JSON_ELEM_SIZE(tag_elems), tag_elems);
        if (ret != 0) {
            goto decode_fail;
        }

        p_tag->type      = tag_elems[0].v.val_int;
        p_tag->name      = tag_elems[1].v.val_str;
        p_tag->attribute = tag_elems[2].v.val_int;
        p_tag->address   = tag_elems[3].v.val_str;
        p_tag++;
    }

    *result = req;
    return ret;

decode_fail:
    if (req->tags != NULL) {
        free(req->tags);
    }
    if (req != NULL) {
        free(req);
    }
    return -1;
}

void neu_json_decode_datatag_req_free(neu_json_datatag_req_t *req)
{

    neu_json_datatag_req_tag_t *p_tag = req->tags;
    for (int i = 0; i < req->n_tag; i++) {
        free(p_tag->name);
        free(p_tag->address);
        p_tag++;
    }

    free(req->tags);

    free(req);
}

int neu_json_encode_datatag_resp(void *json_object, void *param)
{
    int                      ret  = 0;
    neu_json_datatag_resp_t *resp = (neu_json_datatag_resp_t *) param;

    void *                       tag_array = json_array();
    neu_json_datatag_resp_tag_t *p_tag     = resp->tags;
    for (int i = 0; i < resp->n_tag; i++) {
        neu_json_elem_t tag_elems[] = { {
                                            .name      = "type",
                                            .t         = NEU_JSON_INT,
                                            .v.val_int = p_tag->type,
                                        },
                                        {
                                            .name      = "name",
                                            .t         = NEU_JSON_STR,
                                            .v.val_str = p_tag->name,
                                        },
                                        {
                                            .name      = "attribute",
                                            .t         = NEU_JSON_INT,
                                            .v.val_int = p_tag->attribute,
                                        },
                                        {
                                            .name      = "address",
                                            .t         = NEU_JSON_STR,
                                            .v.val_str = p_tag->address,
                                        } };
        tag_array = neu_json_encode_array(tag_array, tag_elems,
                                          NEU_JSON_ELEM_SIZE(tag_elems));
        p_tag++;
    }

    neu_json_elem_t resp_elems[] = { {
        .name         = "tags",
        .t            = NEU_JSON_OBJECT,
        .v.val_object = tag_array,
    } };
    ret = neu_json_encode_field(json_object, resp_elems,
                                NEU_JSON_ELEM_SIZE(resp_elems));

    return ret;
}
