#include "CLightpack.h"

#define SETTINGS_FILE "settings.ini"
#define PROJECT_NAME "lightpack-filter"

DWORD WINAPI CLightpack::IOThread(LPVOID lpvThreadParm)
{
    CLightpack* pLightpack = (CLightpack*)lpvThreadParm;
    return pLightpack->loadSettingsThreadStart();
}

void CLightpack::startLoadSettingsThread()
{
    if (mLoadSettingsThreadId != NULL && mhLoadSettingsThread != INVALID_HANDLE_VALUE) {
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
            mHasReadSettings = true;

            // Prepare the path to the settings file
            char path[MAX_PATH];
            wcstombs(path, getCurrentDirectory(), wcslen(getCurrentDirectory()));
            path[wcslen(getCurrentDirectory())] = '\0';
            strcat(path, "\\" SETTINGS_FILE);

            INIReader reader(path);
            if (!reader.ParseError()) {
                readSettingsFile(reader);
            }
            else {
                // If there is no settings file there then we should try appdata file, otherwise just use defaults
                wchar_t buffer[MAX_PATH];
                if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, buffer))) {
                    wcstombs(path, buffer, wcslen(buffer));
                    path[wcslen(buffer)] = '\0';
                    strcat(path, "\\" PROJECT_NAME "\\" SETTINGS_FILE);

                    // Try again
                    reader = INIReader(path);
                    if (!reader.ParseError()) {
                        readSettingsFile(reader);
                    }
                }
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
