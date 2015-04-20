#include "CLightpack.h"

Lightpack::RGBCOLOR CLightpack::meanColorFromRGB32(Lightpack::Rect& rect) {
    ASSERT(mStride >= mWidth || mStride == 0);
    const unsigned int sampleWidth = mStride == 0 ? mWidth : mStride;
    const unsigned int totalPixels = rect.area();
    const int yOffset = mSampleUpsideDown ? 1 - mHeight : 0;
    const int yMakePositive = mSampleUpsideDown ? -1 : 1;

    unsigned int totalR = 0, totalG = 0, totalB = 0;
    for (int r = 0; r < rect.height; r++) {
        int y = (rect.y + r + yOffset) * yMakePositive;

        BYTE* pixel = mFrameBuffer + (rect.x + y * sampleWidth) * 4;      // 4 bytes per pixel
        for (int c = 0; c < rect.width; c++) {
            totalB += pixel[0];
            totalG += pixel[1];
            totalR += pixel[2];
            pixel += 4;
        }
    }
    return RGB((int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels));
}

Lightpack::RGBCOLOR CLightpack::meanColorFromNV12(Lightpack::Rect& rect) {
    ASSERT(mStride >= mWidth || mStride == 0);
    const unsigned int sampleWidth = mStride == 0 ? mWidth : mStride;
    const unsigned int pixel_total = sampleWidth * mHeight;
    const unsigned int totalPixels = rect.area();
    BYTE* Y = mFrameBuffer;
    BYTE* U = mFrameBuffer + pixel_total;
    BYTE* V = mFrameBuffer + pixel_total + 1;
    const int dUV = 2;

    BYTE* U_pos = U;
    BYTE* V_pos = V;

    // YUV420 to RGB
    unsigned int totalR = 0, totalG = 0, totalB = 0;
    for (int r = 0; r < rect.height; r++) {
        int y = r + rect.y;

        Y = mFrameBuffer + y * sampleWidth + rect.x;
        U = mFrameBuffer + pixel_total + (y / 2) * sampleWidth + (rect.x & 0x1 ? rect.x - 1 : rect.x);
        V = U + 1;

        for (int c = 0; c < rect.width; c++) {
            Lightpack::RGBCOLOR color = YUVToRGB(*(Y++), *U, *V);
            totalR += GET_RED(color);
            totalG += GET_GREEN(color);
            totalB += GET_BLUE(color);

            if ((rect.x + c) & 0x1) {
                U += dUV;
                V += dUV;
            }
        }
    }
    return RGB((int)floor(totalR / totalPixels), (int)floor(totalG / totalPixels), (int)floor(totalB / totalPixels));
}
