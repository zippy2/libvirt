/* virhostdev.h: hostdev management
 *
 * Copyright (C) 2006-2007, 2009-2013 Red Hat, Inc.
 * Copyright (C) 2006 Daniel P. Berrange
 * Copyright (C) 2014 SUSE LINUX Products GmbH, Nuernberg, Germany.
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

#include "internal.h"

#include "virpci.h"
#include "virusb.h"
#include "virscsi.h"
#include "virscsivhost.h"
#include "conf/domain_conf.h"
#include "virmdev.h"
#include "virnvme.h"

typedef enum {
    VIR_HOSTDEV_STRICT_ACS_CHECK     = (1 << 0), /* strict acs check */
    VIR_HOSTDEV_COLD_BOOT            = (1 << 1), /* cold boot */

    VIR_HOSTDEV_SP_PCI               = (1 << 8), /* support pci passthrough */
    VIR_HOSTDEV_SP_USB               = (1 << 9), /* support usb passthrough */
    VIR_HOSTDEV_SP_SCSI              = (1 << 10), /* support scsi passthrough */
} virHostdevFlag;


typedef struct _virHostdevManager virHostdevManager;
typedef virHostdevManager *virHostdevManagerPtr;
struct _virHostdevManager {
    virObject parent;

    char *stateDir;

    virPCIDeviceListPtr activePCIHostdevs;
    virPCIDeviceListPtr inactivePCIHostdevs;
    virUSBDeviceListPtr activeUSBHostdevs;
    virSCSIDeviceListPtr activeSCSIHostdevs;
    virSCSIVHostDeviceListPtr activeSCSIVHostHostdevs;
    virMediatedDeviceListPtr activeMediatedHostdevs;
    /* NVMe devices are PCI devices really, but one NVMe disk can
     * have multiple namespaces. */
    virNVMeDeviceListPtr activeNVMeHostdevs;
};

G_DEFINE_AUTOPTR_CLEANUP_FUNC(virHostdevManager, virObjectUnref);


virHostdevManagerPtr virHostdevManagerGetDefault(void);
int
virHostdevPreparePCIDevices(virHostdevManagerPtr hostdev_mgr,
                            const char *drv_name,
                            const char *dom_name,
                            const unsigned char *uuid,
                            virDomainHostdevDefPtr *hostdevs,
                            int nhostdevs,
                            unsigned int flags)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3)
    ATTRIBUTE_NONNULL(4);

int
virHostdevFindUSBDevice(virDomainHostdevDefPtr hostdev,
                        bool mandatory,
                        virUSBDevicePtr *usb)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(3);
int
virHostdevPrepareUSBDevices(virHostdevManagerPtr hostdev_mgr,
                            const char *drv_name,
                            const char *dom_name,
                            virDomainHostdevDefPtr *hostdevs,
                            int nhostdevs,
                            unsigned int flags)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
int
virHostdevPrepareSCSIDevices(virHostdevManagerPtr hostdev_mgr,
                             const char *drv_name,
                             const char *dom_name,
                             virDomainHostdevDefPtr *hostdevs,
                             int nhostdevs)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
int
virHostdevPrepareSCSIVHostDevices(virHostdevManagerPtr hostdev_mgr,
                                  const char *drv_name,
                                  const char *dom_name,
                                  virDomainHostdevDefPtr *hostdevs,
                                  int nhostdevs)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
int
virHostdevPrepareMediatedDevices(virHostdevManagerPtr hostdev_mgr,
                                 const char *drv_name,
                                 const char *dom_name,
                                 virDomainHostdevDefPtr *hostdevs,
                                 int nhostdevs)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
void
virHostdevReAttachPCIDevices(virHostdevManagerPtr hostdev_mgr,
                             const char *drv_name,
                             const char *dom_name,
                             virDomainHostdevDefPtr *hostdevs,
                             int nhostdevs)
    ATTRIBUTE_NONNULL(1);
void
virHostdevReAttachUSBDevices(virHostdevManagerPtr hostdev_mgr,
                              const char *drv_name,
                              const char *dom_name,
                              virDomainHostdevDefPtr *hostdevs,
                              int nhostdevs)
    ATTRIBUTE_NONNULL(1);
void
virHostdevReAttachSCSIDevices(virHostdevManagerPtr hostdev_mgr,
                              const char *drv_name,
                              const char *dom_name,
                              virDomainHostdevDefPtr *hostdevs,
                              int nhostdevs)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
void
virHostdevReAttachSCSIVHostDevices(virHostdevManagerPtr hostdev_mgr,
                                   const char *drv_name,
                                   const char *dom_name,
                                   virDomainHostdevDefPtr *hostdevs,
                                   int nhostdevs)
    ATTRIBUTE_NONNULL(1);
void
virHostdevReAttachMediatedDevices(virHostdevManagerPtr hostdev_mgr,
                                  const char *drv_name,
                                  const char *dom_name,
                                  virDomainHostdevDefPtr *hostdevs,
                                  int nhostdevs)
    ATTRIBUTE_NONNULL(1);
int
virHostdevUpdateActivePCIDevices(virHostdevManagerPtr mgr,
                                 virDomainHostdevDefPtr *hostdevs,
                                 int nhostdevs,
                                 const char *drv_name,
                                 const char *dom_name)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(4) ATTRIBUTE_NONNULL(5);
int
virHostdevUpdateActiveUSBDevices(virHostdevManagerPtr mgr,
                                 virDomainHostdevDefPtr *hostdevs,
                                 int nhostdevs,
                                 const char *drv_name,
                                 const char *dom_name)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(4) ATTRIBUTE_NONNULL(5);
int
virHostdevUpdateActiveSCSIDevices(virHostdevManagerPtr mgr,
                                  virDomainHostdevDefPtr *hostdevs,
                                  int nhostdevs,
                                  const char *drv_name,
                                  const char *dom_name)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(4) ATTRIBUTE_NONNULL(5);
int
virHostdevUpdateActiveMediatedDevices(virHostdevManagerPtr mgr,
                                      virDomainHostdevDefPtr *hostdevs,
                                      int nhostdevs,
                                      const char *drv_name,
                                      const char *dom_name)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(4) ATTRIBUTE_NONNULL(5);
int
virHostdevUpdateActiveDomainDevices(virHostdevManagerPtr mgr,
                                    const char *driver,
                                    virDomainDefPtr def,
                                    unsigned int flags)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
int
virHostdevPrepareDomainDevices(virHostdevManagerPtr mgr,
                               const char *driver,
                               virDomainDefPtr def,
                               unsigned int flags)
    ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);
void
virHostdevReAttachDomainDevices(virHostdevManagerPtr mgr,
                                const char *driver,
                                virDomainDefPtr def,
                                unsigned int flags)
    ATTRIBUTE_NONNULL(2) ATTRIBUTE_NONNULL(3);

/* functions used by NodeDevDetach/Reattach/Reset */
int virHostdevPCINodeDeviceDetach(virHostdevManagerPtr mgr,
                                  virPCIDevicePtr pci)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2);
int virHostdevPCINodeDeviceReAttach(virHostdevManagerPtr mgr,
                                    virPCIDevicePtr pci)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2);
int virHostdevPCINodeDeviceReset(virHostdevManagerPtr mgr,
                                 virPCIDevicePtr pci)
    ATTRIBUTE_NONNULL(1) ATTRIBUTE_NONNULL(2);

int
virHostdevPrepareOneNVMeDevice(virHostdevManagerPtr hostdev_mgr,
                               const char *drv_name,
                               const char *dom_name,
                               virStorageSourcePtr src);

int
virHostdevPrepareNVMeDevices(virHostdevManagerPtr hostdev_mgr,
                             const char *drv_name,
                             const char *dom_name,
                             virDomainDiskDefPtr *disks,
                             size_t ndisks);

int
virHostdevReAttachOneNVMeDevice(virHostdevManagerPtr hostdev_mgr,
                                const char *drv_name,
                                const char *dom_name,
                                virStorageSourcePtr src);

int
virHostdevReAttachNVMeDevices(virHostdevManagerPtr hostdev_mgr,
                              const char *drv_name,
                              const char *dom_name,
                              virDomainDiskDefPtr *disks,
                              size_t ndisks);

int
virHostdevUpdateActiveNVMeDevices(virHostdevManagerPtr hostdev_mgr,
                                  const char *drv_name,
                                  const char *dom_name,
                                  virDomainDiskDefPtr *disks,
                                  size_t ndisks);

bool virHostdevIsPCIDevice(const virDomainHostdevDef *hostdev);
