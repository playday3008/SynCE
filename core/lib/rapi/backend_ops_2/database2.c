#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "backend_ops_2.h"
#include "rapi_context.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define RAPI_DATABASE_DEBUG 0

#if RAPI_DATABASE_DEBUG
#define rapi_database_trace(args...)    synce_trace(args)
#define rapi_database_trace_wstr(args...)    synce_trace_wstr(args)
#define rapi_database_warning(args...)  synce_warning(args)
#define rapi_database_error(args...)    synce_error(args)
#else
#define rapi_database_trace(args...)
#define rapi_database_trace_wstr(args...)
#define rapi_database_warning(args...)
#define rapi_database_error(args...)
#endif


CEOID _NotImplementedCeCreateDatabase2(
		RapiContext *context,
		LPWSTR lpszName SYNCE_UNUSED,
		DWORD dwDbaseType SYNCE_UNUSED,
		WORD wNumSortOrder SYNCE_UNUSED,
		SORTORDERSPEC *rgSortSpecs SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}

BOOL _NotImplementedCeDeleteDatabase2(/*{{{*/
		RapiContext *context,
		CEOID oid SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}/*}}}*/

BOOL _NotImplementedCeFindAllDatabases2(/*{{{*/
		RapiContext *context,
		DWORD dwDbaseType SYNCE_UNUSED,
		WORD wFlags SYNCE_UNUSED,
		LPWORD cFindData SYNCE_UNUSED,
		LPLPCEDB_FIND_DATA ppFindData SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}/*}}}*/

HANDLE _NotImplementedCeFindFirstDatabase2(/*{{{*/
		RapiContext *context,
		DWORD dwDbaseType SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return INVALID_HANDLE_VALUE;
}/*}}}*/

CEOID _NotImplementedCeFindNextDatabase2(/*{{{*/
		RapiContext *context,
		HANDLE hEnum SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}/*}}}*/

HANDLE _NotImplementedCeOpenDatabase2(/*{{{*/
		RapiContext *context,
		PCEOID poid SYNCE_UNUSED,
		LPWSTR lpszName SYNCE_UNUSED,
		CEPROPID propid SYNCE_UNUSED,
		DWORD dwFlags SYNCE_UNUSED,
		HWND hwndNotify SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return INVALID_HANDLE_VALUE;
}/*}}}*/


CEOID _NotImplementedCeReadRecordProps2(/*{{{*/
		RapiContext *context,
		HANDLE hDbase SYNCE_UNUSED,
		DWORD dwFlags SYNCE_UNUSED,
		LPWORD lpcPropID SYNCE_UNUSED,
		CEPROPID *rgPropID SYNCE_UNUSED,
		LPBYTE *lplpBuffer SYNCE_UNUSED,
		LPDWORD lpcbBuffer SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}

CEOID _NotImplementedCeWriteRecordProps2(
		RapiContext *context,
		HANDLE hDbase SYNCE_UNUSED, 
		CEOID oidRecord SYNCE_UNUSED,
		WORD cPropID SYNCE_UNUSED,
		CEPROPVAL* rgPropVal SYNCE_UNUSED)/*{{{*/
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}


CEOID _NotImplementedCeSeekDatabase2(/*{{{*/
		RapiContext *context,
		HANDLE hDatabase SYNCE_UNUSED,
		DWORD dwSeekType SYNCE_UNUSED,
		DWORD dwValue SYNCE_UNUSED,
		LPDWORD lpdwIndex SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}/*}}}*/

BOOL _NotImplementedCeDeleteRecord2(/*{{{*/
    RapiContext *context,
    HANDLE hDatabase SYNCE_UNUSED,
    CEOID oidRecord SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}/*}}}*/

BOOL _NotImplementedCeSetDatabaseInfo2(
    RapiContext *context,
    CEOID oidDbase SYNCE_UNUSED,
    CEDBASEINFO* pNewInfo SYNCE_UNUSED)
{
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_SUCCESS;
  return 0;
}

