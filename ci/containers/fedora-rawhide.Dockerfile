# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.fedoraproject.org/fedora:rawhide

RUN dnf update -y --nogpgcheck fedora-gpg-keys && \
    dnf install -y nosync && \
    printf '#!/bin/sh\n\
if test -d /usr/lib64\n\
then\n\
    export LD_PRELOAD=/usr/lib64/nosync/nosync.so\n\
else\n\
    export LD_PRELOAD=/usr/lib/nosync/nosync.so\n\
fi\n\
exec "$@"\n' > /usr/bin/nosync && \
    chmod +x /usr/bin/nosync && \
    nosync dnf distro-sync -y && \
    nosync dnf install -y \
               audit-libs-devel \
               augeas \
               bash-completion-devel \
               ca-certificates \
               ccache \
               clang \
               codespell \
               compiler-rt \
               cpp \
               cppi \
               cyrus-sasl-devel \
               device-mapper-devel \
               diffutils \
               dwarves \
               ebtables \
               firewalld-filesystem \
               fuse-devel \
               gcc \
               gettext \
               git \
               glib2-devel \
               glibc-devel \
               glibc-langpack-en \
               glusterfs-api-devel \
               gnutls-devel \
               grep \
               json-c-devel \
               libacl-devel \
               libattr-devel \
               libblkid-devel \
               libcap-ng-devel \
               libcurl-devel \
               libiscsi-devel \
               libnbd-devel \
               libnl3-devel \
               libpcap-devel \
               libpciaccess-devel \
               librbd-devel \
               libselinux-devel \
               libssh-devel \
               libssh2-devel \
               libtirpc-devel \
               libwsman-devel \
               libxml2 \
               libxml2-devel \
               libxslt \
               make \
               meson \
               ninja-build \
               numactl-devel \
               parted-devel \
               perl-base \
               pkgconfig \
               python3 \
               python3-black \
               python3-docutils \
               python3-flake8 \
               python3-pytest \
               qemu-img \
               readline-devel \
               rpm-build \
               sanlock-devel \
               sed \
               systemd-devel \
               systemd-rpm-macros \
               systemtap-sdt-devel \
               systemtap-sdt-dtrace \
               wireshark-devel \
               xen-devel && \
    nosync dnf autoremove -y && \
    nosync dnf clean all -y && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED && \
    rpm -qa | sort > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/clang && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"
