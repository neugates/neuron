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

#ifndef NEU_PLUGIN_MONITOR_H
#define NEU_PLUGIN_MONITOR_H

#include <pthread.h>

#include <nng/nng.h>
#include <nng/supplemental/http/http.h>

#include "adapter.h"
#include "connection/mqtt_client.h"
#include "event/event.h"
#include "monitor_config.h"
#include "plugin.h"

struct neu_plugin {
    neu_plugin_common_t common;
    bool                started;
    neu_events_t *      events;
    neu_event_timer_t * heartbeat_timer;
    monitor_config_t *  config;
    neu_mqtt_client_t * mqtt_client;
};

neu_plugin_t *neu_monitor_get_plugin();

#endif
