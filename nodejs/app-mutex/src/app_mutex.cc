#include <node_extension.h>
#include <map>
#include <string>
#include <iostream>

// ===========================================================================================
//      AppMutexes
// ===========================================================================================
typedef std::map<unsigned int, std::pair<std::string, HANDLE>> MutexEntry;
class AppMutexes {
public:
    AppMutexes()
        :mNum(1) {
    }
    ~AppMutexes() {
        // Remove all the other handlers
        for (MutexEntry::iterator iterator = mAppMutexes.begin(); iterator != mAppMutexes.end(); iterator++) {
            std::pair<std::string, HANDLE>& pair = iterator->second;
            CloseHandle(pair.second);
        }
        mAppMutexes.clear();
    }

    int create(std::string& name) {
        // See if this mutex exists already or not
        HANDLE mutex = CreateMutex(NULL, FALSE, name.c_str());
        if (mutex == NULL || GetLastError() != ERROR_SUCCESS) {
            // This mutex name already exists
            return 0;
        }
        else {
            // Made a new mutex
            int ret = mNum;
            mAppMutexes.insert(std::make_pair(mNum, std::make_pair(name, mutex)));
            mNum++;
            return ret;
        }
    }

    std::string* getName(unsigned int idNum) {
        if (mAppMutexes.find(idNum) != mAppMutexes.end()) {
            std::pair<std::string, HANDLE>& pair = mAppMutexes.at(idNum);
            return &pair.first;
        }
        return NULL;
    }

    void destroy(unsigned int idNum) {
        if (mAppMutexes.find(idNum) != mAppMutexes.end()) {
            std::pair<std::string, HANDLE>& pair = mAppMutexes.at(idNum);
            CloseHandle(pair.second);
            mAppMutexes.erase(idNum);
        }
    }

private:
    MutexEntry mAppMutexes;
    int mNum;
};

// ===========================================================================================
//      Class Wrapper
// ===========================================================================================
static AppMutexes sMutexes;

LAZY_DECLARE(create, args) {
    int retNum = 0;
    if (args.Length() > 0) {
        v8::String::Utf8Value param1(args[0]->ToString());
        std::string mutexName = std::string(*param1);

        // See if this mutex exists already or not
        retNum = sMutexes.create(mutexName);
    }
    LAZY_RETURN_NUMBER(retNum);
}

LAZY_DECLARE(destroy, args) {
    if (args.Length() > 0) {
        int idNum = ARG2INT(0);
        sMutexes.destroy(idNum);
    }
    LAZY_END();
}

LAZY_DECLARE(getName, args) {
    Handle<Value> ret = v8::Null(isolate);
    if (args.Length() > 0) {
        int idNum = ARG2INT(0);
        std::string* name = sMutexes.getName(idNum);
        if (name != NULL) {
            ret = v8::String::NewFromUtf8(isolate, name->c_str());
        }
    }
    LAZY_RETURN(ret);
}

// ===========================================================================================
//      Initialization
// ===========================================================================================

void init(Handle<v8::Object> exports) {
    NODE_SET_METHOD(exports, "create", create);
    NODE_SET_METHOD(exports, "destroy", destroy);
    NODE_SET_METHOD(exports, "getName", getName);
}

NODE_MODULE(app_mutex, init)
