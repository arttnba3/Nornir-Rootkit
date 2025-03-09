#!/usr/bin/env bash
#
# SPDX-License-Identifier: GPL-2.0
# Copyright (c) 2024 arttnba3 <arttnba@gmail.com>

export KCONF_BIN_PATH=$(which kconfig-conf 2>/dev/null)
export KMCONF_BIN_PATH=$(which kconfig-mconf 2>/dev/null)

export NORNIR_SCRIPTS_DIR=$(pwd)/scripts
export NORNIR_SRC_DIR=$(pwd)/src
export NORNIR_KCONFIG_PATH=$NORNIR_SRC_DIR/Kconfig

export PREREQUISITE_CHECKER=check_prerequisite.sh

function err_report() 
{
    echo "[x] Error occurred at line $1. Exiting..."
}

function check_src()
{
    if [[ ! -f $NORNIR_KCONFIG_PATH ]]; then
        echo '[x] Please specific the correct path of the project, and make sure the `src/Kconfig` was not lost.'
        exit 1
    fi
}

function install_kconfig_pkg_apt()
{
    trap 'err_report $LINENO; exit 1;' ERR
    sudo apt-get update
    sudo apt-get install kconfig-frontends
}

function get_kconfig_pkg_src()
{
    trap 'err_report $LINENO; exit 1;' ERR

    TEMP_DIR=""
    KCONFIG_SRC_DIR=""
    KCONFIG_BUILD_DIR=""

    while true; do
        read -p "Cloning source code to current directory or into /tmp ? [1/2/c] " choice
        case $choice in 
            1)
                TEMP_DIR=$(pwd)/temp
                mkdir -p $TEMP_DIR
                break
                ;;
            2)
                TEMP_DIR=$(mktemp -d)
                break
                ;;
            C|c)
                echo "Operation canceled. See you next time..."
                exit 0
                ;;
            "")
                ;;
            *)
                echo "[x] Invalid choice"
                ;;
        esac
    done

    KCONFIG_SRC_DIR=$TEMP_DIR/kconfig-frontends
    KCONFIG_BUILD_DIR=$TEMP_DIR/build
    if [[ ! -d $KCONFIG_SRC_DIR ]]; then
        echo "[*] Cloning kconfig-frontends source code into directory: $KCONFIG_SRC_DIR..."
        git clone https://salsa.debian.org/philou/kconfig-frontends.git $KCONFIG_SRC_DIR
        # use a specific commit we known instead of latest to keep the stability and usability
        cd $KCONFIG_SRC_DIR
        git checkout e8d38cb0

        # some bugs need to be fixed manually, as this package is old and unmaintained :( and we're searching for alternatives soon...
        sed -i 's/kconf_id_lookup (register const char \*str, register unsigned int len)/kconf_id_lookup (register const char \*str, register GPERF_LEN_TYPE len)/g' $KCONFIG_SRC_DIR/libs/parser/hconf.c

        # autoreconf is needed, as the version of your automake toolchains may diverse
        cd $KCONFIG_SRC_DIR
        autoreconf -f -i
    else
        echo "[*] Using existed kconfig-frontends source code directory: $KCONFIG_SRC_DIR"
    fi

    if [[ ! -d $KCONFIG_BUILD_DIR || ! -f $KCONFIG_BUILD_DIR/frontends/conf/kconfig-conf || ! -f $KCONFIG_BUILD_DIR/frontends/mconf/kconfig-mconf ]]; then
        if [[ -d $KCONFIG_BUILD_DIR ]]; then
            rm -r $KCONFIG_BUILD_DIR
        fi
        mkdir -p $KCONFIG_BUILD_DIR
        cd $KCONFIG_BUILD_DIR
        $KCONFIG_SRC_DIR/configure --enable-mconf
        make
    fi

    echo "[*] Using kconfig-frontends binaries in directory: $KCONFIG_BUILD_DIR"

    while true; do
        read -p "Just use kconfig-frontends binaries directly, or install them into your system(not recommended)? [1/2/c]" choice
        case $choice in 
            1)
                KCONF_BIN_PATH=$KCONFIG_BUILD_DIR/frontends/conf/kconfig-conf
                KMCONF_BIN_PATH=$KCONFIG_BUILD_DIR/frontends/mconf/kconfig-mconf
                break
                ;;
            2)
                cd $KCONFIG_BUILD_DIR
                sudo make install
                KCONF_BIN_PATH=$(which kconfig-conf 2>/dev/null)
                KMCONF_BIN_PATH=$(which kconfig-mconf 2>/dev/null)
                break
                ;;
            C|c)
                echo "Operation canceled. See you next time..."
                exit 0
                ;;
            "")
                ;;
            *)
                echo "[x] Invalid choice"
                ;;
        esac
    done
}

function get_kconfig_pkg_distro()
{
    distro=$(grep "^ID=" /etc/os-release | cut -d'=' -f2 | sed -e 's/"//g')

    case $distro in
        "ubuntu"|"debian")
            install_kconfig_pkg_apt
            ;;
        *)
            echo "[!] Distro \"${distro}\" is not officially supportted."
            echo "[*] Trying to search known package managers for the installation."

            if command -v apt-get; then
                install_kconfig_pkg_apt
            else
                echo "[x] No known package managers were found."
                read -p "Do you wish to install from source? [Y/n]" choice
                case $choice in 
                    Y|y|"")
                        get_kconfig_pkg_src
                        ;;
                    N|n)
                        echo "See you next time..."
                        exit 0
                        ;;
                    *)
                        echo "[x] Invalid choice"
                        exit 1
                        ;;
                esac
            fi
    esac
}

function get_kconfig_pkg()
{
    while true; do
        read -p "Install kconfig-frontends from your distro's package repository or compile it from source? [1/2/c] " choice
        case $choice in 
            1)
                get_kconfig_pkg_distro
                break
                ;;
            2)
                get_kconfig_pkg_src
                break
                ;;
            C|c)
                echo "Operation canceled. See you next time..."
                exit 0
                ;;
            "")
                ;;
            *)
                echo "[x] Invalid choice"
                ;;
        esac
    done
}

function check_kconfig()
{
    echo "[*] Checking kconfig-frontends..."

    if [[ ! -n $KCONF_BIN_PATH || ! -n $KMCONF_BIN_PATH ]]; then
        read -p "No pre-installed kconfig-frontends found, do you wish to install? [Y/n]: " choice

        case $choice in
            Y|y|"")
                get_kconfig_pkg
                ;;
            N|n)
                echo "See you next time..."
                exit 0
                ;;
            *)
                echo "[x] Invalid choice"
                exit 1
                ;;
        esac
    fi
}

function setup_config()
{
    case "$1" in
        menuconfig)
            echo "[*] Using kmconf binary: $KMCONF_BIN_PATH"
            $KMCONF_BIN_PATH $NORNIR_KCONFIG_PATH
            ;;
        config)
            echo "[*] Using kconf binary: $KCONF_BIN_PATH"
            $KCONF_BIN_PATH $NORNIR_KCONFIG_PATH
            ;;
        *)
            echo "[x] Unknown param $1"
            exit 1
            ;;
    esac
}

function main()
{
    check_src
    $NORNIR_SCRIPTS_DIR/$PREREQUISITE_CHECKER || exit 1
    check_kconfig
    setup_config $@
}

main $@
