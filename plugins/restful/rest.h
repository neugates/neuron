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

#ifndef _NEU_PLUGIN_REST_H_
#define _NEU_PLUGIN_REST_H_

#include "handle.h"
#include "plugin.h"
#include <nng/nng.h>
#include <nng/supplemental/http/http.h>

extern const neu_plugin_module_t default_dashboard_plugin_module;

struct neu_plugin {
    neu_plugin_common_t    common;
    nng_http_server *      server;
    neu_rest_handle_ctx_t *handle_ctx;
};

#endif
