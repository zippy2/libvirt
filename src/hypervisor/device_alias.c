/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <config.h>

#include "device_alias.h"
#include "virlog.h"
#include "virstring.h"

VIR_LOG_INIT("hypervisor/device_alias");

static int
virDomainDeviceAliasIndex(const virDomainDeviceInfo *info,
                          const char *prefix)
{
    int idx;

    if (!info->alias)
        return -1;

    if (!STRPREFIX(info->alias, prefix))
        return -1;

    if (virStrToLong_i(info->alias + strlen(prefix), NULL, 10, &idx) < 0)
        return -1;

    return idx;
}


void
virAssignDeviceDiskAlias(virDomainDef *def,
                         virDomainDiskDef *disk,
                         const char *prefix,
                         int idx)
{
    if (disk->info.alias)
        return;

    if (idx < 0) {
        size_t i;

        for (i = 0; i < def->ndisks; i++) {
            int thisidx = virDomainDeviceAliasIndex(&def->disks[i]->info, prefix);

            if (thisidx >= idx)
                idx = thisidx + 1;
        }
    }

    disk->info.alias = g_strdup_printf("%s%d", prefix, idx);
}


/**
 * virDomainAssignDeviceAliases:
 * @aliasBase: array of alias prefixes
 * @def: domain definition
 *
 * Assign aliases to devices in domain definition @def. The
 * @aliasBase is then an array of alias prefixes, where item at
 * position VIR_DOMAIN_DEVICE_DISK contains alias prefix for
 * disks, and so on. If item at any position is NULL, setting
 * aliases for corresponding devices is skipped.
 *
 * Returns: 0 on success,
 *         -1 otherwise (with error reported).
 */
int
virDomainAssignDeviceAliases(const char *aliasBase[VIR_DOMAIN_DEVICE_LAST],
                             virDomainDef *def)
{
    size_t dev;

    for (dev = 0; dev < VIR_DOMAIN_DEVICE_LAST; dev++) {
        size_t i;

        if (!aliasBase[dev])
            continue;

        switch ((virDomainDeviceType) dev) {
        case VIR_DOMAIN_DEVICE_DISK:
            for (i = 0; i < def->ndisks; i++) {
                virAssignDeviceDiskAlias(def, def->disks[i], aliasBase[dev], i);
            }
            break;
        case VIR_DOMAIN_DEVICE_LEASE:
        case VIR_DOMAIN_DEVICE_FS:
        case VIR_DOMAIN_DEVICE_NET:
        case VIR_DOMAIN_DEVICE_INPUT:
        case VIR_DOMAIN_DEVICE_SOUND:
        case VIR_DOMAIN_DEVICE_VIDEO:
        case VIR_DOMAIN_DEVICE_HOSTDEV:
        case VIR_DOMAIN_DEVICE_WATCHDOG:
        case VIR_DOMAIN_DEVICE_CONTROLLER:
        case VIR_DOMAIN_DEVICE_GRAPHICS:
        case VIR_DOMAIN_DEVICE_HUB:
        case VIR_DOMAIN_DEVICE_REDIRDEV:
        case VIR_DOMAIN_DEVICE_SMARTCARD:
        case VIR_DOMAIN_DEVICE_CHR:
        case VIR_DOMAIN_DEVICE_MEMBALLOON:
        case VIR_DOMAIN_DEVICE_NVRAM:
        case VIR_DOMAIN_DEVICE_RNG:
        case VIR_DOMAIN_DEVICE_SHMEM:
        case VIR_DOMAIN_DEVICE_TPM:
        case VIR_DOMAIN_DEVICE_PANIC:
        case VIR_DOMAIN_DEVICE_MEMORY:
        case VIR_DOMAIN_DEVICE_IOMMU:
        case VIR_DOMAIN_DEVICE_VSOCK:
        case VIR_DOMAIN_DEVICE_AUDIO:
        case VIR_DOMAIN_DEVICE_CRYPTO:
        case VIR_DOMAIN_DEVICE_PSTORE:
        case VIR_DOMAIN_DEVICE_NONE:
        case VIR_DOMAIN_DEVICE_LAST:
            break;
        }
    }

    return 0;
}
