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

// PIMPL Implementation to avoid dlib compile dependency
class Lightpack::socket_impl {
private:
    dlib::iosockstream mSocket;
public:
    dlib::iosockstream& socket() {
        return mSocket;
    }
};

Lightpack::Lightpack(const std::string& host, unsigned int port, const std::vector<int>& ledMap, const std::string& apikey) 
: mHost(host)
, mPort(port)
, mLedMap(ledMap)
, mApiKey(apikey)
, pimpl(new socket_impl())
{
}

Lightpack::~Lightpack() {
    disconnect();
}

std::string Lightpack::readResultLine() {
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

std::string Lightpack::readResultValue() {
    std::string result = readResultLine();
    int pos = result.find(":");
    if (pos == -1) {
        throw std::runtime_error("Failed to parse the value from the result.");
    }
    return result.substr(pos + 1);
}

Lightpack::RESULT Lightpack::getResult() {
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

std::vector<std::string> Lightpack::getProfiles() {
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

std::string Lightpack::getProfile() {
    pimpl->socket() << CMD_GET_PROFILE;
    return readResultValue();
}

Lightpack::STATUS Lightpack::getStatus() {
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

int Lightpack::getCountLeds() {
    pimpl->socket() << CMD_GET_COUNT_LEDS;
    return atoi(readResultValue().c_str());
}

Lightpack::RESULT Lightpack::getAPIStatus() {
    pimpl->socket() << CMD_GET_STATUS_API;
    return readResultValue() == "busy" ? BUSY : IDLE;
}

Lightpack::RESULT Lightpack::connect() {
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

Lightpack::RESULT Lightpack::setColor(int n, int r, int g, int b) {
    sprintf(mCmdCache, CMD_SET_COLOR, n, r, g, b);
    pimpl->socket() << mCmdCache;
    return getResult();
}

Lightpack::RESULT Lightpack::setColorToAll(int r, int g, int b) {
    std::stringstream ss;
    for (size_t i = 0; i < mLedMap.size(); i++) {
        ss << mLedMap[i] << "-" << r << "," << g << "," << b << ";";
    }
    std::string& tmp = ss.str();
    sprintf(mCmdCache, CMD_SET_COLOR_TO_ALL, tmp.c_str());
    pimpl->socket() << mCmdCache;
    return getResult();
}

Lightpack::RESULT Lightpack::setGamma(float g) {
    sprintf(mCmdCache, CMD_SET_GAMMA, g);
    pimpl->socket() << mCmdCache;
    return getResult();
}

Lightpack::RESULT Lightpack::setSmooth(int s) {
    sprintf(mCmdCache, CMD_SET_SMOOTH, s);
    pimpl->socket() << mCmdCache;
    return getResult();
}

Lightpack::RESULT Lightpack::setBrightness(int s) {
    sprintf(mCmdCache, CMD_SET_BRIGHTNESS, s);
    pimpl->socket() << mCmdCache;
    return getResult();
}

Lightpack::RESULT Lightpack::setProfile(std::string profile) {
    sprintf(mCmdCache, CMD_SET_PROFILE, profile.c_str());
    pimpl->socket() << mCmdCache;
    return getResult();
}

Lightpack::RESULT Lightpack::lock() {
    pimpl->socket() << CMD_LOCK;
    return readResultValue() == "success" ? OK : BUSY;
}

Lightpack::RESULT Lightpack::unlock() {
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

Lightpack::RESULT Lightpack::turnOn() {
    pimpl->socket() << CMD_SET_STATUS_ON;
    return getResult();
}

Lightpack::RESULT Lightpack::turnOff() {
    pimpl->socket() << CMD_SET_STATUS_OFF;
    return getResult();
}

void Lightpack::disconnect() {
    unlock();
    pimpl->socket().close();
}
