/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <uapi/linux/stat.h>
#include <linux/version.h>
#include "common/logger.h"
#include "uapi/procfs.h"

#define NORNIR_PROCFS_VERSION_1 KERNEL_VERSION(2, 6, 16)

static int nornir_procfs_open(struct inode *inode, struct file *file)
{
    logger_info("nornir_procfs_open had not been implemented yet.");
    return 0;
}

static ssize_t nornir_procfs_read(struct file *file, char __user *buf,
                                  size_t count, loff_t *start)
{
    logger_info("nornir_procfs_read had not been implemented yet.");
    return count;
}

static ssize_t nornir_procfs_write(struct file *file, const char __user *buf, 
                                   size_t count, loff_t *start)
{
    logger_info("nornir_procfs_write had not been implemented yet.");
    return count;
}

static int nornir_procfs_release(struct inode *inode, struct file *file)
{
    logger_info("nornir_procfs_release had not been implemented yet.");
    return 0;
}

static long
nornir_procfs_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    logger_info("nornir_procfs_ioctl had not been implemented yet.");
    return 0;
}

static const struct proc_ops nornir_procfs_fops = {
    .proc_open      = nornir_procfs_open,
    .proc_read      = nornir_procfs_read,
    .proc_write     = nornir_procfs_write,
    .proc_release   = nornir_procfs_release,
    .proc_ioctl     = nornir_procfs_ioctl,
};

#define NORNIR_PROCFS_UAPI_NODE_NAME "nornir"
static struct proc_dir_entry *nornir_procfs_uapi_dir_entry = NULL;

int nornir_procfs_uapi_init(void)
{
#if LINUX_VERSION_CODE >= NORNIR_PROCFS_VERSION_1
    nornir_procfs_uapi_dir_entry = proc_create(
        NORNIR_PROCFS_UAPI_NODE_NAME,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
        NULL,
        &nornir_procfs_fops
    );
#else
    logger_error(
        "Unsupported kernel version for procfs operations: %d.%d.%d\n",
        LINUX_VERSION_MAJOR,
        LINUX_VERSION_PATCHLEVEL,
        LINUX_VERSION_SUBLEVEL
    );

    return -ECANCELED;
#endif

    if (IS_ERR(nornir_procfs_uapi_dir_entry)) {
        logger_error(
            "Unable to create procfs node, error code: %d\n",
            PTR_ERR(nornir_procfs_uapi_dir_entry)
        );

        return PTR_ERR(nornir_procfs_uapi_dir_entry);
    }

    return 0;
}

void nornir_procfs_uapi_exit(void)
{
#if LINUX_VERSION_CODE >= NORNIR_PROCFS_VERSION_1
    remove_proc_entry(NORNIR_PROCFS_UAPI_NODE_NAME, NULL);
#else
    logger_error(
        "Unsupported kernel version for procfs operations: %d.%d.%d\n",
        LINUX_VERSION_MAJOR,
        LINUX_VERSION_PATCHLEVEL,
        LINUX_VERSION_SUBLEVEL
    );
#endif
}
