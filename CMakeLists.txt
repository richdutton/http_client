cmake_minimum_required(VERSION 3.2)
project(myproject)

set(CMAKE_CXX_STANDARD 14)

find_package(Boost COMPONENTS system thread REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(app http_client_sync_ssl.cpp)
message(${Boost_INCLUDE_DIRS})
target_include_directories(app PUBLIC "3rd/beast/include" ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR})
target_link_libraries(app ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES})
