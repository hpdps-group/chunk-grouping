#include "index/palantir_index.h"
#include <glog/logging.h>
namespace Delta {
std::optional<chunk_id> PalantirIndex::GetBaseChunkID(const Feature &feat) {
  //const auto &features = std::get<std::vector<std::vector<uint64_t>>>(feat);
  const auto &features = std::get< std::vector< std::variant <std::vector<uint64_t>, std::vector<float> > > > (feat);
  for (int i = 0; i < features.size()-1; i++) {
    // 这个GetBaseChunkID和上面那个GetBaseChunkID不同，这个是superfeatureindex里面的GetBaseChunkID函数
    // 对每一层的sf查找是否有相同的sf
    
    auto chunk = levels_[i]->GetBaseChunkID(features[i]);
    // 找到base block
    if(chunk.has_value())
      return chunk;
  }
  auto chunk = levels_[features.size()-2]->GetBaseChunkID(features[features.size()-1]);
  if(chunk.has_value())
      return chunk;

  return std::nullopt; // 没找到base 返回nullptr
}

void PalantirIndex::AddFeature(const Feature &feat, chunk_id id) {
  //const auto &features = std::get<std::vector<std::vector<uint64_t>>>(feat);
  const auto &features = std::get< std::vector< std::variant <std::vector<uint64_t>, std::vector<float> > > > (feat);
  for (int i = 0; i < features.size()-1; i++) {
    // 遍历，将feature和块ID关联
    //LOG(INFO) << "Add feature to level "<< i;
    levels_[i]->AddFeature(features[i], id);
  }
  levels_[features.size()-2]->AddFeature(features[features.size()-1], id);
}
} // namespace Delta