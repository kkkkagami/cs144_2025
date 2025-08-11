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
      uint64_t checkpoint=reassembler_.writer().bytes_pushed();
      isn_=message.seqno.unwrap(message.seqno,checkpoint);//据此初始化isn_

      isn_initialnized=true;
    }
  }

  //如果还没有收到syn标志，则结束函数
  if(!isn_initialnized){
    return;
  }

  bool is_last_subs=message.FIN;//标识当前是否是最后一段数据
 
  reassembler_.insert(isn_,message.payload,is_last_subs);//送入reassembler
}

TCPReceiverMessage TCPReceiver::send() const
{
  // Your code here.
  debug( "unimplemented send() called" );
  return {};
}
