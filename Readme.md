
### 如果用ja308的工具链

```shell
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=`pwd`/out -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$HOME/robot/toolchain/aarch64-ja308-linux-gnu.cmake -DPROTOBUF_PROTOC_EXECUTABLE=$HOME/robot/toolchain/aarch64-ja308-linux-gnu/bin/protoc ..
make -j8
make install
```

### 如果用其他工具链

1. 重新编译brpc（主要是因为不同工具链protobuf版本不同，brpc中和protobuf相关的头文件options.pb.h等对版本有要求）。

a. 
修改CMakeLists.txt中关于leveldb的部分：
set(LEVELDB_INCLUDE_PATH /root/brpc_demo/third_party/leveldb/include)
set(LEVELDB_LIB /root/brpc_demo/third_party/leveldb/lib/libleveldb.so)

b. 
cmake -DCMAKE_INSTALL_PREFIX=`pwd`/out -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=xxx.cmake -DPROTOBUF_PROTOC_EXECUTABLE=/path/to/toolchain/bin/protoc ..
make -j8
make install
aarch64-linux-gnu-strip --strip-unneeded out/lib/libbrpc.a

2. 再用相应工具链编译brpc_demo
