/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_LIBS_KSYM_H
#define NORNIR_LIBS_KSYM_H

#include <linux/types.h>

#define KALLSYMS_NAME_LEN 512
#define KALLSYMS_MODNAME_LEN 512 

struct ksym_info {
    char name[KALLSYMS_NAME_LEN];
    char type;
    size_t addr;
    char module[KALLSYMS_MODNAME_LEN];
};

extern int nornir_ksym_addr_lookup(const char *name,
                                   size_t *res,
                                   const char **ignore_mods,
                                   const char *ignore_types);

#endif // NORNIR_LIBS_KSYM_H
