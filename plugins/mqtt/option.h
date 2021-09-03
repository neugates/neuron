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

#ifndef NEURON_PLUGIN_PAHO_OPTION
#define NEURON_PLUGIN_PAHO_OPTION

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* debug app options */
    int   publisher; /* publisher app? */
    int   quiet;
    int   verbose;
    int   tracelevel;
    char *delimiter;
    int   maxdatalen;
    /* message options */
    char *message;
    char *filename;
    int   stdin_lines;
    int   stdlin_complete;
    int   null_message;
    /* MQTT options */
    int   MQTT_version; // 3-3_1, 4-3_3_1, 5-5
    char *topic;
    char *clientid;
    int   qos;
    int   retained;
    char *username;
    char *password;
    char *host;
    char *port;
    char *connection;
    int   keepalive;

    int keepalive_interval;
    int clean_session;
    /* will options */
    char *will_topic;
    char *will_payload;
    int   will_qos;
    int   will_retain;
    /* TLS options */
    int   insecure;
    char *capath;
    char *cert;
    char *cafile;
    char *key;
    char *keypass;
    char *ciphers;
    char *psk_identity;
    char *psk;
    /* MQTT V5 options */
    int message_expiry;
    struct {
        char *name;
        char *value;
    } user_property;
    /* websocket HTTP proxies */
    char *http_proxy;
    char *https_proxy;
} option_t;

#ifdef __cplusplus
}
#endif
#endif