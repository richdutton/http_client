cmake_minimum_required(VERSION 3.2)
project(myproject)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost COMPONENTS system thread REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(app http_client_sync_ssl.cpp)
target_include_directories(app PUBLIC "3rd/beast/include" ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
target_link_libraries(app ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})

add_library(libgtest STATIC IMPORTED)
set_property(TARGET libgtest PROPERTY IMPORTED_LOCATION ../3rd/googletest/build/googlemock/gtest/libgtest.a)
add_library(libgtest_main STATIC IMPORTED)
set_property(TARGET libgtest_main PROPERTY IMPORTED_LOCATION ../3rd/googletest/build/googlemock/gtest/libgtest_main.a)

add_executable(tests tests.cpp)
target_include_directories(tests PUBLIC "3rd/googletest/googletest/include")
# observe results of grep "3rd" CMakeFiles/tests.dir/link.txt
target_link_libraries(tests libgtest libgtest_main)
