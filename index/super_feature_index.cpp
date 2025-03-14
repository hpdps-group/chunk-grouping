#include "chunk/chunk.h"
#include "index/super_feature_index.h"
#include "feature/features.h"
#include <assert.h>
#include <fstream>
#include <glog/logging.h>
#include <optional>
#include <variant>
#include <vector>
#include <cmath>
namespace Delta {
using chunk_id = uint32_t;
std::optional<chunk_id> SuperFeatureIndex::GetBaseChunkID(const Feature &feat) 
{
  // using Feature = std::variant<std::vector<std::vector<uint64_t>>,std::vector<uint64_t>>;
  const auto feature = std::get<std::variant<std::vector<uint64_t>, std::vector<float> >>(feat);
  std::optional<chunk_id> result = std::nullopt; // 初始化为空指针
  // 处理super feature
  if (std::holds_alternative<std::vector<uint64_t>>(feature))
  {
    const auto &super_feature = std::get<std::vector<uint64_t>>(feature);
    for (int i = 0; i < super_feature_count_; i++) 
    {
      /*LOG(INFO) << "super feature: " << super_feature[i];
      LOG(INFO) << "index_["<< i <<"] :";
      for(auto pair : index_[i]){
        LOG(INFO) << pair.first;
      }*/
      // get a matched super feature //
      // 相等就认为是base
      if(index_[i].count(super_feature[i]))
      { // index[i]表示第i层 
        // first fit //
        result = index_[i][super_feature[i]]; // result:std::optional<chunk_id>类型,用于存储匹配到的块ID
        //LOG(INFO) << "base by sf, chunkid: " << result.value() << "has value " << result.has_value();
        break;
      }
    }
  }

  const float threshold = 0.01;
  if(!result.has_value() && std::holds_alternative< std::vector<float> >(feature)){
    const auto &our_feature = std::get<std::vector<float>>(feature);
    for(int i = 0; i < our_feature_count_; i++){
      for(auto index : index_float_[i])
      {
        if(std::abs(index.first - our_feature[i]) < threshold){
          result = index.second;
          break;
        }
      }
    }
  }
  
  //if (!result.has_value()) {LOG(INFO) << "self base";}
  //else {LOG(INFO) << "result: " << result.value();}
  return result;
}

void SuperFeatureIndex::AddFeature(const Feature &feat, chunk_id id) {
  /*const auto &super_feature = std::get<std::vector<uint64_t>>(feat);
  for (int i = 0; i < super_feature_count_; i++) {
    index_[i][super_feature[i]] = id; // 将sf与块ID关联
  }*/
  const auto &super_feature = std::get<std::variant<std::vector<uint64_t>, std::vector<float> >>(feat);
  if(std::holds_alternative<std::vector<uint64_t>>(super_feature)){
    auto feature = std::get<std::vector<uint64_t>>(super_feature);
    for (int i = 0; i < super_feature_count_; i++) {
      index_[i][feature[i]] = id; // 将sf与块ID关联
    }
  }
  else if(std::holds_alternative<std::vector<float>>(super_feature)){
    auto feature = std::get<std::vector<float>>(super_feature);
    for (int j = 0; j < our_feature_count_; j++) {
      index_float_[j][feature[j]] = id;
      //LOG(INFO) << "id: " << id << " attach to feature " << feature[j];
    }
  }
}
bool SuperFeatureIndex::DumpToFile(const std::string &path) {
  std::ofstream outFile(path, std::ios::binary);
  if (!outFile) {
    LOG(FATAL) << "SuperFeatureIndex::DumpToFile: cannot open output file "
              << path;
    return false;
  }
  auto write_uint64 = [&](uint64_t data) {
    outFile.write(reinterpret_cast<const char *>(&data), sizeof(uint64_t));
  };
  auto write_uint32 = [&](uint32_t data) {
    outFile.write(reinterpret_cast<const char *>(&data), sizeof(uint32_t));
  };
  write_uint32(super_feature_count_);
  for (int i = 0; i < super_feature_count_; i++) {
    write_uint64(index_[i].size());
    for (const auto &[k, v] : index_[i]) {
      write_uint64(k);
      write_uint32(v);
    }
  }
  return true;
}

bool SuperFeatureIndex::RecoverFromFile(const std::string &path) {
  std::ifstream inFile(path, std::ios::binary);
  if (!inFile) {
    LOG(FATAL) << "SuperFeatureIndex::RecoverFromFile: cannot open output file "
              << path;
    return false;
  }
  int super_feature_count = 0;
  inFile.read(reinterpret_cast<char *>(&super_feature_count), sizeof(int));
  if (super_feature_count_ != super_feature_count) {
    LOG(FATAL) << "super feature count changed after recover, abort";
    return false;
  }
  auto read_uint64 = [&]() -> uint64_t {
    uint64_t result = 0;
    inFile.read(reinterpret_cast<char *>(&result), sizeof(uint64_t));
    return result;
  };
  auto read_uint32 = [&]() -> uint32_t {
    uint32_t result = 0;
    inFile.read(reinterpret_cast<char *>(&result), sizeof(uint32_t));
    return result;
  };
  for (int i = 0; i < super_feature_count_; i++) {
    uint64_t mapSize = read_uint64();
    for (int j = 0; j < mapSize; j++) {
      auto feature = read_uint64();
      auto chunk_id = read_uint32();
      index_[i][feature] = chunk_id;
    }
  }
  return true;
}
} // namespace Delta