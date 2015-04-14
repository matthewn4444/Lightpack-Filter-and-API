#include <node.h>
#include <v8.h>
#include <map>
#include <string>
#include <iostream>

#ifndef WIN32
#error This Nodejs module only works with Windows (and node-webkit)
#endif

#include <Windows.h>

using v8::Handle;
using v8::Array;
using v8::Value;
using v8::Local;

// ===========================================================================================
//      Macros to simplify programming
// ===========================================================================================

#define NODE_SET_NUMBER(expName, number) \
    exports->Set(v8::String::NewSymbol(#expName), v8::Number::New(number));

#define ARG2INT(i) \
    (int)v8::Local<v8::Integer>::Cast(args[i])->Int32Value()

#define ARG2DOUBLE(i) \
    (double)Local<v8::Number>::Cast(args[i])->Value()

// TODO make this work with node-gyp as well

// Lazy way to create Node extension functions
#define LAZY_DECLARE(name, arguments) v8::Handle<Value> name(const v8::Arguments& arguments) { \
    v8::Isolate* isolate = v8::Isolate::GetCurrent(); v8::HandleScope s(isolate);
#define LAZY_RETURN(value) return value; }
#define LAZY_RETURN_BOOL(value) return value ? v8::True() : v8::False(); }
#define LAZY_RETURN_UNDEFINED() return v8::Undefined(); }

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
    LAZY_RETURN(v8::Number::New(retNum));
}

LAZY_DECLARE(destroy, args) {
    if (args.Length() > 0) {
        int idNum = ARG2INT(0);
        sMutexes.destroy(idNum);
    }
    LAZY_RETURN_UNDEFINED(args);
}

LAZY_DECLARE(getName, args) {
    Handle<Value> ret = v8::Null();
    if (args.Length() > 0) {
        int idNum = ARG2INT(0);
        std::string* name = sMutexes.getName(idNum);
        if (name != NULL) {
            ret = v8::String::New(name->c_str());
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
