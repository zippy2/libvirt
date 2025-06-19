/*
 * libvirt_nss_log: Logging for Name Service Switch plugin
 *
 * Copyright (C) 2025 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

typedef enum {
    NSS_DEBUG,
    NSS_ERROR,
} nssLogPriority;

#define DEBUG(...) \
    nssLog(NSS_DEBUG, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define ERROR(...) \
    nssLog(NSS_ERROR, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

#define NSS_LOG_ENV_VAR "LIBVIRT_NSS_DEBUG"

void
nssLog(nssLogPriority prio,
       const char *filename,
       const char *func,
       int linenr,
       const char *fmt, ...) __attribute__ ((format(printf, 5, 6)));
