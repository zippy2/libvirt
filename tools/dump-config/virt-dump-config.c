/*
 * virt-dump-config.c: tool to dump libvirt configuration
 *
 * Copyright (C) 2022 Red Hat, Inc.
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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "internal.h"
#include "virerror.h"
#include "virgettext.h"
#include "virjson.h"
#include "virxml.h"

#define VIR_FROM_THIS VIR_FROM_NONE

static void
show_help(FILE *out, const char *argv0)
{
    fprintf(out,
            _("\n"
              "syntax: %s [OPTIONS]\n"
              "\n"
              " Options:\n"
              "   -h, --help     Display command line help\n"
              "   -v, --version  Display command version\n"
              "\n"),
            argv0);
}


static void
show_version(FILE *out, const char *argv0)
{
    fprintf(out, "version: %s %s\n", argv0, VERSION);
}


static void
hideErrorFunc(void *opaque G_GNUC_UNUSED,
              virErrorPtr err G_GNUC_UNUSED)
{
}


static virJSONValue *
doDumpHost(virConnect *conn)
{
    g_autoptr(virJSONValue) ret = NULL;
    unsigned long libVer = 0;
    unsigned long hvVer = 0;
    g_autofree char *caps = NULL;

    if (!(ret = virJSONValueNewObject()))
        return NULL;

    if (virConnectGetLibVersion(conn, &libVer) >= 0 &&
        virJSONValueObjectAppendNumberUlong(ret, "version", libVer) < 0)
        return NULL;

    if (virConnectGetVersion(conn, &hvVer) >= 0 &&
        hvVer != 0 &&
        virJSONValueObjectAppendNumberUlong(ret, "hypervisor-version", hvVer) < 0)
        return NULL;

    /* Parse capabilities XML so that some values can be stored directly in
     * the output JSON. */
    if ((caps = virConnectGetCapabilities(conn))) {
        g_autoptr(xmlDoc) xml = NULL;
        g_autoptr(xmlXPathContext) ctxt = NULL;
        g_autofree char *uuid = NULL;

        if (!(xml = virXMLParseStringCtxt(caps, _("(capabilities)"), &ctxt)))
            return NULL;

        if ((uuid = virXPathString("string(/capabilities/host/uuid)", ctxt)) &&
            virJSONValueObjectAppendString(ret, "uuid", uuid) < 0)
            return NULL;

        if (virJSONValueObjectAppendString(ret, "capabilities", caps) < 0)
            return NULL;
    }

    return g_steal_pointer(&ret);
}


static virJSONValue *
dumpOneGuest(virDomainPtr dom)
{
    g_autoptr(virJSONValue) ret = NULL;
    const char *name = NULL;
    char uuid[VIR_UUID_STRING_BUFLEN] = { 0 };
    unsigned int id = 0;
    int ms = 0;
    int pers = 0;
    g_autofree char *xml = NULL;

    if (!(ret = virJSONValueNewObject()))
        return NULL;

    if ((name = virDomainGetName(dom)) &&
        virJSONValueObjectAppendString(ret, "name", name) < 0)
        return NULL;

    if (virDomainGetUUIDString(dom, uuid) >= 0 &&
        virJSONValueObjectAppendString(ret, "uuid", uuid) < 0)
        return NULL;

    if (virDomainIsActive(dom) == 1 &&
        (id = virDomainGetID(dom)) != -1 &&
        virJSONValueObjectAppendNumberUint(ret, "id", id) < 0)
        return NULL;

    if ((pers = virDomainIsPersistent(dom)) >= 0 &&
        virJSONValueObjectAppendBoolean(ret, "persistent", pers) < 0)
        return NULL;

    if ((ms = virDomainHasManagedSaveImage(dom, 0)) >= 0 &&
        virJSONValueObjectAppendBoolean(ret, "managed-save", ms) < 0)
        return NULL;

    if ((xml = virDomainGetXMLDesc(dom, 0)) &&
        virJSONValueObjectAppendString(ret, "xml", xml) < 0)
        return NULL;

    return g_steal_pointer(&ret);
}


static virJSONValue *
doDumpGuests(virConnect *conn)
{
    g_autoptr(virJSONValue) arr = NULL;
    virJSONValue *ret = NULL;
    virDomainPtr *domains = NULL;
    size_t i;
    int n;

    if (!(arr = virJSONValueNewArray()))
        return NULL;

    if ((n = virConnectListAllDomains(conn, &domains, 0)) < 0)
        return NULL;

    for (i = 0; i < n; i++) {
        g_autoptr(virJSONValue) item = NULL;

        if (!(item = dumpOneGuest(domains[i])))
            continue;

        if (virJSONValueArrayAppend(arr, &item) < 0)
            goto cleanup;
    }

    ret = g_steal_pointer(&arr);
 cleanup:
    if (n > 0) {
        for (i = 0; i < n; i++)
            virDomainFree(domains[i]);
        g_free(domains);
    }
    return ret;
}


static virJSONValue *
dumpOneNetwork(virNetworkPtr net)
{
    g_autoptr(virJSONValue) ret = NULL;
    const char *name = NULL;
    char uuid[VIR_UUID_STRING_BUFLEN] = { 0 };
    int active = 0;
    int pers = 0;
    g_autofree char *xml = NULL;

    if (!(ret = virJSONValueNewObject()))
        return NULL;

    if ((name = virNetworkGetName(net)) &&
        virJSONValueObjectAppendString(ret, "name", name) < 0)
        return NULL;

    if (virNetworkGetUUIDString(net, uuid) >= 0 &&
        virJSONValueObjectAppendString(ret, "uuid", uuid) < 0)
        return NULL;

    if ((active = virNetworkIsActive(net)) >= 0 &&
        virJSONValueObjectAppendBoolean(ret, "active", active) < 0)
        return NULL;

    if ((pers = virNetworkIsPersistent(net)) >= 0 &&
        virJSONValueObjectAppendBoolean(ret, "persistent", pers) < 0)
        return NULL;

    if ((xml = virNetworkGetXMLDesc(net, 0)) &&
        virJSONValueObjectAppendString(ret, "xml", xml) < 0)
        return NULL;

    return g_steal_pointer(&ret);
}


static virJSONValue *
doDumpNetworks(virConnectPtr conn)
{
    g_autoptr(virJSONValue) arr = NULL;
    virJSONValue *ret = NULL;
    virNetworkPtr *networks = NULL;
    size_t i;
    int n;

    if (!(arr = virJSONValueNewArray()))
        return NULL;

    if ((n = virConnectListAllNetworks(conn, &networks, 0)) < 0)
        return NULL;

    for (i = 0; i < n; i++) {
        g_autoptr(virJSONValue) item = NULL;

        if (!(item = dumpOneNetwork(networks[i])))
            continue;

        if (virJSONValueArrayAppend(arr, &item) < 0)
            goto cleanup;
    }

    ret = g_steal_pointer(&arr);
 cleanup:
    if (n > 0) {
        for (i = 0; i < n; i++)
            virNetworkFree(networks[i]);
        g_free(networks);
    }
    return ret;
}


static int
doDump(const char *uri)
{
    int ret = -1;
    virConnectPtr conn = NULL;
    g_autoptr(virJSONValue) cfg = NULL;
    g_autoptr(virJSONValue) host = NULL;
    g_autoptr(virJSONValue) guests = NULL;
    g_autoptr(virJSONValue) nets = NULL;
    g_autofree char *str = NULL;
    g_autofree char *cannon_uri = NULL;

    if (!(conn = virConnectOpenReadOnly(uri)))
        goto cleanup;

    if (!(cfg = virJSONValueNewObject()))
        goto cleanup;

    if (virJSONValueObjectAppendString(cfg, "version", PACKAGE_VERSION) < 0)
        goto cleanup;

    if ((cannon_uri = virConnectGetURI(conn)) &&
        virJSONValueObjectAppendString(cfg, "uri", cannon_uri) < 0)
        goto cleanup;

    if (!(host = doDumpHost(conn)) ||
        virJSONValueObjectAppend(cfg, "host", &host) < 0)
        goto cleanup;

    if (!(guests = doDumpGuests(conn)) ||
        virJSONValueObjectAppend(cfg, "domains", &guests) < 0)
        goto cleanup;

    if (!(nets = doDumpNetworks(conn)) ||
        virJSONValueObjectAppend(cfg, "networks", &nets) < 0)
        goto cleanup;

    if (!(str = virJSONValueToString(cfg, true)))
        goto cleanup;

    printf("%s", str);
    fflush(stdout);

    ret = 0;
 cleanup:
    if (conn)
        virConnectClose(conn);
    return ret;
}


int
main(int argc, char **argv)
{
    const char *uri = NULL;
    int c;
    static const struct option argOptions[] = {
        { "help", no_argument, NULL, 'h', },
        { "version", no_argument, NULL, 'v', },
        {"connect", required_argument, NULL, 'c'},
        { NULL, 0, NULL, '\0', }
    };

    if (virInitialize() < 0) {
        fprintf(stderr, _("%s: initialization failed\n"), argv[0]);
        return EXIT_FAILURE;
    }

    virSetErrorFunc(NULL, hideErrorFunc);
    virSetErrorLogPriorityFunc(NULL);

    if (virGettextInitialize() < 0)
        return EXIT_FAILURE;

    while ((c = getopt_long(argc, argv, "hvc:", argOptions, NULL)) != -1) {
        switch (c) {
        case 'v':
            show_version(stdout, argv[0]);
            return EXIT_SUCCESS;

        case 'h':
            show_help(stdout, argv[0]);
            return EXIT_SUCCESS;

        case 'c':
            uri = optarg;
            break;

        case '?':
        default:
            show_help(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if ((argc-optind) > 0) {
        fprintf(stderr, _("%s: too many command line arguments\n"), argv[0]);
        show_help(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    if (doDump(uri) < 0)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}
