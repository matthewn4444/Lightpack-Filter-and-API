#include "CLightpack.h"

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

// Media Types
const AMOVIESETUP_MEDIATYPE sudPinTypesIn[] =
{
    { &MEDIATYPE_NULL, &MEDIASUBTYPE_NULL },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_NV12 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB32 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB565 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB555 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
};

const AMOVIESETUP_MEDIATYPE sudPinTypesOut[] =
{
    { &MEDIATYPE_NULL, &MEDIASUBTYPE_NULL },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_NV12 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB32 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB565 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB555 },
    { &MEDIATYPE_Video, &MEDIASUBTYPE_RGB24 },
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
        _countof(sudPinTypesIn),
        sudPinTypesIn
    },
    {
        L"Output",
        FALSE,
        TRUE,
        FALSE,
        FALSE,
        &CLSID_NULL,
        NULL,
        _countof(sudPinTypesOut),
        sudPinTypesOut
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
    MERIT_PREFERRED,
    _countof(psudPins),
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
