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

#include <chrono>
class TraceBlock {
public:
    TraceBlock(Log* log, const char* funcName, int n): name(funcName), mLog(log), lineNumber(n){
        start = chrono::high_resolution_clock::now();
        _logf("Trace block start: %s[%d]", name, lineNumber);
    }
    ~TraceBlock() {
        float d = elasped();
        if (d > 20) {
            _logf("    Trace block end: %s[%d] Elasped %.5f ms", name, lineNumber, d);
        }
    }
    void lap(int n) {
        float d = elasped();
        if (d > 20) {
            _logf("    |- Trace lap: %s[%d] Elasped %.5f ms", name, n, d);
        }
    }

    float elasped() {
        return chrono::duration_cast<chrono::microseconds>
            (chrono::high_resolution_clock::now() - start).count() / 1000.0f;
    }

    Log* mLog;
    chrono::high_resolution_clock::time_point start;
    int lineNumber;
    const char* name;
};
#define trace_start() TraceBlock ___block(mLog, __func__, __LINE__);
#define trace_lap() ___block.lap(__LINE__);

#else
#define _logf(...)
#define _log(x)

#define trace_start()
#define trace_lap()
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
    static DWORD WINAPI DeviceCheckThread(LPVOID lpvThreadParm);

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

    // Device Check Thread Variables and Methods
    HANDLE mhDeviceCheckThread;
    DWORD mDeviceCheckThreadId;
    bool mDeviceCheckThreadStopRequested;

    void startDeviceCheckThread();
    void destroyDeviceCheckThread();
    DWORD deviceCheckStart();

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
    bool mIsReconnecting;

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
