### Introduction
This project implements a chunk grouping/clustering algorithm to enhance the compression ratio for general lossless compression.

(C) 2025 by Institute of Computing Technology, Chinese Academy of Sciences.

* Developers: Junyu Long, Liyang Zhao
* Advisors: Dingwen Tao, Guangming Tan

### Dependences
Ensure the following libraries are installed before building:
- Compression Libraries: LZ4, Zstandard (ZSTD)
- Logging & Flags: glog (version < 0.6.0), gflags
- Testing Framework: gtest
- Mathematical Libraries: OpenBLAS, GSL

### Test Files
Test data should be placed in
```
/cluster_compress/test_data/test/
```

### Build the Project
Run the following commands to clone, build, and compile the project:

```
git clone git@github.com:xiaodou1117/cluster_compress.git
cd cluster_compress
mkdir build && cd build
cmake ..
make -j
cd ..
```

### Running the Program
Use the following command to execute the chunk grouping algorithm:
```
./build/cluster_compression --config="config.toml"
```

### Example Output
![Example Output](example_output.png)
