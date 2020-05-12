#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <stdexcept>
#include <iostream>

#include <wiringPi.h>

#include <Antilatency.Api.h>
#include <Antilatency.InterfaceContract.LibraryLoader.h>

#include "Parameters.h"

#ifndef ANTILATENCY_PACKAGE_DIR
#   define ANTILATENCY_PACKAGE_DIR "./"
#endif

#ifndef ADN_LIBRARY
#   define ADN_LIBRARY ANTILATENCY_PACKAGE_DIR "/lib/libAntilatencyDeviceNetwork.so"
#endif

#ifndef ALT_TRACKING_LIBRARY
#   define ALT_TRACKING_LIBRARY ANTILATENCY_PACKAGE_DIR "/lib/libAntilatencyAltTracking.so"
#endif

#ifndef ANTILATENCY_IP_NETWORK_LIB
#   define ANTILATENCY_IP_NETWORK_LIB ANTILATENCY_PACKAGE_DIR "/lib/libAntilatencyIpNetwork.so"
#endif

namespace Antilatency::IpTrackingDemoProvider {
struct TrackingNode {
    Antilatency::DeviceNetwork::NodeHandle node = Antilatency::DeviceNetwork::NodeHandle::Null;
    Antilatency::Alt::Tracking::ITrackingCotask trackingCotask = nullptr;
    Antilatency::IpNetwork::RawString32 tag{};
};
}

using namespace Antilatency::IpTrackingDemoProvider;
using namespace Antilatency::IpNetwork;

constexpr auto SocketOldId = Antilatency::DeviceNetwork::UsbDeviceType
    { Antilatency::DeviceNetwork::UsbVendorId::AntilatencyLegacy , 0x0000u };
constexpr auto SocketNewId = Antilatency::DeviceNetwork::UsbDeviceType
    { Antilatency::DeviceNetwork::UsbVendorId::Antilatency, 0x0000u };

std::string getInterfaceMacAddress(const std::string &ifname) {
    try {
        std::ifstream iface("/sys/class/net/"+ifname+"/address");
        std::string macAddress((std::istreambuf_iterator<char>(iface)),
                                std::istreambuf_iterator<char>());
        if (macAddress.length() > 0) {
            macAddress.pop_back();
            return macAddress;
        }
    } catch(const std::exception &) {}

    return "00:00:00:00:00:00";
}

int main(int argc, char *argv[]) {
    auto params = Parameters();
    try {
        params = ParametersParser::getParameters(argc, argv);
    } catch (const std::exception &ex) {
        printError(ex.what(), true);
        return 1;
    }

    if (params.showUsage == true) {
        return 0;
    }

    Antilatency::DeviceNetwork::ILibrary adnLibrary{};
    Antilatency::Alt::Tracking::ILibrary altTrackingLibrary{};
    Antilatency::DeviceNetwork::INetwork deviceNetwork{};
    Antilatency::Alt::Tracking::IEnvironment environment{};
    Antilatency::Alt::Tracking::ITrackingCotaskConstructor cotaskConstructor{};

    Antilatency::IpNetwork::ILibrary ainLibrary{};
    ainLibrary = Antilatency::InterfaceContract::getLibraryInterface
            <Antilatency::IpNetwork::ILibrary>(ANTILATENCY_IP_NETWORK_LIB);
    if (ainLibrary == nullptr) {
        printError(std::string("Failed to load  Antilatency::IpNetwork::ILibrary")
                       + std::string(ANTILATENCY_IP_NETWORK_LIB),
                   true);
        return 1;
    }
    auto id = getInterfaceMacAddress("wlan0");
    Antilatency::IpNetwork::INetworkServer netServer = ainLibrary.getNetworkServer(
                id,
                Constants::DefaultIfaceAddress,
                params.receiver,
                std::stoi(params.port),
                Constants::DefaultCommandPort
                );

    try {
        adnLibrary = Antilatency::InterfaceContract::getLibraryInterface
                        <Antilatency::DeviceNetwork::ILibrary>(ADN_LIBRARY);
        if (adnLibrary == nullptr) {
            printError(Antilatency::enumToString(ErrorType::AdnLibraryLoad), true);
            netServer.sendStateMessages({}, {}, Antilatency::enumToString(ErrorType::AdnLibraryLoad));
            return 1;
        }
        if (params.verbose) {
            adnLibrary.setLogLevel(Antilatency::DeviceNetwork::LogLevel::Info);
        } else {
            adnLibrary.setLogLevel(Antilatency::DeviceNetwork::LogLevel::Off);
        }

        deviceNetwork = adnLibrary.createNetwork({SocketNewId, SocketOldId});

        altTrackingLibrary = Antilatency::InterfaceContract::getLibraryInterface
                <Antilatency::Alt::Tracking::ILibrary>(ALT_TRACKING_LIBRARY);
        if (altTrackingLibrary == nullptr) {
            printError(Antilatency::enumToString(ErrorType::AltTrackingLibraryLoad), true);
            netServer.sendStateMessages({}, {}, Antilatency::enumToString(ErrorType::AltTrackingLibraryLoad));
            return 1;
        }

        cotaskConstructor = altTrackingLibrary.createTrackingCotaskConstructor();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (cotaskConstructor == nullptr) {
            printError(Antilatency::enumToString(ErrorType::TrakingCotaskConstructFailed), true);
            netServer.sendStateMessages({} , {}, Antilatency::enumToString(ErrorType::TrakingCotaskConstructFailed));
            return 1;
        }
    } catch (const std::exception &ex) {
        netServer.sendStateMessages({}, {}, ex.what());
        printError(ex.what(), true);
        return 1;
    }

    int setupGpio = wiringPiSetup();

    if (0 == setupGpio ) {
        for (auto gpioPin : params.gpioPinsDefaultState) {
            pinMode(gpioPin.wiringPiPinNumber, gpioPin.mode);
            digitalWrite(gpioPin.wiringPiPinNumber, gpioPin.value);
        }
    }

    std::string prevEnvCode{};
    std::uint32_t prevUpdateId = 0;
    std::vector<TrackingNode> trackingNodes{};

    try {
        netServer.startCommandListening();
    } catch (const std::exception &ex) {
        printError(ex.what(), params.verbose);
        netServer.sendStateMessages({} , {}, ex.what());
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(params.waitTime));

        try {
            auto commands = netServer.getCommands();
            for (std::size_t index = 0; index < commands.size(); index++) {
                auto command = commands.get(index);
                if (command.key() == CommandKey::SetEnvinromentCode) {
                    params.environmentCode = command.value();
                } else if (command.key() == CommandKey::SetSendingRate) {
                    params.waitTime = std::stoi(command.value());
                }
            }
        } catch (const std::exception &ex) {
            try {

                netServer.sendStateMessages({}, {}, ex.what());
            } catch (const std::exception &ex) {
                printError(ex.what(), params.verbose);
            }
            printError(ex.what(), params.verbose);
            continue;
        }

        std::vector<GpioPinState> gpioState{};
        if (0 == setupGpio) {
            for (std::size_t index = 0; index < 28; index++) {
                if (index == 17) {
                    index = index + 4;
                }
                GpioPinState pin;
                pin.number = index;
                pin.value = digitalRead(index);

                gpioState.push_back(pin);
            }
        }

        if (prevEnvCode != params.environmentCode) {
            try {
                environment = altTrackingLibrary.createEnvironment(params.environmentCode);
            } catch (const std::exception &) {
                try {
                    netServer.sendStateMessages(
                        {},
                        gpioState,
                        Antilatency::enumToString(ErrorType::AltEnvironmentArbitrary2D)
                        );
                } catch (const std::exception &ex) {
                    printError(ex.what(), params.verbose);
                }
                printError(Antilatency::enumToString(ErrorType::AltEnvironmentArbitrary2D),
                           params.verbose);
                continue;
            }

            prevEnvCode = params.environmentCode;
        }

        if (prevUpdateId != deviceNetwork.getUpdateId()) {
            trackingNodes = {};

            auto nodes = cotaskConstructor.findSupportedNodes(deviceNetwork);
            if (nodes.empty()) {
                try {
                    netServer.sendStateMessages(
                        {},
                        gpioState,
                        Antilatency::enumToString(ErrorType::TrackingNodeNotFound)
                        );
                } catch (const std::exception &ex) {
                    printError(ex.what(), params.verbose);
                }
                printError(Antilatency::enumToString(ErrorType::TrackingNodeNotFound),
                           params.verbose);
                continue;
            }

            for (auto node : nodes) {
                if (deviceNetwork.nodeGetStatus(node) == Antilatency::DeviceNetwork::NodeStatus::Idle) {
                    if (node != Antilatency::DeviceNetwork::NodeHandle::Null) {
                        TrackingNode trackingNode;
                        trackingNode.node = node;

                        auto parent = deviceNetwork.nodeGetParent(node);
                        std::string tag{};
                        try {
                            tag = deviceNetwork.nodeGetStringProperty(parent, "Tag");
                        } catch (const std::exception &ex) {
                            printError(ex.what(), params.verbose);
                        }

                        if (true == tag.empty()) {
                            try {
                                tag = deviceNetwork.nodeGetStringProperty(
                                          parent,
                                          Antilatency::DeviceNetwork::Interop
                                              ::Constants::HardwareSerialNumberKey
                                          );
                            } catch (const std::exception &ex) {
                                printError(ex.what(), params.verbose);
                            }
                        }

                        trackingNode.tag = ainLibrary.getRawTagFromString(tag);

                        try {
                            trackingNode.trackingCotask =
                                cotaskConstructor.startTask(deviceNetwork, node, environment);
                        } catch (const std::exception &ex) {
                            printError(ex.what(), params.verbose);
                        }

                        trackingNodes.push_back(trackingNode);
                    }
                }
            }

            prevUpdateId = deviceNetwork.getUpdateId();
        }

        std::vector<StateMessage> poses{};

        for (TrackingNode trackingNode : trackingNodes) {
            StateMessage poseSample{};

            poseSample.trackerError = ErrorType::None;

            if (trackingNode.trackingCotask == nullptr) {
                poseSample.trackerError = ErrorType::TrakingCotaskConstructFailed;
                printError(
                    std::to_string(static_cast<uint32_t>(trackingNode.node))
                        + ": "
                        + Antilatency::enumToString(ErrorType::TrakingCotaskConstructFailed),
                    params.verbose
                    );

                continue;
            }

            if (trackingNode.trackingCotask.isTaskFinished()) {
                poseSample.trackerError = ErrorType::TrackingTaskRestartMessage;
                printError(
                    std::to_string(static_cast<uint32_t>(trackingNode.node))
                        + ": "
                        + Antilatency::enumToString(ErrorType::TrackingTaskRestartMessage),
                    params.verbose
                    );
                trackingNode.trackingCotask = cotaskConstructor.startTask(deviceNetwork, trackingNode.node, environment);
                continue;
            }

            try {
                auto state = trackingNode.trackingCotask.getState(
                            Antilatency::Alt::Tracking::Constants::DefaultAngularVelocityAvgTime
                            );

                poseSample.positionX = state.pose.position.x;
                poseSample.positionY = state.pose.position.y;
                poseSample.positionZ = state.pose.position.z;
                poseSample.rotationX = state.pose.rotation.x;
                poseSample.rotationY = state.pose.rotation.y;
                poseSample.rotationZ = state.pose.rotation.z;
                poseSample.rotationW = state.pose.rotation.w;
            } catch(const std::exception &) {
                poseSample.trackerError = ErrorType::GetTrackerStateFailed;
            }

            poseSample.rawTag = trackingNode.tag;
            poses.push_back(poseSample);
        }

        std::string deviceError{};

        if (true == trackingNodes.empty()) {
            deviceError += Antilatency::enumToString(ErrorType::TrackingNodeNotFound) + std::string(" ");
        }
        if (0 != setupGpio) {
            deviceError += Antilatency::enumToString(ErrorType::SetupGpio) + std::string(" ");
        }

        if (true == params.verbose) {
            std::stringstream output{};
            output << "id: " << params.identifier
                   << "; c_time: " << ainLibrary.getCurrentTime()
                   << "; error: " << deviceError
                   << "; gpio: ";
            for (auto pin : gpioState) {
                output << pin.value;
            }
            for (auto const &pose : poses) {
                output << "\n\t"
                       << "tag: " << ainLibrary.getTagFromRawTag(pose.rawTag)
                       << "; err: "  << Antilatency::enumToString(pose.trackerError)
                       << "; posX: " << pose.positionX
                       << "; posY: " << pose.positionY
                       << "; posZ: " << pose.positionZ
                       << "; rotX: " << pose.rotationX
                       << "; rotY: " << pose.rotationY
                       << "; rotZ: " << pose.rotationZ
                       << "; rotW: " << pose.rotationW;
            }
            output << "\n";
            std::cout << output.str();
            std::flush(std::cout);
        }

        try {
            netServer.sendStateMessages(poses, gpioState, deviceError);
        } catch (const std::exception &ex) {
            printError(ex.what(), params.verbose);
        }
    }

    return 0;
}
