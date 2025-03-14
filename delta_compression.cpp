#include "delta_compression.h"
#include "chunk/chunk.h"
#include "chunk/fast_cdc.h"
#include "chunk/rabin_cdc.h"
#include "config.h"
#include "encoder/xdelta.h"
#include "index/best_fit_index.h"
#include "index/palantir_index.h"
#include "index/super_feature_index.h"
#include "storage/storage.h"
#include <glog/logging.h>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <lz4.h>
#include <lz4frame.h>
#include <lz4frame_static.h>
#include <bitset>
#include <chrono>
namespace Delta {
void DeltaCompression::AddFile(const std::string &file_name) {
  FileMeta file_meta;
  file_meta.file_name = file_name;
  file_meta.start_chunk_id = -1;
  this->chunker_->ReinitWithFile(file_name);  // 初始化chunker
  while (true) {
    auto chunk = chunker_->GetNextChunk(); // fastCDC取一个数据块
    if (nullptr == chunk) // 没数据块就break
      break;
    if (-1 == file_meta.start_chunk_id)  // 如果是第一个块
      file_meta.start_chunk_id = chunk->id();  // 起始块id

    // total_size_origin_ += chunk->len();
    // deduplication
    /* ------- Bug_TODO deduplication开关之后，after的size没变 ------- */
    /*uint32_t dedup_base_id = dedup_->ProcessChunk(chunk);
    total_size_origin_ += chunk->len();
    // duplicate chunk //
    if (dedup_base_id != chunk->id()) {  // 存在重复的块
      storage_->WriteDuplicateChunk(chunk, dedup_base_id);
      duplicate_chunk_count_++;
      continue;
    }*/

    // single compression
    /*size_t single_src_size = chunk->len();
    char* single_src_data = new char[single_src_size];
    memcpy(single_src_data, chunk->buf(), single_src_size);
    size_t single_max_dst_size = LZ4_compressBound(single_src_size);
    char* single_compressed_data = new char[single_max_dst_size];
    size_t single_compressed_data_size = LZ4_compress_default(single_src_data,single_compressed_data,single_src_size,single_max_dst_size);
    LOG(INFO) << "src size: " << single_src_size << ", compressed size: " << single_compressed_data_size << "ratio: " << (double)single_src_size / single_compressed_data_size;
    test_single_compressed_size_ += single_compressed_data_size;

    delete[] single_compressed_data;
    delete[] single_src_data;*/
    
    // delta
    // 写base block并计数
    auto write_base_chunk = [this](const std::shared_ptr<Chunk> &chunk) {
      // 将当前块作为base块写入文件
      storage_->WriteBaseChunk(chunk);
      base_chunk_count_++;
      total_size_origin_ += chunk->len();
      //total_size_compressed_ += chunk->len();

      // 压缩每个base块
      /*
      int src_size = chunk->len();
      const int max_dst_size = LZ4_compressBound(src_size);
      char* compressed_data = new char[max_dst_size];
      const size_t compressed_data_size = LZ4_compress_default((const char*)chunk->buf(),compressed_data,src_size,max_dst_size);
      total_size_compressed_ += compressed_data_size;
      std::cout << " lz4 compress base block:" << "src_size: " << src_size  << " compressed_data_size: " << compressed_data_size << '\n';
      */
    };

    // 写delta块并计数
    auto write_delta_chunk = [this](const std::shared_ptr<Chunk> &chunk,
                                    const std::shared_ptr<Chunk> &delta_chunk,
                                    const uint32_t base_chunk_id) {
      chunk_size_before_delta_ += chunk->len(); // 统计压缩前数据块大小
      storage_->WriteDeltaChunk(delta_chunk, base_chunk_id); // 写入
      delta_chunk_count_++; // delta块计数
      chunk_size_after_delta_ += delta_chunk->len(); // 统计压缩后数据块大小
      //total_size_compressed_ += delta_chunk->len(); // 数据总大小
    };

    // 计算feature
    // 返回feature.cpp里面的result
    auto start = std::chrono::high_resolution_clock::now();

    auto feature = (*feature_)(chunk);  // feature_指向featurecalculator函数，解引用()相当于函数调用
    // 根据feature查找base block
    auto base_chunk_id = index_->GetBaseChunkID(feature);
    if (!base_chunk_id.has_value()) { // 没找到base block
      LOG(INFO) << "No base chunk found for chunk ";
      index_->AddFeature(feature, chunk->id());
      write_base_chunk(chunk); // 将当前块作为base block
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> duration = end - start;
      time += duration.count();
      continue;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    time += duration.count();

    // delta compression
    auto delta_chunk = storage_->GetDeltaEncodedChunk(chunk, base_chunk_id.value()); // 对块进行delta编码
    // 写delta并计数
    write_delta_chunk(chunk, delta_chunk, base_chunk_id.value());
    file_meta.end_chunk_id = chunk->id();
  }
  file_meta_writer_.Write(file_meta);

  // 压缩
  storage_->groupByBase(16, total_size_origin_,total_size_compressed_,grouping_num_,grouping_blocks_, 
                            sequence_compressed_size_, single_compressed_size_, zstd_total_size_compressed_,
                            zstd_sequence_size_compressed_,zstd_single_size_compressed_);  
}

DeltaCompression::~DeltaCompression() {
  auto print_ratio = [](size_t a, size_t b) {
    double ratio = (double)a / (double)b;
    LOG(INFO) << std::fixed << std::setprecision(1) << "(" << ratio * 100 << "%)" << std::defaultfloat;
  };

  LOG(INFO) << "================lz4================";
  LOG(INFO) << "";
  // 单独压每个块
  LOG(INFO) << "====original compression====";
  LOG(INFO) << "before: " << total_size_origin_ << "----> "
            << " after: " << single_compressed_size_;
  double ratio1 = (double)total_size_origin_ / (double)single_compressed_size_;
  LOG(INFO) << "original compression ratio: " << std::fixed << std::setprecision(1) 
            << ratio1 * 100 << "%" << std::defaultfloat;
  //print_ratio(total_size_origin_, single_compressed_size_);
  LOG(INFO) << "";
  // lz4 clustering总压缩比
  LOG(INFO) << "====grouping compression====";
  LOG(INFO) << "before: " << total_size_origin_ << "----> "
            << " after: " << total_size_compressed_;
  double ratio2 = (double)total_size_origin_ / (double)total_size_compressed_;
  LOG(INFO) << "compression ratio after grouping: " << std::fixed << std::setprecision(1) 
            << ratio2 * 100 << "%" << std::defaultfloat;

  LOG(INFO) << "compression improvement: " << (ratio2 / ratio1) << "x";
            
  LOG(INFO) << "";
  LOG(INFO) << "================zstd================";
  LOG(INFO) << "";
  // zstd single
  LOG(INFO) << "====original compression====";
  LOG(INFO) << "before: " << total_size_origin_ << "  ----> "
            << " after: " << single_compressed_size_;
  double ratio3 = (double)total_size_origin_ / (double)zstd_single_size_compressed_;
  LOG(INFO) << "original compression ratio: " << ratio3; 
  LOG(INFO) << "";
  // zstd total
  LOG(INFO) << "====grouping compression====";
  LOG(INFO) << "before: " << total_size_origin_ << "  ----> "
            << " after: " << zstd_total_size_compressed_;
  double ratio4 = (double)total_size_origin_ / (double)zstd_total_size_compressed_;
  LOG(INFO) << "compression ratio after grouping: "  << ratio4;

  LOG(INFO) << "";
  LOG(INFO) << "compression improvement: " << (ratio4 / ratio3) << "x";
  LOG(INFO) << "";
  LOG(INFO) << "grouping time: " << time << "s";
  LOG(INFO) << "thoughput: " << total_size_origin_ /1024 /1024 /1024 / time << "GB/s";
}

#define declare_feature_type(NAME, FEATURE, INDEX)                             \
  {                                                                            \
#NAME, \
[]() -> FeatureIndex { \
  return {std::make_unique<FEATURE>(), \
          std::make_unique<INDEX>()}; \
}                                                                       \
  }

DeltaCompression::DeltaCompression() { // 构造函数
  auto config = Config::Instance().get();  // 获取config
  // 读取配置参数
  auto index_path = *config->get_as<std::string>("index_path");
  auto chunk_data_path = *config->get_as<std::string>("chunk_data_path");
  auto chunk_meta_path = *config->get_as<std::string>("chunk_meta_path");
  auto file_meta_path = *config->get_as<std::string>("file_meta_path");
  auto dedup_index_path = *config->get_as<std::string>("dedup_index_path");
  
  // 分块器
  auto chunker = config->get_table("chunker");
  auto chunker_type = *chunker->get_as<std::string>("type");

  // 两种分块器
  if (chunker_type == "rabin-cdc" || chunker_type == "fast-cdc") {
    auto min_chunk_size = *chunker->get_as<int64_t>("min_chunk_size");
    auto max_chunk_size = *chunker->get_as<int64_t>("max_chunk_size");
    auto stop_mask = *chunker->get_as<int64_t>("stop_mask");
    if (chunker_type == "rabin-cdc") {
      this->chunker_ =
          std::make_unique<RabinCDC>(min_chunk_size, max_chunk_size, stop_mask);
      LOG(INFO) << "Add RabinCDC chunker, min_chunk_size=" << min_chunk_size
                << " max_chunk_size=" << max_chunk_size
                << " stop_mask=" << stop_mask;
    } else if (chunker_type == "fast-cdc") {
      // 构造fastcdc对象的智能指针
      this->chunker_ =
          std::make_unique<FastCDC>(min_chunk_size, max_chunk_size, stop_mask);
      LOG(INFO) << "Add FastCDC chunker, min_chunk_size=" << min_chunk_size
                << " max_chunk_size=" << max_chunk_size
                << " stop_mask=" << stop_mask;
    }
  } else {
    LOG(FATAL) << "Unknown chunker type " << chunker_type;
  }

  // 读取配置参数
  auto feature = config->get_table("feature");
  auto feature_type = *feature->get_as<std::string>("type");
  // feature计算器和index方法
  using FeatureIndex =
      std::pair<std::unique_ptr<FeatureCalculator>, std::unique_ptr<Index>>;
  // 键：feature_type，值：函数指针，返回值是FeatureIndex    
  std::unordered_map<std::string, std::function<FeatureIndex()>>
      feature_index_map = {
          declare_feature_type(finesse, FinesseFeature, SuperFeatureIndex),
          declare_feature_type(odess, OdessFeature, SuperFeatureIndex),
          declare_feature_type(n-transform, NTransformFeature,
                               SuperFeatureIndex),
          declare_feature_type(palantir, PalantirFeature, PalantirIndex),
          declare_feature_type(bestfit, OdessSubfeatures, BestFitIndex)};

  if (!feature_index_map.count(feature_type))
    LOG(FATAL) << "Unknown feature type " << feature_type;
  // feature_ptr获取feature计算器 index_ptr获取index
  auto [feature_ptr, index_ptr] = feature_index_map[feature_type]();
  this->feature_ = std::move(feature_ptr); // feature_ptr拷贝到this->feature_ 类似于引用 获取palantirfeature
  this->index_ = std::move(index_ptr); // index_获取palantirIndex

  this->dedup_ = std::make_unique<Dedup>(dedup_index_path);

  // 获取配置参数
  auto storage = config->get_table("storage");
  auto encoder_name = *storage->get_as<std::string>("encoder");
  auto cache_size = *storage->get_as<int64_t>("cache_size");
  std::unique_ptr<Encoder> encoder;
  if (encoder_name == "xdelta") {
    encoder = std::make_unique<XDelta>();
  } else {
    LOG(FATAL) << "Unknown encoder type " << encoder_name;
  }
  this->storage_ = std::make_unique<Storage>(
      chunk_data_path, chunk_meta_path, std::move(encoder), true, cache_size);
  this->file_meta_writer_.Init(file_meta_path);
}
} // namespace Delta