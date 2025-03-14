#pragma once
#include "3party/xdelta3/xdelta3.h"
#include "chunk/chunk.h"
#include "encoder/encoder.h"
#include <glog/logging.h>
namespace Delta {
class XDelta : public Encoder { // XDelata: 差分编码器，用于计算两个块之间的delta
public:
  std::shared_ptr<Chunk> encode(std::shared_ptr<Chunk> base_chunk,
                                std::shared_ptr<Chunk> new_chunk) override {
    long unsigned int delta_chunk_size = 0;
    // 调用xd3进行编码
    // - new_chunk->buf()：新块的数据缓冲区
    // - new_chunk->len()：新块的数据长度
    // - base_chunk->buf()：基础块的数据缓冲区
    // - base_chunk->len()：基础块的数据长度
    // - chunk_buffer_：用于存储编码结果的缓冲区
    // - &delta_chunk_size：返回 delta 块的大小
    // - 32768：缓冲区大小
    // - 0：标志位（无特殊设置）
    int result = xd3_encode_memory(new_chunk->buf(), new_chunk->len(),
                                   base_chunk->buf(), base_chunk->len(),
                                   chunk_buffer_, &delta_chunk_size, 32768, 0);
    if (0 != result) {
      LOG(FATAL) << "some wrong happens in XDelta encode, returns " << result << " delta chunk size " << delta_chunk_size;
    }
    LOG(INFO) << "delta chunk size is " << delta_chunk_size;
    // 基于 chunk_buffer_ 创建新的 Chunk 对象并返回
    return Chunk::FromMemoryRef(chunk_buffer_, delta_chunk_size,
                                new_chunk->id());
  }
  std::shared_ptr<Chunk> decode(std::shared_ptr<Chunk> base,
                                std::shared_ptr<Chunk> delta) override { // 解码
    long unsigned int decode_chunk_size = 0;
    // 调用xd3进行解码
    if (0 != xd3_decode_memory(delta->buf(), delta->len(), base->buf(),
                               base->len(), chunk_buffer_, &decode_chunk_size,
                               32768, 0)) {
      LOG(FATAL) << "some wrong happens in XDelta decode, returns ";
      return nullptr;
    }
    LOG(INFO) << "decoded chunk size is " << decode_chunk_size;
    return Chunk::FromMemoryRef(chunk_buffer_, decode_chunk_size, delta->id());
  }

private:
  uint8_t chunk_buffer_[32768];
};
} // namespace Delta