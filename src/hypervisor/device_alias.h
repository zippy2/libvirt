/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#include "domain_conf.h"
#include "virconftypes.h"

void
virAssignDeviceDiskAlias(virDomainDef *def,
                         virDomainDiskDef *disk,
                         const char *prefix,
                         int idx);

int
virDomainAssignDeviceAliases(const char *aliasBase[VIR_DOMAIN_DEVICE_LAST],
                             virDomainDef *def);
