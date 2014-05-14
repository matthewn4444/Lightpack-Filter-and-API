#include "CLightpack.h"

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

// Connection to GUI
#define DEFAULT_GUI_HOST "127.0.0.1"
#define DEFAULT_GUI_PORT "6000"

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

void CLightpack::receiveMessages(Socket& socket)
{
    char buffer[512];
    while (!mCommThreadStopRequested) {
        if (socket.Receive(buffer, 500) && strlen(buffer) > 0) {
            if (mDevice != NULL) {
                // Parse each message
                int messageType = buffer[0] - '0';
                int leds, n, r, g, b;
                bool result = EOF;
                switch (messageType) {
                    // Format: <0>
                    case COMM_MSG_COUNT_LEDS:
                        EnterCriticalSection(&mDeviceLock);
                        leds = mDevice->getCountLeds();
                        LeaveCriticalSection(&mDeviceLock);
                        sprintf(buffer, "%d", leds);
                        break;
                    // Format: <1><n>,<r>,<g>,<b>
                    case COMM_MSG_SET_COLOR:
                        result = sscanf(buffer + 1, "%d,%d,%d,%d", &n, &r, &g, &b);
                        if (result != EOF) {
                            EnterCriticalSection(&mDeviceLock);
                            result = mDevice->setColor(n, MAKE_RGB(r, g, b)) == Lightpack::RESULT::OK;
                            LeaveCriticalSection(&mDeviceLock);
                            sprintf(buffer, "%d", result);
                        }
                        break;
                    // Format: <2><r>,<g>,<b>
                    case COMM_MSG_SET_ALL_COLOR:
                        result = sscanf(buffer + 1, "%d,%d,%d", &r, &g, &b);
                        if (result != EOF) {
                            EnterCriticalSection(&mDeviceLock);
                            result = mDevice->setColorToAll(MAKE_RGB(r, g, b)) == Lightpack::RESULT::OK;
                            LeaveCriticalSection(&mDeviceLock);
                            sprintf(buffer, "%d", result);
                        }
                        break;
                    case COMM_MSG_SET_RECTS:
                        break;
                    // Format: <4><[0-100]>
                    case COMM_MSG_SET_BRIGHTNESS:
                        result = sscanf(buffer + 1, "%d", &n);
                        if (result != EOF) {
                            EnterCriticalSection(&mDeviceLock);
                            result = mDevice->setBrightness(n) == Lightpack::RESULT::OK;
                            LeaveCriticalSection(&mDeviceLock);
                            sprintf(buffer, "%d", result);
                        }
                        break;
                    // Format: <5><[0-255]>
                    case COMM_MSG_SET_SMOOTH:
                        result = sscanf(buffer + 1, "%d", &n);
                        if (result != EOF) {
                            EnterCriticalSection(&mDeviceLock);
                            result = mDevice->setSmooth(n) == Lightpack::RESULT::OK;
                            LeaveCriticalSection(&mDeviceLock);
                            sprintf(buffer, "%d", result);
                        }
                        break;
                    // Format: <6><0-100>
                    case COMM_MSG_SET_GAMMA:
                        result = sscanf(buffer + 1, "%d", &n);
                        if (result != EOF) {
                            EnterCriticalSection(&mDeviceLock);
                            result = mDevice->setGamma(n / 10.0) == Lightpack::RESULT::OK;
                            LeaveCriticalSection(&mDeviceLock);
                            sprintf(buffer, "%d", result);
                        }
                        break;
                    // Format: <7>
                    case COMM_MSG_TURN_OFF:
                        EnterCriticalSection(&mDeviceLock);
                        result = mDevice->turnOff() == Lightpack::RESULT::OK;
                        LeaveCriticalSection(&mDeviceLock);
                        sprintf(buffer, "%d", result);
                        break;
                    // Format: <8>
                    case COMM_MSG_TURN_ON:
                        EnterCriticalSection(&mDeviceLock);
                        result = mDevice->turnOn() == Lightpack::RESULT::OK;
                        LeaveCriticalSection(&mDeviceLock);
                        sprintf(buffer, "%d", result);
                        break;
                }
                // Failed to parse the message
                if (result == EOF) {
                    buffer[0] = '0';
                    buffer[1] = '\0';
                }
            }
            else {
                buffer[0] = '0';
                buffer[1] = '\0';
            }
            // Respond back to the server
            socket.Send(buffer);
        }
    }
}

DWORD CLightpack::commThreadStart()
{
    log("Running communication thread");
    // Trying to connect
    Socket socket;
    if (socket.Open(DEFAULT_GUI_HOST, DEFAULT_GUI_PORT)) {
        receiveMessages(socket);
    }
    else {
        WCHAR wDllPath[MAX_PATH] = { 0 };
        if (GetModuleFileName((HINSTANCE)&__ImageBase, wDllPath, _countof(wDllPath)) != ERROR_INSUFFICIENT_BUFFER) {
            std::wstring temp(wDllPath);
            std::wstring axPath = temp.substr(0, temp.find_last_of('\\'));

            // Run the application (if already running this does nothing), try to connect, if fail then give up
            ShellExecute(NULL, NULL, L"nw.exe", L"app.nw", axPath.c_str(), SW_SHOW);   // TODO change this when gui is complete
            if (socket.Open(DEFAULT_GUI_HOST, DEFAULT_GUI_PORT)) {
                receiveMessages(socket);
            }
            else {
                log("Failed to connect to gui");
            }
        }
        else {
            log("Failed parse this directory");
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