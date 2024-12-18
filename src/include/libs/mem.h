/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_LIBS_MEM_H
#define NORNIR_LIBS_MEM_H

#include <linux/kernel.h>
#include <linux/types.h>

#ifdef CONFIG_X86_64

static __always_inline u64 nornir_read_cr0(void)
{
    u64 cr0;

    asm volatile (
        "movq  %%cr0, %%rax;"
        "movq  %%rax, %0;   "
        : "=r" (cr0) :: "%rax"
    );

    return cr0;
}

static __always_inline void nornir_write_cr0(u64 cr0)
{
    asm volatile (
        "movq   %0, %%rax;  "
        "movq  %%rax, %%cr0;"
        :: "r" (cr0) : "%rax"
    );
}

static __always_inline void nornir_disable_write_protect(void)
{
    u64 cr0;

    cr0 = nornir_read_cr0();

    if ((cr0 >> 16) & 1) {
        cr0 &= ~(1 << 16);
        nornir_write_cr0(cr0);
    }
}

static __always_inline void nornir_enable_write_protect(void)
{
    size_t cr0;

    cr0 = nornir_read_cr0();

    if (!((cr0 >> 16) & 1)) {
        cr0 |= (1 << 16);
        nornir_write_cr0(cr0);
    }
}

#endif // CONFIG_X86_64

extern void nornir_overwrite_romem(void *dst, void *src, size_t len);

#endif // NORNIR_LIBS_MEM_H
