/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/types.h>
#include <linux/mman.h>
#include <linux/pid_namespace.h>

#include "common/logger.h"
#include "libs/ksym.h"

static const char common_blank_seperators[] = {
    ' ', '\t', '\n', '\b'
};

static const int
common_blank_seperator_nr = sizeof(common_blank_seperators) / sizeof(char);

static __always_inline
int is_seperator(const char ch,const char *seperators,const size_t seperator_nr)
{
    for (size_t i = 0; i < seperator_nr; i++) {
        if (ch == seperators[i]) {
            return 1;
        }
    }

    return 0;
}

static __always_inline size_t get_next_token_in_str(const char *src,
                                                    char *res,
                                                    size_t *ppos,
                                                    size_t end_pos,
                                                    const char *seperators,
                                                    int seperator_nr)
{
    size_t len = 0;

    /* default seperators */
    if ((!seperators) || seperator_nr == -1) {
        seperators = common_blank_seperators;
        seperator_nr = common_blank_seperator_nr;
    }

    /* read a token */
    while (!is_seperator(src[*ppos], seperators, seperator_nr)) {
        if ((*ppos) >= end_pos) {
            break;
        }

        res[len++] = src[(*ppos)++];
    }

    res[len] ='\0';

    /* clear redundant seperators */
    while ((*ppos)<end_pos && is_seperator(src[*ppos],seperators,seperator_nr)){
        (*ppos)++;
    }

    return len;
}

static __always_inline int get_next_kallsyms_info(char *ksym_buf,
                                                  size_t ksym_buf_len,
                                                  struct ksym_info *res)
{
    char *buf;
    size_t len, pos = 0;
    enum {
        KADDR = 0,
        KTYPE,
        KNAME,
        KMODNAME,
        KUNKNOWN,
    } status;
    int ret = 0;

    buf = kmalloc(ksym_buf_len, GFP_KERNEL);
    if (!buf) {
        return -ENOMEM;
    }

    for (status = KADDR; ; status++) {
        len = get_next_token_in_str(ksym_buf, buf, &pos, ksym_buf_len, NULL,-1);
        if (len == 0) {
            break;
        }

        switch (status) {
        case KADDR:
            if (unlikely(len != 16)) {
                for (size_t i = 0; i < len; i++) {
                    if ((buf[i] < '0' || buf[i] > '9')
                        && (buf[i] < 'a' || buf[i] > 'f')) {
                        printk(KERN_ERR
                               "Got unknown char [%c] in ksyms addr\n", buf[i]);
                        ret = -EINVAL;
                        goto out;
                    }
                }
            }

            res->addr = 0;
            for (int i = 0; i < len; i++) {
                res->addr <<= 4;
                if (buf[i] >= '0' && buf[i] <= '9') {
                    res->addr |= buf[i] - '0';
                } else if (buf[i] >= 'a' && buf[i] <= 'f') {
                    res->addr |= buf[i] - 'a' + 0xa;
                } else {
                    printk(KERN_ERR
                           "Got unknown char [%c] in ksyms addr", buf[i]);
                    ret = -EINVAL;
                    goto out;
                }
            }

            break;

        case KTYPE:
            /* sometimes it might be missing, fall through */
            if (strlen(buf) == 1) {
                res->type = buf[0];
                continue;
            }

            fallthrough;

        case KNAME:
            strcpy(res->name, buf);
            break;

        case KMODNAME:
            strcpy(res->module, buf);
            break;

        default:
            printk(KERN_ERR
                   "Got unhandlable token [%s] in kallsyms line\n", buf);
            ret = -EINVAL;
            goto out;
            break;
        }
    }

out:
    kfree(buf);

    return ret;
}

static ssize_t nornir_find_ksym_info(struct file *ksym_fp,
                                     const char *target_name,
                                     struct ksym_info *res,
                                     const char **ignore_mods,
                                     const char *ignore_types)
{
    char *ksym_buf, *per_ksym_buf;
    size_t ksym_buf_len, per_ksym_len, per_ksym_pos;
    ssize_t read_len;
    struct ksym_info *curr_info;
    ssize_t ret = 0;
    int found = 0;
    loff_t fpos;
    static const char line_seperator[] = { '\n' };
    int parse_ret;
    char __user *ubuf;

    if (!ksym_fp || !res) {
        ret = -EFAULT;
        goto out_ret;
    }

    ubuf = (void*) vm_mmap(NULL,
                           0,
                           PAGE_SIZE,
                           PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE,
                           0);
    if (IS_ERR(ubuf)) {
        ret = PTR_ERR(ubuf);
        goto out_ret;
    }

    curr_info = kmalloc(sizeof(*curr_info), GFP_KERNEL);
    if (!curr_info) {
        ret = -ENOMEM;
        goto out_free_ubuf;
    }

    per_ksym_buf = kmalloc(0x200, GFP_KERNEL);
    if (!per_ksym_buf) {
        ret = -ENOMEM;
        goto out_free_buf;
    }

    ksym_buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!ksym_buf) {
        ret = -ENOMEM;
        goto out_free_info;
    }

    for (;;) {
        fpos = ksym_fp->f_pos;
        read_len = ksym_fp->f_op->read(ksym_fp, ubuf, PAGE_SIZE, &fpos);
        if (read_len < 0) {
            ret = read_len;
            goto out_free_linebuf;
        }

        ksym_fp->f_pos = fpos;
        if (read_len == 0) {
            break;
        }
        ksym_buf_len = read_len;

        if (!ksym_buf || !ubuf) {
            ret = -EFAULT;
            goto out_free_linebuf;
        }

        if (copy_from_user(ksym_buf, ubuf, ksym_buf_len)) {
            ret = -EFAULT;
            goto out_free_linebuf;
        }

        per_ksym_pos = 0;
        for (;;) {
        token_again:
            per_ksym_len = get_next_token_in_str(ksym_buf,
                                                  per_ksym_buf,
                                                  &per_ksym_pos,
                                                  ksym_buf_len,
                                                  line_seperator,
                                                  1);
            if (per_ksym_len == 0) {
                break;
            }

            memset(res, 0, sizeof(*res));
            parse_ret = get_next_kallsyms_info(per_ksym_buf,per_ksym_len,res);
            if (parse_ret) {
                ret = parse_ret;
                goto out_free_linebuf;
            }

            if (!strcmp(res->name, target_name)) {
                /* we may have some symbols in other modules we don't want */
                if (strlen(res->module) != 0 && ignore_mods) {
                    for (const char **mod = ignore_mods; *mod; mod++) {
                        if (!strcmp(*mod, res->module)) {
                            goto token_again;
                        }
                    }
                }

                if (ignore_types) {
                    for (const char *ptyp = ignore_types; *ptyp; ptyp++) {
                        if (res->type == *ptyp) {
                            goto token_again;
                        }
                    }
                }

                found = 1;
                goto out_loop;
            }
        }
    }

out_loop:
    if (!found) {
        ret = -ENODATA;
    }

out_free_linebuf:
    kfree(per_ksym_buf);

out_free_buf:
    kfree(ksym_buf);

out_free_info:
    kfree(curr_info);

out_free_ubuf:
    vm_munmap((unsigned long) ubuf, PAGE_SIZE);

out_ret:

    return ret;
}

static int nornir_ksym_addr_lookup_internal(const char *name,
                                            size_t *res,
                                            const char **ignore_mods,
                                            const char *ignore_types)
{
    int error = 0;
    struct file *ksym_fp;
    struct ksym_info *info;

    ksym_fp = filp_open("/proc/kallsyms", O_RDONLY, 0);

    if (IS_ERR(ksym_fp)) {
        error = PTR_ERR(ksym_fp);
        goto out_ret;
    }

    info = kmalloc(sizeof(*info), GFP_KERNEL);
    if (!info) {
        error = -ENOMEM;
        goto out_free_file;
    }

    error=nornir_find_ksym_info(ksym_fp, name, info, ignore_mods, ignore_types);
    if (error) {
        goto out_free_info;
    }

    *res = info->addr;

out_free_info:
    kfree(info);
out_free_file:
    filp_close(ksym_fp, NULL);
out_ret:
    return error;
}

int nornir_ksym_addr_lookup(const char *name,
                         size_t *res,
                         const char **ignore_mods,
                         const char *ignore_types)
{
    struct cred *old, *root;
    int ret;

    old = (struct cred*) get_current_cred();

    root = prepare_kernel_cred(
        pid_task(
            find_pid_ns(1, task_active_pid_ns(current)),
            PIDTYPE_PID
        )
    );
    if (!root) {
        logger_error("FAILED to allocated a new cred, kallsyms lookup failed.");
        put_cred(old);
        return -ENOMEM;
    }

    get_cred(root);
    commit_creds(root);

    ret = nornir_ksym_addr_lookup_internal(name, res, ignore_mods, ignore_types);

    commit_creds(old);

    put_cred(root);

    return ret;
}

