#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  // 发生错误，将连接终止
  if ( message.RST ) {
    reassembler_.set_output_error();
  }

  // 当前收到初始序列号
  if ( message.SYN ) {
    // isn_如果还未被初始化，则进入以下初始化过程
    if ( !isn_initialnized ) {
      // checkpoint作为unwrap的参数，是当前reassembler已经处理掉的字节数
      // uint64_t checkpoint=reassembler_.writer().bytes_pushed();
      isn_ = message.seqno;

      isn_initialnized = true;
    }
  }

  // 如果还没有收到syn标志，则结束函数
  if ( !isn_initialnized ) {
    return;
  }

  bool is_last_subs = message.FIN; // 标识当前是否是最后一段数据

  // 求出数据流起始绝对索引
  uint64_t stream_index = message.seqno.unwrap( isn_, reassembler_.writer().bytes_pushed() );

  // 如果已经收到了syn，那么数据流index要-1
  if ( isn_initialnized && !message.SYN ) {
    --stream_index;
  } else if ( message.SYN ) {
    stream_index = 0;
  }

  // 处理message不包含数据的情况
  if ( !message.payload.empty() ) {
    uint64_t first_unacceptable = reassembler_.writer().bytes_pushed() + reassembler_.ava_capacity();
    if ( stream_index >= first_unacceptable ) {
      return;
    } // 如果传入的message超出reassembler_的范围
    if ( stream_index < reassembler_.writer().bytes_pushed() ) { // 如果传入的message未达到reassembler_的范围
      size_t overlap = reassembler_.writer().bytes_pushed() - stream_index;
      if ( overlap >= message.payload.size() ) {
        return;
      }
      message.payload.erase( 0, overlap );
      stream_index = reassembler_.writer().bytes_pushed();
    }

    reassembler_.insert( stream_index, message.payload, is_last_subs ); // 送入reassembler
  }

  // 确认是当前的最后一段数据
  //  如果这个段带 FIN
  if ( is_last_subs ) {
    // FIN 在数据之后的位置
    uint64_t fin_index = stream_index + message.payload.size();

    // 如果 FIN 已经可以按序到达
    if ( fin_index == reassembler_.writer().bytes_pushed() ) {
      reassembler_.close_output();
    }
  }
}

//--------------------------------------------------

// 去除了该函数的const
TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage send_message {};

  // 未接收到初始序列号，ack字段记为空
  if ( !isn_initialnized ) {
    send_message.ackno = std::nullopt;
  } else {
    // 接收到，则ack记为所需的下一个序号
    uint64_t abs_ackno = reassembler_.writer().bytes_pushed() + 1; //+syn
    if ( reassembler_.writer().is_closed() ) {
      ++abs_ackno;
    }
    send_message.ackno = Wrap32::wrap( abs_ackno, isn_ );
  }

  // 确认window_size为tcp可以接受的容量
  send_message.window_size = reassembler_.ava_capacity();

  send_message.RST = reassembler_.writer().has_error();
  return send_message;
}