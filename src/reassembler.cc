#include "reassembler.hh"
//#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  
  Writer& writeable_output=output_.writer();//将output_转换成可写的Writer&类型

  //step0.确认is_last_substring的情况
  uint64_t final_byte_index{};//insert函数所需要的最后一位输入
  if(is_last_substring){
    //如果获取了最后一部分输入的字节
    final_byte_index=first_index+data.size()-1;
  }

  //step1.检查map中是否有可以送入output_的数据
  //搜索map中索引index小于next_expected_index_的最大值
  for(auto it=stroed_segments_.begin();it!=stroed_segments_.end();++it){
    uint64_t map_data_end_index=it->first+it->second.size()-1;//当前map数据末位元素索引
    if(map_data_end_index>=next_expected_index_&&it->first<=next_expected_index_){
      //当前数据可以送入output_
      std::string map_data=it->second.substr(next_expected_index_-it->first);//截断前面的多余部分
      writeable_output.push(map_data);//写入完成
      next_expected_index_=map_data_end_index+1;//更新next_expected_index_

      //检查是否结束输入
      if(next_expected_index_>=final_byte_index){
        writeable_output.close();
      }
    }
  }

  //step2.根据当前收到的data的首位索引first_index和长度len_data确定当前收到数据的索引范围
  //并根据范围情况预处理data
  uint64_t last_index=first_index+data.size()-1;//当前收到的data的最后一位的索引

  //2.1 last_index超出了'最大可接受索引' -->直接截断多余部分
  //最大可接受索引[这个索引位置是最大的未output_范围的索引]
  uint64_t max_acceptable_index=writeable_output.bytes_pushed()+writeable_output.available_capacity()-1;
  if(last_index>=max_acceptable_index) { 
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
    first_index=next_expected_index_;
    data=data.substr(next_expected_index_);//data自前方截断
  }

  
  //step3.first_index与next_expected_index_是什么样的关系？
  //3.1 如果相等，那么进行下面的操作：直接将data送入output_(用writeable_output的形式)
  if(first_index==next_expected_index_){
    writeable_output.push(data);

    if(next_expected_index_>=final_byte_index){
      writeable_output.close();
    }

    return;//至此结束这种情况的操作，函数返回
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
