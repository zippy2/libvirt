# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

FROM docker.io/library/debian:sid-slim

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
                      codespell \
                      cpp \
                      diffutils \
                      dwarves \
                      ebtables \
                      flake8 \
                      gettext \
                      git \
                      grep \
                      libclang-rt-dev \
                      libnbd-dev \
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
                      xsltproc && \
    eatmydata apt-get autoremove -y && \
    eatmydata apt-get autoclean -y && \
    sed -Ei 's,^# (en_US\.UTF-8 .*)$,\1,' /etc/locale.gen && \
    dpkg-reconfigure locales && \
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED

ENV CCACHE_WRAPPERSDIR "/usr/libexec/ccache-wrappers"
ENV LANG "en_US.UTF-8"
ENV MAKE "/usr/bin/make"
ENV NINJA "/usr/bin/ninja"
ENV PYTHON "/usr/bin/python3"

RUN export DEBIAN_FRONTEND=noninteractive && \
    dpkg --add-architecture armhf && \
    eatmydata apt-get update && \
    eatmydata apt-get dist-upgrade -y && \
    eatmydata apt-get install --no-install-recommends -y dpkg-dev && \
    eatmydata apt-get install --no-install-recommends -y \
                      gcc-arm-linux-gnueabihf \
                      libacl1-dev:armhf \
                      libapparmor-dev:armhf \
                      libattr1-dev:armhf \
                      libaudit-dev:armhf \
                      libblkid-dev:armhf \
                      libc6-dev:armhf \
                      libcap-ng-dev:armhf \
                      libcurl4-gnutls-dev:armhf \
                      libdevmapper-dev:armhf \
                      libfuse-dev:armhf \
                      libglib2.0-dev:armhf \
                      libgnutls28-dev:armhf \
                      libiscsi-dev:armhf \
                      libjson-c-dev:armhf \
                      libnl-3-dev:armhf \
                      libnl-route-3-dev:armhf \
                      libnuma-dev:armhf \
                      libparted-dev:armhf \
                      libpcap0.8-dev:armhf \
                      libpciaccess-dev:armhf \
                      libreadline-dev:armhf \
                      libsanlock-dev:armhf \
                      libsasl2-dev:armhf \
                      libselinux1-dev:armhf \
                      libssh-dev:armhf \
                      libssh2-1-dev:armhf \
                      libtirpc-dev:armhf \
                      libudev-dev:armhf \
                      libxen-dev:armhf \
                      libxml2-dev:armhf \
                      systemtap-sdt-dev:armhf && \
    eatmydata apt-get autoremove -y && \
    eatmydata apt-get autoclean -y && \
    mkdir -p /usr/local/share/meson/cross && \
    printf "[binaries]\n\
c = '/usr/bin/arm-linux-gnueabihf-gcc'\n\
ar = '/usr/bin/arm-linux-gnueabihf-gcc-ar'\n\
strip = '/usr/bin/arm-linux-gnueabihf-strip'\n\
pkgconfig = '/usr/bin/arm-linux-gnueabihf-pkg-config'\n\
\n\
[host_machine]\n\
system = 'linux'\n\
cpu_family = 'arm'\n\
cpu = 'armhf'\n\
endian = 'little'\n" > /usr/local/share/meson/cross/arm-linux-gnueabihf && \
    dpkg-query --showformat '${Package}_${Version}_${Architecture}\n' --show > /packages.txt && \
    mkdir -p /usr/libexec/ccache-wrappers && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/arm-linux-gnueabihf-cc && \
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/arm-linux-gnueabihf-gcc

ENV ABI "arm-linux-gnueabihf"
ENV MESON_OPTS "--cross-file=arm-linux-gnueabihf"
