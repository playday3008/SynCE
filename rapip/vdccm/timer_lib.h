/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/


#ifndef LIB_TIMER_H
#define LIB_TIMER_H

#include <sys/time.h>
#include "list_lib.h"


typedef struct timer_node_s timer_node_t;
typedef timer_node_t *timer_node_p;

typedef struct timer_s voc_timer_t;
typedef voc_timer_t *timer_p;


struct timer_node_s
{
    struct timeval insert_time;     /* Time when this item is added to the list */
    struct timeval trigger_time;    /* Time when the alarm should be fired up */
    struct timeval interval_time;   /* Interval for a continous timer */
    int (*tmfnc)(void *data);       /* Function which is to be called at fire time */
    void *data;                     /* Data which should be passed to the (*tmfnc) */
};


struct timer_s
{
    list_p timer_node_l;
    list_p timer_node_base_l;
    int (*timer_destroy)(void *);
};


struct timer_s *voc_timer_create(void);
struct timeval voc_timer_process_timer(timer_p timer);
timer_node_p voc_timer_add_continous_node(timer_p timer, struct timeval interval_time,
                                      int (*tmfnc)(void *), void *data);

#endif
