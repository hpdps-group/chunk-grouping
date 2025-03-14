add_library(storage STATIC cache.cpp storage.cpp our_feature.cpp)
target_link_libraries(storage glog gflags gsl gslcblas)