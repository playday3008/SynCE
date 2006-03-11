/* $Id$ */
#ifndef __recurrence_h__
#define __recurrence_h__

#include <stdbool.h>
#include <libmimedir.h>
#include "mdir_line_vector.h"
#include "timezone.h"

struct _CEPROPVAL;
struct _Generator;
struct _Parser;

bool recurrence_generate_rrule(
    struct _Generator* g, 
    struct _CEPROPVAL* propval);

bool recurrence_parse_rrule(
    struct _Parser* p, 
    mdir_line* dtstart,
    mdir_line* dtend,
    mdir_line* rrule, 
    RRA_MdirLineVector* exdates);

#endif

