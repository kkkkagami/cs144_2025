#include "reassembler.hh"
//#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  //debug( "unimplemented insert({}, {}, {}) called", first_index, data, is_last_substring );
  
  Writer& writeable_output=output_.writer();//将output_转换成可写的Writer&类型

  //step1.根据当前收到的data的首位索引first_index和长度len_data确定当前收到数据的索引范围
  //并根据范围情况预处理data
  uint64_t last_index=first_index+data.size()-1;//当前收到的data的最后一位的索引

  //1.1 last_index超出了'最大可接受索引' -->直接截断多余部分
  //最大可接受索引[这个索引位置是最大的未output_范围的索引]
  uint64_t max_acceptable_index=writeable_output.bytes_pushed()+writeable_output.available_capacity()-1;
  if(last_index>=max_acceptable_index) { 
    if(first_index>max_acceptable_index){
      //data首位已经超出可接受范围
      return;
    }
    last_index=max_acceptable_index;
    data=data.substr(0,last_index-first_index+1);//data自后方截断 
  }

  //1.2 first_index还没有达到'最小需要索引'
  if(first_index<next_expected_index_) { 
    if(last_index<next_expected_index_) { 
      //data的末位也比next_expected_index_小，那么这段数据已经传送过，函数返回
      return; 
    }
    first_index=next_expected_index_;
    data=data.substr(next_expected_index_);//data自前方截断
  }

  //step2.first_index与next_expected_index_是什么样的关系？
  //2.1 如果相等，那么进行下面的操作：直接将data送入output_(用writeable_output的形式)
  if(first_index==next_expected_index_){
    writeable_output.push(data);
    return;//至此结束这种情况的操作，函数返回
  }

  //2.2 如果不相等，那只能是first_index>next_expected_index_ [小于的情况在step1已经清除]
  //那么需要将这段data放入map中暂存

  //Q:什么地方检查是否可以从map中取出数据？怎么检查？
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  //debug( "unimplemented count_bytes_pending() called" );
  return {};
}
