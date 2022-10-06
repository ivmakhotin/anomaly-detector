cmake_minimum_required(VERSION 3.16.3)

project(anomaly-detector)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(include)

set(SOURCE_FILES
        include/exceptions.h
        include/config.h
        src/main.cpp
)

add_executable(anomaly-detector
    ${SOURCE_FILES}
)

find_package(Poco REQUIRED Foundation Net NetSSL JSON)

target_link_libraries(anomaly-detector
    PocoFoundation
    PocoNet
    PocoNetSSL
    PocoJSON
)