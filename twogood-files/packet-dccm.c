/* packet-dccm.c
 * Routines for Windows CE Direct Cable Connection Manager (DCCM) dissection
 * Copyright 2004, David Eriksson <twogood@users.sourceforge.net>
 *
 * $Id$
 *
 * Ethereal - Network traffic analyzer
 * By Gerald Combs <gerald@ethereal.com>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#ifdef NEED_SNPRINTF_H
# include "snprintf.h"
#endif

#include <epan/packet.h>
/* #include "packet-DCCM.h" */

#define DCCM_TCP_PORT 5679

/* Initialize the protocol and registered fields */
static int proto_DCCM = -1;
static int hf_DCCM_data = -1;

/* Initialize the subtree pointers */
static gint ett_DCCM = -1;

/* Code to actually dissect the packets */
static void
dissect_DCCM(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  int bytes;

/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *DCCM_tree;

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "DCCM");
    
/* This field shows up as the "Info" column in the display; you should make
   it, if possible, summarize what's in the packet, so that a user looking
   at the list of packets can tell what type of packet it is. See section 1.5
   for more information.

   If you are setting it to a constant string, use "col_set_str()", as
   it's more efficient than the other "col_set_XXX()" calls.

   If you're setting it to a string you've constructed, or will be
   appending to the column later, use "col_add_str()".

   "col_add_fstr()" can be used instead of "col_add_str()"; it takes
   "printf()"-like arguments.  Don't use "col_add_fstr()" with a format
   string of "%s" - just use "col_add_str()" or "col_set_str()", as it's
   more efficient than "col_add_fstr()".

   If you will be fetching any data from the packet before filling in
   the Info column, clear that column first, in case the calls to fetch
   data from the packet throw an exception because they're fetching data
   past the end of the packet, so that the Info column doesn't have data
   left over from the previous dissector; do

	if (check_col(pinfo->cinfo, COL_INFO)) 
		col_clear(pinfo->cinfo, COL_INFO);

   */

  if (check_col(pinfo->cinfo, COL_INFO))  {
    if (pinfo->srcport == DCCM_TCP_PORT) {
      col_set_str(pinfo->cinfo, COL_INFO, "(reply)");
    }
    else {
      col_set_str(pinfo->cinfo, COL_INFO, "(request)");
    }
  }

/* In the interest of speed, if "tree" is NULL, avoid building a
   protocol tree and adding stuff to it if possible.  Note,
   however, that you must call subdissectors regardless of whether
   "tree" is NULL or not. */
  /*if (tree)*/ {
    int offset = 0;
    const guint8 *data;

      /* NOTE: The offset and length values in the call to
         "proto_tree_add_item()" define what data bytes to highlight in the hex
         display window when the line in the protocol tree display
         corresponding to that item is selected.

         Supplying a length of -1 is the way to highlight all data from the
         offset to the end of the packet. */

    bytes = tvb_length_remaining(tvb, offset);

      /* create display subtree for the protocol */
      ti = proto_tree_add_item(tree, proto_DCCM, tvb, offset, bytes, FALSE);

      DCCM_tree = proto_item_add_subtree(ti, ett_DCCM);

      if (bytes == 4)
      {
        guint32 command = tvb_get_letohl(tvb, offset);

        switch (command)
        {
          case 0:
            proto_tree_add_text(DCCM_tree, tvb, offset, 4, "Initialize connection without password");
            break;

          case 0x12345678:
            /* Ping sent from PC to Windows CE and Pong the other way around */
            if (pinfo->srcport == DCCM_TCP_PORT) {
              proto_tree_add_text(DCCM_tree, tvb, offset, 4, "Ping");
              if (check_col(pinfo->cinfo, COL_INFO)) 
                col_set_str(pinfo->cinfo, COL_INFO, "Ping");
            }
            else {
              proto_tree_add_text(DCCM_tree, tvb, offset, 4, "Pong");
              if (check_col(pinfo->cinfo, COL_INFO)) 
                col_set_str(pinfo->cinfo, COL_INFO, "Pong");
            }
            break;

          default:
            proto_tree_add_text(DCCM_tree, tvb, offset, 4, "Password challenge. Key = 0x%02x", 
                command & 0xff);
            break;
        }
      }
      else
      {
        data = tvb_get_ptr(tvb, offset, bytes);
        proto_tree_add_bytes_format(DCCM_tree, hf_DCCM_data, tvb,
            offset,
            bytes,
            data,
            "Data (%i bytes)", bytes);
      }
  } /* if (tree) */

  /* If this protocol has a sub-dissector call it here, see section 1.8 */
}


/* Register the protocol with Ethereal */

/* this format is require because a script is used to build the C function
   that calls all the protocol registration.
*/

void
proto_register_DCCM(void)
{                 

/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
#if 0
		{ &hf_DCCM_size,
			{ "Size", "dccm.size", FT_UINT32, BASE_HEX, NULL, 0, "Size of remaining packet", HFILL }
    },
		{ &hf_DCCM_command,
			{ "Command", "dccm.command", FT_UINT32, BASE_HEX, NULL, 0, "Command code", HFILL }
		},
		{ &hf_DCCM_protocol_error_flag,
			{ "Protocol error flag", "dccm.protocol_error_flag", FT_UINT32, BASE_HEX, NULL, 0, "Protocol error flag", HFILL }
		},
		{ &hf_DCCM_protocol_error_code,
			{ "Protocol error code", "dccm.protocol_error_code", FT_UINT32, BASE_HEX, NULL, 0, "Protocol error code (HRESULT)", HFILL }
		},
#endif
		{ &hf_DCCM_data,
			{ "Data", "dccm.data", FT_BYTES, BASE_HEX, NULL, 0, "Data", HFILL }
		}
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_DCCM,
	};

/* Register the protocol name and description */
	proto_DCCM = proto_register_protocol("Windows CE Direct Cable Connection Manager",
	    "DCCM", "DCCM");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_DCCM, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_DCCM(void)
{
	dissector_handle_t DCCM_handle;

	DCCM_handle = create_dissector_handle(dissect_DCCM,
	    proto_DCCM);
	dissector_add("tcp.port", DCCM_TCP_PORT, DCCM_handle);
}


