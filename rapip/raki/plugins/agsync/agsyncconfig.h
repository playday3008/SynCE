/****************************************************************************
** Form interface generated from reading ui file './agsyncconfig.ui'
**
** Created: Fri Oct 24 22:58:09 2003
**      by: The User Interface Compiler ($Id$)
**
** WARNING! All changes made in this file will be lost!
****************************************************************************/

#ifndef AGSYNCCONFIG_H
#define AGSYNCCONFIG_H

#include <qvariant.h>
#include <qdialog.h>

class QVBoxLayout;
class QHBoxLayout;
class QGridLayout;
class QGroupBox;
class QLabel;
class KLineEdit;
class QCheckBox;
class QButtonGroup;
class QRadioButton;
class QPushButton;

class AGSyncConfig : public QDialog
{
    Q_OBJECT

public:
    AGSyncConfig( QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~AGSyncConfig();

    QGroupBox* groupBox2;
    QLabel* textLabel3;
    QLabel* textLabel4;
    KLineEdit* httpProxyHost;
    KLineEdit* httpProxyPort;
    QCheckBox* useAuthentication;
    QLabel* _________________q;
    QLabel* textLabel2;
    KLineEdit* userName;
    KLineEdit* passWord;
    QGroupBox* groupBox3;
    QLabel* textLabel5;
    QLabel* textLabel6;
    KLineEdit* socksProxyHost;
    KLineEdit* socksProxyPort;
    QButtonGroup* buttonGroup1;
    QRadioButton* noProxy;
    QRadioButton* httpProxy;
    QRadioButton* socksProxy;
    QPushButton* buttonHelp;
    QPushButton* buttonOk;
    QPushButton* buttonCancel;

public slots:
    virtual void noProxy_stateChanged( int );
    virtual void httpProxy_stateChanged( int );
    virtual void socksProxy_stateChanged( int );
    virtual void contentChanged();

protected:
    QVBoxLayout* AGSyncConfigLayout;
    QHBoxLayout* layout19;
    QVBoxLayout* layout18;
    QVBoxLayout* groupBox2Layout;
    QHBoxLayout* layout8;
    QVBoxLayout* layout3;
    QVBoxLayout* layout4;
    QHBoxLayout* layout7;
    QVBoxLayout* layout1;
    QVBoxLayout* layout2;
    QHBoxLayout* groupBox3Layout;
    QVBoxLayout* layout6;
    QVBoxLayout* layout5;
    QVBoxLayout* buttonGroup1Layout;
    QHBoxLayout* Layout1;

protected slots:
    virtual void languageChange();

};

#endif // AGSYNCCONFIG_H
