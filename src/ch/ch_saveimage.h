/*
 * ch_saveimage.h: Infrastructure for saving CH state into a file
 *
 * Copyright Intel Corp. 2020-2021
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

#include "ch_conf.h"
#include "virconftypes.h"

int
chDomainSaveImageRead(virConnectPtr conn,
                      const char *path,
                      virDomainDef **ret_def,
                      unsigned int flags,
                      int (*ensureACL)(virConnectPtr, virDomainDef *),
                      int (*ensureACLWithFlags)(virConnectPtr,
                                                virDomainDef *,
                                                unsigned int));

int
chDomainSaveRestoreAdditionalValidation(virCHDriver *driver,
                                        virDomainDef *vmdef);

int
chDoDomainSave(virCHDriver *driver,
               virDomainObj *vm,
               const char *to_dir,
               bool managed);
