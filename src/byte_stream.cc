#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  uint64_t size_of_data=data.size();
  if(size_of_data>available_capacity()){
    size_of_data=available_capacity();
  }

  for(uint64_t i=0;i<size_of_data;++i){
    //注释：不可以用const auto c:data来遍历，这样会导致越界，size()出错
    byte_stream_.push_back(data[i]);
  }
  bytes_pushed_+=size_of_data;
}

void Writer::close()
{
  is_closed_=true;
}

bool Writer::is_closed() const
{
  return is_closed_;
}

uint64_t Writer::available_capacity() const
{
  return capacity_-byte_stream_.size();
}

uint64_t Writer::bytes_pushed() const
{
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  if(byte_stream_.empty())  return "";

  std::string_view peek_byte(&byte_stream_[0],1);
  return peek_byte;
}

void Reader::pop( uint64_t len )
{
  uint64_t pop_size=min(len,byte_stream_.size());
  for(uint64_t i=0;i<pop_size;++i){
    byte_stream_.pop_front();
  }

  bytes_popped_+=pop_size;
}

bool Reader::is_finished() const
{
  return is_closed_&&byte_stream_.empty();
}

uint64_t Reader::bytes_buffered() const
{
  return byte_stream_.size();
}

uint64_t Reader::bytes_popped() const
{
  return bytes_popped_;
}