# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM docker.io/library/debian:12-slim

RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update && \
    apt-get install -y eatmydata && \
    eatmydata apt-get dist-upgrade -y && \
    eatmydata apt-get install --no-install-recommends -y \
                      augeas-lenses \
                      augeas-tools \
                      bash-completion \
                      black \
                      ca-certificates \
                      ccache \
                      clang \
                      codespell \
                      cpp \
                      diffutils \
                      dwarves \
                      ebtables \
                      flake8 \
                      gcc \
                      gettext \
                      git \
                      grep \
                      libacl1-dev \
                      libapparmor-dev \
                      libattr1-dev \
                      libaudit-dev \
                      libblkid-dev \
                      libc6-dev \
                      libcap-ng-dev \
                      libclang-rt-dev \
                      libcurl4-gnutls-dev \
                      libdevmapper-dev \
                      libfuse-dev \
                      libglib2.0-dev \
                      libglusterfs-dev \
                      libgnutls28-dev \
                      libiscsi-dev \
                      libjson-c-dev \
                      libnbd-dev \
                      libnl-3-dev \
                      libnl-route-3-dev \
                      libnuma-dev \
                      libparted-dev \
                      libpcap0.8-dev \
                      libpciaccess-dev \
                      librbd-dev \
                      libreadline-dev \
                      libsanlock-dev \
                      libsasl2-dev \
                      libselinux1-dev \
                      libssh-dev \
                      libssh2-1-dev \
                      libtirpc-dev \
                      libudev-dev \
                      libxen-dev \
                      libxml2-dev \
                      libxml2-utils \
                      locales \
                      make \
                      meson \
                      ninja-build \
                      perl-base \
                      pkgconf \
                      python3 \
                      python3-docutils \
                      python3-pytest \
                      qemu-utils \
                      sed \
                      systemtap-sdt-dev \
                      wireshark-dev \
                      xsltproc && \
    eatmydata apt-get autoremove -y && \
    eatmydata apt-get autoclean -y && \
    sed -Ei 's,^# (en_US\.UTF-8 .*)$,\1,' /etc/locale.gen && \
    dpkg-reconfigure locales && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED && \
    dpkg-query --showformat '${Package}_${Version}_${Architecture}\n' --show > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/clang && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/gcc

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"
