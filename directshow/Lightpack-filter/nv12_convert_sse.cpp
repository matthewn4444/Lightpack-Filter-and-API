#include "CLightpack.h"

// Static constant variables
static const int divideShift = 6;
static const unsigned short shortMask = 0xFF00;
static const unsigned short setMask = 0xFFFF;
static const __m128i ysub = _mm_set1_epi16(16);
static const __m128i facy = _mm_set1_epi16(74);
static const __m128i uvsub = _mm_set1_epi16(128);
static const __m128i facrv = _mm_set1_epi16(102);    // 1.596        102
static const __m128i facgu = _mm_set1_epi16(25);     // 0.391        25
static const __m128i facgv = _mm_set1_epi16(52);     // 0.813        52
static const __m128i facbu = _mm_set1_epi16(129);    // 2.018        129

static const __m128i minZeroOdd = _mm_set1_epi16(0xFF);
static const __m128i minZeroEven = _mm_set1_epi16(shortMask);
static const __m128i zero = _mm_setzero_si128();
static const __m128i one = _mm_set1_epi16(1);

// Sums up all the 4 chunks of 8bit signed/unsigned chars to a 32 bit integer (4 in the end)
inline void __mm_sum_epi8(const __m128i& ucharReg, __m128i& uintStoreReg) {
    __m128i a = _mm_unpacklo_epi8(ucharReg, zero);
    __m128i b = _mm_unpackhi_epi8(ucharReg, zero);
    uintStoreReg = _mm_add_epi32(uintStoreReg, _mm_madd_epi16(a, one));
    uintStoreReg = _mm_add_epi32(uintStoreReg, _mm_madd_epi16(b, one));
}

void calculateFactors(const __m128i& u, const __m128i& v, __m128i& rv, __m128i& gu, __m128i& gv, __m128i& bu) {
    rv = _mm_mullo_epi16(facrv, v);
    gu = _mm_mullo_epi16(facgu, u);
    gv = _mm_mullo_epi16(facgv, v);
    bu = _mm_mullo_epi16(facbu, u);
}

// Unpacks the UVPlane specifically for NV12 and outputs 16 v and u values
void unpackUVPlane(const __m128i* uvp, __m128i& u0, __m128i& v0) {
    __m128i uv0, uvEven, uvOdd;
    uv0 = _mm_load_si128(uvp);

    // Get the U and V values
    uvEven = _mm_and_si128(uv0, minZeroOdd);
    uvOdd = _mm_slli_si128(uvEven, 1);
    u0 = _mm_adds_epu8(uvEven, uvOdd);

    uvOdd = _mm_and_si128(uv0, minZeroEven);
    uvEven = _mm_srli_si128(uvOdd, 1);
    v0 = _mm_adds_epu8(uvOdd, uvEven);
}

// Calculates the sum of r, g, b for a single row
void __calculate_row_i16(const __m128i& y, const __m128i& rv, const __m128i& gu, const __m128i& gv, const  __m128i& bu, __m128i& sumR, __m128i& sumG, __m128i& sumB) {
    __m128i r, g, b;

    r = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y, rv), divideShift), zero);
    g = _mm_packus_epi16(_mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y, gu), gv), divideShift), zero);
    b = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y, bu), divideShift), zero);
    __mm_sum_epi8(r, sumR);
    __mm_sum_epi8(g, sumG);
    __mm_sum_epi8(b, sumB);
}

// Calculates the sum of r, g, b for 2 rows because of common values
void __calculate_row_i32(const __m128i& y00, const __m128i& y01,
    const __m128i& rv00, const __m128i& rv01, const __m128i& gu00, const __m128i& gv00, const __m128i& gu01, const __m128i& gv01, const __m128i& bu00, const  __m128i& bu01,
    __m128i& sumR, __m128i& sumG, __m128i& sumB) {
    __m128i r00, r01, g00, g01, b00, b01;

    r00 = _mm_srai_epi16(_mm_add_epi16(y00, rv00), divideShift);
    r01 = _mm_srai_epi16(_mm_add_epi16(y01, rv01), divideShift);
    g00 = _mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y00, gu00), gv00), divideShift);
    g01 = _mm_srai_epi16(_mm_sub_epi16(_mm_sub_epi16(y01, gu01), gv01), divideShift);
    b00 = _mm_srai_epi16(_mm_add_epi16(y00, bu00), divideShift);
    b01 = _mm_srai_epi16(_mm_add_epi16(y01, bu01), divideShift);

    r00 = _mm_packus_epi16(r00, r01);         // rrrr.. saturated
    g00 = _mm_packus_epi16(g00, g01);         // gggg.. saturated
    b00 = _mm_packus_epi16(b00, b01);         // bbbb.. saturated

    __mm_sum_epi8(r00, sumR);
    __mm_sum_epi8(g00, sumG);
    __mm_sum_epi8(b00, sumB);
}

void prepareYAndCalculateRow(const __m128i* ySrc,
    const __m128i& rv0, const __m128i& rv1, const __m128i& gu0, const __m128i& gv0, const __m128i& gu1, const __m128i& gv1, const __m128i& bu0, const __m128i& bu1,
    __m128i& sumR, __m128i& SumG, __m128i& SumB) {
    __m128i y = _mm_load_si128(ySrc);
    __m128i yLo = _mm_mullo_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(y, zero), ysub), facy);
    __m128i yHi = _mm_mullo_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(y, zero), ysub), facy);
    __calculate_row_i32(yLo, yHi, rv0, rv1, gu0, gv0, gu1, gv1, bu0, bu1, sumR, SumG, SumB);
}

void prepareYAndCalculateRowWithMask(const __m128i* ySrc, const __m128i& loMask, const __m128i& hiMask,
    const __m128i& rv0, const __m128i& rv1, const __m128i& gu0, const __m128i& gv0, const __m128i& gu1, const __m128i& gv1, const __m128i& bu0, const __m128i& bu1,
    __m128i& sumR, __m128i& SumG, __m128i& SumB) {
    __m128i y = _mm_load_si128(ySrc);
    __m128i yLo = _mm_and_si128(_mm_mullo_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(y, zero), ysub), facy), loMask);
    __m128i yHi = _mm_and_si128(_mm_mullo_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(y, zero), ysub), facy), hiMask);
    __calculate_row_i32(yLo, yHi, rv0, rv1, gu0, gv0, gu1, gv1, bu0, bu1, sumR, SumG, SumB);
}

void prepareYAndCalculateRowLoWithMask(const __m128i* ySrc, const __m128i& mask,
    const __m128i& rv, const __m128i& gu, const __m128i& gv, const __m128i& bu,
    __m128i& sumR, __m128i& SumG, __m128i& SumB) {
    __m128i y = _mm_and_si128(_mm_mullo_epi16(_mm_sub_epi16(_mm_unpacklo_epi8(_mm_load_si128(ySrc), zero), ysub), facy), mask);
    __calculate_row_i16(y, rv, gu, gv, bu, sumR, SumG, SumB);
}

void prepareYAndCalculateRowHiWithMask(const __m128i* ySrc, const __m128i& mask,
    const __m128i& rv, const __m128i& gu, const __m128i& gv, const __m128i& bu,
    __m128i& sumR, __m128i& SumG, __m128i& SumB) {
    __m128i y = _mm_and_si128(_mm_mullo_epi16(_mm_sub_epi16(_mm_unpackhi_epi8(_mm_load_si128(ySrc), zero), ysub), facy), mask);
    __calculate_row_i16(y, rv, gu, gv, bu, sumR, SumG, SumB);
}

void prepareUVlo(const __m128i& u, const __m128i& v, __m128i& outU, __m128i& outV, __m128i& rv, __m128i& gu, __m128i& gv, __m128i& bu) {
    outU = _mm_sub_epi16(_mm_unpacklo_epi8(u, zero), uvsub);
    outV = _mm_sub_epi16(_mm_unpacklo_epi8(v, zero), uvsub);

    calculateFactors(outU, outV, rv, gu, gv, bu);
}

void prepareUVloWithMask(const __m128i& mask, const __m128i& u, const __m128i& v, __m128i& outU, __m128i& outV, __m128i& rv, __m128i& gu, __m128i& gv, __m128i& bu) {
    outU = _mm_sub_epi16(_mm_unpacklo_epi8(u, zero), uvsub);
    outV = _mm_sub_epi16(_mm_unpacklo_epi8(v, zero), uvsub);

    outU = _mm_and_si128(outU, mask);
    outV = _mm_and_si128(outV, mask);

    calculateFactors(outU, outV, rv, gu, gv, bu);
}

void prepareUVhi(const __m128i& u, const __m128i& v, __m128i& outU, __m128i& outV, __m128i& rv, __m128i& gu, __m128i& gv, __m128i& bu) {
    outU = _mm_sub_epi16(_mm_unpackhi_epi8(u, zero), uvsub);
    outV = _mm_sub_epi16(_mm_unpackhi_epi8(v, zero), uvsub);

    calculateFactors(outU, outV, rv, gu, gv, bu);
}

void prepareUVhiWithMask(const __m128i& mask, const __m128i& u, const __m128i& v, __m128i& outU, __m128i& outV, __m128i& rv, __m128i& gu, __m128i& gv, __m128i& bu) {
    outU = _mm_sub_epi16(_mm_unpackhi_epi8(u, zero), uvsub);
    outV = _mm_sub_epi16(_mm_unpackhi_epi8(v, zero), uvsub);

    outU = _mm_and_si128(outU, mask);
    outV = _mm_and_si128(outV, mask);

    calculateFactors(outU, outV, rv, gu, gv, bu);
}

// Converts NV12 source from the rectangle given and returns the average color
Lightpack::RGBCOLOR CLightpack::meanColorFromNV12SSE(BYTE* src, Lightpack::Rect& rect)
{
    const size_t sampleWidth = mStride == 0 ? mWidth : mStride;
    const size_t pixelTotal = sampleWidth * mHeight;
    const size_t area = rect.area();

    __m128i u0, v0;
    __m128i u00, u01, v00, v01;
    __m128i rv00, rv01, gu00, gu01, gv00, gv01, bu00, bu01;
    __m128i *srcy128r0, *srcy128r1;
    __m128i *srcuv128;
    __m128i wRemainingLoMask = _mm_setzero_si128();
    __m128i wRemainingHiMask = _mm_setzero_si128();
    __m128i wStartLoMask = _mm_setzero_si128();
    __m128i wStartHiMask = _mm_setzero_si128();

    __m128i sumRed = _mm_setzero_si128();
    __m128i sumGreen = _mm_setzero_si128();
    __m128i sumBlue = _mm_setzero_si128();
    BYTE* ySrc, *uvSrc;
    size_t x, y;

    // Fix source alignment for copying by shifting the rect's x
    int xOffset = 0;
    int startX = rect.x;
    ySrc = src + rect.x + rect.y * sampleWidth;
    bool isAligned = (((size_t)ySrc) & 0xF) == 0;
    if (!isAligned) {
        size_t alignedPos = ((size_t)ySrc / 16) * 16;
        xOffset = (size_t)ySrc - alignedPos;

        // Check to see if we can go left of the image
        ASSERT(rect.x - xOffset >= 0);          // If this fails, then the width is not 16bit aligned and this will need to be handled
        startX -= xOffset;
        ySrc -= xOffset;
    }
    uvSrc = src + pixelTotal + (rect.y / 2) * sampleWidth + (startX & 0x1 ? startX - 1 : startX);

    // Calculate the optimal height and width size to run the normal routine without mask
    const bool isHeightOdd = rect.height & 0x1;
    const size_t optimalHeight = isHeightOdd ? rect.height - 1 : rect.height;

    size_t optimalWidth = ((rect.width + xOffset) / 16) * 16;
    int remainingWidth = (rect.width + xOffset) - optimalWidth;

    // Get the mask to remove bytes off the non-aligned 16 pixels
    if (remainingWidth > 0) {
        wRemainingLoMask = _mm_setr_epi16(
            setMask,
            remainingWidth > 1 ? setMask : 0,
            remainingWidth > 2 ? setMask : 0,
            remainingWidth > 3 ? setMask : 0,
            remainingWidth > 4 ? setMask : 0,
            remainingWidth > 5 ? setMask : 0,
            remainingWidth > 6 ? setMask : 0,
            remainingWidth > 7 ? setMask : 0);

        if (remainingWidth > 8) {
            wRemainingHiMask = _mm_setr_epi16(
                setMask,
                remainingWidth > 9 ? setMask : 0,
                remainingWidth > 10 ? setMask : 0,
                remainingWidth > 11 ? setMask : 0,
                remainingWidth > 12 ? setMask : 0,
                remainingWidth > 13 ? setMask : 0,
                remainingWidth > 14 ? setMask : 0,
                remainingWidth > 15 ? setMask : 0);
        }
    }

    int startOffset = 16 - xOffset;
    if (startOffset > 0) {
        optimalWidth -= 16;
        wStartHiMask = _mm_set_epi16(
            setMask,
            startOffset > 1 ? setMask : 0,
            startOffset > 2 ? setMask : 0,
            startOffset > 3 ? setMask : 0,
            startOffset > 4 ? setMask : 0,
            startOffset > 5 ? setMask : 0,
            startOffset > 6 ? setMask : 0,
            startOffset > 7 ? setMask : 0);

        if (startOffset > 8) {
            wStartLoMask = _mm_set_epi16(
                setMask,
                startOffset > 9 ? setMask : 0,
                startOffset > 10 ? setMask : 0,
                startOffset > 11 ? setMask : 0,
                startOffset > 12 ? setMask : 0,
                startOffset > 13 ? setMask : 0,
                startOffset > 14 ? setMask : 0,
                startOffset > 15 ? setMask : 0);
        }
    }

    for (y = 0; y < optimalHeight; y += 2) {
        srcy128r0 = (__m128i *)(ySrc + sampleWidth * y);
        srcy128r1 = (__m128i *)(ySrc + sampleWidth * y + sampleWidth);
        srcuv128 = (__m128i *)(uvSrc + (sampleWidth / 2) * y);

        // Do not do the pixels before the start offset
        if (startOffset) {
            unpackUVPlane(srcuv128++, u0, v0);
            prepareUVhiWithMask(wStartHiMask, u0, v0, u01, v01, rv01, gu01, gv01, bu01);
            if (startOffset >= 8) {
                // Do both low and high
                prepareUVloWithMask(wStartLoMask, u0, v0, u00, v00, rv00, gu00, gv00, bu00);

                prepareYAndCalculateRowWithMask(srcy128r0++, wStartLoMask, wStartHiMask, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
                prepareYAndCalculateRowWithMask(srcy128r1++, wStartLoMask, wStartHiMask, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
            }
            else {
                // Do only high
                prepareYAndCalculateRowHiWithMask(srcy128r0++, wStartHiMask, rv01, gu01, gv01, bu01, sumRed, sumGreen, sumBlue);
                prepareYAndCalculateRowHiWithMask(srcy128r1++, wStartHiMask, rv01, gu01, gv01, bu01, sumRed, sumGreen, sumBlue);
            }
        }

        for (x = 0; x < optimalWidth; x += 16) {
            unpackUVPlane(srcuv128++, u0, v0);
            prepareUVlo(u0, v0, u00, v00, rv00, gu00, gv00, bu00);
            prepareUVhi(u0, v0, u01, v01, rv01, gu01, gv01, bu01);

            prepareYAndCalculateRow(srcy128r0++, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
            prepareYAndCalculateRow(srcy128r1++, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
        }

        // Handle left over width that was not divisible by 16
        if (remainingWidth) {
            unpackUVPlane(srcuv128++, u0, v0);
            prepareUVloWithMask(wRemainingLoMask, u0, v0, u00, v00, rv00, gu00, gv00, bu00);
            if (remainingWidth >= 8) {
                // Do both low and high
                prepareUVhiWithMask(wRemainingHiMask, u0, v0, u01, v01, rv01, gu01, gv01, bu01);

                prepareYAndCalculateRowWithMask(srcy128r0++, wRemainingLoMask, wRemainingHiMask, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
                prepareYAndCalculateRowWithMask(srcy128r1++, wRemainingLoMask, wRemainingHiMask, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
            }
            else {
                // Do only low
                prepareYAndCalculateRowLoWithMask(srcy128r0++, wRemainingLoMask, rv00, gu00, gv00, bu00, sumRed, sumGreen, sumBlue);
                prepareYAndCalculateRowLoWithMask(srcy128r1++, wRemainingLoMask, rv00, gu00, gv00, bu00, sumRed, sumGreen, sumBlue);
            }
        }
    }

    // Handle the last row if height is odd
    if (isHeightOdd) {
        srcy128r0 = (__m128i *)(ySrc + sampleWidth * y);
        srcuv128 = (__m128i *)(uvSrc + (sampleWidth / 2) * y);

        // Do not do the pixels before the start offset
        if (startOffset) {
            unpackUVPlane(srcuv128++, u0, v0);
            prepareUVhiWithMask(wStartHiMask, u0, v0, u01, v01, rv01, gu01, gv01, bu01);
            if (startOffset >= 8) {
                // Do both low and high
                prepareUVloWithMask(wStartLoMask, u0, v0, u00, v00, rv00, gu00, gv00, bu00);
                prepareYAndCalculateRowWithMask(srcy128r0++, wStartLoMask, wStartHiMask, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
            }
            else {
                // Do only high
                prepareYAndCalculateRowHiWithMask(srcy128r0++, wStartHiMask, rv01, gu01, gv01, bu01, sumRed, sumGreen, sumBlue);
            }
        }

        for (x = 0; x < optimalWidth; x += 16) {
            unpackUVPlane(srcuv128++, u0, v0);
            prepareUVlo(u0, v0, u00, v00, rv00, gu00, gv00, bu00);
            prepareUVhi(u0, v0, u01, v01, rv01, gu01, gv01, bu01);

            prepareYAndCalculateRow(srcy128r0++, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
        }

        // Handle left over width that was not divisible by 16
        if (remainingWidth) {
            unpackUVPlane(srcuv128++, u0, v0);
            prepareUVloWithMask(wRemainingLoMask, u0, v0, u00, v00, rv00, gu00, gv00, bu00);
            if (remainingWidth >= 8) {
                // Do both low and high
                prepareUVhiWithMask(wRemainingHiMask, u0, v0, u01, v01, rv01, gu01, gv01, bu01);
                prepareYAndCalculateRowWithMask(srcy128r0++, wRemainingLoMask, wRemainingHiMask, rv00, rv01, gu00, gv00, gu01, gv01, bu00, bu01, sumRed, sumGreen, sumBlue);
            }
            else {
                // Do only low
                prepareYAndCalculateRowLoWithMask(srcy128r0++, wRemainingLoMask, rv00, gu00, gv00, bu00, sumRed, sumGreen, sumBlue);
            }
        }
    }

    // Average the accumative sum of colors
    size_t red = 0;
    red += _mm_extract_epi16(sumRed, 1) << 16 | _mm_extract_epi16(sumRed, 0);
    red += _mm_extract_epi16(sumRed, 3) << 16 | _mm_extract_epi16(sumRed, 2);
    red += _mm_extract_epi16(sumRed, 5) << 16 | _mm_extract_epi16(sumRed, 4);
    red += _mm_extract_epi16(sumRed, 7) << 16 | _mm_extract_epi16(sumRed, 6);
    red /= area;

    size_t green = 0;
    green += _mm_extract_epi16(sumGreen, 1) << 16 | _mm_extract_epi16(sumGreen, 0);
    green += _mm_extract_epi16(sumGreen, 3) << 16 | _mm_extract_epi16(sumGreen, 2);
    green += _mm_extract_epi16(sumGreen, 5) << 16 | _mm_extract_epi16(sumGreen, 4);
    green += _mm_extract_epi16(sumGreen, 7) << 16 | _mm_extract_epi16(sumGreen, 6);
    green /= area;

    size_t blue = 0;
    blue += _mm_extract_epi16(sumBlue, 1) << 16 | _mm_extract_epi16(sumBlue, 0);
    blue += _mm_extract_epi16(sumBlue, 3) << 16 | _mm_extract_epi16(sumBlue, 2);
    blue += _mm_extract_epi16(sumBlue, 5) << 16 | _mm_extract_epi16(sumBlue, 4);
    blue += _mm_extract_epi16(sumBlue, 7) << 16 | _mm_extract_epi16(sumBlue, 6);
    blue /= area;
    return RGB(red, green, blue);
}
