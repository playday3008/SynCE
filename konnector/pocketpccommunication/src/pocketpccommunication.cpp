/***************************************************************************
 *   Copyright (C) 2004 by Christian Fremgen                               *
 *   cfremgen@users.sourceforge.net                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "pocketpccommunication.h"

#include <qlabel.h>

#include <kmainwindow.h>
#include <klocale.h>

#include <rapiwrapper.h>

PocketPCCommunication::PocketPCCommunication()
    : KMainWindow( 0, "PocketPCCommunication" )
{
    // set the shell's ui resource file
    setXMLFile("pocketpccommunicationui.rc");

    /* so tell me little drummer-boy.. what the hell does the next line do in here???? */
    /* seems this was just a test... *nirg* */
    //pocketPCCommunication::Ce::rapiInit("");
    
    new QLabel( "Hello World", this, "hello label" );
}

PocketPCCommunication::~PocketPCCommunication()
{
}

#include "pocketpccommunication.moc"
