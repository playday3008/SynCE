/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _ERROREVENT_H_
#define _ERROREVENT_H_

#include <qevent.h>

/**
 * 
 * Volker Christian,,,
 **/
class ErrorEvent : public QCustomEvent
{

public:
  enum errorTypes {
    REMOTE_FILE_CREATE_ERROR = 1,
    REMOTE_FILE_WRITE_ERROR,
    LOCALE_FILE_OPEN_ERROR,
    LOCALE_FILE_READ_ERROR,
    REMOTE_FILE_EXECUTE_ERROR,
    NO_FILENAME_ERROR
  };

public:
  ErrorEvent(enum errorTypes errorType, void *data);
  ~ErrorEvent();
  int getErrorType();

private:
  int errorType;

};

#endif
