/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/

void ConfigDialog::init()
{
    startDccm = true;
    usePassword = false;
    password = "";
    readConfig();
    updateFields();
}


void ConfigDialog::updateFields()
{
    startDccmCheckbox->setChecked(startDccm);
    passwordCheckbox->setChecked(usePassword);
    passwordEdit->setText(password);
    buttonApply->setDisabled(true);
}


bool ConfigDialog::getUsePassword()
{
    return usePassword;
}


bool ConfigDialog::getStartDccm()
{
    return startDccm;
}


QString ConfigDialog::getPassword()
{
    return password;
}


void ConfigDialog::applySlot()
{
    if (buttonApply->isEnabled()) {
        this->password = passwordEdit->text();
        startDccm = startDccmCheckbox->isChecked();
        usePassword = passwordCheckbox->isChecked();
        writeConfig();
        buttonApply->setDisabled(true);
        ((Raki *) parent())->restartDccm();
    }
}


void ConfigDialog::writeConfig()
{
    ksConfig->setGroup("DCCM");
    ksConfig->writeEntry("StartDccm", startDccm);
    ksConfig->writeEntry("UsePassword", usePassword);
    ksConfig->writeEntry("Password", password);
    ksConfig->sync();
}


void ConfigDialog::readConfig()
{
    ksConfig=kapp->config();
    ksConfig->setGroup("DCCM");
    startDccm = ksConfig->readBoolEntry("StartDccm");
    usePassword = ksConfig->readBoolEntry("UsePassword");
    password = ksConfig->readEntry("Password");
}


void ConfigDialog::okSlot()
{
    applySlot();
    accept();
}


void ConfigDialog::changedSlot()
{
    buttonApply->setEnabled(true);
}
