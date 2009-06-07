/*

    Support functions for FUR: special names fort /proc like functions
    FUR is Copyright (C) Riccardo Di Meo <riccardo@infis.univ.trieste.it>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.

    FUSE is Copyright of Miklos Szeredi <miklos@szeredi.hu>
*/
#include "special_names.h"
#include <string.h>


int proclen;
int powerlen;
int registrylen;
int locallen,alllen,rootlen,userlen;

int init_names(void)
{
  // proc directory name
  proclen=strlen(PROC);

  // Power status name
  powerlen=strlen(POWER_STATUS_NAME);

  // Registry container name
  registrylen=strlen(REGISTRY);

  // Main registry directory
  locallen=strlen(LOCAL);
  alllen=strlen(ALL);
  rootlen=strlen(ROOT);
  userlen=strlen(USER);

}

