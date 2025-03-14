### Dependences
lz4 zstd glog-under0.6.0 gflags gtest OpenBLAS gsl

### Test Files
test data dir: `/cluster_compress/test_data/test/`

### Build
To build the library, run the following commands：
```
git clone git@github.com:xiaodou1117/cluster_compress.git
cd cluster_compress
mkdir build
cd build
cmake ..
make
cd ..
```

### Run
```
./build/cluster_compression --config="config.toml"
```