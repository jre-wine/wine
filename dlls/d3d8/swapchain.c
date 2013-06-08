/*
 * IDirect3DSwapChain8 implementation
 *
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline struct d3d8_swapchain *impl_from_IDirect3DSwapChain8(IDirect3DSwapChain8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_swapchain, IDirect3DSwapChain8_iface);
}

static HRESULT WINAPI d3d8_swapchain_QueryInterface(IDirect3DSwapChain8 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DSwapChain8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DSwapChain8_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_swapchain_AddRef(IDirect3DSwapChain8 *iface)
{
    struct d3d8_swapchain *swapchain = impl_from_IDirect3DSwapChain8(iface);
    ULONG ref = InterlockedIncrement(&swapchain->refcount);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        if (swapchain->parent_device)
            IDirect3DDevice8_AddRef(swapchain->parent_device);
        wined3d_mutex_lock();
        wined3d_swapchain_incref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI d3d8_swapchain_Release(IDirect3DSwapChain8 *iface)
{
    struct d3d8_swapchain *swapchain = impl_from_IDirect3DSwapChain8(iface);
    ULONG ref = InterlockedDecrement(&swapchain->refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice8 *parent_device = swapchain->parent_device;

        wined3d_mutex_lock();
        wined3d_swapchain_decref(swapchain->wined3d_swapchain);
        wined3d_mutex_unlock();

        if (parent_device)
            IDirect3DDevice8_Release(parent_device);
    }
    return ref;
}

static HRESULT WINAPI d3d8_swapchain_Present(IDirect3DSwapChain8 *iface,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region)
{
    struct d3d8_swapchain *swapchain = impl_from_IDirect3DSwapChain8(iface);
    HRESULT hr;

    TRACE("iface %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p.\n",
            iface, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect), dst_window_override, dirty_region);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_present(swapchain->wined3d_swapchain, src_rect,
            dst_rect, dst_window_override, dirty_region, 0);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_swapchain_GetBackBuffer(IDirect3DSwapChain8 *iface,
        UINT backbuffer_idx, D3DBACKBUFFER_TYPE backbuffer_type, IDirect3DSurface8 **backbuffer)
{
    struct d3d8_swapchain *swapchain = impl_from_IDirect3DSwapChain8(iface);
    struct wined3d_surface *wined3d_surface = NULL;
    struct d3d8_surface *surface_impl;
    HRESULT hr;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, backbuffer_idx, backbuffer_type, backbuffer);

    wined3d_mutex_lock();
    hr = wined3d_swapchain_get_back_buffer(swapchain->wined3d_swapchain,
            backbuffer_idx, (enum wined3d_backbuffer_type)backbuffer_type, &wined3d_surface);
    if (SUCCEEDED(hr) && wined3d_surface)
    {
        surface_impl = wined3d_surface_get_parent(wined3d_surface);
        *backbuffer = &surface_impl->IDirect3DSurface8_iface;
        IDirect3DSurface8_AddRef(*backbuffer);
        wined3d_surface_decref(wined3d_surface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DSwapChain8Vtbl d3d8_swapchain_vtbl =
{
    d3d8_swapchain_QueryInterface,
    d3d8_swapchain_AddRef,
    d3d8_swapchain_Release,
    d3d8_swapchain_Present,
    d3d8_swapchain_GetBackBuffer
};

static void STDMETHODCALLTYPE d3d8_swapchain_wined3d_object_released(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_swapchain_wined3d_parent_ops =
{
    d3d8_swapchain_wined3d_object_released,
};

static HRESULT swapchain_init(struct d3d8_swapchain *swapchain, struct d3d8_device *device,
        struct wined3d_swapchain_desc *desc)
{
    HRESULT hr;

    swapchain->refcount = 1;
    swapchain->IDirect3DSwapChain8_iface.lpVtbl = &d3d8_swapchain_vtbl;

    wined3d_mutex_lock();
    hr = wined3d_swapchain_create(device->wined3d_device, desc,
            WINED3D_SURFACE_TYPE_OPENGL, swapchain, &d3d8_swapchain_wined3d_parent_ops,
            &swapchain->wined3d_swapchain);
    wined3d_mutex_unlock();

    if (FAILED(hr))
    {
        WARN("Failed to create wined3d swapchain, hr %#x.\n", hr);
        return hr;
    }

    swapchain->parent_device = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(swapchain->parent_device);

    return D3D_OK;
}

HRESULT d3d8_swapchain_create(struct d3d8_device *device, struct wined3d_swapchain_desc *desc,
        struct d3d8_swapchain **swapchain)
{
    struct d3d8_swapchain *object;
    HRESULT hr;

    if (!(object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object))))
    {
        ERR("Failed to allocate swapchain memory.\n");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr = swapchain_init(object, device, desc)))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);
    *swapchain = object;

    return D3D_OK;
}
