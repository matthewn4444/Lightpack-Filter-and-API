#include "../include/Lightpack.h"
#include <dlib/iosockstream.h>

// Commands
#define CMD_GET_PROFILE         "getprofile\n"
#define CMD_GET_PROFILES        "getprofiles\n"
#define CMD_GET_STATUS          "getstatus\n"
#define CMD_GET_COUNT_LEDS      "getcountleds\n"
#define CMD_GET_STATUS_API      "getstatusapi\n"
#define CMD_API_KEY             "apikey:%s\n"
#define CMD_SET_COLOR           "setcolor:%d-%d,%d,%d\n"
#define CMD_SET_COLOR_TO_ALL    "setcolor:%s\n"
#define CMD_SET_GAMMA           "setgamma:%.3g\n"
#define CMD_SET_SMOOTH          "setsmooth:%d\n"
#define CMD_SET_BRIGHTNESS      "setbrightness:%d\n"
#define CMD_SET_PROFILE         "setprofile:%s\n"
#define CMD_LOCK                "lock\n"
#define CMD_UNLOCK              "unlock\n"
#define CMD_SET_STATUS_ON       "setstatus:on\n"
#define CMD_SET_STATUS_OFF      "setstatus:off\n"

namespace Lightpack {

    // PIMPL Implementation to avoid dlib compile dependency
    class PrismatikClient::socket_impl {
    private:
        dlib::iosockstream mSocket;
    public:
        dlib::iosockstream& socket() {
            return mSocket;
        }
    };

    PrismatikClient::PrismatikClient(const std::string& host, unsigned int port, const std::vector<int>& ledMap, const std::string& apikey)
        : mHost(host)
        , mPort(port)
        , mLedMap(ledMap)
        , mApiKey(apikey)
        , pimpl(new socket_impl())
    {
    }

    PrismatikClient::~PrismatikClient() {
        disconnect();
    }

    std::string PrismatikClient::readResultLine() {
        std::stringstream ss;
        while (pimpl->socket().peek() != EOF)
        {
            char c = pimpl->socket().get();
            // Windows line endings
            if (c == '\r') {
                pimpl->socket().get();      // Next character is \n
                break;
            }
            // Linux/Mac line ending
            else if (c == '\n') {
                break;
            }
            ss.put(c);
        }
        std::string& str = ss.str();
        return str;
    }

    std::string PrismatikClient::readResultValue() {
        std::string result = readResultLine();
        int pos = result.find(":");
        if (pos == -1) {
            throw std::runtime_error("Failed to parse the value from the result.");
        }
        return result.substr(pos + 1);
    }

    RESULT PrismatikClient::getResult() {
        std::string result = readResultLine();
        if (result == "ok") {
            return OK;
        }
        else if (result == "error") {
            return FAIL;
        }
        else if (result == "busy") {
            return BUSY;
        }
        return NOT_LOCKED;
    }

    std::vector<std::string> PrismatikClient::getProfiles() {
        pimpl->socket() << CMD_GET_PROFILES;
        std::istringstream iss(readResultValue());
        std::string segment;
        std::vector<std::string> profiles;
        while (std::getline(iss, segment, ';'))
        {
            if (segment.size() > 1 || segment.size() == 1 && (segment[0] != '\r' && segment[0] != '\n')) {
                profiles.push_back(segment);
            }
        }
        return profiles;
    }

    std::string PrismatikClient::getProfile() {
        pimpl->socket() << CMD_GET_PROFILE;
        return readResultValue();
    }

    PrismatikClient::STATUS PrismatikClient::getStatus() {
        pimpl->socket() << CMD_GET_STATUS;
        std::string result = readResultValue();
        if (result == "on") {
            return ON;
        }
        else if (result == "off") {
            return OFF;
        }
        else if (result == "device_error") {
            return DEVICE_ERROR;
        }
        return UNKNOWN;
    }

    int PrismatikClient::getCountLeds() {
        pimpl->socket() << CMD_GET_COUNT_LEDS;
        return atoi(readResultValue().c_str());
    }

    RESULT PrismatikClient::getAPIStatus() {
        pimpl->socket() << CMD_GET_STATUS_API;
        return readResultValue() == "busy" ? BUSY : IDLE;
    }

    RESULT PrismatikClient::connect() {
        if (mPort <= 0 || mHost.empty()) {
            return FAIL;
        }
        try {
            pimpl->socket().open(mHost + ":" + std::to_string(mPort));
            readResultLine();
            if (!mApiKey.empty()) {
                sprintf(mCmdCache, CMD_API_KEY, mApiKey.c_str());
                pimpl->socket() << mCmdCache;
                readResultLine();
            }
        }
        catch (std::exception&) {
            printf("Lightpack API server is missing!\n");
            return FAIL;
        }
        return OK;
    }

    RESULT PrismatikClient::setColor(int n, int r, int g, int b) {
        sprintf(mCmdCache, CMD_SET_COLOR, n, r, g, b);
        printf("%s\n", mCmdCache);
        pimpl->socket() << mCmdCache;
        return getResult();
    }

    RESULT PrismatikClient::setColorToAll(int r, int g, int b) {
        std::stringstream ss;
        for (size_t i = 0; i < mLedMap.size(); i++) {
            ss << mLedMap[i] << "-" << r << "," << g << "," << b << ";";
        }
        std::string& tmp = ss.str();
        sprintf(mCmdCache, CMD_SET_COLOR_TO_ALL, tmp.c_str());
        pimpl->socket() << mCmdCache;
        return getResult();
    }

    RESULT PrismatikClient::setGamma(double g) {
        sprintf(mCmdCache, CMD_SET_GAMMA, g);
        pimpl->socket() << mCmdCache;
        return getResult();
    }

    RESULT PrismatikClient::setSmooth(int s) {
        sprintf(mCmdCache, CMD_SET_SMOOTH, s);
        pimpl->socket() << mCmdCache;
        return getResult();
    }

    RESULT PrismatikClient::setBrightness(int s) {
        sprintf(mCmdCache, CMD_SET_BRIGHTNESS, s);
        pimpl->socket() << mCmdCache;
        return getResult();
    }

    RESULT PrismatikClient::setProfile(std::string profile) {
        sprintf(mCmdCache, CMD_SET_PROFILE, profile.c_str());
        pimpl->socket() << mCmdCache;
        return getResult();
    }

    RESULT PrismatikClient::lock() {
        pimpl->socket() << CMD_LOCK;
        return readResultValue() == "success" ? OK : BUSY;
    }

    RESULT PrismatikClient::unlock() {
        pimpl->socket() << CMD_UNLOCK;
        std::string value;
        try {
            value = readResultValue();
        }
        catch (std::exception&) {
            return OK;
        }
        return value == "success" ? OK : NOT_LOCKED;
    }

    RESULT PrismatikClient::turnOn() {
        pimpl->socket() << CMD_SET_STATUS_ON;
        return getResult();
    }

    RESULT PrismatikClient::turnOff() {
        pimpl->socket() << CMD_SET_STATUS_OFF;
        return getResult();
    }

    void PrismatikClient::disconnect() {
        unlock();
        pimpl->socket().close();
    }
};
