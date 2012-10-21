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
#include <opensync/opensync.h>

#include <rra/appointment.h>
#include <rra/contact.h>
#include <rra/task.h>
#include <rra/matchmaker.h>
#include <rra/uint32vector.h>
#include <rra/syncmgr.h>
#include <rapi.h>

#include "synce_plugin.h"
#include "synce_file.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <glib.h>

typedef struct
{
        const char* osync_name;
        const char* synce_name;
} typename_conv;

static typename_conv typenames[] = 
{
        { "contact", RRA_SYNCMGR_TYPE_CONTACT },
        { "todo", RRA_SYNCMGR_TYPE_TASK },
        { "event", RRA_SYNCMGR_TYPE_APPOINTMENT }
};

typedef struct _SynceObject
{
        int type_index;
        uint32_t type_id;
        uint32_t object_id;
        RRA_SyncMgrTypeEvent event;
        int change_counter;
} SynceObject;

/*
 * opensync interface function
 */
static void *
initialize(OSyncMember *member, OSyncError **error)
{
	char *configdata;
	int configsize;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	//You need to specify the <some name>_environment somewhere with
	//all the members you need
	SyncePluginPtr *env = g_malloc0(sizeof(SyncePluginPtr));

	/* File sync needs a hash table */
	env->hashtable = osync_hashtable_new();

	//now you can get the config file for this plugin
	if (!osync_member_get_config(member, &configdata, &configsize, error)) {
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		free(env);
		return NULL;
	}
	
	if (!synce_parse_settings(env, configdata, configsize, error)) {
		g_free(env);
		return NULL;
	}

	//Process the configdata here and set the options on your environment
	free(configdata);
	env->member = member;

	//Now your return your struct.
	return (void *)env;
}

/*
 * opensync interface function
 */
static void
connect(OSyncContext *ctx)
{
	RRA_Matchmaker* matchmaker = NULL;
	HRESULT hr;
        const RRA_SyncMgrType* type = NULL;
        int i;
        LONG result;
        HKEY intl_key;
        DWORD codepage;
        DWORD reg_type;
        DWORD size;
        WCHAR *wstr_tmp = NULL;
        char *iconv_codepage = NULL;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	 
	//Initializing RAPI
	hr = CeRapiInit();
  	if (FAILED(hr))
  	{
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "initializing RAPI");
		return;
        }

        wstr_tmp = wstr_from_utf8("\\Software\\Microsoft\\International");
        result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, wstr_tmp, 0, 0, &intl_key);
        wstr_free_string(wstr_tmp);
        if (result != ERROR_SUCCESS) {
                osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "CeRegOpenKeyEx failed getting device codepage: %s", synce_strerror(result));
                return;
        }

        wstr_tmp = wstr_from_utf8("ACP");
        size = sizeof(codepage);
        result = CeRegQueryValueEx(intl_key, wstr_tmp, 0, &reg_type, (LPBYTE)&codepage, &size);
        wstr_free_string(wstr_tmp);

        CeRegCloseKey(intl_key);

        if (result != ERROR_SUCCESS) {
                osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "CeRegQueryValueEx failed getting device codepage: %s", synce_strerror(result));
                return;
        }

        if ((reg_type != REG_DWORD) || (size != sizeof(DWORD))) {
                osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Unexpected value type for device codepage: 0x%08x = %i: size = %d", reg_type, reg_type, size);
                return;
        }

        iconv_codepage = malloc(16);
        snprintf(iconv_codepage, 16, "CP%d", codepage);

        env->codepage = iconv_codepage;

	//1 - creating matchmaker
	matchmaker = rra_matchmaker_new(NULL);
	if (!matchmaker){
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "building matchmaker");
		return;
	}
	osync_debug("SynCE-SYNC", 4, "matchmaker built");

        /* select correct partnership here*/

	//2 - setting partnership 
	if (!rra_matchmaker_set_current_partner(matchmaker, 1)){
		osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "set current partner");
		goto exit;
	}
	osync_debug("SynCE-SYNC", 4, "partner set");

        //3 -setting timezone
        if (!rra_timezone_get(&(env->timezone), NULL)){
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "getting timezone");
		goto exit;
    	}
       	osync_debug("SynCE-SYNC", 4, "timezone set");
	
	//4- creating syncmgr
	env->syncmgr = rra_syncmgr_new();

	if (!rra_syncmgr_connect(env->syncmgr, NULL))
	{
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "can't connect");
		goto exit;
	}
    	osync_debug("SynCE-SYNC", 4, "syncmgr created");

	/*
	 * if (!osync_member_objtype_enabled(env->member, "contact"))
	 * 	env->config_contacts = FALSE;
	 * if (!osync_member_objtype_enabled(env->member, "todos"))
	 * 	env->config_todos = FALSE;
	 * if (!osync_member_objtype_enabled(env->member, "calendar"))
	 * 	env->config_calendar = FALSE;
	 */
	if (env->config_file) {
		/* Load the hash table */
		OSyncError *error = NULL;

		if (!osync_hashtable_load(env->hashtable, env->member, &error)) {
			osync_context_report_osyncerror(ctx, &error);
			goto exit;
		}
	}

        for (i = 0; i < TYPE_INDEX_MAX; i++) {
                env->type_ids[i] = 0;
                env->objects[i] = NULL;
                if (env->config_types[i] == TRUE) {
                        type = rra_syncmgr_type_from_name(env->syncmgr, typenames[i].synce_name);
                        if (type) {
                                env->type_ids[i] = type->id;
                                env->objects[i] = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, g_free);
                        }
                }
        }

	osync_context_report_success(ctx);

exit:
        if (matchmaker)
                rra_matchmaker_destroy(matchmaker);
        return;
}

/*
 * callback used by RRA Sync Manager when events are received
 */
static bool
callback (RRA_SyncMgrTypeEvent event, uint32_t type, uint32_t count, uint32_t* ids, void* cookie)
{
	const char* event_str;
        int index, i;
        SynceObject* object = NULL;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);

        SyncePluginPtr *env = (SyncePluginPtr*)cookie;

        /* Find object type */
        for (index = 0; index < TYPE_INDEX_MAX; index++)
                if (type == env->type_ids[index])
                        break;

        if (TYPE_INDEX_MAX == index) {
                /* Unknown object type */
                return false;
        }

        /* Update the apropriate hashtable */
        for (i = 0; i < count; i++) {

                /* Create new object and add to hash table */
                object = g_new0(SynceObject, 1);

                object->type_index  = index;
                object->type_id     = type;
                object->object_id   = ids[i];
                object->event       = event;
                object->change_counter = 0;

                g_hash_table_insert(env->objects[index],
                                    &object->object_id,
                                    object);
        }

        switch (event)
  	{
    	case SYNCMGR_TYPE_EVENT_UNCHANGED:
		event_str = "%i Unchanged";
      		break;
	case SYNCMGR_TYPE_EVENT_CHANGED:
		event_str = "%i Changed";
		break;
	case SYNCMGR_TYPE_EVENT_DELETED:
		event_str = "%i Deleted";
		break;
	default:
      		event_str = "%i Unknown";
		break;
	}

	osync_debug("SynCE-SYNC", 4, event_str, count);
	return true;
}

/*
 * report_changes: get data for the objects changed, and report them to opensync engine
 */
static void
report_changes(gpointer id, gpointer value, gpointer user_data)
{
        OSyncContext *ctx = (OSyncContext*)user_data;
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
        SynceObject *object = (SynceObject*)value;

        char* out_data = NULL;
        uint8_t* data = NULL;
        char strid[10];
        size_t data_size = 0;
        bool result = FALSE;
        OSyncChangeType change_type;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);

        switch(object->event) {
        case SYNCMGR_TYPE_EVENT_UNCHANGED:
                switch (object->type_index) {
                case TYPE_INDEX_CONTACT:
                        if (!osync_member_get_slow_sync(env->member, "contact"))
                                return;
                        break;

                case TYPE_INDEX_TODO:
                        if (!osync_member_get_slow_sync(env->member, "todo"))
                                return;
                        break;

                case TYPE_INDEX_CALENDAR:
                        if (!osync_member_get_slow_sync(env->member, "event"))
                                return;
                        break;
                }
                change_type = CHANGE_ADDED;
                break;
        case SYNCMGR_TYPE_EVENT_CHANGED:
                change_type = CHANGE_MODIFIED;
                break;
        case SYNCMGR_TYPE_EVENT_DELETED:
                change_type = CHANGE_DELETED;
                break;
        default:
                osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "unknown event tyoe from object %i. id=%i", object->object_id, object->event);
                return;
        }

        if (!rra_syncmgr_get_single_object(env->syncmgr, object->type_id, object->object_id, &data, &data_size)){
                osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "get_single_object fail. id=%i", object->object_id);
                return;
        }
		
        sprintf(strid,"%08x", object->object_id);

        osync_debug("SynCE-SYNC", 4, "got object type: %s ids: %08x data_size: %i", typenames[object->type_index].osync_name, object->object_id, data_size);

        switch (object->type_index) {
        case TYPE_INDEX_CONTACT:
                result = rra_contact_to_vcard(RRA_CONTACT_ID_UNKNOWN,data,data_size,&out_data,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0, env->codepage);
                if (!result)
                        break;

                osync_trace(TRACE_INTERNAL, "Generated vcard: %s", osync_print_binary((unsigned char *)out_data, strlen(out_data)));
                break;

        case TYPE_INDEX_TODO:
                result = rra_task_to_vtodo(RRA_TASK_ID_UNKNOWN,data,data_size,&out_data,RRA_TASK_UTF8,&env->timezone, env->codepage);
                if (!result)
                        break;

                osync_trace(TRACE_INTERNAL, "Generated vtodo: %s", osync_print_binary((unsigned char *)out_data, strlen(out_data)));
                break;

        case TYPE_INDEX_CALENDAR:
                result = rra_appointment_to_vevent(RRA_APPOINTMENT_ID_UNKNOWN,data,data_size,&out_data,RRA_APPOINTMENT_UTF8,&env->timezone, env->codepage);
                if (!result)
                        break;

                osync_trace(TRACE_INTERNAL, "Generated vevent: %s", osync_print_binary((unsigned char *)out_data, strlen(out_data)));
                break;
        }
		
        if (!result) {
                osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "conversion failed. id=%i",object->object_id);
                return;
        }

        OSyncChange *change = osync_change_new();
        osync_change_set_member(change, env->member);
	
        osync_change_set_uid(change, strid); // %08x

        switch (object->type_index) {
        case TYPE_INDEX_CONTACT:
                osync_change_set_objformat_string(change, "vcard30");
                break;
        case TYPE_INDEX_TODO:
                osync_change_set_objformat_string(change, "vtodo10");
                break;
        case TYPE_INDEX_CALENDAR:
                osync_change_set_objformat_string(change, "vevent10");
                break;
        }

        osync_change_set_data(change, out_data, strlen(out_data) + 1, TRUE);

        osync_change_set_changetype(change, change_type);

        osync_context_report_change(ctx, change);

        object->change_counter = 1;
	return;
}


/*
 get changes from device, and report them via report_changes
*/
static bool
subscribe(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	bool got_event = false;
        int i;
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	

        for (i = 0; i < TYPE_INDEX_MAX; i++)
                if (env->config_types[i]) {
                        osync_debug("SynCE-SYNC", 4, "checking %ss", typenames[i].osync_name);
                        rra_syncmgr_subscribe(env->syncmgr, env->type_ids[i], callback, env);
                }

	if (!rra_syncmgr_start_events(env->syncmgr))  {
		osync_context_report_error(ctx, OSYNC_ERROR_IO_ERROR, "can't start events");
		return FALSE;
	}
	
	osync_debug("SynCE-SYNC", 4, "event started");

	//waiting for events
	while (rra_syncmgr_event_wait(env->syncmgr, 3, &got_event) && got_event) {
		osync_debug("SynCE-SYNC", 4, "*event received, processing");
		rra_syncmgr_handle_event(env->syncmgr);
	}

	rra_syncmgr_handle_all_pending_events(env->syncmgr);

	osync_debug("SynCE-SYNC", 4, "finished receiving events");
	
        for (i = 0; i < TYPE_INDEX_MAX; i++)
                if (env->config_types[i])
                        rra_syncmgr_unsubscribe(env->syncmgr, env->type_ids[i]);

	//report changes
	osync_debug("SynCE-SYNC", 4, "report changes");

        for (i = 0; i < TYPE_INDEX_MAX; i++)
                if (env->config_types[i])
                        g_hash_table_foreach(env->objects[i], report_changes, ctx);

	osync_debug("SynCE-SYNC", 4, "done reporting changes");
	
	return true;
}

/*
 * opensync interface function: report the changes to the opensync engine
 */
static void
get_changeinfo(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);
	osync_debug("SynCE-SYNC", 4,
			"Get_ChangeInfo(todos %d contacts %d calendar %d files(%s)\n",
			env->config_types[TYPE_INDEX_TODO], env->config_types[TYPE_INDEX_CONTACT],
                        env->config_types[TYPE_INDEX_CALENDAR], env->config_file);

	//test RRA connection
	osync_debug("SynCE-SYNC", 4, "Testing connection");
	if (!env->syncmgr || !rra_syncmgr_is_connected(env->syncmgr)){
		//not connected, exit
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "not connected to device, exit.");
		return;
	}
	osync_debug("SynCE-SYNC", 4, "Testing connection -> ok");

        if (!subscribe(ctx))
                return;

        //need to reinit the connection
        rra_syncmgr_disconnect(env->syncmgr);
		
        if (!rra_syncmgr_connect(env->syncmgr, NULL)) {
                osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "can't connect");
                return;
        }

	if (env->config_file) {
		osync_debug("SynCE-SYNC", 4, "checking files to synchronize");

		if (! synce_file_get_changeinfo(ctx)) {
			osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Error while checking files");
			return;
		}
		
		//need to reinit the connection
		rra_syncmgr_disconnect(env->syncmgr);
	
		if (!rra_syncmgr_connect(env->syncmgr, NULL))
		{
			osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "can't connect");
			return;
		}
	}
	
	//All Right
	osync_context_report_success(ctx);
}

/*
commit_contacts_change: called when it's time to update device once a time for every update
*/
static osync_bool commit_contacts_change(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	const RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "contact");

        id=strtol(osync_change_get_uid(change),NULL,16);
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			osync_debug("SynCE-SYNC", 4, "deleting contact id: %08x",id);
		
			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't delete contact id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id;
			
			object=osync_change_get_data(change);

			osync_debug("SynCE-SYNC", 4, "adding contact id %08x",id);

			if (!rra_contact_from_vcard(object,&dummy_id,&data,&data_size,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0, env->codepage)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Failed to convert contact id: %08x",id);
				return FALSE;
			}

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't add contact id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id;
			
			object=osync_change_get_data(change);

			osync_debug("SynCE-SYNC", 4, "updating contact id %08x",id);

			if (!rra_contact_from_vcard(object,&dummy_id,&data,&data_size,RRA_CONTACT_UTF8 | RRA_CONTACT_VERSION_3_0, env->codepage)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Failed to convert contact id: %08x",id);
				return FALSE;
			}

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't update contact id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SynCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);

	return TRUE;
}

static osync_bool commit_todo_change(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	const RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "task");

        id=strtol(osync_change_get_uid(change),NULL,16);
	
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			osync_debug("SynCE-SYNC", 4, "deleting task id: %08x",id);
		
			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't delete task id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id;
			
			object=osync_change_get_data(change);

			osync_debug("SynCE-SYNC", 4, "adding task id %08x",id);

			if (!rra_task_from_vtodo(object,&dummy_id,&data,&data_size,RRA_TASK_UTF8,&env->timezone, env->codepage)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Failed to convert task id: %08x",id);
				return FALSE;
			}

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't add task id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id;

			object=osync_change_get_data(change);

			osync_debug("SynCE-SYNC", 4, "updating task id %08x",id);

			if (!rra_task_from_vtodo(object,&dummy_id,&data,&data_size,RRA_TASK_UTF8,&env->timezone, env->codepage)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Failed to convert task id: %08x",id);
				return FALSE;
			}

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't update task id: %08x",id);
				return FALSE;
			}
		
			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SynCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	
	return TRUE;
}

static osync_bool commit_cal_change(OSyncContext *ctx, OSyncChange *change)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	const RRA_SyncMgrType *type = NULL;	
	uint32_t id=0;

	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
		
	type = rra_syncmgr_type_from_name(env->syncmgr, "appointment");

        id=strtol(osync_change_get_uid(change),NULL,16);

	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			osync_debug("SynCE-SYNC", 4, "deleting cal id: %08x",id);

			if (!rra_syncmgr_delete_object(env->syncmgr, type->id , id))  {
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't delete cal id: %08x",id);
    				return FALSE;
  			}	

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		case CHANGE_ADDED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id;
			
			object=osync_change_get_data(change);
			
			osync_debug("SynCE-SYNC", 4, "adding cal id %08x",id);

			if (!rra_appointment_from_vevent(object,&dummy_id,&data,&data_size,RRA_APPOINTMENT_UTF8,&env->timezone, env->codepage)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Failed to convert cal id: %08x",id);
				return FALSE;
			}

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_NEW_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't add cal id: %08x",id);
				return FALSE;
			}

			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		case CHANGE_MODIFIED:
		{
			char *object=NULL;
			size_t data_size;
			uint8_t *data;
			uint32_t dummy_id;
			
			object=osync_change_get_data(change);

			osync_debug("SynCE-SYNC", 4, "updating cal id %08x",id);

			if (!rra_appointment_from_vevent(object,&dummy_id,&data,&data_size,RRA_APPOINTMENT_UTF8,&env->timezone, env->codepage)) {
				osync_context_report_error(ctx, OSYNC_ERROR_CONVERT, "Failed to convert cal id: %08x",id);
				return FALSE;
			}

			if (!rra_syncmgr_put_single_object(env->syncmgr,type->id,id,RRA_SYNCMGR_UPDATE_OBJECT,data,data_size,&dummy_id)){
				osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Can't update cal id: %08x",id);
				return FALSE;
			}
		
			osync_debug("SynCE-SYNC", 4, "done");
			break;
		}
		default:
			osync_debug("SynCE-SYNC", 4, "Unknown change type");
	}
	//Answer the call
	osync_context_report_success(ctx);
	
	return TRUE;
}


static gboolean
mark_as_unchanged(gpointer key, gpointer value, gpointer user_data)
{
        OSyncContext *ctx = (OSyncContext*)user_data;
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
        SynceObject *object = (SynceObject*)value;

        if (object->change_counter == 0)
                return FALSE;

        /* commit any change done to forget contact changes */
        osync_debug("SynCE-SYNC", 4, "commit change for id %08x", object->object_id);

        if (!rra_syncmgr_mark_object_unchanged(env->syncmgr, object->type_id, object->object_id))
                osync_context_report_error(ctx, OSYNC_ERROR_GENERIC, "Failed to mark as unchanged id: %08x", object->object_id);

        return TRUE;
}

/*
 Sync_done: This function will only be called if the sync was successfull
*/
static void
sync_done(OSyncContext *ctx)
{
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
	int i;
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	

        for (i = 0; i < TYPE_INDEX_MAX; i++)
                if (env->objects[i] != NULL)
                        g_hash_table_foreach_remove(env->objects[i], mark_as_unchanged, ctx);

	if (env->config_file) {
		osync_hashtable_forget(env->hashtable);
	}

	osync_debug("SynCE-SYNC", 4, "Sync done.");	
	
	osync_context_report_success(ctx);
}

/*
 * Disconnect function: Called at the end of the process, should close the RRA connection.
 */
static void
disconnect(OSyncContext *ctx)
{
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	SyncePluginPtr *env = (SyncePluginPtr *)osync_context_get_plugin_data(ctx);
        int i;
	
	if (env->syncmgr==NULL) {
		osync_context_report_error(ctx, OSYNC_ERROR_NO_CONNECTION, "ERROR: no connection established");
		return;
	}

	if (env->config_file) {
		osync_hashtable_close(env->hashtable);
	}
	
	rra_syncmgr_disconnect(env->syncmgr);
	
	rra_syncmgr_destroy(env->syncmgr);
        env->syncmgr = NULL;

        for (i = 0; i < TYPE_INDEX_MAX; i++) {
                if (env->objects[i] != NULL) {
                        g_hash_table_destroy(env->objects[i]);
                        env->objects[i] = NULL;
                }
        }

        free(env->codepage);

	CeRapiUninit();

	osync_debug("SynCE-SYNC", 4, "Connection closed.");	
		
	osync_context_report_success(ctx);
}

/*
 * Finalize function: Called to unalloc all the structs and libraries used.
 */
static void
finalize(void *data)
{
	osync_debug("SynCE-SYNC", 4, "start: %s", __func__);	
	
	SyncePluginPtr *env = (SyncePluginPtr *)data;

	if (env->config_file) {
		osync_hashtable_free(env->hashtable);
	}

	free(env);
}

void get_info(OSyncPluginInfo *env)
{
	OSyncPluginInfo *info = osync_plugin_new_info((void*)env);

	info->name = "synce-plugin";
	info->longname = "Plugin to synchronize with Windows Mobile devices older than WM5";
	info->description = "by MirkuZ and Danny Backx";
	
	info->version = 1;
	
	info->is_threadsafe = FALSE;
	
	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;
	info->functions.get_data = synce_file_getdata;

	info->timeouts.connect_timeout = 5;
	
	osync_plugin_accept_objtype(info, "contact");
	osync_plugin_accept_objformat(info, "contact", "vcard30", NULL);
	osync_plugin_set_commit_objformat(info, "contact", "vcard30", commit_contacts_change);
	
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent10", NULL);
	osync_plugin_set_commit_objformat(info, "event", "vevent10", commit_cal_change);

	osync_plugin_accept_objtype(info, "todo");
    	osync_plugin_accept_objformat(info, "todo", "vtodo10", NULL);
    	osync_plugin_set_commit_objformat(info, "todo", "vtodo10", commit_todo_change);
	
	osync_plugin_accept_objtype(info, "data");
	osync_plugin_accept_objformat(info, "data", "file", NULL);
	osync_plugin_set_commit_objformat(info, "data", "file",
                                          synce_file_commit);
}
