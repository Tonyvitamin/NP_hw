#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
//#include <boost/asio/io_context.hpp>
#include <boost/algorithm/string.hpp>
////#include <boost/algorithm/string_regex.hpp>
////#include <boost/regex.hpp>
#include <bitset>
#include <regex>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <ctime>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
//
using namespace boost::asio;
using namespace boost::asio::ip;
io_service global_io_service;

std::string copy_data(std::array<char, 65536> data, int length){
  std::string raw_request ;
  for(int i=0;i<length;i++){
      raw_request = raw_request + data.at(i);
  }
  return raw_request;
}
/*
class connection : public std::enable_shared_from_this<connection> {

    public:
        connection(tcp::socket new_socket)
            :connection_socket(std::move(new_socket)){
                read_request();
            }
        void write_reply(std::string basic_reply);
        void read_request();
        tcp::socket connection_socket;
        std::array<char, 8192> data;
};
*/

class socks_request{
	public:
		uint8_t version;
		uint8_t command;
		uint16_t D_port;
		uint32_t D_ip;
		int userid;
		std::string domain_name; // socks 4a
		void parse(const uint8_t * data, std::size_t length); 	
};
void socks_request::parse(const uint8_t * data, std::size_t length){

	//socks_request req;
  //std::cout<<"version byte is "<<std::bitset<8>(data[0])<<std::endl;
	version = data[0];
  //std::cout<<"command byte is "<<std::bitset<8>(data[1])<<std::endl;
	command = data[1];
  //command = 0x02;
  //std::cout<<"higher byte of port is "<<std::bitset<8>(data[2])<<" lower byte of port is "<<std::bitset<8>(data[3])<<std::endl;
	D_port = data[2] << 8 | data[3];
  //std::cout<<"first byte "<<std::bitset<8>(data[4])<<" second byte "<<std::bitset<8>(data[5])<<" third byte "<<std::bitset<8>(data[6])<<" fourth byte "<<std::bitset<8>(data[7])<<std::endl;
	D_ip = data[4] << 24 | data[5] << 16 | data[6] << 8 | data[7];
	
	int i;

	for(i = 8;i<length;i++){
		if(data[i]==0)
			break;
	}

	if(D_ip > 0 && D_ip <= 255 && i+1<length){
		domain_name.assign((char *)&data[i+1], length-i-1);
	}

	return ;

}



class socks_server{
    private:
      boost::asio::io_service &server_ioservice;        
      tcp::acceptor tcp_acceptor;
      tcp::socket tcp_socket_1;
      tcp::socket tcp_socket_2;
      tcp::resolver tcp_resolver;
      boost::asio::signal_set server_signals;

      // firewall conf
      std::vector<std::string> c_whites;
      std::vector<std::string> b_whites;

      // socks request
      std::array<uint8_t , 8192> data;

      // socks reply
      std::array<uint8_t, 8> reply;

      // message (connect or bind)
      std::array< char, 65536> client;
      std::array< char, 65536> server;


      void do_accept();

      void connect_target(std::string addr, std::string port);
      
      void set_firewall();
      bool firewall_filter(std::string IP, int command);

      void do_read_socks_request();
      void handle_accept(boost::system::error_code &ec);
      void wait_for_signal(); 
      
      void read_from_target();
      void read_from_source();
      void write_to_target(size_t length);
      void write_to_source(size_t length);


	

    public:
      socks_server(unsigned short port)
      : server_ioservice(global_io_service),
        tcp_acceptor(global_io_service, {tcp::v4(), port}),
        tcp_socket_1(server_ioservice),
	      tcp_socket_2(server_ioservice),
        tcp_resolver(global_io_service),

        server_signals(server_ioservice){

        server_signals.add(SIGCHLD);
        wait_for_signal();
        do_accept();
      }
      void run();

};

bool socks_server::firewall_filter(std::string IP, int command){
	std::string raw_ip = IP.c_str();
	std::vector<std::string> IP_subs;
	boost::split(IP_subs, raw_ip, boost::is_any_of("."), boost::algorithm::token_compress_on);
  if(command == 1){
    for(int i=0;i<c_whites.size();i++){
	    std::string raw_query = c_whites[i].c_str();
	    std::vector<std::string> whites;
	    boost::split(whites, raw_query, boost::is_any_of("."), boost::algorithm::token_compress_on);
      bool flag = true;
      for(int j=0;j<4;j++){
        //std::cout<<"rule "<< i <<" sub part is "<<whites[j]<<" IP part is "<<IP_subs[j]<<std::endl;
        if(whites[j]!="*" && IP_subs[j] != whites[j])
          flag = false;
      }

      if(flag)
        return true;
    }
  }
  else if(command == 2){
    for(int i=0;i<b_whites.size();i++){
	    std::string raw_query = b_whites[i].c_str();
	    std::vector<std::string> whites;
	    boost::split(whites, raw_query, boost::is_any_of("."), boost::algorithm::token_compress_on);
      bool flag = true;
      for(int j=0;j<4;j++){
        //std::cout<<"rule "<< i <<" sub part is "<<whites[j]<<" IP part is "<<IP_subs[j]<<std::endl;

        if(whites[j]!="*" && IP_subs[j] != whites[j])
          flag = false;
      }

      if(flag)
        return true;
    }
  }

  return false;
}

void socks_server::set_firewall(){
  std::fstream file;
  file.open("./socks.conf", std::ios::in);
  std::string line;
  while(std::getline(file, line)){
    if(line[7]=='c'){
      std::string rule = line.substr(9);
      c_whites.push_back(rule);
    }
    else if(line[7]=='b'){
      std::string rule = line.substr(9);
      b_whites.push_back(rule);
    }
  }
}

void socks_server::connect_target(std::string addr, std::string port){
  tcp::resolver::query q(boost::asio::ip::tcp::v4(), addr.c_str(), port.c_str());
  tcp_resolver.async_resolve(q, [this](boost::system::error_code ec, tcp::resolver::iterator it){
    if(!ec){
      tcp_socket_2.async_connect(*it, [this](boost::system::error_code ec1){
        if(!ec1){
          boost::asio::async_write(tcp_socket_1, boost::asio::buffer(reply), [](const boost::system::error_code& error, std::size_t bytes_transferred){});
          read_from_target();
          read_from_source();
        }
      });
    }


  });
}

// relay data
void socks_server::read_from_target(){
  tcp_socket_2.async_read_some(boost::asio::buffer(server),
    [this](boost::system::error_code ec, std::size_t length){
      if(!ec){
    //    std::cout<<"read from server "<<copy_data(server, length)<<std::endl;
    //    std::cout<<"length is "<<length<<std::endl;
       	write_to_source(length);
      }
      else if(ec==boost::asio::error::eof){
        //tcp_socket_1.close();
        //tcp_socket_2.shutdown(ip::tcp::socket::shutdown_receive);

        tcp_socket_1.shutdown(ip::tcp::socket::shutdown_send);
        //tcp_socket_1.close();
        //tcp_socket_2.close();
        //std::cout<<ec.message()<<std::endl;
      }

    });
}
void socks_server::read_from_source(){
  tcp_socket_1.async_read_some(boost::asio::buffer(client),
    [this](boost::system::error_code ec, std::size_t length){
      if(!ec){
    //    std::cout<<"read from client "<<copy_data(client, length)<<std::endl;
    //    std::cout<<"length is "<<length<<std::endl;
	//client[length] = '\n';
        write_to_target(length);
      }
      else if(ec==boost::asio::error::eof){
        //tcp_socket_2.close();
        //tcp_socket_1.shutdown(ip::tcp::socket::shutdown_receive);

        tcp_socket_2.shutdown(ip::tcp::socket::shutdown_send);

      }
    });
}
void socks_server::write_to_target(size_t length){
  boost::asio::async_write(tcp_socket_2, boost::asio::buffer(client, length), [this](boost::system::error_code ec, std::size_t length1){
    if(!ec){
    //  std::cout<<"write to server " << copy_data(client, length1)<<std::endl;
    //    std::cout<<"length is "<<length1<<std::endl;

      read_from_source();
    }
    

  });
}
void socks_server::write_to_source(size_t length){
  boost::asio::async_write(tcp_socket_1, boost::asio::buffer(server, length), [this](boost::system::error_code ec, std::size_t length1){
    if(!ec){
      //std::cout<<"write to client " << copy_data(server, length1)<<std::endl ;
      //  std::cout<<"length is "<<length1<<std::endl;

      read_from_target();
    }

  });
}

void socks_server::wait_for_signal(){
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

void socks_server::run(){
  server_ioservice.run();
}

void socks_server::do_read_socks_request(){
	tcp_socket_1.async_read_some(boost::asio::buffer(data), [this] (boost::system::error_code ec, size_t bytes_transferred){
//	boost::asio::read(tcp_socket_1, boost::asio::buffer(data));
		socks_request req;
    int command = 0;
		req.parse(data.data(), bytes_transferred);
    //std::cout<<req.domain_name<<std::endl;

    if(req.D_ip > 0 && req.D_ip <= 255){
      boost::asio::ip::tcp::resolver resolver(global_io_service);
      //std::cout<<"here1\n";
      boost::asio::ip::tcp::resolver::query query(boost::asio::ip::tcp::v4(), req.domain_name.c_str(), std::to_string(req.D_port).c_str());
      //std::cout<<"here2\n";
      auto iter = resolver.resolve(query);
      //std::cout<<"here3\n";
      boost::asio::ip::tcp::endpoint ep = *iter;
      //std::cout<<"here4\n";
      //std::cout<<ep.address()<<std::endl;
      auto addr = ep.address().to_v4().to_bytes();
      req.D_ip = (addr[0] << 24) | (addr[1] << 16) | (addr[2] << 8) | addr[3];
    }
    std::cout<<"<S_IP>: "<<tcp_socket_1.remote_endpoint().address().to_string()<<std::endl;
    std::cout<<"<S_PORT>: "<<tcp_socket_1.remote_endpoint().port()<<std::endl;
    
    std::string D_IP = std::to_string(((req.D_ip>>24) & 0xFF)) + "." + std::to_string(((req.D_ip>>16) & 0x00FF)) + "." +  std::to_string(((req.D_ip>>8) & 0x0000FF)) + "." + std::to_string((req.D_ip & 0x000000FF));
    std::string D_PORT = std::to_string(req.D_port);
    
    std::cout<<"<D_IP>: "<< ((req.D_ip>>24) & 0xFF) <<"."<< ((req.D_ip>>16) & 0x00FF) <<"."<< ((req.D_ip>>8) & 0x0000FF)<<"."<< (req.D_ip & 0x000000FF)<<std::endl;
    std::cout<<"<D_PORT>: "<<req.D_port<<std::endl;
    
    if(req.command == 0x01){
      std::cout<<"<Command>: CONNECT\n";
      command = 1;
    }
    else if(req.command == 0x02){
      std::cout<<"<Command>: BIND\n";
      command = 2;
    }

    // firewall conf part
    if(firewall_filter(D_IP, command)){
      std::cout<<"<Reply>: Accept\n";

      if(req.command == 0x01){
        // version & command
        reply[0] = 0x00;
        reply[1] = 0x5A;
        // DST_PORT
        reply[2] = (req.D_port>>8) & 0xFF;
        reply[3] = req.D_port & 0x00FF;
          
        // DST_IP
        reply[4] = (req.D_ip>>24) & 0xFF;
        reply[5] = (req.D_ip>>16) & 0x00FF;
        reply[6] = (req.D_ip>>8) & 0x0000FF;
        reply[7] = req.D_ip & 0x000000FF;
        
        /*
        std::cout<<std::bitset<8>(reply[0])<<std::endl;
        std::cout<<std::bitset<8>(reply[1])<<std::endl;
        std::cout<<std::bitset<8>(reply[2])<<std::endl;
        std::cout<<std::bitset<8>(reply[3])<<std::endl;
        std::cout<<std::bitset<8>(reply[4])<<std::endl;
        std::cout<<std::bitset<8>(reply[5])<<std::endl;
        std::cout<<std::bitset<8>(reply[6])<<std::endl;
        std::cout<<std::bitset<8>(reply[7])<<std::endl;


        std::cout<<D_IP<<std::endl;
        std::cout<<D_PORT<<std::endl;
        */
        connect_target(D_IP, D_PORT);

      }

      else if(req.command == 0x02){
        boost::asio::ip::tcp::endpoint bind_endpoint(boost::asio::ip::tcp::v4(), 0);

        tcp::acceptor bind_acceptor(global_io_service, bind_endpoint);

        bind_acceptor.listen();

        auto ep = bind_acceptor.local_endpoint();

        // version & command
        reply[0] = 0x00;
        reply[1] = 0x5A;
        
        //DST_PORT
        reply[2] = (ep.port() >> 8) & 0xFF;
        reply[3] = ep.port() & 0x00FF;

        //DST_IP
        //auto addr = boost::asio::ip::make_address_v4("0.0.0.0").to_bytes();
        reply[4] = 0x00;
        reply[5] = 0x00;
        reply[6] = 0x00;
        reply[7] = 0x00;

        // First reply
        boost::asio::write(tcp_socket_1, boost::asio::buffer(reply));
        
        // wait for connection
        bind_acceptor.accept(tcp_socket_2);

        //second reply
        boost::asio::write(tcp_socket_1, boost::asio::buffer(reply));

        read_from_target();
        read_from_source();
        //tcp_socket_1.close();
        //tcp_socket_2.close();
      }
    }

    else {
      std::cout<<"<Reply>: Reject\n";
      reply[0] = 0x00;
      reply[1] = 0x5B;
      // DST_PORT
      reply[2] = (req.D_port>>8) & 0xFF;
      reply[3] = req.D_port & 0x00FF;
          
      // DST_IP
      reply[4] = (req.D_ip>>24) & 0xFF;
      reply[5] = (req.D_ip>>16) & 0x00FF;
      reply[6] = (req.D_ip>>8) & 0x0000FF;
      reply[7] = req.D_ip & 0x000000FF;
      boost::asio::async_write(tcp_socket_1, boost::asio::buffer(reply), [](const boost::system::error_code& error, std::size_t bytes_transferred){});

    }
    


  

	});

}

void socks_server::do_accept(){

  tcp_acceptor.async_accept(
      [this](boost::system::error_code ec, boost::asio::ip::tcp::socket new_socket)
      {
        tcp_socket_1 = std::move(new_socket);

        if (!ec)
        {
          server_ioservice.notify_fork(boost::asio::io_service::fork_prepare);
          if(fork()==0){
            server_ioservice.notify_fork(boost::asio::io_service::fork_child);
            tcp_acceptor.close();
            server_signals.cancel();

            //set firewall
            set_firewall();

            do_read_socks_request();
          }
          else{
            server_ioservice.notify_fork(boost::asio::io_service::fork_parent);
            tcp_socket_1.close();
            tcp_socket_2.close();
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



int main(int argc, char * argv[], char * const envp[]){
  unsigned short port = atoi(argv[1]);
  socks_server server(port);
  server.run();
  return 0;
}
