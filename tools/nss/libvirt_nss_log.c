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

#include <config.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "libvirt_nss_log.h"
#include "libvirt_nss.h"

#define NULLSTR(s) ((s) ? (s) : "<null>")

static const char *  __attribute__((returns_nonnull))
nssLogPriorityToString(nssLogPriority prio)
{
    switch (prio) {
    case NSS_DEBUG:
        return "DEBUG";
    case NSS_ERROR:
        return "ERROR";
    }

    return "";
}

void
nssLog(nssLogPriority prio,
       const char *func,
       int linenr,
       const char *fmt, ...)
{
    int saved_errno = errno;
    const size_t ebuf_size = 512;
    g_autofree char *ebuf = NULL;
    va_list ap;

    fprintf(stderr, "%s %s:%d : ", nssLogPriorityToString(prio), func, linenr);

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    switch (prio) {
    case NSS_DEBUG:
        break;

    case NSS_ERROR:
        ebuf = calloc(ebuf_size, sizeof(*ebuf));
        if (ebuf)
            strerror_r(saved_errno, ebuf, ebuf_size);
        fprintf(stderr, " : %s", NULLSTR(ebuf));
        break;
    }

    fprintf(stderr, "\n");
}
