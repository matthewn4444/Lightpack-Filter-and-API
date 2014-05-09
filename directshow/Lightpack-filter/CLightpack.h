#ifndef __CLIGHTPACK__
#define __CLIGHTPACK__

#include <streams.h>
#include <Dvdmedia.h>
#include <Lightpack.h>

#include <vector>
#include <queue>

#include "gpu_memcpy_sse4.h"
#include "Converters.h"

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


enum VideoFormat { RGB32, NV12, OTHER };

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

    virtual STDMETHODIMP Run(REFERENCE_TIME StartTime);
    virtual STDMETHODIMP Stop();
    virtual STDMETHODIMP Pause();

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

private:
    static const DWORD sDeviceCheckElapseTime;      // Check every 2 seconds

    Lightpack::RGBCOLOR meanColorFromRGB32(Lightpack::Rect& rect) {
        ASSERT(mStride >= mWidth);
        const unsigned int totalPixels = rect.area();

        unsigned int totalR = 0, totalG = 0, totalB = 0;
        for (int r = 0; r < rect.height; r++) {
            int y = rect.y + r;

            BYTE* pixel = mFrameBuffer + (rect.x + y * mStride) * 4;      // 4 bytes per pixel
            for (int c = 0; c < rect.width; c++) {
                totalB += pixel[0];
                totalG += pixel[1];
                totalR += pixel[2];
                pixel += 4;
            }
        }
        return RGB((int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels));
    }

    Lightpack::RGBCOLOR meanColorFromNV12(Lightpack::Rect& rect) {
        ASSERT(mStride >= mWidth);
        const unsigned int pixel_total = mStride * mHeight;
        const unsigned int totalPixels = rect.area();
        BYTE* Y = mFrameBuffer;
        BYTE* U = mFrameBuffer + pixel_total;
        BYTE* V = mFrameBuffer + pixel_total + 1;
        const int dUV = 2;

        BYTE* U_pos = U;
        BYTE* V_pos = V;

        // YUV420 to RGB
        unsigned int totalR = 0, totalG = 0, totalB = 0;
        for (int r = 0; r < rect.height; r++) {
            int y = r + rect.y;

            Y = mFrameBuffer + y * mStride + rect.x;
            U = mFrameBuffer + pixel_total + (y / 2) * mStride + (rect.x & 0x1 ? rect.x - 1 : rect.x);
            V = U + 1;

            for (int c = 0; c < rect.width; c++) {
                Lightpack::RGBCOLOR color = YUVToRGB(*(Y++), *U, *V);
                totalR += GET_RED(color);
                totalG += GET_GREEN(color);
                totalB += GET_BLUE(color);

                if ((rect.x + c) & 0x1) {
                    U += dUV;
                    V += dUV;
                }
            }
        }
        return RGB((int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels));
    }

    typedef std::pair<REFERENCE_TIME, Lightpack::RGBCOLOR*> LightEntry;

    static DWORD WINAPI ParsingThread(LPVOID lpvThreadParm);
    void queueLight(REFERENCE_TIME startTime);
    void displayLight(Lightpack::RGBCOLOR* colors);
    bool connectDevice();
    void disconnectDevice();

    void CancelNotification();

    bool ScheduleNextDisplay();
    CAMEvent mDisplayLightEvent;

    std::queue<LightEntry> mColorQueue;

    long getStreamTime() {
        if (!m_tStart) return 0;
        REFERENCE_TIME now;
        m_pClock->GetTime(&now);
        return (long)(now - m_tStart);
    }

    long getStreamTimeMilliSec() {
        return getStreamTime() / (UNITS / MILLISECONDS);
    }

    void clearQueue();

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
    bool mThreadCleanUpRequested;
    CRITICAL_SECTION mQueueLock;
    CRITICAL_SECTION mAdviseLock;
    CRITICAL_SECTION mDeviceLock;

    Lightpack::LedBase* mDevice;

    VideoFormat mVideoType;
    int mStride;
    int mWidth;
    int mHeight;

    DWORD mLastDeviceCheck;

    BYTE* mFrameBuffer;
    std::queue<DWORD_PTR> mAdviseQueue;
};

#endif // __CLIGHTPACK__