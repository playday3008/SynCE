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
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE S     *
 * OFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   *
 ***************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "timer_lib.h"


static struct timeval epsilon_time = {0, 10000};
static struct timeval null_time = {0, 0};


static struct timeval timer_add_time(struct timeval t1, struct timeval t2)
{
    struct timeval restime;

    restime.tv_usec = t1.tv_usec + t2.tv_usec;
    restime.tv_sec = t1.tv_sec + t2.tv_sec;

    if (restime.tv_usec > 1000000)
    {
        restime.tv_usec -= 1000000;
        restime.tv_sec += 1;
    }

    return restime;
}


static struct timeval timer_sub_time(struct timeval t1, struct timeval t2)
{
    struct timeval restime;


    restime.tv_usec = t1.tv_usec - t2.tv_usec;
    restime.tv_sec = t1.tv_sec - t2.tv_sec;

    if (restime.tv_usec < 0)
    {
        restime.tv_usec += 1000000;
        restime.tv_sec -= 1;
    }

    return restime;
}


static struct timeval timer_get_trigger_time(timer_node_p *current_node, list_p timer_node_l)
{
    struct timeval current_time;
    struct timeval trigger_time;

    *current_node = (timer_node_p) list_get_first(timer_node_l);

    if (*current_node == NULL)
        return null_time;

    gettimeofday(&current_time, NULL);

    trigger_time = timer_sub_time((*current_node)->trigger_time, current_time);

    return trigger_time;
}


static int timer_sort_crit(void *prev, void *next)
{
    timer_node_p timer_node_prev = prev;
    timer_node_p timer_node_next = next;
    struct timeval trigger_time_prev = timer_node_prev->trigger_time;
    struct timeval trigger_time_next = timer_node_next->trigger_time;


    if (timercmp(&trigger_time_prev, &trigger_time_next, >)) {
        return 1;
    } else if (timercmp(&trigger_time_prev, &trigger_time_next, <)) {
        return -1;
    }

    return 0;
}


static struct timeval timer_process_timer_real(timer_p timer)
{
    int cont = 0;
    struct timeval trigger_time = {0, 0};
    timer_node_p timer_node;
    int counter = 0;


    do
    {
        trigger_time = timer_get_trigger_time(&timer_node, timer->timer_node_l);

        if (timercmp(&trigger_time, &epsilon_time, <=) && timer_node != NULL) {

            (timer_node->tmfnc)(timer_node->data);

            timer_node->trigger_time = timer_add_time(timer_node->trigger_time,
                                        timer_node->interval_time);
            list_quick_sort(timer->timer_node_l, timer_sort_crit);

            counter++;

            cont = 1;
        } else {
            cont = 0;
        }
    } while (cont);

    return trigger_time;
}


struct timeval voc_timer_process_timer(timer_p timer)
{
    struct timeval trigger_time = null_time;

    if (timer != NULL)
        trigger_time = timer_process_timer_real(timer);

    return trigger_time;
}


timer_node_p voc_timer_add_continous_node(timer_p timer, struct timeval interval_time,
                                      int (*tmfnc)(void *), void *data)
{
    timer_node_p timer_node;

    timer_node = (timer_node_p) malloc(sizeof(timer_node_t));
    if (timer_node == NULL)    
        return NULL;

    timer_node->tmfnc = tmfnc;
    timer_node->data = data;
    timer_node->interval_time = interval_time;

    gettimeofday(&timer_node->insert_time, NULL);

    timer_node->trigger_time = timer_add_time(timer_node->interval_time, timer_node->insert_time);
    
    if (list_insert_sorted(timer->timer_node_l, timer_sort_crit, timer_node) < 0)
        return NULL;

    if (list_is_member(timer->timer_node_l, timer_node))
        return timer_node;

    return NULL;
}


struct timer_s *voc_timer_create(void)
{
    timer_p timer;

    timer = (timer_p) malloc(sizeof(voc_timer_t));
    
    if (timer == NULL)
        return NULL;
    
    timer->timer_node_base_l = list_create();
    timer->timer_node_l = list_create();
    
    return timer;
}
