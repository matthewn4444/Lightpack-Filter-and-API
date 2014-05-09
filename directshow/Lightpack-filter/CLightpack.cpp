#include "CLightpack.h"

const DWORD CLightpack::sDeviceCheckElapseTime = 2000;

CLightpack::CLightpack(LPUNKNOWN pUnk, HRESULT *phr)
: CTransInPlaceFilter(FILTER_NAME, pUnk, CLSID_Lightpack, phr)
, mDevice(NULL)
, mWidth(0)
, mHeight(0)
, mFrameBuffer(0)
, mhThread(INVALID_HANDLE_VALUE)
, mThreadId(0)
, mThreadStopRequested(false)
, mStride(0)
, mLastDeviceCheck(GetTickCount())
, mThreadCleanUpRequested(false)
{
#ifdef LOG_ENABLED
    mLog = new Log("log.txt");
#endif

    // Hard coded values from profile
    mLedArea = {
        { 2269, 947, 291, 493 },
        { 2268, 441, 294, 500 },
        { 2347, 0, 216, 441 },
        { 1835, 0, 514, 264 },
        { 1320, 0, 514, 264 },
        { 807, 0, 514, 263 },
        { 294, 0, 513, 262 },
        { 78, 0, 216, 441 },
        { 79, 440, 293, 500 },
        { 80, 941, 293, 506 }
    };

    InitializeCriticalSection(&mQueueLock);
    InitializeCriticalSection(&mAdviseLock);
    InitializeCriticalSection(&mDeviceLock);

    connectDevice();
}

CLightpack::~CLightpack(void)
{
    destroyThread();

    ASSERT(mThreadId == NULL);
    ASSERT(mhThread == INVALID_HANDLE_VALUE);
    ASSERT(mThreadStopRequested == false);

    delete[] mFrameBuffer;

    disconnectDevice();
#ifdef LOG_ENABLED
    if (mLog) {
        delete mLog;
        mLog = NULL;
    }
#endif
}

HRESULT CLightpack::CheckInputType(const CMediaType* mtIn)
{
    CAutoLock lock(m_pLock);

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
            if (MEDIASUBTYPE_RGB32 == pmt->subtype) {
                mVideoType = VideoFormat::RGB32;
            }
            else if (MEDIASUBTYPE_NV12 == pmt->subtype) {
                mVideoType = VideoFormat::NV12;
            }
            else {
                mVideoType = VideoFormat::OTHER;
            }

            int desktopWidth, desktopHeight;
            GetDesktopResolution(desktopWidth, desktopHeight);

            //logf("Desktop: %d %d", desktopWidth, desktopHeight);

            float scaleX = (float)mWidth / desktopWidth;
            float scaleY = (float)mHeight / desktopHeight;

            //logf("Scale: %f %f", scaleX, scaleY);

            for (size_t i = 0; i < mLedArea.size(); i++) {
                Lightpack::Rect rect = mLedArea[i];
                int x = (int)(scaleX * rect.x), y = (int)(scaleY * rect.y), w = (int)(scaleX * rect.width), h = (int)(scaleY * rect.height);
                mScaledRects.push_back({
                    x, y, ( x + w >= mWidth ? mWidth - x : w ), ( y + h >= mHeight ? mHeight - y : h )
                });

                //logf("%d [%d %d %d %d]", i, x, y, w, h);
            }

            if (!mFrameBuffer) {
                mFrameBuffer = new BYTE[mWidth * mHeight * 4];
                std::fill(mFrameBuffer, mFrameBuffer + sizeof(mFrameBuffer), 0);
            }
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

    CancelNotification();

    startThread();
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
    destroyThread();
    return NOERROR;
}

void CLightpack::connectDevice()
{
    if (!mDevice) {
        EnterCriticalSection(&mDeviceLock);
        if (!mDevice) {
            mDevice = new Lightpack::LedDevice();
            if (!mDevice->open()) {
                delete mDevice;
                mDevice = 0;
                log("Device not connected")
            }
            else {
                mDevice->setBrightness(100);
                mDevice->setSmooth(20);
                log("Device connected")
                startThread();
            }
        }
        LeaveCriticalSection(&mDeviceLock);
    }
}

void CLightpack::disconnectDevice()
{
    if (mDevice) {
        EnterCriticalSection(&mDeviceLock);
        if (mDevice) {
            delete mDevice;
            mDevice = NULL;
            log("Disconnected device");
            // DO NOT REMOVE THREAD HERE, there will be a deadlock
        }
        LeaveCriticalSection(&mDeviceLock);
    }
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

void CLightpack::startThread()
{
    if (mThreadId != NULL && mhThread != INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mThreadId == NULL);
    ASSERT(mhThread == INVALID_HANDLE_VALUE);

    mhThread = CreateThread(NULL, 0, ParsingThread, (void*) this, 0, &mThreadId);
    mThreadCleanUpRequested = false;

    ASSERT(mhThread);
    if (mhThread == NULL) {
        log("Failed to create thread");
    }
}

void CLightpack::destroyThread()
{
    if (mThreadId == NULL && mhThread == INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mThreadId != NULL);
    ASSERT(mhThread != INVALID_HANDLE_VALUE);

    EnterCriticalSection(&mQueueLock);
    mThreadStopRequested = true;
    LeaveCriticalSection(&mQueueLock);

    mDisplayLightEvent.Set();

    WaitForSingleObject(mhThread, INFINITE);
    CloseHandle(mhThread);

    // Reset Values
    mThreadId = 0;
    mhThread = INVALID_HANDLE_VALUE;
    mThreadStopRequested = false;
    mThreadCleanUpRequested = false;
    CancelNotification();
}

void CLightpack::queueLight(REFERENCE_TIME startTime)
{
    CAutoLock lock(m_pLock);

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
        //logf("Pixel: %d [%d, %d, %d]", i, RED(colors[i]), GREEN(colors[i]), BLUE(colors[i]));
    }

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
            if (mDevice->setColors(colors, mScaledRects.size()) != Lightpack::RESULT::OK) {
                // Device is/was disconnected
                LeaveCriticalSection(&mDeviceLock);
                mThreadCleanUpRequested = true;
                disconnectDevice();
                return;
            }
        }
        LeaveCriticalSection(&mDeviceLock);
    }
}

DWORD CLightpack::threadStart()
{
    while (true) {
        WaitForSingleObject(mDisplayLightEvent, INFINITE);

        EnterCriticalSection(&mQueueLock);

        if (mThreadStopRequested) {
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
    return pLightpack->threadStart();
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
    if (mThreadCleanUpRequested) {
        mThreadCleanUpRequested = false;
        destroyThread();
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
        // Reconnect device every 2 seconds if not connected
        DWORD now = GetTickCount();
        if ((now - mLastDeviceCheck) > sDeviceCheckElapseTime) {
            connectDevice();
            mLastDeviceCheck = now;
        }
    } else {
        if (!mScaledRects.empty() && mVideoType != VideoFormat::OTHER) {
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
