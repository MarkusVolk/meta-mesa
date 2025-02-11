SUMMARY = "A free implementation of the OpenGL API"
DESCRIPTION = "Mesa is an open-source implementation of the OpenGL specification - \
a system for rendering interactive 3D graphics.  \
A variety of device drivers allows Mesa to be used in many different environments \
ranging from software emulation to complete hardware acceleration for modern GPUs. \
Mesa is used as part of the overall Direct Rendering Infrastructure and X.org \
environment."

HOMEPAGE = "http://mesa3d.org"
BUGTRACKER = "https://bugs.freedesktop.org"
SECTION = "x11"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://docs/license.rst;md5=17a4ea65de7a9ab42437f3131e616a7f"

PE = "2"

OVERRIDES_append_aarch64 = ":armarch:arm64:64bit"
OVERRIDES_append_aarch64_be = ":armarch:arm64:64bit"
OVERRIDES_append_arm = ":armarch:arm32:32bit"
OVERRIDES_append_armeb = ":armarch:arm32:32bit"
OVERRIDES_append_x86 = ":x86arch:32bit"
OVERRIDES_append_x86-64 = ":x86arch:64bit"

DEPENDS = "expat makedepend-native flex-native bison-native libxml2-native zlib chrpath-replacement-native python3-mako-native gettext-native"
EXTRANATIVEPATH += "chrpath-native"
PROVIDES = " \
    ${@bb.utils.contains('PACKAGECONFIG', 'opengl', 'virtual/libgl', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'gles', 'virtual/libgles1 virtual/libgles2', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'egl', 'virtual/egl', '', d)} \
    ${@bb.utils.contains('PACKAGECONFIG', 'gbm', 'virtual/libgbm', '', d)} \
    virtual/mesa \
    "

inherit meson pkgconfig python3native gettext features_check


SRC_URI = "git://github.com/mesa3d/mesa.git;nobranch=1 \
           file://0001-meson.build-check-for-all-linux-host_os-combinations.patch \
           file://0002-meson.build-make-TLS-ELF-optional.patch \
           file://0001-meson-misdetects-64bit-atomics-on-mips-clang.patch \
           file://0001-futex.h-Define-__NR_futex-if-it-does-not-exist.patch \
           "

VERSION = "21.1.1"
SRCREV = "mesa-${VERSION}"
PV = "${VERSION}+git${SRCPV}"
S = "${WORKDIR}/git"

UPSTREAM_CHECK_GITTAGREGEX = "mesa-(?P<pver>\d+(\.\d+)+)"

CFLAGS += "-fcommon"

#because we cannot rely on the fact that all apps will use pkgconfig,
#make eglplatform.h independent of MESA_EGL_NO_X11_HEADER
do_install_append() {
    if ${@bb.utils.contains('PACKAGECONFIG', 'egl', 'true', 'false', d)}; then
        sed -i -e 's/^#elif defined(__unix__) && defined(EGL_NO_X11)$/#elif defined(__unix__) \&\& defined(EGL_NO_X11) || ${@bb.utils.contains('PACKAGECONFIG', 'x11', '0', '1', d)}/' ${D}${includedir}/EGL/eglplatform.h
    fi
}


# Unset these to stop python trying to report the target Python setup
_PYTHON_SYSCONFIGDATA_NAME[unexport] = "1"
STAGING_INCDIR[unexport] = "1"
STAGING_LIBDIR[unexport] = "1"

BBCLASSEXTEND = "native nativesdk"

ANY_OF_DISTRO_FEATURES_class-target = "opengl vulkan"

PLATFORMS ??= "${@bb.utils.filter('PACKAGECONFIG', 'x11 wayland', d)}"

export YOCTO_ALTERNATE_EXE_PATH = "${STAGING_LIBDIR}/llvm${MESA_LLVM_RELEASE}/llvm-config"
export YOCTO_ALTERNATE_MULTILIB_NAME = "${base_libdir}"
export LLVM_CONFIG = "${STAGING_BINDIR_NATIVE}/llvm-config${MESA_LLVM_RELEASE}"
export WANT_LLVM_RELEASE = "${MESA_LLVM_RELEASE}"

MESA_LLVM_RELEASE ?= "${LLVMVERSION}"

# set the MESA_BUILD_TYPE to either 'release' (default) or 'debug'
# by default the upstream mesa sources build a debug release
# here we assume the user will want a release build by default
MESA_BUILD_TYPE ?= "release"
def check_buildtype(d):
    _buildtype = d.getVar('MESA_BUILD_TYPE')
    if _buildtype not in ['release', 'debug']:
        bb.fatal("unknown build type (%s), please set MESA_BUILD_TYPE to either 'release' or 'debug'" % _buildtype)
    if _buildtype == 'debug':
        return 'debugoptimized'
    return 'plain'
MESON_BUILDTYPE = "${@check_buildtype(d)}"


def strip_comma(s):
    return s.strip(',')

EXTRA_OEMESON = " \
    -Dshared-glapi=true \
    -Dgallium-opencl=disabled \
    -Dglx-read-only-text=true \
    -Dplatforms='${@",".join("${PLATFORMS}".split())}' \
"

OVERRIDES_append = "${@bb.utils.contains_any('PACKAGECONFIG', [ 'r300', 'r600', 'radeonsi' ], ':amdgpu', '', d)}"
OVERRIDES_append_x86arch = "${@bb.utils.contains_any('PACKAGECONFIG', [ 'i915', 'iris' ], ':intelgpu', '', d)}"

PACKAGECONFIG_class-target ??= " \
    ${@bb.utils.contains('DISTRO_FEATURES', 'opengl', 'opengl egl gles gbm dri gallium', '', d)} \
    ${@bb.utils.contains('DISTRO_FEATURES', 'x11 opengl', 'x11 dri3', '', d)} \
    ${@bb.utils.contains('DISTRO_FEATURES', 'x11 vulkan', 'dri3', '', d)} \
    ${@bb.utils.filter('DISTRO_FEATURES', 'wayland vulkan', d)} \
    elf-tls \
    lto \
"

PACKAGECONFIG_append_amdgpu_class-target = " gallium gallium-llvm"

PACKAGECONFIG_class-native ?= "gbm dri egl opengl elf-tls"
PACKAGECONFIG_class-nativesdk ?= "gbm dri egl opengl elf-tls"

PACKAGECONFIG_remove_libc-musl = "elf-tls"

PACKAGECONFIG[lto] = ",,"

MESA_LTO_FLAGS ?= "-flto=${@oe.utils.cpu_count()}"

TARGET_CFLAGS += "${@bb.utils.contains('PACKAGECONFIG', 'lto', '${MESA_LTO_FLAGS}', '', d)}"
TARGET_LDFLAGS += "${@bb.utils.contains('PACKAGECONFIG', 'lto', '${TARGET_CFLAGS}', '', d)}"

# "gbm" requires "dri", "opengl"
PACKAGECONFIG[gbm] = "-Dgbm=true,-Dgbm=false"

X11_DEPS = "xorgproto virtual/libx11 libxext libxxf86vm libxdamage libxfixes xrandr"
# "x11" requires "opengl"
PACKAGECONFIG[x11] = ",-Dglx=disabled,${X11_DEPS}"
PACKAGECONFIG[elf-tls] = "-Delf-tls=true, -Delf-tls=false,elfutils"
PACKAGECONFIG[xvmc] = "-Dgallium-xvmc=true,-Dgallium-xvmc=false,libxvmc"
PACKAGECONFIG[wayland] = ",,wayland-native wayland libdrm wayland-protocols"

DRIDRIVERS_class-native = "nouveau"
DRIDRIVERS_class-nativesdk = "nouveau"
DRIDRIVERS_append_x86_class-target = ",r100,r200,nouveau,i965"
DRIDRIVERS_append_x86-64_class-target = ",r100,r200,nouveau,i965"
# "dri" requires "opengl"
PACKAGECONFIG[dri] = "-Ddri-drivers=${@strip_comma('${DRIDRIVERS}')}, -Ddri-drivers='', xorgproto libdrm"
PACKAGECONFIG[dri3] = "-Ddri3=true, -Ddri3=false, xorgproto libxshmfence"

# Vulkan drivers need dri3 enabled
# amd could be enabled as well but requires gallium-llvm with llvm >= 3.9

VULKAN_DRIVERS = ""
VULKAN_DRIVERS_append_amdgpu_class-target = ",amd"
VULKAN_DRIVERS_append_intelgpu_class-target = ",intel"
VULKAN_DRIVERS_append_rpi = ",broadcom"

PACKAGECONFIG[vulkan] = "-Dvulkan-drivers=${@strip_comma('${VULKAN_DRIVERS}')}, -Dvulkan-drivers='',"

PACKAGECONFIG[opengl] = "-Dopengl=true, -Dopengl=false"

# "gles" requires "opengl"
PACKAGECONFIG[gles] = "-Dgles1=true -Dgles2=true, -Dgles1=false -Dgles2=false"

# "egl" requires "dri", "opengl"
PACKAGECONFIG[egl] = "-Degl=true, -Degl=false"

# Drivers

PACKAGECONFIG[etnaviv] = ""
PACKAGECONFIG[freedreno] = ""
PACKAGECONFIG[i915] = ""
PACKAGECONFIG[iris] = ""
PACKAGECONFIG[kmsro] = ""
PACKAGECONFIG[lima] = ""
PACKAGECONFIG[nouveau] = ""
PACKAGECONFIG[panfrost] = ""
PACKAGECONFIG[r300] = ""
PACKAGECONFIG[r600] = ""
PACKAGECONFIG[radeonsi] = ""
PACKAGECONFIG[v3d] = ""
PACKAGECONFIG[vc4] = ""

GALLIUMDRIVERS = ""
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'etnaviv', ',etnaviv', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'freedreno', ',freedreno', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'kmsro', ',kmsro', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'lima', ',lima', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'nouveau', ',nouveau', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'panfrost', ',panfrost', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'r300', ',r300', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'r600', ',r600', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'radeonsi', ',radeonsi', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'v3d', ',v3d', '', d)}"
GALLIUMDRIVERS_append = "${@bb.utils.contains('PACKAGECONFIG', 'vc4', ',vc4', '', d)}"
GALLIUMDRIVERS_append = ",virgl"
GALLIUMDRIVERS_append_intelgpu = "${@bb.utils.contains('PACKAGECONFIG', 'i915', ',i915', '', d)}"
GALLIUMDRIVERS_append_intelgpu = "${@bb.utils.contains('PACKAGECONFIG', 'iris', ',iris', '', d)}"

PACKAGECONFIG[gallium] = "-Dgallium-drivers=${@strip_comma('${GALLIUMDRIVERS}')}, -Dgallium-drivers=''"
PACKAGECONFIG[gallium-llvm] = "-Dllvm=true -Dshared-llvm=true, -Dllvm=false, llvm${MESA_LLVM_RELEASE} llvm-native"
PACKAGECONFIG[xa]  = "-Dgallium-xa=true, -Dgallium-xa=false"
PACKAGECONFIG[va] = "-Dgallium-va=true,-Dgallium-va=false,libva-initial"

PACKAGECONFIG[vdpau] = "-Dgallium-vdpau=true,-Dgallium-vdpau=false,libvdpau"

OSMESA = "${@bb.utils.contains('PACKAGECONFIG', 'gallium', 'gallium', 'classic', d)}"
PACKAGECONFIG[osmesa] = "-Dosmesa=${OSMESA},-Dosmesa=false"

PACKAGECONFIG[unwind] = "-Dlibunwind=true,-Dlibunwind=false,libunwind"

# llvmpipe is slow if compiled with -fomit-frame-pointer (e.g. -O2)
FULL_OPTIMIZATION_append = " -fno-omit-frame-pointer"

CFLAGS_append_armv5 = " -DMISSING_64BIT_ATOMICS"

# Remove the mesa dependency on mesa-dev, as mesa is empty
RDEPENDS_${PN}-dev = ""

# Add dependency so that GLES3 header don't need to be added manually
RDEPENDS_libgles2-mesa-dev += "libgles3-mesa-dev"

PACKAGES =+ "libegl-mesa libegl-mesa-dev \
             libosmesa libosmesa-dev \
             libgl-mesa libgl-mesa-dev \
             libglapi libglapi-dev \
             libgbm libgbm-dev \
             libgles1-mesa libgles1-mesa-dev \
             libgles2-mesa libgles2-mesa-dev \
             libgles3-mesa libgles3-mesa-dev \
             libxatracker libxatracker-dev \
             mesa-megadriver mesa-vulkan-drivers \
             mesa-vdpau-drivers \
            "

do_install_append () {
    # Drivers never need libtool .la files
    rm -f ${D}${libdir}/dri/*.la
    rm -f ${D}${libdir}/egl/*.la
    rm -f ${D}${libdir}/gallium-pipe/*.la
    rm -f ${D}${libdir}/gbm/*.la

    # it was packaged in libdricore9.1.3-1 and preventing upgrades when debian.bbclass was used 
    chrpath --delete ${D}${libdir}/dri/*_dri.so || true

    # libwayland-egl has been moved to wayland 1.15+
    rm -f ${D}${libdir}/libwayland-egl*
    rm -f ${D}${libdir}/pkgconfig/wayland-egl.pc
}

# For the packages that make up the OpenGL interfaces, inject variables so that
# they don't get Debian-renamed (which would remove the -mesa suffix), and
# RPROVIDEs/RCONFLICTs on the generic libgl name.
python __anonymous() {
    pkgconfig = (d.getVar('PACKAGECONFIG') or "").split()
    for p in (("egl", "libegl", "libegl1"),
              ("dri", "libgl", "libgl1"),
              ("gles", "libgles1", "libglesv1-cm1"),
              ("gles", "libgles2", "libglesv2-2"),
              ("gles", "libgles3",)):
        if not p[0] in pkgconfig:
            continue
        fullp = p[1] + "-mesa"
        pkgs = " ".join(p[1:])
        d.setVar("DEBIAN_NOAUTONAME_" + fullp, "1")
        d.appendVar("RREPLACES_" + fullp, pkgs)
        d.appendVar("RPROVIDES_" + fullp, pkgs)
        d.appendVar("RCONFLICTS_" + fullp, pkgs)

        d.appendVar("RRECOMMENDS_" + fullp, " mesa-megadriver")

        # For -dev, the first element is both the Debian and original name
        fullp += "-dev"
        pkgs = p[1] + "-dev"
        d.setVar("DEBIAN_NOAUTONAME_" + fullp, "1")
        d.appendVar("RREPLACES_" + fullp, pkgs)
        d.appendVar("RPROVIDES_" + fullp, pkgs)
        d.appendVar("RCONFLICTS_" + fullp, pkgs)
}

python mesa_populate_packages() {
    pkgs = ['mesa', 'mesa-dev', 'mesa-dbg']
    for pkg in pkgs:
        d.setVar("RPROVIDES_%s" % pkg, pkg.replace("mesa", "mesa-dri", 1))
        d.setVar("RCONFLICTS_%s" % pkg, pkg.replace("mesa", "mesa-dri", 1))
        d.setVar("RREPLACES_%s" % pkg, pkg.replace("mesa", "mesa-dri", 1))

    import re
    dri_drivers_root = oe.path.join(d.getVar('PKGD'), d.getVar('libdir'), "dri")
    if os.path.isdir(dri_drivers_root):
        dri_pkgs = os.listdir(dri_drivers_root)
        lib_name = d.expand("${MLPREFIX}mesa-megadriver")
        for p in dri_pkgs:
            m = re.match(r'^(.*)_dri\.so$', p)
            if m:
                pkg_name = " ${MLPREFIX}mesa-driver-%s" % legitimize_package_name(m.group(1))
                d.appendVar("RPROVIDES_%s" % lib_name, pkg_name)
                d.appendVar("RCONFLICTS_%s" % lib_name, pkg_name)
                d.appendVar("RREPLACES_%s" % lib_name, pkg_name)

    pipe_drivers_root = os.path.join(d.getVar('libdir'), "gallium-pipe")
    do_split_packages(d, pipe_drivers_root, r'^pipe_(.*)\.so$', 'mesa-driver-pipe-%s', 'Mesa %s pipe driver', extra_depends='')
}

PACKAGESPLITFUNCS_prepend = "mesa_populate_packages "

PACKAGES_DYNAMIC += "^mesa-driver-.*"

FILES_mesa-megadriver = "${libdir}/dri/* ${datadir}/drirc.d/00-mesa-defaults.conf"
FILES_mesa-vulkan-drivers = "${libdir}/libvulkan_*.so ${datadir}/vulkan"
FILES_${PN}-vdpau-drivers = "${libdir}/vdpau/*.so.*"
FILES_libegl-mesa = "${libdir}/libEGL.so.*"
FILES_libgbm = "${libdir}/libgbm.so.*"
FILES_libgles1-mesa = "${libdir}/libGLESv1*.so.*"
FILES_libgles2-mesa = "${libdir}/libGLESv2.so.*"
FILES_libgl-mesa = "${libdir}/libGL.so.*"
FILES_libglapi = "${libdir}/libglapi.so.*"
FILES_libosmesa = "${libdir}/libOSMesa.so.*"
FILES_libxatracker = "${libdir}/libxatracker.so.*"

FILES_${PN}-dev = "${libdir}/pkgconfig/dri.pc ${includedir}/vulkan ${libdir}/vdpau/*.so"
FILES_libegl-mesa-dev = "${libdir}/libEGL.* ${includedir}/EGL ${includedir}/KHR ${libdir}/pkgconfig/egl.pc"
FILES_libgbm-dev = "${libdir}/libgbm.* ${libdir}/pkgconfig/gbm.pc ${includedir}/gbm.h"
FILES_libgl-mesa-dev = "${libdir}/libGL.* ${includedir}/GL ${libdir}/pkgconfig/gl.pc"
FILES_libglapi-dev = "${libdir}/libglapi.*"
FILES_libgles1-mesa-dev = "${libdir}/libGLESv1*.* ${includedir}/GLES ${libdir}/pkgconfig/glesv1*.pc"
FILES_libgles2-mesa-dev = "${libdir}/libGLESv2.* ${includedir}/GLES2 ${libdir}/pkgconfig/glesv2.pc"
FILES_libgles3-mesa-dev = "${includedir}/GLES3"
FILES_libosmesa-dev = "${libdir}/libOSMesa.* ${includedir}/GL/osmesa.h ${libdir}/pkgconfig/osmesa.pc"
FILES_libxatracker-dev = "${libdir}/libxatracker.so ${libdir}/libxatracker.la \
                          ${includedir}/xa_tracker.h ${includedir}/xa_composite.h ${includedir}/xa_context.h \
                          ${libdir}/pkgconfig/xatracker.pc"

# Fix upgrade path from mesa to mesa-megadriver
RREPLACES_mesa-megadriver = "mesa"
RCONFLICTS_mesa-megadriver = "mesa"
RPROVIDES_mesa-megadriver = "mesa"
