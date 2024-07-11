/*
 * virt-host-validate.c: Sanity check a hypervisor host
 *
 * Copyright (C) 2012 Red Hat, Inc.
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

#ifdef WITH_LIBINTL_H
# include <libintl.h>
#endif /* WITH_LIBINTL_H */
#include <getopt.h>

#include "internal.h"
#include "virerror.h"
#include "virgettext.h"
#include "virglibutil.h"

#include "virt-host-validate-common.h"
#if WITH_QEMU
# include "virt-host-validate-qemu.h"
#endif
#if WITH_LXC
# include "virt-host-validate-lxc.h"
#endif
#if WITH_BHYVE
# include "virt-host-validate-bhyve.h"
#endif
#if WITH_CH
# include "virt-host-validate-ch.h"
#endif

typedef struct _virValidateCallbacks virValidateCallbacks;
struct _virValidateCallbacks {
    const char *name;
    int (*callback)(void);
};

static virValidateCallbacks validateCallbacks[] = {
#if WITH_QEMU
    { "qemu", virHostValidateQEMU },
#endif
#if WITH_LXC
    { "lxc", virHostValidateLXC },
#endif
#if WITH_BHYVE
    { "bhyve", virHostValidateBhyve },
#endif
#if WITH_CH
    { "ch", virHostValidateCh },
#endif
};

static void
show_help(FILE *out, const char *argv0)
{
    g_autofree char *hvs = NULL;
    char *hvs_list[G_N_ELEMENTS(validateCallbacks) + 1] = { };
    size_t i;

    for (i = 0; i < G_N_ELEMENTS(validateCallbacks); i++) {
        hvs_list[i] = g_strdup_printf("   - %1$s", validateCallbacks[i].name);
    }

    hvs = g_strjoinv("\n", hvs_list);

    for (i = 0; i < G_N_ELEMENTS(validateCallbacks); i++) {
        g_free(hvs_list[i]);
    }

    fprintf(out,
            _("\n"
              "syntax: %1$s [OPTIONS] [HVTYPE]\n"
              "\n"
              " Hypervisor types:\n"
              "\n"
              "%2$s\n"
              "\n"
              " Options:\n"
              "   -h, --help     Display command line help\n"
              "   -v, --version  Display command version\n"
              "   -q, --quiet    Don't display progress information\n"
              "\n"),
            argv0, hvs);
}

static void
show_version(FILE *out, const char *argv0)
{
    fprintf(out, "version: %s %s\n", argv0, VERSION);
}

static const struct option argOptions[] = {
    { "help", 0, NULL, 'h' },
    { "version", 0, NULL, 'v' },
    { "quiet", 0, NULL, 'q' },
    { NULL, 0, NULL, '\0' },
};

int
main(int argc, char **argv)
{
    const char *hvname = NULL;
    int c;
    int ret = EXIT_SUCCESS;
    bool quiet = false;
    bool usedHvname = false;
    size_t i;

    if (virGettextInitialize() < 0 ||
        virErrorInitialize() < 0) {
        fprintf(stderr, _("%1$s: initialization failed\n"), argv[0]);
        return EXIT_FAILURE;
    }

    while ((c = getopt_long(argc, argv, "hvq", argOptions, NULL)) != -1) {
        switch (c) {
        case 'v':
            show_version(stdout, argv[0]);
            return EXIT_SUCCESS;

        case 'h':
            show_help(stdout, argv[0]);
            return EXIT_SUCCESS;

        case 'q':
            quiet = true;
            break;

        case '?':
        default:
            show_help(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if ((argc-optind) > 2) {
        fprintf(stderr, _("%1$s: too many command line arguments\n"), argv[0]);
        show_help(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    if (argc > 1)
        hvname = argv[optind];

    virValidateSetQuiet(quiet);

    for (i = 0; i < G_N_ELEMENTS(validateCallbacks); i++) {
        if (!hvname || STREQ(hvname, validateCallbacks[i].name)) {
            usedHvname = true;
            if (validateCallbacks[i].callback() < 0)
                ret = EXIT_FAILURE;
        }
    }

    if (hvname && !usedHvname) {
        fprintf(stderr, _("%1$s: unsupported hypervisor name %2$s\n"),
                argv[0], hvname);
        return EXIT_FAILURE;
    }

    return ret;
}
