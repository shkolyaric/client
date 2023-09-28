cmake_minimum_required(VERSION 3.10)
project(client)

## Common flags #################################################################
set(CMAKE_CXX_STANDARD 20)
set(Boost_NO_WARN_NEW_VERSIONS 1)
set(TARGET_EXE_NAME ${PROJECT_NAME})

### Boost ###

set(Boost_COMPONENTS log log_setup system)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.69 COMPONENTS ${Boost_COMPONENTS} REQUIRED)

set(LIBS
        ${Boost_LIBRARIES}
)

add_executable(${TARGET_EXE_NAME} client/client.cpp ${SRC} )

target_include_directories(${TARGET_EXE_NAME} PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${TARGET_EXE_NAME} ${LIBS})

