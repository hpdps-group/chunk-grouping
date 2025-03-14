### dependences
lz4 zstd glog-0.6.0以下 gflags gtest OpenBLAS gsl

### 测试数据
测试数据放入/cluster_compress/test_data/test/目录下

### 编译运行 
常规的cmake项目构建方式：
```
cd cluster_compress
mkdir build
cd build
cmake ..
make
cd ..
```

### 运行：
```
./build/cluster_compression --config="config.toml"
```