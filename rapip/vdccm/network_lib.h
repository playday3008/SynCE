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


#ifndef NETWORK_LIB_H
#define NETWORK_LIB_H

#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#ifndef PF_LOCAL
#define PF_LOCAL PF_UNIX
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_UNIX
#endif

#include "select_lib.h"

typedef struct local_s local_t;
typedef local_t *local_p;

typedef int (*local_fnc_p)(local_p local, void *data);

typedef struct localwrap_s localwrap_t;
typedef localwrap_t *localwrap_p;

struct local_s
{
    struct sockaddr_un laddr;
    struct sockaddr_un raddr;
    int fd;
    list_p local_l;
    local_p local_b;
    int count;
    void *user_data;

    localwrap_p rlocalwrap;
    localwrap_p wlocalwrap;
    localwrap_p elocalwrap;
    select_p rsel;
    select_p wsel;
    select_p esel;
};


struct localwrap_s
{
    local_fnc_p local_fnc;
    local_p local;
    void *data;
};


int local_write(local_p local, void *buf, size_t count);
int local_read(local_p local, void *buf, size_t count);
int local_select_delfrom_rlist(local_p local);
local_p local_accept_socket(local_p local);
int local_select_addto_rlist(select_p sel, local_p local,
                             local_fnc_p local_fnc, void *data);
local_p local_listen_socket(char *path, int backlog);
local_p local_connect_socket(const char *path);
int local_close_socket(local_p local);

#endif
