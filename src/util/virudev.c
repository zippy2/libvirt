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
#include "virhash.h"
#include "virobject.h"
#include "virudev.h"

#define VIR_FROM_THIS VIR_FROM_SECURITY

struct _virUdevMgr {
    virObjectLockable parent;
    virHashTablePtr labels;
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
virUdevMgrRemoveAllLabels(virUdevMgrPtr mgr,
                          const char *device)
{
    int ret = -1;

    virObjectLock(mgr);
    ret = virHashRemoveEntry(mgr->labels, device);
    virObjectUnlock(mgr);
    return ret;
}
