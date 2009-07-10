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

static HRESULT CeRapiInvokeCommon2(
    RapiContext* context,
    LPCWSTR pDllPath,
    LPCWSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD dwReserved,
    BOOL inRapiStream
    )
{
  if (cbInput)
    if (!pInput)
      return E_INVALIDARG;

  rapi_context_begin_command(context, 0x4c);
  rapi_buffer_write_uint32(context->send_buffer, dwReserved);
  rapi2_buffer_write_string(context->send_buffer, pDllPath);
  rapi2_buffer_write_string(context->send_buffer, pFunctionName);
  rapi_buffer_write_uint32(context->send_buffer, cbInput);
  if (cbInput)
    rapi_buffer_write_data  (context->send_buffer, pInput, cbInput);
  rapi_buffer_write_uint32(context->send_buffer, inRapiStream);

  return S_OK;
}

static HRESULT CeRapiInvokeBuffers2(
    LPCWSTR pDllPath,
    LPCWSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    DWORD dwReserved)
{
  RapiContext* context = rapi_context_current();
  HRESULT return_value = E_UNEXPECTED;
  HRESULT hr;

  synce_trace("begin");
  
  hr = CeRapiInvokeCommon2(
      context,
      pDllPath,
      pFunctionName,
      cbInput,
      pInput,
      dwReserved,
      FALSE);
  if (FAILED(hr))
  {
    synce_error("CeRapiInvokeCommon2 failed");
    return hr;
  }

  if ( !rapi2_context_call(context) )
  {
    synce_error("rapi2_context_call failed");
    return E_FAIL;
  }

  synce_trace("pInput: 0x%08x", pInput);
  rapi_buffer_read_uint32(context->recv_buffer, &context->last_error);
  synce_trace("last error: 0x%08x", context->last_error);
  rapi_buffer_read_int32(context->recv_buffer, &return_value);
  synce_trace("return_value: 0x%08x", return_value);

  if (FAILED(return_value))
    return return_value;

  if (!pcbOutput)
    return return_value;

  rapi_buffer_read_uint32(context->recv_buffer, pcbOutput);
  synce_trace("output_size: 0x%08x", *pcbOutput);
  if (*pcbOutput > 0 && ppOutput)
  {
    *ppOutput = malloc(*pcbOutput);
    if(!*ppOutput)
    {
      return E_OUTOFMEMORY;
    }
    if (!rapi_buffer_read_data(context->recv_buffer, *ppOutput, *pcbOutput))
    {
      synce_error("Failed to read output data");
      hr = E_FAIL;
    }
    else
    {
      synce_trace("output_buffer: 0x%0x", ppOutput);
    }
  }

  if (SUCCEEDED(hr))
    return return_value;
  else
    return hr;
}

HRESULT _CeRapiInvoke2( /*{{{*/
    LPCWSTR pDllPath,
    LPCWSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved)
{
  if (ppIRAPIStream)
  {
    synce_error("Stream mode is not implemented");
    return E_NOTIMPL;
  }
  else
    return CeRapiInvokeBuffers2(pDllPath, pFunctionName, cbInput, pInput,
        pcbOutput, ppOutput, dwReserved);
}/*}}}*/

HRESULT _CeRapiInvokeA2( /*{{{*/
    LPCSTR pDllPath,
    LPCSTR pFunctionName,
    DWORD cbInput,
    const BYTE *pInput,
    DWORD *pcbOutput,
    BYTE **ppOutput,
    IRAPIStream **ppIRAPIStream,
    DWORD dwReserved)
{
  HRESULT hr;
  WCHAR* wide_dll_path      = wstr_from_current(pDllPath);
  WCHAR* wide_function_name = wstr_from_current(pFunctionName);

  if ((!wide_dll_path) || (!wide_function_name)) {
          wstr_free_string(wide_dll_path);
          wstr_free_string(wide_function_name);
          return E_INVALIDARG;
  }

  hr = CeRapiInvoke( wide_dll_path, wide_function_name, cbInput, pInput,
      pcbOutput, ppOutput, ppIRAPIStream, dwReserved);

  wstr_free_string(wide_dll_path);
  wstr_free_string(wide_function_name);

  return hr;
}
