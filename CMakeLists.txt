cmake_minimum_required(VERSION 2.8.10)
project(brpc_demo C CXX)

include(GNUInstallDirs)

option(LINK_SO "Whether examples are linked dynamically" OFF)

include(FindThreads)
include(FindProtobuf)
protobuf_generate_cpp(PROTO_SRC PROTO_HEADER echo.proto)
# include PROTO_HEADER
include_directories(${CMAKE_CURRENT_BINARY_DIR})

find_path(BRPC_INCLUDE_PATH NAMES brpc/server.h PATHS ${CMAKE_SOURCE_DIR}/third_party/brpc/include NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
if(LINK_SO)
    find_library(BRPC_LIB NAMES brpc PATHS ${CMAKE_SOURCE_DIR}/third_party/brpc/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
else()
    find_library(BRPC_LIB NAMES libbrpc.a brpc PATHS ${CMAKE_SOURCE_DIR}/third_party/brpc/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
endif()
if((NOT BRPC_INCLUDE_PATH) OR (NOT BRPC_LIB))
    message(FATAL_ERROR "Fail to find brpc")
endif()
include_directories(${BRPC_INCLUDE_PATH})

find_path(GFLAGS_INCLUDE_PATH gflags/gflags.h)
find_library(GFLAGS_LIBRARY NAMES gflags libgflags)
if((NOT GFLAGS_INCLUDE_PATH) OR (NOT GFLAGS_LIBRARY))
    message(FATAL_ERROR "Fail to find gflags")
endif()
include_directories(${GFLAGS_INCLUDE_PATH})

execute_process(
    COMMAND bash -c "grep \"namespace [_A-Za-z0-9]\\+ {\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $2}' | tr -d '\n'"
    OUTPUT_VARIABLE GFLAGS_NS
)
if(${GFLAGS_NS} STREQUAL "GFLAGS_NAMESPACE")
    execute_process(
        COMMAND bash -c "grep \"#define GFLAGS_NAMESPACE [_A-Za-z0-9]\\+\" ${GFLAGS_INCLUDE_PATH}/gflags/gflags_declare.h | head -1 | awk '{print $3}' | tr -d '\n'"
        OUTPUT_VARIABLE GFLAGS_NS
    )
endif()

set(CMAKE_CPP_FLAGS "-DGFLAGS_NS=${GFLAGS_NS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CPP_FLAGS} -DNDEBUG -O2 -D__const__=__unused__ -pipe -W -Wall -Wno-unused-parameter -fPIC -fno-omit-frame-pointer")

if(CMAKE_VERSION VERSION_LESS "3.1.3")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_path(LEVELDB_INCLUDE_PATH NAMES leveldb/db.h PATHS ${CMAKE_SOURCE_DIR}/third_party/leveldb/include NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
if(LINK_SO)
    find_library(LEVELDB_LIB NAMES leveldb PATHS ${CMAKE_SOURCE_DIR}/third_party/leveldb/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
else()
    find_library(LEVELDB_LIB NAMES libleveldb.a leveldb PATHS ${CMAKE_SOURCE_DIR}/third_party/leveldb/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH)
endif()
if ((NOT LEVELDB_INCLUDE_PATH) OR (NOT LEVELDB_LIB))
    message(FATAL_ERROR "Fail to find leveldb")
endif()
include_directories(${LEVELDB_INCLUDE_PATH})

find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR})

set(DYNAMIC_LIB
    ${CMAKE_THREAD_LIBS_INIT}
    ${GFLAGS_LIBRARY}
    ${PROTOBUF_LIBRARIES}
    ${LEVELDB_LIB}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    dl
    )

add_executable(streaming_echo_client client.cpp ${PROTO_SRC} ${PROTO_HEADER})
add_executable(streaming_echo_server server.cpp ${PROTO_SRC} ${PROTO_HEADER})

target_link_libraries(streaming_echo_client PRIVATE ${BRPC_LIB} ${DYNAMIC_LIB})
target_link_libraries(streaming_echo_server PRIVATE ${BRPC_LIB} ${DYNAMIC_LIB})

install(TARGETS streaming_echo_client streaming_echo_server DESTINATION ${CMAKE_INSTALL_BINDIR})

if(LINK_SO)
    install(FILES ${BRPC_LIB} third_party/leveldb/lib/libleveldb.so
                              third_party/leveldb/lib/libleveldb.so.1 
                              third_party/leveldb/lib/libleveldb.so.1.20
            DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# TODO: 由于编译用的mr813的toolchain不是最新版，机器里已有的更新，所以拷贝老的到机器中运行。
install(FILES /opt/aarch64-mr813-linux-gnu/aarch64-mr813-linux-gnu/sysroot/usr/lib/libprotobuf.so
              /opt/aarch64-mr813-linux-gnu/aarch64-mr813-linux-gnu/sysroot/usr/lib/libprotobuf.so.22
              /opt/aarch64-mr813-linux-gnu/aarch64-mr813-linux-gnu/sysroot/usr/lib/libprotobuf.so.22.0.0
        DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES /opt/aarch64-mr813-linux-gnu/aarch64-mr813-linux-gnu/sysroot/usr/lib/libprotoc.so
              /opt/aarch64-mr813-linux-gnu/aarch64-mr813-linux-gnu/sysroot/usr/lib/libprotoc.so.22
              /opt/aarch64-mr813-linux-gnu/aarch64-mr813-linux-gnu/sysroot/usr/lib/libprotoc.so.22.0.0
        DESTINATION ${CMAKE_INSTALL_LIBDIR})
