/*
 * virscsi.c: helper APIs for managing host SCSI devices
 *
 * Copyright (C) 2013-2014 Red Hat, Inc.
 * Copyright (C) 2013 Fujitsu, Inc.
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

#include <dirent.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "virlog.h"
#include "virscsi.h"
#include "virfile.h"
#include "virstring.h"
#include "virerror.h"
#include "viralloc.h"

#define SYSFS_SCSI_DEVICES "/sys/bus/scsi/devices"

#define VIR_FROM_THIS VIR_FROM_NONE

VIR_LOG_INIT("util.scsi");

struct _virUsedByInfo {
    char *drvname; /* which driver */
    char *domname; /* which domain */
};
typedef struct _virUsedByInfo virUsedByInfo;


/* Keep in sync with scsi/scsi_proto.h */
typedef enum {
    VIR_SCSI_DEVICE_TYPE_NONE = -1,
    VIR_SCSI_DEVICE_TYPE_DISK = 0x00,
    VIR_SCSI_DEVICE_TYPE_TAPE = 0x01,
    VIR_SCSI_DEVICE_TYPE_PRINTER = 0x02,
    VIR_SCSI_DEVICE_TYPE_PROCESSOR = 0x03,
    VIR_SCSI_DEVICE_TYPE_WORM = 0x04,
    VIR_SCSI_DEVICE_TYPE_ROM = 0x05,
    VIR_SCSI_DEVICE_TYPE_SCANNER = 0x06,
    VIR_SCSI_DEVICE_TYPE_MOD = 0x07,
    VIR_SCSI_DEVICE_TYPE_MEDIUM_CHANGER = 0x08,
    VIR_SCSI_DEVICE_TYPE_COMM = 0x09,
    VIR_SCSI_DEVICE_TYPE_RAID = 0x0c,
    VIR_SCSI_DEVICE_TYPE_ENCLOSURE = 0x0d,
    VIR_SCSI_DEVICE_TYPE_RBC = 0x0e,
    VIR_SCSI_DEVICE_TYPE_OSD = 0x11,
    VIR_SCSI_DEVICE_TYPE_ZBC = 0x14,
    VIR_SCSI_DEVICE_TYPE_WLUN = 0x1e,
    VIR_SCSI_DEVICE_TYPE_NO_LUN = 0x7f,

    VIR_SCSI_DEVICE_TYPE_LAST,
} virSCSIDeviceType;


struct _virSCSIDevice {
    unsigned int adapter;
    unsigned int bus;
    unsigned int target;
    unsigned long long unit;

    char *name; /* adapter:bus:target:unit */
    char *id;   /* model:vendor */
    char *sg_path; /* e.g. /dev/sg2 */
    virUsedByInfo **used_by; /* driver:domain(s) using this dev */
    size_t n_used_by; /* how many domains are using this dev */

    bool readonly;
    bool shareable;
};

struct _virSCSIDeviceList {
    virObjectLockable parent;
    size_t count;
    virSCSIDevice **devs;
};

static virClass *virSCSIDeviceListClass;

static void virSCSIDeviceListDispose(void *obj);

static int
virSCSIOnceInit(void)
{
    if (!VIR_CLASS_NEW(virSCSIDeviceList, virClassForObjectLockable()))
        return -1;

    return 0;
}

VIR_ONCE_GLOBAL_INIT(virSCSI);

static int
virSCSIDeviceGetAdapterId(const char *adapter,
                          unsigned int *adapter_id)
{
    if (STRPREFIX(adapter, "scsi_host") &&
        virStrToLong_ui(adapter + strlen("scsi_host"),
                        NULL, 0, adapter_id) == 0)
        return 0;
    virReportError(VIR_ERR_INTERNAL_ERROR,
                   _("Cannot parse adapter '%1$s'"), adapter);
    return -1;
}

char *
virSCSIDeviceGetSgName(const char *sysfs_prefix,
                       const char *adapter,
                       unsigned int bus,
                       unsigned int target,
                       unsigned long long unit)
{
    g_autoptr(DIR) dir = NULL;
    struct dirent *entry;
    g_autofree char *path = NULL;
    unsigned int adapter_id;
    const char *prefix = sysfs_prefix ? sysfs_prefix : SYSFS_SCSI_DEVICES;

    if (virSCSIDeviceGetAdapterId(adapter, &adapter_id) < 0)
        return NULL;

    path = g_strdup_printf("%s/%d:%u:%u:%llu/scsi_generic", prefix, adapter_id,
                           bus, target, unit);

    if (virDirOpen(&dir, path) < 0)
        return NULL;

    /* Assume a single directory entry */
    if (virDirRead(dir, &entry, path) > 0)
        return  g_strdup(entry->d_name);

    return NULL;
}


static int
virSCSIDeviceGetType(const char *prefix,
                     unsigned int adapter,
                     unsigned int bus,
                     unsigned int target,
                     unsigned long long unit,
                     virSCSIDeviceType *type)
{
    int intType;

    if (virFileReadValueInt(&intType,
                            "%s/%d:%u:%u:%llu/type",
                            prefix, adapter, bus, target, unit) < 0)
        return -1;

    switch (intType) {
    case VIR_SCSI_DEVICE_TYPE_DISK:
    case VIR_SCSI_DEVICE_TYPE_TAPE:
    case VIR_SCSI_DEVICE_TYPE_PRINTER:
    case VIR_SCSI_DEVICE_TYPE_PROCESSOR:
    case VIR_SCSI_DEVICE_TYPE_WORM:
    case VIR_SCSI_DEVICE_TYPE_ROM:
    case VIR_SCSI_DEVICE_TYPE_SCANNER:
    case VIR_SCSI_DEVICE_TYPE_MOD:
    case VIR_SCSI_DEVICE_TYPE_MEDIUM_CHANGER:
    case VIR_SCSI_DEVICE_TYPE_COMM:
    case VIR_SCSI_DEVICE_TYPE_RAID:
    case VIR_SCSI_DEVICE_TYPE_ENCLOSURE:
    case VIR_SCSI_DEVICE_TYPE_RBC:
    case VIR_SCSI_DEVICE_TYPE_OSD:
    case VIR_SCSI_DEVICE_TYPE_ZBC:
    case VIR_SCSI_DEVICE_TYPE_WLUN:
    case VIR_SCSI_DEVICE_TYPE_NO_LUN:
        *type = intType;
        break;

    default:
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("unknown SCSI device type: %x"),
                       intType);
        return -1;
    }

    return 0;
}


static char *
virSCSIDeviceGetDevNameBlock(const char *prefix,
                             unsigned int adapter,
                             unsigned int bus,
                             unsigned int target,
                             unsigned long long unit)
{
    g_autoptr(DIR) dir = NULL;
    struct dirent *entry;
    g_autofree char *path = NULL;
    char *name = NULL;

    path = g_strdup_printf("%s/%d:%u:%u:%llu/block",
                           prefix, adapter, bus, target, unit);

    if (virDirOpen(&dir, path) < 0)
        return NULL;

    while (virDirRead(dir, &entry, path) > 0) {
        name = g_strdup(entry->d_name);
        break;
    }

    return name;
}


static char *
virSCSIDeviceGetDevNameTape(const char *prefix,
                            unsigned int adapter,
                            unsigned int bus,
                            unsigned int target,
                            unsigned long long unit)
{
    g_autofree char *path = NULL;
    g_autofree char *resolvedPath = NULL;
    g_autoptr(GError) gerr = NULL;

    path = g_strdup_printf("%s/%d:%u:%u:%llu/tape",
                           prefix, adapter, bus, target, unit);

    if (!(resolvedPath = g_file_read_link(path, &gerr))) {
        virReportError(VIR_ERR_SYSTEM_ERROR,
                       _("failed to resolve symlink %s: %s"), path, gerr->message);
        return NULL;
    }

    return g_path_get_basename(resolvedPath);
}


/* Returns device name (e.g. "sdc") on success, or NULL
 * on failure.
 */
char *
virSCSIDeviceGetDevName(const char *sysfs_prefix,
                        const char *adapter,
                        unsigned int bus,
                        unsigned int target,
                        unsigned long long unit)
{
    char *name = NULL;
    unsigned int adapter_id;
    virSCSIDeviceType type;
    const char *prefix = sysfs_prefix ? sysfs_prefix : SYSFS_SCSI_DEVICES;

    if (virSCSIDeviceGetAdapterId(adapter, &adapter_id) < 0)
        return NULL;

    if (virSCSIDeviceGetType(prefix, adapter_id,
                             bus, target, unit, &type) < 0)
        return NULL;

    switch (type) {
    case VIR_SCSI_DEVICE_TYPE_DISK:
        name = virSCSIDeviceGetDevNameBlock(prefix, adapter_id, bus, target, unit);
        break;

    case VIR_SCSI_DEVICE_TYPE_TAPE:
        name = virSCSIDeviceGetDevNameTape(prefix, adapter_id, bus, target, unit);
        break;

    case VIR_SCSI_DEVICE_TYPE_PRINTER:
    case VIR_SCSI_DEVICE_TYPE_PROCESSOR:
    case VIR_SCSI_DEVICE_TYPE_WORM:
    case VIR_SCSI_DEVICE_TYPE_ROM:
    case VIR_SCSI_DEVICE_TYPE_SCANNER:
    case VIR_SCSI_DEVICE_TYPE_MOD:
    case VIR_SCSI_DEVICE_TYPE_MEDIUM_CHANGER:
    case VIR_SCSI_DEVICE_TYPE_COMM:
    case VIR_SCSI_DEVICE_TYPE_RAID:
    case VIR_SCSI_DEVICE_TYPE_ENCLOSURE:
    case VIR_SCSI_DEVICE_TYPE_RBC:
    case VIR_SCSI_DEVICE_TYPE_OSD:
    case VIR_SCSI_DEVICE_TYPE_ZBC:
    case VIR_SCSI_DEVICE_TYPE_WLUN:
    case VIR_SCSI_DEVICE_TYPE_NO_LUN:
    case VIR_SCSI_DEVICE_TYPE_NONE:
    case VIR_SCSI_DEVICE_TYPE_LAST:
    default:
        virReportError(VIR_ERR_CONFIG_UNSUPPORTED,
                       _("unsupported SCSI device type: %x"),
                       type);
        break;
    }

    return name;
}


virSCSIDevicePtr
virSCSIDevice *
virSCSIDeviceNew(const char *sysfs_prefix,
                 const char *adapter,
                 unsigned int bus,
                 unsigned int target,
                 unsigned long long unit,
                 bool readonly,
                 bool shareable)
{
    g_autoptr(virSCSIDevice) dev = NULL;
    g_autofree char *sg = NULL;
    g_autofree char *vendor_path = NULL;
    g_autofree char *model_path = NULL;
    g_autofree char *vendor = NULL;
    g_autofree char *model = NULL;
    const char *prefix = sysfs_prefix ? sysfs_prefix : SYSFS_SCSI_DEVICES;

    dev = g_new0(virSCSIDevice, 1);

    dev->bus = bus;
    dev->target = target;
    dev->unit = unit;
    dev->readonly = readonly;
    dev->shareable = shareable;

    if (!(sg = virSCSIDeviceGetSgName(prefix, adapter, bus, target, unit)))
        return NULL;

    if (virSCSIDeviceGetAdapterId(adapter, &dev->adapter) < 0)
        return NULL;

    dev->name = g_strdup_printf("%d:%u:%u:%llu", dev->adapter,
                                dev->bus, dev->target, dev->unit);
    dev->sg_path = g_strdup_printf("%s/%s",
                                   sysfs_prefix ? sysfs_prefix : "/dev", sg);

    if (!virFileExists(dev->sg_path)) {
        virReportSystemError(errno,
                             _("SCSI device '%1$s': could not access %2$s"),
                             dev->name, dev->sg_path);
        return NULL;
    }

    vendor_path = g_strdup_printf("%s/%s/vendor", prefix, dev->name);
    model_path = g_strdup_printf("%s/%s/model", prefix, dev->name);

    if (virFileReadAll(vendor_path, 1024, &vendor) < 0)
        return NULL;

    if (virFileReadAll(model_path, 1024, &model) < 0)
        return NULL;

    virTrimSpaces(vendor, NULL);
    virTrimSpaces(model, NULL);

    dev->id = g_strdup_printf("%s:%s", vendor, model);

    return g_steal_pointer(&dev);
}

static void
virSCSIDeviceUsedByInfoFree(virUsedByInfo *used_by)
{
    g_free(used_by->drvname);
    g_free(used_by->domname);
    g_free(used_by);
}
G_DEFINE_AUTOPTR_CLEANUP_FUNC(virUsedByInfo, virSCSIDeviceUsedByInfoFree);

void
virSCSIDeviceFree(virSCSIDevice *dev)
{
    size_t i;

    if (!dev)
        return;

    g_free(dev->id);
    g_free(dev->name);
    g_free(dev->sg_path);
    for (i = 0; i < dev->n_used_by; i++)
        virSCSIDeviceUsedByInfoFree(dev->used_by[i]);
    g_free(dev->used_by);
    g_free(dev);
}

void
virSCSIDeviceSetUsedBy(virSCSIDevice *dev,
                       const char *drvname,
                       const char *domname)
{
    g_autoptr(virUsedByInfo) copy = NULL;

    copy = g_new0(virUsedByInfo, 1);
    copy->drvname = g_strdup(drvname);
    copy->domname = g_strdup(domname);

    VIR_APPEND_ELEMENT(dev->used_by, dev->n_used_by, copy);
}

bool
virSCSIDeviceIsAvailable(virSCSIDevice *dev)
{
    return dev->n_used_by == 0;
}

const char *
virSCSIDeviceGetName(virSCSIDevice *dev)
{
    return dev->name;
}

const char *
virSCSIDeviceGetPath(virSCSIDevice *dev)
{
    return dev->sg_path;
}

unsigned int
virSCSIDeviceGetAdapter(virSCSIDevice *dev)
{
    return dev->adapter;
}

unsigned int
virSCSIDeviceGetBus(virSCSIDevice *dev)
{
    return dev->bus;
}

unsigned int
virSCSIDeviceGetTarget(virSCSIDevice *dev)
{
    return dev->target;
}

unsigned long long
virSCSIDeviceGetUnit(virSCSIDevice *dev)
{
    return dev->unit;
}

bool
virSCSIDeviceGetReadonly(virSCSIDevice *dev)
{
    return dev->readonly;
}

bool
virSCSIDeviceGetShareable(virSCSIDevice *dev)
{
    return dev->shareable;
}

int
virSCSIDeviceFileIterate(virSCSIDevice *dev,
                         virSCSIDeviceFileActor actor,
                         void *opaque)
{
    return (actor)(dev, dev->sg_path, opaque);
}

virSCSIDeviceList *
virSCSIDeviceListNew(void)
{
    virSCSIDeviceList *list;

    if (virSCSIInitialize() < 0)
        return NULL;

    if (!(list = virObjectLockableNew(virSCSIDeviceListClass)))
        return NULL;

    return list;
}

static void
virSCSIDeviceListDispose(void *obj)
{
    virSCSIDeviceList *list = obj;
    size_t i;

    for (i = 0; i < list->count; i++)
        virSCSIDeviceFree(list->devs[i]);

    g_free(list->devs);
}

int
virSCSIDeviceListAdd(virSCSIDeviceList *list,
                     virSCSIDevice *dev)
{
    if (virSCSIDeviceListFind(list, dev)) {
        virReportError(VIR_ERR_INTERNAL_ERROR,
                       _("Device %1$s already exists"),
                       dev->name);
        return -1;
    }

    VIR_APPEND_ELEMENT(list->devs, list->count, dev);

    return 0;
}

virSCSIDevice *
virSCSIDeviceListGet(virSCSIDeviceList *list, int idx)
{
    if (idx >= list->count || idx < 0)
        return NULL;

    return list->devs[idx];
}

size_t
virSCSIDeviceListCount(virSCSIDeviceList *list)
{
    return list->count;
}

virSCSIDevice *
virSCSIDeviceListSteal(virSCSIDeviceList *list,
                       virSCSIDevice *dev)
{
    virSCSIDevice *ret = NULL;
    size_t i;

    for (i = 0; i < list->count; i++) {
        if (list->devs[i]->adapter == dev->adapter &&
            list->devs[i]->bus == dev->bus &&
            list->devs[i]->target == dev->target &&
            list->devs[i]->unit == dev->unit) {
            ret = list->devs[i];
            VIR_DELETE_ELEMENT(list->devs, i, list->count);
            break;
        }
    }

    return ret;
}

void
virSCSIDeviceListDel(virSCSIDeviceList *list,
                     virSCSIDevice *dev,
                     const char *drvname,
                     const char *domname)
{
    size_t i;

    for (i = 0; i < dev->n_used_by; i++) {
        if (STREQ_NULLABLE(dev->used_by[i]->drvname, drvname) &&
            STREQ_NULLABLE(dev->used_by[i]->domname, domname)) {
            if (dev->n_used_by > 1) {
                virSCSIDeviceUsedByInfoFree(dev->used_by[i]);
                VIR_DELETE_ELEMENT(dev->used_by, i, dev->n_used_by);
            } else {
                virSCSIDeviceFree(virSCSIDeviceListSteal(list, dev));
            }
            break;
        }
    }
}

virSCSIDevice *
virSCSIDeviceListFind(virSCSIDeviceList *list,
                      virSCSIDevice *dev)
{
    size_t i;

    for (i = 0; i < list->count; i++) {
        if (list->devs[i]->adapter == dev->adapter &&
            list->devs[i]->bus == dev->bus &&
            list->devs[i]->target == dev->target &&
            list->devs[i]->unit == dev->unit)
            return list->devs[i];
    }

    return NULL;
}
