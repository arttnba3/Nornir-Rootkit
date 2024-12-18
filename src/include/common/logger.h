/**
 * Copyright (c) 2024 arttnba3 <arttnba@gmail.com>
 * 
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 */

#ifndef NORNIR_COMMON_LOGGER_H
#define NORNIR_COMMON_LOGGER_H

#include <linux/kernel.h>
#include "common/module.h"

#define LOGGER_PREFIX "[" MODULE_NAME ":] "

/* level 0: system is unusable */
#define logger_emerg(fmt, ...) \
        logger_wrapper(KERN_EMERG, fmt, ##__VA_ARGS__)

/* level 1: action must be taken immediately */
#define logger_alert(fmt, ...) \
        logger_wrapper(KERN_ALERT, fmt, ##__VA_ARGS__)

/* level 2: critical conditions */
#define logger_critical(fmt, ...) \
        logger_wrapper(KERN_CRIT, fmt, ##__VA_ARGS__)

/* level 3: error conditions */
#define logger_error(fmt, ...) \
        logger_wrapper(KERN_ERR, fmt, ##__VA_ARGS__)

/* level 4: warning conditions */
#define logger_warn(fmt, ...) \
        logger_wrapper(KERN_WARNING, fmt, ##__VA_ARGS__)

/* level 5: normal but significant condition */
#define logger_notice(fmt, ...) \
        logger_wrapper(KERN_NOTICE, fmt, ##__VA_ARGS__)

/* level 6: informational */
#define logger_info(fmt, ...) \
        logger_wrapper(KERN_INFO, fmt, ##__VA_ARGS__)

/* level 7: debug-level messages */
#define logger_debug(fmt, ...) \
        logger_wrapper(KERN_DEBUG, fmt, ##__VA_ARGS__)

/* internal wrapper for logger, should not be used directly */
#define logger_wrapper(log_level, fmt, ...) \
        logger_internal(log_level LOGGER_PREFIX fmt, ##__VA_ARGS__)

/* internal logger, should not be called directly */
extern int logger_internal(const char *fmt, ...);

#endif // NORNIR_COMMON_LOGGER_H
