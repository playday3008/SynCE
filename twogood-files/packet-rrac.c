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

/*
 * Recommended: to have "Allow subdissector to desegment TCP streams" turned on
 * in the TCP dissector options.
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
#include "packet-tcp.h"

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
static int hf_RRAC_chunk_size = -1;
static int hf_RRAC_chunk_flags = -1;
static int hf_RRAC_data = -1;

/* Initialize the subtree pointers */
static gint ett_RRAC = -1;

static gboolean is_rrac_command_stream(tvbuff_t *tvb, int offset)
{
  guint16 maybe_command = tvb_get_letohs(tvb, offset);

  if (maybe_command >= 0x65 && maybe_command <= 0x70)
    return TRUE;
  
#if 0
  if (tvb_get_letohl(tvb, offset) == 0xffffffff || tvb_length_remaining(tvb, offset) >= 14) {
    guint32 test_value = tvb_get_letohl(tvb, offset + 4);

    if (test_value >= 10000 && test_value <= 10050) {
      return TRUE;
    }
  }
#endif

  return FALSE;
}

static guint
get_rrac_pdu_len(tvbuff_t *tvb, int offset)
{
#if 0
  fprintf(stderr, "get_rrac_pdu_len: tvb_length_remaining = %04x\n", 
      tvb_length_remaining(tvb, offset));
#endif

  if (is_rrac_command_stream(tvb, offset))
    return 4 + tvb_get_letohs(tvb, offset + 2);
  else  
    return 12;
}


/* Code to actually dissect the packets */
static void
dissect_RRAC_pdu(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  int available_packet_data = 0;

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
      col_set_str(pinfo->cinfo, COL_INFO, "Request");
    }
    else {
      col_set_str(pinfo->cinfo, COL_INFO, "Reply");
    }
  }

/* In the interest of speed, if "tree" is NULL, avoid building a
   protocol tree and adding stuff to it if possible.  Note,
   however, that you must call subdissectors regardless of whether
   "tree" is NULL or not. */
  if (tree) {
    int offset = 0;

    while (tvb_length_remaining(tvb, offset) > 0) {
      const guint8 *data;

#if 0
      /* Need at least 4 for size */
      if (tvb_length_remaining(tvb, offset) < 4) {
        pinfo->desegment_offset = offset;
        pinfo->desegment_len = 4;
        return;
      }
#endif

      /* NOTE: The offset and length values in the call to
         "proto_tree_add_item()" define what data available_packet_data to highlight in the hex
         display window when the line in the protocol tree display
         corresponding to that item is selected.

         Supplying a length of -1 is the way to highlight all data from the
         offset to the end of the packet. */

#if 0
      if (test_value == 0)
      {
        int start_offset = offset;
        guint32 object_id;

        /* Assume data protocol */
        if (tvb_length_remaining(tvb, offset) < 12) {
          pinfo->desegment_offset = offset;
          pinfo->desegment_len = 12;
          return;
        }

        object_id = tvb_get_letohs(tvb, offset);

        proto_tree_add_item(RRAC_tree, hf_RRAC_object_id, tvb, offset, 4, TRUE);
        offset += 4;
        proto_tree_add_item(RRAC_tree, hf_RRAC_type_id, tvb, offset, 4, TRUE);
        offset += 4;
        proto_tree_add_item(RRAC_tree, hf_RRAC_data_flags, tvb, offset, 4, TRUE);
        offset += 4;

        if (object_id != 0xffffffff)
        {
          proto_tree_add_item(RRAC_tree, hf_RRAC_chunk_size, tvb, offset, 2, TRUE);
          offset += 2;
          proto_tree_add_item(RRAC_tree, hf_RRAC_chunk_flags, tvb, offset, 2, TRUE);
          offset += 2;

          available_packet_data = tvb_length_remaining(tvb, offset);
          if (available_packet_data > 0)
          {
            data = tvb_get_ptr(tvb, offset, available_packet_data);
            proto_tree_add_available_packet_data_format(RRAC_tree, hf_RRAC_data, tvb,
                offset,
                available_packet_data,
                data,
                "Data (%i available_packet_data)", available_packet_data);
            offset += available_packet_data;
          }
        }
      }
#endif

      if (is_rrac_command_stream(tvb, offset))
      {
        guint16 command = tvb_get_letohs(tvb, offset);
        guint16 total_packet_data = tvb_get_letohs(tvb, offset + 2);

        available_packet_data = tvb_length_remaining(tvb, offset + 4);

#if 0
        fprintf(stderr, 
            "c=%04x t=%04x a=%04x  do=%04x dl=%04x cd=%i\n", 
            command, total_packet_data, available_packet_data,
            pinfo->desegment_offset, pinfo->desegment_len, pinfo->can_desegment);
#endif

        /* create display subtree for the protocol */
        ti = proto_tree_add_item(tree, proto_RRAC, tvb, offset, total_packet_data + 4, FALSE);
        RRAC_tree = proto_item_add_subtree(ti, ett_RRAC);


#if 0
        proto_tree_add_text(RRAC_tree, tvb, offset, 0, "(DEBUG) can_desegment: %i", pinfo->can_desegment);
        proto_tree_add_text(RRAC_tree, tvb, offset, 0, "(DEBUG) desegment_offset: %04x", pinfo->desegment_offset);
        proto_tree_add_text(RRAC_tree, tvb, offset, 0, "(DEBUG) desegment_len: %04x", pinfo->desegment_len);
        proto_tree_add_text(RRAC_tree, tvb, offset, 4, "(DEBUG) Offset: %04x", offset);
        proto_tree_add_text(RRAC_tree, tvb, offset + 4, available_packet_data, "(DEBUG) Bytes: %04x", available_packet_data);
        /*proto_tree_add_text(RRAC_tree, tvb, offset, 2, "(DEBUG) My command: %04x", command);*/
        proto_tree_add_text(RRAC_tree, tvb, offset + 2, 2, "(DEBUG) My packet size: %04x", total_packet_data);
#endif

        proto_tree_add_item(RRAC_tree, hf_RRAC_command, tvb, offset,     2, TRUE);
        proto_tree_add_item(RRAC_tree, hf_RRAC_size,    tvb, offset + 2, 2, TRUE);

        if (available_packet_data < total_packet_data) {
          proto_tree_add_text(RRAC_tree, tvb, offset, -1, 
              "Part of RRAC packet: %i available of %i total", 
              available_packet_data, 4 + total_packet_data);

#if 0
          pinfo->desegment_offset = offset;
          pinfo->desegment_len = 4 + total_packet_data;
          return;
#endif
        }

        offset += 4;

        switch (command)
        {
          case 0x65:
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Object type: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Previous identifier: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "New identifier: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Flags: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            total_packet_data -= 4*4;
            break;
            
          case 0x66:
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Unknown: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Object type: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Identifier: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Unknown: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            total_packet_data -= 4*4;
            break;
            
          case 0x69:
            proto_tree_add_item(RRAC_tree, hf_RRAC_subcommand, tvb, offset, 4, TRUE);
            offset += 4;
            total_packet_data -= 4;

            switch (tvb_get_letohl(tvb, offset - 4))
            {
              case 0x02000000:
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Unknown: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Unknown: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Maybe size of remaining data: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Current partner index: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Partner 1 identifier: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Partner 2 identifier: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                total_packet_data -= 4*6;
                break;

              case 0:
              case 0x04000000:
              case 0x06000000:
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Object type: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Count: %08x", 
                    tvb_get_letohl(tvb, offset));
                offset += 4;

                {
                  guint size = tvb_get_letohl(tvb, offset);

                  proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Size: %08x", size);
                  offset += 4;
                  total_packet_data -= 4*3;

                  while (size >= 4)
                  {
                    proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Identifier: %08x", 
                        tvb_get_letohl(tvb, offset));
                    offset += 4;
                    total_packet_data -= 4;
                    size -= 4;
                  }
                }
                break;
            }
            break;

          case 0x6c:
            proto_tree_add_item(RRAC_tree, hf_RRAC_reply_to, tvb, offset, 4, TRUE);
            offset += 4;
            total_packet_data -= 4;
            break;

          case 0x6f:
            proto_tree_add_item(RRAC_tree, hf_RRAC_subcommand, tvb, offset, 4, TRUE);
            offset += 4;
            total_packet_data -= 4;
            break;

          case 0x70:
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Size of remaining packet: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Unknown: %08x", 
                tvb_get_letohl(tvb, offset));
            offset += 4;
            
            {
              guint subcommand = tvb_get_letohl(tvb, offset);
              proto_tree_add_item(RRAC_tree, hf_RRAC_subcommand, tvb, offset, 4, TRUE);
              offset += 4;
              total_packet_data -= 4*3;

              if (subcommand == 3) {
                unsigned i;
                guint count;

                for (i = 0; i < 4; i++) {
                  proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Unknown: %08x", 
                      tvb_get_letohl(tvb, offset));
                  offset += 4;
                  total_packet_data -= 4;
                }

                count = tvb_get_letohl(tvb, offset);
                proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Count: %08x", count);
                offset += 4;
                total_packet_data -= 4;

                for (i = 0; i < count; i++) {
                  proto_tree_add_text(RRAC_tree, tvb, offset, 4, "Identifier: %08x", 
                      tvb_get_letohl(tvb, offset));
                  offset += 4;
                  total_packet_data -= 4;
                }
              } /* subcommand == 3 */  
            }
            break;
        }

        if (total_packet_data > 0)
        {
          data = tvb_get_ptr(tvb, offset, total_packet_data);
          proto_tree_add_bytes_format(RRAC_tree, hf_RRAC_data, tvb,
              offset,
              total_packet_data,
              data,
              "Data (%i bytes)", total_packet_data);
          offset += total_packet_data;
        }
      }

      /* XXX only show the first command */
      break;
    } /* while */
  } /* if (tree) */

  /* If this protocol has a sub-dissector call it here, see section 1.8 */
}

/* Code to actually dissect the packets */
static void
dissect_RRAC(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
  	tcp_dissect_pdus(tvb, pinfo, tree, TRUE, 4,
		    get_rrac_pdu_len, dissect_RRAC_pdu);
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
    { &hf_RRAC_chunk_size,
      { "Data chunk size", "rrac.chunk_size", FT_UINT16, BASE_HEX, NULL, 0, "Data chunk size", HFILL }
    },
    { &hf_RRAC_chunk_flags,
      { "Data chunk flags", "rrac.chunk_flags", FT_UINT16, BASE_HEX, NULL, 0, "Data chunk flags", HFILL }
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


