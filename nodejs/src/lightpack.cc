#include <node.h>
#include <v8.h>
#include <Lightpack.h>

using namespace v8;

#define REG_FUNC(expName, func) \
    exports->Set(String::NewSymbol(#expName), FunctionTemplate::New(func)->GetFunction());

#define REG_NUMBER(expName, number) \
    exports->Set(String::NewSymbol(#expName), Number::New(number));

#define ARG2INT(i) \
    (int)Local<Integer>::Cast(args[i])->Int32Value()

#define ARG2DOUBLE(i) \
    (double)Local<Integer>::Cast(args[i])->Value()

static Lightpack::LedDevice sDevice;

Handle<Value> Open(const Arguments& args) {
    HandleScope scope;
    bool openned = sDevice.open();
    return scope.Close(Boolean::New(openned));
}

Handle<Value> TryToReopenDevice(const Arguments& args) {
    HandleScope scope;
    bool openned = sDevice.tryToReopenDevice();
    return scope.Close(Boolean::New(openned));
}

Handle<Value> CloseDevices(const Arguments& args) {
    HandleScope scope;
    sDevice.closeDevices();
    return scope.Close(Undefined());
}

Handle<Value> SetColor(const Arguments& args) {
    HandleScope scope;
    if (args.Length() >= 4 && args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColor(ARG2INT(0), ARG2INT(1), ARG2INT(2), ARG2INT(3));
        return scope.Close(Boolean::New(result == Lightpack::RESULT::OK));
    }
    else if (args.Length() >= 2 && args[0]->IsNumber() && args[1]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColor(ARG2INT(0), ARG2INT(1));
        return scope.Close(Boolean::New(result == Lightpack::RESULT::OK));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetColors(const Arguments& args) {
    HandleScope scope;
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
                pColors[i] = (Lightpack::RGBCOLOR)Local<Integer>::Cast(colors->Get(i))->Int32Value();
            }
            else {
                pColors[i] = -1;        // Ignore, same comment as above
            }
        }
        Lightpack::RESULT result = sDevice.setColors(pColors, length);
        delete[] pColors;
        return scope.Close(Boolean::New(result == Lightpack::RESULT::OK));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetColorToAll(const Arguments& args) {
    HandleScope scope;
    if (args.Length() >= 3 && args[0]->IsNumber() && args[1]->IsNumber() && args[2]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColorToAll(ARG2INT(0), ARG2INT(1), ARG2INT(2));
        return scope.Close(Boolean::New(result == Lightpack::RESULT::OK));
    }
    else if (args.Length() >= 1 && args[0]->IsNumber()) {
        Lightpack::RESULT result = sDevice.setColorToAll(ARG2INT(0));
        return scope.Close(Boolean::New(result == Lightpack::RESULT::OK));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetSmooth(const Arguments& args) {
    HandleScope scope;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        return scope.Close(Boolean::New(sDevice.setSmooth(ARG2INT(0)) == Lightpack::RESULT::OK));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetGamma(const Arguments& args) {
    HandleScope scope;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        return scope.Close(Boolean::New(sDevice.setGamma(ARG2DOUBLE(0)) == Lightpack::RESULT::OK));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetBrightness(const Arguments& args) {
    HandleScope scope;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        return scope.Close(Boolean::New(sDevice.setBrightness(ARG2INT(0)) == Lightpack::RESULT::OK));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetColorDepth(const Arguments& args) {
    HandleScope scope;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        return scope.Close(Boolean::New(sDevice.setColorDepth(ARG2INT(0))));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> SetRefreshDelay(const Arguments& args) {
    HandleScope scope;
    if (args.Length() > 0 && args[0]->IsNumber()) {
        return scope.Close(Boolean::New(sDevice.setRefreshDelay(ARG2INT(0))));
    }
    return scope.Close(Boolean::New(false));
}

Handle<Value> GetCountLeds(const Arguments& args) {
    HandleScope scope;
    return scope.Close(Number::New(sDevice.getCountLeds()));
}

Handle<Value> TurnOff(const Arguments& args) {
    HandleScope scope;
    return scope.Close(Boolean::New(sDevice.turnOff() == Lightpack::RESULT::OK));
}

Handle<Value> TurnOn(const Arguments& args) {
    HandleScope scope;
    return scope.Close(Boolean::New(sDevice.turnOn() == Lightpack::RESULT::OK));
}

void init(Handle<Object> exports) {
    REG_FUNC(open, Open)
    REG_FUNC(tryToReopenDevice, TryToReopenDevice)
    REG_FUNC(closeDevices, CloseDevices)

    REG_FUNC(setColor, SetColor)
    REG_FUNC(setColors, SetColors)
    REG_FUNC(setColorToAll, SetColorToAll)

    REG_FUNC(setSmooth, SetSmooth)
    REG_FUNC(setGamma, SetGamma)
    REG_FUNC(setBrightness, SetBrightness)
    REG_FUNC(setColorDepth, SetColorDepth)
    REG_FUNC(setRefreshDelay, SetRefreshDelay)

    REG_FUNC(getCountLeds, GetCountLeds)

    REG_FUNC(turnOff, TurnOff)
    REG_FUNC(turnOn, TurnOn)

    REG_NUMBER(LedsPerDevice, Lightpack::LedDevice::LedsPerDevice)
    REG_NUMBER(DefaultBrightness, Lightpack::LedDevice::DefaultBrightness)
    REG_NUMBER(DefaultGamma, Lightpack::LedDevice::DefaultGamma)
}

NODE_MODULE(lightpack, init)