#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

# DO NOT USE, handled below
# # OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-
SYSROOT=/home/jqiao/Development/arm-cross-compiler/gcc-arm-10.2-2020.11-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc

OUTDIR=$1
if [ -z "${OUTDIR}" ]; then
    OUTDIR=/tmp/aeld
    echo "No outdir specified, using ${OUTDIR}"
fi
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

 cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    # Configure the kernel for the specified architecture
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    # Build the kernel image
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} Image

    # Build the device tree blob (DTB) file
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
    #make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs_check

    # Build the kernel modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules

    # Install the kernel modules to a temporary directory
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} INSTALL_MOD_PATH=${OUTDIR}/modules modules_install

    # Copy the kernel image and DTB file to the output directory
    cp arch/${ARCH}/boot/Image ${OUTDIR}/Image
    # No need for dtb files for assignment 3
    # cp arch/${ARCH}/boot/dts/*.dtb ${OUTDIR}/

fi

# echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# # TODO: Create necessary base directories
 mkdir -p ${OUTDIR}/rootfs/{bin,dev,etc,home,lib,lib64,proc,sbin,sys,tmp,usr,var,usr/bin,usr/lib,usr/sbin,var/log,home/conf}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
    make distclean
    make defconfig
else
    cd busybox
fi

# TODO: Make and install busybox
make -j 4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} V=1
make ARCH=${ARCH} CONFIG_PREFIX=${OUTDIR}/rootfs CROSS_COMPILE=${CROSS_COMPILE} install

# echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

# #DO NOT USE FOR NOW
# #export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
# #echo "SYSROOT is ${SYSROOT}"

# TODO: Add library dependencies to rootfs
# #DO NOT USE FOR NOW
#${CROSS_COMPILE}ldd ${OUTDIR}/rootfs/bin/busybox | grep "=>" | awk '{print $3}' | xargs -I '{}' cp -v '{}' ${OUTDIR}/rootfs/lib
sudo cp $SYSROOT/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib
sudo cp $SYSROOT/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64
sudo cp $SYSROOT/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64
sudo cp $SYSROOT/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64

# # TODO: Make device nodes
 sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
 sudo mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1
 sudo mknod -m 666 ${OUTDIR}/rootfs/dev/ram0 b 1 0

# TODO: Clean and build the writer utility
cd ${FINDER_APP_DIR}
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/finder.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/autorun-qemu.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/finder-test.sh ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/writer ${OUTDIR}/rootfs/home
cp ${FINDER_APP_DIR}/conf/assignment.txt ${OUTDIR}/rootfs/home/conf
cp ${FINDER_APP_DIR}/conf/username.txt ${OUTDIR}/rootfs/home/conf

# TODO: Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root | gzip > ${OUTDIR}/initramfs.cpio.gz
