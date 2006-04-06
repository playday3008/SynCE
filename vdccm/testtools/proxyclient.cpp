//
// C++ Implementation: proxyclient
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <proxyclientsocket.h>
#include <multiplexer.h>
#include <iostream>

using namespace std;

int main(int argc, char* argv[])
{
    ProxyClientSocket *pcs = new ProxyClientSocket(argv[1]);

    if (!pcs->connect()) {
        cout << "Could not connect to vdccm - socket not aviable" << endl;
        exit(0);
    }

//    Multiplexer::self()->getReadManager()->add(pcs);
    Multiplexer::self()->getTimerNodeManager()->add(pcs);

    while(true) {
        Multiplexer::self()->multiplex();
    }
}
