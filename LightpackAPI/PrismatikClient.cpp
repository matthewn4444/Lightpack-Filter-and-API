#include "../include/Lightpack.h"
#include "inih\cpp\INIReader.h"
#include <dlib/iosockstream.h>

#ifdef _WIN32
#include <Shlobj.h>
#endif

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

// Prismatik preference keys
#define PRISM_KEY_ENABLED   "IsEnabled"
#define PRISM_KEY_POSITION  "Position"
#define PRISM_KEY_SIZE      "Size"

#define MAXPATHLENGTH 1024

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

    char PrismatikClient::sCacheHomeDirectory[1024] = {};

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

    const char* PrismatikClient::getHomeDirectory() {
        if (strlen(sCacheHomeDirectory) > 0) {
            return sCacheHomeDirectory;
        }
#ifdef _WIN32
        wchar_t path_w[MAXPATHLENGTH];
        if (SHGetFolderPathW(NULL, CSIDL_PROFILE, NULL, 0, path_w) >= 0) {
            wcstombs(sCacheHomeDirectory, path_w, MAXPATHLENGTH);
            return sCacheHomeDirectory;
        }
#else
        char* buffer = getenv("HOME");
        if (buffer != NULL) {
            strcpy(sCacheHomeDirectory, buffer);
            return sCacheHomeDirectory;
        }
#endif
        return NULL;
    }

    // Parsing the numbers in the format that Prismatik uses to describe the position or size
    //      Format:
    //              Position=@Point(1000 100)       // Parses 1000 and 100 out
    bool PrismatikClient::parseIniPairValue(const std::string& line, int& first, int& second) {
        int a = line.find_first_of('(');
        int b = line.find_first_of(' ', a);
        if (a == -1 || b == -1) return false;

        int c = line.find_first_of(')', ++b);
        if (c == -1) return false;

        char* pEnd;
        a++;
        int temp1 = (int)strtol(line.substr(a, b - a - 1).c_str(), &pEnd, 10);
        if (*pEnd) return false;

        int temp2 = (int)strtol(line.substr(b, c - b).c_str(), &pEnd, 10);
        if (*pEnd) return false;

        first = temp1;
        second = temp2;
        return true;
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

    size_t PrismatikClient::getCountLeds() {
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
        sprintf(mCmdCache, CMD_SET_COLOR, (n + 1), r, g, b);
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

    bool PrismatikClient::loadLedInformation(std::string profileName) {
        // Load the leds into memory from the default ini file
        std::string path = std::string(getHomeDirectory()) + "/Prismatik/Profiles/" + profileName + ".ini";
        INIReader reader(path.c_str());
        if (reader.ParseError()) {
            printf("Failed to read profile preferences.\n");
            return false;
        }

        // Parse the ini file to get the information about the leds
        size_t ledCount = getCountLeds();
        mLeds.clear();
        for (size_t i = 0; i < ledCount; i++) {
            std::string section = "LED_" + std::to_string(mLedMap[i]);

            bool enabled = reader.GetBoolean(section, PRISM_KEY_ENABLED, false);
            std::string positionStr = reader.Get(section, PRISM_KEY_POSITION, "");
            std::string sizeStr = reader.Get(section, PRISM_KEY_SIZE, "");

            if (enabled && !positionStr.empty() && !sizeStr.empty()) {
                int x, y, w, h;

                if (!parseIniPairValue(positionStr, x, y)
                    || !parseIniPairValue(sizeStr, w, h)) {
                    mLeds.clear();
                    printf("Failed to parse profile prefence for LED %d.\n", mLedMap[i]);
                    return false;
                }
                mLeds.push_back(Led(mLedMap[i], Rect(x, y, w, h), enabled));
            }
        }
        return true;
    }
};
