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


#include <stdlib.h>

#include "list_lib.h"

static int qsort_count;


static list_data_p list_get_element_first(list_p list)
{
    return list->head;
}


static list_data_p list_get_element_last(list_p list)
{
    return list->tail;
}


static list_data_p list_get_element_at(list_p list, int at)
{
    list_data_p act;

    if (at > list->count) {
        if (list->count > 0) {
            return list->tail;
        } else {
            return NULL;
        }
    }

    if (at < 1 ) {
        if (list->count > 0) {
            return list->head;
        } else {
            return NULL;
        }
    }

    act = list_get_element_first(list);

    while(--at) {
        act = act->next;
    }

    return act;
}


static list_data_p list_get_element_data(list_p list, void *data)
{
    list_data_p act;

    for (act = list->head; act != NULL; act = act->next)
        if (act->data == data)
            return act;

    return NULL;
}


static int list_del_element(list_p list, list_data_p act)
{
    if (list->act != NULL) {
        if (list->act == act) {
            list->act = NULL;
            list->act_p = act->prev;
            list->act_n = act->next;
        }

        if (list->act_p == act)
            list->act_p = act->prev;

        if (list->act_n == act)
            list->act_n = act->next;
    }

    if (list->head == act)
        list->head = act->next;

    if (list->tail == act)
        list->tail = act->prev;

    if (act->prev != NULL)
        act->prev->next = act->next;

    if (act->next != NULL)
        act->next->prev = act->prev;

    free (act);

    return 0;
}


static list_data_p list_new_data_element(void *data)
{
    list_data_p act;

    if ((act = (list_data_p) malloc(sizeof(list_data_t))) == NULL)
        return NULL;

    act->prev = NULL;
    act->next = NULL;
    act->data = data;

    return act;
}


/*
---------------------------------------------------------
		Extern Functions
---------------------------------------------------------
*/


list_p list_create(void)
{
    list_p act;

    if ((act = (list_p) malloc(sizeof(list_t))) == NULL)
        return NULL;

    if (act == NULL)
        return NULL;

    act->tail = NULL;
    act->head = NULL;
    act->act = NULL;
    act->count = 0;
    act->add = 0;
    act->del = 0;

    return act;
}


static void list_clear(list_p list)
{
    list_data_p act;
    list_data_p tmp;

    if (list == NULL)
        return;

    for(act = list->head; act != NULL; act = tmp) {
        tmp = act->next;
        free(act);
    }
}


static void list_clear_data(list_p list)
{
    list_data_p tmp;

    if (list == NULL)
        return;

    for (tmp = list_get_first(list); tmp != NULL; tmp = list_get_next(list))
        free(tmp);
}


static void list_destroy(list_p *list)
{
    if (*list != NULL) {
        list_clear(*list);
        free(*list);
        *list = NULL;
    }
}


void list_destroy_with_data(list_p *list)
{
    list_clear_data(*list);
    list_destroy(list);
}


int list_insert_tail(list_p list, void *data)
{
    list_data_p act;

    if (list == NULL)
        return -1;

    if ((act = list_new_data_element(data)) == NULL)
        return -1;

    if (list->tail != NULL) {
        act->prev = list->tail;
        list->tail->next = act;
    } else {
        list->head = act;
    }

    list->tail = act;
    list->count++;
    list->add++;

    return 0;
}


int list_insert_at(list_p list, int at, void *data)
{
    list_data_p act;
    list_data_p at_element;

    if (list == NULL)
        return -1;

    if (at > list->count) {
        if (list_insert_tail(list, data) < 0) {
            return -1;
        }
        return (list->count - 1);
    }

    if ((at_element = list_get_element_at(list, at)) == NULL)
        return -1;

    if ((act = list_new_data_element(data)) == NULL)
        return -1;

    if (at_element == list->head) {
        list->head = act;
    } else {
        at_element->prev->next = act;
    }

    act->prev = at_element->prev;
    act->next = at_element;
    at_element->prev = act;

    list->count++;
    list->add++;

    return at;
}


int list_delete_data(list_p list, void *data)
{
    list_data_p act;

    if (list == NULL)
        return -1;

    if (list->count == 0)
        return -1;

    if ((act = list_get_element_data(list, data)) == NULL)
        return -1;

    if (list_del_element(list, act) < 0)
        return -1;

    list->count--;
    list->del++;

    return 0;
}


void *list_get_first(list_p list)
{
    list_data_p act;

    if (list == NULL)
        return NULL;

    if (list->count == 0)
        return NULL;

    act = list_get_element_first(list);

    if (act == NULL)
        return NULL;

    list->act = act;

    return act->data;
}


void *list_get_last(list_p list)
{
    list_data_p act;

    if (list == NULL)
        return NULL;

    if (list->count == 0)
        return NULL;

    act = list_get_element_last(list);

    if (act == NULL)
        return NULL;

    list->act = act;

    return act->data;
}


void *list_get_next(list_p list)
{
    if (list == NULL)
        return NULL;

    if (list->count == 0)
        return NULL;

    if (list->act == NULL) {
        if (list->act_p != NULL) {
            list->act = list->act_p;
            list->act_p = NULL;
            list->act_n = NULL;
            list->act = list->act->next;
        } else
            list->act = list_get_element_first(list);
    } else {
        if (list->act == list->tail)
            return NULL;

        if (list->act == NULL)
            return NULL;

        list->act = list->act->next;
    }

    if (list->act == NULL)
        return NULL;

    return list->act->data;
}


int list_get_count(list_p list)
{
    if (list == NULL)
        return -1;

    return list->count;
}


int list_is_member(list_p list, void *data)
{
    int count = 1;
    list_data_p act;

    for (act = list_get_element_first(list); act != NULL; act = act->next) {
        if (act->data == data)
            return count;
        count++;
    }

    return 0;
}


int list_insert_sorted(list_p list, int (*cmp_fn)(void *prev_data,
                       void *next_data), void *data)
{
    list_data_p act;
    int count = 1;


    act = list_get_element_first(list);

    if (act == NULL) {
        return list_insert_tail(list, data);
    }

    while(cmp_fn(data, act->data) >= 0) {
        act = act->next;
        count++;
        if (act == NULL)
            break;
    }

    return list_insert_at(list, count, data);
}


static list_data_p list_partition(list_data_p lb, list_data_p rb,
                                  int (*cmp_fn)(void *prev_data, void *next_data))
{
    void *t, *pivot;
    list_data_p i, j;
    int done;           /* record if pointers cross (means we're done!) */

    /********************************************************
    * partition list [lb..rb], and return pointer to pivot *
    ********************************************************/

    /* select a pivot */
    pivot = lb->data;
    done = 0;

    /* scan from both ends, swapping when needed */
    /* care must be taken not to address outside [lb..rb] with pointers */
    i = lb; j = rb;
    while(1) {
        while ((cmp_fn)(j->data, pivot) > 0) {
            j = j->prev;
            if (i == j)
                done = 1;
        }

        if (done)
            return j;

        while ((cmp_fn)(i->data, pivot) < 0) {
            i = i->next;
            if (i == j)
                done = 1;
        }

        if (done)
            return j;

        /* swap i, j */
        t = i->data;
        i->data = j->data;
        j->data = t;

        /* examine next element */
        j = j->prev;
        if (i == j)
            done = 1;
        i = i->next;
        if (i == j)
            done = 1;
    }
}

static int quick_sort(list_data_p lb, list_data_p rb,
                      int (*cmp_fn)(void *prev_data, void *next_data))
{
    list_data_p m;


    if (lb == rb)
        return qsort_count;

    qsort_count++;

    m = list_partition(lb, rb, cmp_fn);

    if (m != lb)
        quick_sort(lb, m, cmp_fn);              /* sort left side */
    if (m != rb)
        quick_sort(m->next, rb, cmp_fn);        /* sort right side */

    return qsort_count;
}


int list_quick_sort(list_p list, int (*cmp_fn)(void *prev_data,
                    void *next_data))
{
    int ret;

    qsort_count = 0;

    ret = quick_sort(list_get_element_first(list),
                     list_get_element_last(list),
                     cmp_fn);

    return ret;
}

