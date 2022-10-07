// DriverBindingTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <SetupAPI.h>
#include <iostream>
#include <initguid.h>
#include <devpkey.h>
#include <string>
#include <list>
#include <tchar.h>
#include <newdev.h>
using namespace std;
#pragma comment( lib, "setupapi.lib" )
#pragma comment( lib, "newdev.lib" )

#define BIG_BUFFER_SIZE     4096
#define SMALL_BUFFER_SIZE   256
#define FORMAT_16_BYTES     _T("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X")
#define FORMAT_SYSTEMTIME   _T("%04d/%02d/%02d %02d:%02d:%02d.%03d")

typedef std::basic_string<TCHAR> tstring;

__inline void ParsePropertyToString(BYTE* data, DEVPROPTYPE type, tstring& result)
{
    switch (type)
    {
    case DEVPROP_TYPE_BOOLEAN:
    {
        result = (data[0] > 0) ? _T("TRUE") : _T("FALSE");
    }
    break;

    case DEVPROP_TYPE_SBYTE:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((char*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_BYTE:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((unsigned char*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_INT16:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((short*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_UINT16:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((unsigned short*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_INT32:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((int*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_UINT32:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%d"), ((unsigned int*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_INT64:
    case DEVPROP_TYPE_CURRENCY:     //64bit signed int
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%lld"), ((INT64*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_UINT64:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%lld"), ((UINT64*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_FLOAT:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%f"), ((float*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_DOUBLE:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, _T("%lf"), ((double*)data)[0]);
        result = buf;
    }
    break;
    case DEVPROP_TYPE_DECIMAL:      //128bit int
    case DEVPROP_TYPE_GUID:
    {
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, FORMAT_16_BYTES,
            ((TCHAR*)data)[0], ((TCHAR*)data)[1], ((TCHAR*)data)[2], ((TCHAR*)data)[3],
            ((TCHAR*)data)[4], ((TCHAR*)data)[5], ((TCHAR*)data)[6], ((TCHAR*)data)[7],
            ((TCHAR*)data)[8], ((TCHAR*)data)[9], ((TCHAR*)data)[10], ((TCHAR*)data)[11],
            ((TCHAR*)data)[12], ((TCHAR*)data)[13], ((TCHAR*)data)[14], ((TCHAR*)data)[15]);
        result = buf;
    }
    break;

    case DEVPROP_TYPE_DATE:     //DATE is OLE Automation date. 1899/12/30 0:0:0 ==> 0.00, 1900/01/01 0:0:0 is 2.00
    {
        //todo: implement this convertion
    }
    break;
    case DEVPROP_TYPE_FILETIME:
    {
        SYSTEMTIME systime = { 0 };
        FileTimeToSystemTime((FILETIME*)data, &systime);
        TCHAR buf[SMALL_BUFFER_SIZE] = { 0 };
        _stprintf_s(buf, SMALL_BUFFER_SIZE, FORMAT_SYSTEMTIME,
            systime.wYear, systime.wMonth, systime.wDay,
            systime.wHour, systime.wMinute, systime.wSecond, systime.wMilliseconds);
        result = buf;
    }
    break;

    case DEVPROP_TYPE_STRING:
    {
        result = (TCHAR*)data;
    }
    break;

    case DEVPROP_TYPE_STRING_LIST:
    {
        TCHAR* ptr = (TCHAR*)data;
        while (ptr[0] != _T('\0'))
        {
            size_t len = _tcslen(ptr);
            if (0 == len)
                break;
            if (result.size() > 0)
                result += _T(",");
            result += ptr;
            ptr += (len + 1);
        }
    }
    break;
    }

}

__inline void GetDevicePropertry(HDEVINFO infoset, PSP_DEVINFO_DATA infodata,
    CONST DEVPROPKEY* key, DEVPROPTYPE type, tstring& result)
{
    BYTE buffer[BIG_BUFFER_SIZE] = { 0 };
    BOOL ok = SetupDiGetDeviceProperty(infoset, infodata, key, &type, buffer, BIG_BUFFER_SIZE, NULL, 0);
    if (!ok)
    {
        result = _T("");
        return;
    }
    ParsePropertyToString(buffer, type, result);
}

void BindDriver(wstring hwid, wstring inf_path)
{
    DWORD error = 0;
    BOOL reboot = false;

    SetupSetNonInteractiveMode(true);

    if (!UpdateDriverForPlugAndPlayDevicesW(NULL, hwid.c_str(), inf_path.c_str(), INSTALLFLAG_FORCE, &reboot))
    {
        error = GetLastError();
        wprintf(L"  Error code=%d\n", error);
    }
    wprintf(L"  Installed INF=%s\n", inf_path.c_str());
}

BOOLEAN IsBuiltinNVMeControllerDriver(wstring devpath, wstring& hwid)
{
    HDEVINFO infoset;
    SP_DEVINFO_DATA infodata;
    SP_DEVICE_INTERFACE_DATA devif;
    DWORD error = 0;
    DWORD need_size;
    int err = 0;
    DEVPROPTYPE PropType = 0;
    WCHAR Buffer[4096] = { 0 };
    wstring current_infname;
    size_t comma_index = 0;
    BOOLEAN result = false;

    infoset = SetupDiCreateDeviceInfoList(NULL, NULL);
    if (INVALID_HANDLE_VALUE == infoset) {
        error = GetLastError();
        goto out;
    }

    ZeroMemory(&devif, sizeof(SP_DEVICE_INTERFACE_DATA));
    devif.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    if (!SetupDiOpenDeviceInterface(infoset, devpath.c_str(), 0, &devif)) {
        error = GetLastError();
        goto out;
    }

    ZeroMemory(&infodata, sizeof(SP_DEVINFO_DATA));
    infodata.cbSize = sizeof(SP_DEVINFO_DATA);

    if (!SetupDiGetDeviceInterfaceDetail(infoset, &devif, NULL, 0, &need_size, &infodata)) {
        error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER) {
            goto out;
        }
    }

    GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_DriverInfPath, PropType, current_infname);
    wprintf(L"  Current INF Name=%s\n", current_infname.c_str());
    GetDevicePropertry(infoset, &infodata, &DEVPKEY_Device_HardwareIds, PropType, hwid);
    comma_index = hwid.find(',');
    hwid = hwid.substr(0, comma_index);
    wprintf(L"  Hardware Id=%s\n", hwid.c_str());
    if (current_infname == L"stornvme.inf")
    {
        wprintf(L"  Built-in NVMe Controller Driver\n");
        result = true;
    }

    SetupDiDeleteDeviceInterfaceData(infoset, &devif);
    error = 0;

out:
    if (error != 0)
        wprintf(L"error occurred, LastError=%d\n", error);
    if (infoset != NULL && infoset != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(infoset);

    return result;
}

BOOLEAN ListStorportAdapters(list<wstring>& result)
{
    HDEVINFO devinfo = NULL;
    GUID disk_class_guid = GUID_DEVINTERFACE_STORAGEPORT;

    devinfo = SetupDiGetClassDevs(&disk_class_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (INVALID_HANDLE_VALUE != devinfo)
    {
        SP_DEVICE_INTERFACE_DATA ifdata = { 0 };
        ifdata.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        DWORD devid = 0;
        while (TRUE == SetupDiEnumDeviceInterfaces(devinfo, NULL, &disk_class_guid, devid, &ifdata))
        {
            DWORD need_size = 0;
            DWORD return_size = 0;
            BOOL ok = FALSE;
            PSP_DEVICE_INTERFACE_DETAIL_DATA ifdetail = NULL;
            devid++;
            SetupDiGetDeviceInterfaceDetail(devinfo, &ifdata, NULL, 0, &need_size, NULL);
            need_size = need_size * 2;
            BYTE buffer[1024] = { 0 };

            ifdetail = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
            ifdetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            ok = SetupDiGetDeviceInterfaceDetail(devinfo, &ifdata, ifdetail, need_size, &need_size, NULL);
            if (TRUE == ok)
            {
                HANDLE device = CreateFile(ifdetail->DevicePath, GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                if (INVALID_HANDLE_VALUE != device)
                {
                    result.push_back(ifdetail->DevicePath);
                    CloseHandle(device);
                }
            }
        }
    }

    if (result.size() > 0)
        return TRUE;
    return FALSE;
}

void ShowUsage()
{
    wprintf(L"Usage: DriverBindingTest.exe [INF File]\n");
    wprintf(L"       [INF File] is the INF file you want to bind.\n");
    wprintf(L"example: DriverBindingTest c:\\test\\graid_driver\\nvme.inf \n");
    wprintf(L"\n");
}

int _tmain(int argc, _TCHAR* argv[])
{
    if (argc < 2)
    {
        ShowUsage();
        return -1;
    }

    int count = 0;
    wstring hwid;

    list<wstring> devlist;
    ListStorportAdapters(devlist);
    for (const auto& devpath : devlist)
    {
        count++;
        wprintf(L"#%d %s\n", count, devpath.c_str());
        if (IsBuiltinNVMeControllerDriver(devpath, hwid))
        {
            BindDriver(hwid, argv[1]);
        }
        wprintf(L"\n");
    }
}
