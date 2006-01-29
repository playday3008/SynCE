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
#ifndef POCKETPCHELPERPOCKETPCKONNECTORCONFIG_H
#define POCKETPCHELPERPOCKETPCKONNECTORCONFIG_H

#include <syncekonnectorconfigbase.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qcheckbox.h>

namespace KSync {

/**
@author Christian Fremgen
*/
class PocketPCKonnectorConfig : public SynCEKonnectorConfigBase
{
Q_OBJECT
public:
    PocketPCKonnectorConfig(QWidget *parent = 0, const char *name = 0);

    ~PocketPCKonnectorConfig();

    void loadSettings (KRES::Resource* p_res);
    void saveSettings (KRES::Resource* p_res);

    void enableRaki();

private:
    QLabel*        m_label;
    QLineEdit*     m_textPdaName;
    QGridLayout*   m_layout;
    QGridLayout*   m_layout1;
    QGridLayout*   m_layoutMain;

    QLabel*        m_Synctarget;
    QLabel*        m_ActiveLabel;
    QLabel*        m_FirstSyncLabel;

    QLabel*        m_ContactsLabel;
    QLabel*        m_EventsLabel;
    QLabel*        m_TodosLabel;
    QCheckBox*     m_ContactsEnabled;
    QCheckBox*     m_ContactsFirstSync;
    QCheckBox*     m_EventsEnabled;
    QCheckBox*     m_EventsFirstSync;
    QCheckBox*     m_TodosEnabled;
    QCheckBox*     m_TodosFirstSync;

};

}

#endif
