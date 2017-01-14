#ifndef __CLIGHTPACK__
#define __CLIGHTPACK__

#include "socket.hpp"

#include <shlobj.h>
#include <streams.h>
#include <Dvdmedia.h>
#include <Lightpack.h>

#include <vector>
#include <queue>

#include "Converters.h"
#include "../../LightpackAPI/thirdparty/inih/cpp/INIReader.h"

// Log out the settings on debug
#if defined(_DEBUG)
#include "log.h"
#define _logf(...) if (mLog != 0) { mLog->logf(__VA_ARGS__); }
#define _log(x) if (mLog != 0) { mLog->log(x); }
#else
#define _logf(...)
#define _log(x)
#endif

#ifdef _PERF
#include <chrono>
#include "log.h"
// Usage
//      Prints the elapsed time between the start and print macros
//
//      TIME_START
//          <CODE....>
//      TIME_PRINT
#define TIME_START auto begin = chrono::high_resolution_clock::now();
#define TIME_PRINT \
mLog->logf("Elapsed: %f milliseconds", chrono::duration_cast<chrono::microseconds>(chrono::high_resolution_clock::now() - begin).count() / 1000.0f);

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
#else
#define TIME_START
#define TIME_PRINT
#endif

#define FILTER_NAME L"Lightpack"

#define DEFAULT_SMOOTH 30
#define DEFAULT_GUI_PORT 6000
#define DEFAULT_ON_WHEN_CLOSE false

// {188fb505-04ff-4257-9bdb-3ff431852f99}
static const GUID CLSID_Lightpack =
{ 0x188fb505, 0x04ff, 0x4257, { 0x9b, 0xdb, 0x3f, 0xf4, 0x31, 0x85, 0x2f, 0x99 } };

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

    enum VideoFormat { RGB32, NV12, OTHER };

private:
    typedef std::pair<REFERENCE_TIME, Lightpack::RGBCOLOR*> LightEntry;

    static const DWORD sDeviceCheckElapseTime;      // Check every 2 seconds

    // Video Conversion Methods
    Lightpack::RGBCOLOR meanColorFromRGB32(BYTE* src, Lightpack::Rect& rect);
    Lightpack::RGBCOLOR meanColorFromNV12(BYTE* src, Lightpack::Rect& rect);
    Lightpack::RGBCOLOR meanColorFromNV12SSE(BYTE* src, Lightpack::Rect& rect);

    // Threading Variables and Methods
    static DWORD WINAPI ParsingThread(LPVOID lpvThreadParm);
    static DWORD WINAPI CommunicationThread(LPVOID lpvThreadParm);
    static DWORD WINAPI IOThread(LPVOID lpvThreadParm);
    static DWORD WINAPI ColorParsingThread(LPVOID lpvThreadParm);

    HANDLE mhLightThread;
    DWORD mLightThreadId;
    bool mLightThreadStopRequested;
    bool mLightThreadCleanUpRequested;
    CRITICAL_SECTION mQueueLock;
    CRITICAL_SECTION mAdviseLock;
    CRITICAL_SECTION mDeviceLock;
    CRITICAL_SECTION mCommSendLock;
    CRITICAL_SECTION mScaledRectLock;

    void startLightThread();
    void destroyLightThread();
    DWORD lightThreadStart();

    // Communication thread Methods
    HANDLE mhCommThread;
    DWORD mCommThreadId;
    bool mCommThreadStopRequested;
    bool mShouldSendPlayEvent;
    bool mShouldSendPauseEvent;
    bool mShouldSendConnectEvent;
    bool mShouldSendDisconnectEvent;

    void startCommThread();
    void destroyCommThread();
    DWORD commThreadStart();
    void handleMessages(Socket& socket);
    bool parseReceivedMessages(int messageType, char* buffer, bool* deviceConnected);

    // IO Thread Variables and Methods
    HANDLE mhLoadSettingsThread;
    DWORD mLoadSettingsThreadId;

    void startLoadSettingsThread();
    void destroyLoadSettingsThread();
    DWORD loadSettingsThreadStart();

    // Color Parsing Thread Variables and Methods
    HANDLE mhColorParsingThread;
    DWORD mColorParsingThreadId;
    bool mColorParsingRequested;
    REFERENCE_TIME mColorParsingTime;

    void startColorParsingThread();
    void destroyColorParsingThread();
    DWORD colorParsingThreadStart();

    // Mutexes and only allowing one of these filters per graph
    HANDLE mAppMutex;
    static bool sAlreadyRunning;
    bool mIsFirstInstance;

    // Settings and other file I/O
    bool mHasReadSettings;
    char mSettingsPathFromGUI[MAX_PATH];
    wchar_t mCurrentDirectoryCache[MAX_PATH];

    void readSettingsFile(INIReader& reader);
    bool parseLedRectLine(const char* line, Lightpack::Rect* outRect);
    wchar_t* getCurrentDirectory();

    // Rectangles and scaled rectangles
    std::vector<Lightpack::Rect> mScaledRects;

    void updateScaledRects(std::vector<Lightpack::Rect>& rects);
    void percentageRectToVideoRect(double x, double y, double w, double h, Lightpack::Rect* outRect);

    // Device/Prismatik Connections
    Lightpack::LedBase* mDevice;
    bool mIsConnectedToPrismatik;
    DWORD mLastDeviceCheck;

    bool reconnectDevice();
    bool connectPrismatik();
    void disconnectAllDevices();
    void postConnection();

    // Lightpack Display Methods and Properties
    CAMEvent mDisplayLightEvent;
    CAMEvent mParseColorsEvent;
    std::queue<LightEntry> mColorQueue;
    std::queue<DWORD_PTR> mAdviseQueue;

    void clearQueue();
    void CancelNotification();
    bool ScheduleNextDisplay();
    void queueLight(REFERENCE_TIME startTime, BYTE* src);
    void displayLight(Lightpack::RGBCOLOR* colors);

    // Lightpack properties
    std::vector<Lightpack::RGBCOLOR> mPropColors;
    double mPropGamma;
    unsigned char mPropBrightness;
    unsigned char mPropSmooth;
    unsigned int mPropPort;
    bool mPropOnWhenClose;

    // Video Properties
    VideoFormat mVideoType;
    BYTE* mFrameBuffer;
    int mStride;
    int mWidth;
    int mHeight;
    bool mIsRunning;
    bool mSampleUpsideDown;

    // Frame rate/skip properties
    bool mUseFrameSkip;
    size_t mIterationsPerSec;
    size_t mFrameSkipCount;
    DWORD mLastFrameRateCheck;
    static const size_t sMinIterationPerSec;
    static const size_t sMaxIterationPerSec;

    bool shouldSkipTransform();

    // Miscellaneous
    long getStreamTime() {
        if (!m_tStart) return 0;
        REFERENCE_TIME now;
        m_pClock->GetTime(&now);
        return (long)(now - m_tStart);
    }

    long getStreamTimeMilliSec() {
        return getStreamTime() / (UNITS / MILLISECONDS);
    }

#if defined(_DEBUG) || defined(_PERF)
    Log* mLog;
#endif
};

#endif // __CLIGHTPACK__
