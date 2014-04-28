#ifndef __LOG_H__
#define __LOG_H__

#include <string>
#include <sstream>

using namespace std;

class Log {
public:
    Log(const char* filename) {
        if (filename == NULL) {
            throw new exception("Did not specify a filename to save log file.");
        }
        else {
            mFile = fopen(filename, "w");
        }
    }
    ~Log() {
        if (mFile) {
            flush();
            fclose(mFile);
        }
    }

    void flush() {
        fflush(mFile);
    }

    void log(const char* str) {
        fwrite(str, 1, strlen(str), mFile);
        newLine();
    }

    void log(int num) {
        string a = to_string(num);
        log(a.c_str());
    }

    void log(float num) {
        string a = to_string(num);
        log(a.c_str());
    }

    void logf(const char* format, ...) {
        va_list vl;
        char buffer[1096];
        va_start(vl, format);
        vsprintf(buffer, format, vl);
        va_end(vl);
        log(buffer);
    }

private:
    void newLine() {
        fwrite("\n", 1, 1, mFile);
    }

    FILE* mFile;
};

#endif // __LOG_H__