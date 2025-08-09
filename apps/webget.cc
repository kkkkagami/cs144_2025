#include "socket.hh"

#include <cstdlib>
#include <iostream>
#include <span>
#include <string>

using namespace std;

void get_URL( const string& host, const string& path )
{
  // cerr << "Function called: get_URL(" << host << ", " << path << ")\n";
  // cerr << "Warning: get_URL() has not been implemented yet.\n";

  // 根据域名host和服务名http创建addr对象
  Address addr( host, "http" );

  TCPSocket tcp;       // 创建tcp套接字等待连接
  tcp.connect( addr ); // 发起到服务器的连接

  // write方法，将buffer写入套接字，向已建立的tcp发送数据
  tcp.write( "GET " + path + " HTTP/1.1\r\n" );
  tcp.write( "Host:" + host + "\r\n" );
  tcp.write( "Connection:close\r\n" );
  tcp.write( "\r\n" );

  // 套接字会向应用层返回数据，通过循环读取到buffer并输出
  while ( !tcp.eof() ) {
    std::string buffer;
    tcp.read( buffer );
    cout << buffer;
  }
  tcp.close();
}

int main( int argc, char* argv[] )
{
  try {
    if ( argc <= 0 ) {
      abort(); // For sticklers: don't try to access argv[0] if argc <= 0.
    }

    auto args = span( argv, argc );

    // The program takes two command-line arguments: the hostname and "path" part of the URL.
    // Print the usage message unless there are these two arguments (plus the program name
    // itself, so arg count = 3 in total).
    if ( argc != 3 ) {
      cerr << "Usage: " << args.front() << " HOST PATH\n";
      cerr << "\tExample: " << args.front() << " stanford.edu /class/cs144\n";
      return EXIT_FAILURE;
    }

    // Get the command-line arguments.
    const string host { args[1] };
    const string path { args[2] };

    // Call the student-written function.
    get_URL( host, path );
  } catch ( const exception& e ) {
    cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
