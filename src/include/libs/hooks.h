/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_LIBS_HOOK_H
#define NORNIR_LIBS_HOOK_H

#include <linux/list.h>
#include <linux/ftrace.h>
#include <linux/types.h>

struct hook_info {
    struct list_head list;
    void *target;   /* address to be hooked */
    void *data;     /* internal data used by different hook framework */
};

#define HOOK_BUF_SZ 0x10

typedef size_t (*hook_fn) (size_t, size_t, size_t, size_t, size_t, size_t);

struct asm_hook_info {
    uint8_t orig_data[HOOK_BUF_SZ];
    hook_fn hook_before;
    hook_fn exec_orig;
    hook_fn orig_func;
    hook_fn new_dst;
    size_t (*hook_after) (size_t orig_ret, size_t *args);
};

extern struct hook_info* nornir_find_hook_info(void *target);
extern struct hook_info* nornir_install_hook_info(void *target, void *data);
extern int nornir_remove_hook_info(void *target);

extern int nornir_install_ftrace_hook(void *target, void *new_dst);
extern int nornir_remove_ftrace_hook(void *target);

extern int
nornir_install_inline_asm_hook(struct asm_hook_info *info, void *new_dst);
extern int nornir_remove_inline_asm_hook(struct asm_hook_info *info);
extern size_t
nornir_asm_inline_hook_helper(struct asm_hook_info *info, size_t *args);

#endif // NORNIR_LIBS_HOOK_H
