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

#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <jwt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "persist/persist.h"
#include "user.h"
#include "utils/asprintf.h"
#include "utils/log.h"

#include "argparse.h"
#include "parser/neu_json_login.h"
#include "json/neu_json_fn.h"

#include "handle.h"
#include "utils/http.h"
#include "utils/neu_jwt.h"

#include "normal_handle.h"

void handle_ping(nng_aio *aio)
{
    neu_http_ok(aio, "{}");
}

void handle_login(nng_aio *aio)
{
    NEU_PROCESS_HTTP_REQUEST(
        aio, neu_json_login_req_t, neu_json_decode_login_req, {
            neu_json_login_resp_t login_resp = { 0 };
            neu_user_t *          user       = neu_load_user(req->name);
            int                   pass_len   = strlen(req->pass);

            if (NULL == user) {
                nlog_error("could not find user `%s`", req->name);
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_INVALID_USER_OR_PASSWORD, {
                    neu_http_response(aio, error_code.error, result_error);
                });
            } else if (pass_len < NEU_USER_PASSWORD_MIN_LEN ||
                       pass_len > NEU_USER_PASSWORD_MAX_LEN) {
                nlog_error("user `%s` password too short or too long",
                           req->name);
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_INVALID_PASSWORD_LEN, {
                    neu_http_response(aio, error_code.error, result_error);
                });
            } else if (neu_user_check_password(user, req->pass)) {

                char *token  = NULL;
                char *result = NULL;

                int ret = neu_jwt_new(&token);
                if (ret != 0) {
                    NEU_JSON_RESPONSE_ERROR(NEU_ERR_NEED_TOKEN, {
                        neu_http_response(aio, error_code.error, result_error);
                        jwt_free_str(token);
                    });
                }

                login_resp.token = token;

                neu_json_encode_by_fn(&login_resp, neu_json_encode_login_resp,
                                      &result);
                neu_http_ok(aio, result);
                jwt_free_str(token);
                free(result);
            } else {
                nlog_error("user `%s` password check fail", req->name);
                NEU_JSON_RESPONSE_ERROR(NEU_ERR_INVALID_USER_OR_PASSWORD, {
                    neu_http_response(aio, error_code.error, result_error);
                });
            }

            neu_user_free(user);
        })
}

void handle_password(nng_aio *aio)
{
    NEU_PROCESS_HTTP_REQUEST_VALIDATE_JWT(
        aio, neu_json_password_req_t, neu_json_decode_password_req, {
            neu_user_t *user     = NULL;
            int         rv       = 0;
            int         pass_len = strlen(req->new_pass);

            if (pass_len < NEU_USER_PASSWORD_MIN_LEN ||
                pass_len > NEU_USER_PASSWORD_MAX_LEN) {
                nlog_error("user `%s` new password too short or too long",
                           req->name);
                rv = NEU_ERR_INVALID_PASSWORD_LEN;
            } else if (NULL == (user = neu_load_user(req->name))) {
                nlog_error("could not find user `%s`", req->name);
                rv = NEU_ERR_INVALID_USER_OR_PASSWORD;
            } else if (!neu_user_check_password(user, req->old_pass)) {
                nlog_error("user `%s` password check fail", req->name);
                rv = NEU_ERR_INVALID_USER_OR_PASSWORD;
            } else if (0 !=
                       (rv = neu_user_update_password(user, req->new_pass))) {
                nlog_error("user `%s` update password fail", req->name);
            } else if (0 != (rv = neu_save_user(user))) {
                nlog_error("user `%s` persist fail", req->name);
            }

            NEU_JSON_RESPONSE_ERROR(rv, {
                neu_http_response(aio, error_code.error, result_error);
            });

            neu_user_free(user);
        })
}

static char *file_string_read(size_t *length, const char *const path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        errno   = 0;
        *length = 0;
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    *length = (size_t) ftell(fp);
    if (0 == *length) {
        fclose(fp);
        return NULL;
    }

    char *data = NULL;
    data       = (char *) malloc((*length + 1) * sizeof(char));
    if (NULL != data) {
        data[*length] = '\0';
        fseek(fp, 0, SEEK_SET);
        size_t read = fread(data, sizeof(char), *length, fp);
        if (read != *length) {
            *length = 0;
            free(data);
            data = NULL;
        }
    } else {
        *length = 0;
    }

    fclose(fp);
    return data;
}

static struct {
    const char *plugin;
    const char *schema;
} g_plugin_to_schema_map_[] = {
    {
        .plugin = "Beckhoff ADS",
        .schema = "ads",
    },
    {
        .plugin = "BACnet/IP",
        .schema = "bacnet",
    },
    {
        .plugin = "DLT645-2007",
        .schema = "dlt645",
    },
    {
        .plugin = "eKuiper",
        .schema = "ekuiper",
    },
    {
        .plugin = "EtherNet/IP(CIP)",
        .schema = "EtherNet-IP",
    },
    {
        .plugin = "Fanuc Focas Ethernet",
        .schema = "focas",
    },
    {
        .plugin = "File",
        .schema = "file",
    },
    {
        .plugin = "HJ212-2017",
        .schema = "hj",
    },
    {
        .plugin = "IEC60870-5-104",
        .schema = "iec104",
    },
    {
        .plugin = "IEC61850",
        .schema = "iec61850",
    },
    {
        .plugin = "KNXnet/IP",
        .schema = "knx",
    },
    {
        .plugin = "Mitsubishi 3E",
        .schema = "qna3e",
    },
    {
        .plugin = "Mitsubishi 1E",
        .schema = "a1e",
    },
    {
        .plugin = "Modbus RTU",
        .schema = "modbus-rtu",
    },
    {
        .plugin = "Modbus TCP",
        .schema = "modbus-tcp",
    },
    {
        .plugin = "Monitor",
        .schema = "monitor",
    },
    {
        .plugin = "MQTT",
        .schema = "mqtt",
    },
    {
        .plugin = "NON A11",
        .schema = "nona11",
    },
    {
        .plugin = "OPC UA",
        .schema = "opcua",
    },
    {
        .plugin = "Omron FINS",
        .schema = "fins",
    },
    {
        .plugin = "Profinet",
        .schema = "profinet",
    },
    {
        .plugin = "Siemens S7 ISOTCP",
        .schema = "s7comm",
    },
    {
        .plugin = "Siemens S7 ISOTCP for 300/400",
        .schema = "s7comm_for_300",
    },
    {
        .plugin = "Siemens FetchWrite",
        .schema = "s5fetch-write",
    },
    {
        .plugin = "SparkPlugB",
        .schema = "sparkplugb",
    },
    {
        .plugin = "WebSocket",
        .schema = "websocket",
    },
};

static inline const char *plugin_name_to_schema_name(const char *name)
{
    for (size_t i = 0; i <
         sizeof(g_plugin_to_schema_map_) / sizeof(g_plugin_to_schema_map_[0]);
         ++i) {
        if (0 == strcmp(name, g_plugin_to_schema_map_[i].plugin)) {
            return g_plugin_to_schema_map_[i].schema;
        }
    }

    return name;
}

void handle_get_plugin_schema(nng_aio *aio)
{
    size_t      len                        = 0;
    char *      schema_path                = NULL;
    char        param[NEU_PLUGIN_NAME_LEN] = { 0 };
    const char *schema_name                = param;

    NEU_VALIDATE_JWT(aio);

    int rv = neu_http_get_param_str(aio, "schema_name", param, sizeof(param));
    if (-2 == rv) {
        // fall back to `plugin_name` param
        rv = neu_http_get_param_str(aio, "plugin_name", param, sizeof(param));
        schema_name = plugin_name_to_schema_name(param);
    }
    if (rv < 0) {
        neu_http_bad_request(aio, "{\"error\": 1002}");
        return;
    }

    if (0 > neu_asprintf(&schema_path, "%s/schema/%s.json", g_plugin_dir,
                         schema_name)) {
        NEU_JSON_RESPONSE_ERROR(NEU_ERR_EINTERNAL, {
            neu_http_response(aio, error_code.error, result_error);
        });
        return;
    }

    char *buf = NULL;

    buf = file_string_read(&len, schema_path);
    if (NULL == buf) {
        free(schema_path);
        if (0 > neu_asprintf(&schema_path, "%s/custom/schema/%s.json",
                             g_plugin_dir, schema_name)) {
            NEU_JSON_RESPONSE_ERROR(NEU_ERR_EINTERNAL, {
                neu_http_response(aio, error_code.error, result_error);
            });
            return;
        }
        buf = file_string_read(&len, schema_path);
    }

    if (NULL == buf) {
        free(schema_path);
        if (0 > neu_asprintf(&schema_path, "%s/system/schema/%s.json",
                             g_plugin_dir, schema_name)) {
            NEU_JSON_RESPONSE_ERROR(NEU_ERR_EINTERNAL, {
                neu_http_response(aio, error_code.error, result_error);
            });
            return;
        }
        buf = file_string_read(&len, schema_path);
    }

    if (NULL == buf) {
        nlog_info("open %s error: %d", schema_path, errno);
        neu_http_not_found(aio, "{\"status\": \"error\"}");
        free(schema_path);
        return;
    }

    neu_http_ok(aio, buf);
    free(buf);
    free(schema_path);
}

void handle_status(nng_aio *aio)
{
    char *result = NULL;

    NEU_VALIDATE_JWT(aio);

    int ret = neu_asprintf(&result, "{\"status\":\"%s\"}", g_status);

    if (ret < 0) {
        NEU_JSON_RESPONSE_ERROR(NEU_ERR_EINTERNAL, {
            neu_http_response(aio, error_code.error, result_error);
        });
        return;
    }

    neu_http_response(aio, 0, result);
    free(result);
}
