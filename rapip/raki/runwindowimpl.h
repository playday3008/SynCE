/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _RUNWINDOWIMPL_H_
#define _RUNWINDOWIMPL_H_

#include "runwindow.h"

/**
 * 
 * Volker Christian,,,
 **/
class RunWindowImpl : public RunWindow
{

public:
  RunWindowImpl(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);
  ~RunWindowImpl();
};

#endif
