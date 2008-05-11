/* $Id: invoke.c 3321 2008-03-20 09:44:20Z mark_ellis $ */
#include "rapi2_api.h"
#include "rapi_context.h"
#include <assert.h>
#include <stdlib.h>
#include <sys/socket.h>

struct _IRAPIStream
{
  RapiContext* context;
};

ULONG _NotImplementedIRAPIStream_Release2(IRAPIStream* stream)/*{{{*/
{
  return 0;
}/*}}}*/

HRESULT _NotImplementedIRAPIStream_Read2( /*{{{*/
    IRAPIStream* stream,
    void *pv,
    ULONG cb,
    ULONG *pcbRead)
{
  HRESULT hr = E_NOTIMPL;
  return hr;
}/*}}}*/

HRESULT _NotImplementedIRAPIStream_Write2( /*{{{*/
    IRAPIStream* stream,
    void const *pv,
    ULONG cb,
    ULONG *pcbWritten)
{
  HRESULT hr = E_NOTIMPL;
  return hr;
}/*}}}*/


HRESULT _NotImplementedCeRapiInvoke2( /*{{{*/
    LPCWSTR pDllPath,
    LPCWSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved)
{
  RapiContext* context = rapi_context_current();
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_CALL_NOT_IMPLEMENTED;
  return E_NOTIMPL;
}/*}}}*/

HRESULT _NotImplementedCeRapiInvokeA2( /*{{{*/
    LPCSTR pDllPath,
    LPCSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved)
{
  RapiContext* context = rapi_context_current();
  context->rapi_error = E_NOTIMPL;
  context->last_error = ERROR_CALL_NOT_IMPLEMENTED;
  return E_NOTIMPL;
}

