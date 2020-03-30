cmake_minimum_required(VERSION 3.4) 
project(p2p-gaming
        LANGUAGES CXX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(SOURCE_FILES main.cpp)

set(LIB_JUICE_HEADER juice/juice.h)
#set(LIB_EASYWSCLINET easywsclient.hpp easywsclient.cpp)
set(LIB_JSONCPP json/json.h json/json-forwards.h jsoncpp.cpp)




add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES} ${LIB_JSONCPP} ${LIB_JUICE_HEADER})

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.45.0 COMPONENTS system) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    target_link_libraries(${CMAKE_PROJECT_NAME}  ${Boost_LIBRARIES})
endif()

set(CURL_LIBRARY "-lcurl") 
find_package(CURL REQUIRED) 
include_directories(${CURL_INCLUDE_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} ${CURL_LIBRARIES})

add_library(juice_lib SHARED IMPORTED)
set_property(TARGET juice_lib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libjuice/build/libjuice.so)
target_link_libraries(${CMAKE_PROJECT_NAME} juice_lib)


