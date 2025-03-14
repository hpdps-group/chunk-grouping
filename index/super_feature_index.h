#pragma once
#include "feature/features.h"
#include "index/index.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <variant>
namespace Delta {

using chunk_id = uint32_t;
class Chunk;
class SuperFeatureIndex : public Index {
public:
  // 构造函数 初始化 super_feature_count_ 默认为3
  SuperFeatureIndex(int super_feature_count = 3, int our_feature_count = 1)
      : super_feature_count_(super_feature_count),our_feature_count_(our_feature_count){
    for (int i = 0; i < super_feature_count_; i++) 
    {
      index_.push_back({}); // 初始化空的index_用于sf
    }
    for(int i = 0; i < our_feature_count_; i++)
    {
      index_float_.push_back({});
    }
  }
  // std::optional 显式地表达了一个值可能不存在的情况。防止裸指针的未定义行为
  // 根据特征找到base block的id
  std::optional<chunk_id> GetBaseChunkID(const Feature &feat);
  //std::optional<chunk_id> GetBaseChunkID(const std::variant<std::vector<uint64_t>, float> &feat);
  void AddFeature(const Feature &feat, chunk_id id);
  bool RecoverFromFile(const std::string &path);
  bool DumpToFile(const std::string &path);

private:
  std::vector<std::unordered_map<uint64_t, chunk_id>> index_;
  std::vector<std::unordered_map<float, chunk_id>> index_float_;
  int super_feature_count_; // sf数量
  int our_feature_count_;
  std::function<std::vector<uint64_t>(std::shared_ptr<Chunk>, const int)>
      get_feature_;
};
} // namespace Delta