#include "CLightpack.h"

DWORD WINAPI CLightpack::DeviceCheckThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->deviceCheckStart();
}

void CLightpack::startDeviceCheckThread()
{
    if (mDeviceCheckThreadId != NULL && mhDeviceCheckThread != INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mDeviceCheckThreadId == NULL);
    ASSERT(mhDeviceCheckThread == INVALID_HANDLE_VALUE);
    mhDeviceCheckThread = CreateThread(NULL, 0, DeviceCheckThread, (void*) this, 0, &mDeviceCheckThreadId);

    ASSERT(mhDeviceCheckThread);
    if (mhDeviceCheckThread == NULL) {
        _log("Failed to create device check thread");
    }
}

void CLightpack::destroyDeviceCheckThread()
{
    if (mDeviceCheckThreadId == NULL && mhDeviceCheckThread == INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mDeviceCheckThreadId != NULL);
    ASSERT(mhDeviceCheckThread != INVALID_HANDLE_VALUE);

    EnterCriticalSection(&mDeviceLock);
    mDeviceCheckThreadStopRequested = true;
    LeaveCriticalSection(&mDeviceLock);

    WaitForSingleObject(mhDeviceCheckThread, INFINITE);
    CloseHandle(mhDeviceCheckThread);

    // Reset Values
    mDeviceCheckThreadId = 0;
    mhDeviceCheckThread = INVALID_HANDLE_VALUE;
}

DWORD CLightpack::deviceCheckStart()
{
    while (!mDeviceCheckThreadStopRequested) {
        Sleep(sDeviceCheckElapseTime);
        // Reconnect device every 1 seconds
        if (!reconnectDevice() && !mIsRunning) {
            mShouldSendDisconnectEvent = true;
        }
    }
    _log("Device check thread ended");
    return 0;
}
