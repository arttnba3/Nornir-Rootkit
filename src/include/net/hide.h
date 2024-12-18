/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_NET_HIDE_H
#define NORNIR_NET_HIDE_H

#include <linux/in.h>
#include <linux/in6.h>
#include <linux/list.h>

struct hidden_conn_info {
    struct list_head list;
    struct in_addr addr4;
    struct in6_addr addr6;
};

extern int
nornir_convert_inet4_str_to_addr(const char*addr_str, struct in_addr*result);
extern void nornir_add_new_hidden_conn4(struct in_addr addr);
extern void nornir_add_new_hidden_ipv4_addr(const char *addr);
extern void nornir_remove_hidden_conn4(struct in_addr addr);
extern void nornir_remove_hidden_ipv4_addr(const char *addr);
extern struct hidden_conn_info*
nornir_get_hidden_conn4_info(struct in_addr addr);

extern int nornir_init_conn_hidden_subsystem(void);
extern void nornir_unload_conn_hidden_subsystem(void);

#endif // NORNIR_NET_HIDE_H
