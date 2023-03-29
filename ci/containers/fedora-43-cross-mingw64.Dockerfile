# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.fedoraproject.org/fedora:43

RUN dnf --quiet install -y nosync && \
    printf '#!/bin/sh\n\
if test -d /usr/lib64\n\
then\n\
    export LD_PRELOAD=/usr/lib64/nosync/nosync.so\n\
else\n\
    export LD_PRELOAD=/usr/lib/nosync/nosync.so\n\
fi\n\
exec "$@"\n' > /usr/bin/nosync && \
    chmod +x /usr/bin/nosync && \
    nosync dnf --quiet update -y && \
    nosync dnf --quiet install -y \
                       augeas \
                       bash-completion-devel \
                       ca-certificates \
                       ccache \
                       codespell \
                       compiler-rt \
                       cpp \
                       cppi \
                       diffutils \
                       dwarves \
                       ebtables \
                       firewalld-filesystem \
                       gettext \
                       git \
                       glibc-langpack-en \
                       grep \
                       libnbd-devel \
                       libxml2 \
                       libxslt \
                       make \
                       meson \
                       ninja-build \
                       perl-base \
                       python3 \
                       python3-black \
                       python3-docutils \
                       python3-flake8 \
                       python3-pytest \
                       qemu-img \
                       rpm-build \
                       sed \
                       systemd-rpm-macros && \
    nosync dnf --quiet autoremove -y && \
    nosync dnf --quiet clean all -y && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED

ENV CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
ENV LANG="en_US.UTF-8"
ENV MAKE="/usr/bin/make"
ENV NINJA="/usr/bin/ninja"
ENV PYTHON="/usr/bin/python3"

<<<<<<< HEAD
RUN nosync dnf --quiet install -y \
                       mingw64-curl \
                       mingw64-dlfcn \
                       mingw64-gcc \
                       mingw64-gettext \
                       mingw64-glib2 \
                       mingw64-gnutls \
                       mingw64-headers \
                       mingw64-libssh2 \
                       mingw64-libxml2 \
                       mingw64-pkg-config \
                       mingw64-portablexdr \
                       mingw64-readline && \
    nosync dnf --quiet clean all -y && \
=======
RUN nosync dnf install -y \
               mingw64-curl \
               mingw64-dlfcn \
               mingw64-gcc \
               mingw64-gettext \
               mingw64-glib2 \
               mingw64-gnutls \
               mingw64-headers \
               mingw64-libssh2 \
               mingw64-libxml2 \
               mingw64-pkg-config \
               mingw64-portablexdr \
               mingw64-readline \
               wine && \
    nosync dnf clean all -y && \
    mkdir -p /usr/local/share/meson/cross && \
    printf "[binaries]\n\
c = '/usr/bin/x86_64-w64-mingw32-gcc'\n\
cpp = '/usr/bin/x86_64-w64-mingw32-g++'\n\
fortran = '/usr/bin/x86_64-w64-mingw32-gfortran'\n\
rust = ['rustc', '--target', 'x86_64-pc-windows-msvc', '-C', 'linker=/usr/bin/x86_64-w64-mingw32-gcc']\n\
ar = '/usr/bin/x86_64-w64-mingw32-ar'\n\
pkgconfig = '/usr/bin/x86_64-w64-mingw32-pkg-config'\n\
ranlib = '/usr/bin/x86_64-w64-mingw32-ranlib'\n\
strip = '/usr/bin/x86_64-w64-mingw32-strip'\n\
windres = '/usr/bin/x86_64-w64-mingw32-windres'\n\
dlltool = '/usr/bin/x86_64-w64-mingw32-dlltool'\n\
libgcrypt-config = '/usr/x86_64-w64-mingw32/sys-root/mingw/bin/libgcrypt-config'\n\
glib-mkenums = '/usr/x86_64-w64-mingw32/sys-root/mingw/bin/glib-mkenums'\n\
exe_wrapper = 'wine'\n\
\n\
[properties]\n\
root = '/usr/x86_64-w64-mingw32/sys-root/mingw'\n\
needs_exe_wrapper = true\n\
\n\
[host_machine]\n\
system = 'windows'\n\
cpu_family = 'x86_64'\n\
cpu = 'x86_64'\n\
endian = 'little'\n" > /usr/local/share/meson/cross/x86_64-w64-mingw32 && \
>>>>>>> 68c4fe9177 (ci: Regenerate files)
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/x86_64-w64-mingw32-cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/x86_64-w64-mingw32-gcc

<<<<<<< HEAD
ENV ABI="x86_64-w64-mingw32"
ENV MESON_OPTS="--cross-file=/usr/share/mingw/toolchain-mingw64.meson"
=======
ENV ABI "x86_64-w64-mingw32"
ENV MESON_OPTS "--cross-file=x86_64-w64-mingw32"
>>>>>>> 3f5cae2ca8 (ci: Regenerate files)
