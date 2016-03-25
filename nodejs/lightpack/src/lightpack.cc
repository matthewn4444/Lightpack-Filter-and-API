#include <node_extension.h>
#include <Lightpack.h>

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
    LAZY_END();
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
    LAZY_RETURN_NUMBER(sDevice.getCountLeds());
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

const static unsigned int LedsPerDevice = Lightpack::LedDevice::LedsPerDevice;
const static unsigned int DefaultBrightness = Lightpack::DefaultBrightness;
const static double DefaultGamma = Lightpack::DefaultGamma;
const static unsigned int DefaultSmooth = Lightpack::DefaultSmooth;

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

    NODE_DEFINE_CONSTANT(exports, LedsPerDevice);
    NODE_DEFINE_CONSTANT(exports, DefaultBrightness);
    NODE_DEFINE_CONSTANT(exports, DefaultGamma);
    NODE_DEFINE_CONSTANT(exports, DefaultSmooth);
}

NODE_MODULE(lightpack, init)
