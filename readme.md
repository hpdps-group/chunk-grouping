### Introduction
This project is an implementation of clustering compression algorithm.
* Developers:
* Advisors:

### Dependences
lz4 zstd glog-under0.6.0 gflags gtest OpenBLAS gsl

### Test Files
test data dir: `/cluster_compress/test_data/test/`

put your test data in this dir.

### Build
To build the library, run the following commands：
```
git clone git@github.com:xiaodou1117/cluster_compress.git
cd cluster_compress
mkdir build
cd build
cmake ..
make -j
cd ..
```

### Run
```
./build/cluster_compression --config="config.toml"
```

### Example Output
![Example Output](example_output.png)