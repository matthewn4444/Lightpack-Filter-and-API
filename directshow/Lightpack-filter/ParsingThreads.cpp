#include "CLightpack.h"

static const BOOL sse2supported = ::IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE);

DWORD WINAPI CLightpack::ColorParsingThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->colorParsingThreadStart();
}

void CLightpack::startColorParsingThread()
{
    if (mColorParsingThreadId != NULL && mhColorParsingThread != INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mColorParsingThreadId == NULL);
    ASSERT(mhColorParsingThread == INVALID_HANDLE_VALUE);
    mhColorParsingThread = CreateThread(NULL, 0, ColorParsingThread, (void*) this, 0, &mColorParsingThreadId);

    ASSERT(mhColorParsingThread);
    if (mhColorParsingThread == NULL) {
        _log("Failed to create color parsing thread");
    }
}

void CLightpack::destroyColorParsingThread()
{
    if (mColorParsingThreadId == NULL && mhColorParsingThread == INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mColorParsingThreadId != NULL);
    ASSERT(mhColorParsingThread != INVALID_HANDLE_VALUE);

    mParseColorsEvent.Set();

    WaitForSingleObject(mhColorParsingThread, INFINITE);
    CloseHandle(mhColorParsingThread);

    // Reset Values
    mColorParsingThreadId = 0;
    mhColorParsingThread = INVALID_HANDLE_VALUE;
}

DWORD CLightpack::colorParsingThreadStart()
{
    while (!mLightThreadStopRequested) {
        WaitForSingleObject(mParseColorsEvent, INFINITE);

        if (mLightThreadStopRequested) {
            break;
        }

        while (mColorParsingRequested) {
            if (mLightThreadStopRequested) {
                break;
            }

            REFERENCE_TIME streamTime = getStreamTime(), startTime = mColorParsingTime;
            if ((streamTime && startTime <= streamTime)) {
                continue;
            }

            // Parse the data given to us by the
            mColorParsingRequested = false;
            queueLight(startTime, mFrameBuffer);

            // Falling behind, something else wants to be queued, move to next one...
            if (mColorParsingRequested) {
                continue;
            }

            // Schedule if the start time is defined, otherwise it is still buffering
            if (getStreamTime()) {
                ScheduleNextDisplay();
            }
        }
    }
    mColorParsingRequested = false;
    return 0;
}

void CLightpack::queueLight(REFERENCE_TIME startTime, BYTE* src)
{
    if (mColorParsingRequested) {
        return;
    }
    CAutoLock lock(m_pLock);

    EnterCriticalSection(&mScaledRectLock);
    Lightpack::RGBCOLOR* colors = new Lightpack::RGBCOLOR[mScaledRects.size()];

    switch (mVideoType) {
    case RGB32:
        for (size_t i = 0; i < mScaledRects.size() && !mColorParsingRequested; i++) {
            colors[i] = meanColorFromRGB32(src, mScaledRects[i]);
        }
        break;
    case NV12:
        if (sse2supported) {
            for (size_t i = 0; i < mScaledRects.size() && !mColorParsingRequested; i++) {
                colors[i] = meanColorFromNV12SSE(src, mScaledRects[i]);
            }
        }
        else {
            for (size_t i = 0; i < mScaledRects.size() && !mColorParsingRequested; i++) {
                colors[i] = meanColorFromNV12(src, mScaledRects[i]);
            }
        }
        break;
    default:
        std::fill(colors, colors + mScaledRects.size(), 0);
    }

    LeaveCriticalSection(&mScaledRectLock);

    if (mColorParsingRequested) {
        return;
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
    return false;
}
