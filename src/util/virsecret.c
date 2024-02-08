/*
 * virsecret.c: secret utility functions
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
 */

#include <config.h>

#include "datatypes.h"
#include "viralloc.h"
#include "vircommand.h"
#include "virerror.h"
#include "virlog.h"
#include "virsecret.h"
#include "virutil.h"
#include "viruuid.h"
#include "virfile.h"

#define VIR_FROM_THIS VIR_FROM_NONE

VIR_LOG_INIT("util.secret");

VIR_ENUM_IMPL(virSecretUsage,
              VIR_SECRET_USAGE_TYPE_LAST,
              "none", "volume", "ceph", "iscsi", "tls", "vtpm",
);

void
virSecretLookupDefClear(virSecretLookupTypeDef *def)
{
    if (def->type == VIR_SECRET_LOOKUP_TYPE_USAGE)
        VIR_FREE(def->u.usage);
    else if (def->type == VIR_SECRET_LOOKUP_TYPE_UUID)
        memset(&def->u.uuid, 0, VIR_UUID_BUFLEN);
}


void
virSecretLookupDefCopy(virSecretLookupTypeDef *dst,
                       const virSecretLookupTypeDef *src)
{
    dst->type = src->type;
    if (dst->type == VIR_SECRET_LOOKUP_TYPE_UUID) {
        memcpy(dst->u.uuid, src->u.uuid, VIR_UUID_BUFLEN);
    } else if (dst->type == VIR_SECRET_LOOKUP_TYPE_USAGE) {
        dst->u.usage = g_strdup(src->u.usage);
    }
}


int
virSecretLookupParseSecret(xmlNodePtr secretnode,
                           virSecretLookupTypeDef *def)
{
    g_autofree char *usage = NULL;
    int rc;

    usage = virXMLPropString(secretnode, "usage");

    if ((rc = virXMLPropUUID(secretnode, "uuid",
                             VIR_XML_PROP_NONE, def->u.uuid)) < 0) {
        return -1;
    }

    if (rc > 0) {
        if (usage) {
            virReportError(VIR_ERR_XML_ERROR, "%s",
                           _("either secret uuid or usage expected"));
            return -1;
        }

        def->type = VIR_SECRET_LOOKUP_TYPE_UUID;
    } else {
        if (!usage) {
            virReportError(VIR_ERR_XML_ERROR, "%s",
                           _("missing secret uuid or usage attribute"));
            return -1;
        }

        def->u.usage = g_steal_pointer(&usage);
        def->type = VIR_SECRET_LOOKUP_TYPE_USAGE;
    }

    return 0;
}


void
virSecretLookupFormatSecret(virBuffer *buf,
                            const char *secrettype,
                            virSecretLookupTypeDef *def)
{
    char uuidstr[VIR_UUID_STRING_BUFLEN];

    if (secrettype)
        virBufferAsprintf(buf, "<secret type='%s'", secrettype);
    else
        virBufferAddLit(buf, "<secret");

    if (def->type == VIR_SECRET_LOOKUP_TYPE_UUID) {
        virUUIDFormat(def->u.uuid, uuidstr);
        virBufferAsprintf(buf, " uuid='%s'/>\n", uuidstr);
    } else if (def->type == VIR_SECRET_LOOKUP_TYPE_USAGE) {
        virBufferEscapeString(buf, " usage='%s'/>\n", def->u.usage);
    } else {
        virBufferAddLit(buf, "/>\n");
    }
}


/* virSecretGetSecretString:
 * @conn: Pointer to the connection driver to make secret driver call
 * @seclookupdef: Secret lookup def
 * @secretUsageType: Type of secret usage for usage lookup
 * @secret: returned secret as a sized stream of unsigned chars
 * @secret_size: Return size of the secret - either raw text or base64
 *
 * Lookup the secret for the usage type and return it as raw text.
 * It is up to the caller to encode the secret further.
 *
 * Returns 0 on success, -1 on failure.  On success the memory in secret
 * needs to be cleared and free'd after usage.
 */
int
virSecretGetSecretString(virConnectPtr conn,
                         virSecretLookupTypeDef *seclookupdef,
                         virSecretUsageType secretUsageType,
                         uint8_t **secret,
                         size_t *secret_size)
{
    g_autoptr(virSecret) sec = NULL;

    switch (seclookupdef->type) {
    case VIR_SECRET_LOOKUP_TYPE_UUID:
        sec = conn->secretDriver->secretLookupByUUID(conn, seclookupdef->u.uuid);
        break;

    case VIR_SECRET_LOOKUP_TYPE_USAGE:
        sec = conn->secretDriver->secretLookupByUsage(conn, secretUsageType,
                                                      seclookupdef->u.usage);
        break;
    }

    if (!sec)
        return -1;

    /* NB: NONE is a byproduct of the qemuxmlconftest test mocking
     * for UUID lookups. Normal secret XML processing would fail if
     * the usage type was NONE and since we have no way to set the
     * expected usage in that environment, let's just accept NONE */
    if (sec->usageType != VIR_SECRET_USAGE_TYPE_NONE &&
        sec->usageType != secretUsageType) {
        char uuidstr[VIR_UUID_STRING_BUFLEN];

        virUUIDFormat(seclookupdef->u.uuid, uuidstr);
        virReportError(VIR_ERR_INVALID_ARG,
                       _("secret with uuid %1$s is of type '%2$s' not expected '%3$s' type"),
                       uuidstr, virSecretUsageTypeToString(sec->usageType),
                       virSecretUsageTypeToString(secretUsageType));
        return -1;
    }

    if (!(*secret = conn->secretDriver->secretGetValue(sec, secret_size, 0)))
        return -1;

    return 0;
}


/**
 * virSecretTPMAvailable:
 *
 * Checks whether systemd-creds is available and whether host supports
 * TPM. Use this prior calling other virSecretTPM*() APIs.
 *
 * Returns: 1 in case TPM is available,
 *          0 in case TPM is NOT available,
 *         -1 otherwise.
 */
int
virSecretTPMAvailable(void)
{
    g_autoptr(virCommand) cmd = NULL;
    g_autofree char *out = NULL;
    int exitstatus;
    int rc;

    cmd = virCommandNewArgList("systemd-creds", "has-tpm2", NULL);

    virCommandSetOutputBuffer(cmd, &out);

    rc = virCommandRun(cmd, &exitstatus);

    if (rc < 0) {
        /* systemd-creds not available */
        return -1;
    }

    if (!out || !*out) {
        /* systemd-creds reported nothing. Ouch. */
        return 0;
    }

    /* systemd-creds can report one of these:
     *
     * yes - TPM is available and recognized by FW, kernel, etc.
     * no - TPM is not available or not recognized
     * partial - otherwise
     */

    if (STRPREFIX(out, "yes\n"))
        return 1;

    return 0;
}


/**
 * virSecretTPMEncrypt:
 * @name: credential name
 * @value: credential value
 * @value_size: size of @value
 * @base64: encrypted @value, base64 encoded
 *
 * Encrypts given plaintext (@value) with a secret key automatically
 * derived from the system's TPM2 chip. Ciphertext is base64 encoded and
 * stored into @base64. Pass the same @name to virSecretTPMDecrypt().
 *
 * Returns: 0 on success,
 *          -1 otherwise (with error reported).
 */
int
virSecretTPMEncrypt(const char *name,
                    unsigned const char *value,
                    size_t value_size,
                    char **base64)
{
    g_autoptr(virCommand) cmd = NULL;
    g_autofree char *errBuf = NULL;
    int pipeFD[2] = { -1, -1 };
    int ret = -1;

    if (virPipe(pipeFD) < 0)
        return -1;

    if (virSetCloseExec(pipeFD[1]) < 0) {
        virReportSystemError(errno, "%s",
                             _("Unable to set cloexec flag"));
        goto cleanup;
    }

    cmd = virCommandNewArgList("systemd-creds", "encrypt",
                               "--with-key=tpm2", NULL);
    virCommandAddArgFormat(cmd, "--name=%s", name);
    virCommandAddArgList(cmd, "-", "-", NULL);

    virCommandSetInputFD(cmd, pipeFD[0]);
    virCommandSetOutputBuffer(cmd, base64);
    virCommandSetErrorBuffer(cmd, &errBuf);
    virCommandDoAsyncIO(cmd);

    if (virCommandRunAsync(cmd, NULL) < 0)
        goto cleanup;

    if (safewrite(pipeFD[1], value, value_size) != value_size) {
        virReportSystemError(errno, "%s",
                             _("Unable to pass secret value to systemd-cred"));
        goto cleanup;
    }

    if (VIR_CLOSE(pipeFD[1]) < 0) {
        virReportSystemError(errno, "%s", _("unable to close pipe"));
        goto cleanup;
    }

    if (virCommandWait(cmd, NULL) < 0) {
        if (errBuf && *errBuf) {
            VIR_WARN("systemd-creds reported: %s", errBuf);
        }
        goto cleanup;
    }

    ret = 0;
 cleanup:
    virCommandAbort(cmd);
    VIR_FORCE_CLOSE(pipeFD[0]);
    VIR_FORCE_CLOSE(pipeFD[1]);
    return ret;
}


/**
 * virSecretTPMDecrypt:
 * @name: credential name
 * @base64: encrypted @value, base64 encoded
 * @value: credential value,
 * @value_size: size of @value
 *
 * Decrypts given ciphertext, base64 encoded (@base64) and stores
 * plaintext into @value and its size into @value_size.
 *
 * Returns: 0 on success,
 *         -1 otherwise (with error reported).
 */
int
virSecretTPMDecrypt(const char *name,
                    const char *base64,
                    unsigned char **value,
                    size_t *value_size)
{
    g_autoptr(virCommand) cmd = NULL;
    g_autofree char *out = NULL;
    g_autofree char *errBuf = NULL;

    cmd = virCommandNewArgList("systemd-creds", "decrypt",
                               "--transcode=base64", NULL);
    virCommandAddArgFormat(cmd, "--name=%s", name);
    virCommandAddArgList(cmd, "-", "-", NULL);

    virCommandSetInputBuffer(cmd, base64);
    virCommandSetOutputBuffer(cmd, &out);
    virCommandSetErrorBuffer(cmd, &errBuf);

    if (virCommandRun(cmd, NULL) < 0) {
        if (errBuf && *errBuf) {
            VIR_WARN("systemd-creds reported: %s", errBuf);
        }
        return -1;
    }

    *value = g_base64_decode(out, value_size);

    return 0;
}
