/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_PROC_HIDE_H
#define NORNIR_PROC_HIDE_H

extern void nornir_hide_process_pid(struct task_struct *task, struct pid *pid);
extern void nornir_hide_process_task_struct(struct task_struct *task);

#endif // NORNIR_PROC_HIDE_H