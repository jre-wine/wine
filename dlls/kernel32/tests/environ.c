/*
 * Unit test suite for environment functions.
 *
 * Copyright 2002 Dmitry Timoshkov
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

#include <stdarg.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

static void test_GetSetEnvironmentVariableA(void)
{
    char buf[256];
    BOOL ret;
    DWORD ret_size;
    static const char name[] = "SomeWildName";
    static const char name_cased[] = "sOMEwILDnAME";
    static const char value[] = "SomeWildValue";

    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableA, GetLastError=%d\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    ret_size = GetEnvironmentVariableA(name, NULL, 0);
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%d\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value));
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(ret_size == strlen(value) + 1,
       "should return length with terminating 0 ret_size=%d\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    /* Remove that environment variable */
    ret = SetEnvironmentVariableA(name_cased, NULL);
    ok(ret == TRUE, "should erase existing variable\n");

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, "foo") == 0, "should not touch the buffer\n");
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    /* Check behavior of SetEnvironmentVariableA(name, "") */
    ret = SetEnvironmentVariableA(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableA, GetLastError=%d\n",
       GetLastError());

    lstrcpyA(buf, "foo");
    ret_size = GetEnvironmentVariableA(name_cased, buf, lstrlenA(value) + 1);
    ok(lstrcmpA(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == strlen(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    ret = SetEnvironmentVariableA(name_cased, "");
    ok(ret == TRUE,
       "should not fail with empty value but GetLastError=%d\n", GetLastError());

    lstrcpyA(buf, "foo");
    SetLastError(0);
    ret_size = GetEnvironmentVariableA(name, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 &&
       ((GetLastError() == 0 && lstrcmpA(buf, "") == 0) ||
        (GetLastError() == ERROR_ENVVAR_NOT_FOUND)),
       "%s should be set to \"\" (NT) or removed (Win9x) but ret_size=%d GetLastError=%d and buf=%s\n",
       name, ret_size, GetLastError(), buf);

    /* Test the limits */
    ret_size = GetEnvironmentVariableA(NULL, NULL, 0);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableA(NULL, buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableA("", buf, lstrlenA(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());
}

static void test_GetSetEnvironmentVariableW(void)
{
    WCHAR buf[256];
    BOOL ret;
    DWORD ret_size;
    static const WCHAR name[] = {'S','o','m','e','W','i','l','d','N','a','m','e',0};
    static const WCHAR value[] = {'S','o','m','e','W','i','l','d','V','a','l','u','e',0};
    static const WCHAR name_cased[] = {'s','O','M','E','w','I','L','D','n','A','M','E',0};
    static const WCHAR empty_strW[] = { 0 };
    static const WCHAR fooW[] = {'f','o','o',0};

    ret = SetEnvironmentVariableW(name, value);
    if (ret == FALSE && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* Must be Win9x which doesn't support the Unicode functions */
        return;
    }
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableW, GetLastError=%d\n",
       GetLastError());

    /* Try to retrieve the environment variable we just set */
    ret_size = GetEnvironmentVariableW(name, NULL, 0);
    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%d\n",
       ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value));
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");

    ok(ret_size == lstrlenW(value) + 1,
       "should return length with terminating 0 ret_size=%d\n", ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name_cased, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    /* Remove that environment variable */
    ret = SetEnvironmentVariableW(name_cased, NULL);
    ok(ret == TRUE, "should erase existing variable\n");

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, fooW) == 0, "should not touch the buffer\n");
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    /* Check behavior of SetEnvironmentVariableW(name, "") */
    ret = SetEnvironmentVariableW(name, value);
    ok(ret == TRUE,
       "unexpected error in SetEnvironmentVariableW, GetLastError=%d\n",
       GetLastError());

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(lstrcmpW(buf, value) == 0, "should touch the buffer\n");
    ok(ret_size == lstrlenW(value),
       "should return length without terminating 0 ret_size=%d\n", ret_size);

    ret = SetEnvironmentVariableW(name_cased, empty_strW);
    ok(ret == TRUE, "should not fail with empty value but GetLastError=%d\n", GetLastError());

    lstrcpyW(buf, fooW);
    ret_size = GetEnvironmentVariableW(name, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());
    ok(lstrcmpW(buf, empty_strW) == 0, "should copy an empty string\n");

    /* Test the limits */
    ret_size = GetEnvironmentVariableW(NULL, NULL, 0);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    ret_size = GetEnvironmentVariableW(NULL, buf, lstrlenW(value) + 1);
    ok(ret_size == 0 && GetLastError() == ERROR_ENVVAR_NOT_FOUND,
       "should not find variable but ret_size=%d GetLastError=%d\n",
       ret_size, GetLastError());

    ret = SetEnvironmentVariableW(NULL, NULL);
    ok(ret == FALSE && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ENVVAR_NOT_FOUND),
       "should fail with NULL, NULL but ret=%d and GetLastError=%d\n",
       ret, GetLastError());
}

static void test_ExpandEnvironmentStringsA(void)
{
    char buf[256], buf1[256], buf2[0x8000];
    DWORD ret_size, ret_size1;

    /* test a large destination size */
    strcpy(buf, "12345");
    ret_size = ExpandEnvironmentStringsA(buf, buf2, sizeof(buf2));
    ok(!strcmp(buf, buf2), "ExpandEnvironmentStrings failed %s vs %s. ret_size = %d\n", buf, buf2, ret_size);

    ret_size1 = GetWindowsDirectoryA(buf1,256);
    ok ((ret_size1 >0) && (ret_size1<256), "GetWindowsDirectory Failed\n");
    ret_size = ExpandEnvironmentStringsA("%SystemRoot%",buf,sizeof(buf));
    if (ERROR_ENVVAR_NOT_FOUND == GetLastError())
        return;
    ok(!strcmp(buf, buf1), "ExpandEnvironmentStrings failed %s vs %s. ret_size = %d\n", buf, buf1, ret_size);
}

static BOOL (WINAPI *pGetComputerNameExA)(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
static BOOL (WINAPI *pGetComputerNameExW)(COMPUTER_NAME_FORMAT,LPWSTR,LPDWORD);

static void test_GetComputerName(void)
{
    DWORD size;
    BOOL ret;
    LPSTR name;
    LPWSTR nameW;
    DWORD error;

    size = 0;
    ret = GetComputerNameA((LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameA should have failed with ERROR_MORE_DATA instead of %d\n", error);
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = GetComputerNameA(name, &size);
    ok(ret, "GetComputerNameA failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = GetComputerNameW((LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = GetComputerNameW(nameW, &size);
    ok(ret, "GetComputerNameW failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    pGetComputerNameExA = GetProcAddress(GetModuleHandle("kernel32.dll"), "GetComputerNameExA");
    if (!pGetComputerNameExA)
    {
        skip("GetComputerNameExA function not implemented, so not testing\n");
        return;
    }

    size = 0;
    ret = pGetComputerNameExA(ComputerNameDnsDomain, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExA should have failed with ERROR_MORE_DATA instead of %d\n", error);
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsDomain, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsDomain) failed with error %d\n", GetLastError());
    trace("domain name is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = pGetComputerNameExA(ComputerNameDnsFullyQualified, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExA should have failed with ERROR_MORE_DATA instead of %d\n", error);
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsFullyQualified, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsFullyQualified) failed with error %d\n", GetLastError());
    trace("fully qualified hostname is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = pGetComputerNameExA(ComputerNameDnsHostname, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExA should have failed with ERROR_MORE_DATA instead of %d\n", error);
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameDnsHostname, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameDnsHostname) failed with error %d\n", GetLastError());
    trace("hostname is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    size = 0;
    ret = pGetComputerNameExA(ComputerNameNetBIOS, (LPSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExA should have failed with ERROR_MORE_DATA instead of %d\n", error);
    name = HeapAlloc(GetProcessHeap(), 0, size * sizeof(name[0]));
    ok(name != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExA(ComputerNameNetBIOS, name, &size);
    ok(ret, "GetComputerNameExA(ComputerNameNetBIOS) failed with error %d\n", GetLastError());
    trace("NetBIOS name is \"%s\"\n", name);
    HeapFree(GetProcessHeap(), 0, name);

    pGetComputerNameExW = GetProcAddress(GetModuleHandle("kernel32.dll"), "GetComputerNameExW");
    if (!pGetComputerNameExW)
    {
        skip("GetComputerNameExW function not implemented, so not testing\n");
        return;
    }

    size = 0;
    ret = pGetComputerNameExW(ComputerNameDnsDomain, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsDomain, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsDomain) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    ret = pGetComputerNameExW(ComputerNameDnsFullyQualified, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsFullyQualified, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsFullyQualified) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    ret = pGetComputerNameExW(ComputerNameDnsHostname, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameDnsHostname, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameDnsHostname) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);

    size = 0;
    ret = pGetComputerNameExW(ComputerNameNetBIOS, (LPWSTR)0xdeadbeef, &size);
    error = GetLastError();
    ok(!ret && error == ERROR_MORE_DATA, "GetComputerNameExW should have failed with ERROR_MORE_DATA instead of %d\n", error);
    nameW = HeapAlloc(GetProcessHeap(), 0, size * sizeof(nameW[0]));
    ok(nameW != NULL, "HeapAlloc failed with error %d\n", GetLastError());
    ret = pGetComputerNameExW(ComputerNameNetBIOS, nameW, &size);
    ok(ret, "GetComputerNameExW(ComputerNameNetBIOS) failed with error %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, nameW);
}

START_TEST(environ)
{
    test_GetSetEnvironmentVariableA();
    test_GetSetEnvironmentVariableW();
    test_ExpandEnvironmentStringsA();
    test_GetComputerName();
}
