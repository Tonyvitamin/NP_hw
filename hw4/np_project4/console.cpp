#include <array>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <vector>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <fstream>

using namespace boost::asio;
using namespace boost::asio::ip;

boost::asio::io_service ioservice;

class npshell{
	public:
		std::string host_name;
		std::string host_port;
		std::string host_file;	
};
npshell np_server[5];

void output_shell(std::string session, std::string content){
  boost::replace_all(content, "&", "&amp;");
  boost::replace_all(content, "\"", "&quot;");
  boost::replace_all(content, "\'", "&apos;");

  boost::replace_all(content, "\r", "");

  boost::replace_all(content, ">", "&gt;");  
  boost::replace_all(content, "<", "&lt;");  
  boost::replace_all(content, "\n", "&#13;");
  

  std::cout<< "\"<script>document.getElementById(\'" <<session << "\').innerHTML += \'" << content << "\';</script>" << std::flush; 
}

void output_command(std::string session, std::string content){
  boost::replace_all(content, "&", "&amp;");
  boost::replace_all(content, "\"", "&quot;");
  boost::replace_all(content, "\'", "&apos;");

  boost::replace_all(content, "\r", "");

  boost::replace_all(content, ">", "&gt;");  
  boost::replace_all(content, "<", "&lt;");  
  boost::replace_all(content, "\n", "&#13;"); 


  std::cout<< "\"<script>document.getElementById(\'" <<session << "\').innerHTML += \'<b>" << content << "</b>\';</script>" << std::flush; 
}



std::string basic_console_frame = 
"<!DOCTYPE html>\n"
"<html lang=\"en\">\n"
"  <head>\n"
"    <meta charset=\"UTF-8\" />\n"
"    <title>NP Project 3 Console</title>\n"
"    <link\n"
"      rel=\"stylesheet\"\n"
"      href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
"      integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
"      crossorigin=\"anonymous\"\n"
"    />\n"
"    <link\n"
"      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
"      rel=\"stylesheet\"\n"
"    />\n"
"    <link\n"
"      rel=\"icon\"\n"
"      type=\"image/png\"\n"
"      href=\"https://cdn0.iconfinder.com/data/icons/small-n-flat/24/678068-terminal-512.png\"\n"
"    />\n"
"    <style>\n"
"      * {\n"
"               font-family: 'Source Code Pro', monospace;\n"
"                       font-size: 1rem !important;\n"
"                             }\n"
"                                   body {\n"
"                                           background-color: #212529;\n"
"                                                 }\n"
"                                                       pre {\n"
"                                                               color: #cccccc;\n"
"                                                                     }\n"
"                                                                           b {\n"
"                                                                                   color: #ffffff;\n"
"                                                                                         }\n"
"                                                                                             </style>\n"
"                                                                                               </head>\n"
"      \n";
std::string copy_data(std::array<char, 8192> data, int length){
  std::string raw_data ;
  for(int i=0;i<length;i++){
      raw_data = raw_data + data.at(i);
  }
  return raw_data;
}

class console : public std::enable_shared_from_this<console> {
  public:
     console(std::string host_name, std::string port, std::string sn, std::string file_name)
     : session(sn),
       query(boost::asio::ip::tcp::v4(), host_name.c_str(), port.c_str()){
         file.open("test_case/" + file_name, std::ios::in);
       };
	//private:
		void do_connect_npserver();
    void do_read_from_npshell();
		std::string session;
		//boost::asio::ip::tcp::endpoint
    std::fstream file;
    std::array<char, 8192> data;
    tcp::resolver tcp_resolver{ioservice};
    tcp::socket tcp_socket{ioservice};
    tcp::resolver::query query;
}; 



void console::do_connect_npserver(){
  auto self(shared_from_this());
  tcp_resolver.async_resolve(query, [this, self](boost::system::error_code ec, tcp::resolver::iterator it){
    if(!ec){
          tcp_socket.async_connect(*it, [this, self](boost::system::error_code ec1){
            if(!ec1){
              do_read_from_npshell();
            }
          });
    }
  });
}

void writehandler(const boost::system::error_code& error, std::size_t bytes_transferred); 

void console::do_read_from_npshell(){
  auto self(shared_from_this());


  tcp_socket.async_read_some(boost::asio::buffer(data),
    [this, self](boost::system::error_code ec, std::size_t length){
      if(!ec){
        std::string content = copy_data(data, length);
        output_shell(session, content);
        boost::regex end_line{".*% .*"};
        if(boost::regex_match(content, end_line)){
          std::string command;
          std::getline(file, command);
          command = command + '\n';


          // To transmit complete command of a line
          output_command(session, command);
          //tcp_socket.async_send()
          boost::asio::async_write(tcp_socket, boost::asio::buffer(command), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});

        }

        do_read_from_npshell();


      }    
    
    });
}

class socks_console : public std::enable_shared_from_this<socks_console> {
  public:
    socks_console(std::string host_name, std::string port, std::string sn, std::string file_name, std::string sh, std::string sp)
    :  session(sn),
       np_query(boost::asio::ip::tcp::v4(), host_name.c_str(), port.c_str()),
       socks_query(boost::asio::ip::tcp::v4(), sh.c_str(), sp.c_str()){
         file.open("test_case/" + file_name, std::ios::in);
       }; 
    void do_connect_npserver();
    void do_read_from_npshell();
    std::string session;
    std::fstream file;
    std::array<char, 8192> data;
    std::array<uint8_t, 8> socks_reply;
    std::array<uint8_t, 15> socks_request;
    tcp::resolver tcp_resolver{ioservice};
    tcp::socket tcp_socket{ioservice};
    tcp::resolver::query np_query;
    tcp::resolver::query socks_query;
    
};
void socks_console::do_connect_npserver(){
  auto self(shared_from_this());
  tcp_resolver.async_resolve(socks_query, [this, self](boost::system::error_code ec, tcp::resolver::iterator it){
    if(!ec){
          tcp_socket.async_connect(*it, [this, self](boost::system::error_code ec1){
            if(!ec1){
              auto iter = tcp_resolver.resolve(np_query);
              boost::asio::ip::tcp::endpoint np_endpoint = *iter;
              socks_request[0] = 0x04;
              socks_request[1] = 0x01;
              socks_request[2] = (np_endpoint.port() >> 8) & 0xFF;
              socks_request[3] = np_endpoint.port() & 0x00FF;
              auto addr = np_endpoint.address().to_v4().to_bytes();
              socks_request[4] = addr[0];
              socks_request[5] = addr[1];
              socks_request[6] = addr[2];
              socks_request[7] = addr[3];
              socks_request[8] = 0x05;
              socks_request[9] = 0x00;
              boost::asio::async_write(tcp_socket,boost::asio::buffer(socks_request), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});
              //boost::asio::async_read(tcp_socket, boost::asio::buffer(socks_reply), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});
              tcp_socket.async_read_some(boost::asio::buffer(socks_reply), [this, self](boost::system::error_code ec, std::size_t length){});
              //std::cout<<"check status: "<<socks_reply[0]<<" "<<socks_reply[1]<<" "<<socks_reply[2]<<" "<<socks_reply[3]<<" "<<socks_reply[4]<<" "<<socks_reply[5]<<" "<<socks_reply[6]<<" "<<socks_reply[7]<<std::endl;
              //if(socks_reply[1]==0x5A){
                do_read_from_npshell();
              //}
            }
          });
    }
  });
}

void socks_console::do_read_from_npshell(){
  auto self(shared_from_this());

  tcp_socket.async_read_some(boost::asio::buffer(data),
    [this, self](boost::system::error_code ec, std::size_t length){
      if(!ec){
        //std::cout<<"read in "<< data.data()<<std::endl;
        std::string content = copy_data(data, length);
        output_shell(session, content);
        boost::regex end_line{".*% .*"};
        if(content.find("% ") != std::string::npos){
          std::string command;
          std::getline(file, command);
          command = command + '\n';


          // To transmit complete command of a line
          output_command(session, command);
          //tcp_socket.async_send()
          boost::asio::async_write(tcp_socket, boost::asio::buffer(command, command.length()), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});

        }

        do_read_from_npshell();


      }    
    
    });
}
std::string framework_1 = 
"  <body>\n"
"    <table class=\"table table-dark table-bordered\">\n"
"      <thead>\n"
"        <tr>\n";

std::string framework_2 = 
"        </tr>\n"
"      </thead>\n"
"      <tbody>\n"
"        <tr>\n";

std::string framework_3 =
"        </tr>\n"
"      </tbody>\n"
"    </table>\n"
"  </body>\n"
"</html>\n";




int main(){
	boost::regex host_reg{"h.*"};
	boost::regex port_reg{"p.*"};
	boost::regex file_reg{"f.*"};
  boost::regex sh_reg{"sh.*"};
  boost::regex sp_reg{"sp.*"};

  std::string socks_server;
  std::string socks_port;
	// Parse query string 	
	std::string raw_query = getenv("QUERY_STRING");
	std::vector<std::string> query_info;
	boost::split(query_info, raw_query, boost::is_any_of("&"), boost::algorithm::token_compress_on);

	for(auto single_info: query_info){
		if(boost::regex_match(single_info, host_reg)){
			std::vector<std::string> host_mapping;
			boost::split(host_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
			if(host_mapping.size() < 2)
				continue;
			int index = atoi(host_mapping[0].substr(1).c_str());
			np_server[index].host_name = host_mapping[1];
		}
		else if(boost::regex_match(single_info, port_reg)){
                        std::vector<std::string> port_mapping;
                        boost::split(port_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
                        if(port_mapping.size() < 2)
                                continue;
                        int index = atoi(port_mapping[0].substr(1).c_str());
                        np_server[index].host_port = port_mapping[1];
		}
		else if(boost::regex_match(single_info, file_reg)){
                        std::vector<std::string> file_mapping;
                        boost::split(file_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
                        if(file_mapping.size() < 2)
                                continue;
                        int index = atoi(file_mapping[0].substr(1).c_str());
                        np_server[index].host_file = file_mapping[1];
		}
		else if(boost::regex_match(single_info, sh_reg)){
                        std::vector<std::string> file_mapping;
                        boost::split(file_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
                        if(file_mapping.size() < 2)
                                continue;
                        socks_server = file_mapping[1];
		}
		else if(boost::regex_match(single_info, sp_reg)){
                        std::vector<std::string> file_mapping;
                        boost::split(file_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
                        if(file_mapping.size() < 2)
                                continue;
                        socks_port = file_mapping[1];
		}
		else{
			std::cerr<<"Wrong format " << raw_query << std::endl;
		}	
	
	}

  // HTML appearence setting
	std::cout<< "Content-type: text/html\r\n\r\n";

	std::cout<< basic_console_frame << framework_1;

	for(int i=0;i<5;i++){
		if(!np_server[i].host_name.empty()){
			std::cout << "          <th scope=\"col\">" << np_server[i].host_name << ":" << np_server[i].host_port << "</th>\n";
		}

	}

	std::cout<< framework_2;

  for(int i=0;i<5;i++){
    if(!np_server[i].host_name.empty()){
			std::cout << "          <td><pre id=\"s" << std::to_string(i) << "\" class=\"mb-0\"></pre></td>\n"; 

    }

  }

	std::cout << framework_3;

  
  // Console CGI work
	//boost::asio::io_service io_service;

  for(int i=0;i<5;i++){
    if(socks_server.empty()&&socks_port.empty()){
      if(!np_server[i].host_name.empty()){
        //boost::asio::ip::address addr;
        std::string session = "s" + std::to_string(i);
        //addr.from_string(np_server[i].host_name);
        //unsigned short port = (unsigned short) stoi(np_server[i].host_port);
        //tcp::endpoint endpoint(addr, port);
        std::make_shared<console>(np_server[i].host_name, np_server[i].host_port, session, np_server[i].host_file)->do_connect_npserver();
      }
    }
    else {
        if(!np_server[i].host_name.empty()){

          //boost::asio::ip::address addr;
          std::string session = "s" + std::to_string(i);
          //addr.from_string(np_server[i].host_name);
          //unsigned short port = (unsigned short) stoi(np_server[i].host_port);
          //tcp::endpoint endpoint(addr, port);
          std::make_shared<socks_console>(np_server[i].host_name, np_server[i].host_port, session, np_server[i].host_file, socks_server, socks_port)->do_connect_npserver();
        }
      }

  }
  ioservice.run();
  //std::cout<<"finished"<<std::flush;

	return 0;
}
