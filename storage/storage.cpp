#include "storage/storage.h"
#include "delta_compression.h"
#include "chunk/chunk.h"
#include <cstdlib>
#include <glog/logging.h>
#include <bitset>
#include <assert.h>
#include <ios>
#include <iomanip>
//lz4
#include <lz4.h>
#include <lz4frame.h>
#include <lz4frame_static.h>
//zstd
#include <zstd.h>

#include "our_feature.h"
namespace Delta {
Storage::Storage(std::string DataPath, std::string MetaPath,
                 std::unique_ptr<Encoder> encoder, bool compress_mode,
                 size_t cache_size)
    : cache_(cache_size) {
  this->encoder_ = std::move(encoder); // 初始化编码器
  char fopen_flag[3] = {'r', '+', '\0'};
  if (compress_mode) {
    fopen_flag[0] = 'w'; // 压缩模式，模式改为写
  }
  // 打开文件
  data_ = fopen(DataPath.c_str(), fopen_flag);
  meta_ = fopen(MetaPath.c_str(), fopen_flag);
  if (!data_ || !meta_) {
    LOG(FATAL) << "Fail to open " << DataPath << " or " << MetaPath;
  }
  // 将文件指针移动到文件末尾，准备追加写入
  fseek(data_, 0, SEEK_END);
  fseek(meta_, 0, SEEK_END);
}

void Storage::WriteBaseChunk(std::shared_ptr<Chunk> chunk) {
  ChunkMeta meta;
  meta.offset = ftell(data_); // 获取当前文件指针的位置，即base block的偏移量
  meta.base_chunk_id = chunk->id(); // 设置base id为自己的id
  meta.size = chunk->len(); // 设置大小
  meta.type = BaseChunk; // 标记为基础块

  // 写入chunk数据到data_指向的文件
  fwrite(chunk->buf(), 1, chunk->len(), data_);
  // 写入chunk元数据到meta_指向的文件
  fwrite(&meta, sizeof(ChunkMeta), 1, meta_);
  LOG(INFO) << "write base chunk " << chunk->id() << ", len " << chunk->len();
}

std::shared_ptr<Chunk>
Storage::GetDeltaEncodedChunk(std::shared_ptr<Chunk> chunk,
                              chunk_id base_chunk_id) {
  auto base_chunk = cache_.get(base_chunk_id); // 尝试从cache中获取base block
  if (nullptr == base_chunk) { // 不在cache中，则从存储中获取
    base_chunk = GetChunkContent(base_chunk_id); // 从存储中获取base块的内容
    cache_.add(base_chunk_id, base_chunk);
  }
  // 根据base_chunk和chunk生成delta_chunk（delta编码）
  //auto delta_chunk = encoder_->encode(base_chunk, chunk);
  // 不编码，直接返回
  auto delta_chunk = chunk;
  return delta_chunk;
}

int Storage::WriteDeltaChunk(std::shared_ptr<Chunk> delta_chunk,
                             chunk_id base_chunk_id) {
  ChunkMeta meta;
  meta.offset = ftell(data_); // 当前写入位置
  meta.base_chunk_id = base_chunk_id; // 设置base block id
  meta.size = delta_chunk->len();  // 设置大小
  meta.type = DeltaChunk; // 标记为delta块

  fwrite(delta_chunk->buf(), 1, delta_chunk->len(), data_);
  fwrite(&meta, sizeof(ChunkMeta), 1, meta_);
  LOG(INFO) << "write delta chunk " << delta_chunk->id() << ", len "
            << delta_chunk->len();
  return delta_chunk->len();
}

// 从<存储>加载块内容（可以是base块也可以是delta块）
// 如果是base块，直接返回块内容。如果是delta块，找到对应的base块进行解码再返回内容。
std::shared_ptr<Chunk> Storage::GetChunkContent(chunk_id id) {
  // TODO: support recover data content of delta chunk
  uint64_t meta_offset = id * sizeof(ChunkMeta);
  fseek(meta_, meta_offset, SEEK_SET);
  ChunkMeta meta;
  fread(&meta, sizeof(ChunkMeta), 1, meta_);
  fseek(data_, meta.offset, SEEK_SET);
  auto raw_content = Chunk::FromFileStream(data_, meta.size, id);
  fseek(meta_, 0, SEEK_END);
  fseek(data_, 0, SEEK_END);
  /*if (meta.type == DeltaChunk) { // 如果是delta块
    auto base_chunk = cache_.get(meta.base_chunk_id);
    if (nullptr == base_chunk) {
      base_chunk = GetChunkContent(meta.base_chunk_id); // 递归查找base block
      cache_.add(meta.base_chunk_id, base_chunk);
    }
    return encoder_->decode(base_chunk, raw_content); // 解码delta块
  }*/
  // 如果没有进行delta 直接返回
  return raw_content;
}

void Storage::WriteDuplicateChunk(std::shared_ptr<Chunk> chunk,
                                  chunk_id base_chunk_id) {
  ChunkMeta meta;
  meta.base_chunk_id = base_chunk_id;
  meta.size = chunk->len();
  meta.type = DuplicateChunk;
  fseek(meta_, base_chunk_id & sizeof(ChunkMeta), SEEK_SET);
  ChunkMeta base_meta;
  fread(&base_meta, sizeof(ChunkMeta), 1, meta_);
  meta.offset = base_meta.offset;
  fseek(meta_, 0, SEEK_END);
  fwrite(&meta, sizeof(ChunkMeta), 1, meta_);
  LOG(INFO) << "write duplicate chunk " << chunk->id() << ", base id "
            << base_chunk_id;
}

std::map<chunk_id, std::vector<std::shared_ptr<Chunk>>> Storage::dividByBase(
                                size_t& total_size_compressed_, 
                                size_t& sequence_compressed_size_, 
                                size_t& single_compressed_size_,
                                size_t& zstd_total_size_compressed_,
                                size_t& zstd_sequence_size_compressed_,
                                size_t& zstd_single_size_compressed_
                                ) 
{
  std::map<chunk_id, std::vector<std::shared_ptr<Chunk>>> grouped_chunks; // 用于存储分组结果
  fseek(meta_, 0, SEEK_SET); // 定位到元数据文件的起始位置
  if (!meta_) 
  {
    LOG(INFO) << "meta file is not open.";
    assert(0);
  }
  ChunkMeta meta;

  std::vector<std::shared_ptr<Chunk>> chunks; // for sequence compression
  std::vector<std::shared_ptr<Chunk>> test_groups; // for test cluster

  // 遍历元数据文件
  //chunk_id current_id = 0; // for delta encode
  while (fread(&meta, sizeof(ChunkMeta), 1, meta_) == 1) 
  {
    // 定位到当前位置
    fseek(data_, meta.offset, SEEK_SET);
    if (!meta_) 
    {
    LOG(INFO) << "Data file is not open.";
    assert(0);
    }
    // 计算当前块的id
    chunk_id current_id = meta.offset / 8192;  // for no delta encode
    
    // single compress
    LOG(INFO) << "========single compression=========";
    LOG(INFO) << "current_id: " << current_id << " base: " << meta.base_chunk_id;

    // 从文件读入块内容 也可以（GetChunkContent）
    auto chunk = Chunk::FromFileStream(data_, meta.size, current_id);
    LOG(INFO) << "offset: " << meta.offset << ",base_chunk_id: " << meta.base_chunk_id << ", type: " << (char)meta.type;

    // lz4 single
    LZ4F_preferences_t kPrefs0 = {};
    kPrefs0.frameInfo.blockMode = LZ4F_blockLinked;
    kPrefs0.frameInfo.blockSizeID = LZ4F_max256KB;
    size_t single_src_size = chunk->len();
    char* single_src_data = new char[single_src_size];
    memcpy(single_src_data, chunk->buf(), single_src_size);
    size_t single_max_dst_size = LZ4F_compressFrameBound(single_src_size,&kPrefs0);
    char* single_compressed_data = new char[single_max_dst_size];
    size_t single_compressed_data_size = LZ4F_compressFrame(single_compressed_data,single_max_dst_size,single_src_data,single_src_size,&kPrefs0);
    //LOG(INFO) << "src size: " << single_src_size << ", compressed size: " << single_compressed_data_size << ",ratio: " << (double)single_src_size / single_compressed_data_size;
    
    // zstd single
    size_t zstd_single_max_dst_size = ZSTD_compressBound(single_src_size);
    char* zstd_single_compressed_data = new char[zstd_single_max_dst_size];
    size_t zstd_single_compressed_data_size = ZSTD_compress(zstd_single_compressed_data, zstd_single_max_dst_size, single_src_data, single_src_size, 10);
    //LOG(INFO) << "zstd src size: " << single_src_size << ", zstd compressed size: " << zstd_single_compressed_data_size << ",zstd ratio: " << (double)single_src_size / zstd_single_compressed_data_size;

    // 记录 块ID | ratio
    //single_file << chunk->id() << "," << meta.base_chunk_id << "," << (double)single_src_size / single_compressed_data_size << "," << std::endl;

    /* 累加单独压缩后长度*/
    // lz4
    single_compressed_size_ += single_compressed_data_size;
    // zstd
    zstd_single_size_compressed_ += zstd_single_compressed_data_size;

    delete[] single_compressed_data;
    delete[] single_src_data;
    delete[] zstd_single_compressed_data;
    
    chunks.push_back(chunk);
    LZ4F_compressionContext_t ctx;
    LZ4F_errorCode_t errorCode = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
    if (LZ4F_isError(errorCode)) 
    {
      LOG(INFO) << ("Failed to create LZ4 context: %s\n", LZ4F_getErrorName(errorCode));
    }
    LZ4F_preferences_t kPrefs = {};
    kPrefs.frameInfo.blockMode = LZ4F_blockLinked;
    kPrefs.frameInfo.blockSizeID = LZ4F_max256KB;

    if(meta.type == BaseChunk)
    {
      grouped_chunks[meta.base_chunk_id];
    }
    if(meta.type == DeltaChunk) // 仅处理delta块
    { 
      fseek(data_, meta.offset, SEEK_SET); // 定位到数据文件中的对应位置
      auto delta_chunk = Chunk::FromFileStream(data_, meta.size, current_id); // 从文件中加载delta块

      // 将delta块按base_chunk_id分组
      //LOG(INFO) << "group by base chunk id: " << meta.base_chunk_id << "delta chunk id: " << delta_chunk->id();
      grouped_chunks[meta.base_chunk_id].push_back(delta_chunk);
    }
    current_id++;
  }

  fseek(meta_, 0, SEEK_END); // 恢复元数据文件指针位置
  fseek(data_, 0, SEEK_END); // 恢复数据文件指针位置

  return grouped_chunks;
}
void Storage::groupByBase(size_t group_size,
                          size_t& total_size_origin_,
                          size_t& total_size_compressed_, 
                          size_t& grouping_num_,
                          size_t& grouping_blocks_, 
                          size_t& sequence_compressed_size_, 
                          size_t& single_compressed_size_,
                          size_t& zstd_total_size_compressed_,
                          size_t& zstd_sequence_size_compressed_,
                          size_t& zstd_single_size_compressed_) 
{
  // 获取所有按base块分组的delta块
  auto grouped_chunks = dividByBase(total_size_compressed_,sequence_compressed_size_,
                                    single_compressed_size_,zstd_total_size_compressed_,
                                    zstd_sequence_size_compressed_,zstd_single_size_compressed_);

  // 遍历聚合块
  for (const auto &[base_chunk_id, delta_chunks] : grouped_chunks) 
  {
    LOG(INFO) << "------lz4 cluster compression start------";
    LOG(INFO) << "Processing base chunk ID: " << base_chunk_id << " grouped " << delta_chunks.size() << " blocks";
    auto test_base_chunk = GetChunkContent(base_chunk_id);
    std::vector<unsigned char> content(test_base_chunk->buf(), test_base_chunk->buf() + test_base_chunk->len());
    

    // 单独压没有group起来的base块
    if(delta_chunks.empty())
    {
      LOG(INFO) << "base chunk id: " << base_chunk_id << " has no delta chunk";
      auto base_chunk = GetChunkContent(base_chunk_id);
      size_t src_size = base_chunk->len();
      
      // lz4
      char* src_data = new char[src_size];
      LOG(INFO) << "compress not group block";
      // src_data = new char[src_size];
      memcpy(src_data, base_chunk->buf(), base_chunk->len()); // 将base块内容拷贝到src_data
      size_t max_dst_size = LZ4_compressBound(src_size);
      char* compressed_data = new char[max_dst_size];
      size_t compressed_data_size = LZ4_compress_default(src_data, compressed_data, src_size, max_dst_size);
      //LOG(INFO) << "src size: " << src_size << ", compressed size: " << compressed_data_size << "ratio: " << (double)src_size / compressed_data_size;
      //cluster_file << base_chunk_id << "," << delta_chunks.size() << "," << (double)src_size/compressed_data_size << "," << base_chunk_id << '\n';
      total_size_compressed_ += compressed_data_size;

      // zstd
      size_t zstd_max_dst_size = ZSTD_compressBound(src_size);
      char* zstd_compressed_data = new char[zstd_max_dst_size];
      size_t zstd_compressed_data_size = ZSTD_compress(zstd_compressed_data, zstd_max_dst_size, src_data, src_size, 10);
      //LOG(INFO) << "zstd src size: " << src_size << ", zstd compressed size: " << zstd_compressed_data_size << ",zstd ratio: " << (double)src_size / zstd_compressed_data_size;
      zstd_total_size_compressed_ += zstd_compressed_data_size;

      delete[] src_data;
      delete[] compressed_data;
      delete[] zstd_compressed_data;
      continue;
    }
    bool base_comp = false;
    
    // 计算group起来的块的数量
    if(delta_chunks.size() > 1) 
    {
      // group nums
      grouping_num_++;
      // group blocks
      grouping_blocks_ += delta_chunks.size();
    }

    // 按每 group_size（16）个delta块进行操作
    for (size_t i = 0; i < delta_chunks.size(); i += group_size) 
    {
      size_t end = std::min(i + group_size, delta_chunks.size());

      // 提取当前分组的delta块
      std::vector<std::shared_ptr<Chunk>> chunk_group(delta_chunks.begin() + i, delta_chunks.begin() + end);

      // lz4 cluster compression
      // lz4 config
      LZ4F_compressionContext_t ctx;
      LZ4F_errorCode_t errorCode = LZ4F_createCompressionContext(&ctx, LZ4F_VERSION);
      if (LZ4F_isError(errorCode)) 
      {
        LOG(INFO) << ("Failed to create LZ4 context: %s", LZ4F_getErrorName(errorCode));
      }
      LZ4F_preferences_t kPrefs = {};
      kPrefs.frameInfo.blockMode = LZ4F_blockLinked;
      kPrefs.frameInfo.blockSizeID = LZ4F_max256KB;
      size_t src_size = 0;
      size_t group_size = 0;

      // 遍历聚合块，计算size
      for (const auto &chunk : chunk_group) 
      {
        src_size += chunk->len();
        // 计算delta块的size
        total_size_origin_ += chunk->len();
        // TEST 每组的块数
        group_size++;
      }

      // lz4
      char* src_data = new char[src_size];
      char* dest_ptr = src_data;

      // 如果聚合块块数不足16块，base块一起压
      if(group_size < 16)
      {
        // TEST base块一起压flag
        base_comp = true;
        LOG(INFO) << "compress with base block";
        // 获取base块数据内容
        auto base_chunk = GetChunkContent(base_chunk_id);
        src_size += base_chunk->len();
        src_data = new char[src_size];
        dest_ptr = src_data;
        memcpy(src_data, base_chunk->buf(), base_chunk->len()); // 将base块内容拷贝到src_data
        dest_ptr += base_chunk->len();
      }

      // lz4 config
      size_t max_dst_size = LZ4F_compressFrameBound(src_size,&kPrefs);
      char* compressed_data = new char[max_dst_size];
      // zstd config
      size_t zstd_max_dst_size = ZSTD_compressBound(src_size);
      char* zstd_compressed_data = new char[zstd_max_dst_size];

      // 遍历聚合块里的块进行memcpy
      for (const auto &chunk : chunk_group) 
      {
        memcpy(dest_ptr,chunk->buf(),chunk->len());
        dest_ptr += chunk->len();
        std::vector<unsigned char> content(chunk->buf(), chunk->buf() + chunk->len());
      }
      // lz4 compress
      size_t compressed_data_size = LZ4F_compressFrame(compressed_data,max_dst_size,src_data,src_size,&kPrefs);
      //zstd compress
      size_t zstd_compressed_data_size = ZSTD_compress(zstd_compressed_data,zstd_max_dst_size,src_data,src_size,10);

      
      // 累加压缩后的size
      total_size_compressed_ += compressed_data_size;
      zstd_total_size_compressed_ += zstd_compressed_data_size;

      // lz4 config
      LZ4F_freeCompressionContext(ctx);
      delete[] src_data;
      delete[] compressed_data;
      delete[] zstd_compressed_data;
    }

    // 刚好16块倍数的，base不会一起压，循环退出表示处理完这一个聚合块，循环完后最后压个base块
    if(delta_chunks.size() % 16 == 0)
    {
      auto base_chunk = GetChunkContent(base_chunk_id);
      size_t src_size = base_chunk->len();
      char* src_data = new char[src_size];
      LOG(INFO) << "compress base block";
      memcpy(src_data, base_chunk->buf(), base_chunk->len()); // 将base块内容拷贝到src_data
      size_t max_dst_size = LZ4_compressBound(src_size);
      char* compressed_data = new char[max_dst_size];
      size_t compressed_data_size = LZ4_compress_default(src_data, compressed_data, src_size, max_dst_size);
      //cluster_file << "," << base_chunk_id << "," << (double)src_size/compressed_data_size << '\n';
      total_size_compressed_ += compressed_data_size;

      size_t zstd_max_dst_size = ZSTD_compressBound(src_size);
      char* zstd_compressed_data = new char[zstd_max_dst_size];
      size_t zstd_compressed_data_size = ZSTD_compress(zstd_compressed_data,zstd_max_dst_size,src_data,src_size,10);
      zstd_total_size_compressed_ += zstd_compressed_data_size;

      delete[] src_data;
      delete[] compressed_data;
      delete[] zstd_compressed_data;
    }
  }
}
} // namespace Delta