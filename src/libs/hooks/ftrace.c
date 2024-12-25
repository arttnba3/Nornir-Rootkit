/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/ftrace.h>
#include <linux/slab.h>
#include <linux/version.h>
#include "common/logger.h"
#include "libs/hooks.h"

static __maybe_unused struct ftrace_ops*
nornir_install_ftrace_hook_internal(void *target, ftrace_func_t new_dst)
{
    struct ftrace_ops *hook_ops;
    int err;
    
    hook_ops = kmalloc(GFP_KERNEL, sizeof(*hook_ops));
    if (!hook_ops) {
        err = -ENOMEM;
        logger_error("Unable to allocate memory for new ftrace_ops.\n");
        goto no_mem;
    }
    memset(hook_ops, 0, sizeof(*hook_ops));
    hook_ops->func = new_dst;
    hook_ops->flags = FTRACE_OPS_FL_SAVE_REGS
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 11, 0)
                    | FTRACE_OPS_FL_RECURSION
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
                    | FTRACE_OPS_FL_RECURSION_SAFE
#else
    #error "Kernel version too old, not supported yet."
#endif
                    | FTRACE_OPS_FL_IPMODIFY;

    err = ftrace_set_filter_ip(hook_ops, (unsigned long) target, 0, 0);
    if (err) {
        logger_error(
            "Failed to set ftrace filter for target addr: %p.\n",
            target
        );
        goto failed;
    }

    err = register_ftrace_function(hook_ops);
    if (err) {
        logger_error(
            "Failed to register ftrace fn for target addr: %p.\n",
            target
        );
        goto failed;
    }

    logger_info(
        "Install ftrace hook at %p, new destination: %p.\n",
        target,
        new_dst
    );

    return hook_ops;

failed:
    kfree(hook_ops);
no_mem:
    return ERR_PTR(err);
}

static __maybe_unused int
nornir_uninstall_ftrace_hook_internal(struct ftrace_ops*hook_ops, void*hook_dst)
{
    int err;

    err = unregister_ftrace_function(hook_ops);
    if (err) {
        logger_error("failed to unregister ftrace.");
        goto out;
    }

    err = ftrace_set_filter_ip(hook_ops, (unsigned long) hook_dst, 1, 0);
    if (err) {
        logger_error("failed to rmove ftrace point.");
        goto out;
    }

out:
    return err;
}

int nornir_install_ftrace_hook(void *target, void *new_dst)
{
    struct ftrace_ops* hook_ops;
    struct hook_info *info;
    int err;

    if (nornir_find_hook_info(target)) {
        err = -ECANCELED;
        logger_error("Duplicate hook point: %lx, operation aborted.\n", target);
        goto err_hooked;
    }

    hook_ops = nornir_install_ftrace_hook_internal(target, new_dst);
    if (IS_ERR(hook_ops)) {
        err = PTR_ERR(hook_ops);
        logger_error("Unable to create new ftrace hook, error code: %d.\n",err);
        goto err_ftrace;
    }

    info = nornir_install_hook_info(target, hook_ops);
    if (IS_ERR(info)) {
        err = PTR_ERR(info);
        logger_error("Unable to install new hook_info, error code: %d.\n",err);
        goto err_info;
    }

    return 0;

err_info:
    nornir_remove_hook_info(target);
    nornir_uninstall_ftrace_hook_internal(hook_ops, new_dst);
    kfree(hook_ops);
err_ftrace:
err_hooked:
    return err;
}

int nornir_remove_ftrace_hook(void *target)
{
    struct ftrace_ops* hook_ops;
    struct hook_info *info;
    int err;

    info = nornir_find_hook_info(target);
    if (!info) {
        logger_error(
            "Unable to find hooked point: %p, oeration aborted.\n",
            target
        );
        err = -ECANCELED;
        goto err_not_found;
    }

    hook_ops = info->data;
    err = nornir_uninstall_ftrace_hook_internal(hook_ops, target);
    if (err) {
        logger_error("Unable to remove ftrace hook, error code: %d.\n", err);
        goto err_uninstall_ftrace;
    }

    kfree(hook_ops);
    nornir_remove_hook_info(target);

    return 0;

err_uninstall_ftrace:
err_not_found:
    return err;
}
