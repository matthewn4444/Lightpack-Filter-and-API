#include <Lightpack.h>
#include <string>
#include <iostream>

using namespace std;

#define HOST "127.0.0.1"
#define PORT 3636
#define APIKEY "{cb832c47-0a85-478c-8445-0b20e3c28cdd}"

void test(const char* name, bool assertion) {
    printf("%s:\t\t\t%s\n", name, assertion ? "Pass" : "Fail");
}

int main()
{
    Lightpack light(HOST, PORT, { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 }, APIKEY);
    try {
        // Test all the functions in the Lightpack APIj
        if (light.connect() == 0) {
            cout << "Connected" << endl;
            test("Lock",            light.lock() == Lightpack::RESULT::OK);
            test("Number of LEDs",  light.getCountLeds() == 10);
            test("Set profile",     light.setProfile("Lightpack") == Lightpack::RESULT::OK);
            test("Smooth",          light.setSmooth(100) == Lightpack::RESULT::OK);
            test("Gamma",           light.setGamma(2.5) == Lightpack::RESULT::OK);
            test("Brightness",      light.setBrightness(10) == Lightpack::RESULT::OK);
            test("API Status",      light.getAPIStatus() == Lightpack::RESULT::BUSY);
            test("Status",          light.getStatus() == Lightpack::STATUS::ON);
            test("Get Profile",     light.getProfile() == "Lightpack");
            test("Color all",       light.setColorToAll(0, 255, 0) == Lightpack::RESULT::OK);
            test("Color all",       light.setColor(1, 255, 0, 0) == Lightpack::RESULT::OK);

            vector<string> profiles = light.getProfiles();
            cout << "Profiles:" << endl;
            for (size_t i = 0; i < profiles.size(); i++) {
                cout << "\t" << i << ": " << profiles[i] << endl;
            }
            test("First profile:",  profiles[0] == "Lightpack");
            cout << "Done" << endl;
        }
    }
    catch (std::exception& e) {
        cout << e.what() << endl;
    }

    system("pause");
}