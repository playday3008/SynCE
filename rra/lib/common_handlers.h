/* $Id$ */
#ifndef __common_handlers_h__
#define __common_handlers_h__

#include <stdbool.h>
#include <libmimedir.h>

struct _CEPROPVAL;
struct _Generator;
struct _Parser;

bool on_propval_location   (struct _Generator* g, struct _CEPROPVAL* propval, void* cookie);
bool on_propval_notes      (struct _Generator* g, struct _CEPROPVAL* propval, void* cookie);
bool on_propval_sensitivity(struct _Generator* g, struct _CEPROPVAL* propval, void* cookie);
bool on_propval_subject    (struct _Generator* g, struct _CEPROPVAL* propval, void* cookie);

bool on_mdir_line_class   (struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_location(struct _Parser* p, mdir_line* line, void* cookie);
bool on_mdir_line_summary (struct _Parser* p, mdir_line* line, void* cookie);

#endif

