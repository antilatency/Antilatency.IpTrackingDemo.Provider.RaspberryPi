cmake_minimum_required(VERSION 3.10)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_INSTALL_PREFIX "/opt/antilatency/")

project(AntilatencyIpTrackingDemoProvider VERSION 0.1.0 LANGUAGES CXX)

add_executable(AntilatencyIpTrackingDemoProvider Src/Main.cpp)

target_compile_features(AntilatencyIpTrackingDemoProvider
                        PRIVATE
                            cxx_std_17
                       )
target_include_directories(
    AntilatencyIpTrackingDemoProvider
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/AntilatencyRaspberryPiSdkCpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
    )
target_compile_options(
    AntilatencyIpTrackingDemoProvider
    PRIVATE
        -Wall
    )
target_compile_definitions(
    AntilatencyIpTrackingDemoProvider
    PRIVATE
        ANTILATENCY_PACKAGE_DIR=\"${CMAKE_INSTALL_PREFIX}\"
    )
if(CMAKE_DL_LIBS)
    target_link_libraries(AntilatencyIpTrackingDemoProvider
        PRIVATE
            ${CMAKE_DL_LIBS}
    )
endif()

install(
    TARGETS AntilatencyIpTrackingDemoProvider
    DESTINATION bin
    )
