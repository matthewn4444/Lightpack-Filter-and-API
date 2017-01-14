#include "CLightpack.h"

#define SETTINGS_FILE "settings.ini"

DWORD WINAPI CLightpack::IOThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->loadSettingsThreadStart();
}

void CLightpack::startLoadSettingsThread()
{
    if (mHasReadSettings || mLoadSettingsThreadId != NULL && mhLoadSettingsThread != INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mLoadSettingsThreadId == NULL);
    ASSERT(mhLoadSettingsThread == INVALID_HANDLE_VALUE);
    mhLoadSettingsThread = CreateThread(NULL, 0, IOThread, (void*) this, 0, &mLoadSettingsThreadId);

    ASSERT(mhLoadSettingsThread);
    if (mhLoadSettingsThread == NULL) {
        _log("Failed to create thread");
    }
}

void CLightpack::destroyLoadSettingsThread()
{
    if (mLoadSettingsThreadId == NULL && mhLoadSettingsThread == INVALID_HANDLE_VALUE) {
        return;
    }

    CAutoLock lock(m_pLock);
    ASSERT(mLoadSettingsThreadId != NULL);
    ASSERT(mhLoadSettingsThread != INVALID_HANDLE_VALUE);

    WaitForSingleObject(mhLoadSettingsThread, INFINITE);
    CloseHandle(mhLoadSettingsThread);

    // Reset Values
    mLoadSettingsThreadId = 0;
    mhLoadSettingsThread = INVALID_HANDLE_VALUE;
}

DWORD CLightpack::loadSettingsThreadStart()
{
    CCritSec settingsLock;
    {
        // Read the settings
        CAutoLock cObjectLock(&settingsLock);
        if (!mHasReadSettings) {
            // Prepare the path to the settings file
            char* path;
            if (strlen(mSettingsPathFromGUI) > 0) {
                path = mSettingsPathFromGUI;
            }
            else {
                char temp_path[MAX_PATH];
                wcstombs(temp_path, getCurrentDirectory(), wcslen(getCurrentDirectory()));
                temp_path[wcslen(getCurrentDirectory())] = '\0';
                path = temp_path;
            }
            strcat(path, "\\" SETTINGS_FILE);

            INIReader reader(path);
            if (!reader.ParseError()) {
                mHasReadSettings = true;
                readSettingsFile(reader);
            }
            else {
                _logf("Unable to open file in path: '%s'", path);
            }
        }

        // Close this thread
        ASSERT(mLoadSettingsThreadId != NULL);
        ASSERT(mhLoadSettingsThread != INVALID_HANDLE_VALUE);
        HANDLE temp = mhLoadSettingsThread;
        mLoadSettingsThreadId = 0;
        mhLoadSettingsThread = INVALID_HANDLE_VALUE;
        CloseHandle(temp);
    }
    return 0;
}

void CLightpack::readSettingsFile(INIReader& reader)
{
    mPropPort = reader.GetInteger("General", "port", DEFAULT_GUI_PORT);
    mPropSmooth = (unsigned char)reader.GetInteger("State", "smooth", DEFAULT_SMOOTH);
    mPropBrightness = (unsigned char)reader.GetInteger("State", "brightness", Lightpack::DefaultBrightness);
    mPropGamma = reader.GetReal("State", "gamma", Lightpack::DefaultGamma);
    mPropOnWhenClose = reader.GetInteger("State", "onWhenClose", DEFAULT_ON_WHEN_CLOSE ? 1 : 0) == 1;

    // Read positions
    int i = 1;
    int x = 0, y = 0, w = 0, h = 0;
    char key[32] = "led1";
    std::vector<Lightpack::Rect> rects;
    Lightpack::Rect rect = {0, 0, 16, 16};
    bool somethingFailed = false;
    std::string line;
    while (!(line = reader.Get("Positions", key, "")).empty()) {
        if (!parseLedRectLine(line.c_str(), &rect)) {
            somethingFailed = true;
        }
        rects.push_back(rect);
        _logf("Got led %d: %d, %d, %d, %d", i, rect.x, rect.y, rect.width, rect.height);
        sprintf(key, "led%d", ++i);
    }
    if (somethingFailed) {
        MessageBox(NULL, L"Settings file is corrupted so lights will be read from the default location.\nUse the application to reconfigure the leds' locations to fix this.\nIf you receive this message even after modifying led positions, email me for more help.", L"Reading Setting Error", MB_ICONWARNING);
    }
    updateScaledRects(rects);
}

bool CLightpack::parseLedRectLine(const char* line, Lightpack::Rect* outRect)
{
    if (mWidth != 0 && mHeight != 0) {
        if (strlen(line) > 0) {
            double x = -1, y = -1, w = -1, h = -1;
            if (sscanf(line, "%*c%*lf:%lf,%lf,%lf,%lf", &x, &y, &w, &h) != EOF) {
                // Make sure that the values are acquired correctly
                if (x >= 0 && y >= 0 && w >= 0 && h > 0) {
                    percentageRectToVideoRect(x, y, w, h, outRect);
                    return true;
                }
            }
        }
    }
    return false;
}

void CLightpack::percentageRectToVideoRect(double x, double y, double w, double h, Lightpack::Rect* outRect)
{
    outRect->x = (int)(min(x / 100.0, 1) * mWidth);
    outRect->y = (int)(min(y / 100.0, 1) * mHeight);
    outRect->width = (int)(min(w / 100.0, 1) * mWidth);
    outRect->height = (int)(min(h / 100.0, 1) * mHeight);
}
