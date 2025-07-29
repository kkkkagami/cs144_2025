#include "reassembler.hh"
//#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  //当前字节的长度
  uint64_t len_data=data.size();

  //情况1.期望接收到的索引号==当前传入data的索引号
  if(first_index==next_expected_index_){
    Writer& writeable_output=output_.writer();//将output_转换成可写的Writer&类型
    
    uint64_t avaliable_len=writeable_output.available_capacity();
    if(len_data>avaliable_len){
      //期望写入的数据超出了可用容量：只写入容量范围内的数据，其余丢弃
      writeable_output.push(data.substr(0,avaliable_len));
    }
    else{
      //期望写入的数据没有超出可用容量范围：全部写入
      writeable_output.push(data);
    }
    next_expected_index_+=len_data;

    //检查stroed_segments_中是否有可以直接送入output_的数据
  }

  //情况2.期望接收到的索引号>当前传入data的索引号
  //需要将数据送入stroed_segments_中
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  //debug( "unimplemented count_bytes_pending() called" );
  return {};
}
