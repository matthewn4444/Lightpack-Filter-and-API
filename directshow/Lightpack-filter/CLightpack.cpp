#include <sstream>
#include "CLightpack.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Connection to Prismatik
#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 3636
#define DEFAULT_APIKEY "{cb832c47-0a85-478c-8445-0b20e3c28cdd}"
#define DEFAULT_SMOOTH 30
#define DEFAULT_GUI_PORT 6000

#define MUTEX_NAME L"LightpackFilterMutex"

#define SETTINGS_FILE "settings.ini"
#define PROJECT_NAME "lightpack-filter"

const DWORD CLightpack::sDeviceCheckElapseTime = 1000;
bool CLightpack::sAlreadyRunning = false;

CLightpack::CLightpack(LPUNKNOWN pUnk, HRESULT *phr)
    : CTransInPlaceFilter(FILTER_NAME, pUnk, CLSID_Lightpack, phr)
    , mDevice(NULL)
    , mIsFirstInstance(false)
    , mWidth(0)
    , mHeight(0)
    , mFrameBuffer(0)
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
    , mIsRunning(false)
    , mIsConnectedToPrismatik(false)
    , mPropGamma(Lightpack::DefaultGamma)
    , mPropSmooth(DEFAULT_SMOOTH)
    , mPropBrightness(Lightpack::DefaultBrightness)
    , mPropPort(DEFAULT_GUI_PORT)
    , mHasReadSettings(false)
{
    mCurrentDirectoryCache[0] = '\0';

    if (!sAlreadyRunning) {
#ifdef _DEBUG
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
        connectDevice();

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

    delete[] mFrameBuffer;

    disconnectAllDevices();
    ASSERT(mDevice == NULL);

    // Once the first instance is deallocated, we should allow usage of this filter again
    if (mIsFirstInstance) {
        sAlreadyRunning = false;
        if (mAppMutex) {
            CloseHandle(mAppMutex);
        }
    }
#ifdef _DEBUG
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
            // Default position for 10 LEDs
            static const double defaultPositions[][4] = {
                { 85, 72.78, 15, 20.76 },
                { 85, 39.58, 15, 20.76 },
                { 85, 6.39, 15, 20.76 },
                { 81.68, 0, 11.68, 20 },
                { 39.92, 80, 11.68, 20 },
                { 0, 25.07, 15, 16.6 },
                { 6.6, 0, 11.68, 20 },
                { 0, 8.47, 15, 16.6 },
                { 0, 41.67, 15, 16.6 },
                { 0, 66.53, 15, 33.19 }
            };
            for (size_t i = 0; i < Lightpack::LedDevice::LedsPerDevice; i++) {
                Lightpack::Rect rect;
                percentageRectToVideoRect(defaultPositions[i][0], defaultPositions[i][1],
                    defaultPositions[i][2], defaultPositions[i][3], &rect);
                mScaledRects.push_back(rect);
            }

            // Load the settings file
            loadSettingsFile();

            if (!mFrameBuffer) {
                mFrameBuffer = new BYTE[mWidth * mHeight * 4];
                std::fill(mFrameBuffer, mFrameBuffer + sizeof(mFrameBuffer), 0);
            }
        }

        // Set the media type
        if (MEDIASUBTYPE_RGB32 == pmt->subtype) {
            log("Going to render RGB32 images");
            mVideoType = VideoFormat::RGB32;
        }
        else if (MEDIASUBTYPE_NV12 == pmt->subtype) {
            log("Going to render NV12 images");
            mVideoType = VideoFormat::NV12;
        }
        else {
            log("Going to render other images");
            mVideoType = VideoFormat::OTHER;
        }
    }
    return S_OK;
}

STDMETHODIMP CLightpack::Run(REFERENCE_TIME StartTime)
{
    logf("Run: %ld %lu %lu", getStreamTimeMilliSec(), ((CRefTime)StartTime).Millisecs(), ((CRefTime)m_tStart).Millisecs());
    if (m_State == State_Running) {
        return NOERROR;
    }

    HRESULT hr = CTransInPlaceFilter::Run(StartTime);
    if (FAILED(hr)) {
        log("Run failed");
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
    logf("Stop %ld", getStreamTimeMilliSec());
    CTransInPlaceFilter::Stop();
    CancelNotification();
    return NOERROR;
}

STDMETHODIMP CLightpack::Pause()
{
    logf("Pause %ld", getStreamTimeMilliSec());

    HRESULT hr = CTransInPlaceFilter::Pause();
    if (FAILED(hr)) {
        log("Failed to pause");
        return hr;
    }
    destroyLightThread();

    EnterCriticalSection(&mCommSendLock);
    mShouldSendPauseEvent = mIsRunning == true;
    LeaveCriticalSection(&mCommSendLock);

    mIsRunning = false;
    return NOERROR;
}

bool CLightpack::connectDevice()
{
    if (!mDevice) {
        EnterCriticalSection(&mDeviceLock);
        if (!mDevice) {
            mDevice = new Lightpack::LedDevice();
            if (!((Lightpack::LedDevice*)mDevice)->open()) {
                delete mDevice;
                mDevice = 0;
                log("Device not connected");
                LeaveCriticalSection(&mDeviceLock);
                return false;
            }
            else {
                postConnection();
                log("Device connected");
                startLightThread();
                mIsConnectedToPrismatik = false;
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
            log("Try to connect to Prismatik");
            mDevice = new Lightpack::PrismatikClient();
            const std::vector<int> blankLedMap;
            if (((Lightpack::PrismatikClient*)mDevice)->connect(DEFAULT_HOST, DEFAULT_PORT, blankLedMap, DEFAULT_APIKEY) != Lightpack::RESULT::OK
                || ((Lightpack::PrismatikClient*)mDevice)->lock() != Lightpack::RESULT::OK) {
                    log("Failed to also connect to Prismatik.");
                    delete mDevice;
                    mDevice = NULL;
                    LeaveCriticalSection(&mDeviceLock);
                    return false;
            }
            else {
                postConnection();
                log("Connected to Prismatik.");
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
            log("Disconnected device");
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
        if (!mAdviseQueue.empty()) {
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

void CLightpack::loadSettingsFile()
{
    if (!mHasReadSettings) {
        mHasReadSettings = true;

        // Prepare the path to the settings file
        char path[MAX_PATH];
        wcstombs(path, getCurrentDirectory(), wcslen(getCurrentDirectory()));
        path[wcslen(getCurrentDirectory())] = '\0';
        strcat(path, "\\"SETTINGS_FILE);

        INIReader reader(path);
        if (!reader.ParseError()) {
            readSettingsFile(reader);
        }
        else {
            // If there is no settings file there then we should try appdata file, otherwise just use defaults
            wchar_t buffer[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buffer))) {
                wcstombs(path, buffer, wcslen(buffer));
                path[wcslen(buffer)] = '\0';
                strcat(path, "\\"PROJECT_NAME"\\"SETTINGS_FILE);

                // Try again
                reader = INIReader(path);
                if (!reader.ParseError()) {
                    readSettingsFile(reader);
                }
            }
        }
    }
}

void CLightpack::readSettingsFile(INIReader& reader)
{
    mPropPort = reader.GetInteger("General", "port", DEFAULT_GUI_PORT);
    mPropSmooth = (unsigned char)reader.GetInteger("State", "smooth", DEFAULT_SMOOTH);
    mPropBrightness = (unsigned char)reader.GetInteger("State", "brightness", Lightpack::DefaultBrightness);
    mPropGamma = reader.GetReal("State", "gamma", Lightpack::DefaultGamma);

    // Read positions
    int i = 1;
    int x = 0, y = 0, w = 0, h = 0;
    char key[32] = "led1";
    std::vector<Lightpack::Rect> rects;
    Lightpack::Rect rect;
    while (parseLedRectLine(reader.Get("Positions", key, "").c_str(), &rect)) {
        rects.push_back(rect);
        logf("Got led %d: %d, %d, %d, %d", i, rect.x, rect.y, rect.width, rect.height);
        sprintf(key, "led%d", ++i);
    }
    updateScaledRects(rects);
}

bool CLightpack::parseLedRectLine(const char* line, Lightpack::Rect* outRect)
{
    if (mWidth != 0 && mHeight != 0) {
        if (strlen(line) > 0) {
            double x, y, w, h;
            if (sscanf(line, "%*c%*lf:%lf,%lf,%lf,%lf", &x, &y, &w, &h) != EOF) {
                percentageRectToVideoRect(x, y, w, h, outRect);
                return true;
            }
        }
    }
    return false;
}

void CLightpack::percentageRectToVideoRect(double x, double y, double w, double h, Lightpack::Rect* outRect)
{
    outRect->x = (int)(min(x / 100.0, 1) * mWidth);
    outRect->y = (int)(min(y / 100.0, 1) * mHeight);
    outRect->width = (int)(min(w / 100.0, 1) * mWidth);
    outRect->height = (int)(min(h / 100.0, 1) * mHeight);
}

void CLightpack::updateScaledRects(std::vector<Lightpack::Rect>& rects)
{
    EnterCriticalSection(&mScaledRectLock);
    size_t i = 0;
    for (; i < min(mScaledRects.size(), rects.size()); i++) {
        if (rects[i].width > 0 && rects[i].height > 0) {
            mScaledRects[i] = rects[i];
        }
    }
    for (; i < rects.size(); i++) {
        if (rects[i].width > 0 && rects[i].height > 0) {
            mScaledRects.push_back(rects[i]);
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
        log("Failed to create thread");
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

void CLightpack::queueLight(REFERENCE_TIME startTime)
{
    CAutoLock lock(m_pLock);

    EnterCriticalSection(&mScaledRectLock);
    Lightpack::RGBCOLOR* colors = new Lightpack::RGBCOLOR[mScaledRects.size()];
    for (size_t i = 0; i < mScaledRects.size(); i++) {
        switch (mVideoType) {
        case RGB32:
            colors[i] = meanColorFromRGB32(mScaledRects[i]);
            break;
        case NV12:
            colors[i] = meanColorFromNV12(mScaledRects[i]);
            break;
        default:
            colors[i] = 0;
        }
        logf("Pixel: %d [%d, %d, %d]", i, GET_RED(colors[i]), GET_GREEN(colors[i]), GET_BLUE(colors[i]));
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
    if (mDevice) {
        EnterCriticalSection(&mDeviceLock);
        if (mDevice) {
            if (mDevice->setColors(colors, mPropColors.size()) != Lightpack::RESULT::OK) {
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
    bool isConnected  = connectPrismatik();

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
        ASSERT(!mAdviseQueue.empty());

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
    log("Failed to advise time");
    return false;
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
            log(pType);
            // Get the newest stride
            if (pType->formattype == FORMAT_VideoInfo && pType->cbFormat >= sizeof(VIDEOINFOHEADER)) {
                VIDEOINFOHEADER *pVih = reinterpret_cast<VIDEOINFOHEADER*>(pType->pbFormat);
                mStride = pVih->bmiHeader.biWidth;
            }
            else if (pType->formattype == FORMAT_VideoInfo2 && pType->cbFormat >= sizeof(VIDEOINFOHEADER2)) {
                VIDEOINFOHEADER2 *pVih = reinterpret_cast<VIDEOINFOHEADER2*>(pType->pbFormat);
                mStride = pVih->bmiHeader.biWidth;
            }
            DeleteMediaType(pType);
        }
    }

    if (mDevice == NULL) {
        // Reconnect device every 1 seconds if not connected
        DWORD now = GetTickCount();
        if ((now - mLastDeviceCheck) > sDeviceCheckElapseTime) {
            connectDevice();
            mLastDeviceCheck = now;
        }
    } else {
        if (!mPropColors.empty() && mVideoType != VideoFormat::OTHER) {
            ASSERT(mStride >= mWidth);

            REFERENCE_TIME startTime, endTime;
            pSample->GetTime(&startTime, &endTime);

            // Do not render old frames, they are discarded anyways
            REFERENCE_TIME streamTime = getStreamTime();
            if (!streamTime || startTime > streamTime) {
                BYTE* pData = NULL;
                pSample->GetPointer(&pData);
                if (pData != NULL) {
                    gpu_memcpy(mFrameBuffer, pData, pSample->GetSize());
                    queueLight(startTime);

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
