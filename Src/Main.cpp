#include <string>
#include <vector>
#include <iostream>
#include <thread>
#include <Antilatency.Api.h>
#include <Antilatency.InterfaceContract.LibraryLoader.h>

#ifndef ANTILATENCY_PACKAGE_DIR
#   define ANTILATENCY_PACKAGE_DIR ""
#endif

#ifndef ADN_LIBRARY
#   define ADN_LIBRARY ANTILATENCY_PACKAGE_DIR "lib/libAntilatencyDeviceNetwork.so"
#endif

#ifndef ALT_TRACKING_LIBRARY
#   define ALT_TRACKING_LIBRARY ANTILATENCY_PACKAGE_DIR "lib/libAntilatencyAltTracking.so"
#endif

static constexpr auto SocketId = Antilatency::DeviceNetwork::UsbDeviceType{ Antilatency::DeviceNetwork::UsbVendorId::Antilatency, 0x0000u };

int main(int argc, char* argv[]) {
    std::vector<std::string> params(argv + 1, argv + argc);

    bool verbose = false;
    std::string environmentCode{};
    for (const auto &param : params) {
        if ("-h" == param) {
            std::cout << argv[0] << " [-v] environment_code" << std::endl;
            return 0;
        } else if ("-v" == param) {
            verbose = true;
        } else {
            environmentCode = param;
        }
    }

    auto adnLibrary = Antilatency::InterfaceContract::getLibraryInterface
            <Antilatency::DeviceNetwork::ILibrary>(ADN_LIBRARY);
    if (adnLibrary == nullptr) {
        std::cerr << "ADN library load failed" << std::endl;
        return 1;
    }
    if (true == verbose) {
        adnLibrary.setLogLevel(Antilatency::DeviceNetwork::LogLevel::Info);
    } else {
        adnLibrary.setLogLevel(Antilatency::DeviceNetwork::LogLevel::Off);
    }
    auto deviceNetwork = adnLibrary.createNetwork({{SocketId}});

    auto altTrackingLibrary = Antilatency::InterfaceContract::getLibraryInterface
            <Antilatency::Alt::Tracking::ILibrary>(ALT_TRACKING_LIBRARY);
    if (altTrackingLibrary == nullptr) {
        std::cerr << "AltTracking library load failed" << std::endl;
        return 1;
    }
    auto cotaskConstructor = altTrackingLibrary.createTrackingCotaskConstructor();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    auto nodes = cotaskConstructor.findSupportedNodes(deviceNetwork);
    if (true == verbose) {
        std::cout << "Supported nodes: " << nodes.size() << std::endl;
    }

    auto trackingNode = Antilatency::DeviceNetwork::NodeHandle::Null;
    if (!nodes.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        for (auto node : nodes) {
            if (deviceNetwork.nodeGetStatus(node) == Antilatency::DeviceNetwork::NodeStatus::Idle) {
                trackingNode = node;
                break;
            }
        }
    }
    if (trackingNode == Antilatency::DeviceNetwork::NodeHandle::Null) {
        std::cerr << "Couldn't find tracking node :-(" << std::endl;
        return 1;
    }

    if (true == environmentCode.empty()) {
        environmentCode = "AAVSaWdpZBcABnllbGxvdwQEBAABAQMBAQEDAAEAAD_W";
    }
    auto environment = altTrackingLibrary.createEnvironment(environmentCode);
    auto trackingCotask = cotaskConstructor.startTask(deviceNetwork, trackingNode, environment);
    while (trackingCotask != nullptr && !trackingCotask.isTaskFinished()) {
        //Get raw tracker state
        auto state = trackingCotask.getState(Antilatency::Alt::Tracking::Constants::DefaultAngularVelocityAvgTime);
        std::cout
                << "position "
                << "x: " << state.pose.position.x
                << ", y: " << state.pose.position.y
                << ", z: " << state.pose.position.z
                << "; rotation "
                << "x: " << state.pose.rotation.x
                << "y: " << state.pose.rotation.y
                << "z: " << state.pose.rotation.z
                << "w: " << state.pose.rotation.w
                << ";" << std::endl;

        //5 FPS pose printing
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
