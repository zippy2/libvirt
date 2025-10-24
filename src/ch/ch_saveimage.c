/*
 * ch_saveimage.c: Infrastructure for saving CH state into a file
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

#include <config.h>

#include <fcntl.h>

#include "ch_saveimage.h"
#include "ch_domain.h"
#include "ch_process.h"
#include "virfile.h"

#define VIR_FROM_THIS VIR_FROM_CH

#define CH_SAVE_MAGIC "libvirt-xml\n \0 \r"
#define CH_SAVE_XML "libvirt-save.xml"

typedef struct _CHSaveXMLHeader CHSaveXMLHeader;
struct _CHSaveXMLHeader {
    char magic[sizeof(CH_SAVE_MAGIC)-1];
    uint32_t xmlLen;
    /* 20 bytes used, pad up to 64 bytes */
    uint32_t unused[11];
};


static char *
chDomainSaveXMLRead(int fd)
{
    g_autofree char *xml = NULL;
    CHSaveXMLHeader hdr;

    if (saferead(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        virReportError(VIR_ERR_OPERATION_FAILED, "%s",
                       _("failed to read CHSaveXMLHeader header"));
        return NULL;
    }

    if (memcmp(hdr.magic, CH_SAVE_MAGIC, sizeof(hdr.magic))) {
        virReportError(VIR_ERR_INVALID_ARG, "%s",
                       _("save image magic is incorrect"));
        return NULL;
    }

    if (hdr.xmlLen <= 0) {
        virReportError(VIR_ERR_OPERATION_FAILED,
                       _("invalid XML length: %1$d"), hdr.xmlLen);
        return NULL;
    }

    xml = g_new0(char, hdr.xmlLen);

    if (saferead(fd, xml, hdr.xmlLen) != hdr.xmlLen) {
        virReportError(VIR_ERR_OPERATION_FAILED, "%s",
                       _("failed to read XML"));
        return NULL;
    }

    return g_steal_pointer(&xml);
}


int
chDomainSaveImageRead(virCHDriver *driver,
                      const char *path,
                      virDomainDef **ret_def)
{
    virCHDriver *driver = conn->privateData;
    g_autoptr(virCHDriverConfig) cfg = virCHDriverGetConfig(driver);
    g_autoptr(virDomainDef) def = NULL;
    g_autofree char *from = NULL;
    g_autofree char *xml = NULL;
    VIR_AUTOCLOSE fd = -1;
    int ret = -1;
    unsigned int parse_flags = VIR_DOMAIN_DEF_PARSE_INACTIVE |
                               VIR_DOMAIN_DEF_PARSE_SKIP_VALIDATE;

    from = g_strdup_printf("%s/%s", path, CH_SAVE_XML);
    if ((fd = virFileOpenAs(from, O_RDONLY, 0, cfg->user, cfg->group, 0)) < 0) {
        virReportSystemError(errno,
                             _("Failed to open domain save file '%1$s'"),
                             from);
        goto end;
    }

    if (!(xml = chDomainSaveXMLRead(fd)))
        goto end;

    if (ensureACL || ensureACLWithFlags) {
        /* Parse only the IDs for ACL checks */
        g_autoptr(virDomainDef) aclDef = virDomainDefIDsParseString(xml,
                                                                    driver->xmlopt,
                                                                    parse_flags);

        if (!aclDef)
            goto end;

        if (ensureACL && ensureACL(conn, aclDef) < 0)
            goto end;

        if (ensureACLWithFlags && ensureACLWithFlags(conn, aclDef, flags) < 0)
            goto end;
    }

    if (!(def = virDomainDefParseString(xml, driver->xmlopt, NULL, parse_flags)))
        goto end;

    *ret_def = g_steal_pointer(&def);
    ret = 0;

 end:
    return ret;
}


int
chDomainSaveRestoreAdditionalValidation(virCHDriver *driver,
                                        virDomainDef *vmdef)
{
    /* SAVE and RESTORE are functional only without any host device
     * passthrough configuration */
    if  (vmdef->nhostdevs > 0) {
        virReportError(VIR_ERR_OPERATION_INVALID, "%s",
                       _("cannot save/restore domain with host devices"));
        return -1;
    }

    if (vmdef->nnets > 0) {
        if (!virBitmapIsBitSet(driver->chCaps, CH_RESTORE_WITH_NEW_TAPFDS)) {
            virReportError(VIR_ERR_OPERATION_INVALID, "%s",
                           _("cannot save/restore domain with network devices"));
            return -1;
        }
    }

    return 0;
}


/**
 * chDoDomainSave:
 * @driver: pointer to driver structure
 * @vm: pointer to virtual machine structure. Must be locked before invocation.
 * @to_dir: directory path (CH needs directory input) to save the domain
 * @managed: whether the VM is managed or not
 *
 * Checks if the domain is running or paused, then suspends it and saves it
 * using CH's vmm.snapshot API. CH creates multiple files for config, memory,
 * device state into @to_dir.
 *
 * Returns 0 on success or -1 in case of error
 */
int
chDoDomainSave(virCHDriver *driver,
               virDomainObj *vm,
               const char *to_dir,
               bool managed)
{
    g_autoptr(virCHDriverConfig) cfg = virCHDriverGetConfig(driver);
    virCHDomainObjPrivate *priv = vm->privateData;
    CHSaveXMLHeader hdr;
    virDomainState domainState;
    g_autofree char *to = NULL;
    g_autofree char *xml = NULL;
    uint32_t xml_len;
    VIR_AUTOCLOSE fd = -1;
    int ret = -1;

    if (chDomainSaveRestoreAdditionalValidation(driver, vm->def) < 0)
        goto end;

    domainState = virDomainObjGetState(vm, NULL);
    if (domainState == VIR_DOMAIN_RUNNING) {
        if (virCHMonitorSuspendVM(priv->monitor) < 0) {
            virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                           _("failed to suspend domain before saving"));
            goto end;
        }
        virDomainObjSetState(vm, VIR_DOMAIN_PAUSED, VIR_DOMAIN_PAUSED_SAVE);
    } else if (domainState != VIR_DOMAIN_PAUSED) {
        virReportError(VIR_ERR_OPERATION_INVALID, "%s",
                       _("only can save running/paused domain"));
        goto end;
    }

    if (virDirCreate(to_dir, 0770, cfg->user, cfg->group,
                     VIR_DIR_CREATE_ALLOW_EXIST) < 0) {
        virReportSystemError(errno, _("Failed to create SAVE dir %1$s"), to_dir);
        goto end;
    }

    to = g_strdup_printf("%s/%s", to_dir, CH_SAVE_XML);
    if ((fd = virFileOpenAs(to, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR,
                            cfg->user, cfg->group, 0)) < 0) {
        virReportSystemError(-fd,
                             _("Failed to create/open domain save xml file '%1$s'"),
                             to);
        goto end;
    }

    if ((xml = virDomainDefFormat(vm->def, driver->xmlopt, 0)) == NULL)
        goto end;
    xml_len = strlen(xml) + 1;

    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, CH_SAVE_MAGIC, sizeof(hdr.magic));
    hdr.xmlLen = xml_len;

    if (safewrite(fd, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        virReportSystemError(errno, "%s", _("Failed to write file header"));
        goto end;
    }

    if (safewrite(fd, xml, xml_len) != xml_len) {
        virReportSystemError(errno, "%s", _("Failed to write xml definition"));
        goto end;
    }

    if (virCHMonitorSaveVM(priv->monitor, to_dir) < 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s", _("Failed to save domain"));
        goto end;
    }

    if (virCHProcessStop(driver, vm,
                         VIR_DOMAIN_SHUTOFF_SAVED, VIR_CH_PROCESS_STOP_FORCE) < 0) {
        virReportError(VIR_ERR_INTERNAL_ERROR, "%s",
                       _("Failed to shutoff after domain save"));
        goto end;
    }

    vm->hasManagedSave = managed;
    ret = 0;

 end:
    return ret;
}
