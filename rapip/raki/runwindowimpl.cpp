/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kpushbutton.h>
#include "runwindowimpl.h"


RunWindowImpl::RunWindowImpl(QWidget* parent, const char* name, bool modal, WFlags fl)
  : RunWindow(parent, name, modal, fl)
{
    cancel->setFocus();
}


RunWindowImpl::~RunWindowImpl()
{
}
