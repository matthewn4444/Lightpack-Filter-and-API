#ifndef __LIGHTPACK__H__
#define __LIGHTPACK__H__

#include <string.h>
#include <vector>

struct hid_device_;
typedef struct hid_device_ hid_device;

namespace Lightpack {
    enum RESULT { OK, FAIL, BUSY, NOT_LOCKED, IDLE };

    // Basic rect structure
    struct Rect {
    public:
        Rect(){}
        Rect(int _x, int _y, int _width, int _height)
            : x(_x), y(_y), width(_width), height(_height) {}
        int x, y, width, height;
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

    // Base class for Prismatik and LedDevice
    class LedBase {
    public:
        virtual size_t getCountLeds() = 0;

        virtual RESULT setColor(int n, int red, int green, int blue) = 0;
        virtual RESULT setColorToAll(int red, int green, int blue) = 0;
        virtual RESULT setGamma(double value) = 0;
        virtual RESULT setBrightness(int value) = 0;
        virtual RESULT setSmooth(int value) = 0;

        virtual RESULT turnOn() = 0;
        virtual RESULT turnOff() = 0;
    };

    class PrismatikClient : public LedBase {
    public:
        enum STATUS { ON, OFF, DEVICE_ERROR, UNKNOWN };

        PrismatikClient(const std::string& host, unsigned int port, const std::vector<int>& ledMap, const std::string& apikey = "");
        ~PrismatikClient();

        std::vector<std::string> getProfiles();
        std::string getProfile();
        STATUS getStatus();
        size_t getCountLeds();
        RESULT getAPIStatus();

        RESULT connect();

        RESULT setColor(int n, int r, int g, int b);
        RESULT setColorToAll(int r, int g, int b);
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
        std::auto_ptr<socket_impl> pimpl;

        std::string readResultLine();
        std::string readResultValue();
        RESULT getResult();

        static const char* getHomeDirectory();
        static bool parseIniPairValue(const std::string& line, int& first, int& second);

        // Private static variable
        static char sCacheHomeDirectory[1024];

        // Private variables
        std::string mHost;
        unsigned int mPort;
        std::vector<int> mLedMap;
        std::string mApiKey;
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

        RESULT setColor(int led, int red, int green, int blue);
        RESULT setColorToAll(int red, int green, int blue);

        RESULT setSmooth(int value);
        RESULT setGamma(double value);
        RESULT setBrightness(int value);
        bool setColorDepth(int value);
        bool setRefreshDelay(int value);

        inline size_t getCountLeds() {
            return mCurrentColors.size();
        }

        inline void pauseUpdating() {
            mEnableUpdating = false;
        }

        inline void resumeUpdating() {
            mEnableUpdating = true;
            updateLeds();
        }

        RESULT turnOff();
        RESULT turnOn();

        static const int LedsPerDevice;         // 10
        static const int DefaultBrightness;     // 100
        static const double DefaultGamma;        // 2.2

    private:
        typedef unsigned int RGBCOLOR;

        bool openDevice(unsigned short vid, unsigned short pid);

        bool writeBufferToDeviceWithCheck(int command, hid_device *phid_device);
        bool writeBufferToDevice(int command, hid_device *phid_device);

        bool readDataFromDeviceWithCheck();
        bool readDataFromDevice();

        bool setColorToAll(RGBCOLOR color);

        bool updateLeds();
        void allocateColors();
        void colorAdjustments(int& red12bit, int& green12bit, int& blue12bit);

        // Private variables
        std::vector<hid_device*> mDevices;
        std::vector<RGBCOLOR> mCurrentColors;

        unsigned char mReadBuffer[96];
        unsigned char mWriteBuffer[96];

        double mGamma;
        int mBrightness;
        bool mLedsOn;
        bool mEnableUpdating;
    };
};

#endif // __LIGHTPACK__H__