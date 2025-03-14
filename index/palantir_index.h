#include "feature/features.h"
#include "index/index.h"
#include "index/super_feature_index.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
namespace Delta {

using chunk_id = uint32_t;
class Chunk;
class PalantirIndex : public Index {
public:
  PalantirIndex() {
    levels_.push_back(new SuperFeatureIndex(3,1)); // 层1，三个sf
    levels_.push_back(new SuperFeatureIndex(4,1)); // 层2
    levels_.push_back(new SuperFeatureIndex(6,1)); // 层3
    //levels_.push_back(new SuperFeatureIndex(8));
    // levels_.push_back(new SuperFeatureIndex(12)); // 层4
  }
  ~PalantirIndex() {
    for (auto level: levels_)
        delete level;
  }
  std::optional<chunk_id> GetBaseChunkID(const Feature &feat);
  void AddFeature(const Feature &feat, chunk_id id);
  bool RecoverFromFile(const std::string &path) { return true;}
  bool DumpToFile(const std::string &path) { return true;}

private:
  std::vector<SuperFeatureIndex*> levels_;
};
} // namespace Delta