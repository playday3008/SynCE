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


#ifndef SELECT_LIB_H
#define SELECT_LIB_H

#include <sys/time.h>
#include <sys/types.h>
#include "list_lib.h"

typedef int (*select_fnc_p)(void *);

struct select_s
{
    list_p rlist;
    int maxfd;
    fd_set rfds;
    struct timeval tv;
    void *data;
};

struct select_fd_s
{
    int fd;
    select_fnc_p fdfn;
    void *data;
};

typedef struct select_s select_t;
typedef select_t *select_p;

typedef struct select_fd_s select_fd_t;
typedef select_fd_t *select_fd_p;

select_p select_create(void);
select_p select_destroy(select_p sel);

int select_addto_rlist(select_p sel, int fd, select_fnc_p fn, void *data);
int select_delfrom_rlist(select_p sel, int fd);
void select_set_timeval(select_p sel, struct timeval timeout);

int select_select(select_p sel);
int select_select_rfd(int fd, int sec, int usec);


#endif
