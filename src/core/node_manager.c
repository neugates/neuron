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
#include <string.h>

#include "errcodes.h"

#include "adapter/adapter_internal.h"
#include "node_manager.h"

typedef struct node_entity {
    char *name;

    neu_adapter_t *    adapter;
    bool               is_static;
    bool               display;
    bool               single;
    bool               is_monitor;
    struct sockaddr_un addr;

    UT_hash_handle hh;
} node_entity_t;

struct neu_node_manager {
    node_entity_t *nodes;
    UT_array *     monitors;
};

neu_node_manager_t *neu_node_manager_create()
{
    neu_node_manager_t *node_manager = calloc(1, sizeof(neu_node_manager_t));

    if (node_manager) {
        utarray_new(node_manager->monitors, &ut_ptr_icd);
    }

    return node_manager;
}

void neu_node_manager_destroy(neu_node_manager_t *mgr)
{
    node_entity_t *el = NULL, *tmp = NULL;

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        HASH_DEL(mgr->nodes, el);
        free(el->name);
        free(el);
    }

    utarray_free(mgr->monitors);
    free(mgr);
}

int neu_node_manager_add(neu_node_manager_t *mgr, neu_adapter_t *adapter)
{
    node_entity_t *node = calloc(1, sizeof(node_entity_t));

    node->adapter = adapter;
    node->name    = strdup(adapter->name);
    node->display = true;
    node->is_monitor =
        (0 == strcmp(node->adapter->module->module_name, "Monitor"));

    HASH_ADD_STR(mgr->nodes, name, node);

    if (node->is_monitor) {
        utarray_push_back(mgr->monitors, &node);
    }

    return 0;
}

int neu_node_manager_add_static(neu_node_manager_t *mgr, neu_adapter_t *adapter)
{
    node_entity_t *node = calloc(1, sizeof(node_entity_t));

    node->adapter   = adapter;
    node->name      = strdup(adapter->name);
    node->is_static = true;
    node->display   = true;

    HASH_ADD_STR(mgr->nodes, name, node);

    return 0;
}

int neu_node_manager_add_single(neu_node_manager_t *mgr, neu_adapter_t *adapter,
                                bool display)
{
    node_entity_t *node = calloc(1, sizeof(node_entity_t));

    node->adapter = adapter;
    node->name    = strdup(adapter->name);
    node->display = display;
    node->single  = true;

    HASH_ADD_STR(mgr->nodes, name, node);

    return 0;
}

int neu_node_manager_update_name(neu_node_manager_t *mgr, const char *node_name,
                                 const char *new_node_name)
{
    node_entity_t *node = NULL;

    HASH_FIND_STR(mgr->nodes, node_name, node);
    if (NULL == node) {
        return NEU_ERR_NODE_NOT_EXIST;
    }

    char *new_name = strdup(new_node_name);
    if (NULL == new_name) {
        return NEU_ERR_EINTERNAL;
    }

    HASH_DEL(mgr->nodes, node);
    free(node->name);
    node->name = new_name;
    HASH_ADD_STR(mgr->nodes, name, node);

    return 0;
}

int neu_node_manager_update(neu_node_manager_t *mgr, const char *name,
                            struct sockaddr_un addr)
{
    node_entity_t *node = NULL;

    HASH_FIND_STR(mgr->nodes, name, node);
    if (NULL == node) {
        return -1;
    }
    node->addr = addr;

    return 0;
}

void neu_node_manager_del(neu_node_manager_t *mgr, const char *name)
{
    node_entity_t *node = NULL;

    HASH_FIND_STR(mgr->nodes, name, node);
    if (node != NULL) {
        HASH_DEL(mgr->nodes, node);
        if (node->is_monitor) {
            utarray_erase(mgr->monitors, utarray_eltidx(mgr->monitors, node),
                          1);
        }
        free(node->name);
        free(node);
    }
}

uint16_t neu_node_manager_size(neu_node_manager_t *mgr)
{
    return HASH_COUNT(mgr->nodes);
}

bool neu_node_manager_exist_uninit(neu_node_manager_t *mgr)
{
    node_entity_t *el = NULL, *tmp = NULL;

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        if (el->addr.sun_path[1] == 0) {
            return true;
        }
    }

    return false;
}

UT_array *neu_node_manager_get(neu_node_manager_t *mgr, int type)
{
    UT_array *     array = NULL;
    UT_icd         icd   = { sizeof(neu_resp_node_info_t), NULL, NULL, NULL };
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(array, &icd);

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        if (!el->is_static && el->display) {
            if (el->adapter->module->type & type) {
                neu_resp_node_info_t info = { 0 };
                strcpy(info.node, el->adapter->name);
                strcpy(info.plugin, el->adapter->module->module_name);
                utarray_push_back(array, &info);
            }
        }
    }

    return array;
}

UT_array *neu_node_manager_filter(neu_node_manager_t *mgr, int type,
                                  const char *plugin, const char *node)
{
    UT_array *     array = NULL;
    UT_icd         icd   = { sizeof(neu_resp_node_info_t), NULL, NULL, NULL };
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(array, &icd);

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        if (!el->is_static && el->display) {
            if (el->adapter->module->type & type) {
                if (strlen(plugin) > 0 &&
                    strcmp(el->adapter->module->module_name, plugin) != 0) {
                    continue;
                }
                if (strlen(node) > 0 &&
                    strstr(el->adapter->name, node) == NULL) {
                    continue;
                }
                neu_resp_node_info_t info = { 0 };
                strcpy(info.node, el->adapter->name);
                strcpy(info.plugin, el->adapter->module->module_name);
                utarray_push_back(array, &info);
            }
        }
    }

    return array;
}

UT_array *neu_node_manager_get_all(neu_node_manager_t *mgr)
{
    UT_array *     array = NULL;
    UT_icd         icd   = { sizeof(neu_resp_node_info_t), NULL, NULL, NULL };
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(array, &icd);

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        neu_resp_node_info_t info = { 0 };
        strcpy(info.node, el->adapter->name);
        strcpy(info.plugin, el->adapter->module->module_name);
        utarray_push_back(array, &info);
    }

    return array;
}

UT_array *neu_node_manager_get_adapter(neu_node_manager_t *mgr, int type)
{
    UT_array *     array = NULL;
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(array, &ut_ptr_icd);

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        if (!el->is_static && el->display) {
            if (el->adapter->module->type & type) {
                utarray_push_back(array, &el->adapter);
            }
        }
    }

    return array;
}

neu_adapter_t *neu_node_manager_find(neu_node_manager_t *mgr, const char *name)
{
    neu_adapter_t *adapter = NULL;
    node_entity_t *node    = NULL;

    HASH_FIND_STR(mgr->nodes, name, node);
    if (node != NULL) {
        adapter = node->adapter;
    }

    return adapter;
}

bool neu_node_manager_is_single(neu_node_manager_t *mgr, const char *name)
{
    node_entity_t *node = NULL;

    HASH_FIND_STR(mgr->nodes, name, node);
    if (node != NULL) {
        return node->single;
    }

    return false;
}

bool neu_node_manager_is_driver(neu_node_manager_t *mgr, const char *name)
{
    node_entity_t *node = NULL;

    HASH_FIND_STR(mgr->nodes, name, node);
    if (node != NULL) {
        return NEU_NA_TYPE_DRIVER == node->adapter->module->type;
    }

    return false;
}

UT_array *neu_node_manager_get_addrs(neu_node_manager_t *mgr, int type)
{
    UT_icd         icd   = { sizeof(struct sockaddr_un), NULL, NULL, NULL };
    UT_array *     addrs = NULL;
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(addrs, &icd);

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        if (!el->is_static) {
            if (el->adapter->module->type & type) {
                utarray_push_back(addrs, &el->addr);
            }
        }
    }

    return addrs;
}

UT_array *neu_node_manager_get_addrs_all(neu_node_manager_t *mgr)
{
    UT_icd         icd   = { sizeof(struct sockaddr_un), NULL, NULL, NULL };
    UT_array *     addrs = NULL;
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(addrs, &icd);

    HASH_ITER(hh, mgr->nodes, el, tmp) { utarray_push_back(addrs, &el->addr); }

    return addrs;
}

struct sockaddr_un neu_node_manager_get_addr(neu_node_manager_t *mgr,
                                             const char *        name)
{
    struct sockaddr_un addr = { 0 };
    node_entity_t *    node = NULL;

    HASH_FIND_STR(mgr->nodes, name, node);
    if (node != NULL) {
        addr = node->addr;
    }

    return addr;
}

bool neu_node_manager_is_monitor(neu_node_manager_t *mgr, const char *name)
{
    node_entity_t *node = NULL;
    HASH_FIND_STR(mgr->nodes, name, node);
    return node ? node->is_monitor : false;
}
int neu_node_manager_for_each_monitor(neu_node_manager_t *mgr,
                                      int (*cb)(const char *       name,
                                                struct sockaddr_un addr,
                                                void *             data),
                                      void *data)
{
    int rv = 0;
    utarray_foreach(mgr->monitors, node_entity_t **, node)
    {
        rv = cb((*node)->name, (*node)->addr, data);
        if (0 != rv) {
            break;
        }
    }
    return rv;
}

UT_array *neu_node_manager_get_state(neu_node_manager_t *mgr)
{
    UT_icd         icd    = { sizeof(neu_nodes_state_t), NULL, NULL, NULL };
    UT_array *     states = NULL;
    node_entity_t *el = NULL, *tmp = NULL;

    utarray_new(states, &icd);

    HASH_ITER(hh, mgr->nodes, el, tmp)
    {
        neu_nodes_state_t state = { 0 };

        if (!el->is_static && el->display) {
            strcpy(state.node, el->adapter->name);
            state.state.running = el->adapter->state;
            state.state.link =
                neu_plugin_to_plugin_common(el->adapter->plugin)->link_state;
            state.state.log_level = el->adapter->log_level;
            state.is_driver = (el->adapter->module->type == NEU_NA_TYPE_DRIVER)
                ? true
                : false;
            neu_metric_entry_t *e = NULL;
            if (NULL != el->adapter->metrics) {
                HASH_FIND_STR(el->adapter->metrics->entries,
                              NEU_METRIC_LAST_RTT_MS, e);
            }
            state.rtt = NULL != e ? e->value : 0;

            utarray_push_back(states, &state);
        }
    }

    return states;
}
