#!/bin/bash

#
# tapiOS build script
# ---
# This script should set everything up for compiling
# both the  tapiOS kernel and userspace programs for it.
#

JOBS=5

DOWNLOAD_BASE="ftp://ftp.funet.fi/pub/gnu/prep"
GCC_VERSION="gcc-5.2.0"
GCC_DOWNLOAD_BASE="$DOWNLOAD_BASE/gcc"
BINUTILS_DOWNLOAD_BASE="$DOWNLOAD_BASE/binutils"
BINUTILS_VERSION="binutils-2.25"
AUTOCONF_DOWNLOAD_BASE="$DOWNLOAD_BASE/autoconf"
AUTOCONF_VERSION="autoconf-2.64"
AUTOMAKE_DOWNLOAD_BASE="$DOWNLOAD_BASE/automake"
AUTOMAKE_VERSION="automake-1.11.1"

function debug_echo ()
{
	echo "[DEBUG]" $@ 1>&2
}

function util_exists ()
{
	which "$1" &>/dev/null
	if [[ $? -eq 0 ]]; then
		return 0
	else
		return -1
	fi
}

function download_if_does_not_exist ()
{
	if [[ ! -e $2 ]]; then
		wget $1 -O $2
	fi
}

function check_missing_utils ()
{
	local ret=""
	local needed_utils="i586-tapios-gcc grub-mkrescue gpg"
	for util in $needed_utils; do
		util_exists $util
		if [[ ! $? -eq 0 ]]; then
			ret+=$util" "
		fi
	done
	echo $ret
}

function verify_integrity ()
{
	gpg --verify $1.sig $1 &> /dev/null
}

function prompt_yes_no ()
{
	while true; do
		read -p "$1 [Y/n] " RETRY
		case $RETRY in
			y|Y|"")
				return 0
				;;
			n|N)
				return -1
				;;
			*)
				echo "Please answer y or n"
				;;
		esac
	done
}

# $1 = XXX_VERSION
# $2 = XXX_DOWNLOAD_BASE
# $3 = GPG_KEY
# $4 = Name of package
function download_and_verify ()
{
	while true; do
		if [[ -d build-tmp/$1 ]]; then
			echo "$4 sources found, using them instead of downloading"
			break
		fi
		local TARBALL=build-tmp/$1.tar.bz2
		download_if_does_not_exist $2/$1.tar.bz2 $TARBALL
		download_if_does_not_exist $2/$1.tar.bz2.sig $TARBALL.sig
		gpg --recv-keys $3 &> /dev/null
		echo "Verifying integrity of $4"
		verify_integrity $TARBALL
		if [[ ! $? -eq 0 ]]; then
			prompt_yes_no "Integrity check of $4 failed, retry download?"
			if [[ $? -eq 0 ]]; then
				rm $TARBALL
			else
				exit 1
			fi
		else
			echo "Extracting $4"
			tar -C build-tmp/ -xf $TARBALL
			break
		fi
	done
}

function download_and_verify_gcc ()
{
	download_and_verify $GCC_VERSION $GCC_DOWNLOAD_BASE "FC26A641" "GCC"
}

function download_and_verify_binutils ()
{
	download_and_verify $BINUTILS_VERSION $BINUTILS_DOWNLOAD_BASE "4AE55E93" "binutils"
}

function download_and_verify_autotools ()
{
	download_and_verify $AUTOCONF_VERSION $AUTOCONF_DOWNLOAD_BASE "F4850180" "autoconf"
	download_and_verify $AUTOMAKE_VERSION $AUTOMAKE_DOWNLOAD_BASE "5D0CDCFC" "automake"
}
function build_autotools ()
{
	if [[ -d build-tmp/autotools ]]; then
		echo "autotools already built, skipping"
	else
		if [[ ! -d build-tmp/autoconf-2.64 ]]; then
			echo "Fatal error: autoconf directory not found, exiting"
			exit 2
		fi
		if [[ ! -d build-tmp/automake-1.11.1 ]]; then
			echo "Fatal error: automake directory not found, exiting"
			exit 2
		fi
		pushd . &>/dev/null
		cd build-tmp/autoconf-2.64
		./configure --prefix=$(pwd)/../autotools
		make
		make install
		popd
		pushd . &>/dev/null
		cd build-tmp/automake-1.11.1
		./configure --prefix=$(pwd)/../autotools
		make
		make install
		popd
	fi
}

function patch_binutils ()
{
	if [[ -f build-tmp/binutils-2.25/tapios_binutils_patched ]]; then
		echo "binutils already patched, skipping"
	else
		pushd . &>/dev/null
		cd build-tmp/binutils-2.25/
		find ../../patches -name '*.patch' -print0 | xargs -0 -I{} patch -p1 -i {}
		cp ../../patches/elf_i386_tapios.sh ld/emulparams
		pushd . &>/dev/null
		cd ld
		PATH=../../autotools/bin:$PATH automake
		popd
		touch tapios_binutils_patched
		popd
	fi
}

function build_binutils ()
{
	if [[ -f cross/bin/i586-pc-tapios-ar ]]; then
		echo "binutils already built, skipping"
	else
		if [[ ! -d build-tmp/binutils-build ]]; then
			mkdir build-tmp/binutils-build
			if [[ ! $? -eq 0 ]]; then
				echo "Could not create binutils build folder, exiting"
				exit 3
			fi
		fi
		pushd . &>/dev/null
		cd build-tmp/binutils-build
		../binutils-2.25/configure --target=i586-pc-tapios --prefix="$(pwd)/../../cross" --with-sysroot --disable-nls --disable-werror
		make -j$JOBS
		make install
		popd
	fi
}

function print_missing_utils ()
{
	echo "Utils missing:"
	for util in $@; do
		echo "* $util"
		case $util in
			i586-tapios-gcc)
				echo "Building GCC (using version $GCC_VERSION)"
				if [[ ! -d build-tmp ]]; then
					mkdir build-tmp
				fi
				download_and_verify_gcc
				echo "Building binutils (using version $BINUTILS_VERSION)"
				download_and_verify_binutils
				echo "Downloading correct versions of autotools and automake ($AUTOCONF_VERSION, $AUTOMAKE_VERSION)"
				download_and_verify_autotools
				build_autotools
				patch_binutils
				build_binutils
				;;
			grub-mkrescue)
				echo "tapiOS needs grub-mkrescue to create an .iso file."
				exit 0
				;;
			gpg)
				echo "tapiOS needs gpg to verify integrity of downloaded GCC"
				exit 0
				;;
			*)
				debug_echo "This is a bug :("
				;;
		esac
	done
}

missing_utils=$(check_missing_utils)
if [[ ! -z $missing_utils ]]; then
	print_missing_utils $missing_utils
fi
