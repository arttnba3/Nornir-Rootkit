#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2025 arttnba3 <arttnba@gmail.com>

export KCONF_BIN_PATH=$(which kconfig-conf 2>/dev/null)
export KMCONF_BIN_PATH=$(which kconfig-mconf 2>/dev/null)

export NORNIR_SRC_DIR=$(pwd)/src
export NORNIR_KCONFIG_PATH=$NORNIR_SRC_DIR/Kconfig

function err_report() 
{
    echo "[x] Error occurred at line $1. Exiting..."
}

declare -a needed_pkgs_apt=("git" "gcc" "make" "flex" "bison" "binutils" "autoconf" "automake" "autotools-dev" "dh-autoreconf" "gperf" "libtool" "pkg-config" "libncurses-dev" "linux-headers-$(uname -r)")
declare -a missing_pkgs_apt=()

function check_toolchain_apt_find_missing_pkgs()
{
    for pkg in "${needed_pkgs_apt[@]}"; do
        dpkg -s "$pkg" &>/dev/null
        pkg_query_res=$?
        if [[ $pkg_query_res -ne 0 ]]; then
            missing_pkgs_apt+=("$pkg")
        fi
    done

    for pkg in "${missing_pkgs_apt[@]}"; do
        echo "package missing: $pkg"
    done
}

function install_toolchain_apt_missing_pkgs()
{
    sudo apt-get update -y
    for pkg in "${missing_pkgs_apt[@]}"; do
        if ! sudo apt-get install -y "$pkg"; then
            echo "[x] Unable to install package \"$pkg\""
            exit 1
        fi
    done
}

function check_toolchain_apt()
{
    trap 'err_report $LINENO; exit 1;' ERR

    check_toolchain_apt_find_missing_pkgs

    if [ ${#missing_pkgs_apt[@]} -ne 0 ]; then
        echo "[!] Some toolchains are missing."
        read -p "Do you wish to install them now? [Y/n]" choice
        case $choice in 
            Y|y|"")
                ;;
            N|n)
                echo "See you next time..."
                exit 1
                ;;
            *)
                echo "[x] Invalid choice"
                exit 1
                ;;
        esac
        install_toolchain_apt_missing_pkgs
    fi
}

declare -a needed_pkgs_zypper=("git" "gcc" "make" "flex" "bison" "binutils" "autoconf" "automake" "gperf" "libtool" "pkg-config" "ncurses-devel" "kernel-devel")
declare -a missing_pkgs_zypper=()

function check_toolchain_zypper_find_missing_pkgs()
{
    for pkg in "${needed_pkgs_zypper[@]}"; do
        zypper search --installed-only --match-exact "$pkg" &>/dev/null
        pkg_query_res=$?
        if [[ $pkg_query_res -ne 0 ]]; then
            missing_pkgs_zypper+=("$pkg")
        fi
    done

    for pkg in "${missing_pkgs_zypper[@]}"; do
        echo "package missing: $pkg"
    done
}

function install_toolchain_zypper_missing_pkgs()
{
    sudo zypper ref

    case $1 in
        "opensuse-tumbleweed"|"opensuse-slowroll")
            sudo zypper dup -y --auto-agree-with-licenses
            ;;
        "opensuse-leap")
            sudo zypper up -y --auto-agree-with-licenses
            ;;
        *)
            echo "Unknown distro: $1"
            exit 1
            ;;
    esac

    for pkg in "${missing_pkgs_zypper[@]}"; do
        if ! sudo zypper install -y "$pkg"; then
            echo "[x] Unable to install package \"$pkg\""
            exit 1
        fi
    done
}

function check_toolchain_zypper()
{
    trap 'err_report $LINENO; exit 1;' ERR

    check_toolchain_zypper_find_missing_pkgs

    if [ ${#missing_pkgs_zypper[@]} -ne 0 ]; then
        echo "[!] Some toolchains are missing."
        read -p "Do you wish to install them now? [Y/n]" choice
        case $choice in 
            Y|y|"")
                ;;
            N|n)
                echo "See you next time..."
                exit 1
                ;;
            *)
                echo "[x] Invalid choice"
                exit 1
                ;;
        esac
        install_toolchain_zypper_missing_pkgs $@
    fi
}

declare -a needed_pkgs_emerge=("dev-vcs/git" "sys-devel/gcc" "dev-build/make" "sys-devel/flex" "sys-devel/bison" "sys-devel/binutils" "dev-build/autoconf" "dev-build/automake" "dev-util/gperf" "dev-build/libtool" "dev-util/pkgconf" "sys-libs/ncurses" "sys-kernel/gentoo-sources")
declare -a missing_pkgs_emerge=()

function check_toolchain_emerge_find_missing_pkgs()
{
    for pkg in "${needed_pkgs_emerge[@]}"; do
        qlist -I "$pkg" &>/dev/null
        pkg_query_res=$?
        if [[ $pkg_query_res -ne 0 ]]; then
            missing_pkgs_emerge+=("$pkg")
        fi
    done

    for pkg in "${missing_pkgs_emerge[@]}"; do
        echo "package missing: $pkg"
    done
}

function install_toolchain_emerge_missing_pkgs()
{
    trap 'err_report $LINENO; exit 1;' ERR

    sudo emerge --sync

    for pkg in "${missing_pkgs_emerge[@]}"; do
        if ! sudo emerge --ask --verbose --yes "$pkg"; then
            echo "[x] Unable to install package \"$pkg\""
            exit 1
        fi
    done
}

function check_toolchain_emerge()
{
    trap 'err_report $LINENO; exit 1;' ERR

    check_toolchain_emerge_find_missing_pkgs

    if [ ${#missing_pkgs_emerge[@]} -ne 0 ]; then
        echo "[!] Some toolchains are missing."
        read -p "Do you wish to install them now? [Y/n]" choice
        case $choice in 
            Y|y|"")
                ;;
            N|n)
                echo "See you next time..."
                exit 1
                ;;
            *)
                echo "[x] Invalid choice"
                exit 1
                ;;
        esac
        install_toolchain_emerge_missing_pkgs $@
    fi
}

function check_toolchain()
{
    echo "[*] Checking for necessary toolchains..."

    distro=$(grep "^ID=" /etc/os-release | cut -d'=' -f2 | sed -e 's/"//g')

    case $distro in
        "gentoo")
            check_toolchain_emerge
            ;;
        "opensuse-tumbleweed"|"opensuse-slowroll"|"opensuse-leap")
            check_toolchain_zypper $distro
            ;;
        "ubuntu"|"debian")
            check_toolchain_apt
            ;;
        *)
            echo "[!] Distro \"${distro}\" is not officially supportted."
            echo "[*] Trying to search known package managers for the checking."

            if command -v apt-get; then
                check_toolchain_apt
            elif command -v zypper; then
                check_toolchain_zypper
            else
                echo "[x] No known package managers were found."
                read -p "Do you wish to continue directly? [Y/n]" choice
                case $choice in 
                    Y|y|"")
                        exit 0
                        ;;
                    N|n)
                        echo "See you next time..."
                        exit 1
                        ;;
                    *)
                        echo "[x] Invalid choice"
                        exit 1
                        ;;
                esac
            fi
    esac
}

function main()
{
    check_toolchain
    exit 0
}

main
