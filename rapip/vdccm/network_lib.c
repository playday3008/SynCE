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


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <errno.h>

#include "network_lib.h"



/* UNIX-Functions */

local_p local_listen_socket(char *path, int backlog)
{
    size_t size;
    local_p local;


    errno = 0;

    unlink(path);

    local = (local_p) malloc(sizeof(local_t));
    local->rlocalwrap = NULL;
    local->wlocalwrap = NULL;
    local->elocalwrap = NULL;
    local->local_b = NULL;
    local->local_l = NULL;
    local->rsel = NULL;
    local->wsel = NULL;
    local->esel = NULL;

    if ((local->fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
        local_close_socket(local);
        return NULL;
    }

    local->laddr.sun_family = AF_LOCAL;
    strncpy(local->laddr.sun_path, path, sizeof(local->laddr.sun_path));

    size = (offsetof (struct sockaddr_un, sun_path)
            + strlen(local->laddr.sun_path) + 1);

    if (bind(local->fd, (struct sockaddr *) &local->laddr, size) < 0) {
        local_close_socket(local);
        return NULL;
    }

    if (listen(local->fd, backlog) < 0) {
        local_close_socket(local);
        return NULL;
    }

    local->local_l = list_create();
    local->local_b = NULL;
    local->count = 0;

    return local;
}


local_p local_accept_socket(local_p local)
{
    int true = 1;
    socklen_t len = sizeof(struct sockaddr);
    local_p local_c;


    errno = 0;

    local_c = (local_p) malloc(sizeof(local_t));
    local_c->rlocalwrap = NULL;
    local_c->wlocalwrap = NULL;
    local_c->elocalwrap = NULL;
    local_c->rsel = NULL;
    local_c->wsel = NULL;
    local_c->esel = NULL;
    local_c->local_b = NULL;
    local_c->local_l = NULL;

    if ((local_c->fd = accept(local->fd, (struct sockaddr *) &local_c->raddr, &len)) < 0) {
        free(local_c);
        return NULL;
    }

    if (setsockopt(local_c->fd, SOL_SOCKET, SO_KEEPALIVE, &true, sizeof(true)) < 0) {
        local_close_socket(local_c);
        return NULL;
    }

    local_c->laddr = local->laddr;
    local_c->local_b = local;
    local->count++;
    list_insert_tail(local->local_l, local_c);

    return local_c;
}


local_p local_connect_socket(const char *path)
{
	size_t size;
	local_p local;
	socklen_t len = sizeof(struct sockaddr);


	errno = 0;

	local = (local_p) malloc(sizeof(local_t));
	local->rlocalwrap = NULL;
	local->wlocalwrap = NULL;
	local->elocalwrap = NULL;
	local->local_b = NULL;
	local->local_l = NULL;
	local->rsel = NULL;
	local->wsel = NULL;
	local->esel = NULL;

	if ((local->fd = socket(PF_LOCAL, SOCK_STREAM, 0)) < 0) {
		local_close_socket(local);
        free(local);
		return NULL;
	}

	local->raddr.sun_family = AF_LOCAL;
	strncpy(local->raddr.sun_path, path, sizeof(local->raddr.sun_path));

	size = (offsetof (struct sockaddr_un, sun_path)
			+ strlen(local->raddr.sun_path) + 1);

	if (connect(local->fd, (struct sockaddr *) &local->raddr, sizeof(local->raddr)) < 0) {
        free(local);
		return NULL;
	}

	if (getsockname(local->fd, (struct sockaddr *) &local->laddr, &len) < 0) {
        free(local);
		return NULL;
	}

	return local;
}


int local_close_socket(local_p local)
{
    errno = 0;


    if (local == NULL)
        return -1;

    local_select_delfrom_rlist(local);

    if (local->local_b != NULL) {
        list_delete_data(local->local_b->local_l, local);
        local->local_b->count--;
    }

    if (shutdown(local->fd, 2) < 0) {
        close(local->fd);
        free(local);
        return -1;
    }

    if (close(local->fd) < 0) {
        free(local);
        return -1;
    }

    free(local);

    return 0;
}


int local_write(local_p local, void *buf, size_t count)
{
    int ret;

    ret = write(local->fd, buf, count);

    return ret;
}


int local_read(local_p local, void *buf, size_t count)
{
    int ret;

    ret = read(local->fd, buf, count);

    return ret;
}


static int local_wrapper_fnc(void *data)
{
    int ret;


    localwrap_p localwrap = (localwrap_p) data;

    ret = (*localwrap->local_fnc)(localwrap->local, localwrap->data);

    return ret;
}


int local_select_addto_rlist(select_p sel, local_p local,
                             local_fnc_p local_fnc, void *data)
{
    int ret;
    localwrap_p localwrap;


    if (local->rsel != NULL)
        return -1;

    if ((localwrap = (localwrap_p) malloc(sizeof(localwrap_t))) == NULL) {
        return -1;
    }

    localwrap->local_fnc = local_fnc;
    localwrap->local = local;
    localwrap->data = data;
    local->rlocalwrap = localwrap;
    local->rsel = sel;

    ret = select_addto_rlist(sel, local->fd, local_wrapper_fnc, localwrap);

    return ret;
}


int local_select_delfrom_rlist(local_p local)
{
    int ret;


    if (local->rsel != NULL) {
        ret = select_delfrom_rlist(local->rsel, local->fd);
        if (local->rlocalwrap != NULL) {
            free(local->rlocalwrap);
            local->rlocalwrap = NULL;
        }
        local->rsel = NULL;
    } else {
        ret = -1;
    }

    return ret;
}
