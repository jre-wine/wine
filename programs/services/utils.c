/*
 * Services.exe - some utility functions
 *
 * Copyright 2007 Google (Mikolaj Zalewski)
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

#define WIN32_LEAN_AND_MEAN

#include <stdarg.h>
#include <windows.h>

#include "wine/unicode.h"
#include "wine/debug.h"
#include "services.h"

WINE_DEFAULT_DEBUG_CHANNEL(service);

LPWSTR strdupW(LPCWSTR str)
{
    int len;
    WCHAR *buf;

    if (str == NULL)
        return NULL;
    len = strlenW(str);
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*(len+1));
    if (buf == NULL)
        return NULL;
    strcpyW(buf, str);
    return buf;
}

BOOL check_multisz(LPCWSTR lpMultiSz, DWORD cbSize)
{
    if (cbSize == 0 || (cbSize == sizeof(WCHAR) && lpMultiSz[0] == 0))
        return TRUE;
    if ((cbSize % sizeof(WCHAR)) != 0 || cbSize < 2*sizeof(WCHAR))
        return FALSE;
    if (lpMultiSz[cbSize/2 - 1] || lpMultiSz[cbSize/2 - 2])
        return FALSE;
    return TRUE;
}

DWORD load_reg_string(HKEY hKey, LPCWSTR szValue, BOOL bExpand, LPWSTR *output)
{
    DWORD size, type;
    LPWSTR buf = NULL;
    DWORD err;

    *output = NULL;
    if ((err = RegQueryValueExW(hKey, szValue, 0, &type, NULL, &size)) != 0)
    {
        if (err == ERROR_FILE_NOT_FOUND)
            return ERROR_SUCCESS;
        goto failed;
    }
    if (!(type == REG_SZ || (type == REG_EXPAND_SZ && bExpand)))
    {
        err = ERROR_INVALID_DATATYPE;
        goto failed;
    }
    buf = HeapAlloc(GetProcessHeap(), 0, size + sizeof(WCHAR));
    if ((err = RegQueryValueExW(hKey, szValue, 0, &type, (LPBYTE)buf, &size)) != 0)
        goto failed;
    buf[size/sizeof(WCHAR)] = 0;

    if (type == REG_EXPAND_SZ)
    {
        LPWSTR str;
        DWORD size = ExpandEnvironmentStringsW(buf, NULL, 0);
        if (size == 0)
        {
            err = GetLastError();
            goto failed;
        }
        str = HeapAlloc(GetProcessHeap(), 0, size * sizeof(WCHAR));
        ExpandEnvironmentStringsW(buf, str, size);
        HeapFree(GetProcessHeap(), 0, buf);
        *output = str;
    }
    else
        *output = buf;
    return ERROR_SUCCESS;

failed:
    WINE_ERR("Error %d while reading value %s\n", err, wine_dbgstr_w(szValue));
    HeapFree(GetProcessHeap(), 0, buf);
    return err;
}

DWORD load_reg_multisz(HKEY hKey, LPCWSTR szValue, BOOL bAllowSingle, LPWSTR *output)
{
    DWORD size, type;
    LPWSTR buf = NULL;
    DWORD err;

    *output = NULL;
    if ((err = RegQueryValueExW(hKey, szValue, 0, &type, NULL, &size)) != 0)
    {
        if (err == ERROR_FILE_NOT_FOUND)
        {
            *output = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR));
            return ERROR_SUCCESS;
        }
        goto failed;
    }
    if (!((type == REG_MULTI_SZ) || ((type == REG_SZ) && bAllowSingle)))
    {
        err = ERROR_INVALID_DATATYPE;
        goto failed;
    }
    buf = HeapAlloc(GetProcessHeap(), 0, size + 2*sizeof(WCHAR));
    if ((err = RegQueryValueExW(hKey, szValue, 0, &type, (LPBYTE)buf, &size)) != 0)
        goto failed;
    buf[size/sizeof(WCHAR)] = 0;
    buf[size/sizeof(WCHAR) + 1] = 0;
    *output = buf;
    return ERROR_SUCCESS;

failed:
    WINE_ERR("Error %d while reading value %s\n", err, wine_dbgstr_w(szValue));
    HeapFree(GetProcessHeap(), 0, buf);
    return err;
}

DWORD load_reg_dword(HKEY hKey, LPCWSTR szValue, DWORD *output)
{
    DWORD size, type;
    DWORD err;

    *output = 0;
    size = sizeof(DWORD);
    if ((err = RegQueryValueExW(hKey, szValue, 0, &type, (LPBYTE)output, &size)) != 0)
    {
        if (err == ERROR_FILE_NOT_FOUND)
            return ERROR_SUCCESS;
        goto failed;
    }
    if (type != REG_DWORD || size != sizeof(DWORD))
    {
        err = ERROR_INVALID_DATATYPE;
        goto failed;
    }
    return ERROR_SUCCESS;

failed:
    WINE_ERR("Error %d while reading value %s\n", err, wine_dbgstr_w(szValue));
    return err;
}
