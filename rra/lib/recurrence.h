/* $Id$ */
#ifndef __recurrence_h__
#define __recurrence_h__

#include <stdbool.h>
#include <libmimedir.h>

struct _Parser;

bool recurrence_parse_rrule(
    struct _Parser* p, 
    mdir_line* line, 
    mdir_line* dtstart,
    mdir_line* dtend);

#endif

