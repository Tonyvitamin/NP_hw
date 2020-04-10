#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
//#include <boost/asio/io_context.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

#include <string>
#include <cstdlib>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <ctime>

using namespace boost::asio;
using namespace boost::asio::ip;

boost::asio::io_service global_ioservice;

class http_server{
    private:
        boost::asio::io_service &server_ioservice;
        tcp::acceptor tcp_acceptor;
        tcp::socket tcp_socket;
        boost::asio::signal_set server_signals;
        std::array<char, 8192> data_;

        void do_accept();

        void do_read();

        void wait_for_signal();    

    public:
      http_server(unsigned short port)
      : server_ioservice(global_ioservice),
        tcp_acceptor(global_ioservice, {tcp::v4(), port}),
        tcp_socket(server_ioservice),
        server_signals(server_ioservice){

        server_signals.add(SIGCHLD);
        wait_for_signal();
        do_accept();
      }
      void run();

};

void http_server::wait_for_signal(){
    server_signals.async_wait(
        [this](boost::system::error_code /*ec*/, int /*signo*/)
        {
          if (tcp_acceptor.is_open())
          {
            int status = 0;
            while (waitpid(-1, &status, WNOHANG) > 0) {}

            wait_for_signal();
          }
        });

}

void http_server::run(){
  server_ioservice.run();
}

void http_server::do_accept(){

  tcp_acceptor.async_accept(
      [this](boost::system::error_code ec, boost::asio::ip::tcp::socket new_socket)
      {
        tcp_socket = std::move(new_socket);

        if (!ec)
        {
          server_ioservice.notify_fork(boost::asio::io_service::fork_prepare);
          if(fork()==0){
            server_ioservice.notify_fork(boost::asio::io_service::fork_child);
            tcp_acceptor.close();
            server_signals.cancel();
            do_read();
          }
          else{
            server_ioservice.notify_fork(boost::asio::io_service::fork_parent);
            tcp_socket.close();
            do_accept();
          }
        }
        else
        {
            std::cerr << "Accept error: " << ec.message() << std::endl;
            do_accept();
        }
      });
}
std::string copy_data(std::array<char, 8192> data, int length){
  std::string raw_request ;
  for(int i=0;i<length;i++){
      raw_request = raw_request + data.at(i);
  }
  return raw_request;
}

std::string parse(std::string raw_request){
  boost::regex GET_reg{"GET.*"};
  boost::regex HOST_reg{"Host.*"};
  std::vector<std::string> http_request;
  boost::split(http_request, raw_request, boost::is_any_of("\r\n"), boost::algorithm::token_compress_on);

  std::string cgi_program;
  for(auto request_line:http_request){
    //std::cout<<request_line<<std::endl;
    if(boost::regex_match(request_line, GET_reg)){
      std::vector<std::string> first_line;
      boost::split(first_line, request_line, boost::is_space(), boost::algorithm::token_compress_on);
      
      //std::cout<<first_line[1].c_str()<<std::endl;
      setenv("REQUEST_URI", first_line[1].c_str(), 1);
      
      std::vector<std::string> query_line;
      boost::split(query_line, first_line[1], boost::is_any_of("?"), boost::algorithm::token_compress_on);

      cgi_program =  query_line[0];
      //std::cout<<cgi_program<<std::endl;

      if(query_line.size()>1){
        //std::cout<<query_line[1].c_str()<<std::endl;
        setenv("QUERY_STRING", query_line[1].c_str(), 1);
      }
      else {
        setenv("QUERY_STRING", "", 1);
      }

      //std::cout<<first_line[2].c_str()<<std::endl;
      setenv("SERVER_PROTOCOL", first_line[2].c_str(), 1);
    }
    else if(boost::regex_match(request_line, HOST_reg)){
      std::vector<std::string> host_line;
      boost::split(host_line, request_line, boost::is_any_of(":"), boost::algorithm::token_compress_on);
      setenv("HTTP_HOST",host_line[1].c_str(), 1);
    }
  }
  //std::cout<<"Parse finished"<<std::endl;
  return cgi_program;
}
std::string parse_request(std::array<char, 8192> data, int length){
  std::string raw_request = copy_data(data, length);
  return parse(raw_request);
}



void http_server::do_read(){
    tcp_socket.async_read_some(boost::asio::buffer(data_),
        [this](boost::system::error_code ec, std::size_t length){
          if (!ec){
            std::string cgi_program = parse_request(data_, length).substr(1);
            
            // Default environment setting
            // no use for parsing request
            setenv("REQUEST_METHOD", "GET", 1);
            setenv("SERVER_ADDR", tcp_socket.local_endpoint().address().to_string().c_str(), 1);
            setenv("SERVER_PORT", std::to_string(tcp_socket.local_endpoint().port()).c_str(), 1);
            setenv("REMOTE_ADDR", tcp_socket.remote_endpoint().address().to_string().c_str(), 1);
            setenv("REMOTE_PORT", std::to_string(tcp_socket.remote_endpoint().port()).c_str(), 1);

            dup2(tcp_socket.native_handle(), 0);
            dup2(tcp_socket.native_handle(), 1);
            dup2(tcp_socket.native_handle(), 2);
            //almost all browsers return HTTP/1.1
            std::cout<<"HTTP/1.1" << " 200 OK\r\n" << std::flush;
            char * argv [] = {NULL};
            execv(cgi_program.c_str(), argv);
          }


        });


}

int main(int argc, char * argv[], char * const envp[])
{
  
  unsigned short port = atoi(argv[1]);
  //boost::asio::io_service ioservice;
  http_server server(port);
  server.run();
  

  return 0;
}
