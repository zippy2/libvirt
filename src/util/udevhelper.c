/*
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

#include <stdio.h>
#include <stdlib.h>

#include "configmake.h"
#include "viralloc.h"
#include "virgettext.h"
#include "virobject.h"
#include "virstring.h"
#include "virthread.h"
#include "virudev.h"
#include "virutil.h"

#define VIR_FROM_THIS VIR_FROM_SECURITY


static void
usage(const char *progname)
{
    fprintf(stderr,
            _("Usage: %s [device]\n"
              "\n"
              "This program is intended to be run from udev to\n"
              "maintain proper security labels on devices used by\n"
              "domains managed by libvirt.\n"
              "\n"
              "For given device (either passed as the only\n"
              "command line argument or set by DEVNODE environment\n"
              "variable) all the security labels are printed onto\n"
              "standard output.\n"), progname);
}


static int
printLabels(const char *device)
{
    char *filename = NULL;
    virUdevMgrPtr mgr = NULL;
    int ret = -1;
    virSecurityDeviceLabelDefPtr *labels = NULL;
    size_t i, nlabels = 0;
    const char *dacLabel = NULL;
    const char *seLabel = NULL;

    if (virAsprintf(&filename,
                    "%s/run/libvirt/qemu/devices.udev", LOCALSTATEDIR) < 0)
        goto cleanup;

    if (!(mgr = virUdevMgrNewFromFile(filename)))
        goto cleanup;

    if (virUdevMgrLookupLabels(mgr, device, &labels, &nlabels) < 0)
        goto cleanup;

    if (!labels) {
        /* Device not found in our DB. Claim success. */
        ret = 0;
        goto cleanup;
    }

    for (i = 0; i < nlabels; i++) {
        virSecurityDeviceLabelDefPtr tmp = labels[i];

        if (STREQ(tmp->model, "dac"))
            dacLabel = tmp->label;
        else if (STREQ(tmp->model, "selinux"))
            seLabel = tmp->label;
    }

    if (dacLabel)
        printf("%s", dacLabel);
    if (seLabel)
        printf(" %s", seLabel);
    printf("\n");

    ret = 0;
 cleanup:
    for (i = 0; i < nlabels; i++)
        virSecurityDeviceLabelDefFree(labels[i]);
    VIR_FREE(labels);
    virObjectUnref(mgr);
    return ret;
}


int
main(int argc, char *argv[])
{
    const char *device = NULL;
    int ret = EXIT_FAILURE;

    if (virGettextInitialize() < 0 ||
        virThreadInitialize() < 0 ||
        virErrorInitialize() < 0) {
        fprintf(stderr, _("%s: initialization failed\n"), argv[0]);
        exit(EXIT_FAILURE);
    }

    if (argc > 1)
        device = argv[1];
    if (!device)
        device = virGetEnvBlockSUID("DEVNODE");
    if (!device || STREQ(device, "-h") || STREQ(device, "--help")) {
        usage(argv[0]);
        if (device)
            ret = EXIT_SUCCESS;
        goto cleanup;
    }

    if (printLabels(device) < 0)
        goto cleanup;

    ret = EXIT_SUCCESS;
 cleanup:
    return ret;
}
