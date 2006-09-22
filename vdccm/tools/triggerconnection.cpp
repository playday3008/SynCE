//
// C++ Implementation: triggerconnection
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>


int main(int argc, char *argv[])
{
    int fd;

    struct hostent *rhostent;
    struct sockaddr_in raddr;

    if ((rhostent = gethostbyname(argv[1])) == NULL) {
        return -1;
    }
    if ((fd = ::socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        return -1;
    }

    raddr.sin_port = htons(5679);
    raddr.sin_family = AF_INET;
    memcpy(&raddr.sin_addr, rhostent->h_addr, rhostent->h_length);

    char buf = 0x7f;

    if (sendto(fd, &buf, 1, 0, (struct sockaddr *) &raddr, sizeof(raddr)) < 0) {
        return -1;
    }

    return 0;
}
