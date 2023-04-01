/*
 * virarchmock.c: Mock some virArch functions
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

#include "virarch.h"

/**
 * virArchFromHost:
 *
 * Return whatever architecture was set by virTestSetHostArch(),
 * or VIR_ARCH_X86_64 by default.
 */
virArch
virArchFromHost(void)
{
    const char *archStr = g_getenv("VIR_TEST_HOST_ARCH");

    if (archStr)
        return virArchFromString(archStr);

    return VIR_ARCH_X86_64;
}
