#ifndef __CLIGHTPACK__
#define __CLIGHTPACK__

#include "socket.hpp"

#include <shlobj.h>
#include <streams.h>
#include <Dvdmedia.h>
#include <Lightpack.h>

#include <vector>
#include <queue>

#include "gpu_memcpy_sse4.h"
#include "Converters.h"
#include "../../LightpackAPI/inih/cpp/INIReader.h"

// Log out the settings on debug
#if defined(_DEBUG) || defined(_PERF)
#include "log.h"
#define logf(...) if (mLog != 0) { mLog->logf(__VA_ARGS__); }
#define log(x) if (mLog != 0) { mLog->log(x); }
#else
#define logf(...)
#define log(x)
#endif

#ifdef _PERF
// Usage
//      Prints the elapsed time between the start and print macros
//
//      TIME_START
//          <CODE....>
//      TIME_PRINT
#define TIME_START UINT64 start = getTime();
#define TIME_PRINT \
    logf("Elapsed: %I64d milliseconds", (getTime()-start)/10000);

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
    static const DWORD sDeviceCheckElapseTime;      // Check every 2 seconds

    // Video Conversion Methods
    Lightpack::RGBCOLOR meanColorFromRGB32(Lightpack::Rect& rect);
    Lightpack::RGBCOLOR meanColorFromNV12(Lightpack::Rect& rect);


    typedef std::pair<REFERENCE_TIME, Lightpack::RGBCOLOR*> LightEntry;

    static DWORD WINAPI ParsingThread(LPVOID lpvThreadParm);
    static DWORD WINAPI CommunicationThread(LPVOID lpvThreadParm);

    void queueLight(REFERENCE_TIME startTime);
    void displayLight(Lightpack::RGBCOLOR* colors);
    bool connectDevice();
    bool connectPrismatik();
    void disconnectAllDevices();
    void postConnection();

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

    void loadSettingsFile();
    void readSettingsFile(INIReader& reader);
    bool parseLedRectLine(const char* line, Lightpack::Rect* outRect);
    void percentageRectToVideoRect(double x, double y, double w, double h, Lightpack::Rect* outRect);
    void updateScaledRects(std::vector<Lightpack::Rect>& rects);
    wchar_t* getCurrentDirectory();

    // Avoid multiple MPC for making multiple instances of this class
    static bool sAlreadyRunning;
    bool mIsFirstInstance;

    // Thread function
    void startLightThread();
    void destroyLightThread();
    DWORD lightThreadStart();

    // Communication thread
    void startCommThread();
    void destroyCommThread();
    DWORD commThreadStart();
    bool mShouldSendPlayEvent;
    bool mShouldSendPauseEvent;
    bool mShouldSendConnectEvent;
    bool mShouldSendDisconnectEvent;

    void handleMessages(Socket& socket);
    bool parseReceivedMessages(int messageType, char* buffer, bool* deviceConnected);
#ifdef _DEBUG
    Log* mLog;
#endif
    std::vector<Lightpack::Rect> mScaledRects;

    // Threading variables
    HANDLE mhLightThread;
    DWORD mLightThreadId;
    bool mLightThreadStopRequested;
    bool mLightThreadCleanUpRequested;
    CRITICAL_SECTION mQueueLock;
    CRITICAL_SECTION mAdviseLock;
    CRITICAL_SECTION mDeviceLock;
    CRITICAL_SECTION mCommSendLock;
    CRITICAL_SECTION mScaledRectLock;

    // Communication thread to the GUI
    HANDLE mhCommThread;
    DWORD mCommThreadId;
    bool mCommThreadStopRequested;

    Lightpack::LedBase* mDevice;
    bool mIsConnectedToPrismatik;

    VideoFormat mVideoType;
    int mStride;
    int mWidth;
    int mHeight;

    // Lightpack properties
    std::vector<Lightpack::RGBCOLOR> mPropColors;
    double mPropGamma;
    unsigned char mPropBrightness;
    unsigned char mPropSmooth;
    unsigned int mPropPort;

    DWORD mLastDeviceCheck;
    bool mIsRunning;

    BYTE* mFrameBuffer;
    std::queue<DWORD_PTR> mAdviseQueue;
    bool mHasReadSettings;
    wchar_t mCurrentDirectoryCache[MAX_PATH];

    HANDLE mAppMutex;
};

#endif // __CLIGHTPACK__