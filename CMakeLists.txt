cmake_minimum_required(VERSION 3.18)
project(dht22)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lwiringPi -lcurl")

add_executable(dht22 main.cpp dht22.h dht22.cpp)
TARGET_LINK_LIBRARIES(SensorTemperatureDHT22 wiringPi curl)
