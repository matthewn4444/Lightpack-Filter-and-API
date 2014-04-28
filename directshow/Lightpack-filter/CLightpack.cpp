#include "CLightpack.h"

CLightpack::CLightpack(LPUNKNOWN pUnk, HRESULT *phr)
: CTransInPlaceFilter(FILTER_NAME, pUnk, CLSID_Lightpack, phr)
, mDevice(NULL)
, mWidth(0)
, mHeight(0)
, thing(0)
, mFrameBuffer(0)
{
    mLog = new Log("log.txt");

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

    mDevice = new Lightpack::LedDevice();
    if (!mDevice->open()) {
        delete mDevice;
        mDevice = 0;
    }
    else {
        mDevice->setBrightness(100);
        mDevice->setSmooth(20);
    }
}

CLightpack::~CLightpack(void)
{
    if (mDevice) {
        delete mDevice;
        mDevice = NULL;
    }

    if (mLog) {
        delete mLog;
        mLog = NULL;
    }
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
            }
        }
    }
    return S_OK;
}

STDMETHODIMP CLightpack::Pause()
{
    log("pause");
    return CTransInPlaceFilter::Pause();
}

STDMETHODIMP CLightpack::Stop()
{
    log("stop");
    /*
    if (stuff) {
        FILE* f = fopen("block.dat", "wb");
        if (f) {
            fwrite(stuff, 1, 1280 * 720 * 4, f);
            fclose(f);
        }
    }
    */
    return CTransInPlaceFilter::Stop();
}

void CLightpack::parseFrameThread()
{
    Timer timer;
    timer.start();

    memcpy(mFrameBuffer, m_pData, mWidth * mHeight * 4);

    mDevice->pauseUpdating();
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
        mDevice->setColor(i, (int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels));
        //logf("Pixel: Led: %d  [%d %d %d]", (i + 1), (int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels))
    }
    mDevice->resumeUpdating();

    logf("Elapsed: %f", timer.elapsed());
    timer.stop();
}

HRESULT CLightpack::Transform(IMediaSample *pSample)
{
    if (mDevice != NULL && !mScaledRects.empty()) {
        if (mVideoType == MEDIASUBTYPE_RGB32) {
            pSample->GetPointer(&m_pData);

            parseFrameThread();
        }
    }

    thing++;

    if (thing == 24) {
        thing = 0;
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
