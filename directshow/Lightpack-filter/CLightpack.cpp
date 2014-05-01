#include "CLightpack.h"

#define RED(n)      n & 0xFF
#define GREEN(n)    (n & 0xFF00) >> 8
#define BLUE(n)     (n & 0xFF0000) >> 16

CLightpack::CLightpack(LPUNKNOWN pUnk, HRESULT *phr)
: CTransInPlaceFilter(FILTER_NAME, pUnk, CLSID_Lightpack, phr)
, mDevice(NULL)
, mWidth(0)
, mHeight(0)
, mFrameBuffer(0)
, mhThread(INVALID_HANDLE_VALUE)
, mThreadId(0)
, mThreadStopRequested(false)
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
    }


    startThread();
}

CLightpack::~CLightpack(void)
{
    destroyThread();

    delete[] mFrameBuffer;

    // Free the queue's memory usage
    EnterCriticalSection(&mQueueLock);
    size_t items = mColorQueue.size();
    for (size_t i = 0; i < items; i++) {
        std::pair<REFERENCE_TIME, COLORREF*>& pair = mColorQueue.front();
        COLORREF* colors = pair.second;
        delete[] colors;
        mColorQueue.pop();
    }
    LeaveCriticalSection(&mQueueLock);

    if (mDevice) {
        delete mDevice;
        mDevice = NULL;
    }
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

    if (mtIn->subtype != MEDIASUBTYPE_RGB555 &&
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

    return E_FAIL;
}

HRESULT CLightpack::SetMediaType(PIN_DIRECTION direction, const CMediaType *pmt)
{
    if (direction == PINDIR_INPUT)
    {
        mVideoType = pmt->subtype;
        VIDEOINFOHEADER* pvih = (VIDEOINFOHEADER*)pmt->pbFormat;
        mVidHeader = *pvih;

        BITMAPINFOHEADER bih = mVidHeader.bmiHeader;
        mStride = bih.biBitCount / 8 * bih.biWidth;
        mWidth = pvih->bmiHeader.biWidth;
        mHeight = pvih->bmiHeader.biHeight;

        // Scale all the rects to the size of the video
        if (mScaledRects.empty() && mWidth > 0 && mHeight > 0) {
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

void CLightpack::startThread()
{
    CAutoLock lock(m_pLock);

    mhThread = CreateThread(NULL, 0, ParsingThread, (void*) this, 0, &mThreadId);

    ASSERT(mhThread);
    if (mhThread == NULL) {
        log("Failed to create thread");
    }
}

void CLightpack::destroyThread()
{
    CAutoLock lock(m_pLock);

    EnterCriticalSection(&mQueueLock);
    mThreadStopRequested = true;
    LeaveCriticalSection(&mQueueLock);

    mDisplayLightEvent.Set();

    WaitForSingleObject(mhThread, INFINITE);
    CloseHandle(mhThread);
}

void CLightpack::queueLight(REFERENCE_TIME startTime)
{
    CAutoLock lock(m_pLock);

    COLORREF* colors = new COLORREF[mScaledRects.size()];
    for (size_t i = 0; i < mScaledRects.size(); i++) {
        Lightpack::Rect& rect = mScaledRects[i];
        int totalPixels = rect.area();

        // Find the average color
        unsigned int totalR = 0, totalG = 0, totalB = 0;
        for (int r = 0; r < rect.height; r++) {
            int y = rect.y + r;
            BYTE* pixel = mFrameBuffer + (rect.x + y * mWidth) * 4;      // 4 bytes per pixel

            for (int c = 0; c < rect.width; c++) {
                totalB += pixel[0];
                totalG += pixel[1];
                totalR += pixel[2];
                pixel += 4;
            }
        }

        colors[i] = RGB((int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels));
        logf("Pixel: Led: %d  [%d %d %d]", (i + 1), (int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels))
    }

    EnterCriticalSection(&mQueueLock);
    mColorQueue.push(std::make_pair(startTime, colors));
    LeaveCriticalSection(&mQueueLock);
}

void CLightpack::displayLight(COLORREF* colors)
{
    CAutoLock lock(m_pLock);
    mDevice->pauseUpdating();
    for (size_t i = 0; i < mScaledRects.size(); i++) {
        COLORREF color = colors[i];
        mDevice->setColor(i, RED(color), GREEN(color), BLUE(color));
    }
    mDevice->resumeUpdating();
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

        std::pair<REFERENCE_TIME, COLORREF*>& pair = mColorQueue.front();
        COLORREF* colors = pair.second;
        mColorQueue.pop();

        LeaveCriticalSection(&mQueueLock);

        logf("  Update colors [Stream Time: %lu]", getStreamTime());

        displayLight(colors);
        delete[] colors;
    }
    return 0;
}

DWORD WINAPI CLightpack::ParsingThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->threadStart();
}

bool CLightpack::ScheduleNextDisplay()
{
    EnterCriticalSection(&mQueueLock);
    REFERENCE_TIME sampleTime = -1;
    if (!mColorQueue.empty()) {
        std::pair<REFERENCE_TIME, COLORREF*> pair = mColorQueue.front();
        sampleTime = pair.first;
    }
    LeaveCriticalSection(&mQueueLock);
    if (sampleTime == -1) {
        return false;
    }

    logf("Schedule next time at %lu now %lu", ((CRefTime)sampleTime).Millisecs(), getStreamTime());

    DWORD_PTR adviseTime;
    HRESULT hr = m_pClock->AdviseTime(
        m_tStart,
        sampleTime,
        (HEVENT)(HANDLE)mDisplayLightEvent,
        &adviseTime);

    if (SUCCEEDED(hr)) {
        return true;
    }
    log("Failed to advise time");
    return false;
}

HRESULT CLightpack::Transform(IMediaSample *pSample)
{
    if (mDevice != NULL && !mScaledRects.empty()) {
        if (mVideoType == MEDIASUBTYPE_RGB32) {
            BYTE* pData = NULL;
            pSample->GetPointer(&pData);
            if (pData != NULL) {
                CopyFrame(mFrameBuffer, pData, mWidth, mHeight);

                // Get the time for this frame and queue it
                REFERENCE_TIME startTime, endTime;
                pSample->GetTime(&startTime, &endTime);
                queueLight(startTime);

                // Schedule if the start time is defined, otherwise it is still buffering
                if (m_tStart) {
                    ScheduleNextDisplay();
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