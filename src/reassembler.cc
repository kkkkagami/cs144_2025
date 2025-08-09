#include "reassembler.hh"
// #include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  // debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  Writer& writeable_output = output_.writer(); // 将output_转换成可写的Writer&类型

  // 1.处理元数据，裁剪传入的数据----------------------------------------------------------------------------------------------------
  // 1.1检查更新流的末位情况
  if ( is_last_substring ) {
    // 如果获取了最后一部分输入的字节,更新final_byte_index_确认末位索引
    final_byte_index_ = first_index + data.size();
    have_last_substring_received_ = true;
  }

  // 1.2 first_index还没有达到'最小需要索引'的情况，需要对data自前方裁剪->进入if
  uint64_t last_index = first_index + data.size(); // 当前收到的data末位的后一位的索引[首个空位的索引]
  if ( first_index < next_expected_index_ ) {
    if ( last_index <= next_expected_index_ ) {
      // data的末位也比next_expected_index_小，那么这段数据已经传送过，函数返回
      return;
    }
    uint64_t overlap_len = next_expected_index_ - first_index;
    data = data.substr( overlap_len ); // data自前方截断
    first_index = next_expected_index_;
  }

  // 1.3 last_index超出了'最大可接受索引' -->截断多余部分
  uint64_t max_acceptable_index
    = writeable_output.bytes_pushed()
      + writeable_output.available_capacity(); // 最大可接受索引[这个索引位置是最大的未output_范围的索引]
  if ( last_index > max_acceptable_index ) {
    if ( first_index > max_acceptable_index ) {
      // data首位已经超出可接受范围，直接返回
      return;
    }
    last_index = max_acceptable_index;
    data = data.substr( 0, last_index - first_index ); // data自后方截断
  }

  // 2.将裁剪后的数据(first_index,data)插入map----------------------------------------------
  // 寻找重叠片段，合并与裁剪

  // 找到第一个可能重叠的现有片段
  auto it = stroed_segments_.upper_bound( first_index );
  if ( it != stroed_segments_.begin() ) {
    --it;
  }

  // 2.1 处理重叠的状况
  while ( it != stroed_segments_.end() && it->first < first_index + data.size() ) {
    uint64_t old_first_index = it->first;
    std::string old_data = it->second;

    if ( old_first_index + old_data.size() > first_index ) {
      // 如果新片段被现有片段包含，直接返回
      if ( old_first_index <= first_index && old_first_index + old_data.size() >= first_index + data.size() ) {
        return;
      }

      // 如果新片段包含现有片段，删除现有片段
      if ( first_index <= old_first_index && old_first_index + old_data.size() <= first_index + data.size() ) {
        it = stroed_segments_.erase( it );
        continue;
      }

      // 如果新片段左侧与已有片段重合：
      if ( old_first_index <= first_index && old_first_index + old_data.size() < first_index + data.size() ) {
        uint64_t old_data_end_index = old_first_index + old_data.size(); // 现有片段终止索引
        data = data.substr( old_data_end_index - first_index );
        it->second += data;

        // 从头重新检查
        first_index = it->first;
        data = it->second;
        it = stroed_segments_.upper_bound( first_index );
        if ( it != stroed_segments_.begin() ) {
          --it;
        }
      }

      // 如果新片段右侧与已有片段重合
      if ( first_index < old_first_index && first_index + data.size() <= old_data.size() + old_first_index ) {
        std::string new_data = data.substr( 0, old_first_index - first_index );
        new_data += it->second;

        it = stroed_segments_.erase( it );
        stroed_segments_[first_index] = new_data;

        data = new_data;
        it = stroed_segments_.upper_bound( first_index );
        if ( it != stroed_segments_.begin() ) {
          --it;
        }
      }
    }
    ++it;
  }

  // 2.2 处理不发生重叠的情况
  stroed_segments_[first_index] = data;

  // 3.检查map中是否有可以送入output_的数据:合并与裁剪--------------------------------------------------------------------------
  while ( true ) {
    // 3.1 检查map中是否存在恰好的片段
    auto it2 = stroed_segments_.find( next_expected_index_ );
    if ( it2 == stroed_segments_.end() ) {
      // 如果it指向end表示没有找到对应的片段，结束在map中的查找
      break;
    }

    // 3.2 到达这一步说明map中存在恰好的片段，由it指向
    std::string segment_it_data = it2->second;                      // it指向数据的副本
    uint64_t segment_it_data_len = segment_it_data.size();          // 当前数据的大小
    uint64_t avaliable_cap = writeable_output.available_capacity(); // 当前可容纳数据量

    // 3.2.1 对map中it指向的片段做裁剪
    if ( segment_it_data_len > writeable_output.available_capacity() ) {
      // 超出output_容纳的范围，裁剪副本
      segment_it_data = segment_it_data.substr( 0, avaliable_cap );

      // 数据推入
      writeable_output.push( segment_it_data );
      next_expected_index_ += segment_it_data.size();

      // 这里需要更新map中it对应的条目
      // erase一个旧的条目，在insert一个新的
      uint64_t segment_insert_first_index = next_expected_index_; // insert段的起始索引
      std::string segment_insert_data = it2->second.substr( segment_it_data.size() );

      stroed_segments_.erase( it2 );

      stroed_segments_[segment_insert_first_index] = segment_insert_data;

      continue;
    }

    // 3.2.2  不需要裁剪
    writeable_output.push( segment_it_data );
    next_expected_index_ += segment_it_data.size();

    // 3.3 删除map中已经送入的片段
    stroed_segments_.erase( it2 );

    // 3.4 检查是否要关闭输入
    if ( next_expected_index_ == final_byte_index_ && have_last_substring_received_ == true ) {
      writeable_output.close();
    }
  }

  // step2.根据当前收到的data的首位索引first_index和长度len_data确定当前收到数据的索引范围-------------------
  // 并根据范围情况预处理data
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  // debug( "unimplemented count_bytes_pending() called" );

  uint64_t bytes_stored {}; // 当前存储在汇编器的字节数
  for ( auto it = stroed_segments_.begin(); it != stroed_segments_.end(); ++it ) {
    // 遍历stored_segments_,将字节逐个数据累加
    bytes_stored += it->second.size();
  }

  return bytes_stored;
}
