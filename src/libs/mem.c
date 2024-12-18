/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include "common/logger.h"
#include "libs/mem.h"

static __maybe_unused
void nornir_overwrite_romem_by_vmap(void *dst, void *src, size_t len)
{
    size_t dst_virt, dst_off, dst_remap;
    struct page **pages;
    unsigned int page_nr, i;

    page_nr = (len >> PAGE_SHIFT) + 2;
    pages = kcalloc(page_nr, sizeof(struct page*), GFP_KERNEL);
    if (!pages) {
        logger_error(
            "Unable to allocate page array for vmap, operation aborted.\n"
        );
        return ;
    }

    dst_virt = (size_t) dst & PAGE_MASK;
    dst_off = (size_t) dst & (PAGE_SIZE - 1);
    for (i = 0; i < page_nr; i++) {
        pages[i] = virt_to_page(dst_virt);
        dst_virt += PAGE_SIZE;
    }

    dst_remap = (size_t) vmap(pages, page_nr, VM_MAP, PAGE_KERNEL);
    if (dst_remap == 0) {
        logger_error("Unable to map pages with vmap, operation aborted.\n");
        goto free_pages;
    }

    memcpy((void*) (dst_remap + dst_off), src, len);

    vunmap((void*) dst_remap);

free_pages:
    kfree(pages);
}

static __maybe_unused
void nornir_overwrite_romem_by_ioremap(void *dst, void *src, size_t len)
{
    size_t dst_phys, dst_off, dst_remap;

    dst_phys = page_to_pfn(virt_to_page(dst)) * PAGE_SIZE;
    dst_off = (size_t) dst & (PAGE_SIZE - 1);

    dst_remap = (size_t) ioremap(dst_phys, len + PAGE_SIZE);
    memcpy((void*) (dst_remap + dst_off), src, len);
    iounmap((void*) dst_remap);
}

static __maybe_unused
void nornir_overwrite_romem_by_cr0(void *dst, void *src, size_t len)
{
    u64 orig_cr0;

    orig_cr0 = nornir_read_cr0();
    nornir_disable_write_protect();

    memcpy(dst, src, len);

    if ((orig_cr0 >> 16) & 1) {
        nornir_enable_write_protect();
    }
}

static __maybe_unused
void nornir_overwrite_romem_by_pgtbl(void *dst, void *src, size_t len)
{
    pte_t *dst_pte;
    pte_t orig_pte_val;
    unsigned int level;
    size_t left;

    do {
        dst_pte = lookup_address((unsigned long) dst, &level);
        orig_pte_val.pte = dst_pte->pte;

        left = PAGE_SIZE - ((size_t) dst & (PAGE_SIZE - 1));

        dst_pte->pte |= _PAGE_RW;
        memcpy(dst, src, left);

        dst_pte->pte = orig_pte_val.pte;

        dst = (void*) ((size_t) dst + left);
        src = (void*) ((size_t) src + left);
        len -= left;
    } while (len > PAGE_SIZE);
}

void nornir_overwrite_romem(void *dst, void *src, size_t len)
{
#ifdef CONFIG_NORNIR_VMAP_TAMPER_ROMEM
    nornir_overwrite_romem_by_vmap(dst, src, len);
#elif CONFIG_NORNIR_IOREMAP_TAMPER_ROMEM
    #ifndef CONFIG_X86_64
        #error "Current kernel do not enable CONFIG_X86_64"
    #endif
    nornir_overwrite_romem_by_ioremap(dst, src, len);
#elif CONFIG_NORNIR_CR0_TAMPER_ROMEM
    #ifndef CONFIG_X86_64
        #error "Current kernel do not enable CONFIG_X86_64"
    #endif
    nornir_overwrite_romem_by_cr0(dst, src, len);
#elif CONFIG_NORNIR_PGTBL_TAMPER_ROMEM
    #ifndef CONFIG_X86_64
        #error "Current kernel do not enable CONFIG_X86_64"
    #endif
    nornir_overwrite_romem_by_pgtbl(dst, src, len);
#else
    #error "No romem-overwrite techniques were chosen"
#endif
}
