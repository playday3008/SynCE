/* $Id$ */
#ifndef __rra_matchmaker_h__
#define __rra_matchmaker_h__

#include <synce.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* 
   Manage partnerships 
 */

typedef struct _RRA_Matchmaker RRA_Matchmaker;

/** Create a new matchmaker object for managing partnerships */
RRA_Matchmaker* rra_matchmaker_new();

/** Destroy the resources allocated by the matchmaker object */
void rra_matchmaker_destroy(RRA_Matchmaker* matchmaker);

/** Set the current partnership index (1 or 2) */
bool rra_matchmaker_set_current_partner(RRA_Matchmaker* matchmaker, uint32_t index);

/** Get the current partnership index (1 or 2) */
bool rra_matchmaker_get_current_partner(RRA_Matchmaker* matchmaker, uint32_t* index);

/** Get numeric ID for partnership 1 or 2 */
bool rra_matchmaker_get_partner_id(RRA_Matchmaker* matchmaker, uint32_t index, uint32_t* id);

/** Get computer name for partnership 1 or 2 */
bool rra_matchmaker_get_partner_name(RRA_Matchmaker* matchmaker, uint32_t index, char** name);

/** Generate a new partnership at empty index 1 or 2 */
bool rra_matchmaker_new_partnership(RRA_Matchmaker* matchmaker, uint32_t index);

/** Remove a partnership at index 1 or 2 */
bool rra_matchmaker_clear_partnership(RRA_Matchmaker* matchmaker, uint32_t index);

/** Do we have a valid partnership at index 1 or 2 */
bool rra_matchmaker_have_partnership_at(RRA_Matchmaker* matchmaker, uint32_t index);

/** Do we have a valid partnership */
bool rra_matchmaker_have_partnership(RRA_Matchmaker* matchmaker, uint32_t* index);

/** 
  If we don't have a partnership with this device:
    If there is an empty partnership slot:
      Create a partnership in empty slot
    Else
      Fail
 
  If we now have a partnership with this device:
    Set current partnership to our partnership with the device
 */ 

bool rra_matchmaker_create_partnership(RRA_Matchmaker*
      matchmaker, uint32_t* index);

#define rra_matchmaker_free_partner_name(name)  rapi_reg_free_string(name)

#ifdef __cplusplus
}
#endif



#endif
