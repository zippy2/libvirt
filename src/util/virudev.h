/*
 * virudev.h: udev rules engine
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

#ifndef __VIR_UDEV_H__
# define __VIR_UDEV_H__

# include "virseclabel.h"

typedef struct _virUdevMgr virUdevMgr;
typedef virUdevMgr *virUdevMgrPtr;

/* Filter some devices out in virUdevMgrAddLabel.
 * Return 0 to NOT record label for device,
 *        1 to record the label for device,
 *       -1 on error.
 */
typedef int (*virUdevMgrFilter)(const char *device,
                                const virSecurityDeviceLabelDef *seclabel,
                                void *opaque);

virUdevMgrPtr virUdevMgrNew(void);
virUdevMgrPtr virUdevMgrNewFromStr(const char *str);
virUdevMgrPtr virUdevMgrNewFromFile(const char *filename);

int virUdevMgrAddLabel(virUdevMgrPtr mgr,
                       const char *device,
                       const virSecurityDeviceLabelDef *seclabel);
int virUdevMgrRemoveAllLabels(virUdevMgrPtr mgr,
                              const char *device);
int virUdevMgrLookupLabels(virUdevMgrPtr mgr,
                           const char *device,
                           virSecurityDeviceLabelDefPtr **seclabels,
                           size_t *nseclabels);

char *virUdevMgrDumpStr(virUdevMgrPtr mgr);

int virUdevMgrDumpFile(virUdevMgrPtr mgr,
                       const char *filename);

void virUdevMgrSetFilter(virUdevMgrPtr mgr,
                         virUdevMgrFilter filter,
                         void *opaque);

#endif
