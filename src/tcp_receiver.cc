#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  //发生错误，将连接终止
  if ( message.RST ) { reassembler_.set_output_error(); }

  //当前收到初始序列号
  if(message.SYN ) {
    //isn_如果还未被初始化，则进入以下初始化过程
    if(!isn_initialnized){
      //checkpoint作为unwrap的参数，是当前reassembler已经处理掉的字节数
      //uint64_t checkpoint=reassembler_.writer().bytes_pushed();
      isn_=message.seqno;

      isn_initialnized=true;
    }
  }

  //如果还没有收到syn标志，则结束函数
  if(!isn_initialnized){
    return;
  }

  bool is_last_subs=message.FIN;//标识当前是否是最后一段数据
 
  uint64_t abs_seqno=message.seqno.unwrap(isn_,reassembler_.writer().bytes_pushed());
  uint64_t stream_index=abs_seqno-1;
  reassembler_.insert(stream_index,message.payload,is_last_subs);//送入reassembler

  //确认是当前的最后一段数据
  if(is_last_subs){
    reassembler_.close_output();//关闭字节流
    reassembler_.add_1_next_expected_index();
  }
}

//去除了该函数的const
TCPReceiverMessage TCPReceiver::send() const 
{
  TCPReceiverMessage send_message{}; 
  
  //未接收到初始序列号，ack字段记为空
  if(!isn_initialnized) { 
    send_message.ackno=std::nullopt; 
  }else{
    //接收到，则ack记为所需的下一个序号
    uint64_t abs_ackno=reassembler_.writer().bytes_pushed()+1;//+syn
    if(reassembler_.writer().is_closed()){
      ++abs_ackno;
    }
    send_message.ackno=Wrap32::wrap(abs_ackno,isn_);
  }

  //确认window_size未tcp可以接受的容量
  send_message.window_size=reassembler_.ava_capacity();

  send_message.RST=reassembler_.writer().has_error();
  return send_message;
}