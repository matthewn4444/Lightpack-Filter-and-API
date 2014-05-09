#include <Lightpack.h>
#include <string>
#include <iostream>

using namespace std;
using namespace Lightpack;

#define HOST "127.0.0.1"
#define PORT 3636
#define APIKEY "{cb832c47-0a85-478c-8445-0b20e3c28cdd}"

void test(const char* name, bool assertion) {
    printf("%s:\t\t\t%s\n", name, assertion ? "Pass" : "Fail");
}

int main()
{
    LedDevice device;
    if (device.open()) {
        test("Smooth", device.setSmooth(100) == OK);
        test("Brightness", device.setBrightness(20) == OK);
        test("Color at all", device.setColorToAll(255, 255, 0) == OK);
        test("Color", device.setColor(4, 255, 0, 0) == OK);
        vector<RGBCOLOR> thing{ MAKE_RGB(0, 255, 0), -1, MAKE_RGB(0, 255, 255), MAKE_RGB(100, 255, 50), -1, -1, -1, -1, 255 };
        test("Set colors", device.setColors(thing) == OK);
        // Color order you should see
        // Green, yellow, blue, light green, red, yellow, yellow, yellow, red, yellow
        system("pause");
    }
    else {
        cout << "Failed to connect to the leds, try to connect to client" << endl;

        PrismatikClient light(HOST, PORT, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, APIKEY);
        try {
            // Test all the functions in the Lightpack API
            if (light.connect() == Lightpack::OK) {
                cout << "Connected" << endl;
                test("Lock", light.lock() == Lightpack::OK);
                test("Number of LEDs", light.getCountLeds() == 10);
                test("Set profile", light.setProfile("Lightpack") == Lightpack::OK);
                test("Smooth", light.setSmooth(100) == Lightpack::OK);
                test("Gamma", light.setGamma(2.5) == Lightpack::OK);
                test("Brightness", light.setBrightness(40) == Lightpack::OK);
                test("API Status", light.getAPIStatus() == Lightpack::BUSY);
                test("Status", light.getStatus() == PrismatikClient::ON);
                test("Get Profile", light.getProfile() == "Lightpack");
                test("Color all", light.setColorToAll(0, 255, 0) == Lightpack::OK);
                test("Color ", light.setColor(1, 255, 0, 0) == Lightpack::OK);

                vector<RGBCOLOR> thing{ MAKE_RGB(0, 255, 0), -1, MAKE_RGB(0, 255, 255), MAKE_RGB(100, 255, 50), -1, -1, -1, -1, 255 };
                test("Color some", light.setColors(thing) == Lightpack::OK);

                vector<string> profiles = light.getProfiles();
                cout << "Profiles:" << endl;
                for (size_t i = 0; i < profiles.size(); i++) {
                    cout << "\t" << i << ": " << profiles[i] << endl;
                }
                test("First profile:", profiles[0] == "Lightpack");
                cout << "Done" << endl;
            }
            else {
                cout << "USB not connected" << endl;
            }
        }
        catch (std::exception& e) {
            cout << e.what() << endl;
        }
        system("pause");
    }
}