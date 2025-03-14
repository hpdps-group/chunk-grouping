#pragma once
#include <cstring>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
namespace Delta {
using chunk_id = uint32_t;
class Chunk {
public:
  // 禁用默认构造函数
  Chunk() = delete;
  // 有参构造
  Chunk(uint8_t *buf, bool needs_to_free, int length, chunk_id id)
      : buf_(buf), needs_to_free_(needs_to_free), length_(length), id_(id) {};
  ~Chunk() {
    if (buf_ != nullptr && needs_to_free_)
      delete[] buf_;
  }
  // 从内存创建一个chunk对象
  static std::shared_ptr<Chunk> FromMemory(void *start, size_t size, uint32_t id) {
    auto buf = new uint8_t[size]; // 分配内存
    memcpy(buf, start, size); // 拷贝数据到buf中
    return std::make_shared<Chunk>(buf, true, size, id); // 调用构造函数创建chunk 并提供一个指针管理对象的生命周期
  }
  static std::shared_ptr<Chunk> FromFileStream(FILE *fp, size_t size, uint32_t id) {
    auto buf = new uint8_t[size];
    fread(buf, 1, size, fp);
    return std::make_shared<Chunk>(buf, true, size, id);
  }
  std::shared_ptr<Chunk> DeepCopy() {
    return FromMemory(buf_, length_, id_);
  }
  int len() const { return length_; }
  uint8_t *buf() const { return buf_; }
  chunk_id id() const { return id_; }
  /*
   * look carefully to the life cycle of the buffer,
   * this function only record the address and length of the chunk content,
   * if you want to make a copy,
   * use FromMemory instead.
   */
  static std::shared_ptr<Chunk> FromMemoryRef(void *start, size_t size, uint32_t id) {
    return std::make_shared<Chunk>((uint8_t*)start, false, size, id); // 直接引用内存，不拷贝
  }
  // static std::shared_ptr<Chunk> FromFile(std::string file_name) {
  //     std::ifstream in(file_name, std::ios::binary);
  //     auto buf = new uint8_t[in.size];
  //     in.read(buf, in.size);
  //     return std::make_shared<Chunk>(buf);
  // }
  // static std::shared_ptr<Chunk> FromFile(std::string file_name, size_t
  // start_pos, size_t size) {
  //     std::ifstream in(file_name, std::ios::binary);
  //     auto buf = new uint8_t[in.size];
  //     in.seek(start_pos, std::ios::beg);
  //     in.read(buf, in.size);
  //     return std::make_shared<Chunk>(buf);
  // }

private:
  uint8_t *buf_ = nullptr;
  bool needs_to_free_ = false; // 是否free
  int length_;
  chunk_id id_;  // chunk id
};
} // namespace Delta