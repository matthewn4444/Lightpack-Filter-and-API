#include "CLightpack.h"
#include <sstream>

// Connection to GUI
#define DEFAULT_GUI_HOST "127.0.0.1"

#define RECV_TIMEOUT 200

// These messages are uses in the communication channel to the GUI
#define COMM_REC_COUNT_LEDS     0
#define COMM_REC_SET_COLOR      1
#define COMM_REC_SET_COLORS     2
#define COMM_REC_SET_ALL_COLOR  3
#define COMM_REC_SET_RECTS      4
#define COMM_REC_SET_BRIGHTNESS 5
#define COMM_REC_SET_SMOOTH     6
#define COMM_REC_SET_GAMMA      7
#define COMM_REC_TURN_OFF       8
#define COMM_REC_TURN_ON        9
#define COMM_REC_CONNECT        10

// These messages are used to send data back to the server
#define COMM_SEND_RETURN        0
#define COMM_SEND_PLAYING       1
#define COMM_SEND_PAUSED        2
#define COMM_SEND_CONNECTED     3
#define COMM_SEND_DISCONNECTED  4
#define COMM_SEND_INVALID_ARGS  5

void CLightpack::startCommThread()
{
    if (mCommThreadId != NULL && mhCommThread != INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mCommThreadId == NULL);
    ASSERT(mhCommThread == INVALID_HANDLE_VALUE);

    mhCommThread = CreateThread(NULL, 0, CommunicationThread, (void*) this, 0, &mCommThreadId);

    ASSERT(mhCommThread);
    if (mhCommThread == NULL) {
        log("Failed to create communication thread");
    }
}

void CLightpack::destroyCommThread()
{
    if (mCommThreadId == NULL && mhCommThread == INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mCommThreadId != NULL);
    ASSERT(mhCommThread != INVALID_HANDLE_VALUE);

    mCommThreadStopRequested = true;
    WaitForSingleObject(mhCommThread, INFINITE);
    CloseHandle(mhCommThread);

    // Reset Values
    mCommThreadId = 0;
    mhCommThread = INVALID_HANDLE_VALUE;
    mCommThreadStopRequested = false;
}

bool CLightpack::parseReceivedMessages(int messageType, char* buffer, bool* deviceConnected) {
    // Parse each message
    int leds, n, r, g, b, result = EOF;
    switch (messageType) {
        // Format: <0>
        case COMM_REC_COUNT_LEDS:
            result = 0;
            EnterCriticalSection(&mDeviceLock);
            leds = mDevice->getCountLeds();
            LeaveCriticalSection(&mDeviceLock);
            sprintf(buffer, "%d%d", COMM_SEND_RETURN, leds);
            break;
        // Format: <1><n>,<r>,<g>,<b>
        case COMM_REC_SET_COLOR:
            if (!mIsRunning) {
                result = sscanf(buffer + 1, "%d,%d,%d,%d", &n, &r, &g, &b);
                if (result != EOF) {
                    EnterCriticalSection(&mDeviceLock);
                    *deviceConnected = result = mDevice->setColor(n, MAKE_RGB(r, g, b)) == Lightpack::RESULT::OK;
                    mPropColors[n] = MAKE_RGB(r, g, b);
                    LeaveCriticalSection(&mDeviceLock);
                    sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
                }
            }
            break;
        // Format: <2><n>,<r>,<g>,<b>
        case COMM_REC_SET_COLORS:
        // Read the length of number of leds
            if (!mIsRunning) {
                std::vector<Lightpack::RGBCOLOR> colors;
                std::stringstream ss(buffer + 1);
                while (ss >> n) {
                    colors.push_back(n);
                    if (ss.peek() == ',') {
                        ss.ignore();
                    }
                }
                if (!colors.empty()) {
                    EnterCriticalSection(&mDeviceLock);
                    *deviceConnected = result = mDevice->setColors(colors) == Lightpack::RESULT::OK;
                    for (size_t i = 0; i < mDevice->getCountLeds(); i++) {
                        if (i < 0) {
                            continue;
                        }
                        mPropColors[i] = colors[i];
                    }
                    LeaveCriticalSection(&mDeviceLock);
                    sprintf(buffer, "%d1", COMM_SEND_RETURN, result);
                }
            }
            break;
        // Format: <3><r>,<g>,<b>
        case COMM_REC_SET_ALL_COLOR:
            if (!mIsRunning) {
                result = sscanf(buffer + 1, "%d,%d,%d", &r, &g, &b);
                if (result != EOF) {
                    EnterCriticalSection(&mDeviceLock);
                    *deviceConnected = result = mDevice->setColorToAll(MAKE_RGB(r, g, b)) == Lightpack::RESULT::OK;
                    mPropColors.assign(mDevice->getCountLeds(), MAKE_RGB(r, g, b));
                    LeaveCriticalSection(&mDeviceLock);
                    sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
                }
            }
            break;
        case COMM_REC_SET_RECTS:
            break;
        // Format: <5><[0-100]>
        case COMM_REC_SET_BRIGHTNESS:
            result = sscanf(buffer + 1, "%d", &n);
            if (result != EOF) {
                EnterCriticalSection(&mDeviceLock);
                *deviceConnected = result = mDevice->setBrightness(n) == Lightpack::RESULT::OK;
                mPropBrightness = n;
                LeaveCriticalSection(&mDeviceLock);
                sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
            }
            break;
        // Format: <6><[0-255]>
        case COMM_REC_SET_SMOOTH:
            result = sscanf(buffer + 1, "%d", &n);
            if (result != EOF) {
                EnterCriticalSection(&mDeviceLock);
                *deviceConnected = result = mDevice->setSmooth(n) == Lightpack::RESULT::OK;
                mPropSmooth = n;
                LeaveCriticalSection(&mDeviceLock);
                sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
            }
            break;
        // Format: <7><0-100>
        case COMM_REC_SET_GAMMA:
            result = sscanf(buffer + 1, "%d", &n);
            if (result != EOF) {
                EnterCriticalSection(&mDeviceLock);
                *deviceConnected = result = mDevice->setGamma(n / 10.0) == Lightpack::RESULT::OK;
                mPropGamma = n / 10.0;
                LeaveCriticalSection(&mDeviceLock);
                sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
            }
            break;
        // Format: <8>
        case COMM_REC_TURN_OFF:
            EnterCriticalSection(&mDeviceLock);
            *deviceConnected = result = mDevice->turnOff() == Lightpack::RESULT::OK;
            LeaveCriticalSection(&mDeviceLock);
            sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
            break;
        // Format: <9>
        case COMM_REC_TURN_ON:
            EnterCriticalSection(&mDeviceLock);
            *deviceConnected = result = mDevice->turnOn() == Lightpack::RESULT::OK;
            LeaveCriticalSection(&mDeviceLock);
            sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
            break;
        case COMM_REC_CONNECT:
            // Don't do anything because we try to connect when already connected!
            result = 0;
            sprintf(buffer, "%d1", COMM_SEND_RETURN);
            break;
        }
    return result != EOF;
}

void CLightpack::handleMessages(Socket& socket)
{
    char buffer[512];
    mShouldSendPlayEvent = false;
    mShouldSendPauseEvent = false;
    mShouldSendConnectEvent = false;
    mShouldSendDisconnectEvent = false;
    while (!mCommThreadStopRequested) {
        // Ping the device every 1 seconds and resolve failed connection
        if (!mIsRunning) {
            DWORD now = GetTickCount();
            if ((now - mLastDeviceCheck) > sDeviceCheckElapseTime) {
                EnterCriticalSection(&mDeviceLock);
                bool justDisconnected = false;
                if (mDevice != NULL) {
                    // To ping, set the brightness, if fail then we should try to reconnect device
                    if (mDevice->setBrightness(mPropBrightness) != Lightpack::RESULT::OK) {
                        justDisconnected = true;
                        disconnectAllDevices();     // Disconnect devices because it has failed
                    }
                }

                // Reconnect device if fails
                if (mDevice == NULL) {
                    if (connectDevice()) {
                        if (!justDisconnected) {
                            mShouldSendConnectEvent = true;
                            mShouldSendDisconnectEvent = false;
                        }
                        else {
                            mShouldSendConnectEvent = mShouldSendDisconnectEvent = false;
                        }
                    }
                    else if (justDisconnected) {
                        mShouldSendConnectEvent = false;
                        mShouldSendDisconnectEvent = true;
                    }
                }
                LeaveCriticalSection(&mDeviceLock);
                mLastDeviceCheck = now;
            }
        }

        // Handle send events
        EnterCriticalSection(&mCommSendLock);
        if (mShouldSendPauseEvent) {
            mShouldSendPauseEvent = false;
            LeaveCriticalSection(&mCommSendLock);
            sprintf(buffer, "%d", COMM_SEND_PAUSED);
            socket.Send(buffer);
        }
        else if (mShouldSendPlayEvent) {
            mShouldSendPlayEvent = false;
            LeaveCriticalSection(&mCommSendLock);
            sprintf(buffer, "%d", COMM_SEND_PLAYING);
            socket.Send(buffer);
        }
        else if (mShouldSendConnectEvent) {
            mShouldSendConnectEvent = false;
            LeaveCriticalSection(&mCommSendLock);
            sprintf(buffer, "%d", COMM_SEND_CONNECTED);
            socket.Send(buffer);
        }
        else if (mShouldSendDisconnectEvent) {
            mShouldSendDisconnectEvent = false;
            LeaveCriticalSection(&mCommSendLock);
            sprintf(buffer, "%d", COMM_SEND_DISCONNECTED);
            socket.Send(buffer);
        }
        else {
            mShouldSendPlayEvent = false;
            mShouldSendPauseEvent = false;
            mShouldSendConnectEvent = false;
            mShouldSendDisconnectEvent = false;
            LeaveCriticalSection(&mCommSendLock);
        }

        // Handle Receiving events
        if (socket.Receive(buffer, RECV_TIMEOUT) > 0 && strlen(buffer) > 0) {
            int messageType = buffer[0] - 'a';
            bool parsingError = true;
            if (mDevice != NULL) {
                // Parse the message: you can never get a parsing error and a disconnect, they are mutually exclusive!
                bool isDeviceConnected = true;
                parsingError = !parseReceivedMessages(messageType, buffer, &isDeviceConnected);
                if (!isDeviceConnected) {
                    // Disconnect the device because it is not physically connected
                    bool wasConnectedToPrismatik = mIsConnectedToPrismatik;
                    disconnectAllDevices();
                    mShouldSendDisconnectEvent = false;

                    if (wasConnectedToPrismatik) {
                        // Prismatik is not available and disconnected, try to connect to the device and try parsing again
                        if (connectDevice()) {
                            mShouldSendConnectEvent = false;
                            parseReceivedMessages(messageType, buffer, &isDeviceConnected);
                        }
                    }
                    if (!isDeviceConnected) {
                        // Device is not connected anymore, tell gui that this has disconnected from device
                        sprintf(buffer, "%d", COMM_SEND_DISCONNECTED);
                    }
                }
            }
            // Format: <10>
            else if (messageType == COMM_REC_CONNECT) {
                // Reconnect to all devices if not connected already
                parsingError = false;
                if (mDevice == NULL) {
                    // Try directly again
                    if (!connectDevice()) {
                        sprintf(buffer, "%d0", COMM_SEND_RETURN);
                    }
                    else {
                        sprintf(buffer, "%d1", COMM_SEND_RETURN);
                        mShouldSendConnectEvent = false;
                    }
                }
            }

            if (messageType >= 0) {
                // Failed to parse the message
                if (parsingError) {
                    sprintf(buffer, "%d", COMM_SEND_INVALID_ARGS);
                }

                // Respond back to the server
                socket.Send(buffer);
            }
        }
        else {
            // Received a socket error
            log("Socket left");
            socket.Close();
            break;
        }
    }
}

DWORD CLightpack::commThreadStart()
{
    log("Running communication thread");
    Socket socket;

    // Trying to connect; when port changes in handleMessages, it will reconnect port
    if (socket.Open(DEFAULT_GUI_HOST, mPropPort)) {
        handleMessages(socket);
    }
    else {
        // Run the application (if already running this does nothing), try to connect, if fail then give up
        ShellExecute(NULL, NULL, L"nw.exe", L"app.nw", getCurrentDirectory(), SW_SHOW);   // TODO change this when gui is complete
        if (socket.Open(DEFAULT_GUI_HOST, mPropPort)) {
            handleMessages(socket);
        }
        else {
            log("Failed to connect to gui");
        }
    }
    mCommThreadStopRequested = true;
    return 0;
}

DWORD WINAPI CLightpack::CommunicationThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->commThreadStart();
}