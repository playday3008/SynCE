#include <klocale.h>
/****************************************************************************
** Form implementation generated from reading ui file './agsyncconfig.ui'
**
** Created: Fri Oct 24 22:58:09 2003
**      by: The User Interface Compiler ($Id$)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#include "agsyncconfig.h"

#include <qvariant.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <klineedit.h>
#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include "./agsyncconfig.ui.h"

/*
 *  Constructs a AGSyncConfig as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
AGSyncConfig::AGSyncConfig( QWidget* parent, const char* name, bool modal, WFlags fl )
    : QDialog( parent, name, modal, fl )
{
    if ( !name )
	setName( "AGSyncConfig" );
    setSizeGripEnabled( FALSE );
    AGSyncConfigLayout = new QVBoxLayout( this, 11, 6, "AGSyncConfigLayout"); 

    layout19 = new QHBoxLayout( 0, 0, 6, "layout19"); 

    layout18 = new QVBoxLayout( 0, 0, 6, "layout18"); 

    groupBox2 = new QGroupBox( this, "groupBox2" );
    groupBox2->setColumnLayout(0, Qt::Vertical );
    groupBox2->layout()->setSpacing( 6 );
    groupBox2->layout()->setMargin( 11 );
    groupBox2Layout = new QVBoxLayout( groupBox2->layout() );
    groupBox2Layout->setAlignment( Qt::AlignTop );

    layout8 = new QHBoxLayout( 0, 0, 6, "layout8"); 

    layout3 = new QVBoxLayout( 0, 0, 6, "layout3"); 

    textLabel3 = new QLabel( groupBox2, "textLabel3" );
    layout3->addWidget( textLabel3 );

    textLabel4 = new QLabel( groupBox2, "textLabel4" );
    layout3->addWidget( textLabel4 );
    layout8->addLayout( layout3 );

    layout4 = new QVBoxLayout( 0, 0, 6, "layout4"); 

    httpProxyHost = new KLineEdit( groupBox2, "httpProxyHost" );
    layout4->addWidget( httpProxyHost );

    httpProxyPort = new KLineEdit( groupBox2, "httpProxyPort" );
    layout4->addWidget( httpProxyPort );
    layout8->addLayout( layout4 );
    groupBox2Layout->addLayout( layout8 );

    useAuthentication = new QCheckBox( groupBox2, "useAuthentication" );
    groupBox2Layout->addWidget( useAuthentication );

    layout7 = new QHBoxLayout( 0, 0, 6, "layout7"); 

    layout1 = new QVBoxLayout( 0, 0, 6, "layout1"); 

    _________________q = new QLabel( groupBox2, "_________________q" );
    layout1->addWidget( _________________q );

    textLabel2 = new QLabel( groupBox2, "textLabel2" );
    layout1->addWidget( textLabel2 );
    layout7->addLayout( layout1 );

    layout2 = new QVBoxLayout( 0, 0, 6, "layout2"); 

    userName = new KLineEdit( groupBox2, "userName" );
    userName->setEnabled( FALSE );
    userName->setMinimumSize( QSize( 200, 0 ) );
    layout2->addWidget( userName );

    passWord = new KLineEdit( groupBox2, "passWord" );
    passWord->setEnabled( FALSE );
    passWord->setEchoMode( KLineEdit::Password );
    layout2->addWidget( passWord );
    layout7->addLayout( layout2 );
    groupBox2Layout->addLayout( layout7 );
    layout18->addWidget( groupBox2 );

    groupBox3 = new QGroupBox( this, "groupBox3" );
    groupBox3->setColumnLayout(0, Qt::Vertical );
    groupBox3->layout()->setSpacing( 6 );
    groupBox3->layout()->setMargin( 11 );
    groupBox3Layout = new QHBoxLayout( groupBox3->layout() );
    groupBox3Layout->setAlignment( Qt::AlignTop );

    layout6 = new QVBoxLayout( 0, 0, 6, "layout6"); 

    textLabel5 = new QLabel( groupBox3, "textLabel5" );
    layout6->addWidget( textLabel5 );

    textLabel6 = new QLabel( groupBox3, "textLabel6" );
    layout6->addWidget( textLabel6 );
    groupBox3Layout->addLayout( layout6 );

    layout5 = new QVBoxLayout( 0, 0, 6, "layout5"); 

    socksProxyHost = new KLineEdit( groupBox3, "socksProxyHost" );
    layout5->addWidget( socksProxyHost );

    socksProxyPort = new KLineEdit( groupBox3, "socksProxyPort" );
    layout5->addWidget( socksProxyPort );
    groupBox3Layout->addLayout( layout5 );
    layout18->addWidget( groupBox3 );
    layout19->addLayout( layout18 );

    buttonGroup1 = new QButtonGroup( this, "buttonGroup1" );
    buttonGroup1->setColumnLayout(0, Qt::Vertical );
    buttonGroup1->layout()->setSpacing( 6 );
    buttonGroup1->layout()->setMargin( 11 );
    buttonGroup1Layout = new QVBoxLayout( buttonGroup1->layout() );
    buttonGroup1Layout->setAlignment( Qt::AlignTop );

    noProxy = new QRadioButton( buttonGroup1, "noProxy" );
    buttonGroup1Layout->addWidget( noProxy );

    httpProxy = new QRadioButton( buttonGroup1, "httpProxy" );
    buttonGroup1Layout->addWidget( httpProxy );

    socksProxy = new QRadioButton( buttonGroup1, "socksProxy" );
    buttonGroup1Layout->addWidget( socksProxy );
    QSpacerItem* spacer = new QSpacerItem( 20, 41, QSizePolicy::Minimum, QSizePolicy::Expanding );
    buttonGroup1Layout->addItem( spacer );
    layout19->addWidget( buttonGroup1 );
    AGSyncConfigLayout->addLayout( layout19 );
    QSpacerItem* spacer_2 = new QSpacerItem( 20, 16, QSizePolicy::Minimum, QSizePolicy::Expanding );
    AGSyncConfigLayout->addItem( spacer_2 );

    Layout1 = new QHBoxLayout( 0, 0, 6, "Layout1"); 

    buttonHelp = new QPushButton( this, "buttonHelp" );
    buttonHelp->setAutoDefault( TRUE );
    Layout1->addWidget( buttonHelp );
    QSpacerItem* spacer_3 = new QSpacerItem( 202, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    Layout1->addItem( spacer_3 );

    buttonOk = new QPushButton( this, "buttonOk" );
    buttonOk->setAutoDefault( TRUE );
    buttonOk->setDefault( TRUE );
    Layout1->addWidget( buttonOk );

    buttonCancel = new QPushButton( this, "buttonCancel" );
    buttonCancel->setAutoDefault( TRUE );
    Layout1->addWidget( buttonCancel );
    AGSyncConfigLayout->addLayout( Layout1 );
    languageChange();
    resize( QSize(455, 315).expandedTo(minimumSizeHint()) );
    clearWState( WState_Polished );

    // signals and slots connections
    connect( socksProxy, SIGNAL( stateChanged(int) ), this, SLOT( socksProxy_stateChanged(int) ) );
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
    connect( userName, SIGNAL( textChanged(const QString&) ), this, SLOT( contentChanged() ) );
    connect( passWord, SIGNAL( textChanged(const QString&) ), this, SLOT( contentChanged() ) );
    connect( httpProxyHost, SIGNAL( textChanged(const QString&) ), this, SLOT( contentChanged() ) );
    connect( httpProxyPort, SIGNAL( textChanged(const QString&) ), this, SLOT( contentChanged() ) );
    connect( socksProxyHost, SIGNAL( textChanged(const QString&) ), this, SLOT( contentChanged() ) );
    connect( socksProxyPort, SIGNAL( textChanged(const QString&) ), this, SLOT( contentChanged() ) );
    connect( noProxy, SIGNAL( toggled(bool) ), this, SLOT( contentChanged() ) );
    connect( httpProxy, SIGNAL( toggled(bool) ), this, SLOT( contentChanged() ) );
    connect( socksProxy, SIGNAL( toggled(bool) ), this, SLOT( contentChanged() ) );
    connect( httpProxy, SIGNAL( stateChanged(int) ), this, SLOT( httpProxy_stateChanged(int) ) );
    connect( noProxy, SIGNAL( stateChanged(int) ), this, SLOT( noProxy_stateChanged(int) ) );
    connect( useAuthentication, SIGNAL( toggled(bool) ), userName, SLOT( setEnabled(bool) ) );
    connect( useAuthentication, SIGNAL( toggled(bool) ), passWord, SLOT( setEnabled(bool) ) );
    connect( useAuthentication, SIGNAL( toggled(bool) ), this, SLOT( contentChanged() ) );
}

/*
 *  Destroys the object and frees any allocated resources
 */
AGSyncConfig::~AGSyncConfig()
{
    // no need to delete child widgets, Qt does it all for us
}

/*
 *  Sets the strings of the subwidgets using the current
 *  language.
 */
void AGSyncConfig::languageChange()
{
    setCaption( tr2i18n( "AvantGo Synchronizer" ) );
    groupBox2->setTitle( tr2i18n( "HTTP Proxy" ) );
    textLabel3->setText( tr2i18n( "Host:" ) );
    textLabel4->setText( tr2i18n( "Port:" ) );
    useAuthentication->setText( tr2i18n( "Use Authentication" ) );
    _________________q->setText( tr2i18n( "Username:" ) );
    textLabel2->setText( tr2i18n( "Password:" ) );
    groupBox3->setTitle( tr2i18n( "Socks Proxy" ) );
    textLabel5->setText( tr2i18n( "Host:" ) );
    textLabel6->setText( tr2i18n( "Port:" ) );
    buttonGroup1->setTitle( tr2i18n( "Active Proxy" ) );
    noProxy->setText( tr2i18n( "No Proxy" ) );
    httpProxy->setText( tr2i18n( "HTTP Proxy" ) );
    socksProxy->setText( tr2i18n( "Socks Proxy" ) );
    buttonHelp->setText( tr2i18n( "&Help" ) );
    buttonHelp->setAccel( QKeySequence( tr2i18n( "F1" ) ) );
    buttonOk->setText( tr2i18n( "&OK" ) );
    buttonOk->setAccel( QKeySequence( QString::null ) );
    buttonCancel->setText( tr2i18n( "&Cancel" ) );
    buttonCancel->setAccel( QKeySequence( QString::null ) );
}

#include "agsyncconfig.moc"
