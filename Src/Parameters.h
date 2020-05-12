#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <array>
#include <cstring>

#include <cxxopts.hpp>

#include <Antilatency.Api.h>

namespace Antilatency::IpTrackingDemoProvider {

const std::array<std::uint8_t, 28> wiringPiPins
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
     10, 11, 12, 13, 14, 15, 16,
     21, 22, 23, 24, 25, 26, 27, 28, 29,
     30, 31};

struct GpioPin {
    std::int8_t wiringPiPinNumber = -1;
    std::int8_t mode = -1;
    std::int8_t value = -1;
};

void printMessage(const std::string &message, bool verbose = false) {
    if (verbose) {
        std::cout << message << "\n";
    }
}

void printError(const std::string &message, bool verbose = false) {
    if (verbose) {
        std::cerr << message << "\n";
    }
}

std::int32_t getRandomId() {
    std::srand(std::time(0)); //use current time as seed for random generator
    int randomValue = std::rand();
    return randomValue;
}

struct Parameters {
public:
    Parameters () {
        std::int32_t randomValue = getRandomId();
        identifier = {"rand" + std::to_string(randomValue)};
    }

    bool showUsage = false;
    bool verbose = false;
    std::string processName{};
    std::string receiver{};
    std::string port = std::to_string(Antilatency::IpNetwork::Constants::DefaultTrackingPort);
    std::string environmentCode = "AAVSaWdpZBcABnllbGxvdwQEBAABAQMBAQEDAAEAAD_W";
    std::string identifier = "";
    std::string configFile = "";
    std::int32_t waitTime = 200;
    std::string tag{};
    std::vector<GpioPin> gpioPinsDefaultState{};
};


class ParametersParser {
public:
    static Parameters getParameters(int argc, char *argv[]) {
        auto params = Parameters();

        cxxopts::Options cxxoptsOptions("RaspberryPiUdpTrackingProvider", "Antilatency Raspberry Pi tracking provider");

        cxxoptsOptions.add_options()
            ("h,help", "Print usage", cxxopts::value<bool>())
            ("v,verbose", "Verbose output", cxxopts::value<bool>())
            ("r,receiver", "Network name or address of Antilatency.RaspberryPiSdk.Unity", cxxopts::value<std::string>())
            ("p,port", "Network port of UdpTrackingReceiver", cxxopts::value<std::string>())
            ("e,environment", "Tracking environment code", cxxopts::value<std::string>())
            ("w,wait-time", "A number of milliseconds between a new position request", cxxopts::value<std::int32_t>())
            ("i,identifier", "The identifier of the app instance", cxxopts::value<std::string>())
            ("c,config", "Try to read parameters from a file first (one per line)", cxxopts::value<std::string>())
            ("g,gpio",
             "Set initial state of a GPIO pins. Format: 25:output:high,27:input:low (Number:Mode:Value)",
             cxxopts::value<std::string>())
            ;

        cxxopts::ParseResult result = cxxoptsOptions.parse(argc, argv);
        if (result.count("config") > 0) {
            std::vector<std::string> args{};

            std::string configFile = result["config"].as<std::string>();
            try {
                args = readConfigFile(configFile);
            } catch (const std::exception &ex) {
                throw std::runtime_error({"error while parsing configuration file: "
                                          + std::string(ex.what())
                                          + "\n"});
            }

            if (args.size() == 0) {
                throw std::runtime_error({"error configuration file "
                                          + configFile
                                          + " has wrong format or does not contain parameters\n"});
            }

            args.insert(args.begin(), std::string(argv[0]));
            char **tmpArgv = new char*[args.size()];
            for (std::size_t index = 0; index < args.size(); index++) {
                tmpArgv[index] = new char[args[index].size() + 1];
                std::strcpy(tmpArgv[index], args[index].c_str());
            }

            std::vector<std::string> argsFromFile(tmpArgv, tmpArgv + args.size());
            int tmpArgc = args.size();
            cxxopts::ParseResult argsFromConfig = cxxoptsOptions.parse(tmpArgc, tmpArgv);
            parseArgs(argsFromConfig, params);

            for (std::size_t index = 0; index < args.size(); index++) {
                delete [] tmpArgv[index];
            }
            delete [] tmpArgv;

            params.configFile = configFile;
        }

        parseArgs(result, params);

        if (true == params.showUsage) {
            auto help = cxxoptsOptions.help();
            printMessage(help, true);
        }

        if (true == params.receiver.empty()) {
            params.receiver = Antilatency::IpNetwork::Constants::DefaultTargetAddress;
        }

        return params;
    }
private:
    static std::vector<std::string> readConfigFile(const std::string &filePath) {
        std::string line{};
        std::vector<std::string> args{};

        try  {
            std::ifstream f(filePath);
            if (!f.is_open()) {
                throw std::runtime_error("Could not open configuration file\n");
            }
            while(std::getline(f, line)) {
                if (false == line.empty()) {
                    if ('#' != line.at(0)) {
                        args.push_back(line);
                    }
                }
            }
            f.close();
        } catch (const std::exception &ex) {
            std::stringstream ss;
            ss << "Error while parsing configuration file: " << ex.what() << "\n";
            throw std::runtime_error(ss.str());
        }

        return args;
    }

    static std::vector<GpioPin> parseGpioParameter(const std::string &pins) {
        std::vector<GpioPin> result{};

        if (false == pins.empty()) {
            std::string pin{};
            std::stringstream ss(pins);

            while (std::getline(ss, pin, ',')) {
                std::stringstream sss(pin);
                std::string tmp{};
                std::vector<std::string> gpioPinProperties{};

                while (std::getline(sss, tmp, ':')) {
                    gpioPinProperties.push_back(tmp);
                }

                if (gpioPinProperties.size() == 3) {
                    std::int8_t wiringPiPinNumber = std::stoi(gpioPinProperties[0]);
                    std::string mode = gpioPinProperties[1];
                    std::string value = gpioPinProperties[2];

                    GpioPin gpioPin{};
                    if (0 == std::count(wiringPiPins.begin(), wiringPiPins.end(), wiringPiPinNumber)) {
                        throw std::runtime_error("Could not parse GPIO pin number");
                    }
                    gpioPin.wiringPiPinNumber = wiringPiPinNumber;

                    if ("output" == mode) {
                        gpioPin.mode = 1;
                    } else if ("input" == mode) {
                        gpioPin.mode = 0;
                    } else {
                        throw std::runtime_error("Could not parse GPIO pin mode");
                    }

                    if ("high" == value) {
                        gpioPin.value = 1;
                    } else if ("low" == value) {
                        gpioPin.value = 0;
                    } else {
                        throw std::runtime_error("Could not parse GPIO pin value");
                    }

                    result.push_back(gpioPin);
                }

            }

        }

        return result;
    }

    static void parseArgs(const cxxopts::ParseResult &args, Parameters &inParams) {
        if (args.count("help") > 0) {
            inParams.showUsage = true;
        }

        if (args.count("verbose") > 0) {
            inParams.verbose = args["verbose"].as<bool>();
        }

        if (args.count("receiver") > 0) {
            inParams.receiver = args["receiver"].as<std::string>();
        }

        if (args.count("port") > 0) {
            inParams.port = args["port"].as<std::string>();
        }

        if (args.count("environment") > 0) {
            inParams.environmentCode = args["environment"].as<std::string>();
        }

        if (args.count("wait-time") > 0) {
            inParams.waitTime = args["wait-time"].as<std::int32_t>();
        }

        if (args.count("identifier") > 0) {
            inParams.identifier = args["identifier"].as<std::string>();
        }

        if ( args.count("gpio") > 0) {
            std::string pins{};
            pins = args["gpio"].as<std::string>();

            inParams.gpioPinsDefaultState = parseGpioParameter(pins);
        }
    }
};

}
