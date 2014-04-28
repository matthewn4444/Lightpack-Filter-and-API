#ifndef __CLIGHTPACK__
#define __CLIGHTPACK__

#include <streams.h>
#include <Lightpack.h>

#include <vector>

// TEMP
#include "log.h"
#define logf(...) if (mLog != 0) { mLog->logf(__VA_ARGS__); }
#define log(x) if (mLog != 0) { mLog->log(x); }

#include "timer\timer.h"

#define FILTER_NAME L"Lightpack"

// {188fb505-04ff-4257-9bdb-3ff431852f99}
static const GUID CLSID_Lightpack =
{ 0x188fb505, 0x04ff, 0x4257, { 0x9b, 0xdb, 0x3f, 0xf4, 0x31, 0x85, 0x2f, 0x99 } };

static void GetDesktopResolution(int& horizontal, int& vertical)        // TODO make smarter for multiple desktops
{
    RECT desktop;
    const HWND hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    horizontal = desktop.right;
    vertical = desktop.bottom;
}

class CLightpack : public CTransInPlaceFilter {
public:
    DECLARE_IUNKNOWN;

    CLightpack(LPUNKNOWN pUnk, HRESULT *phr);
    virtual ~CLightpack(void);

    virtual HRESULT CheckInputType(const CMediaType* mtIn);
    virtual HRESULT Transform(IMediaSample *pSample);
    virtual HRESULT SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt);

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

private:
    void parseFrameThread();

    Log* mLog;
    std::vector<Lightpack::Rect> mLedArea;
    std::vector<Lightpack::Rect> mScaledRects;

    Lightpack::LedDevice* mDevice;

    GUID mVideoType;
    int mStride;
    VIDEOINFOHEADER mVidHeader;
    int mWidth;
    int mHeight;
    int thing;

    BYTE* mFrameBuffer;
    BYTE* m_pData;
};

#endif // __CLIGHTPACK__