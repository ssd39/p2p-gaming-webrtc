cmake_minimum_required(VERSION 3.4) 
project(p2p-gaming
        LANGUAGES CXX)
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

set(SOURCE_FILES main.cpp)

set(LIB_JUICE_HEADER juice/juice.h)
set(LIB_EASYWSCLINET easywsclient.hpp easywsclient.cpp)
set(LIB_JSONCPP json/json.h json/json-forwards.h jsoncpp.cpp)




add_executable(${CMAKE_PROJECT_NAME} ${SOURCE_FILES} ${LIB_JSONCPP} ${LIB_JUICE_HEADER} ${LIB_EASYWSCLINET})
#set_property(TARGET p2p-gaming PROPERTY CXX_STANDARD 17)
if(WIN32)
	target_link_libraries(${CMAKE_PROJECT_NAME} "wsock32" "ws2_32" "crypt32" "Wldap32") # winsock2

endif()



add_library(pthread STATIC IMPORTED)
set_property(TARGET pthread PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/pthreadVC1.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} pthread)

#add_library(boostlib STATIC IMPORTED)
#set_property(TARGET boostlib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/boost.lib)
#target_link_libraries(${CMAKE_PROJECT_NAME} boostlib)

add_library(juicelib STATIC IMPORTED)
set_property(TARGET juicelib PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/juice.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} juicelib)

add_library(ssl1 STATIC IMPORTED)
set_property(TARGET ssl1 PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libeay32.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} ssl1)

add_library(ssl2 STATIC IMPORTED)
set_property(TARGET ssl2 PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ssleay32.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} ssl2)


add_library(cur STATIC IMPORTED)
set_property(TARGET cur PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/libcurl.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} cur)

add_library(wsl STATIC IMPORTED)
set_property(TARGET wsl PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/ixwebsocket.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} wsl)

add_library(wsl1 STATIC IMPORTED)
set_property(TARGET wsl1 PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/zlibstatic.lib)
target_link_libraries(${CMAKE_PROJECT_NAME} wsl1)

