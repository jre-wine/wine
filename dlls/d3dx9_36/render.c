/*
 * Copyright (C) 2012 Józef Kucia
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
 *
 */

#include "wine/debug.h"
#include "d3dx9_36_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3dx);

struct render_to_surface
{
    ID3DXRenderToSurface ID3DXRenderToSurface_iface;
    LONG ref;

    IDirect3DDevice9 *device;
    D3DXRTS_DESC desc;

    IDirect3DSurface9 *dst_surface;

    IDirect3DSurface9 *render_target;
    IDirect3DSurface9 *depth_stencil;

    DWORD num_render_targets;
    D3DVIEWPORT9 previous_viewport;
    IDirect3DSurface9 **previous_render_targets;
    IDirect3DSurface9 *previous_depth_stencil;
};

static inline struct render_to_surface *impl_from_ID3DXRenderToSurface(ID3DXRenderToSurface *iface)
{
    return CONTAINING_RECORD(iface, struct render_to_surface, ID3DXRenderToSurface_iface);
}

static void restore_previous_device_state(struct render_to_surface *render)
{
    unsigned int i;

    for (i = 0; i < render->num_render_targets; i++)
    {
        IDirect3DDevice9_SetRenderTarget(render->device, i, render->previous_render_targets[i]);
        if (render->previous_render_targets[i])
            IDirect3DSurface9_Release(render->previous_render_targets[i]);
        render->previous_render_targets[i] = NULL;
    }

    IDirect3DDevice9_SetDepthStencilSurface(render->device, render->previous_depth_stencil);
    if (render->previous_depth_stencil)
    {
        IDirect3DSurface9_Release(render->previous_depth_stencil);
        render->previous_depth_stencil = NULL;
    }

    IDirect3DDevice9_SetViewport(render->device, &render->previous_viewport);
}

static HRESULT WINAPI D3DXRenderToSurface_QueryInterface(ID3DXRenderToSurface *iface,
                                                         REFIID riid,
                                                         void **out)
{
    TRACE("iface %p, riid %s, out %p\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3DXRenderToSurface)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI D3DXRenderToSurface_AddRef(ID3DXRenderToSurface *iface)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    ULONG ref = InterlockedIncrement(&render->ref);

    TRACE("%p increasing refcount to %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI D3DXRenderToSurface_Release(ID3DXRenderToSurface *iface)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    ULONG ref = InterlockedDecrement(&render->ref);
    unsigned int i;

    TRACE("%p decreasing refcount to %u\n", iface, ref);

    if (!ref)
    {
        if (render->dst_surface) IDirect3DSurface9_Release(render->dst_surface);

        if (render->render_target) IDirect3DSurface9_Release(render->render_target);
        if (render->depth_stencil) IDirect3DSurface9_Release(render->depth_stencil);

        for (i = 0; i < render->num_render_targets; i++)
        {
            if (render->previous_render_targets[i])
                IDirect3DSurface9_Release(render->previous_render_targets[i]);
        }

        HeapFree(GetProcessHeap(), 0, render->previous_render_targets);

        if (render->previous_depth_stencil) IDirect3DSurface9_Release(render->previous_depth_stencil);

        IDirect3DDevice9_Release(render->device);

        HeapFree(GetProcessHeap(), 0, render);
    }

    return ref;
}

static HRESULT WINAPI D3DXRenderToSurface_GetDevice(ID3DXRenderToSurface *iface,
                                                    IDirect3DDevice9 **device)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);

    TRACE("(%p)->(%p)\n", iface, device);

    if (!device) return D3DERR_INVALIDCALL;

    IDirect3DDevice9_AddRef(render->device);
    *device = render->device;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_GetDesc(ID3DXRenderToSurface *iface,
                                                  D3DXRTS_DESC *desc)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);

    TRACE("(%p)->(%p)\n", iface, desc);

    if (!desc) return D3DERR_INVALIDCALL;

    *desc = render->desc;
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_BeginScene(ID3DXRenderToSurface *iface,
                                                     IDirect3DSurface9 *surface,
                                                     const D3DVIEWPORT9 *viewport)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    unsigned int i;
    IDirect3DDevice9 *device;
    D3DSURFACE_DESC surface_desc;
    HRESULT hr = D3DERR_INVALIDCALL;
    D3DMULTISAMPLE_TYPE multi_sample_type = D3DMULTISAMPLE_NONE;
    DWORD multi_sample_quality = 0;

    TRACE("(%p)->(%p, %p)\n", iface, surface, viewport);

    if (!surface || render->dst_surface) return D3DERR_INVALIDCALL;

    IDirect3DSurface9_GetDesc(surface, &surface_desc);
    if (surface_desc.Format != render->desc.Format
            || surface_desc.Width != render->desc.Width
            || surface_desc.Height != render->desc.Height)
        return D3DERR_INVALIDCALL;

    if (viewport)
    {
        if (viewport->X > render->desc.Width || viewport->Y > render->desc.Height
                || viewport->X + viewport->Width > render->desc.Width
                || viewport->Y + viewport->Height > render->desc.Height)
            return D3DERR_INVALIDCALL;

        if (!(surface_desc.Usage & D3DUSAGE_RENDERTARGET)
                && (viewport->X != 0 || viewport->Y != 0
                || viewport->Width != render->desc.Width
                || viewport->Height != render->desc.Height))
            return D3DERR_INVALIDCALL;
    }

    device = render->device;

    /* save device state */
    IDirect3DDevice9_GetViewport(device, &render->previous_viewport);

    for (i = 0; i < render->num_render_targets; i++)
    {
        hr = IDirect3DDevice9_GetRenderTarget(device, i, &render->previous_render_targets[i]);
        if (FAILED(hr)) render->previous_render_targets[i] = NULL;
    }

    hr = IDirect3DDevice9_GetDepthStencilSurface(device, &render->previous_depth_stencil);
    if (FAILED(hr)) render->previous_depth_stencil = NULL;

    /* prepare for rendering to surface */
    for (i = 1; i < render->num_render_targets; i++)
        IDirect3DDevice9_SetRenderTarget(device, i, NULL);

    if (surface_desc.Usage & D3DUSAGE_RENDERTARGET)
    {
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, surface);
        multi_sample_type = surface_desc.MultiSampleType;
        multi_sample_quality = surface_desc.MultiSampleQuality;
    }
    else
    {
        hr = IDirect3DDevice9_CreateRenderTarget(device, render->desc.Width, render->desc.Height,
                render->desc.Format, multi_sample_type, multi_sample_quality, FALSE,
                &render->render_target, NULL);
        if (FAILED(hr)) goto cleanup;
        hr = IDirect3DDevice9_SetRenderTarget(device, 0, render->render_target);
    }

    if (FAILED(hr)) goto cleanup;

    if (render->desc.DepthStencil)
    {
        hr = IDirect3DDevice9_CreateDepthStencilSurface(device, render->desc.Width, render->desc.Height,
                render->desc.DepthStencilFormat, multi_sample_type, multi_sample_quality, TRUE,
                &render->depth_stencil, NULL);
    }
    else render->depth_stencil = NULL;

    if (FAILED(hr)) goto cleanup;

    hr = IDirect3DDevice9_SetDepthStencilSurface(device, render->depth_stencil);
    if (FAILED(hr)) goto cleanup;

    if (viewport) IDirect3DDevice9_SetViewport(device, viewport);

    IDirect3DSurface9_AddRef(surface);
    render->dst_surface = surface;
    return IDirect3DDevice9_BeginScene(device);

cleanup:
    restore_previous_device_state(render);

    if (render->dst_surface) IDirect3DSurface9_Release(render->dst_surface);
    render->dst_surface = NULL;

    if (render->render_target) IDirect3DSurface9_Release(render->render_target);
    render->render_target = NULL;
    if (render->depth_stencil) IDirect3DSurface9_Release(render->depth_stencil);
    render->depth_stencil = NULL;

    return hr;
}

static HRESULT WINAPI D3DXRenderToSurface_EndScene(ID3DXRenderToSurface *iface,
                                                   DWORD filter)
{
    struct render_to_surface *render = impl_from_ID3DXRenderToSurface(iface);
    HRESULT hr;

    TRACE("(%p)->(%#x)\n", iface, filter);

    if (!render->dst_surface) return D3DERR_INVALIDCALL;

    hr = IDirect3DDevice9_EndScene(render->device);

    /* copy render target data to destination surface, if needed */
    if (render->render_target)
    {
        hr = D3DXLoadSurfaceFromSurface(render->dst_surface, NULL, NULL,
                render->render_target, NULL, NULL, filter, 0);
        if (FAILED(hr)) ERR("Copying render target data to surface failed %#x\n", hr);
    }

    restore_previous_device_state(render);

    /* release resources */
    if (render->render_target)
    {
        IDirect3DSurface9_Release(render->render_target);
        render->render_target = NULL;
    }

    if (render->depth_stencil)
    {
        IDirect3DSurface9_Release(render->depth_stencil);
        render->depth_stencil = NULL;
    }

    IDirect3DSurface9_Release(render->dst_surface);
    render->dst_surface = NULL;

    return hr;
}

static HRESULT WINAPI D3DXRenderToSurface_OnLostDevice(ID3DXRenderToSurface *iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return D3D_OK;
}

static HRESULT WINAPI D3DXRenderToSurface_OnResetDevice(ID3DXRenderToSurface *iface)
{
    FIXME("(%p)->(): stub\n", iface);
    return D3D_OK;
}

static const ID3DXRenderToSurfaceVtbl d3dx_render_to_surface_vtbl =
{
    /* IUnknown methods */
    D3DXRenderToSurface_QueryInterface,
    D3DXRenderToSurface_AddRef,
    D3DXRenderToSurface_Release,
    /* ID3DXRenderToSurface methods */
    D3DXRenderToSurface_GetDevice,
    D3DXRenderToSurface_GetDesc,
    D3DXRenderToSurface_BeginScene,
    D3DXRenderToSurface_EndScene,
    D3DXRenderToSurface_OnLostDevice,
    D3DXRenderToSurface_OnResetDevice
};

HRESULT WINAPI D3DXCreateRenderToSurface(IDirect3DDevice9 *device,
                                         UINT width,
                                         UINT height,
                                         D3DFORMAT format,
                                         BOOL depth_stencil,
                                         D3DFORMAT depth_stencil_format,
                                         ID3DXRenderToSurface **out)
{
    HRESULT hr;
    D3DCAPS9 caps;
    struct render_to_surface *render;
    unsigned int i;

    TRACE("(%p, %u, %u, %#x, %d, %#x, %p)\n", device, width, height, format,
            depth_stencil, depth_stencil_format, out);

    if (!device || !out) return D3DERR_INVALIDCALL;

    hr = IDirect3DDevice9_GetDeviceCaps(device, &caps);
    if (FAILED(hr)) return hr;

    render = HeapAlloc(GetProcessHeap(), 0, sizeof(struct render_to_surface));
    if (!render) return E_OUTOFMEMORY;

    render->ID3DXRenderToSurface_iface.lpVtbl = &d3dx_render_to_surface_vtbl;
    render->ref = 1;

    render->desc.Width = width;
    render->desc.Height = height;
    render->desc.Format = format;
    render->desc.DepthStencil = depth_stencil;
    render->desc.DepthStencilFormat = depth_stencil_format;

    render->dst_surface = NULL;
    render->render_target = NULL;
    render->depth_stencil = NULL;

    render->num_render_targets = caps.NumSimultaneousRTs;
    render->previous_render_targets = HeapAlloc(GetProcessHeap(), 0,
            render->num_render_targets * sizeof(IDirect3DSurface9 *));
    if (!render->previous_render_targets)
    {
        HeapFree(GetProcessHeap(), 0, render);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < render->num_render_targets; i++)
        render->previous_render_targets[i] = NULL;
    render->previous_depth_stencil = NULL;

    IDirect3DDevice9_AddRef(device);
    render->device = device;

    *out = &render->ID3DXRenderToSurface_iface;
    return D3D_OK;
}
