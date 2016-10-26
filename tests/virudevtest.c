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
    virUdevMgrFilter filter;
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

    virUdevMgrSetFilter(mgr, data->filter, NULL);

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
testParse(const void *opaque)
{
    const struct testUdevData *data = opaque;
    virUdevMgrPtr mgr = NULL;
    char *filename = NULL;
    char *state = NULL;
    int ret = -1;

    if (virAsprintf(&filename, "%s/virudevtestdata/%s.json",
                    abs_srcdir, data->file) < 0)
        goto cleanup;

    if (!(mgr = virUdevMgrNewFromFile(filename)))
        goto cleanup;

    if (!(state = virUdevMgrDumpStr(mgr)))
        goto cleanup;

    if (virTestCompareToFile(state, filename))
        goto cleanup;

    ret = 0;
 cleanup:
    VIR_FREE(state);
    VIR_FREE(filename);
    virObjectUnref(mgr);
    return ret;
}


static int
testLookup(const void *opaque)
{
    const struct testUdevData *data = opaque;
    virUdevMgrPtr mgr = NULL;
    int ret = -1;
    const char * const *tmp;
    char *filename = NULL;
    virSecurityDeviceLabelDefPtr *seclabels = NULL;
    size_t i, nseclabels = 0;

    if (virAsprintf(&filename, "%s/virudevtestdata/%s.json",
                    abs_srcdir, data->file) < 0)
        goto cleanup;

    if (!(mgr = virUdevMgrNewFromFile(filename)))
        goto cleanup;

    tmp = data->labels;
    while (*tmp) {
        const char *device;
        const char *model;
        const char *label;

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

        if (virUdevMgrLookupLabels(mgr, device, &seclabels, &nseclabels) < 0)
            goto cleanup;

        for (i = 0; i < nseclabels; i++) {
            virSecurityDeviceLabelDefPtr seclabel = seclabels[i];

            if (STREQ(seclabel->model, model) &&
                STREQ(seclabel->label, label))
                break;
        }

        if (i == nseclabels) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s", "Label not found");
            goto cleanup;
        }

        for (i = 0; i < nseclabels; i++)
            virSecurityDeviceLabelDefFree(seclabels[i]);
        VIR_FREE(seclabels);
        nseclabels = 0;
    }

    ret = 0;
 cleanup:
    for (i = 0; i < nseclabels; i++)
        virSecurityDeviceLabelDefFree(seclabels[i]);
    VIR_FREE(seclabels);
    VIR_FREE(filename);
    virObjectUnref(mgr);
    return ret;
}


static int
filterAll(const char *device ATTRIBUTE_UNUSED,
          const virSecurityDeviceLabelDef *seclabel ATTRIBUTE_UNUSED,
          void *opaque ATTRIBUTE_UNUSED)
{
    return 0;
}


static int
filterAllowSda(const char *device,
               const virSecurityDeviceLabelDef *seclabel ATTRIBUTE_UNUSED,
               void *opaque ATTRIBUTE_UNUSED)
{
    return STRPREFIX(device, "/dev/sda") ? 1 : 0;
}



static int
mymain(void)
{
    int ret = 0;

#define DO_TEST_DUMP(filename, ...)                                 \
    do {                                                            \
        const char *labels[] = {__VA_ARGS__, NULL};                 \
        struct testUdevData data = {                                \
            .file = filename, .labels = labels, .filter = NULL,     \
        };                                                          \
        if (virTestRun("Dump " filename, testDump, &data) < 0)      \
            ret = -1;                                               \
    } while (0)

#define DO_TEST_PARSE(filename)                                     \
    do {                                                            \
        struct testUdevData data = {                                \
            .file = filename,                                       \
        };                                                          \
        if (virTestRun("Parse " filename, testParse, &data) < 0)    \
            ret = -1;                                               \
    } while (0)

#define DO_TEST_LOOKUP(filename, ...)                               \
    do {                                                            \
        const char *labels[] = {__VA_ARGS__, NULL};                 \
        struct testUdevData data = {                                \
            .file = filename, .labels = labels,                     \
        };                                                          \
        if (virTestRun("Lookup " filename, testLookup, &data) < 0)  \
            ret = -1;                                               \
    } while (0)

#define DO_TEST_FILTER(filename, fltr, ...)                         \
    do {                                                            \
        const char *labels[] = {__VA_ARGS__, NULL};                 \
        struct testUdevData data = {                                \
            .file = filename, .labels = labels, .filter = fltr,     \
        };                                                          \
        if (virTestRun("Filter " filename, testDump, &data) < 0)    \
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

    DO_TEST_PARSE("empty");
    DO_TEST_PARSE("simple-selinux");
    DO_TEST_PARSE("simple-dac");
    DO_TEST_PARSE("complex");

    DO_TEST_LOOKUP("empty", NULL);
    DO_TEST_LOOKUP("simple-selinux",
                   "/dev/sda", "selinux", "someSELinuxLabel");
    DO_TEST_LOOKUP("simple-dac",
                   "/dev/sda", "dac", "someDACLabel");
    DO_TEST_LOOKUP("complex",
                   "/dev/sda", "dac",     "someDACLabel",
                   "/dev/sda", "selinux", "someSELinuxLabel",
                   "/dev/sdb", "dac",     "otherDACLabel",
                   "/dev/sdb", "selinux", "otherSELinuxLabel");

    DO_TEST_FILTER("empty", filterAll,
                   "/dev/sda", "dac",     "someDACLabel",
                   "/dev/sda", "selinux", "someSELinuxLabel",
                   "/dev/sdb", "dac",     "otherDACLabel",
                   "/dev/sdb", "selinux", "otherSELinuxLabel");
    DO_TEST_FILTER("simple-selinux", filterAllowSda,
                   "/dev/sda", "selinux", "someSELinuxLabel",
                   "/dev/sdb", "dac",     "otherDACLabel",
                   "/dev/sdb", "selinux", "otherSELinuxLabel");
    DO_TEST_FILTER("simple-dac", filterAllowSda,
                   "/dev/sda", "dac", "someDACLabel",
                   "/dev/sdb", "dac",     "otherDACLabel",
                   "/dev/sdb", "selinux", "otherSELinuxLabel");
    DO_TEST_FILTER("complex", NULL,
                   "/dev/sda", "dac",     "someDACLabel",
                   "/dev/sda", "selinux", "someSELinuxLabel",
                   "/dev/sdb", "dac",     "otherDACLabel",
                   "/dev/sdb", "selinux", "otherSELinuxLabel");

    return ret == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

VIRT_TEST_MAIN_PRELOAD(mymain, abs_builddir "/.libs/virudevmock.so")
