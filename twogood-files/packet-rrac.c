/* packet-rrac.c
 * Routines for Windows CE Remote Replication Agent Connection (RRAC) dissection
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
/* #include "packet-RRAC.h" */

#define RRAC_TCP_PORT 5678

/* Initialize the protocol and registered fields */
static int proto_RRAC = -1;
static int hf_RRAC_command = -1;
static int hf_RRAC_size = -1;
static int hf_RRAC_reply_to = -1;
static int hf_RRAC_subcommand = -1; 
static int hf_RRAC_object_id = -1;
static int hf_RRAC_type_id = -1;
static int hf_RRAC_data_flags = -1;
static int hf_RRAC_data = -1;

/* Initialize the subtree pointers */
static gint ett_RRAC = -1;


/* Code to actually dissect the packets */
static void
dissect_RRAC(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  int bytes;

/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *RRAC_tree;

/* Make entries in Protocol column and Info column on summary display */
	if (check_col(pinfo->cinfo, COL_PROTOCOL)) 
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "RRAC");
    
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
    if (pinfo->srcport == RRAC_TCP_PORT) {
      col_set_str(pinfo->cinfo, COL_INFO, "(request)");
    }
    else {
      col_set_str(pinfo->cinfo, COL_INFO, "(reply)");
    }
  }

/* In the interest of speed, if "tree" is NULL, avoid building a
   protocol tree and adding stuff to it if possible.  Note,
   however, that you must call subdissectors regardless of whether
   "tree" is NULL or not. */
  if (tree) {
    int offset = 0;
    const guint8 *data;
    guint16 test_value;

    /* Need at least 4 for size */
    if (tvb_length_remaining(tvb, offset) < 4) {
      pinfo->desegment_offset = offset;
      pinfo->desegment_len = 4;
      return;
    }


    /* NOTE: The offset and length values in the call to
       "proto_tree_add_item()" define what data bytes to highlight in the hex
       display window when the line in the protocol tree display
       corresponding to that item is selected.

       Supplying a length of -1 is the way to highlight all data from the
       offset to the end of the packet. */

    bytes = tvb_length_remaining(tvb, offset);

    /* create display subtree for the protocol */
    ti = proto_tree_add_item(tree, proto_RRAC, tvb, offset, bytes, FALSE);

    RRAC_tree = proto_item_add_subtree(ti, ett_RRAC);

    test_value = tvb_get_letohs(tvb, offset + 2);

    if (test_value == 0)
    {
      /* Assume data protocol */
      if (tvb_length_remaining(tvb, offset) < 12) {
        pinfo->desegment_offset = offset;
        pinfo->desegment_len = 12;
        return;
      }

      proto_tree_add_item(RRAC_tree, hf_RRAC_object_id, tvb, offset, 4, TRUE);
      offset += 4;
      proto_tree_add_item(RRAC_tree, hf_RRAC_type_id, tvb, offset, 4, TRUE);
      offset += 4;
      proto_tree_add_item(RRAC_tree, hf_RRAC_data_flags, tvb, offset, 4, TRUE);
      offset += 4;
    }
    else
    {
      /* Assume command protocol */
      guint16 command = tvb_get_letohs(tvb, offset);
      guint16 packet_size = tvb_get_letohs(tvb, offset + 2);
      /*proto_item *command_item =*/ proto_tree_add_item(RRAC_tree,
          hf_RRAC_command, tvb, offset, 2, TRUE);
      proto_tree_add_item(RRAC_tree, hf_RRAC_size, tvb, offset + 2, 2, TRUE);

      if (tvb_length_remaining(tvb, offset) < packet_size) {
        pinfo->desegment_offset = offset;
        pinfo->desegment_len = 4 + packet_size;
        return;
      }

      offset += 4;

      switch (command)
      {
        case 0x6c:
          proto_tree_add_item(RRAC_tree, hf_RRAC_reply_to, tvb, offset, 4, TRUE);
          offset += 4;
          break;

        case 0x6f:
          proto_tree_add_item(RRAC_tree, hf_RRAC_subcommand, tvb, offset, 4, TRUE);
          offset += 4;
          break;
      }
    }

    bytes = tvb_length_remaining(tvb, offset);
    if (bytes > 0)
    {
      data = tvb_get_ptr(tvb, offset, bytes);
      proto_tree_add_bytes_format(RRAC_tree, hf_RRAC_data, tvb,
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
proto_register_RRAC(void)
{                 

/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
    { &hf_RRAC_command,
      { "Command", "rrac.command", FT_UINT16, BASE_HEX, NULL, 0, "Command code", HFILL }
    },
    { &hf_RRAC_subcommand,
      { "Subcommand", "rrac.subcommand", FT_UINT32, BASE_HEX, NULL, 0, "Subcommand code", HFILL }
    },
    { &hf_RRAC_size,
      { "Size", "rrac.size", FT_UINT16, BASE_HEX, NULL, 0, "Size of remaining packet", HFILL }
    },
    { &hf_RRAC_reply_to,
      { "Reply to", "rrac.reply_to", FT_UINT32, BASE_HEX, NULL, 0, "Reply to command", HFILL }
    },
    { &hf_RRAC_object_id,
      { "Object ID", "rrac.object_id", FT_UINT32, BASE_HEX, NULL, 0, "Object identifier", HFILL }
    },
    { &hf_RRAC_type_id,
      { "Type ID", "rrac.type_id", FT_UINT32, BASE_HEX, NULL, 0, "Type identifier", HFILL }
    },
    { &hf_RRAC_data_flags,
      { "Data flags", "rrac.data_flags", FT_UINT32, BASE_HEX, NULL, 0, "Data flags", HFILL }
    },
#if 0
		{ &hf_RRAC_protocol_error_flag,
			{ "Protocol error flag", "rrac.protocol_error_flag", FT_UINT32, BASE_HEX, NULL, 0, "Protocol error flag", HFILL }
		},
		{ &hf_RRAC_protocol_error_code,
			{ "Protocol error code", "rrac.protocol_error_code", FT_UINT32, BASE_HEX, NULL, 0, "Protocol error code (HRESULT)", HFILL }
		},
#endif
		{ &hf_RRAC_data,
			{ "Data", "rrac.data", FT_BYTES, BASE_HEX, NULL, 0, "Data", HFILL }
		}
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_RRAC,
	};

/* Register the protocol name and description */
	proto_RRAC = proto_register_protocol("Windows CE Remote Replication Agent Connection",
	    "RRAC", "RRAC");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_RRAC, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
}


/* If this dissector uses sub-dissector registration add a registration routine.
   This format is required because a script is used to find these routines and
   create the code that calls these routines.
*/
void
proto_reg_handoff_RRAC(void)
{
	dissector_handle_t RRAC_handle;

	RRAC_handle = create_dissector_handle(dissect_RRAC,
	    proto_RRAC);
	dissector_add("tcp.port", RRAC_TCP_PORT, RRAC_handle);
}


