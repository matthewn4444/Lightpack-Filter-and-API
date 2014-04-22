#ifndef __LIGHTPACK__H__
#define __LIGHTPACK__H__

#include <string.h>
#include <vector>

class Lightpack {
public:
    enum RESULT { OK, FAIL, BUSY, NOT_LOCKED, IDLE };
    enum STATUS { ON, OFF, DEVICE_ERROR, UNKNOWN };
    
    Lightpack(const std::string& host, unsigned int port, const std::vector<int>& ledMap, const std::string& apikey = "");
    ~Lightpack();

    std::vector<std::string> getProfiles();
    std::string getProfile();
    STATUS getStatus();
    int getCountLeds();
    RESULT getAPIStatus();

    RESULT connect();

    RESULT setColor(int n, int r, int g, int b);
    RESULT setColorToAll(int r, int g, int b);
    RESULT setGamma(float g);
    RESULT setSmooth(int s);
    RESULT setBrightness(int s);
    RESULT setProfile(std::string profile);

    RESULT lock();
    RESULT unlock();
    RESULT turnOn();
    RESULT turnOff();
    void disconnect();

private:
    // PIMPL implemenation to avoid people to compile dlib dependency
    class socket_impl;
    std::auto_ptr<socket_impl> pimpl;

    std::string readResultLine();
    std::string readResultValue();
    RESULT getResult();

    // Private variables
    std::string mHost;
    unsigned int mPort;
    std::vector<int> mLedMap;
    std::string mApiKey;
    char mCmdCache[1024];
};

#endif // __LIGHTPACK__H__