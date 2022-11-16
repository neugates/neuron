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

#include <stdlib.h>

#include "parser/neu_json_plugin.h"
#include "plugin.h"
#include "json/neu_json_fn.h"

#include "handle.h"
#include "http.h"

#include "plugin_handle.h"

void handle_add_plugin(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST_VALIDATE_JWT(
        aio, neu_json_add_plugin_req_t, neu_json_decode_add_plugin_req, {
            int                  ret    = 0;
            neu_reqresp_head_t   header = { 0 };
            neu_req_add_plugin_t cmd    = { 0 };

            header.ctx  = aio;
            header.type = NEU_REQ_ADD_PLUGIN;
            strcpy(cmd.library, req->library);
            ret = neu_plugin_op(plugin, header, &cmd);
            if (ret != 0) {
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_IS_BUSY, {
                    http_response(aio, NEU_ERR_IS_BUSY, result_error);
                });
            }
        })
}

void handle_del_plugin(nng_aio *aio)
{
    neu_plugin_t *plugin = neu_rest_get_plugin();

    REST_PROCESS_HTTP_REQUEST_VALIDATE_JWT(
        aio, neu_json_del_plugin_req_t, neu_json_decode_del_plugin_req, {
            int                  ret    = 0;
            neu_reqresp_head_t   header = { 0 };
            neu_req_del_plugin_t cmd    = { 0 };

            header.ctx  = aio;
            header.type = NEU_REQ_DEL_PLUGIN;
            strcpy(cmd.plugin, req->plugin);
            ret = neu_plugin_op(plugin, header, &cmd);
            if (ret != 0) {
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_IS_BUSY, {
                    http_response(aio, NEU_ERR_IS_BUSY, result_error);
                });
            }
        })
}

void handle_get_plugin(nng_aio *aio)
{
    neu_plugin_t *     plugin = neu_rest_get_plugin();
    neu_reqresp_head_t header = { 0 };
    int                ret    = 0;

    VALIDATE_JWT(aio);

    header.ctx  = aio;
    header.type = NEU_REQ_GET_PLUGIN;

    ret = neu_plugin_op(plugin, header, NULL);
    if (ret != 0) {
        NEU_JSON_RESPONSE_ERROR(NEU_ERR_IS_BUSY, {
            http_response(aio, NEU_ERR_IS_BUSY, result_error);
        });
    }
}

void handle_get_plugin_resp(nng_aio *aio, neu_resp_get_plugin_t *plugins)
{
    neu_json_get_plugin_resp_t plugin_res = { 0 };
    char *                     result     = NULL;

    plugin_res.n_plugin_lib = utarray_len(plugins->plugins);
    plugin_res.plugin_libs  = calloc(
        plugin_res.n_plugin_lib, sizeof(neu_json_get_plugin_resp_plugin_lib_t));

    utarray_foreach(plugins->plugins, neu_resp_plugin_info_t *, info)
    {
        int index = utarray_eltidx(plugins->plugins, info);

        plugin_res.plugin_libs[index].node_type   = info->type;
        plugin_res.plugin_libs[index].kind        = info->kind;
        plugin_res.plugin_libs[index].name        = (char *) info->name;
        plugin_res.plugin_libs[index].library     = (char *) info->library;
        plugin_res.plugin_libs[index].description = (char *) info->description;
        plugin_res.plugin_libs[index].description_zh =
            (char *) info->description_zh;
    }

    neu_json_encode_by_fn(&plugin_res, neu_json_encode_get_plugin_resp,
                          &result);

    http_ok(aio, result);
    free(result);
    free(plugin_res.plugin_libs);

    utarray_free(plugins->plugins);
}