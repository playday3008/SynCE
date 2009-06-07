/*

    Useful macros for FUR.
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef MACROS_H
#define MACROS_H


#ifdef VERBOSE
#include <stdio.h>

extern FILE *logfile;
#endif


// FIXME put __LINE__ and __FILE__ in a decent way
#ifdef VERBOSE
#define VERB(...) {fprintf(logfile,__VA_ARGS__);fflush(logfile);}
#else
#define VERB(...) 
#endif

#define ERR(...) {fprintf(stderr,"ERROR: "__VA_ARGS__);fflush(stderr);}


#ifdef VERBOSE_CACHE
#define CVERB(...) {fprintf(logfile,__VA_ARGS__);fflush(logfile);}
#define CPRT(INDEX) {print_cache_entry(INDEX);fflush(logfile);}
#else
#define CVERB(...)
#define CPRT(INDEX)
#endif

#endif
