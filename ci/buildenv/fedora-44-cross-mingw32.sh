# THIS FILE WAS AUTO-GENERATED
#
#  $ lcitool manifest ci/manifest.yml
#
# https://gitlab.com/libvirt/libvirt-ci

function install_buildenv() {
    dnf --quiet update -y
    dnf --quiet install -y \
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
                systemd-rpm-macros
    rm -f /usr/lib*/python3*/EXTERNALLY-MANAGED
<<<<<<< HEAD
    dnf --quiet install -y \
                mingw32-curl \
                mingw32-dlfcn \
                mingw32-gcc \
                mingw32-gettext \
                mingw32-glib2 \
                mingw32-gnutls \
                mingw32-headers \
                mingw32-libssh2 \
                mingw32-libxml2 \
                mingw32-pkg-config \
                mingw32-portablexdr \
                mingw32-readline
=======
    dnf install -y \
        mingw32-curl \
        mingw32-dlfcn \
        mingw32-gcc \
        mingw32-gettext \
        mingw32-glib2 \
        mingw32-gnutls \
        mingw32-headers \
        mingw32-libssh2 \
        mingw32-libxml2 \
        mingw32-pkg-config \
        mingw32-portablexdr \
<<<<<<< HEAD
        mingw32-readline \
        wine
    mkdir -p /usr/local/share/meson/cross
    printf "[binaries]\n\
c = '/usr/bin/i686-w64-mingw32-gcc'\n\
cpp = '/usr/bin/i686-w64-mingw32-g++'\n\
fortran = '/usr/bin/i686-w64-mingw32-gfortran'\n\
rust = ['rustc', '--target', 'i686-pc-windows-msvc', '-C', 'linker=/usr/bin/i686-w64-mingw32-gcc']\n\
ar = '/usr/bin/i686-w64-mingw32-ar'\n\
pkgconfig = '/usr/bin/i686-w64-mingw32-pkg-config'\n\
ranlib = '/usr/bin/x86_64-w64-mingw32-ranlib'\n\
strip = '/usr/bin/i686-w64-mingw32-strip'\n\
windres = '/usr/bin/i686-w64-mingw32-windres'\n\
dlltool = '/usr/bin/i686-w64-mingw32-dlltool'\n\
libgcrypt-config = '/usr/i686-w64-mingw32/sys-root/mingw/bin/libgcrypt-config'\n\
glib-mkenums = '/usr/i686-w64-mingw32/sys-root/mingw/bin/glib-mkenums'\n\
exe_wrapper = 'wine'\n\
\n\
[properties]\n\
root = '/usr/i686-w64-mingw32/sys-root/mingw'\n\
needs_exe_wrapper = true\n\
\n\
[host_machine]\n\
system = 'windows'\n\
cpu_family = 'x86'\n\
cpu = 'i686'\n\
endian = 'little'\n" > /usr/local/share/meson/cross/i686-w64-mingw32
>>>>>>> 68c4fe9177 (ci: Regenerate files)
=======
        mingw32-readline
>>>>>>> 7507cf0ac8 (sysctl)
    rpm -qa | sort > /packages.txt
    mkdir -p /usr/libexec/ccache-wrappers
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/i686-w64-mingw32-cc
    ln -s /usr/bin/ccache /usr/libexec/ccache-wrappers/i686-w64-mingw32-gcc
}

export CCACHE_WRAPPERSDIR="/usr/libexec/ccache-wrappers"
export LANG="en_US.UTF-8"
export MAKE="/usr/bin/make"
export NINJA="/usr/bin/ninja"
export PYTHON="/usr/bin/python3"

export ABI="i686-w64-mingw32"
export MESON_OPTS="--cross-file=/usr/share/mingw/toolchain-mingw32.meson"
