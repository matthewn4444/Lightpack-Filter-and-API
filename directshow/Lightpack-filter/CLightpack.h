#ifndef __CLIGHTPACK__
#define __CLIGHTPACK__

#include <streams.h>
#include <Lightpack.h>

#include <vector>

// TEMP
//#define LOG_ENABLED

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

/*
typedef     unsigned int        UINT;
#define     CACHED_BUFFER_SIZE  4096   

void CopyFrame(void * pSrc, void * pDest, UINT width, UINT height, UINT pitch)
{
    tbb::enumerable_thread_specific<cache_buffer> cache_buffers;

    void * pCacheBlock = cache_buffers.local().data;

    __m128i x0, x1, x2, x3;
    __m128i *pLoad;
    __m128i *pStore;
    __m128i *pCache;
    UINT x, y, yLoad, yStore;
    UINT rowsPerBlock;
    UINT width64;
    UINT extraPitch;

    rowsPerBlock = CACHED_BUFFER_SIZE / pitch;
    width64 = (width + 63) & ~0x03f;
    extraPitch = (pitch - width64) / 16;

    pLoad = (__m128i *)pSrc;
    pStore = (__m128i *)pDest;

    // COPY THROUGH 4KB CACHED BUFFER
    for (y = 0; y < height; y += rowsPerBlock)
    {
        // ROWS LEFT TO COPY AT END
        if (y + rowsPerBlock > height)
            rowsPerBlock = height - y;

        pCache = (__m128i *)pCacheBlock;

        _mm_mfence();

        // LOAD ROWS OF PITCH WIDTH INTO CACHED BLOCK
        for (yLoad = 0; yLoad < rowsPerBlock; yLoad++)
        {
            // COPY A ROW, CACHE LINE AT A TIME
            for (x = 0; x < pitch; x += 64)
            {
                x0 = _mm_stream_load_si128(pLoad + 0);
                x1 = _mm_stream_load_si128(pLoad + 1);
                x2 = _mm_stream_load_si128(pLoad + 2);
                x3 = _mm_stream_load_si128(pLoad + 3);

                _mm_store_si128(pCache + 0, x0);
                _mm_store_si128(pCache + 1, x1);
                _mm_store_si128(pCache + 2, x2);
                _mm_store_si128(pCache + 3, x3);

                pCache += 4;
                pLoad += 4;
            }
        }

        _mm_mfence();

        pCache = (__m128i *)pCacheBlock;

        // STORE ROWS OF FRAME WIDTH FROM CACHED BLOCK
        for (yStore = 0; yStore < rowsPerBlock; yStore++)
        {
            // copy a row, cache line at a time
            for (x = 0; x < width64; x += 64)
            {
                x0 = _mm_load_si128(pCache);
                x1 = _mm_load_si128(pCache + 1);
                x2 = _mm_load_si128(pCache + 2);
                x3 = _mm_load_si128(pCache + 3);

                _mm_stream_si128(pStore, x0);
                _mm_stream_si128(pStore + 1, x1);
                _mm_stream_si128(pStore + 2, x2);
                _mm_stream_si128(pStore + 3, x3);

                pCache += 4;
                pStore += 4;
            }

            pCache += extraPitch;
            pStore += extraPitch;
        }
    }
}
*/


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

    STDMETHODIMP Pause();
    STDMETHODIMP Stop();

    static CUnknown *WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

private:
    static DWORD WINAPI ParsingThread(LPVOID lpvThreadParm);
    void updateLights();

    // Thread function
    void startThread();
    void destroyThread();
    DWORD threadStart();

    Log* mLog;
    std::vector<Lightpack::Rect> mLedArea;
    std::vector<Lightpack::Rect> mScaledRects;

    // Threading variables
    HANDLE mhThread;
    DWORD mThreadId;
    bool mThreadStopRequested;
    CRITICAL_SECTION mBufferLock;
    CONDITION_VARIABLE mShouldRunUpdate;
    bool mIsWorking;

    Lightpack::LedDevice* mDevice;

    GUID mVideoType;
    int mStride;
    VIDEOINFOHEADER mVidHeader;
    int mWidth;
    int mHeight;
    int thing;

    BYTE* mFrameBuffer;
    BYTE* m_pData;
};

#endif // __CLIGHTPACK__