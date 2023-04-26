# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM registry.opensuse.org/opensuse/tumbleweed:latest

RUN zypper dist-upgrade -y && \
    zypper install -y \
           audit-devel \
           augeas \
           augeas-lenses \
           bash-completion-devel \
           ca-certificates \
           ccache \
           clang \
           clang-devel \
           codespell \
           cpp \
           cppi \
           cyrus-sasl-devel \
           device-mapper-devel \
           diffutils \
           dwarves \
           ebtables \
           fuse-devel \
           gcc \
           gettext-runtime \
           git \
           glib2-devel \
           glibc-devel \
           glibc-locale \
           glusterfs-devel \
           grep \
           iproute2 \
           iptables \
           kmod \
           libacl-devel \
           libapparmor-devel \
           libattr-devel \
           libblkid-devel \
           libcap-ng-devel \
           libcurl-devel \
           libgnutls-devel \
           libiscsi-devel \
           libjson-c-devel \
           libnbd-devel \
           libnl3-devel \
           libnuma-devel \
           libpcap-devel \
           libpciaccess-devel \
           librbd-devel \
           libselinux-devel \
           libssh-devel \
           libssh2-devel \
           libtirpc-devel \
           libudev-devel \
           libwsman-devel \
           libxml2 \
           libxml2-devel \
           libxslt \
           lvm2 \
           make \
           meson \
           nfs-utils \
           ninja \
           numad \
           open-iscsi \
           parted-devel \
           perl-base \
           pkgconfig \
           polkit \
           procps-ng \
           python3-base \
           python3-black \
           python3-docutils \
           python3-flake8 \
           python3-pytest \
           qemu-tools \
           readline-devel \
           rpm-build \
           sanlock-devel \
           sed \
           systemd-rpm-macros \
           systemtap-dtrace \
           systemtap-headers \
           wireshark-devel \
           xen-devel && \
    zypper clean --all && \
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
