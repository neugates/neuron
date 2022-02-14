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

#include "neu_log.h"
#include <syslog.h>

static struct {
    int  level;
    bool quiet;
} L;

static const char *level_strings[] = { "TRACE", "DEBUG", "INFO",
                                       "WARN",  "ERROR", "FATAL" };

static inline int to_syslog_priority(int level)
{
    int priority = LOG_ERR;
    switch (level) {
    case NEU_LOG_TRACE:
        // fall through
    case NEU_LOG_DEBUG:
        level = LOG_DEBUG;
        break;
    case NEU_LOG_INFO:
        level = LOG_INFO;
        break;
    case NEU_LOG_WARN:
        level = LOG_WARNING;
        break;
    case NEU_LOG_ERROR:
        level = LOG_ERR;
        break;
    case NEU_LOG_FATAL:
        level = LOG_ALERT;
        break;
    }

    return priority;
}

const char *log_level_string(int level)
{
    return level_strings[level];
}

void log_set_level(int level)
{
    L.level = level;
}

void log_set_quiet(bool enable)
{
    L.quiet = enable;
}

#ifdef PROJECT_ROOT_DIR
#define PRJ_ROOT_DIR_LEN sizeof(PROJECT_ROOT_DIR);
#else
#define PRJ_ROOT_DIR_LEN 0
#endif

void log_log(int level, const char *file, int line, const char *func,
             const char *label, const char *fmt, ...)
{
    if (L.quiet || level < L.level) {
        return;
    }

    va_list     ap;
    const char *file_name = file + PRJ_ROOT_DIR_LEN;
    int         priority  = to_syslog_priority(level);
    char        buf[1024] = { 0 };

    va_start(ap, fmt);

    size_t n = snprintf(buf, sizeof(buf), "[%s] %s:%d %s: ", label, file_name,
                        line, func);
    if (n < sizeof(buf)) {
        // may truncate, but don't care for now
        vsnprintf(buf + n, sizeof(buf) - n, fmt, ap);
    }
    syslog(priority, "%s", buf);

    va_end(ap);
}
