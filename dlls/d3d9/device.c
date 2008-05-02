/*
 * IDirect3DDevice9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);


/* IDirect3D IUnknown parts follow: */
static HRESULT WINAPI IDirect3DDevice9Impl_QueryInterface(LPDIRECT3DDEVICE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DDevice9)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DDevice9Impl_AddRef(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DDevice9Impl_Release(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    ULONG ref;

    if (This->inDestruction) return 0;
    ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
      unsigned i;
      This->inDestruction = TRUE;

      EnterCriticalSection(&d3d9_cs);
      for(i = 0; i < This->numConvertedDecls; i++) {
          /* Unless Wine is buggy or the app has a bug the refcount will be 0, because decls hold a reference to the
           * device
           */
          IDirect3DVertexDeclaration9Impl_Destroy(This->convertedDecls[i]);
      }
      HeapFree(GetProcessHeap(), 0, This->convertedDecls);

      IWineD3DDevice_Uninit3D(This->WineD3DDevice, D3D9CB_DestroyDepthStencilSurface, D3D9CB_DestroySwapChain);
      IWineD3DDevice_Release(This->WineD3DDevice);
      LeaveCriticalSection(&d3d9_cs);
      HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
static HRESULT  WINAPI  IDirect3DDevice9Impl_TestCooperativeLevel(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) : Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_TestCooperativeLevel(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static UINT     WINAPI  IDirect3DDevice9Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetAvailableTextureMem(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_EvictManagedResources(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) : Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_EvictManagedResources(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetDirect3D(LPDIRECT3DDEVICE9 iface, IDirect3D9** ppD3D9) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3D* pWineD3D;

    TRACE("(%p) Relay\n", This);

    if (NULL == ppD3D9) {
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetDirect3D(This->WineD3DDevice, &pWineD3D);
    if (hr == D3D_OK && pWineD3D != NULL)
    {
        IWineD3D_GetParent(pWineD3D,(IUnknown **)ppD3D9);
        IWineD3D_Release(pWineD3D);
    } else {
        FIXME("Call to IWineD3DDevice_GetDirect3D failed\n");
        *ppD3D9 = NULL;
    }
    TRACE("(%p) returning %p\n", This, *ppD3D9);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetDeviceCaps(LPDIRECT3DDEVICE9 iface, D3DCAPS9* pCaps) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) : Relay pCaps %p\n", This, pCaps);
    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /* well this is what MSDN says to return */
    }

    memset(pCaps, 0, sizeof(*pCaps));
    D3D9CAPSTOWINECAPS(pCaps, pWineCaps)
    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DDevice_GetDeviceCaps(This->WineD3DDevice, pWineCaps);
    LeaveCriticalSection(&d3d9_cs);
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    /* Some functionality is implemented in d3d9.dll, not wined3d.dll. Add the needed caps */
    pCaps->DevCaps2 |= D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;

    filter_caps(pCaps);

    TRACE("Returning %p %p\n", This, pCaps);
    return hrc;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetDisplayMode(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DDISPLAYMODE* pMode) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetDisplayMode(This->WineD3DDevice, iSwapChain, (WINED3DDISPLAYMODE *) pMode);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetCreationParameters(LPDIRECT3DDEVICE9 iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetCreationParameters(This->WineD3DDevice, (WINED3DDEVICE_CREATION_PARAMETERS *) pParameters);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetCursorProperties(LPDIRECT3DDEVICE9 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *pSurface = (IDirect3DSurface9Impl*)pCursorBitmap;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);
    if(!pCursorBitmap) {
        WARN("No cursor bitmap, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetCursorProperties(This->WineD3DDevice, XHotSpot, YHotSpot, pSurface->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static void     WINAPI  IDirect3DDevice9Impl_SetCursorPosition(LPDIRECT3DDEVICE9 iface, int XScreenSpace, int YScreenSpace, DWORD Flags) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    IWineD3DDevice_SetCursorPosition(This->WineD3DDevice, XScreenSpace, YScreenSpace, Flags);
    LeaveCriticalSection(&d3d9_cs);
}

static BOOL     WINAPI  IDirect3DDevice9Impl_ShowCursor(LPDIRECT3DDEVICE9 iface, BOOL bShow) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    BOOL ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DDevice_ShowCursor(This->WineD3DDevice, bShow);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

static HRESULT WINAPI reset_enum_callback(IWineD3DResource *resource, void *data) {
    BOOL *resources_ok = (BOOL *) data;
    WINED3DRESOURCETYPE type;
    HRESULT ret = S_OK;
    WINED3DSURFACE_DESC surface_desc;
    WINED3DVOLUME_DESC volume_desc;
    WINED3DINDEXBUFFER_DESC index_desc;
    WINED3DVERTEXBUFFER_DESC vertex_desc;
    WINED3DFORMAT dummy_format;
    DWORD dummy_dword;
    WINED3DPOOL pool = WINED3DPOOL_SCRATCH; /* a harmless pool */
    IUnknown *parent;

    type = IWineD3DResource_GetType(resource);
    switch(type) {
        case WINED3DRTYPE_SURFACE:
            surface_desc.Format = &dummy_format;
            surface_desc.Type = &type;
            surface_desc.Usage = &dummy_dword;
            surface_desc.Pool = &pool;
            surface_desc.Size = &dummy_dword;
            surface_desc.MultiSampleType = &dummy_dword;
            surface_desc.MultiSampleQuality = &dummy_dword;
            surface_desc.Width = &dummy_dword;
            surface_desc.Height = &dummy_dword;

            IWineD3DSurface_GetDesc((IWineD3DSurface *) resource, &surface_desc);
            break;

        case WINED3DRTYPE_VOLUME:
            volume_desc.Format = &dummy_format;
            volume_desc.Type = &type;
            volume_desc.Usage = &dummy_dword;
            volume_desc.Pool = &pool;
            volume_desc.Size = &dummy_dword;
            volume_desc.Width = &dummy_dword;
            volume_desc.Height = &dummy_dword;
            volume_desc.Depth = &dummy_dword;
            IWineD3DVolume_GetDesc((IWineD3DVolume *) resource, &volume_desc);
            break;

        case WINED3DRTYPE_INDEXBUFFER:
            IWineD3DIndexBuffer_GetDesc((IWineD3DIndexBuffer *) resource, &index_desc);
            pool = index_desc.Pool;
            break;

        case WINED3DRTYPE_VERTEXBUFFER:
            IWineD3DVertexBuffer_GetDesc((IWineD3DVertexBuffer *) resource, &vertex_desc);
            pool = index_desc.Pool;
            break;

        /* No need to check for textures. If there is a D3DPOOL_DEFAULT texture, there
         * is a D3DPOOL_DEFAULT surface or volume as well
         */
        default:
            break;
    }

    if(pool == WINED3DPOOL_DEFAULT) {
        IWineD3DResource_GetParent(resource, &parent);
        if(IUnknown_Release(parent) == 0) {
            TRACE("Parent %p is an implicit resource with ref 0\n", parent);
        } else {
            WARN("Resource %p(wineD3D %p) with pool D3DPOOL_DEFAULT blocks the Reset call\n", parent, resource);
            ret = S_FALSE;
            *resources_ok = FALSE;
        }
    }
    IWineD3DResource_Release(resource);

    return ret;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_Reset(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    WINED3DPRESENT_PARAMETERS localParameters;
    HRESULT hr;
    BOOL resources_ok = TRUE;
    UINT i;

    TRACE("(%p) Relay pPresentationParameters(%p)\n", This, pPresentationParameters);

    /* Reset states that hold a COM object. WineD3D holds an internal reference to set objects, because
     * such objects can still be used for rendering after their external d3d9 object has been destroyed.
     * These objects must not be enumerated. Unsetting them tells WineD3D that the application will not
     * make use of the hidden reference and destroys the objects.
     *
     * Unsetting them is no problem, because the states are supposed to be reset anyway. If the validation
     * below fails, the device is considered "lost", and _Reset and _Release are the only allowed calls
     */
    IWineD3DDevice_SetIndices(This->WineD3DDevice, NULL);
    for(i = 0; i < 16; i++) {
        IWineD3DDevice_SetStreamSource(This->WineD3DDevice, i, NULL, 0, 0);
    }
    for(i = 0; i < 16; i++) {
        IWineD3DDevice_SetTexture(This->WineD3DDevice, i, NULL);
    }

    IWineD3DDevice_EnumResources(This->WineD3DDevice, reset_enum_callback, &resources_ok);
    if(!resources_ok) {
        WARN("The application is holding D3DPOOL_DEFAULT resources, rejecting reset\n");
        return WINED3DERR_INVALIDCALL;
    }

    localParameters.BackBufferWidth                     = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                    = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                    = pPresentationParameters->BackBufferFormat;
    localParameters.BackBufferCount                     = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                     = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                  = pPresentationParameters->MultiSampleQuality;
    localParameters.SwapEffect                          = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                       = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                            = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil              = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat              = pPresentationParameters->AutoDepthStencilFormat;
    localParameters.Flags                               = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz          = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                = pPresentationParameters->PresentationInterval;

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_Reset(This->WineD3DDevice, &localParameters);
    LeaveCriticalSection(&d3d9_cs);

    pPresentationParameters->BackBufferWidth            = localParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight           = localParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat           = localParameters.BackBufferFormat;
    pPresentationParameters->BackBufferCount            = localParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType            = localParameters.MultiSampleType;
    pPresentationParameters->MultiSampleQuality         = localParameters.MultiSampleQuality;
    pPresentationParameters->SwapEffect                 = localParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow              = localParameters.hDeviceWindow;
    pPresentationParameters->Windowed                   = localParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil     = localParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat     = localParameters.AutoDepthStencilFormat;
    pPresentationParameters->Flags                      = localParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz = localParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->PresentationInterval       = localParameters.PresentationInterval;

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_Present(LPDIRECT3DDEVICE9 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA*
 pDirtyRegion) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_Present(This->WineD3DDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
 }

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetBackBuffer(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 ** ppBackBuffer) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DSurface *retSurface = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    rc = IWineD3DDevice_GetBackBuffer(This->WineD3DDevice, iSwapChain, BackBuffer, (WINED3DBACKBUFFER_TYPE) Type, &retSurface);
    if (rc == D3D_OK && NULL != retSurface && NULL != ppBackBuffer) {
        IWineD3DSurface_GetParent(retSurface, (IUnknown **)ppBackBuffer);
        IWineD3DSurface_Release(retSurface);
    }
    LeaveCriticalSection(&d3d9_cs);
    return rc;
}
static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRasterStatus(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetRasterStatus(This->WineD3DDevice, iSwapChain, (WINED3DRASTER_STATUS *) pRasterStatus);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetDialogBoxMode(LPDIRECT3DDEVICE9 iface, BOOL bEnableDialogs) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetDialogBoxMode(This->WineD3DDevice, bEnableDialogs);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static void WINAPI IDirect3DDevice9Impl_SetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    IWineD3DDevice_SetGammaRamp(This->WineD3DDevice, iSwapChain, Flags, (CONST WINED3DGAMMARAMP *)pRamp);
    LeaveCriticalSection(&d3d9_cs);
}

static void WINAPI IDirect3DDevice9Impl_GetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DGAMMARAMP* pRamp) {    
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    IWineD3DDevice_GetGammaRamp(This->WineD3DDevice, iSwapChain, (WINED3DGAMMARAMP *) pRamp);
    LeaveCriticalSection(&d3d9_cs);
}


static HRESULT  WINAPI IDirect3DDevice9Impl_CreateSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IDirect3DSurface9 **ppSurface,D3DRESOURCETYPE Type, UINT Usage, D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality,HANDLE* pSharedHandle )  {
    HRESULT hrc;
    IDirect3DSurface9Impl *object;
    IDirect3DDevice9Impl  *This = (IDirect3DDevice9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    
    if(MultisampleQuality > 0){
        FIXME("MultisampleQuality set to %d, bstituting 0\n", MultisampleQuality);
    /*
    MultisampleQuality
 [in] Quality level. The valid range is between zero and one less than the level returned by pQualityLevels used by IDirect3D9::CheckDeviceMultiSampleType. Passing a larger value returns the error D3DERR_INVALIDCALL. The MultisampleQuality values of paired render targets, depth stencil surfaces, and the MultiSample type must all match.
 */
 
        MultisampleQuality=0;
    }
    /*FIXME: Check MAX bounds of MultisampleQuality*/

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface9Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DSurface9_Vtbl;
    object->ref = 1;
    
    TRACE("(%p) : w(%d) h(%d) fmt(%d) surf@%p\n", This, Width, Height, Format, *ppSurface);
           
    hrc = IWineD3DDevice_CreateSurface(This->WineD3DDevice, Width, Height, Format, Lockable, Discard, Level,  &object->wineD3DSurface, Type, Usage & WINED3DUSAGE_MASK, (WINED3DPOOL) Pool,MultiSample,MultisampleQuality,pSharedHandle,SURFACE_OPENGL,(IUnknown *)object);
    
    if (hrc != D3D_OK || NULL == object->wineD3DSurface) {

       /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateSurface failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        TRACE("(%p) : Created surface %p\n", This, object);
        *ppSurface = (LPDIRECT3DSURFACE9) object;
    }
    return hrc;
}



static HRESULT  WINAPI  IDirect3DDevice9Impl_CreateRenderTarget(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, 
                                                         D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, 
                                                         DWORD MultisampleQuality, BOOL Lockable, 
                                                         IDirect3DSurface9 **ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr;
    TRACE("Relay\n");

   /* Is this correct? */
   EnterCriticalSection(&d3d9_cs);
   hr = IDirect3DDevice9Impl_CreateSurface(iface,Width,Height,Format,Lockable,FALSE/*Discard*/, 0/*Level*/, ppSurface,D3DRTYPE_SURFACE,D3DUSAGE_RENDERTARGET,D3DPOOL_DEFAULT,MultiSample,MultisampleQuality,pSharedHandle);
   LeaveCriticalSection(&d3d9_cs);
   return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height,
                                                                D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                                                DWORD MultisampleQuality, BOOL Discard,
                                                                IDirect3DSurface9 **ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr;
    TRACE("Relay\n");

     EnterCriticalSection(&d3d9_cs);
     hr = IDirect3DDevice9Impl_CreateSurface(iface,Width,Height,Format,TRUE/* Lockable */,Discard, 0/* Level */
                                               ,ppSurface,D3DRTYPE_SURFACE,D3DUSAGE_DEPTHSTENCIL,
                                                D3DPOOL_DEFAULT,MultiSample,MultisampleQuality,pSharedHandle);
     LeaveCriticalSection(&d3d9_cs);
     return hr;
}


static HRESULT  WINAPI  IDirect3DDevice9Impl_UpdateSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_UpdateSurface(This->WineD3DDevice, ((IDirect3DSurface9Impl *)pSourceSurface)->wineD3DSurface, pSourceRect, ((IDirect3DSurface9Impl *)pDestinationSurface)->wineD3DSurface, pDestPoint);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_UpdateTexture(LPDIRECT3DDEVICE9 iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_UpdateTexture(This->WineD3DDevice,  ((IDirect3DBaseTexture9Impl *)pSourceTexture)->wineD3DBaseTexture, ((IDirect3DBaseTexture9Impl *)pDestinationTexture)->wineD3DBaseTexture);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

/* This isn't in MSDN!
static HRESULT  WINAPI  IDirect3DDevice9Impl_GetFrontBuffer(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pDestSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}
*/

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderTargetData(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *renderTarget = (IDirect3DSurface9Impl *)pRenderTarget;
    IDirect3DSurface9Impl *destSurface = (IDirect3DSurface9Impl *)pDestSurface;
    HRESULT hr;
    TRACE("(%p)->(%p,%p)\n" , This, renderTarget, destSurface);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_BltFast(destSurface->wineD3DSurface, 0, 0, renderTarget->wineD3DSurface, NULL, WINEDDBLTFAST_NOCOLORKEY);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetFrontBufferData(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *destSurface = (IDirect3DSurface9Impl *)pDestSurface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetFrontBufferData(This->WineD3DDevice, iSwapChain, destSurface->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_StretchRect(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *src = (IDirect3DSurface9Impl *) pSourceSurface;
    IDirect3DSurface9Impl *dst = (IDirect3DSurface9Impl *) pDestSurface;
    HRESULT hr;

    TRACE("(%p)->(%p,%p,%p,%p,%d)\n" , This, src, pSourceRect, dst, pDestRect, Filter);
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DSurface_Blt(dst->wineD3DSurface, (RECT *) pDestRect, src->wineD3DSurface, (RECT *) pSourceRect, 0, NULL, Filter);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_ColorFill(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *surface = (IDirect3DSurface9Impl *)pSurface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DRECT is compatible with WINED3DRECT */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_ColorFill(This->WineD3DDevice, surface->wineD3DSurface, (CONST WINED3DRECT*)pRect, color);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_CreateOffscreenPlainSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9 **ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr;
    TRACE("Relay\n");
    if(Pool == D3DPOOL_MANAGED ){
        FIXME("Attempting to create a managed offscreen plain surface\n");
        return D3DERR_INVALIDCALL;
    }    
        /*MSDN: D3DPOOL_SCRATCH will return a surface that has identical characteristics to a surface created by the Microsoft DirectX 8.x method CreateImageSurface.
        
        'Off-screen plain surfaces are always lockable, regardless of their pool types.'
        but then...
        D3DPOOL_DEFAULT is the appropriate pool for use with the IDirect3DDevice9::StretchRect and IDirect3DDevice9::ColorFill.
        Why, their always lockable?
        should I change the usage to dynamic?        
        */
    EnterCriticalSection(&d3d9_cs);
    hr = IDirect3DDevice9Impl_CreateSurface(iface,Width,Height,Format,TRUE/*Loackable*/,FALSE/*Discard*/,0/*Level*/ , ppSurface,D3DRTYPE_SURFACE, 0/*Usage (undefined/none)*/,(WINED3DPOOL) Pool,D3DMULTISAMPLE_NONE,0/*MultisampleQuality*/,pSharedHandle);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

/* TODO: move to wineD3D */
static HRESULT  WINAPI  IDirect3DDevice9Impl_SetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *pSurface = (IDirect3DSurface9Impl*)pRenderTarget;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetRenderTarget(This->WineD3DDevice, RenderTargetIndex, pSurface ? pSurface->wineD3DSurface : NULL);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex, IDirect3DSurface9 **ppRenderTarget) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pRenderTarget;

    TRACE("(%p) Relay\n" , This);

    if (ppRenderTarget == NULL) {
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    hr=IWineD3DDevice_GetRenderTarget(This->WineD3DDevice,RenderTargetIndex,&pRenderTarget);

    if (hr == D3D_OK && pRenderTarget != NULL) {
        IWineD3DSurface_GetParent(pRenderTarget,(IUnknown**)ppRenderTarget);
        IWineD3DSurface_Release(pRenderTarget);
    } else {
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed\n");
        *ppRenderTarget = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pZStencilSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *pSurface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    pSurface = (IDirect3DSurface9Impl*)pZStencilSurface;
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetDepthStencilSurface(This->WineD3DDevice, NULL==pSurface ? NULL : pSurface->wineD3DSurface);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9 **ppZStencilSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pZStencilSurface;

    TRACE("(%p) Relay\n" , This);
    if(ppZStencilSurface == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetDepthStencilSurface(This->WineD3DDevice,&pZStencilSurface);
    if(hr == D3D_OK) {
        if(pZStencilSurface != NULL){
            IWineD3DSurface_GetParent(pZStencilSurface,(IUnknown**)ppZStencilSurface);
            IWineD3DSurface_Release(pZStencilSurface);
        } else {
            *ppZStencilSurface = NULL;
        }
    } else {
        WARN("Call to IWineD3DDevice_GetDepthStencilSurface failed\n");
        *ppZStencilSurface = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_BeginScene(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_BeginScene(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_EndScene(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_EndScene(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_Clear(LPDIRECT3DDEVICE9 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DRECT is compatible with WINED3DRECT */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_Clear(This->WineD3DDevice, Count, (CONST WINED3DRECT*) pRects, Flags, Color, Z, Stencil);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* lpMatrix) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) lpMatrix);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    return IWineD3DDevice_GetTransform(This->WineD3DDevice, State, (WINED3DMATRIX*) pMatrix);
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_MultiplyTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_MultiplyTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) pMatrix);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetViewport(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DVIEWPORT9 is compatible with WINED3DVIEWPORT */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetViewport(This->WineD3DDevice, (const WINED3DVIEWPORT *)pViewport);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetViewport(LPDIRECT3DDEVICE9 iface, D3DVIEWPORT9* pViewport) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DVIEWPORT9 is compatible with WINED3DVIEWPORT */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetViewport(This->WineD3DDevice, (WINED3DVIEWPORT *)pViewport);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetMaterial(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATERIAL9 is compatible with WINED3DMATERIAL */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetMaterial(This->WineD3DDevice, (const WINED3DMATERIAL *)pMaterial);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetMaterial(LPDIRECT3DDEVICE9 iface, D3DMATERIAL9* pMaterial) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATERIAL9 is compatible with WINED3DMATERIAL */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetMaterial(This->WineD3DDevice, (WINED3DMATERIAL *)pMaterial);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST D3DLIGHT9* pLight) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DLIGHT9 is compatible with WINED3DLIGHT */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetLight(This->WineD3DDevice, Index, (const WINED3DLIGHT *)pLight);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, D3DLIGHT9* pLight) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DLIGHT9 is compatible with WINED3DLIGHT */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetLight(This->WineD3DDevice, Index, (WINED3DLIGHT *)pLight);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_LightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL Enable) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetLightEnable(This->WineD3DDevice, Index, Enable);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetLightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL* pEnable) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetLightEnable(This->WineD3DDevice, Index, pEnable);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST float* pPlane) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetClipPlane(This->WineD3DDevice, Index, pPlane);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, float* pPlane) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetClipPlane(This->WineD3DDevice, Index, pPlane);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetRenderState(This->WineD3DDevice, State, Value);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD* pValue) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetRenderState(This->WineD3DDevice, State, pValue);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetClipStatus(LPDIRECT3DDEVICE9 iface, CONST D3DCLIPSTATUS9* pClipStatus) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetClipStatus(This->WineD3DDevice, (const WINED3DCLIPSTATUS *)pClipStatus);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetClipStatus(LPDIRECT3DDEVICE9 iface, D3DCLIPSTATUS9* pClipStatus) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetClipStatus(This->WineD3DDevice, (WINED3DCLIPSTATUS *)pClipStatus);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9 **ppTexture) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DBaseTexture *retTexture = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n" , This);

    if(ppTexture == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    rc = IWineD3DDevice_GetTexture(This->WineD3DDevice, Stage, &retTexture);
    if (rc == D3D_OK && NULL != retTexture) {
        IWineD3DBaseTexture_GetParent(retTexture, (IUnknown **)ppTexture);
        IWineD3DBaseTexture_Release(retTexture);
    }else{
        FIXME("Call to get texture  (%d) failed (%p)\n", Stage, retTexture);
        *ppTexture = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);

    return rc;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9* pTexture) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay %d %p\n" , This, Stage, pTexture);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetTexture(This->WineD3DDevice, Stage,
                                   pTexture==NULL ? NULL:((IDirect3DBaseTexture9Impl *)pTexture)->wineD3DBaseTexture);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetTextureStageState(This->WineD3DDevice, Stage, Type, pValue);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetTextureStageState(This->WineD3DDevice, Stage, Type, Value);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;    
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetSamplerState(This->WineD3DDevice, Sampler, Type, pValue);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetSamplerState(This->WineD3DDevice, Sampler, Type, Value);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_ValidateDevice(LPDIRECT3DDEVICE9 iface, DWORD* pNumPasses) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_ValidateDevice(This->WineD3DDevice, pNumPasses);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;    
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT* PaletteNumber) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetScissorRect(LPDIRECT3DDEVICE9 iface, CONST RECT* pRect) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetScissorRect(This->WineD3DDevice, pRect);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetScissorRect(LPDIRECT3DDEVICE9 iface, RECT* pRect) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetScissorRect(This->WineD3DDevice, pRect);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface, BOOL bSoftware) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetSoftwareVertexProcessing(This->WineD3DDevice, bSoftware);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static BOOL     WINAPI  IDirect3DDevice9Impl_GetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetSoftwareVertexProcessing(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetNPatchMode(LPDIRECT3DDEVICE9 iface, float nSegments) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetNPatchMode(This->WineD3DDevice, nSegments);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static float    WINAPI  IDirect3DDevice9Impl_GetNPatchMode(LPDIRECT3DDEVICE9 iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetNPatchMode(This->WineD3DDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;    
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_DrawPrimitive(This->WineD3DDevice, PrimitiveType, StartVertex, PrimitiveCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* D3D8 passes the baseVertexIndex in SetIndices, and due to the stateblock functions wined3d has to work that way */
    EnterCriticalSection(&d3d9_cs);
    IWineD3DDevice_SetBaseVertexIndex(This->WineD3DDevice, BaseVertexIndex);
    hr = IWineD3DDevice_DrawIndexedPrimitive(This->WineD3DDevice, PrimitiveType, MinVertexIndex, NumVertices, startIndex, primCount);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;    
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_DrawPrimitiveUP(This->WineD3DDevice, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex,
                                                             UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_DrawIndexedPrimitiveUP(This->WineD3DDevice, PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount,
                                                 pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_ProcessVertices(LPDIRECT3DDEVICE9 iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *Decl = (IDirect3DVertexDeclaration9Impl *) pVertexDecl;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_ProcessVertices(This->WineD3DDevice,SrcStartIndex, DestIndex, VertexCount, ((IDirect3DVertexBuffer9Impl *)pDestBuffer)->wineD3DVertexBuffer, Decl ? Decl->wineD3DVertexDeclaration : NULL, Flags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

IDirect3DVertexDeclaration9 *getConvertedDecl(IDirect3DDevice9Impl *This, DWORD fvf) {
    HRESULT hr;
    D3DVERTEXELEMENT9* elements = NULL;
    IDirect3DVertexDeclaration9* pDecl = NULL;
    int p, low, high; /* deliberately signed */
    IDirect3DVertexDeclaration9  **convertedDecls = This->convertedDecls;

    TRACE("Searching for declaration for fvf %08x... ", fvf);

    low = 0;
    high = This->numConvertedDecls - 1;
    while(low <= high) {
        p = (low + high) >> 1;
        TRACE("%d ", p);
        if(((IDirect3DVertexDeclaration9Impl *) convertedDecls[p])->convFVF == fvf) {
            TRACE("found %p\n", convertedDecls[p]);
            return convertedDecls[p];
        } else if(((IDirect3DVertexDeclaration9Impl *) convertedDecls[p])->convFVF < fvf) {
            low = p + 1;
        } else {
            high = p - 1;
        }
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    hr = vdecl_convert_fvf(fvf, &elements);
    if (hr != S_OK) return NULL;

    hr = IDirect3DDevice9Impl_CreateVertexDeclaration((IDirect3DDevice9 *) This, elements, &pDecl);
    HeapFree(GetProcessHeap(), 0, elements); /* CreateVertexDeclaration makes a copy */
    if (hr != S_OK) return NULL;

    if(This->declArraySize == This->numConvertedDecls) {
        int grow = max(This->declArraySize / 2, 8);
        convertedDecls = HeapReAlloc(GetProcessHeap(), 0, convertedDecls,
                                     sizeof(convertedDecls[0]) * (This->numConvertedDecls + grow));
        if(!convertedDecls) {
            /* This will destroy it */
            IDirect3DVertexDeclaration9_Release(pDecl);
            return NULL;
        }
        This->convertedDecls = convertedDecls;
        This->declArraySize += grow;
    }

    memmove(convertedDecls + low + 1, convertedDecls + low, sizeof(IDirect3DVertexDeclaration9Impl *) * (This->numConvertedDecls - low));
    convertedDecls[low] = pDecl;
    This->numConvertedDecls++;

    /* Will prevent the decl from being destroyed */
    ((IDirect3DVertexDeclaration9Impl *) pDecl)->convFVF = fvf;
    IDirect3DVertexDeclaration9_Release(pDecl); /* Does not destroy now */

    TRACE("Returning %p. %d decls in array\n", pDecl, This->numConvertedDecls);
    return pDecl;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_SetFVF(LPDIRECT3DDEVICE9 iface, DWORD FVF) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    if (0 != FVF) {
         HRESULT hr;
         IDirect3DVertexDeclaration9* pDecl = getConvertedDecl(This, FVF);

         if(!pDecl) {
             /* Any situation when this should happen, except out of memory? */
             ERR("Failed to create a converted vertex declaration\n");
             LeaveCriticalSection(&d3d9_cs);
             return D3DERR_DRIVERINTERNALERROR;
         }

         hr = IDirect3DDevice9Impl_SetVertexDeclaration(iface, pDecl);
         if (hr != S_OK) {
             LeaveCriticalSection(&d3d9_cs);
             return hr;
         }
    }
    hr = IWineD3DDevice_SetFVF(This->WineD3DDevice, FVF);
    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetFVF(LPDIRECT3DDEVICE9 iface, DWORD* pFVF) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetFVF(This->WineD3DDevice, pFVF);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_SetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetStreamSource(This->WineD3DDevice, StreamNumber,
                                          pStreamData==NULL ? NULL:((IDirect3DVertexBuffer9Impl *)pStreamData)->wineD3DVertexBuffer, 
                                          OffsetInBytes, Stride);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

HRESULT  WINAPI  IDirect3DDevice9Impl_GetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9 **pStream, UINT* OffsetInBytes, UINT* pStride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DVertexBuffer *retStream = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n" , This);

    if(pStream == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    rc = IWineD3DDevice_GetStreamSource(This->WineD3DDevice, StreamNumber, &retStream, OffsetInBytes, pStride);
    if (rc == D3D_OK  && NULL != retStream) {
        IWineD3DVertexBuffer_GetParent(retStream, (IUnknown **)pStream);
        IWineD3DVertexBuffer_Release(retStream);
    }else{
        if (rc != D3D_OK){
            FIXME("Call to GetStreamSource failed %p %p\n", OffsetInBytes, pStride);
        }
        *pStream = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);

    return rc;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, UINT Divider) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;    
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetStreamSourceFreq(This->WineD3DDevice, StreamNumber, Divider);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, UINT* Divider) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_GetStreamSourceFreq(This->WineD3DDevice, StreamNumber, Divider);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_SetIndices(This->WineD3DDevice,
            pIndexData ? ((IDirect3DIndexBuffer9Impl *)pIndexData)->wineD3DIndexBuffer : NULL);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9 **ppIndexData) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DIndexBuffer *retIndexData = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    if(ppIndexData == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d9_cs);
    rc = IWineD3DDevice_GetIndices(This->WineD3DDevice, &retIndexData);
    if (SUCCEEDED(rc) && retIndexData) {
        IWineD3DIndexBuffer_GetParent(retIndexData, (IUnknown **)ppIndexData);
        IWineD3DIndexBuffer_Release(retIndexData);
    } else {
        if (FAILED(rc)) FIXME("Call to GetIndices failed\n");
        *ppIndexData = NULL;
    }
    LeaveCriticalSection(&d3d9_cs);
    return rc;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawRectPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_DrawRectPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DRECTPATCH_INFO *)pRectPatchInfo);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}
/*http://msdn.microsoft.com/library/default.asp?url=/library/en-us/directx9_c/directx/graphics/reference/d3d/interfaces/idirect3ddevice9/DrawTriPatch.asp*/
static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawTriPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_DrawTriPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DTRIPATCH_INFO *)pTriPatchInfo);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DeletePatch(LPDIRECT3DDEVICE9 iface, UINT Handle) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_DeletePatch(This->WineD3DDevice, Handle);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

const IDirect3DDevice9Vtbl Direct3DDevice9_Vtbl =
{
    /* IUnknown */
    IDirect3DDevice9Impl_QueryInterface,
    IDirect3DDevice9Impl_AddRef,
    IDirect3DDevice9Impl_Release,
    /* IDirect3DDevice9 */
    IDirect3DDevice9Impl_TestCooperativeLevel,
    IDirect3DDevice9Impl_GetAvailableTextureMem,
    IDirect3DDevice9Impl_EvictManagedResources,
    IDirect3DDevice9Impl_GetDirect3D,
    IDirect3DDevice9Impl_GetDeviceCaps,
    IDirect3DDevice9Impl_GetDisplayMode,
    IDirect3DDevice9Impl_GetCreationParameters,
    IDirect3DDevice9Impl_SetCursorProperties,
    IDirect3DDevice9Impl_SetCursorPosition,
    IDirect3DDevice9Impl_ShowCursor,
    IDirect3DDevice9Impl_CreateAdditionalSwapChain,
    IDirect3DDevice9Impl_GetSwapChain,
    IDirect3DDevice9Impl_GetNumberOfSwapChains,
    IDirect3DDevice9Impl_Reset,
    IDirect3DDevice9Impl_Present,
    IDirect3DDevice9Impl_GetBackBuffer,
    IDirect3DDevice9Impl_GetRasterStatus,
    IDirect3DDevice9Impl_SetDialogBoxMode,
    IDirect3DDevice9Impl_SetGammaRamp,
    IDirect3DDevice9Impl_GetGammaRamp,
    IDirect3DDevice9Impl_CreateTexture,
    IDirect3DDevice9Impl_CreateVolumeTexture,
    IDirect3DDevice9Impl_CreateCubeTexture,
    IDirect3DDevice9Impl_CreateVertexBuffer,
    IDirect3DDevice9Impl_CreateIndexBuffer,
    IDirect3DDevice9Impl_CreateRenderTarget,
    IDirect3DDevice9Impl_CreateDepthStencilSurface,
    IDirect3DDevice9Impl_UpdateSurface,
    IDirect3DDevice9Impl_UpdateTexture,
    IDirect3DDevice9Impl_GetRenderTargetData,
    IDirect3DDevice9Impl_GetFrontBufferData,
    IDirect3DDevice9Impl_StretchRect,
    IDirect3DDevice9Impl_ColorFill,
    IDirect3DDevice9Impl_CreateOffscreenPlainSurface,
    IDirect3DDevice9Impl_SetRenderTarget,
    IDirect3DDevice9Impl_GetRenderTarget,
    IDirect3DDevice9Impl_SetDepthStencilSurface,
    IDirect3DDevice9Impl_GetDepthStencilSurface,
    IDirect3DDevice9Impl_BeginScene,
    IDirect3DDevice9Impl_EndScene,
    IDirect3DDevice9Impl_Clear,
    IDirect3DDevice9Impl_SetTransform,
    IDirect3DDevice9Impl_GetTransform,
    IDirect3DDevice9Impl_MultiplyTransform,
    IDirect3DDevice9Impl_SetViewport,
    IDirect3DDevice9Impl_GetViewport,
    IDirect3DDevice9Impl_SetMaterial,
    IDirect3DDevice9Impl_GetMaterial,
    IDirect3DDevice9Impl_SetLight,
    IDirect3DDevice9Impl_GetLight,
    IDirect3DDevice9Impl_LightEnable,
    IDirect3DDevice9Impl_GetLightEnable,
    IDirect3DDevice9Impl_SetClipPlane,
    IDirect3DDevice9Impl_GetClipPlane,
    IDirect3DDevice9Impl_SetRenderState,
    IDirect3DDevice9Impl_GetRenderState,
    IDirect3DDevice9Impl_CreateStateBlock,
    IDirect3DDevice9Impl_BeginStateBlock,
    IDirect3DDevice9Impl_EndStateBlock,
    IDirect3DDevice9Impl_SetClipStatus,
    IDirect3DDevice9Impl_GetClipStatus,
    IDirect3DDevice9Impl_GetTexture,
    IDirect3DDevice9Impl_SetTexture,
    IDirect3DDevice9Impl_GetTextureStageState,
    IDirect3DDevice9Impl_SetTextureStageState,
    IDirect3DDevice9Impl_GetSamplerState,
    IDirect3DDevice9Impl_SetSamplerState,
    IDirect3DDevice9Impl_ValidateDevice,
    IDirect3DDevice9Impl_SetPaletteEntries,
    IDirect3DDevice9Impl_GetPaletteEntries,
    IDirect3DDevice9Impl_SetCurrentTexturePalette,
    IDirect3DDevice9Impl_GetCurrentTexturePalette,
    IDirect3DDevice9Impl_SetScissorRect,
    IDirect3DDevice9Impl_GetScissorRect,
    IDirect3DDevice9Impl_SetSoftwareVertexProcessing,
    IDirect3DDevice9Impl_GetSoftwareVertexProcessing,
    IDirect3DDevice9Impl_SetNPatchMode,
    IDirect3DDevice9Impl_GetNPatchMode,
    IDirect3DDevice9Impl_DrawPrimitive,
    IDirect3DDevice9Impl_DrawIndexedPrimitive,
    IDirect3DDevice9Impl_DrawPrimitiveUP,
    IDirect3DDevice9Impl_DrawIndexedPrimitiveUP,
    IDirect3DDevice9Impl_ProcessVertices,
    IDirect3DDevice9Impl_CreateVertexDeclaration,
    IDirect3DDevice9Impl_SetVertexDeclaration,
    IDirect3DDevice9Impl_GetVertexDeclaration,
    IDirect3DDevice9Impl_SetFVF,
    IDirect3DDevice9Impl_GetFVF,
    IDirect3DDevice9Impl_CreateVertexShader,
    IDirect3DDevice9Impl_SetVertexShader,
    IDirect3DDevice9Impl_GetVertexShader,
    IDirect3DDevice9Impl_SetVertexShaderConstantF,
    IDirect3DDevice9Impl_GetVertexShaderConstantF,
    IDirect3DDevice9Impl_SetVertexShaderConstantI,
    IDirect3DDevice9Impl_GetVertexShaderConstantI,
    IDirect3DDevice9Impl_SetVertexShaderConstantB,
    IDirect3DDevice9Impl_GetVertexShaderConstantB,
    IDirect3DDevice9Impl_SetStreamSource,
    IDirect3DDevice9Impl_GetStreamSource,
    IDirect3DDevice9Impl_SetStreamSourceFreq,
    IDirect3DDevice9Impl_GetStreamSourceFreq,
    IDirect3DDevice9Impl_SetIndices,
    IDirect3DDevice9Impl_GetIndices,
    IDirect3DDevice9Impl_CreatePixelShader,
    IDirect3DDevice9Impl_SetPixelShader,
    IDirect3DDevice9Impl_GetPixelShader,
    IDirect3DDevice9Impl_SetPixelShaderConstantF,
    IDirect3DDevice9Impl_GetPixelShaderConstantF,
    IDirect3DDevice9Impl_SetPixelShaderConstantI,
    IDirect3DDevice9Impl_GetPixelShaderConstantI,
    IDirect3DDevice9Impl_SetPixelShaderConstantB,
    IDirect3DDevice9Impl_GetPixelShaderConstantB,
    IDirect3DDevice9Impl_DrawRectPatch,
    IDirect3DDevice9Impl_DrawTriPatch,
    IDirect3DDevice9Impl_DeletePatch,
    IDirect3DDevice9Impl_CreateQuery
};


/* Internal function called back during the CreateDevice to create a render target  */
HRESULT WINAPI D3D9CB_CreateSurface(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, DWORD Usage, WINED3DPOOL Pool, UINT Level,
                                         WINED3DCUBEMAP_FACES Face,IWineD3DSurface** ppSurface,
                                         HANDLE* pSharedHandle) {

    HRESULT res = D3D_OK;
    IDirect3DSurface9Impl *d3dSurface = NULL;
    BOOL Lockable = TRUE;
    
    if((Pool == D3DPOOL_DEFAULT &&  Usage != D3DUSAGE_DYNAMIC)) 
        Lockable = FALSE;
        
    TRACE("relay\n");
    res = IDirect3DDevice9Impl_CreateSurface((IDirect3DDevice9 *)device, Width, Height, (D3DFORMAT)Format,
                Lockable, FALSE/*Discard*/, Level,  (IDirect3DSurface9 **)&d3dSurface, D3DRTYPE_SURFACE,
                Usage, (D3DPOOL) Pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */, pSharedHandle);  

    if (SUCCEEDED(res)) {
        *ppSurface = d3dSurface->wineD3DSurface;
        d3dSurface->container = pSuperior;
        IUnknown_Release(d3dSurface->parentDevice);
        d3dSurface->parentDevice = NULL;
        d3dSurface->forwardReference = pSuperior;
    } else {
        FIXME("(%p) IDirect3DDevice9_CreateSurface failed\n", device);
    }
    return res;
}

ULONG WINAPI D3D9CB_DestroySurface(IWineD3DSurface *pSurface) {
    IDirect3DSurface9Impl* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, (IUnknown **) &surfaceParent);
    /* GetParent's AddRef was forwarded to an object in destruction.
     * Releasing it here again would cause an endless recursion. */
    surfaceParent->forwardReference = NULL;
    return IDirect3DSurface9_Release((IDirect3DSurface9*) surfaceParent);
}
