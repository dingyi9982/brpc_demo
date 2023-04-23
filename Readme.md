
mkdir build && cd build

cmake -DCMAKE_INSTALL_PREFIX=`pwd`/out -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=~/robot/cmake/aarch64-mr813-linux-gnu.cmake -DPROTOBUF_PROTOC_EXECUTABLE=/opt/aarch64-mr813-linux-gnu/bin/protoc ..

make install/strip
