#include "feature/features.h"
#include "chunk/chunk.h"
#include "utils/gear.h"
#include "utils/rabin.cpp"
#include <algorithm>
#include <cstdint>
#include <queue>
#include <glog/logging.h>
#include <iomanip>
#include "our_feature.h"
#include "storage/our_feature.cpp"
#include <variant>

namespace Delta {
Feature FinesseFeature::operator()(std::shared_ptr<Chunk> chunk) {
  int sub_chunk_length = chunk->len() / (sf_subf_ * sf_cnt_);
  uint8_t *content = chunk->buf();
  std::vector<uint64_t> sub_features(sf_cnt_ * sf_subf_, 0);
  std::vector<uint64_t> super_features(sf_cnt_, 0);

  // calculate sub features.//
  for (int i = 0; i < sub_features.size(); i++) {
    rabin_t rabin_ctx;
    rabin_init(&rabin_ctx);
    for (int j = 0; j < sub_chunk_length; j++) {
      rabin_append(&rabin_ctx, content[j]);
      sub_features[i] = std::max(rabin_ctx.digest, sub_features[i]);
    }
    content += sub_chunk_length;
  }

  // group the sub features into super features. //
  for (int i = 0; i < sub_features.size(); i += sf_subf_) {
    std::sort(sub_features.begin() + i, sub_features.begin() + i + sf_subf_);
  }
  for (int i = 0; i < sf_cnt_; i++) {
    rabin_t rabin_ctx;
    rabin_init(&rabin_ctx);
    for (int j = 0; j < sf_subf_; j++) {
      auto sub_feature = sub_features[sf_subf_ * i + j];
      auto data_ptr = (uint8_t *)&sub_feature;
      for (int k = 0; k < 8; k++) {
        rabin_append(&rabin_ctx, data_ptr[k]);
      }
    }
    super_features[i] = rabin_ctx.digest;
  }
  return super_features;
}

static uint32_t M[] = {
    0x5b49898a, 0xe4f94e27, 0x95f658b2, 0x8f9c99fc, 0xeba8d4d8, 0xba2c8e92,
    0xa868aeb4, 0xd767df82, 0x843606a4, 0xc1e70129, 0x32d9d1b0, 0xeb91e53c,
};

static uint32_t A[] = {
    0xff4be8c,  0x6f485986, 0x12843ff,  0x5b47dc4d, 0x7faa9b8a, 0xd547b8ba,
    0xf9979921, 0x4f5400da, 0x725f79a9, 0x3c9321ac, 0x32716d,   0x3f5adf5d,
};

Feature NTransformFeature::operator()(std::shared_ptr<Chunk> chunk) {
  int features_num = sf_cnt_ * sf_subf_;
  std::vector<uint32_t> sub_features(features_num, 0);
  std::vector<uint64_t> super_features(sf_cnt_, 0);

  int chunk_length = chunk->len();
  uint8_t *content = chunk->buf();
  uint64_t finger_print = 0;
  // calculate sub features.//
  for (int i = 0; i < chunk_length; i++) {
    finger_print = (finger_print << 1) + GEAR_TABLE[content[i]];
    for (int j = 0; j < features_num; j++) {
      const uint32_t transform = (M[j] * finger_print + A[j]);
      // we need to guarantee that when sub_features[i] is not inited, //
      // always set its value //
      if (sub_features[j] >= transform || 0 == sub_features[j])
        sub_features[j] = transform;
    }
  }

  // group sub features into super features. //
  auto hash_buf = (const uint8_t *const)(sub_features.data());
  for (int i = 0; i < sf_cnt_; i++) {
    uint64_t hash_value = 0;
    auto this_hash_buf = hash_buf + i * sf_subf_ * sizeof(uint32_t);
    for (int j = 0; j < sf_subf_ * sizeof(uint32_t); j++) {
      hash_value = (hash_value << 1) + GEAR_TABLE[this_hash_buf[j]];
    }
    super_features[i] = hash_value;
  }
  return super_features;
}

Feature OdessFeature::operator()(std::shared_ptr<Chunk> chunk) {
  int features_num = sf_cnt_ * sf_subf_;
  std::vector<uint32_t> sub_features(features_num, 0);
  std::vector<uint64_t> super_features(sf_cnt_, 0);

  int chunk_length = chunk->len();
  uint8_t *content = chunk->buf();
  uint64_t finger_print = 0;
  // calculate sub features.//
  for (int i = 0; i < chunk_length; i++) {
    finger_print = (finger_print << 1) + GEAR_TABLE[content[i]];
    if ((finger_print & mask_) == 0) {
      for (int j = 0; j < features_num; j++) {
        const uint32_t transform = (M[j] * finger_print + A[j]);
        // we need to guarantee that when sub_features[i] is not inited,
        // always set its value
        if (sub_features[j] >= transform || 0 == sub_features[j])
          sub_features[j] = transform;
      }
    }
  }

  // group sub features into super features. //
  auto hash_buf = (const uint8_t *const)(sub_features.data());
  for (int i = 0; i < sf_cnt_; i++) {
    uint64_t hash_value = 0;
    auto this_hash_buf = hash_buf + i * sf_subf_ * sizeof(uint32_t);
    for (int j = 0; j < sf_subf_ * sizeof(uint32_t); j++) {
      hash_value = (hash_value << 1) + GEAR_TABLE[this_hash_buf[j]];
    }
    super_features[i] = hash_value;
  }
  return super_features;
}

Feature OdessSubfeatures::operator()(std::shared_ptr<Chunk> chunk) {
  int mask_ = default_odess_mask;
  int features_num = 12; // feature数量
  // 初始化sub_feature为feature_num的vector，每个subfeature是unint64_t
  std::vector<uint64_t> sub_features(features_num, 0); 

  int chunk_length = chunk->len(); // 获取块长度
  uint8_t *content = chunk->buf(); // 获取块内容
  uint32_t finger_print = 0;
  // calculate sub features. //
  for (int i = 0; i < chunk_length; i++) {
    finger_print = (finger_print << 1) + GEAR_TABLE[content[i]]; // 根据内容计算hash值
    if ((finger_print & mask_) == 0) {
      for (int j = 0; j < features_num; j++) {
        const uint64_t transform = (M[j] * finger_print + A[j]); // transform变换
        // we need to guarantee that when sub_features[i] is not inited, //
        // always set its value //
        if (sub_features[j] >= transform || 0 == sub_features[j])
          sub_features[j] = transform;
      }
    }
  }

  return sub_features; // 子特征 也就是用来计算sf的12个feature
}

//std::ofstream super_feature_file("super_feature.csv", std::ios::out);
// 根据sub_features来计算各层的sf
Feature PalantirFeature::operator()(std::shared_ptr<Chunk> chunk) 
{
  // get_sub_features_是OdessSubfeatures对象，调用重载()来计算odess subfeature
  auto sub_features = std::get<std::vector<uint64_t>>(get_sub_features_(chunk));
  std::vector< std::variant< std::vector<uint64_t>, std::vector<float> > > results;
  std::vector<float> ourfeats; 

  // sf_cnt:super feature数量 sf_subf:每个super feature包含的子feature数量
  auto group = [&](int sf_cnt, int sf_subf) -> std::vector<uint64_t> {
    std::vector<uint64_t> super_features(sf_cnt, 0);  // 大小为sf_cnt 初始化为0的vector
    auto hash_buf = (const uint8_t *const)(sub_features.data());  // uint64->uint8 转换为字节流
    for (int i = 0; i < sf_cnt; i++) { // 遍历每组
      uint64_t hash_value = 0;
      auto this_hash_buf = hash_buf + i * sf_subf * sizeof(uint64_t); // 计算当前组的起始位置
      for (int j = 4; j < sf_subf * sizeof(uint64_t); j++) { // 通过一组内的feature计算hash_value
        // +(hash_value << 1)增强位置敏感性和数据分散性
        hash_value = (hash_value << 1) + GEAR_TABLE[this_hash_buf[j]]; // gear_table预定义哈希加速表 对每个字节计算hash值
      }
      super_features[i] = hash_value; // i个sf
    }
    return super_features;
  };
  std::vector<unsigned char> content(chunk->buf(), chunk->buf() + chunk->len());
  results.push_back(group(3, 4));
  results.push_back(group(4, 3));
  results.push_back(group(6, 2));
  ourfeats.push_back(cal(ZERO_CAL, content));
  //ourfeats.push_back(cal(SKEWNESS, content));
  results.push_back(ourfeats);
  
  return results;
}
} // namespace Delta