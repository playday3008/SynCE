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
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include "select_lib.h"


select_p select_create(void)
{
    select_p act;

    if ((act = (select_p) malloc(sizeof(select_t))) == NULL)
        return NULL;

    FD_ZERO(&act->rfds);
    act->tv.tv_sec = 10;
    act->tv.tv_usec = 0;
    act->rlist = list_create();
    act->maxfd = 0;     /* contributed by "Laurent Vivier" <LaurentVivier@wanadoo.fr> */

    return act;
}


select_p select_destroy(select_p s)
{
    list_destroy_with_data(&s->rlist);

    free(s);

    return NULL;
}


static int select_maxfd(select_p s)
{
    int maxfd = 0;
    select_fd_p mylink;


    list_iterator(s->rlist, mylink)
        maxfd = (maxfd < mylink->fd) ? mylink->fd : maxfd;

    return maxfd;
}


int select_addto_rlist(select_p s, int fd, select_fnc_p fdfn, void *data)
{
    select_fd_p mylink;

    if ((mylink = (select_fd_p) malloc(sizeof(select_fd_t))) == NULL)
        return -1;

    mylink->fd = fd;
    mylink->fdfn = fdfn;
    mylink->data = data;

    if (list_insert_tail(s->rlist, mylink) < 0)
        return -1;

    FD_SET(fd, &s->rfds);

    s->maxfd = (s->maxfd < fd) ? fd : s->maxfd;

    return s->maxfd;
}


int select_delfrom_rlist(select_p s, int fd)
{
    select_fd_p mylink;

    for (mylink = list_get_first(s->rlist); mylink != NULL;
            mylink = list_get_next(s->rlist)) {
        if (mylink->fd == fd) {

            list_delete_data(s->rlist, mylink);
            free(mylink);
            mylink = NULL;

            FD_CLR(fd, &s->rfds);

            s->maxfd = (fd == s->maxfd) ? select_maxfd(s) : s->maxfd;

            return 0;
        }
    }
    return -1;
}


void select_set_timeval(select_p s, struct timeval time)
{
    s->tv = time;
}


static int select_process_selected(fd_set fds, list_p list, int count)
{
    int processed = 0;
    select_fd_p mylink;

    for (mylink = list_get_first(list); (mylink != NULL) &&
            (count - processed != 0); mylink = list_get_next(list)) {
        if (FD_ISSET(mylink->fd, &fds)) {
            FD_CLR(mylink->fd, &fds);
            if (mylink->fdfn(mylink->data) < 0) {
                processed = -mylink->fd;
                break;
            }
        }
    }
    return processed;
}


int select_select(select_p s)
{
    int ret = 0;
    int count = 0;
    int ret1 = 0;

    fd_set rfds = s->rfds;
    struct timeval tv = s->tv;

    if ((ret = select(s->maxfd + 1, &rfds, NULL, NULL, &tv)) > 0) {
        if ((ret1 = select_process_selected(rfds, s->rlist,
                                            ret - count)) >= 0) {
            count += ret1;
        }
        if (count != ret) {
            return ret1;
        }
    }

    if (ret < 0 && errno == EINTR)
        ret = 0;

    return ret;
}


static int select_select_fd(int rf, int sec, int usec)
{
    int ret = 0;

    fd_set rfd;
    int maxfd = 0;

    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    FD_ZERO(&rfd);

    if (rf) {
        maxfd = rf;
        FD_SET(rf, &rfd);
    }

    ret = select(maxfd + 1, &rfd, NULL, NULL, &tv);

    return ret;
}


int select_select_rfd(int fd, int sec, int usec)
{
    int ret;

    ret = select_select_fd(fd, sec, usec);

    return ret;
}
