#include <algorithm>
#include "../include/Lightpack.h"
#include "hidapi\hidapi.h"
#include "device\commands.h"
#include "device\USB_ID.h"

#define WRITE_BUFFER_INDEX_REPORT_ID    0
#define WRITE_BUFFER_INDEX_COMMAND      1
#define WRITE_BUFFER_INDEX_DATA_START   2

namespace Lightpack {
    const int LedDevice::LedsPerDevice = 10;
    const int LedDevice::DefaultBrightness = 100;
    const double LedDevice::DefaultGamma = 2.2;

    LedDevice::LedDevice() 
        : mGamma(DefaultGamma)
        , mBrightness(DefaultBrightness)
        , mLedsOn(true)
    {
    }

    LedDevice::~LedDevice() {
        closeDevices();
    }

    bool LedDevice::open() {
        if (!mDevices.empty()) {
            return true;
        }
        openDevice(USB_VENDOR_ID, USB_PRODUCT_ID);
        openDevice(USB_OLD_VENDOR_ID, USB_OLD_PRODUCT_ID);
        allocateColors();
        return !mDevices.empty();
    }

    bool LedDevice::tryToReopenDevice() {
        closeDevices();
        open();
        return !mDevices.empty();
    }

    void LedDevice::closeDevices() {
        for (size_t i = 0; i < mDevices.size(); i++) {
            hid_close(mDevices[i]);
        }
        mDevices.clear();
    }

    RESULT LedDevice::setColor(int led, RGBCOLOR color) {
        return setColor(led, GET_RED(color), GET_GREEN(color), GET_BLUE(color));
    }

    RESULT LedDevice::setColor(int led, int red, int green, int blue) {
        if (led < 0 || led >= (int)mCurrentColors.size()) {
            return FAIL;
        }
        mCurrentColors[led] = MAKE_RGB(red, green, blue);
        if (!updateLeds()) {
            return FAIL;
        }
        return OK;
    }

    RESULT LedDevice::setColors(std::vector<RGBCOLOR>& colors) {
        if (colors.empty()) {
            return OK;
        }
        for (size_t i = 0; i < std::min(colors.size(), mDevices.size() * LedsPerDevice); i++) {
            if (colors[i] < 0) {
                continue;
            }
            mCurrentColors[i] = colors[i];
        }
        if (!updateLeds()) {
            return FAIL;
        }
        return OK;
    }

    RESULT LedDevice::setColors(const RGBCOLOR* colors, size_t length) {
        if (!length) {
            return OK;
        }
        for (size_t i = 0; i < std::min(length, getCountLeds()); i++) {
            if (colors[i] < 0) {
                continue;
            }
            mCurrentColors[i] = colors[i];
        }
        if (!updateLeds()) {
            return FAIL;
        }
        return OK;
    }

    RESULT LedDevice::setColorToAll(RGBCOLOR color) {
        mCurrentColors.assign(mCurrentColors.size(), color);
        return updateLeds() ? OK : FAIL;
    }

    RESULT LedDevice::setColorToAll(int red, int green, int blue) {
        return setColorToAll(MAKE_RGB(red, green, blue));
    }

    RESULT LedDevice::setSmooth(int value) {
        if (mDevices.empty() && !tryToReopenDevice()) {
            return FAIL;
        }
        mWriteBuffer[WRITE_BUFFER_INDEX_DATA_START] = (unsigned char)value;

        bool flag = true;
        for (size_t i = 0; i < mDevices.size(); i++) {
            if (!writeBufferToDeviceWithCheck(CMD_SET_SMOOTH_SLOWDOWN, mDevices[i])) {
                flag = false;
            }
        }
        return flag ? OK : FAIL;
    }

    bool LedDevice::setColorDepth(int value) {
        mWriteBuffer[WRITE_BUFFER_INDEX_DATA_START] = (unsigned char)value;

        bool flag = true;
        for (size_t i = 0; i < mDevices.size(); i++) {
            if (!writeBufferToDeviceWithCheck(CMD_SET_PWM_LEVEL_MAX_VALUE, mDevices[i])) {
                flag = false;
            }
        }
        return flag;
    }

    bool LedDevice::setRefreshDelay(int value) {
        mWriteBuffer[WRITE_BUFFER_INDEX_DATA_START] = value & 0xff;
        mWriteBuffer[WRITE_BUFFER_INDEX_DATA_START + 1] = (value >> 8);

        bool flag = true;
        for (size_t i = 0; i < mDevices.size(); i++) {
            if (!writeBufferToDeviceWithCheck(CMD_SET_TIMER_OPTIONS, mDevices[i])) {
                flag = false;
            }
        }
        return flag;
    }

    RESULT LedDevice::setGamma(double value) {
        mGamma = value;
        if (isBufferAllBlack()) {
            return !mDevices.empty() ? OK : FAIL;
        }
        return updateLeds() ? OK : FAIL;
    }

    RESULT LedDevice::setBrightness(int value) {
        mBrightness = value;
        if (isBufferAllBlack()) {
            return !mDevices.empty() ? OK : FAIL;
        }
        return updateLeds() ? OK : FAIL;
    }

    RESULT LedDevice::turnOff() {
        mLedsOn = false;
        return updateLeds() ? OK : FAIL;
    }

    RESULT LedDevice::turnOn() {
        mLedsOn = true;
        return updateLeds() ? OK : FAIL;
    }

    bool LedDevice::openDevice(unsigned short vid, unsigned short pid) {
        struct hid_device_info *devs, *cur_dev;
        const char *path_to_open = NULL;
        hid_device * handle = NULL;
        bool flag = true;

        devs = hid_enumerate(vid, pid);
        cur_dev = devs;
        while (cur_dev) {
            path_to_open = cur_dev->path;
            if (path_to_open) {
                /* Open the device */
                handle = hid_open_path(path_to_open);

                if (handle != NULL) {

                    // Immediately return from hid_read() if no data available
                    hid_set_nonblocking(handle, 1);
                    if (cur_dev->serial_number != NULL && wcslen(cur_dev->serial_number) > 0) {
                        char buffer[96];
                        wcstombs(buffer, cur_dev->serial_number, strlen(buffer));
                        mDevices.push_back(handle);
                    }
                    else {
                        mDevices.push_back(handle);
                    }
                }
                else {
                    flag = false;
                }
            }
            cur_dev = cur_dev->next;
        }
        hid_free_enumeration(devs);
        return flag;
    }

    bool LedDevice::writeBufferToDeviceWithCheck(int command, hid_device *phid_device) {
        if (phid_device != NULL) {
            if (!writeBufferToDevice(command, phid_device)) {
                if (!writeBufferToDevice(command, phid_device)) {
                    if (tryToReopenDevice()) {
                        if (!writeBufferToDevice(command, phid_device)) {
                            if (tryToReopenDevice()) {
                                return writeBufferToDevice(command, phid_device);
                            }
                            else {
                                return false;
                            }
                        }
                    }
                    else {
                        return false;
                    }
                }
            }
            return true;
        }
        else {
            if (tryToReopenDevice()) {
                return writeBufferToDevice(command, phid_device);
            }
            else {
                return false;
            }
        }
        return false;
    }

    bool LedDevice::writeBufferToDevice(int command, hid_device *phid_device) {
        mWriteBuffer[WRITE_BUFFER_INDEX_REPORT_ID] = 0x00;
        mWriteBuffer[WRITE_BUFFER_INDEX_COMMAND] = command;

        int error = hid_write(phid_device, mWriteBuffer, sizeof(mWriteBuffer));
        if (error < 0)
        {
            // Trying to repeat sending data:
            error = hid_write(phid_device, mWriteBuffer, sizeof(mWriteBuffer));
            if (error < 0){
                return false;
            }
        }
        return true;
    }

    bool LedDevice::readDataFromDeviceWithCheck() {
        if (!mDevices.empty()) {
            if (!readDataFromDevice()) {
                if (tryToReopenDevice()) {
                    return readDataFromDevice();
                }
                else {
                    return false;
                }
            }
            return true;
        }
        else if (tryToReopenDevice()) {
            return readDataFromDevice();
        }
        return false;
    }

    bool LedDevice::readDataFromDevice() {
        int bytes_read = hid_read(mDevices[0], mReadBuffer, sizeof(mReadBuffer));
        return bytes_read >= 0;
    }

    bool LedDevice::updateLeds() {
        if (mDevices.empty() && !tryToReopenDevice()) {
            return false;
        }

        int buffIndex = WRITE_BUFFER_INDEX_DATA_START;
        int ledCount = 10;
        bool ok = true;
        static const int kLedRemap[] = { 4, 3, 2, 0, 1, 5, 6, 7, 8, 9 };
        static const size_t kSizeOfLedColor = 6;
        bool flag = true;

        memset(mWriteBuffer, 0, sizeof(mWriteBuffer));

        for (size_t d = 0; d < mDevices.size(); d++) {
            for (size_t i = 0; i < LedsPerDevice; i++) {
                RGBCOLOR color = mLedsOn ? mCurrentColors[d * 10 + i] : 0;
                buffIndex = WRITE_BUFFER_INDEX_DATA_START + kLedRemap[i] * kSizeOfLedColor;

                int r = GET_RED(color);
                int g = GET_GREEN(color);
                int b = GET_BLUE(color);

                colorAdjustments(r, g, b);

                mWriteBuffer[buffIndex++] = (r & 0x0FF0) >> 4;
                mWriteBuffer[buffIndex++] = (g & 0x0FF0) >> 4;
                mWriteBuffer[buffIndex++] = (b & 0x0FF0) >> 4;

                // Send over 4 bits for devices revision >= 6
                // All existing devices ignore it
                mWriteBuffer[buffIndex++] = (r & 0x000F);
                mWriteBuffer[buffIndex++] = (g & 0x000F);
                mWriteBuffer[buffIndex++] = (b & 0x000F);
            }

            // Write individual set of Lightpack modules
            if (!writeBufferToDeviceWithCheck(CMD_UPDATE_LEDS, mDevices[d])) {
                flag = false;
            }

            buffIndex = WRITE_BUFFER_INDEX_DATA_START;
            memset(mWriteBuffer, 0, sizeof(mWriteBuffer));
        }
        return flag;
    }

    bool LedDevice::isBufferAllBlack() {
        // Check the current color buffer, if all 0, then dont update; could be new allocated buffer
        for (size_t i = 0; i < mCurrentColors.size(); i++) {
            if (mCurrentColors[i]) {
                return false;
            }
        }
        return true;
    }

    void LedDevice::allocateColors() {
        if (!mDevices.empty()) {
            mCurrentColors.clear();
            mCurrentColors.assign(mDevices.size() * LedsPerDevice, 0);
        }
    }

    void LedDevice::colorAdjustments(int&  red12bit, int& green12bit, int& blue12bit) {
        // Normalize to 12 bit
        static const double k = 4095 / 255.0;
        double r = red12bit * k;
        double g = green12bit * k;
        double b = blue12bit * k;

        r = 4095 * pow(r / 4095.0, mGamma);
        g = 4095 * pow(g / 4095.0, mGamma);
        b = 4095 * pow(b / 4095.0, mGamma);

        // Brightness correction
        r = (mBrightness / 100.0) * r;
        g = (mBrightness / 100.0) * g;
        b = (mBrightness / 100.0) * b;

        red12bit = (int)r;
        green12bit = (int)g;
        blue12bit = (int)b;
    }
};