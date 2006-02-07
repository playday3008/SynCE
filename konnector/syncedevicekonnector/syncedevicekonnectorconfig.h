/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                    Christian Fremgen <cfremgen@users.sourceforge.net>   *
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
class SynCEDeviceKonnectorConfig : public SynCEKonnectorConfigBase
{
Q_OBJECT
public:
    SynCEDeviceKonnectorConfig(QWidget *parent = 0, const char *name = 0);

    ~SynCEDeviceKonnectorConfig();

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
