/*
 *  Copyright (c) 2008 Atheros Communications Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef _PROJECT_H_
#define _PROJECT_H_

/*
 * Different Atheros codebases have similar yet slightly different
 * low-level primitives, e.g. for OS abstractions.
 * Each codebase that shares code with other codebases needs a project.h
 * file to provide a translation from the codebase-agnostic abstractions
 * of the differing primitives into the codebase's local vernacular.
 *
 * This project.h file translates from the codebase-agnostic primitive
 * abstraction into the newma / MBU codebase's own primitives.
 */

#include "if_athvar.h"
#include "ath_timer.h"

#define COM_DEV struct ieee80211com

/* Timer functions */
typedef struct ath_timer timer_struct_t;

#define TIMER_FUNC(fn) int fn(void *context)
typedef int (*timer_func_t)(void *context);

#define TIMER_INIT(osdev, timer_obj, timer_hdler, context)\
    ath_initialize_timer(osdev, timer_obj, 0, timer_hdler, context) /* Timer period = 0 */
#define TIMER_START(timer_obj, period) ath_set_timer_period(timer_obj, period); ath_start_timer(timer_obj)
#define TIMER_CANCEL(timer_obj, flags) ath_cancel_timer(timer_obj, flags)
#define TIMER_FREE(timer_obj) ath_free_timer(timer_obj)
/* Return 0 to reschedule timer */
#define TIMER_RETURN(arg) return arg


#endif /* _PROJECT_H_ */

