#pragma once

#include "byte_stream.hh"
#include <map>

class Reassembler
{
public:
  // Construct Reassembler to write into given ByteStream.
  explicit Reassembler( ByteStream&& output ) : output_( std::move( output ) ) {}

  /*
   * Insert a new substring to be reassembled into a ByteStream.
   *   `first_index`: the index of the first byte of the substring
   *   `data`: the substring itself
   *   `is_last_substring`: this substring represents the end of the stream
   *   `output`: a mutable reference to the Writer
   *
   * The Reassembler's job is to reassemble the indexed substrings (possibly out-of-order
   * and possibly overlapping) back into the original ByteStream. As soon as the Reassembler
   * learns the next byte in the stream, it should write it to the output.
   *
   * If the Reassembler learns about bytes that fit within the stream's available capacity
   * but can't yet be written (because earlier bytes remain unknown), it should store them
   * internally until the gaps are filled in.
   *
   * The Reassembler should discard any bytes that lie beyond the stream's available capacity
   * (i.e., bytes that couldn't be written even if earlier gaps get filled in).
   *
   * The Reassembler should close the stream after writing the last byte.
   */
  void insert( uint64_t first_index, std::string data, bool is_last_substring );

  // How many bytes are stored in the Reassembler itself?
  // This function is for testing only; don't add extra state to support it.
  uint64_t count_bytes_pending() const;

  // Access output stream reader
  Reader& reader() { return output_.reader(); }
  const Reader& reader() const { return output_.reader(); }

  // Access output stream writer, but const-only (can't write from outside)
  const Writer& writer() const { return output_.writer(); }

  // 调用output_的set_error函数
  void set_output_error() { output_.set_error(); }

  // 返回output_下一位希望接收到的索引
  uint64_t get_next_expected_index() const { return next_expected_index_; }

  //返回output_的可用字节数
  uint16_t ava_capacity() const { 
    const uint16_t max16=0xFFFFu;
    return (output_.writer().available_capacity()>max16)? (uint16_t)max16:output_.writer().available_capacity();
  }

  //关闭output_字节流
  void close_output() { output_.writer().close(); }

  //FIN的情况，next_expected_index向后一位
  void add_1_next_expected_index() { ++next_expected_index_; }

private:
  ByteStream output_; // 组合出的原始有序字节流

  std::map<uint64_t, std::string> stroed_segments_ {}; // 使用map存储索引index和对应的data
  uint64_t next_expected_index_ {};                    // output_接下来应该接收到的索引
  uint64_t final_byte_index_ {};                       // insert函数所需要的最后一位输入
  bool have_last_substring_received_ = false;          // 确认是否收到'is_last_substring'信号
};

// close()的时机？接收到结束信号&&所有字节都已经送入output_