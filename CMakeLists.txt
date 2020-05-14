cmake_minimum_required(VERSION 3.10)

project(AntilatencyIpTrackingDemoProvider VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_THREAD_PREFER_PTHREAD ON)
set(THREADS_PREFER_PTHREAD_FLAG ON)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/opt/antilatency/" CACHE PATH "Cmake prefix" FORCE)
endif()

find_package(Threads REQUIRED)

add_executable(${PROJECT_NAME} Src/Main.cpp)

target_compile_features(
    ${PROJECT_NAME}
        PRIVATE
            cxx_std_17
)

target_compile_options(
    ${PROJECT_NAME}
        PRIVATE
            $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic-errors>
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/AntilatencyRaspberryPiSdkCpp/include
        ${CMAKE_CURRENT_SOURCE_DIR}/cxxopts/include
        ${CMAKE_CURRENT_SOURCE_DIR}/Src
)

target_compile_definitions(
    ${PROJECT_NAME}
    PRIVATE
        ANTILATENCY_PACKAGE_DIR=\"${CMAKE_INSTALL_PREFIX}\"
)

if(CMAKE_DL_LIBS)
    target_link_libraries(
        ${PROJECT_NAME}
            PRIVATE
                ${CMAKE_DL_LIBS}
    )
endif()

target_link_libraries(
    ${PROJECT_NAME}
        PRIVATE
            Threads::Threads
)

add_library(wiringPi SHARED IMPORTED)
set_property(
    TARGET wiringPi
    PROPERTY IMPORTED_LOCATION "${CMAKE_SYSROOT}/usr/lib/libwiringPi.so"
)
target_link_libraries(
    ${PROJECT_NAME}
        PRIVATE
            wiringPi
)

set(SDK_PATH "https://github.com/antilatency/Antilatency.RaspberryPiSdk.Cpp/releases/download/0.1.0/")
set(AIP_LIB "libAntilatencyIpNetwork.so")
set(ADN_LIB "libAntilatencyDeviceNetwork.so")
set(ALT_TRACKING_LIB "libAntilatencyAltTracking.so")

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/download.cmake "file(DOWNLOAD \"${SDK_PATH}${AIP_LIB}\" \"./${AIP_LIB}\")\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/download.cmake "file(DOWNLOAD \"${SDK_PATH}${ADN_LIB}\" \"./${ADN_LIB}\")\n")
file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/download.cmake "file(DOWNLOAD \"${SDK_PATH}${ALT_TRACKING_LIB}\" \"./${ALT_TRACKING_LIB}\")\n")

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/download.cmake
    COMMENT "Downloading SDK binaries"
    VERBATIM
)

# Deployment rules for QtCreator
macro(add_deployment_file SRC DEST)
    file(RELATIVE_PATH path ${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    file(APPEND "${CMAKE_SOURCE_DIR}/QtCreatorDeployment.txt" "${path}/${SRC}:${DEST}\n")
endmacro()

file(WRITE "${CMAKE_SOURCE_DIR}/QtCreatorDeployment.txt" "${CMAKE_INSTALL_PREFIX}\n")
add_deployment_file("${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}" bin)
add_deployment_file("${CMAKE_CURRENT_BINARY_DIR}/${AIP_LIB}" lib)
add_deployment_file("${CMAKE_CURRENT_BINARY_DIR}/${ADN_LIB}" lib)
add_deployment_file("${CMAKE_CURRENT_BINARY_DIR}/${ALT_TRACKING_LIB}" lib)
