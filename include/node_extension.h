#ifndef __NODE_EXTENSION__H__
#define __NODE_EXTENSION__H__

#include <node.h>
#include <v8.h>

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

#define ARG2INT(i) \
    (int)v8::Local<v8::Integer>::Cast(args[i])->Int32Value()

#define ARG2DOUBLE(i) \
    (double)Local<v8::Number>::Cast(args[i])->Value()

// Lazy way to create Node extension functions
#define LAZY_DECLARE(name, arguments) void name(const v8::FunctionCallbackInfo<Value>& arguments) { \
    const v8::FunctionCallbackInfo<Value>& ___args = arguments; \
    v8::Isolate* isolate = v8::Isolate::GetCurrent(); \
    v8::HandleScope scope(isolate);
#define LAZY_RETURN(value) ___args.GetReturnValue().Set(value); }
#define LAZY_RETURN_NUMBER(value) LAZY_RETURN(v8::Number::New(isolate, value))
#define LAZY_RETURN_BOOL(value) LAZY_RETURN(value ? True(isolate) : False(isolate))
#define LAZY_END() }

#endif // __NODE_EXTENSION__H__