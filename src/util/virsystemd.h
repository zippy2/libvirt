/*
 * virsystemd.h: helpers for using systemd APIs
 *
 * Copyright (C) 2013 Red Hat, Inc.
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

#pragma once

#include "internal.h"
#include "virsocketaddr.h"

typedef struct _virSystemdActivation virSystemdActivation;

char *virSystemdMakeScopeName(const char *name,
                              const char *drivername,
                              bool legacy_behaviour);
char *virSystemdMakeSliceName(const char *partition);

int virSystemdCreateMachine(const char *name,
                            const char *drivername,
                            const unsigned char *uuid,
                            const char *rootdir,
                            pid_t pidleader,
                            bool iscontainer,
                            size_t nnicindexes,
                            int *nicindexes,
                            const char *partition,
                            unsigned int maxthreads,
                            bool daemonDomainShutdown);

int virSystemdTerminateMachine(const char *name);

void virSystemdNotifyReady(void);
void virSystemdNotifyReload(void);
void virSystemdNotifyStopping(void);
void virSystemdNotifyExtendTimeout(int secs);
void virSystemdNotifyStatus(const char *fmt, ...) G_GNUC_PRINTF(1, 2);

int virSystemdHasMachined(void);

int virSystemdHasLogind(void);

int virSystemdHasResolved(void);

int virSystemdCanSuspend(bool *result);

int virSystemdCanHibernate(bool *result);

int virSystemdCanHybridSleep(bool *result);

char *virSystemdGetMachineNameByPID(pid_t pid);

char *virSystemdGetMachineUnitByPID(pid_t pid);

int virSystemdGetActivation(virSystemdActivation **act);

bool virSystemdActivationHasName(virSystemdActivation *act,
                                 const char *name);

int virSystemdActivationComplete(virSystemdActivation *act);

void virSystemdActivationClaimFDs(virSystemdActivation *act,
                                  const char *name,
                                  int **fds,
                                  size_t *nfds);

void virSystemdActivationFree(virSystemdActivation *act);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(virSystemdActivation, virSystemdActivationFree);

int virSystemdResolvedRegisterNameServer(int link,
                                         const char *domain,
                                         virSocketAddr *addr);
