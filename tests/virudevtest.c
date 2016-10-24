/*
 * Copyright (C) 2016 Red Hat, Inc.
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
 *
 * Author: Michal Privoznik <mprivozn@redhat.com>
 */

#include <config.h>

#include "testutils.h"
#include "virudev.h"
#include "virjson.h"

#define VIR_FROM_THIS VIR_FROM_NONE

struct testUdevData {
    const char *file;
    const char *const *labels;
};


static int
testDump(const void *opaque)
{
    const struct testUdevData *data = opaque;
    virUdevMgrPtr mgr = NULL;
    int ret = -1;
    const char * const *tmp;
    char *state = NULL;
    char *filename = NULL;

    if (virAsprintf(&filename, "%s/virudevtestdata/%s.json",
                    abs_srcdir, data->file) < 0)
        goto cleanup;

    if (!(mgr = virUdevMgrNew()))
        goto cleanup;

    tmp = data->labels;
    while (*tmp) {
        const char *device;
        const char *model;
        const char *label;
        virSecurityDeviceLabelDefPtr seclabel;

        device = *tmp;
        if (!++tmp) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s", "Invalid seclabels list");
            goto cleanup;
        }
        model = *tmp;
        if (!++tmp) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s", "Invalid seclabels list");
            goto cleanup;
        }
        label = *tmp;
        tmp++;

        if (!(seclabel = virSecurityDeviceLabelDefNewLabel(model, label)))
            goto cleanup;

        if (virUdevMgrAddLabel(mgr, device, seclabel) < 0)
            goto cleanup;
        virSecurityDeviceLabelDefFree(seclabel);
    }

    if (!(state = virUdevMgrDumpStr(mgr)))
        goto cleanup;

    if (virTestCompareToFile(state, filename) < 0)
        goto cleanup;

    ret = 0;
 cleanup:
    VIR_FREE(filename);
    VIR_FREE(state);
    virObjectUnref(mgr);
    return ret;
}


static int
mymain(void)
{
    int ret = 0;

#define DO_TEST_DUMP(filename, ...)                                 \
    do {                                                            \
        const char *labels[] = {__VA_ARGS__, NULL};                 \
        struct testUdevData data = {                                \
            .file = filename, .labels = labels,                     \
        };                                                          \
        if (virTestRun("Dump " filename, testDump, &data) < 0)      \
            ret = -1;                                               \
    } while (0)

    DO_TEST_DUMP("empty", NULL);
    DO_TEST_DUMP("simple-selinux",
                 "/dev/sda", "selinux", "someSELinuxLabel");
    DO_TEST_DUMP("simple-dac",
                 "/dev/sda", "dac", "someDACLabel");
    DO_TEST_DUMP("complex",
                 "/dev/sda", "dac",     "someDACLabel",
                 "/dev/sda", "selinux", "someSELinuxLabel",
                 "/dev/sdb", "dac",     "otherDACLabel",
                 "/dev/sdb", "selinux", "otherSELinuxLabel");

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

VIRT_TEST_MAIN_PRELOAD(mymain, abs_builddir "/.libs/virudevmock.so")
