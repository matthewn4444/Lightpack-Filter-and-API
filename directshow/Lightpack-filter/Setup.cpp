#include "CLightpack.h"

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// Media Types
const AMOVIESETUP_MEDIATYPE sudPinTypes[] =
{
    {
        &MEDIATYPE_Video,
        &MEDIASUBTYPE_NULL
    }
};

// Pins
const AMOVIESETUP_PIN psudPins[] =
{
    {
        L"Input",
        FALSE,
        FALSE,
        FALSE,
        FALSE,
        &CLSID_NULL,
        NULL,
        1,
        &sudPinTypes[0]
    },
    {
        L"Output",
        FALSE,
        TRUE,
        FALSE,
        FALSE,
        &CLSID_NULL,
        NULL,
        1,
        &sudPinTypes[0]
    }
};

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
    return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDllHandle), dwReason, lpReserved);
}

// Filters
const AMOVIESETUP_FILTER sudAudioVolume =
{
    &CLSID_Lightpack,
    FILTER_NAME,
    MERIT_UNLIKELY,
    2,
    psudPins
};

// Templates
CFactoryTemplate g_Templates[] =
{
    {
        FILTER_NAME,
        &CLSID_Lightpack,
        CLightpack::CreateInstance,
        NULL,
        &sudAudioVolume
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}
