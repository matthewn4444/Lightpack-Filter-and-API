#include <node.h>
#include <v8.h>
#include <Lightpack.h>

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
//      Lightpack device
// ===========================================================================================

// Static variable to run lightpack functions
static Lightpack::LedDevice sDevice;

LAZY_DECLARE(Open, args) {
    LAZY_RETURN_BOOL(sDevice.open());
}

LAZY_DECLARE(TryToReopenDevice, args) {
    LAZY_RETURN_BOOL(sDevice.tryToReopenDevice());
}

LAZY_DECLARE(CloseDevices, args) {
    sDevice.closeDevices();
    LAZY_RETURN_UNDEFINED(args);
}

LAZY_DECLARE(SetColor, args) {
    bool success = false;
    if (args.Length() >= 4 && args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber() && args[3]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColor(ARG2INT(0), ARG2INT(1), ARG2INT(2), ARG2INT(3));
        success = result == Lightpack::RESULT::OK;
    }
    else if (args.Length() >= 2 && args[0]->IsNumber() && args[1]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColor(ARG2INT(0), ARG2INT(1));
        success = result == Lightpack::RESULT::OK;
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetColors, args) {
    bool success = false;
    if (args.Length() > 0 && args[0]->IsArray()) {
        Handle<Array> colors = Handle<Array>::Cast(args[0]);
        size_t length = colors->Length();
        Lightpack::RGBCOLOR * pColors = new  Lightpack::RGBCOLOR[length];
        for (size_t i = 0; i < length; i++) {
            Handle<Value> val = colors->Get(i);
            if (val->IsArray()) {
                Handle<Array> rgbVal = Handle<Array>::Cast(val);
                if (rgbVal->Length() >= 3 && rgbVal->Get(0)->IsNumber() && rgbVal->Get(1)->IsNumber() && rgbVal->Get(2)->IsNumber()) {
                    pColors[i] = MAKE_RGB(rgbVal->Get(0)->Int32Value(), rgbVal->Get(1)->Int32Value(), rgbVal->Get(2)->Int32Value());
                }
                else {
                    pColors[i] = -1;        // Cannot interpret this value so we ignore this index
                }
            }
            else if (val->IsNumber()) {
                pColors[i] = (Lightpack::RGBCOLOR)Local<v8::Integer>::Cast(colors->Get(i))->Int32Value();
            }
            else {
                pColors[i] = -1;        // Ignore, same comment as above
            }
        }
        Lightpack::RESULT result = sDevice.setColors(pColors, length);
        delete[] pColors;
        success = result == Lightpack::RESULT::OK;
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetColorToAll, args) {
    bool success = false;
    if (args.Length() >= 3 && args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColorToAll(ARG2INT(0), ARG2INT(1), ARG2INT(2));
        success = result == Lightpack::RESULT::OK;
    }
    else if (args.Length() >= 1 && args[0]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColorToAll(ARG2INT(0));
        success = result == Lightpack::RESULT::OK;
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetSmooth, args) {
    bool success = false;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        success = sDevice.setSmooth(ARG2INT(0)) == Lightpack::RESULT::OK;
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetGamma, args) {
    bool success = false;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        success = sDevice.setGamma(ARG2DOUBLE(0)) == Lightpack::RESULT::OK;
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetBrightness, args) {
    bool success = false;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        success = sDevice.setBrightness(ARG2INT(0)) == Lightpack::RESULT::OK;
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetColorDepth, args) {
    bool success = false;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        success = sDevice.setColorDepth(ARG2INT(0));
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(SetRefreshDelay, args) {
    bool success = false;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        success = sDevice.setRefreshDelay(ARG2INT(0));
    }
    LAZY_RETURN_BOOL(success);
}

LAZY_DECLARE(GetCountLeds, args) {
    LAZY_RETURN(v8::Number::New(sDevice.getCountLeds()));
}

LAZY_DECLARE(TurnOff, args) {
    LAZY_RETURN_BOOL(sDevice.turnOff() == Lightpack::RESULT::OK);
}

LAZY_DECLARE(TurnOn, args) {
    LAZY_RETURN_BOOL(sDevice.turnOn() == Lightpack::RESULT::OK);
}

// ===========================================================================================
//      Initialization
// ===========================================================================================

void init(Handle<v8::Object> exports) {
    NODE_SET_METHOD(exports, "open", Open);
    NODE_SET_METHOD(exports, "tryToReopenDevice", TryToReopenDevice);
    NODE_SET_METHOD(exports, "closeDevices", CloseDevices);

    NODE_SET_METHOD(exports, "setColor", SetColor);
    NODE_SET_METHOD(exports, "setColors", SetColors);
    NODE_SET_METHOD(exports, "setColorToAll", SetColorToAll);

    NODE_SET_METHOD(exports, "setSmooth", SetSmooth);
    NODE_SET_METHOD(exports, "setGamma", SetGamma);
    NODE_SET_METHOD(exports, "setBrightness", SetBrightness);
    NODE_SET_METHOD(exports, "setColorDepth", SetColorDepth);
    NODE_SET_METHOD(exports, "setRefreshDelay", SetRefreshDelay);

    NODE_SET_METHOD(exports, "getCountLeds", GetCountLeds);

    NODE_SET_METHOD(exports, "turnOff", TurnOff);
    NODE_SET_METHOD(exports, "turnOn", TurnOn);

    NODE_SET_NUMBER(LedsPerDevice, Lightpack::LedDevice::LedsPerDevice);
    NODE_SET_NUMBER(DefaultBrightness, Lightpack::DefaultBrightness);
    NODE_SET_NUMBER(DefaultGamma, Lightpack::DefaultGamma);
    NODE_SET_NUMBER(DefaultSmooth, Lightpack::DefaultSmooth);
}

NODE_MODULE(lightpack, init)
