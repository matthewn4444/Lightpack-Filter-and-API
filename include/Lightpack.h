#ifndef __LIGHTPACK__H__
#define __LIGHTPACK__H__

#include <string>
#include <vector>
#include <memory>

#define MAKE_RGB(r,g,b) ((Lightpack::RGBCOLOR)(unsigned char)(r)|((unsigned char)(g)<<8)|((unsigned char)(b)<<16))
#define GET_RED(n)      n & 0xFF
#define GET_GREEN(n)    (n & 0xFF00) >> 8
#define GET_BLUE(n)     (n & 0xFF0000) >> 16

struct hid_device_;
typedef struct hid_device_ hid_device;

namespace Lightpack {
    enum RESULT { OK, FAIL, BUSY, NOT_LOCKED, IDLE };
    typedef int RGBCOLOR;

    // Basic rect structure
    struct Rect {
    public:
        Rect(){}
        Rect(int _x, int _y, int _width, int _height)
            : x(_x), y(_y), width(_width), height(_height) {}
        int x, y, width, height;

        inline int area() {
            return width * height;
        }
    };

    // Basic LED structure
    struct Led {
    public:
        Led(){}
        Led(int _id, Rect _rect, bool _enabled = true)
            : id(_id), rect(_rect), enabled(_enabled) {}
        int id;
        Rect rect;
        bool enabled;
    };

    static const unsigned int DefaultBrightness = 100;
    static const unsigned int DefaultSmooth     = 255;
    static const double DefaultGamma            = 2.2;

    // Base class for Prismatik and LedDevice
    class LedBase {
    public:
        virtual size_t getCountLeds() = 0;

        virtual RESULT setColor(int n, int red, int green, int blue) = 0;
        virtual RESULT setColor(int n, RGBCOLOR color) = 0;
        virtual RESULT setColors(std::vector<RGBCOLOR>& colors) = 0;
        virtual RESULT setColors(const RGBCOLOR* colors, size_t length) = 0;
        virtual RESULT setColorToAll(int red, int green, int blue) = 0;
        virtual RESULT setColorToAll(RGBCOLOR color) = 0;

        virtual RESULT setGamma(double value) = 0;
        virtual RESULT setBrightness(int value) = 0;
        virtual RESULT setSmooth(int value) = 0;

        virtual RESULT turnOn() = 0;
        virtual RESULT turnOff() = 0;
    };

    class PrismatikClient : public LedBase {
    public:
        enum STATUS { ON, OFF, DEVICE_ERROR, UNKNOWN };

        PrismatikClient();
        ~PrismatikClient();

        std::vector<std::string> getProfiles();
        std::string getProfile();
        STATUS getStatus();
        size_t getCountLeds();
        RESULT getAPIStatus();

        RESULT connect(const std::string& host, unsigned int port, const std::vector<int>& ledMap, const std::string& apikey = "");

        RESULT setColor(int n, int red, int green, int blue);
        RESULT setColor(int n, RGBCOLOR color);
        RESULT setColors(std::vector<RGBCOLOR>& colors);
        RESULT setColors(const RGBCOLOR* colors, size_t length);
        RESULT setColorToAll(int red, int green, int blue);
        RESULT setColorToAll(RGBCOLOR color);

        RESULT setGamma(double g);
        RESULT setSmooth(int s);
        RESULT setBrightness(int s);
        RESULT setProfile(std::string profile);

        RESULT lock();
        RESULT unlock();
        RESULT turnOn();
        RESULT turnOff();
        void disconnect();

        bool loadLedInformation(std::string profileName);

        inline const Led& getLedInfo(int i) {
            return mLeds[i];
        }

    private:
        // PIMPL implemenation to avoid people to compile dlib dependency
        class socket_impl;
        std::unique_ptr<socket_impl> pimpl;

        std::string readResultLine();
        std::string readResultValue();
        RESULT getResult();

        static const char* getHomeDirectory();
        static bool parseIniPairValue(const std::string& line, int& first, int& second);

        // Private static variable
        static char sCacheHomeDirectory[1024];

        // Private variables
        std::vector<int> mLedMap;
        char mCmdCache[1024];
        std::vector<Led> mLeds;
    };

    class LedDevice : public LedBase {
    public:
        LedDevice();
        ~LedDevice();

        bool open();
        bool tryToReopenDevice();
        void closeDevices();

        RESULT setColor(int n, int red, int green, int blue);
        RESULT setColor(int n, RGBCOLOR color);
        RESULT setColors(std::vector<RGBCOLOR>& colors);
        RESULT setColors(const RGBCOLOR* colors, size_t length);
        RESULT setColorToAll(int red, int green, int blue);
        RESULT setColorToAll(RGBCOLOR color);

        RESULT setSmooth(int value);
        RESULT setGamma(double value);
        RESULT setBrightness(int value);
        bool setColorDepth(int value);
        bool setRefreshDelay(int value);

        inline size_t getCountLeds() {
            return mDevices.empty() ? 0 : mCurrentColors.size();
        }

        RESULT turnOff();
        RESULT turnOn();

        static const unsigned int LedsPerDevice = 10;

    private:
        bool openDevice(unsigned short vid, unsigned short pid);

        bool writeBufferToDeviceWithCheck(int command, hid_device *phid_device);
        bool writeBufferToDevice(int command, hid_device *phid_device);

        bool readDataFromDeviceWithCheck();
        bool readDataFromDevice();

        bool updateLeds();
        void allocateColors();
        void colorAdjustments(int& red12bit, int& green12bit, int& blue12bit);

        bool isBufferAllBlack();

        // Private variables
        std::vector<hid_device*> mDevices;
        std::vector<RGBCOLOR> mCurrentColors;

        unsigned char mReadBuffer[65];
        unsigned char mWriteBuffer[65];

        double mGamma;
        int mBrightness;
        bool mLedsOn;
    };
};

#endif // __LIGHTPACK__H__