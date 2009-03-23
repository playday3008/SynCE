/*
 * OpenSync SynCE plugin
 *
 * Copyright © 2005 by MirKuZ
 * Copyright © 2005 Danny Backx <dannybackx@users.sourceforge.net>
 * Copyright © 2008 Mark Ellis <mark_ellis@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

#ifndef	_OPENSYNC_SYNCE_PLUGIN_H_
#define	_OPENSYNC_SYNCE_PLUGIN_H_

#include <rra/syncmgr.h>
#include <rra/timezone.h>

#include <glib.h>

enum {
        TYPE_INDEX_CONTACT,
        TYPE_INDEX_TODO,
        TYPE_INDEX_CALENDAR,
        TYPE_INDEX_MAX
};

typedef struct SyncePluginPtr {
	OSyncMember	*member;
	OSyncHashTable	*hashtable;	/* Need a hash for the file sync part. */

	RRA_SyncMgr*	syncmgr;	/* This is the connection to SynCE */
	RRA_Timezone	timezone;
	char *codepage;
        uint32_t	type_ids[3];
	int		last_change_counter;
	int		change_counter;

        GHashTable *objects[3];

	/* Configuration */
	osync_bool	config_types[3];
	char		*config_file;
} SyncePluginPtr;

extern osync_bool synce_parse_settings(SyncePluginPtr *env, char *data, int size, OSyncError **error);

#endif
