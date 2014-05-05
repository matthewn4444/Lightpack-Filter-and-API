#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <sstream>
#include <streams.h>

using namespace std;

class Log {
public:
    Log(const char* filename) {
        if (filename == NULL) {
            throw new exception("Did not specify a filename to save log file.");
        }
        else {
            mFile = fopen(filename, "w");
        }
    }
    ~Log() {
        if (mFile) {
            flush();
            fclose(mFile);
        }
    }

    void flush() {
        fflush(mFile);
    }

    void log(const char* str) {
        fwrite(str, 1, strlen(str), mFile);
        newLine();
    }

    void log(int num) {
        string a = to_string(num);
        log(a.c_str());
    }

    void log(float num) {
        string a = to_string(num);
        log(a.c_str());
    }

    void logf(const char* format, ...) {
        va_list vl;
        char buffer[1096];
        va_start(vl, format);
        vsprintf(buffer, format, vl);
        va_end(vl);
        log(buffer);
    }

    void log(AM_MEDIA_TYPE* type) {
        if (type) {
            logf("AM_MEDIA_TYPE => bFixedSizeSamples: %s, bTemporalCompression: %s, cbFormat: %lu, "
                "isVideoHeader2: %s, lSampleSize: %lu, isNV12: %s",
                type->bFixedSizeSamples ? "true" : "false",
                type->bTemporalCompression ? "true" : "false",
                type->cbFormat,
                type->formattype == FORMAT_VideoInfo2 ? "true" : "false",
                type->lSampleSize,
                type->subtype == MEDIASUBTYPE_NV12 ? "true" : "false");
            if (type->formattype == FORMAT_VideoInfo2 && type->cbFormat >= sizeof(VIDEOINFOHEADER2)) {
                VIDEOINFOHEADER2 *pVih = reinterpret_cast<VIDEOINFOHEADER2*>(type->pbFormat);
                log(pVih, true);
            }
        }
    }

    void log(VIDEOINFOHEADER2* vih, bool indent = false) {
        if (vih) {
            RECT& rect = vih->rcSource;
            RECT& rect2 = vih->rcTarget;
            std::string indentStr = indent ? "\t" : "";
            logf("%sVideoHeader2 => avgtime/frame: %lu, BitErrRate: %d, bitrate: %d, dwPictAspectRatioX: %d, "
                "dwPictAspectRatioY: %d, dwControlFlags: %d, Rect Source [%d, %d, %d, %d], Rect Target [%d, %d, %d, %d]",
                indentStr.c_str(),
                vih->AvgTimePerFrame,
                vih->dwBitErrorRate,
                vih->dwBitRate,
                vih->dwPictAspectRatioX,
                vih->dwPictAspectRatioY,
                vih->dwControlFlags,
                rect.left, rect.top, rect.right, rect.bottom,
                rect2.left, rect2.top, rect2.right, rect2.bottom);
            log(vih->bmiHeader, true);
        }
    }

    void log(const BITMAPINFOHEADER& header, bool indent = false) {
        std::string indentStr = indent ? "\t" : "";
        logf("%sbmiHeader => biBitCount: %ld, biClrImportant: %d, biClrUsed: %d, biCompression: %d, height: %d, width: %d, biPlanes: %d, biSize: %lu, "
            "biSizeImage: %d, biXPelsPerMeter: %d",
            indentStr.c_str(),
            header.biBitCount,
            header.biClrImportant,
            header.biClrUsed,
            header.biCompression,
            header.biHeight,
            header.biWidth,
            header.biPlanes,
            header.biSize,
            header.biSizeImage,
            header.biXPelsPerMeter);
    }

private:
    void newLine() {
        fwrite("\n", 1, 1, mFile);
    }

    FILE* mFile;
};

#endif // __LOG_H__