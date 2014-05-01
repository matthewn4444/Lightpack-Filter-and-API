#ifndef __CLIGHTPACK__
#define __CLIGHTPACK__

#include <streams.h>
#include <Lightpack.h>

#include <vector>
#include <queue>
#include "copyframe.h"

// TEMP
#define LOG_ENABLED

#ifdef LOG_ENABLED
#include "log.h"
#define logf(...) if (mLog != 0) { mLog->logf(__VA_ARGS__); }
#define log(x) if (mLog != 0) { mLog->log(x); }
#else
#include "log.h"
#define logf(...)
#define log(x)
#endif

#define TIME_START UINT64 start = getTime();

#define TIME_PRINT \
    logf("Elapsed: %I64d milliseconds", (getTime()-start)/10000);

#define FILTER_NAME L"Lightpack"

static UINT64 getTime()
{
    SYSTEMTIME st;
    GetSystemTime(&st);

    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);  // converts to file time format
    ULARGE_INTEGER ui;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;

    return ui.QuadPart;
}

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

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

private:
    static DWORD WINAPI ParsingThread(LPVOID lpvThreadParm);
    void queueLight(REFERENCE_TIME startTime);
    void displayLight(COLORREF* colors);

    bool ScheduleNextDisplay();
    CAMEvent mDisplayLightEvent;
    std::queue<std::pair<REFERENCE_TIME, COLORREF*>> mColorQueue;

    long getStreamTime() {
        if (!m_tStart) return 0;        // Not running yet
        REFERENCE_TIME now;
        m_pClock->GetTime(&now);
        return (long)(now - m_tStart);
    }

    // Thread function
    void startThread();
    void destroyThread();
    DWORD threadStart();
#ifdef LOG_ENABLED
    Log* mLog;
#endif
    std::vector<Lightpack::Rect> mLedArea;
    std::vector<Lightpack::Rect> mScaledRects;

    // Threading variables
    HANDLE mhThread;
    DWORD mThreadId;
    bool mThreadStopRequested;
    CRITICAL_SECTION mQueueLock;

    Lightpack::LedDevice* mDevice;

    GUID mVideoType;
    int mStride;
    VIDEOINFOHEADER mVidHeader;
    int mWidth;
    int mHeight;

    BYTE* mFrameBuffer;
};

#endif // __CLIGHTPACK__