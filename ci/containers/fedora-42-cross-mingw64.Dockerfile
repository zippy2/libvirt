# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.fedoraproject.org/fedora:42

RUN dnf install -y nosync && \
    printf '#!/bin/sh\n\
if test -d /usr/lib64\n\
then\n\
    export LD_PRELOAD=/usr/lib64/nosync/nosync.so\n\
else\n\
    export LD_PRELOAD=/usr/lib/nosync/nosync.so\n\
fi\n\
exec "$@"\n' > /usr/bin/nosync && \
    chmod +x /usr/bin/nosync && \
    nosync dnf update -y && \
    nosync dnf install -y \
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
    nosync dnf autoremove -y && \
    nosync dnf clean all -y && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED

ENV CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
ENV LANG="en_US.UTF-8"
ENV MAKE="/usr/bin/make"
ENV NINJA="/usr/bin/ninja"
ENV PYTHON="/usr/bin/python3"

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
               mingw64-readline && \
    nosync dnf clean all -y && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/x86_64-w64-mingw32-cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/x86_64-w64-mingw32-gcc

<<<<<<< HEAD
ENV ABI="x86_64-w64-mingw32"
ENV MESON_OPTS="--cross-file=/usr/share/mingw/toolchain-mingw64.meson"
=======
ENV ABI "x86_64-w64-mingw32"
<<<<<<< HEAD
ENV MESON_OPTS "--cross-file=x86_64-w64-mingw32"
>>>>>>> 3f5cae2ca8 (ci: Regenerate files)
=======
ENV MESON_OPTS "--cross-file=/usr/share/mingw/toolchain-mingw64.meson"
>>>>>>> f7f38b0d87 (sysctl)
