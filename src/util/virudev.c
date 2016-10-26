/*
 * virudev.c: udev rules engine
 *
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

#include "internal.h"
#include "viralloc.h"
#include "virfile.h"
#include "virhash.h"
#include "virjson.h"
#include "virobject.h"
#include "virstring.h"
#include "virudev.h"

#define VIR_FROM_THIS VIR_FROM_SECURITY

struct _virUdevMgr {
    virObjectLockable parent;
    virHashTablePtr labels;

    virUdevMgrFilter filter;
    void *opaque;
};

struct _udevSeclabel {
    virSecurityDeviceLabelDefPtr *seclabels;
    size_t nseclabels;
};

typedef struct _udevSeclabel udevSeclabel;
typedef udevSeclabel *udevSeclabelPtr;

static virClassPtr virUdevMgrClass;


static virSecurityDeviceLabelDefPtr
udevSeclabelFindByModel(udevSeclabelPtr list,
                        const char *model)
{
    size_t i;

    for (i = 0; list && i < list->nseclabels; i++) {
        if (STREQ(list->seclabels[i]->model, model))
            return list->seclabels[i];
    }

    return NULL;
}


static int
udevSeclabelAppend(udevSeclabelPtr list,
                   const virSecurityDeviceLabelDef *seclabel)
{
    virSecurityDeviceLabelDefPtr copy = virSecurityDeviceLabelDefCopy(seclabel);
    if (VIR_APPEND_ELEMENT_COPY(list->seclabels, list->nseclabels, copy) < 0) {
        virSecurityDeviceLabelDefFree(copy);
        return -1;
    }
    return 0;
}


static void
udevSeclabelFree(void *payload,
                 const void *name ATTRIBUTE_UNUSED)
{
    udevSeclabelPtr list = payload;
    size_t i;

    if (!list)
        return;

    for (i = 0; i < list->nseclabels; i++)
        virSecurityDeviceLabelDefFree(list->seclabels[i]);
    VIR_FREE(list->seclabels);
    VIR_FREE(list);
}


static int
udevSeclabelUpdate(udevSeclabelPtr list,
                   const virSecurityDeviceLabelDef *seclabel)
{
    size_t i;

    for (i = 0; list && i < list->nseclabels; i++) {
        if (STREQ(list->seclabels[i]->model, seclabel->model)) {
            virSecurityDeviceLabelDefPtr copy = virSecurityDeviceLabelDefCopy(seclabel);
            if (!copy)
                return -1;
            virSecurityDeviceLabelDefFree(list->seclabels[i]);
            list->seclabels[i] = copy;
            return 0;
        }
    }

    return -1;
}


static virJSONValuePtr
udevSeclabelDump(const virSecurityDeviceLabelDef *seclabel)
{
    virJSONValuePtr object;

    if (!(object = virJSONValueNewObject()) ||
        virJSONValueObjectAppendString(object, "model", seclabel->model) < 0 ||
        virJSONValueObjectAppendString(object, "label", seclabel->label) < 0)
        goto error;

    return object;

 error:
    virJSONValueFree(object);
    return NULL;
}


static int
udevSeclabelsDump(void *payload,
                  const void *name,
                  void *opaque)
{
    udevSeclabelPtr list = payload;
    const char *device = name;
    virJSONValuePtr seclabels = opaque;
    virJSONValuePtr deviceLabels = NULL;
    virJSONValuePtr deviceJSON = NULL;
    size_t i;
    int ret = -1;

    if (!(deviceLabels = virJSONValueNewArray()))
        return ret;

    for (i = 0; i < list->nseclabels; i++) {
        virJSONValuePtr seclabel = udevSeclabelDump(list->seclabels[i]);

        if (!seclabel ||
            virJSONValueArrayAppend(deviceLabels, seclabel) < 0) {
            virJSONValueFree(seclabel);
            goto cleanup;
        }
    }

    if (!(deviceJSON = virJSONValueNewObject()) ||
        virJSONValueObjectAppendString(deviceJSON, "device", device) < 0 ||
        virJSONValueObjectAppend(deviceJSON, "labels", deviceLabels) < 0)
        goto cleanup;
    deviceLabels = NULL;

    if (virJSONValueArrayAppend(seclabels, deviceJSON) < 0)
        goto cleanup;
    deviceJSON = NULL;

    ret = 0;
 cleanup:
    virJSONValueFree(deviceJSON);
    virJSONValueFree(deviceLabels);
    return ret;
}


static udevSeclabelPtr
udevSeclabelRestore(virJSONValuePtr labels)
{
    udevSeclabelPtr ret = NULL;
    size_t i;

    if (VIR_ALLOC(ret) < 0)
        goto error;

    for (i = 0; i < virJSONValueArraySize(labels); i++) {
        virJSONValuePtr labelJSON = virJSONValueArrayGet(labels, i);
        virSecurityDeviceLabelDefPtr seclabel;
        const char *model;
        const char *label;

        if (!(model = virJSONValueObjectGetString(labelJSON, "model"))) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                           _("seclabel missing model in JSON"));
            goto error;
        }

        if (!(label = virJSONValueObjectGetString(labelJSON, "label"))) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                           _("seclabel missing label in JSON"));
            goto error;
        }

        if (!(seclabel = virSecurityDeviceLabelDefNewLabel(model, label)) ||
            udevSeclabelAppend(ret, seclabel) < 0) {
            virSecurityDeviceLabelDefFree(seclabel);
            goto error;
        }
        virSecurityDeviceLabelDefFree(seclabel);
    }

    return ret;

 error:
    udevSeclabelFree(ret, NULL);
    return NULL;
}


static void
virUdevMgrDispose(void *obj)
{
    virUdevMgrPtr mgr = obj;
    virHashFree(mgr->labels);
}


static int virUdevMgrOnceInit(void)
{
    if (!(virUdevMgrClass = virClassNew(virClassForObjectLockable(),
                                        "virUdevMgr",
                                        sizeof(virUdevMgr),
                                        virUdevMgrDispose)))
        return -1;

    return 0;
}


VIR_ONCE_GLOBAL_INIT(virUdevMgr)


virUdevMgrPtr virUdevMgrNew(void)
{
    virUdevMgrPtr mgr;

    if (virUdevMgrInitialize() < 0)
        return NULL;

    if (!(mgr = virObjectLockableNew(virUdevMgrClass)))
        return NULL;

    if (!(mgr->labels = virHashCreate(10, udevSeclabelFree)))
        goto error;

    return mgr;

 error:
    virObjectUnref(mgr);
    return NULL;
}


int
virUdevMgrAddLabel(virUdevMgrPtr mgr,
                   const char *device,
                   const virSecurityDeviceLabelDef *seclabel)
{
    int ret = -1;
    udevSeclabelPtr list;

    virObjectLock(mgr);

    if (mgr->filter) {
        int rc = mgr->filter(device, seclabel, mgr->opaque);
        if (rc < 0)
            goto cleanup;
        if (rc == 0) {
            /* Claim success. */
            ret = 0;
            goto cleanup;
        }
    }

    if ((list = virHashLookup(mgr->labels, device))) {
        virSecurityDeviceLabelDefPtr entry = udevSeclabelFindByModel(list, seclabel->model);

        if (entry) {
            udevSeclabelUpdate(list, seclabel);
        } else {
            if (udevSeclabelAppend(list, seclabel) < 0)
                goto cleanup;
        }
    } else {
        if (VIR_ALLOC(list) < 0 ||
            udevSeclabelAppend(list, seclabel) < 0 ||
            virHashAddEntry(mgr->labels, device, list) < 0) {
            udevSeclabelFree(list, NULL);
            goto cleanup;
        }
    }

    ret = 0;
 cleanup:
    virObjectUnlock(mgr);
    return ret;
}


int
virUdevMgrLookupLabels(virUdevMgrPtr mgr,
                       const char *device,
                       virSecurityDeviceLabelDefPtr **seclabels,
                       size_t *nseclabels)
{
    int ret = -1;
    udevSeclabelPtr list;
    size_t i;

    virObjectLock(mgr);

    if (!(list = virHashLookup(mgr->labels, device))) {
        *seclabels = NULL;
        *nseclabels = 0;
        ret = 0;
        goto cleanup;
    }

    if (VIR_ALLOC_N(*seclabels, list->nseclabels) < 0)
        goto cleanup;

    *nseclabels = list->nseclabels;

    for (i = 0; i < list->nseclabels; i++) {
        if (!((*seclabels)[i] = virSecurityDeviceLabelDefCopy(list->seclabels[i])))
            goto cleanup;
    }

    ret = 0;
 cleanup:
    virObjectUnlock(mgr);
    if (ret < 0) {
        if (*seclabels) {
            for (i = 0; i < *nseclabels; i++)
                virSecurityDeviceLabelDefFree((*seclabels)[i]);
            VIR_FREE(*seclabels);
        }
    }
    return ret;
}


int
virUdevMgrRemoveAllLabels(virUdevMgrPtr mgr,
                          const char *device)
{
    int ret = -1;

    virObjectLock(mgr);
    ret = virHashRemoveEntry(mgr->labels, device);
    virObjectUnlock(mgr);
    return ret;
}


static virJSONValuePtr
virUdevSeclabelDump(virUdevMgrPtr mgr)
{
    virJSONValuePtr seclabels;

    if (!(seclabels = virJSONValueNewArray()))
        return NULL;

    if (virHashForEach(mgr->labels, udevSeclabelsDump, seclabels) < 0) {
        virJSONValueFree(seclabels);
        return NULL;
    }

    return seclabels;
}


static char *
virUdevMgrDumpInternal(virUdevMgrPtr mgr)
{
    virJSONValuePtr object = NULL;
    virJSONValuePtr child = NULL;
    char *ret = NULL;

    if (!(object = virJSONValueNewObject()))
        goto cleanup;

    if (!(child = virUdevSeclabelDump(mgr)))
        goto cleanup;

    if (virJSONValueObjectAppend(object, "labels", child) < 0) {
        virJSONValueFree(child);
        goto cleanup;
    }

    ret = virJSONValueToString(object, true);
 cleanup:
    virJSONValueFree(object);
    return ret;
}


char *
virUdevMgrDumpStr(virUdevMgrPtr mgr)
{
    char *ret;

    virObjectLock(mgr);
    ret = virUdevMgrDumpInternal(mgr);
    virObjectUnlock(mgr);
    return ret;
}


static int
virUdevMgrRewriter(int fd, void *opaque)
{
    const char *str = opaque;

    return safewrite(fd, str, strlen(str));
}


int
virUdevMgrDumpFile(virUdevMgrPtr mgr,
                   const char *filename)
{
    int ret = -1;
    char *state;

    virObjectLock(mgr);

    if (!(state = virUdevMgrDumpInternal(mgr)))
        goto cleanup;

    /* Here we shouldn't use pure virFileWriteStr() as that one is not atomic.
     * We can be interrupted in the middle (e.g. due to a context switch) and
     * thus leave the file partially written. */
    if (virFileRewrite(filename, 0644, virUdevMgrRewriter, state) < 0) {
        virReportSystemError(errno,
                             _("Unable to save state file %s"),
                             filename);
        goto cleanup;
    }

    ret = 0;
 cleanup:
    virObjectUnlock(mgr);
    VIR_FREE(state);
    return ret;
}


static int
virUdevRestoreLabels(virUdevMgrPtr mgr ATTRIBUTE_UNUSED,
                     virJSONValuePtr labelsArray)
{
    int ret = -1;
    size_t i;
    udevSeclabelPtr list = NULL;

    for (i = 0; i < virJSONValueArraySize(labelsArray); i++) {
        virJSONValuePtr deviceJSON = virJSONValueArrayGet(labelsArray, i);
        virJSONValuePtr labels;
        const char *device;

        if (!(device = virJSONValueObjectGetString(deviceJSON, "device"))) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                           _("Missing device name"));
            goto cleanup;
        }

        if (!(labels = virJSONValueObjectGetArray(deviceJSON, "labels"))) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                           _("Missing device labels array"));
            goto cleanup;
        }

        if (!(list = udevSeclabelRestore(labels))) {
            virReportError(VIR_ERR_INTERNAL_ERROR,
                           _("Malformed seclabels for device %s"), device);
            goto cleanup;
        }

        if (virHashAddEntry(mgr->labels, device, list) < 0)
            goto cleanup;
        list = NULL;
    }

    ret = 0;
 cleanup:
    udevSeclabelFree(list, NULL);
    return ret;
}


static int
virUdevMgrNewFromStrInternal(virUdevMgrPtr mgr,
                             const char *state)
{
    virJSONValuePtr object;
    virJSONValuePtr child;
    int ret = -1;

    if (!(object = virJSONValueFromString(state)))
        goto cleanup;

    if (!(child = virJSONValueObjectGetArray(object, "labels"))) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Missing 'labels' object in JSON document"));
        goto cleanup;
    }

    if (virUdevRestoreLabels(mgr, child) < 0)
        goto cleanup;

    ret = 0;
 cleanup:
    virJSONValueFree(object);
    return ret;
}


virUdevMgrPtr
virUdevMgrNewFromStr(const char *str)
{
    virUdevMgrPtr mgr;

    if (!(mgr = virUdevMgrNew()))
        goto error;

    if (virUdevMgrNewFromStrInternal(mgr, str) < 0)
        goto error;

    return mgr;
 error:
    virObjectUnref(mgr);
    return NULL;
}


virUdevMgrPtr
virUdevMgrNewFromFile(const char *filename)
{
    virUdevMgrPtr mgr;
    char *state = NULL;

    if (!(mgr = virUdevMgrNew()))
        goto error;

    if (virFileReadAll(filename,
                       1024 * 1024 * 10, /* 10 MB */
                       &state) < 0)
        goto error;

    if (virUdevMgrNewFromStrInternal(mgr, state) < 0)
        goto error;

    VIR_FREE(state);

    return mgr;
 error:
    virObjectUnref(mgr);
    VIR_FREE(state);
    return NULL;
}

void
virUdevMgrSetFilter(virUdevMgrPtr mgr,
                    virUdevMgrFilter filter,
                    void *opaque)
{
    virObjectLock(mgr);
    mgr->filter = filter;
    mgr->opaque = opaque;
    virObjectUnlock(mgr);
}
