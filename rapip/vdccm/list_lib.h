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


#ifndef LIST_H
#define LIST_H


#define list_iterator(list, element) \
    for (element = list_get_first((list)); element != NULL;\
            element = list_get_next((list)))

#define list_iterator_inverse(list, element) \
    for (element = list_get_last((list)); element != NULL;\
            element = list_get_prev((list)))

struct list_data_s
{
    void *data;
    int stamp;
    struct list_data_s *next;
    struct list_data_s *prev;
};

typedef struct list_data_s list_data_t;
typedef list_data_t *list_data_p;

#include <limits.h>

#define MAXSTACK = sizeof(size_t) * CHAR_BIT;


struct list_s
{
    struct list_data_s *head;
    struct list_data_s *tail;
    struct list_data_s *act;
    struct list_data_s *act_p;
    struct list_data_s *act_n;
    int count;
    int add;
    int del;
};

typedef struct list_s list_t;
typedef list_t *list_p;

typedef void (*destroy_func_p)(list_p list, void *listdata, void *data);

list_p list_create(void);
void list_destroy_with_data(list_p *list);
int list_insert_tail(list_p list, void *data);
int list_insert_at(list_p list, int at, void *data);
int list_delete_data(list_p list, void *data);
void *list_get_first(list_p list);
void *list_get_next(list_p list);
int list_is_member(list_p list, void *data);
int list_get_count(list_p list);
int list_quick_sort(list_p list, int (*cmp_fn)(void *prev, void *next));

int list_insert_sorted(list_p list, int (*cmp_fn)(void *prev_data,
                       void *next_data), void *data);
#endif
