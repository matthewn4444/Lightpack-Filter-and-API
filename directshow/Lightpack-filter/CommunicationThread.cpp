#include "CLightpack.h"
#include <sstream>

// Connection to GUI
#define DEFAULT_GUI_HOST "127.0.0.1"

#define SOCKET_BUFFER_SIZE 1024
#define RECV_TIMEOUT 200
#define GUI_MUTEX_NAME L"LightpackFilterGUIMutex"

// These messages are uses in the communication channel to the GUI
#define COMM_REC_COUNT_LEDS     0
#define COMM_REC_SET_COLOR      1
#define COMM_REC_SET_COLORS     2
#define COMM_REC_SET_ALL_COLOR  3
#define COMM_REC_SET_POSITIONS  4
#define COMM_REC_SET_BRIGHTNESS 5
#define COMM_REC_SET_SMOOTH     6
#define COMM_REC_SET_GAMMA      7
#define COMM_REC_TURN_OFF       8
#define COMM_REC_TURN_ON        9
#define COMM_REC_CONNECT        10
#define COMM_REC_NEW_PORT       11
#define COMM_REC_IS_RUNNING     12
#define COMM_REC_CLOSE_WINDOW   13

// These messages are used to send data back to the server
#define COMM_SEND_RETURN        0
#define COMM_SEND_PLAYING       1
#define COMM_SEND_PAUSED        2
#define COMM_SEND_CONNECTED     3
#define COMM_SEND_DISCONNECTED  4
#define COMM_SEND_INVALID_ARGS  5

// Find the current window's handle
static HWND __sFoundHandle = NULL;
static BOOL CALLBACK EnumWindowsProc(HWND h, long processId) {
    if (IsWindow(h) && IsWindowVisible(h)) {
        __sFoundHandle = NULL;
        DWORD handleProcessId;
        GetWindowThreadProcessId(h, &handleProcessId);

        // Found the correct window
        if (processId == handleProcessId) {
            __sFoundHandle = h;
            return FALSE;
        }
    }
    return TRUE;
}
static HWND getWindowHandle()
{
    DWORD processId = GetCurrentProcessId();
    EnumWindows(&EnumWindowsProc, processId);
    if (__sFoundHandle != NULL) {
        HWND ret = __sFoundHandle;
        __sFoundHandle = NULL;
        return ret;
    }
    return NULL;
}

void CLightpack::startCommThread()
{
    if (mCommThreadId != NULL && mhCommThread != INVALID_HANDLE_VALUE) {
        return;
    }

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
    int n, r, g, b, result = EOF;
    switch (messageType) {
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
            } else {
                // Consume the event and do nothing
                result = 0;
                sprintf(buffer, "%d1", COMM_SEND_RETURN);
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
                    for (size_t i = 0; i < min(mDevice->getCountLeds(), colors.size()); i++) {
                        if (i < 0) {
                            continue;
                        }
                        mPropColors[i] = colors[i];
                    }
                    LeaveCriticalSection(&mDeviceLock);
                    sprintf(buffer, "%d%d", COMM_SEND_RETURN, result);
                }
            } else {
                // Consume the event and do nothing
                result = 0;
                sprintf(buffer, "%d1", COMM_SEND_RETURN);
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
            } else {
                // Consume the event and do nothing
                result = 0;
                sprintf(buffer, "%d1", COMM_SEND_RETURN);
            }
            break;
        // Format: <4>...
        case COMM_REC_SET_POSITIONS:
            {
                std::vector<Lightpack::Rect> rects;
                std::istringstream ss(buffer + 1);
                std::string part;
                Lightpack::Rect rect;
                bool success = true;

                while (std::getline(ss, part, '|')) {
                    if (parseLedRectLine(part.c_str(), &rect)) {
                        rects.push_back(rect);
                    }
                    else {
                        success = false;
                        break;
                    }
                }
                if (success) {
                    updateScaledRects(rects);
                    result = 0;
                    sprintf(buffer, "%d1", COMM_SEND_RETURN);
                }
            }
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
        // Format: <10><PORT>
        case COMM_REC_NEW_PORT:
            result = sscanf(buffer + 1, "%d", &n);
            if (result != EOF) {
                mPropPort = n;
                sprintf(buffer, "%d1", COMM_SEND_RETURN);
            }
            break;
    }
    return result != EOF;
}

void CLightpack::handleMessages(Socket& socket)
{
    log("Connected to gui")
    char buffer[SOCKET_BUFFER_SIZE] = { 0 };
    unsigned int currentPort = socket.getPort();
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
                        // False because the filter will autoconnect and we dont need extra events
                        mShouldSendConnectEvent = mShouldSendDisconnectEvent = false;
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
            if (!mIsRunning) {
                sprintf(buffer, "%d", COMM_SEND_PAUSED);
                socket.Send(buffer);
            }
        }
        else if (mShouldSendPlayEvent) {
            mShouldSendPlayEvent = false;
            LeaveCriticalSection(&mCommSendLock);
            if (mIsRunning) {
                sprintf(buffer, "%d", COMM_SEND_PLAYING);
                socket.Send(buffer);
            }
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
        buffer[0] = '\0';

        // Handle Receiving events
        if (socket.Receive(buffer, SOCKET_BUFFER_SIZE, RECV_TIMEOUT) > 0) {
            if (strlen(buffer) > 0) {
                int messageType = buffer[0] - 'a';
                bool parsingError = true;

                // Format: <12>
                if (messageType == COMM_REC_IS_RUNNING) {
                    parsingError = false;
                    sprintf(buffer, "%d%d", COMM_SEND_RETURN, mIsRunning ? 1 : 0);
                }
                // Format: <0>
                else if (messageType == COMM_REC_COUNT_LEDS) {
                    EnterCriticalSection(&mDeviceLock);
                    int leds = mDevice != NULL ? mDevice->getCountLeds() : 0;
                    LeaveCriticalSection(&mDeviceLock);
                    sprintf(buffer, "%d%d", COMM_SEND_RETURN, leds);
                    parsingError = false;
                }
                else if (mDevice != NULL) {
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
                // Format: <13>
                else if (messageType == COMM_REC_CLOSE_WINDOW) {
                    parsingError = false;
                    sprintf(buffer, "%d1", COMM_SEND_RETURN);

                    // Close this window
                    HWND handle = getWindowHandle();
                    if (handle != NULL) {
                        PostMessage(handle, WM_CLOSE, NULL, NULL);
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
        }
        else {
            // Received a socket error
            log("Socket left");
            socket.Close();
            break;
        }
        buffer[0] = '\0';

        // Detect port changes
        if (currentPort != mPropPort) {
            socket.Close();
            return;
        }
    }
}

DWORD CLightpack::commThreadStart()
{
    log("Running communication thread");
    Socket socket;

    // Check if GUI is already openned, if not then open it
    HANDLE h = CreateMutex(NULL, FALSE, GUI_MUTEX_NAME);
    bool guiIsRunning = GetLastError() == ERROR_ALREADY_EXISTS;
    if (!guiIsRunning) {
        CloseHandle(h);

        // Run the application
        ShellExecute(NULL, NULL, L"nw.exe", L"app.nw --hide", getCurrentDirectory(), SW_SHOW);
    }

    // Trying to connect; when port changes in handleMessages, it will reconnect port
    while (!mCommThreadStopRequested) {
        if (socket.Open(DEFAULT_GUI_HOST, mPropPort)) {
            handleMessages(socket);
        }
        else {
            log("Failed to connect to gui");
            Sleep(500);
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
