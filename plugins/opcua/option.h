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

#ifndef NEURON_PLUGIN_OPEN62541_OPTION
#define NEURON_PLUGIN_OPEN62541_OPTION

#ifdef __cplusplus
extern "C" {
#endif

#include <neuron.h>

typedef struct {
    char *host;
    char *port;
    char *username;
    char *password;
    char *cert_file;
    char *key_file;
    char *default_cert_file;
    char *default_key_file;
} option_t;

int  opcua_option_init(option_t *option);
void opcua_option_uninit(option_t *option);
int  opcua_option_init_by_config(neu_config_t *config, option_t *option);

#ifdef __cplusplus
}
#endif
#endif