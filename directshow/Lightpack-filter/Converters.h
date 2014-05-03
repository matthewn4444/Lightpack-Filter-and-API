/*
* Copyright (C) 2011 The Android Open Source Project
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef HW_EMULATOR_CAMERA_CONVERTERS_EDITTED_H
#define HW_EMULATOR_CAMERA_CONVERTERS_EDITTED_H

// This file is editted from Android's source
// https://github.com/allwinner-dev-team/android_device_allwinner_common/blob/master/camera/Converters.h


#ifdef _WIN32
#define __inline__ __inline
#endif

/* Clips a value to the unsigned 0-255 range, treating negative values as zero.
*/
static __inline__ int
clamp(int x)
{
    if (x > 255) return 255;
    if (x < 0) return 0;
    return x;
}

/* "Optimized" macros that take specialy prepared Y, U, and V values:
* C = Y - 16
* D = U - 128
* E = V - 128
*/
#define YUV2RO(C, D, E) clamp((298 * (C) + 409 * (E) + 128) >> 8)
#define YUV2GO(C, D, E) clamp((298 * (C) - 100 * (D) - 208 * (E) + 128) >> 8)
#define YUV2BO(C, D, E) clamp((298 * (C) + 516 * (D) + 128) >> 8)

/* Converts YUV color to RGB32. */
static __inline__ COLORREF
YUVToRGB(int y, int u, int v)
{
    /* Calculate C, D, and E values for the optimized macro. */
    y -= 16; u -= 128; v -= 128;
    return RGB(YUV2RO(y, u, v) & 0xff, YUV2GO(y, u, v) & 0xff, YUV2BO(y, u, v) & 0xff);
}



#endif // HW_EMULATOR_CAMERA_CONVERTERS_EDITTED_H
