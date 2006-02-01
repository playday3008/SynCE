/*
   This file is part of KitchenSync.

   Copyright (c) 2004 Tobias Koenig <tokoe@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <kcombobox.h>
#include <kdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kresources/factory.h>

#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qtabwidget.h>

#include <kitchensync/multisynk/konnectorpair.h>

#include "paireditorwidget.h"

#include "syncekonnectorconfigbase.h"
#include "syncekonnectorbase.h"

using namespace KRES;

PairEditorWidget::PairEditorWidget( QWidget *parent, const char *name, QString pdaName )
        : QWidget( parent, name ), mResolveManually(0), mResolveFirst(0), mResolveSecond(0), mResolveBoth(0), pdaName(pdaName)
{
}

PairEditorWidget::~PairEditorWidget()
{}

void PairEditorWidget::setPair( KonnectorPair *pair )
{
    mPair = pair;

    switch ( mPair->resolveStrategy() ) {
    case KonnectorPair::ResolveManually:
        if (mResolveManually) {
            mResolveManually->setChecked( true );
        }
        break;
    case KonnectorPair::ResolveFirst:
        if (mResolveFirst) {
            mResolveFirst->setChecked( true );
        }
        break;
    case KonnectorPair::ResolveSecond:
        if (mResolveSecond) {
            mResolveSecond->setChecked( true );
        }
        break;
    case KonnectorPair::ResolveBoth:
        if (mResolveBoth) {
            mResolveBoth->setChecked( true );
        }
        break;
    }

    KonnectorManager *manager = mPair->manager();
    KonnectorManager::Iterator it = manager->begin();

    if ( it != manager->end() ) {
        konnector[ 0 ] = *it;
        it++;
    } else {
        konnector[ 0 ] = manager->createResource( "SynCEDevice" );
        KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( konnector[0] );
        if (k) {
            k->setPdaName(pdaName);
            k->initDefaultFilters();
        }
        manager->add( konnector[ 0 ] );
    }

    if ( it != manager->end() ) {
        konnector[ 1 ] = *it;
    } else {
        konnector[ 1 ] = manager->createResource( "SynCELocal" );
        KSync::SynCEKonnectorBase *k = dynamic_cast<KSync::SynCEKonnectorBase *>( konnector[1] );
        if (k) {
            k->setPdaName(pdaName);
            k->initDefaultFilters();
        }
        manager->add( konnector[ 1 ] );
    }
}

void PairEditorWidget::save()
{
    QValueList<QPair<KRES::ConfigWidget *, KSync::Konnector *> >::iterator it;

    for ( it = configWidgets.begin(); it != configWidgets.end(); ++it ) {
        ( *it ).first->saveSettings( ( *it ).second );
    }
}


KonnectorPair *PairEditorWidget::pair() const
{
    mPair->setName( "SynCEKDEPimPair" );

    if ( mResolveManually->isChecked() )
        mPair->setResolveStrategy( KonnectorPair::ResolveManually );
    else if ( mResolveFirst->isChecked() )
        mPair->setResolveStrategy( KonnectorPair::ResolveFirst );
    else if ( mResolveSecond->isChecked() )
        mPair->setResolveStrategy( KonnectorPair::ResolveSecond );
    else if ( mResolveBoth->isChecked() )
        mPair->setResolveStrategy( KonnectorPair::ResolveBoth );

    return mPair;
}

void PairEditorWidget::initGUI()
{
    QVBoxLayout * layout = new QVBoxLayout( this );

    QTabWidget *tabWidget = new QTabWidget( this );
    layout->addWidget( tabWidget );

    tabWidget->addTab( createPluginTab(), i18n( "Resource Settings" ) );
    tabWidget->addTab( createSyncOptionTab(), i18n( "Synchronize Options" ) );
}

QWidget *PairEditorWidget::createPluginTab()
{
    QWidget * widget = new QWidget( this );
    QVBoxLayout *layout = new QVBoxLayout( widget, KDialog::marginHint(), KDialog::spacingHint() );

    QLabel *label = new QLabel( "<h2><b>" + i18n( "PIM Synchronization Settings" ) + "</b></h2>", widget );
    layout->addWidget( label );

    QVBoxLayout *pluginLayout = new QVBoxLayout( 0, KDialog::marginHint(), KDialog::spacingHint() );

    Factory *factory = Factory::self( "konnector" );

    QGroupBox *resourceGroupBox1 = new QGroupBox( 2, Qt::Horizontal, widget );
    resourceGroupBox1->layout() ->setSpacing( KDialog::spacingHint() );

    resourceGroupBox1->setTitle( i18n( "%1 Resource Settings" )
                                 .arg( factory->typeName( konnector[ 0 ] ->type() ) ) );


    ConfigWidget *mConfigWidget1 = factory->configWidget( konnector[ 0 ] ->type(), resourceGroupBox1 );
    if ( mConfigWidget1 ) {
        mConfigWidget1->setInEditMode( false );
        mConfigWidget1->loadSettings( konnector[ 0 ] );
        mConfigWidget1->show();
        KSync::SynCEKonnectorConfigBase *skcb = dynamic_cast<KSync::SynCEKonnectorConfigBase *>(mConfigWidget1);
        if (skcb) {
            skcb->enableRaki();
        }
    }

    QGroupBox *resourceGroupBox2 = new QGroupBox( 2, Qt::Horizontal, widget );
    resourceGroupBox2->layout() ->setSpacing( KDialog::spacingHint() );
    resourceGroupBox2->setTitle( i18n( "%1 Resource Settings" )
                                 .arg( factory->typeName( konnector[ 1 ] ->type() ) ) );

    ConfigWidget *mConfigWidget2 = factory->configWidget( konnector[ 1 ] ->type(), resourceGroupBox2 );
    if ( mConfigWidget2 ) {
        mConfigWidget2->setInEditMode( false );
        mConfigWidget2->loadSettings( konnector[ 1 ] );
        mConfigWidget2->show();
        KSync::SynCEKonnectorConfigBase *skcb = dynamic_cast<KSync::SynCEKonnectorConfigBase *>(mConfigWidget2);
        if (skcb) {
            skcb->enableRaki();
        }
    }

    pluginLayout->addWidget( resourceGroupBox1 );
    pluginLayout->addWidget( resourceGroupBox2 );

    configWidgets.append( QPair<KRES::ConfigWidget *, KSync::Konnector *>( mConfigWidget1, konnector[ 0 ] ) );
    configWidgets.append( QPair<KRES::ConfigWidget *, KSync::Konnector *>( mConfigWidget2, konnector[ 1 ] ) );

    layout->addLayout( pluginLayout );

    layout->addStretch( 10 );

    return widget;
}

QWidget *PairEditorWidget::createSyncOptionTab()
{
    QWidget * widget = new QWidget( this );
    QVBoxLayout *layout = new QVBoxLayout( widget, KDialog::marginHint(), KDialog::spacingHint() );

    QLabel *label = new QLabel( "<h2><b>" + i18n( "Conflicts &amp; Near Duplicates" ) + "</b></h2>", widget );
    layout->addWidget( label );

    QVBoxLayout *groupLayout = new QVBoxLayout( 0, KDialog::marginHint(), KDialog::spacingHint() );

    QButtonGroup *group = new QButtonGroup( 1, Qt::Horizontal, widget );
    group->setRadioButtonExclusive( true );

    mResolveManually = new QRadioButton( i18n( "Resolve it manually" ), group );
    mResolveFirst = new QRadioButton( i18n( "Always use the entry from the first plugin" ), group );
    mResolveSecond = new QRadioButton( i18n( "Always use the entry from the second plugin" ), group );
    mResolveBoth = new QRadioButton( i18n( "Always put both entries on both sides" ), group );

    switch ( mPair->resolveStrategy() ) {
    case KonnectorPair::ResolveManually:
        mResolveManually->setChecked( true );
        break;
    case KonnectorPair::ResolveFirst:
        mResolveFirst->setChecked( true );
        break;
    case KonnectorPair::ResolveSecond:
        mResolveSecond->setChecked( true );
        break;
    case KonnectorPair::ResolveBoth:
        mResolveBoth->setChecked( true );
        break;
    }

    groupLayout->addWidget( group );

    layout->addLayout( groupLayout );

    layout->addStretch( 10 );

    return widget;
}

QWidget *PairEditorWidget::createFilterTab()
{
    return new QWidget( this );
}

#include "paireditorwidget.moc"
