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

#include "syncedesktopkonnectorconfig.h"

#include "syncedesktopkonnector.h"

#include <libkcal/resourcelocal.h>

#include <kconfig.h>
#include <klocale.h>
#include <kabc/resourcefile.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <klineedit.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>

using namespace KSync;

SynCEDesktopKonnectorConfig::SynCEDesktopKonnectorConfig( QWidget *parent, const char *name )
        : SynCEKonnectorConfigBase( parent, name )
{
    QBoxLayout * topLayout = new QVBoxLayout( this );

    QLabel *contactLabel = new QLabel( i18n( "Select the address book you want to sync with." ), this );
    topLayout->addWidget( contactLabel );

    mContactResourceBox = new QComboBox( this );
    topLayout->addWidget( mContactResourceBox );

    topLayout->addSpacing( 4 );

    QLabel *calendarLabel = new QLabel( i18n( "Select the calendar you want to sync with." ), this );
    topLayout->addWidget( calendarLabel );

    mCalendarResourceBox = new QComboBox( this );
    topLayout->addWidget( mCalendarResourceBox );

    KRES::Manager<KABC::Resource> contactManager( "contact" );
    contactManager.readConfig();
    KRES::Manager<KABC::Resource>::ActiveIterator kabcIt;
    for ( kabcIt = contactManager.activeBegin(); kabcIt != contactManager.activeEnd(); ++kabcIt ) {
        mContactResourceIdentifiers.append( (*kabcIt)->identifier() );
        mContactResourceBox->insertItem( (*kabcIt)->resourceName() );
    }

    KRES::Manager<KCal::ResourceCalendar> calendarManager( "calendar" );
    calendarManager.readConfig();
    KRES::Manager<KCal::ResourceCalendar>::ActiveIterator kcalIt;
    for ( kcalIt = calendarManager.activeBegin(); kcalIt != calendarManager.activeEnd(); ++kcalIt ) {
        mCalendarResourceIdentifiers.append( (*kcalIt)->identifier() );
        mCalendarResourceBox->insertItem( (*kcalIt)->resourceName() );
    }
}


SynCEDesktopKonnectorConfig::~SynCEDesktopKonnectorConfig()
{}


void SynCEDesktopKonnectorConfig::loadSettings( KRES::Resource *resource )
{
    SynCEDesktopKonnector *konnector = dynamic_cast<SynCEDesktopKonnector *>( resource );
    if ( konnector ) {
        int pos = mContactResourceIdentifiers.findIndex( konnector->currentContactResource() );
        mContactResourceBox->setCurrentItem( pos );

        pos = mCalendarResourceIdentifiers.findIndex( konnector->currentCalendarResource() );
        mCalendarResourceBox->setCurrentItem( pos );
    }
}


void SynCEDesktopKonnectorConfig::saveSettings( KRES::Resource *resource )
{
    SynCEDesktopKonnector *konnector = dynamic_cast<SynCEDesktopKonnector *>( resource );
    if ( konnector ) {
        int pos = mContactResourceBox->currentItem();
        konnector->setCurrentContactResource( mContactResourceIdentifiers[ pos ] );

        pos = mCalendarResourceBox->currentItem();
        konnector->setCurrentCalendarResource( mCalendarResourceIdentifiers[ pos ] );
    }
}


void SynCEDesktopKonnectorConfig::enableRaki()
{}

#include "syncedesktopkonnectorconfig.moc"
