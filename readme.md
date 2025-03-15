### Introduction
This project implements a chunk grouping/clustering algorithm to enhance the compression ratio for general lossless compression.

(C) 2025 by Institute of Computing Technology, Chinese Academy of Sciences.

* Developers: Junyu Long, Liyang Zhao
* Advisors: Dingwen Tao, Guangming Tan

### software libraries requirements
OS：Linux（Ubuntu is recommended）

gcc(>=7.1)

cmake(>=3.10)
 
### Install Dependencies
Ensure the following libraries are installed before building:
```
mkdir chunk_grouping_ae
cd chunk_grouping_ae
export ROOT_DIR=$pwd
mkdir dependencies
```

- Compression Libraries: LZ4, Zstandard (ZSTD)

**LZ4**
```
cd $ROOT_DIR
    wget https://github.com/lz4/lz4/releases/download/v1.10.0/lz4-1.10.0.tar.gz
    tar -zxvf lz4-1.10.0.tar.gz
    cd lz4-1.10.0
    make
```

**Zstandard**
```
cd $ROOT_DIR
    wget https://github.com/facebook/zstd/releases/download/v1.5.7/zstd-1.5.7.tar.gz
    tar -zxvf zstd-1.5.7.tar.gz
    cd zstd-1.5.7
    make
```

- Logging & Flags: glog (version <= 0.6.0), gflags

**gflags**
```
cd $ROOT_DIR
wget https://github.com/gflags/gflags/archive/refs/tags/v2.2.2.tar.gz
tar -xvzf v2.2.2.tar.gz
cd gflags
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON -DCMAKE_INSTALL_PREFIX=$ROOT_DIR/dependencies
make && make install
```

**glog**
```
cd $ROOT_DIR
wget https://github.com/google/glog/archive/refs/tags/v0.6.0.tar.gz
tar -xvzf v0.6.0.tar.gz
cd glog-0.6.0
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON -DCMAKE_INSTALL_PREFIX=$ROOT_DIR/dependencies
make && make install
```

- Testing Framework: gtest

**gtest**
```
cd $ROOT_DIR
wget https://github.com/google/googletest/archive/refs/tags/v1.16.0.tar.gz
tar -xvzf v1.16.0.tar.gz
cd googletest
mkdir build && cd build
cmake .. -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON -DCMAKE_INSTALL_PREFIX=$ROOT_DIR/dependencies
make && make install
```

- Mathematical Libraries: GSL

**GSL**
```
cd $ROOT_DIR
wget https://mirror.ibcp.fr/pub/gnu/gsl/gsl-latest.tar.gz
tar -xvzf gsl-latest.tar.gz
cd gsl-2.8
./configure --prefix=$ROOT_DIR/dependencies
make && make install
```

### environment path
Write in .bashrc
```
export LD_LIBRARY_PATH=$ROOT_DIR/dependencies/lib:$LD_LIBRARY_PATH
    export LIBRARY_PATH=$ROOT_DIR/dependencies/lib:$LIBRARY_PATH
    export C_INCLUDE_PATH=$ROOT_DIR/dependencies/include:$C_INCLUDE_PATH
    export CPLUS_INCLUDE_PATH=$ROOT_DIR/dependencies/include:$CPLUS_INCLUDE_PATH
    export CMAKE_PREFIX_PATH=$ROOT_DIR/dependencies:$CMAKE_PREFIX_PATH
    export LD_LIBRARY_PATH=$ROOT_DIR/lz4-1.10.0/lib:$ROOT_DIR/zstd-1.5.7/lib:$LD_LIBRARY_PATH
    export LIBRARY_PATH=$ROOT_DIR/lz4-1.10.0/lib:$ROOT_DIR/zstd-1.5.7/lib:$LIBRARY_PATH
    export C_INCLUDE_PATH=$ROOT_DIR/lz4-1.10.0/lib:$ROOT_DIR/zstd-1.5.7/lib:$C_INCLUDE_PATH
    export CPLUS_INCLUDE_PATH=$ROOT_DIR/lz4-1.10.0/lib:$ROOT_DIR/zstd-1.5.7/lib:$CPLUS_INCLUDE_PATH
```
### Test Files
```
cd test_data && mkdir test
```
Test data should be placed in
```
/chunk-grouping/test_data/test/
```

### Build the Project
Run the following commands to clone, build, and compile the project:

```
cd $ROOT_DIR
git clone git@github.com:hpdps-group/chunk-grouping.git
cd chunk-grouping
mkdir build && cd build
cmake .. && make -j
cd ..
mkdir logs
```

### Running the Program
Use the following command to execute the chunk grouping algorithm:
```
./build/cluster_compression --config="config.toml"
```

### Example Output
![Example Output](example_output.png)
