cmake_minimum_required(VERSION 3.5.0)
project(SamplingRef_IMU_GNSS VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

# 32 bit cross compiling
# SET(CMAKE_C_COMPILER /usr/bin/arm-linux-gnueabi-gcc)
# SET(CMAKE_CXX_COMPILER /usr/bin/arm-linux-gnueabi-g++)
# set(CMAKE_SYSTEM_PROCESSOR arm)

# 64 bit cross compiling
# SET(CMAKE_C_COMPILER /usr/bin/aarch64-linux-gnu-gcc)
# SET(CMAKE_CXX_COMPILER /usr/bin/aarch64-linux-gnu-g++)
# set(CMAKE_SYSTEM_PROCESSOR arm)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
INCLUDE_DIRECTORIES(${LOCAL_LIBRARY_DIRS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

include(CPack)

# file(GLOB_RECURSE SOURCE 
#             src/main.cpp
#             src/serial_driver.h
#             src/serial_driver.cpp
#             src/dgnss.h
#             src/dgnss.cpp
#             src/zimu.h
#             src/zimu.cpp
#             src/ColorParse.h
#             src/ColorParse.cpp
#             src/MBUtils.h
#             src/MBUtils.cpp)
file(GLOB_RECURSE SOURCE
            src/*.h
            src/*.cpp)

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(${PROJECT_NAME} ${SOURCE})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
# include(CPack)

# Boost
find_package(Boost)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})