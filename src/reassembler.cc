#include "reassembler.hh"
//#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  
  Writer& writeable_output=output_.writer();//将output_转换成可写的Writer&类型

  //step0.确认is_last_substring的情况
  if(is_last_substring){
    //如果获取了最后一部分输入的字节,更新final_byte_index_确认末位索引
    final_byte_index_=first_index+data.size();
    have_last_substring_received_=true;
  }

  if (have_last_substring_received_ && data.empty() && first_index == next_expected_index_) {
    writeable_output.close();
    return;
  }


  //step1.检查map中是否有可以送入output_的数据----------------------------------------------------------
  while(true){
    //1.1 检查map中是否存在恰好的片段
    auto it=stroed_segments_.find(next_expected_index_);
    if(it==stroed_segments_.end()){
      //如果it指向end表示没有找到对应的片段，结束在map中的查找
      break;
    }

    //1.2 到达这一步说明map中存在恰好的片段，由it指向
    std::string segment_it_data=it->second;//it指向数据的副本
    uint64_t segment_it_data_len=segment_it_data.size();//当前数据的大小
    uint64_t avaliable_cap=writeable_output.available_capacity();//当前可容纳数据量

    //1.2.1 对map中it指向的片段做裁剪
    if(segment_it_data_len>writeable_output.available_capacity()){
      //超出output_容纳的范围，裁剪副本
      segment_it_data=segment_it_data.substr(0,avaliable_cap);

      //数据推入
      writeable_output.push(segment_it_data);
      next_expected_index_+=segment_it_data.size();

      //这里需要更新map中it对应的条目
      //erase一个旧的条目，在insert一个新的
      uint64_t segment_insert_first_index=next_expected_index_;//insert段的起始索引
      std::string segment_insert_data=it->second.substr(segment_it_data.size());

      stroed_segments_.erase(it);

      stroed_segments_[segment_insert_first_index]=segment_insert_data;

      break;
    }

    //1.2.2  不需要裁剪
    writeable_output.push(segment_it_data);
    next_expected_index_+=segment_it_data.size();

    //1.3 删除map中已经送入的片段
    stroed_segments_.erase(it);

    //1.4 检查是否要关闭输入
    if(next_expected_index_==final_byte_index_&&have_last_substring_received_==true){
      writeable_output.close();
    }
  }

  //step2.根据当前收到的data的首位索引first_index和长度len_data确定当前收到数据的索引范围-----------------
  //并根据范围情况预处理data
  uint64_t last_index=first_index+data.size()-1;//当前收到的data的最后一位的索引

  //2.1 last_index超出了'最大可接受索引' -->直接截断多余部分
  //最大可接受索引[这个索引位置是最大的未output_范围的索引]
  uint64_t max_acceptable_index=writeable_output.bytes_pushed()+writeable_output.available_capacity();
  if(last_index>max_acceptable_index) { 
    if(first_index>max_acceptable_index){
      //data首位已经超出可接受范围，直接返回
      return;
    }
    last_index=max_acceptable_index;
    data=data.substr(0,last_index-first_index+1);//data自后方截断 
  }

  //2.2 first_index还没有达到'最小需要索引'
  if(first_index<next_expected_index_) { 
    if(last_index<next_expected_index_) { 
      //data的末位也比next_expected_index_小，那么这段数据已经传送过，函数返回
      return; 
    }
    uint64_t overlap_len=next_expected_index_-first_index;//截断长度
    first_index=next_expected_index_;
    data=data.substr(overlap_len);//data自前方截断
  }

  
  //step3.first_index与next_expected_index_是什么样的关系？----------------------------------------------
  //3.1 如果相等，那么进行下面的操作：直接将data送入output_(用writeable_output的形式)
  if(first_index==next_expected_index_){
    writeable_output.push(data);

    if(next_expected_index_==final_byte_index_&&have_last_substring_received_==true){
      writeable_output.close();
    }
  }

  //3.2 如果不相等，那只能是first_index>next_expected_index_ [小于的情况在step1已经清除]
  //那么需要将这段data放入map中暂存
  stroed_segments_[first_index]=data;

  //step4.检查是否是最后一位传入的数据，是则关闭输入
  //if(is_last_substring)  writeable_output.close();
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  //debug( "unimplemented count_bytes_pending() called" );

  uint64_t bytes_stored{};//当前存储在汇编器的字节数
  for(auto it=stroed_segments_.begin();it!=stroed_segments_.end();++it){
    //遍历stored_segments_,将字节逐个数据累加
    bytes_stored+=it->second.size();
  }

  return bytes_stored;
}
