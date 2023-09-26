cmake_minimum_required(VERSION 3.14)
project(client)
set(CMAKE_CXX_STANDARD 20)

find_package( Boost 1.75 COMPONENTS ${Boost_COMPONENTS} REQUIRED )

add_executable(${PROJECT_NAME}
        client/client.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
        ${Boost_LIBRARIES}
)
