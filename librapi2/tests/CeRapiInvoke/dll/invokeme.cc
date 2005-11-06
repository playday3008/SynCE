#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>

#undef  INTERFACE
#define INTERFACE   IRAPIStream

typedef enum tagRAPISTREAMFLAG
{
  STREAM_TIMEOUT_READ
} RAPISTREAMFLAG;

DECLARE_INTERFACE_ (IRAPIStream,  IStream)
{
  STDMETHOD(SetRapiStat)( THIS_ RAPISTREAMFLAG Flag, DWORD dwValue) PURE;
  STDMETHOD(GetRapiStat)( THIS_ RAPISTREAMFLAG Flag, DWORD *pdwValue) PURE;
};

#define NEED_ISTREAM_MACROS

#ifdef NEED_ISTREAM_MACROS
#define IStream_AddRef(This) (This)->lpVtbl->AddRef(This)
#define IStream_Clone(This,ppstm) (This)->lpVtbl->Clone(This,ppstm)
#define IStream_Read(This,pv,cb,pcbRead) (This)->lpVtbl->Read(This,pv,cb,pcbRead)
#define IStream_Release(This) (This)->lpVtbl->Release(This)
#define IStream_Seek(This,dlibMove,dwOrigin,plibNewPosition) (This)->lpVtbl->Seek(This,dlibMove,dwOrigin,plibNewPosition)
#define IStream_Stat(This,pstatstg,grfStatFlag)	(This)->lpVtbl->Stat(This,pstatstg,grfStatFlag)
#endif

EXTERN_C __declspec(dllexport) HRESULT TestLastError(
    DWORD cbInput,
    BYTE* pInput, 
    DWORD* pcbOutput, 
    BYTE** ppOutput,
    IRAPIStream* pStream)
{
  if (cbInput != sizeof(DWORD))
    return E_FAIL;
  SetLastError(*(DWORD*)pInput);
  return S_OK;
}

EXTERN_C __declspec(dllexport) HRESULT PingResult(
    DWORD cbInput,
    BYTE* pInput, 
    DWORD* pcbOutput, 
    BYTE** ppOutput,
    IRAPIStream* pStream)
{
  if (cbInput != (sizeof(HRESULT)))
    return E_FAIL;
  return *(HRESULT*)pInput;
}

EXTERN_C __declspec(dllexport) HRESULT PingBuffer(
    DWORD cbInput,
    BYTE* pInput, 
    DWORD* pcbOutput, 
    BYTE** ppOutput,
    IRAPIStream* pStream)
{
  *pcbOutput = cbInput;
  *ppOutput = (BYTE*)LocalAlloc(0, cbInput);
  memcpy(*ppOutput, pInput, cbInput);
  return S_OK;
}

EXTERN_C __declspec(dllexport) HRESULT PingStream(
    DWORD cbInput,
    BYTE* pInput, 
    DWORD* pcbOutput, 
    BYTE** ppOutput,
    IRAPIStream* pStream)
{
  DWORD size;
  BYTE* buffer;
  HRESULT hr = E_UNEXPECTED;
  DWORD count;

  if (cbInput != sizeof(DWORD))
    return E_FAIL;

  size = *(DWORD*)pInput;

  *pcbOutput = size;
  buffer = (BYTE*)LocalAlloc(0, size);

  Sleep(1);

  hr = pStream->Read(buffer, size, &count);
  if (FAILED(hr) || size != count)
    goto exit;

  Sleep(1);

  hr = pStream->Write(buffer, size, &count);
  if (FAILED(hr) || size != count)
    goto exit;

  hr = S_OK;

exit:
  LocalFree(buffer);
  return hr;
}

