/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include "common/logger.h"
#include "libs/hooks.h"
#include "libs/mem.h"

#define X86_JMP_PREFIX      0xE9
#define X86_JMP_DISTENCE    12
#define X86_NOP_INSN        0x90

static __always_inline
void nornir_raw_write_inline_hook(void *target, void *new_dst)
{
    size_t dst_off = (size_t) new_dst - (size_t) target;
    uint8_t asm_buf[0x100];

#ifdef CONFIG_X86_64
    memset(asm_buf, X86_NOP_INSN, sizeof(asm_buf));
    asm_buf[0] = X86_JMP_PREFIX;
    *(size_t*) &asm_buf[1] = dst_off - X86_JMP_DISTENCE;
    nornir_overwrite_romem(target, asm_buf, HOOK_BUF_SZ);
#else
    #error "No supported architecture were chosen for inline hook"
#endif
}

static __always_inline
void nornir_raw_write_orig_hook_buf_back(struct asm_hook_info *info)
{
#ifdef CONFIG_X86_64
    nornir_overwrite_romem(info->orig_func, info->orig_data, HOOK_BUF_SZ);
#else
    #error "No supported architecture were chosen for inline hook"
#endif
}

size_t nornir_asm_inline_hook_helper(struct asm_hook_info *info, size_t *args)
{
    size_t ret;

    if (info->hook_before) {
        ret=info->hook_before(args[0],args[1],args[2],args[3],args[4],args[5]);
    }

    if (info->exec_orig
        && info->exec_orig(args[0],args[1],args[2],args[3],args[4],args[5])) {
        nornir_raw_write_orig_hook_buf_back(info);
        ret = info->orig_func(args[0],args[1],args[2],args[3],args[4],args[5]);
        nornir_raw_write_inline_hook(info->orig_func, info->new_dst);
    }

    if (info->hook_after) {
        ret = info->hook_after(ret, args);
    }

    return ret;
}

int nornir_install_inline_asm_hook(struct asm_hook_info *info, void *new_dst)
{
    struct hook_info *new_info;
    int err;

    if (nornir_find_hook_info(info->orig_func)) {
        err = -ECANCELED;
        logger_error(
            "Duplicate hook point: %lx, operation aborted.\n",
            info->orig_func
        );
        goto err_hooked;
    }

    new_info = nornir_install_hook_info(info->orig_func, info);
    if (IS_ERR(new_info)) {
        err = PTR_ERR(new_info);
        logger_error("Unable to install new hook_info, error code: %d.\n", err);
        goto err_info;
    }

    memcpy(info->orig_data, info->orig_func, HOOK_BUF_SZ);
    nornir_raw_write_inline_hook(info->orig_func, new_dst);

    return 0;

err_info:
err_hooked:
    return err;
}

int nornir_remove_inline_asm_hook(struct asm_hook_info *asm_info)
{
    struct hook_info *hook_info;
    int err;

    hook_info = nornir_find_hook_info(asm_info->orig_func);
    if (!hook_info) {
        logger_error(
            "Unable to find hooked point: %lx, oeration aborted.\n",
            asm_info->orig_func
        );
        err = -ECANCELED;
        goto err_not_found;
    }

    nornir_raw_write_orig_hook_buf_back(asm_info);
    nornir_remove_hook_info(asm_info->orig_func);

    return 0;

err_not_found:
    return err;
}
