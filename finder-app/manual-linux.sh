#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

# check input parameters
if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

# create OUTDIR if it doesn't exist
mkdir -p ${OUTDIR}

# clone sources to OUTDIR
echo "Check my current directory:"
pwd
pushd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi

# check out and build only if Image directory does not exist
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here    
    # deep clean
    echo "Doing deep clean..."
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper

    # defconfig, configure for our "virt" arm def board we will simulate in QEMU
    echo "Doing defconfig..."
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig

    # make
    echo "Building kernel..."
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all

    # modules and devicetree
    echo "Building modules and devicetree..."
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
    echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Create necessary base directories
mkdir -p rootfs
cd rootfs
mkdir -p bin dev etc home home/conf lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    # for some reason, this didn't work from within this script, but worked fine when entering command on command line
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
    # TODO:  Configure busybox
else
    cd busybox
fi

# make and install
echo "Building busybox ..."
make distclean
make defconfig
make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-
make CONFIG_PREFIX="${OUTDIR}/rootfs" ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- install

# Add library dependencies to rootfs
cd "$OUTDIR/rootfs"
echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Prepare to copy, find the location of the cross compiler
pathOfExecutable=$(type -P "${CROSS_COMPILE}readelf")
dirOfExecutable=$(dirname $pathOfExecutable)
CROSS_ROOT=$dirOfExecutable/../
echo "CROSS_ROOT was found to be " ${CROSS_ROOT}

# Copy libs
echo "Adding library dependencies ..."
#CROSS_ROOT=/home/georg/Programs/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-linux-gnu
#cp ${CROSS_ROOT}/aarch64-none-linux-gnu/libc/usr/lib64/libm.so ./lib64
#cp ${CROSS_ROOT}/aarch64-none-linux-gnu/libc/usr/lib64/libc.so ./lib64
cp ${CROSS_ROOT}/aarch64-none-linux-gnu/libc/lib64/libm.so.6 ./lib64
cp ${CROSS_ROOT}/aarch64-none-linux-gnu/libc/lib64/libc.so.6 ./lib64
cp ${CROSS_ROOT}/aarch64-none-linux-gnu/libc/usr/lib64/libresolv.so ./lib64
cp ${CROSS_ROOT}/aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 ./lib64
cp ./lib64/ld-linux-aarch64.so.1 ./lib64/ld-linux-aarch64.so
cp ./lib64/libm.so.6 ./lib64/libm.so
cp ./lib64/libc.so.6 ./lib64/libc.so
cp ./lib64/libresolv.so ./lib64/libresolv.so.2
chmod 777 ./lib64/*
cp ./lib64/* ./lib

# Make device nodes
echo "Adding device nodes ..."
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# Clean and build the writer utility
echo "Cross compiling and installing writer ..."

# popd will return me to where the script was started...???
#WORKING_DIR=/home/georg/Dev/training/assignment-1-FreeSpirit9009/finder-app
#cd ${WORKING_DIR}
popd
echo "Check my current directory (2):"
pwd
#pushd ./finder-app
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu-
file writer

# Copy the finder related scripts and executables to the /home directory (TODO: what "related scripts"?)
# on the target rootfs - Copy your finder.sh, conf/username.txt, conf/assignment.txt and finder-test.sh
cp writer "$OUTDIR/rootfs/home"
cp finder.sh "$OUTDIR/rootfs/home"
cp finder-test.sh "$OUTDIR/rootfs/home"
cp conf/username.txt "$OUTDIR/rootfs/home/conf"
cp conf/assignment.txt "$OUTDIR/rootfs/home/conf"
cp autorun-qemu.sh "$OUTDIR/rootfs/home"

# Chown the root directory
echo "Preparing root fs ..."
cd "$OUTDIR"
sudo chown -R root:root rootfs

# Create initramfs.cpio.gz
cd "$OUTDIR/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "$OUTDIR"
gzip -f initramfs.cpio

# return to original directory? (TODO)

