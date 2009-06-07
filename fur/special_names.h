/*

    Support functions for FUR: special names fort /proc like functions
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#ifndef SPECIAL_H
#define SPECIAL_H

// Fake proc filesystem
#define PROC "/proc"

// Power status function
#define POWER_STATUS_NAME "/power_status"

// Registry key collecting dir.
#define REGISTRY "/regkeys"

// Registry root

#define LOCAL "/HKEY_LOCAL_MACHINE"
#define ALL   "/HKEY_ALL_USERS"
#define ROOT  "/HKEY_CLASSES_ROOT"
#define USER  "/HKEY_CURRENT_USER"


// Length of previous strings: 
// can this be done with preprocessor??
extern int proclen;
extern int powerlen;
extern int registrylen;
extern int locallen,alllen,rootlen,userlen;

int init_names(void);

#endif
