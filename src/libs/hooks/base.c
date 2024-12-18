/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include "common/logger.h"
#include "libs/hooks.h"

LIST_HEAD(hook_info_list);
DEFINE_SPINLOCK(hook_info_lock);

struct hook_info* nornir_find_hook_info(void *target)
{
    struct hook_info *info;

    list_for_each_entry(info, &hook_info_list, list) {
        if (info->target == target) {
            return info;
        }
    }

    return NULL;
}

struct hook_info* nornir_install_hook_info(void *target, void *data)
{
    struct hook_info *info;
    void *err_ptr;

    spin_lock(&hook_info_lock);

    info = nornir_find_hook_info(target);
    if (info) {
        logger_error("Duplicate hook point: %p, operation aborted.\n", target);
        err_ptr = ERR_PTR(-ECANCELED);
        goto err_hooked;
    }

    info = kmalloc(sizeof(*info), GFP_KERNEL);
    if (!info) {
        logger_error("Unable to allocate memory for hook_info.\n");
        err_ptr = ERR_PTR(-ENOMEM);
        goto err_nomem;
    }

    info->target = target;
    info->data = data;
    list_add(&info->list, &hook_info_list);
    spin_unlock(&hook_info_lock);

    return info;

err_nomem:
err_hooked:
    spin_unlock(&hook_info_lock);
    return err_ptr;
}

int nornir_remove_hook_info(void *target)
{
    struct hook_info *info;

    spin_lock(&hook_info_lock);

    info = nornir_find_hook_info(target);
    if (!info) {
        logger_error("Unable to unhook %p that were not hooked yet.\n", target);
        spin_unlock(&hook_info_lock);
        return -ECANCELED;
    }

    list_del(&info->list);
    kfree(info);
    spin_unlock(&hook_info_lock);

    return 0;
}
