#include <sstream>
#include "CLightpack.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Connection to Prismatik
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 3636
#define DEFAULT_APIKEY "{cb832c47-0a85-478c-8445-0b20e3c28cdd}"

#define MUTEX_NAME L"LightpackFilterMutex"

static const BOOL sse2supported = ::IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);

const DWORD CLightpack::sDeviceCheckElapseTime = 1000;
const size_t CLightpack::sMinIterationPerSec = 20;
const size_t CLightpack::sMaxIterationPerSec = 35;
bool CLightpack::sAlreadyRunning = false;

CLightpack::CLightpack(LPUNKNOWN pUnk, HRESULT *phr)
    : CTransInPlaceFilter(FILTER_NAME, pUnk, CLSID_Lightpack, phr)
    , mDevice(NULL)
    , mIsFirstInstance(false)
    , mWidth(0)
    , mHeight(0)
    , mhLightThread(INVALID_HANDLE_VALUE)
    , mLightThreadId(0)
    , mLightThreadStopRequested(false)
    , mhCommThread(INVALID_HANDLE_VALUE)
    , mCommThreadId(0)
    , mCommThreadStopRequested(false)
    , mStride(0)
    , mLastDeviceCheck(GetTickCount())
    , mLightThreadCleanUpRequested(false)
    , mShouldSendPlayEvent(false)
    , mShouldSendPauseEvent(false)
    , mShouldSendConnectEvent(false)
    , mShouldSendDisconnectEvent(false)
    , mhLoadSettingsThread(INVALID_HANDLE_VALUE)
    , mLoadSettingsThreadId(0)
    , mIsRunning(false)
    , mSampleUpsideDown(false)
    , mUseFrameSkip(true)
    , mIterationsPerSec(0)
    , mFrameSkipCount(0)
    , mLastFrameRateCheck(0)
    , mIsConnectedToPrismatik(false)
    , mPropGamma(Lightpack::DefaultGamma)
    , mPropSmooth(DEFAULT_SMOOTH)
    , mPropBrightness(Lightpack::DefaultBrightness)
    , mPropPort(DEFAULT_GUI_PORT)
    , mHasReadSettings(false)
{
    mCurrentDirectoryCache[0] = '\0';

    if (!sAlreadyRunning) {
#if defined(_DEBUG) || defined(_PERF)
        mLog = new Log("log.txt");
#endif
        mIsFirstInstance = true;
        InitializeCriticalSection(&mQueueLock);
        InitializeCriticalSection(&mAdviseLock);
        InitializeCriticalSection(&mDeviceLock);
        InitializeCriticalSection(&mCommSendLock);
        InitializeCriticalSection(&mScaledRectLock);

        // Try to connect to the lights directly,
        // if fails the try to connect to Prismatik in the thread
        reconnectDevice();

        startLightThread();

        // Define the single instance mutex for uninstallation inno setup
        mAppMutex = CreateMutex(NULL, FALSE, MUTEX_NAME);
    }
    sAlreadyRunning = true;
}

CLightpack::~CLightpack(void)
{
    destroyLightThread();

    ASSERT(mLightThreadId == NULL);
    ASSERT(mhLightThread == INVALID_HANDLE_VALUE);
    ASSERT(mLightThreadStopRequested == false);

    destroyCommThread();

    ASSERT(mCommThreadId == NULL);
    ASSERT(mhCommThread == INVALID_HANDLE_VALUE);
    ASSERT(mCommThreadStopRequested == false);

    // Destroy thread incase it is not destroyed yet
    destroyLoadSettingsThread();
    ASSERT(mLoadSettingsThreadId == NULL);
    ASSERT(mhLoadSettingsThread == INVALID_HANDLE_VALUE);

    disconnectAllDevices();
    ASSERT(mDevice == NULL);

    // Once the first instance is deallocated, we should allow usage of this filter again
    if (mIsFirstInstance) {
        DeleteCriticalSection(&mQueueLock);
        DeleteCriticalSection(&mAdviseLock);
        DeleteCriticalSection(&mDeviceLock);
        DeleteCriticalSection(&mCommSendLock);
        DeleteCriticalSection(&mScaledRectLock);

        sAlreadyRunning = false;
        if (mAppMutex) {
            CloseHandle(mAppMutex);
        }
    }
#if defined(_DEBUG) || defined(_PERF)
    if (mLog) {
        delete mLog;
        mLog = NULL;
    }
#endif
}

HRESULT CLightpack::CheckInputType(const CMediaType* mtIn)
{
    CAutoLock lock(m_pLock);
    if (!mIsFirstInstance) {
        return E_FAIL;
    }

    if (mtIn->majortype != MEDIATYPE_Video)
    {
        return E_FAIL;
    }

    if (mtIn->subtype != MEDIASUBTYPE_NV12 &&
        mtIn->subtype != MEDIASUBTYPE_RGB555 &&
        mtIn->subtype != MEDIASUBTYPE_RGB565 &&
        mtIn->subtype != MEDIASUBTYPE_RGB24 &&
        mtIn->subtype != MEDIASUBTYPE_RGB32)
    {
        return E_FAIL;
    }

    if ((mtIn->formattype == FORMAT_VideoInfo) &&
        (mtIn->cbFormat >= sizeof(VIDEOINFOHEADER)) &&
        (mtIn->pbFormat != NULL))
    {
        return S_OK;
    }

    if ((mtIn->formattype == FORMAT_VideoInfo2) &&
        (mtIn->cbFormat >= sizeof(VIDEOINFOHEADER2)) &&
        (mtIn->pbFormat != NULL))
    {
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CLightpack::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    if (direction == PINDIR_INPUT)
    {
        VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)pmt->pbFormat;
        mWidth = pvih->bmiHeader.biWidth;
        mHeight = pvih->bmiHeader.biHeight;

        if (!mWidth || !mHeight) {
            // Try VIDEOINFOHEADER2
            VIDEOINFOHEADER2* pvih2 = (VIDEOINFOHEADER2*)pmt->pbFormat;
            mWidth = pvih2->bmiHeader.biWidth;
            mHeight = pvih2->bmiHeader.biHeight;
        }

        // Scale all the rects to the size of the video
        if (mScaledRects.empty() && mWidth > 0 && mHeight > 0) {
            // Load the settings file
            startLoadSettingsThread();
        }

        // Set the media type
        if (MEDIASUBTYPE_RGB32 == pmt->subtype) {
            _log("Going to render RGB32 images");
            mVideoType = VideoFormat::RGB32;
        }
        else if (MEDIASUBTYPE_NV12 == pmt->subtype) {
            _log("Going to render NV12 images");
            mVideoType = VideoFormat::NV12;
        }
        else {
            _log("Going to render other images");
            mVideoType = VideoFormat::OTHER;
        }
    }
    return S_OK;
}

STDMETHODIMP CLightpack::Run(REFERENCE_TIME StartTime)
{
    _logf("Run: %ld %lu %lu", getStreamTimeMilliSec(), ((CRefTime)StartTime).Millisecs(), ((CRefTime)m_tStart).Millisecs());
    if (m_State == State_Running) {
        return NOERROR;
    }

    HRESULT hr = CTransInPlaceFilter::Run(StartTime);
    if (FAILED(hr)) {
        _log("Run failed");
        return hr;
    }

    if (mCommThreadStopRequested) {
        destroyCommThread();
    }

    CancelNotification();

    EnterCriticalSection(&mDeviceLock);
    bool deviceExists = mDevice != NULL;
    LeaveCriticalSection(&mDeviceLock);
    if (deviceExists) {
        startLightThread();
    }

    EnterCriticalSection(&mCommSendLock);
    mShouldSendPlayEvent = mIsRunning == false;
    LeaveCriticalSection(&mCommSendLock);

    mIsRunning = true;
    return NOERROR;
}

STDMETHODIMP CLightpack::Stop()
{
    _logf("Stop %ld", getStreamTimeMilliSec());
    CTransInPlaceFilter::Stop();
    CancelNotification();
    return NOERROR;
}

STDMETHODIMP CLightpack::Pause()
{
    _logf("Pause %ld", getStreamTimeMilliSec());

    HRESULT hr = CTransInPlaceFilter::Pause();
    if (FAILED(hr)) {
        _log("Failed to pause");
        return hr;
    }
    destroyLightThread();

    EnterCriticalSection(&mCommSendLock);
    mShouldSendPauseEvent = mIsRunning == true;
    LeaveCriticalSection(&mCommSendLock);

    mIsRunning = false;
    return NOERROR;
}

bool CLightpack::reconnectDevice()
{
    if (!mIsConnectedToPrismatik) {
        EnterCriticalSection(&mDeviceLock);
        if (!mDevice) {
            mDevice = new Lightpack::LedDevice();
            if (!((Lightpack::LedDevice*)mDevice)->open()) {
                delete mDevice;
                mDevice = 0;
                _log("Device not connected");
                LeaveCriticalSection(&mDeviceLock);
                return false;
            }
            else {
                postConnection();
                _log("Device connected");
                startLightThread();
                mIsConnectedToPrismatik = false;
            }
        }
        else {
            // Ocasionally reopen to see if more modules connect
            if (!((Lightpack::LedDevice*)mDevice)->tryToReopenDevice()) {
                delete mDevice;
                mDevice = 0;
                _log("Device not connected");
                LeaveCriticalSection(&mDeviceLock);
                return false;
            }
            else if (mDevice->getCountLeds() != mPropColors.size()) {
                postConnection();
            }
        }
        LeaveCriticalSection(&mDeviceLock);
    }
    return true;
}

bool CLightpack::connectPrismatik()
{
    if (mDevice == NULL) {
        EnterCriticalSection(&mDeviceLock);
        if (mDevice == NULL) {
            _log("Try to connect to Prismatik");
            mDevice = new Lightpack::PrismatikClient();
            const std::vector<int> blankLedMap;
            if (((Lightpack::PrismatikClient*)mDevice)->connect(DEFAULT_HOST, DEFAULT_PORT, blankLedMap, DEFAULT_APIKEY) != Lightpack::RESULT::OK
                || ((Lightpack::PrismatikClient*)mDevice)->lock() != Lightpack::RESULT::OK) {
                _log("Failed to also connect to Prismatik.");
                delete mDevice;
                mDevice = NULL;
                LeaveCriticalSection(&mDeviceLock);
                return false;
            }
            else {
                postConnection();
                _log("Connected to Prismatik.");
                mIsConnectedToPrismatik = true;
            }
        }
        LeaveCriticalSection(&mDeviceLock);
    }
    return true;
}

void CLightpack::disconnectAllDevices()
{
    if (mDevice) {
        EnterCriticalSection(&mDeviceLock);
        if (mDevice) {
            delete mDevice;
            mDevice = NULL;
            _log("Disconnected device");
            mShouldSendDisconnectEvent = true;
            mIsConnectedToPrismatik = false;
            // DO NOT REMOVE THREAD HERE, there will be a deadlock
        }
        LeaveCriticalSection(&mDeviceLock);
    }
}

void CLightpack::postConnection()
{
    EnterCriticalSection(&mDeviceLock);
    // Apply the current Lightpack properties
    mDevice->setSmooth(mPropSmooth);
    mDevice->setBrightness(mPropBrightness);
    mDevice->setGamma(mPropGamma);

    // Update the colors of the array if not all black
    bool shouldUpdateColors = false;
    for (size_t i = 0; i < mPropColors.size(); i++) {
        if (mPropColors[i] > 0) {
            shouldUpdateColors = true;
            break;
        }
    }
    if (shouldUpdateColors) {
        mDevice->setColors(mPropColors);
    }

    // Update the size of the color array
    int numLeds = mDevice->getCountLeds();
    if (mPropColors.size() != numLeds) {
        mPropColors.resize(numLeds);
    }
    mShouldSendConnectEvent = true;
    LeaveCriticalSection(&mDeviceLock);
}

void CLightpack::CancelNotification()
{
    clearQueue();

    // Clear all advise times from clock
    if (!mAdviseQueue.empty()) {
        EnterCriticalSection(&mAdviseLock);
        if (!mAdviseQueue.empty() && m_pClock) {
            m_pClock->Unadvise(mAdviseQueue.front());
            mAdviseQueue.pop();
        }
        LeaveCriticalSection(&mAdviseLock);
    }

    mDisplayLightEvent.Reset();
}

wchar_t* CLightpack::getCurrentDirectory()
{
    if (wcslen(mCurrentDirectoryCache) == 0) {
        WCHAR wDllPath[MAX_PATH] = { 0 };
        if (GetModuleFileName((HINSTANCE)&__ImageBase, wDllPath, _countof(wDllPath)) != ERROR_INSUFFICIENT_BUFFER) {
            std::wstring temp(wDllPath);
            std::wstring axPath = temp.substr(0, temp.find_last_of('\\'));
            wcscpy(mCurrentDirectoryCache, axPath.c_str());
        }
    }
    return mCurrentDirectoryCache;
}

void CLightpack::updateScaledRects(std::vector<Lightpack::Rect>& rects)
{
    EnterCriticalSection(&mScaledRectLock);
    // Adjust size of rects from inputted rects
    size_t newSize = rects.size();
    if (mScaledRects.size() != newSize) {
        mScaledRects.resize(newSize);
    }
    size_t i = 0;
    for (; i < newSize; i++) {
        if (rects[i].width > 0 && rects[i].height > 0) {
            mScaledRects[i] = rects[i];
        }
    }
    LeaveCriticalSection(&mScaledRectLock);
}

void CLightpack::startLightThread()
{
    if (mLightThreadId != NULL && mhLightThread != INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mLightThreadId == NULL);
    ASSERT(mhLightThread == INVALID_HANDLE_VALUE);

    mhLightThread = CreateThread(NULL, 0, ParsingThread, (void*) this, 0, &mLightThreadId);
    mLightThreadCleanUpRequested = false;

    ASSERT(mhLightThread);
    if (mhLightThread == NULL) {
        _log("Failed to create thread");
    }
}

void CLightpack::destroyLightThread()
{
    if (mLightThreadId == NULL && mhLightThread == INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mLightThreadId != NULL);
    ASSERT(mhLightThread != INVALID_HANDLE_VALUE);

    EnterCriticalSection(&mQueueLock);
    mLightThreadStopRequested = true;
    LeaveCriticalSection(&mQueueLock);

    mDisplayLightEvent.Set();

    WaitForSingleObject(mhLightThread, INFINITE);
    CloseHandle(mhLightThread);

    // Reset Values
    mLightThreadId = 0;
    mhLightThread = INVALID_HANDLE_VALUE;
    mLightThreadStopRequested = false;
    mLightThreadCleanUpRequested = false;
    CancelNotification();
}

void CLightpack::queueLight(REFERENCE_TIME startTime, BYTE* src)
{
    CAutoLock lock(m_pLock);

    EnterCriticalSection(&mScaledRectLock);
    Lightpack::RGBCOLOR* colors = new Lightpack::RGBCOLOR[mScaledRects.size()];

    switch (mVideoType) {
    case RGB32:
        for (size_t i = 0; i < mScaledRects.size(); i++) {
            colors[i] = meanColorFromRGB32(src, mScaledRects[i]);
        }
        break;
    case NV12:
        if (sse2supported) {
            for (size_t i = 0; i < mScaledRects.size(); i++) {
                colors[i] = meanColorFromNV12SSE(src, mScaledRects[i]);
            }
        }
        else {
            for (size_t i = 0; i < mScaledRects.size(); i++) {
                colors[i] = meanColorFromNV12(src, mScaledRects[i]);
            }
        }
        break;
    default:
        std::fill(colors, colors + mScaledRects.size(), 0);
    }

    LeaveCriticalSection(&mScaledRectLock);

    EnterCriticalSection(&mQueueLock);
    // Render the first frame on startup while renderer buffers; happens only once
    if (m_tStart == 0 && startTime == 0 && mColorQueue.empty()) {
        LeaveCriticalSection(&mQueueLock);
        displayLight(colors);
        delete[] colors;
    }
    else {
        mColorQueue.push(std::make_pair(startTime, colors));
        LeaveCriticalSection(&mQueueLock);
    }
}

void CLightpack::displayLight(Lightpack::RGBCOLOR* colors)
{
    ASSERT(mScaledRects.size());
    size_t len = min(mScaledRects.size(), mPropColors.size());
    if (mDevice && len) {
        EnterCriticalSection(&mDeviceLock);
        if (mDevice) {
            if (mDevice->setColors(colors, len) != Lightpack::RESULT::OK) {
                // Device is/was disconnected
                LeaveCriticalSection(&mDeviceLock);
                mLightThreadCleanUpRequested = true;
                disconnectAllDevices();
                return;
            }
            for (size_t i = 0; i < mPropColors.size(); i++) {
                if (i < 0) {
                    continue;
                }
                mPropColors[i] = colors[i];
            }
        }
        LeaveCriticalSection(&mDeviceLock);
    }
}

DWORD CLightpack::lightThreadStart()
{
    bool isConnected = connectPrismatik();

    // Start the communication thread guarenteed after device is connected or not
    startCommThread();

    while (isConnected) {
        WaitForSingleObject(mDisplayLightEvent, INFINITE);

        EnterCriticalSection(&mQueueLock);

        if (mLightThreadStopRequested) {
            LeaveCriticalSection(&mQueueLock);
            break;
        }

        if (mColorQueue.empty()) {
            LeaveCriticalSection(&mQueueLock);
            continue;
        }

        LightEntry& pair = mColorQueue.front();
        Lightpack::RGBCOLOR* colors = pair.second;
        mColorQueue.pop();

        LeaveCriticalSection(&mQueueLock);

        ASSERT(colors != NULL);

        // Was not able to get a scheduled time, skip this color
        if (mAdviseQueue.empty()) {
            delete[] colors;
            continue;
        }

        displayLight(colors);
        delete[] colors;

        EnterCriticalSection(&mAdviseLock);
        m_pClock->Unadvise(mAdviseQueue.front());
        mAdviseQueue.pop();
        LeaveCriticalSection(&mAdviseLock);
    }
    return 0;
}

DWORD WINAPI CLightpack::ParsingThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->lightThreadStart();
}

void CLightpack::clearQueue()
{
    if (!mColorQueue.empty()) {
        EnterCriticalSection(&mQueueLock);
        if (!mColorQueue.empty()) {
            size_t len = mColorQueue.size();
            for (; len > 0; len--) {
                LightEntry& pair = mColorQueue.front();
                Lightpack::RGBCOLOR* colors = pair.second;
                delete[] colors;
                mColorQueue.pop();
            }
        }
        LeaveCriticalSection(&mQueueLock);
    }
    ASSERT(mColorQueue.empty());
}

bool CLightpack::ScheduleNextDisplay()
{
    EnterCriticalSection(&mQueueLock);
    REFERENCE_TIME sampleTime = -1;
    if (!mColorQueue.empty()) {
        LightEntry& pair = mColorQueue.front();
        sampleTime = pair.first;
    }
    LeaveCriticalSection(&mQueueLock);
    if (sampleTime == -1) {
        return false;
    }

    // Schedule next light
    EnterCriticalSection(&mAdviseLock);
    DWORD_PTR pAdvise;
    HRESULT hr = m_pClock->AdviseTime(
        m_tStart,
        sampleTime,
        (HEVENT)(HANDLE)mDisplayLightEvent,
        &pAdvise);
    mAdviseQueue.push(pAdvise);
    LeaveCriticalSection(&mAdviseLock);

    ASSERT(!mAdviseQueue.empty());

    if (SUCCEEDED(hr)) {
        return true;
    }
    _log("Failed to advise time");
    return false;
}

bool CLightpack::shouldSkipTransform()
{
    if (!mUseFrameSkip) {
        return false;
    }

    // Calculate the frame rate from iterations and frameskip so framerate is between 20-35
    DWORD now = GetTickCount();
    if ((now - mLastFrameRateCheck) > 1000) {
        // Above the allowed framerate or below the framerate but already skipping; force adjustments
        if (mIterationsPerSec >= sMinIterationPerSec * 2 || mIterationsPerSec < sMaxIterationPerSec && mFrameSkipCount > 0) {
            size_t skipCount = mFrameSkipCount;
            while (sMaxIterationPerSec < (mIterationsPerSec / (skipCount + 1))) {
                skipCount++;
            }
            if (skipCount > 0 && (mIterationsPerSec / (skipCount + 1)) < sMinIterationPerSec) {
                skipCount--;
            }
            // Change in frames to skip
            if (skipCount != mFrameSkipCount) {
                mFrameSkipCount = skipCount;
                CancelNotification();
            }
        }
        mIterationsPerSec = 0;
        mLastFrameRateCheck = now;
    }
    mIterationsPerSec++;
    return mFrameSkipCount && mIterationsPerSec % (mFrameSkipCount + 1) != 0;
}

HRESULT CLightpack::Transform(IMediaSample *pSample)
{
    // See if the thread needs to be destroyed requested from the thread
    if (mLightThreadCleanUpRequested) {
        mLightThreadCleanUpRequested = false;
        destroyLightThread();
    }

    // Adapt changes in samples
    AM_MEDIA_TYPE* pType;
    if (SUCCEEDED(pSample->GetMediaType(&pType))) {
        if (pType) {
            _log(pType);
            // Set the properties for this frame
            if (pType->formattype == FORMAT_VideoInfo && pType->cbFormat >= sizeof(VIDEOINFOHEADER)) {
                VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pType->pbFormat);
                const BITMAPINFOHEADER& header = pVih->bmiHeader;
                mStride = header.biWidth;
                mSampleUpsideDown = mVideoType != NV12 && header.biHeight > 0;
            }
            else if (pType->formattype == FORMAT_VideoInfo2 && pType->cbFormat >= sizeof(VIDEOINFOHEADER2)) {
                VIDEOINFOHEADER2 *pVih = reinterpret_cast<VIDEOINFOHEADER2*>(pType->pbFormat);
                const BITMAPINFOHEADER& header = pVih->bmiHeader;
                mStride = header.biWidth;
                mSampleUpsideDown = mVideoType != NV12 && header.biHeight > 0;
            }
            DeleteMediaType(pType);
        }
    }

    // Reconnect device every 1 seconds if not connected
    DWORD now = GetTickCount();
    if ((now - mLastDeviceCheck) > sDeviceCheckElapseTime) {
        reconnectDevice();
        mLastDeviceCheck = now;
    }

    // Applies frame skipping to 20-35 iterations/sec to process to reduce cpu load
    if (shouldSkipTransform()) {
        return S_OK;
    }

    if (mDevice != NULL) {
        if (!mScaledRects.empty() && !mPropColors.empty() && mVideoType != VideoFormat::OTHER) {
            REFERENCE_TIME startTime, endTime;
            pSample->GetTime(&startTime, &endTime);

            // Do not render old frames, they are discarded anyways
            REFERENCE_TIME streamTime = getStreamTime();
            if (!streamTime || startTime > streamTime) {
                BYTE* pData = NULL;
                pSample->GetPointer(&pData);
                if (pData != NULL) {
                    queueLight(startTime, pData);

                    // Schedule if the start time is defined, otherwise it is still buffering
                    if (streamTime) {
                        ScheduleNextDisplay();
                    }
                }
            }
        }
    }
    return S_OK;
}

CUnknown *WINAPI CLightpack::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CLightpack *instance = new CLightpack(pUnk, phr);
    if (!instance)
    {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return instance;
}
