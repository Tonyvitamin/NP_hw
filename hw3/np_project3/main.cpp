#include <boost/asio/io_service.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
//#include <boost/asio/io_context.hpp>
#include <boost/algorithm/string.hpp>
//#include <boost/algorithm/string_regex.hpp>
//#include <boost/regex.hpp>
#include <regex>
#include <string>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>
#include <ctime>

using namespace boost::asio;
using namespace boost::asio::ip;
io_service global_io_service;

class npshell{
	public:
		std::string host_name;
		std::string host_port;
		std::string host_file;	
};
npshell np_server[5];

// simulate sample_console cgi output shell
// according to python escape(html)
std::string output_shell(std::string session, std::string content){
  boost::replace_all(content, "&", "&amp;");
  boost::replace_all(content, "\"", "&quot;");
  boost::replace_all(content, "\'", "&apos;");

  boost::replace_all(content, "\r", "");

  boost::replace_all(content, ">", "&gt;");  
  boost::replace_all(content, "<", "&lt;");  
  boost::replace_all(content, "\n", "&#13;");
  
  //std::cout<< "\"<script>document.getElementById(\'" <<session << "\').innerHTML += \'" << content << "\';</script>" << std::flush; 

  return "\"<script>document.getElementById(\'" + session + "\').innerHTML += \'" + content + "\';</script>";// std::flush; 
}

// simulate sample_console cgi output command
// according to python escape(html)
std::string output_command(std::string session, std::string content){
  boost::replace_all(content, "&", "&amp;");
  boost::replace_all(content, "\"", "&quot");
  boost::replace_all(content, "\'", "&apos");


  boost::replace_all(content, "\r", "");

  boost::replace_all(content, ">", "&gt");  
  boost::replace_all(content, "<", "&lt");  
  boost::replace_all(content, "\n", "&#13;");  

  //std::cout<< "\"<script>document.getElementById(\'" <<session << "\').innerHTML += \'<b>" << content << "</b>\';</script>" << std::flush; 

  return "\"<script>document.getElementById(\'" + session + "\').innerHTML += \'<b>" + content + "</b>\';</script>";// << std::flush;   
}

// simulate panel cgi html
std::string get_panel_cgi(){
    std::string header  = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

    std::string server_list =   "<option value=\"nplinux1.cs.nctu.edu.tw\">nplinux1</option>\n"
                                "<option value=\"nplinux2.cs.nctu.edu.tw\">nplinux2</option>\n"
                                "<option value=\"nplinux3.cs.nctu.edu.tw\">nplinux3</option>\n"
                                "<option value=\"nplinux4.cs.nctu.edu.tw\">nplinux4</option>\n"
                                "<option value=\"nplinux5.cs.nctu.edu.tw\">nplinux5</option>\n"
                                "<option value=\"nplinux6.cs.nctu.edu.tw\">nplinux6</option>\n"
                                "<option value=\"nplinux7.cs.nctu.edu.tw\">nplinux7</option>\n"
                                "<option value=\"nplinux8.cs.nctu.edu.tw\">nplinux8</option>\n"
                                "<option value=\"nplinux9.cs.nctu.edu.tw\">nplinux9</option>\n"
                                "<option value=\"nplinux10.cs.nctu.edu.tw\">nplinux10</option>\n"
                                "<option value=\"nplinux11.cs.nctu.edu.tw\">nplinux11</option>\n"
                                "<option value=\"nplinux12.cs.nctu.edu.tw\">nplinux12</option>\n";

    std::string file_list = "<option value=\"t1.txt\">t1.txt</option>\n"
                            "<option value=\"t2.txt\">t2.txt</option>\n"
                            "<option value=\"t3.txt\">t3.txt</option>\n"
                            "<option value=\"t4.txt\">t4.txt</option>\n"
                            "<option value=\"t5.txt\">t5.txt</option>\n"
                            "<option value=\"t6.txt\">t6.txt</option>\n"
                            "<option value=\"t7.txt\">t7.txt</option>\n"
                            "<option value=\"t8.txt\">t8.txt</option>\n"
                            "<option value=\"t9.txt\">t9.txt</option>\n"
                            "<option value=\"t10.txt\">t10.txt</option>\n";

    std::string basic_panel_frame = "<!DOCTYPE html>\n"
                                    "      <html lang=\"en\">\n"
                                    "       <head>\n"
                                    "    <title>NP Project 3 Panel</title>\n"
                                    "    <link\n"
                                    "      rel=\"stylesheet\"\n"
                                    "           href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css\"\n"
                                    "           integrity=\"sha384-MCw98/SFnGE8fJT3GXwEOngsV7Zt27NXFoaoApmYm81iuXoPkFOJwJ8ERdknLPMO\"\n"
                                    "           crossorigin=\"anonymous\"\n"
                                    "    />\n"
                                    "    <link\n"
                                    "      href=\"https://fonts.googleapis.com/css?family=Source+Code+Pro\"\n"
                                    "      rel=\"stylesheet\"\n"
                                    "    />\n"
                                    "    <link\n"
                                    "      rel=\"icon\"\n"
                                    "      type=\"image/png\"\n"
                                    "      href=\"https://cdn4.iconfinder.com/data/icons/iconsimple-setting-time/512/dashboard-512.png\"\n"
                                    "    />\n"
                                    "    <style>\n"
                                    "      * {\n"
                                    "        font-family: \'Source Code Pro\', monospace;\n"
                                    "        }\n"
                                    "    </style>\n"
                                    "  </head>\n"
                                    "  <body class=\"bg-secondary pt-5\">\n"
                                    "    <form action=\"console.cgi\" method=\"GET\">\n"
                                    "      <table class=\"table mx-auto bg-light\" style=\"width: inherit\">\n"
                                    "        <thead class=\"thead-dark\">\n"
                                    "          <tr>\n"
                                    "            <th scope=\"col\">#</th>\n"
                                    "            <th scope=\"col\">Host</th>\n"
                                    "            <th scope=\"col\">Port</th>\n"
                                    "            <th scope=\"col\">Input File</th>\n"
                                    "          </tr>\n"
                                    "        </thead>\n"
                                    "        <tbody>\n";
    std::string table;
    for(int i=0;i<5;i++){
        std::string tmp =  
                           "  <tr>\n <th scope=\"row\" class=\"align-middle\">Session " + std::to_string(i+1) + " </th>\n" +
                           " <td>\n" +
                           " <div class=\"input-group\">\n" +
                           "         <select name=\"h" + std::to_string(i) + "\" class=\"custom-select\">\n" +
                           "  <option></option>\n" + server_list +
                           "</select>\n" +
                           "<div class=\"input-group-append\">\n" +
                           "  <span class=\"input-group-text\">.cs.nctu.edu.tw</span>\n" +
                           "</div>\n" +
                           "  </div>\n" +
                           "</td>\n" +
                           "<td>\n" +
                           "<input name=\"p" + std::to_string(i) + "\" type=\"text\" class=\"form-control\" size=\"5\" />\n" + 
                           "</td>\n" +
                           "<td>\n" +
                           "<select name=\"f" + std::to_string(i) +"\" class=\"custom-select\">\n" +
                           "<option></option>\n" +file_list +
                           "</select>\n" +
                           "</td>\n" +
                           "</tr>\n";
        table = table + tmp;

    }

    std::string frame_end = "          <tr>\n"
                            "            <td colspan=\"3\"></td>\n"
                            "               <td>\n"
                            "              <button type=\"submit\" class=\"btn btn-info btn-block\">Run</button>\n"
                            "            </td>\n"
                            "          </tr>\n"
                            "        </tbody>\n"
                            "      </table>\n"
                            "    </form>\n"
                            "  </body>\n"
                            "</html>\n";
    return header + basic_panel_frame + table + frame_end;
}

// output one part of the console cgi html
std::string get_console_cgi_upper(){
    std::string header  = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";
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
                                    std::string framework_1 = 
                                    "  <body>\n"
                                    "    <table class=\"table table-dark table-bordered\">\n"
                                    "      <thead>\n"
                                    "        <tr>\n";

    return header + basic_console_frame + framework_1;

}

// output the other part of console cgi html
std::string get_console_cgi_lower(std::string raw_query){ 
	std::regex host_reg{"h.*"};
	std::regex port_reg{"p.*"};
	std::regex file_reg{"f.*"};

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
	// Parse query string 	
	std::vector<std::string> query_info;
	boost::split(query_info, raw_query, boost::is_any_of("&"), boost::algorithm::token_compress_on);

	for(auto single_info: query_info){
		if(std::regex_match(single_info, host_reg)){
			std::vector<std::string> host_mapping;
			boost::split(host_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
			if(host_mapping.size() < 2)
				continue;
			int index = atoi(host_mapping[0].substr(1).c_str());
			np_server[index].host_name = host_mapping[1];
		}
		else if(std::regex_match(single_info, port_reg)){
                        std::vector<std::string> port_mapping;
                        boost::split(port_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
                        if(port_mapping.size() < 2)
                                continue;
                        int index = atoi(port_mapping[0].substr(1).c_str());
                        np_server[index].host_port = port_mapping[1];
		}
		else if(std::regex_match(single_info, file_reg)){
                        std::vector<std::string> file_mapping;
                        boost::split(file_mapping, single_info, boost::is_any_of("="), boost::algorithm::token_compress_on);
                        if(file_mapping.size() < 2)
                                continue;
                        int index = atoi(file_mapping[0].substr(1).c_str());
                        np_server[index].host_file = file_mapping[1];
		}
		else{
			std::cerr<<"Wrong format " << raw_query << std::endl;
		}	
	
	}

  // HTML appearence setting
	//std::cout<< "Content-type: text/html\r\n\r\n";

	//std::cout<< basic_console_frame << framework_1;
  std::string tmp_string1;
	for(int i=0;i<5;i++){
		if(!np_server[i].host_name.empty()){
      tmp_string1 = tmp_string1 + "          <th scope=\"col\">" + np_server[i].host_name + ":" + np_server[i].host_port + "</th>\n";
			//std::cout << "          <th scope=\"col\">" << np_server[i].host_name << ":" << np_server[i].host_port << "</th>\n";
		}

	}

	//std::cout<< framework_2;
  std::string tmp_string2;

  for(int i=0;i<5;i++){
    if(!np_server[i].host_name.empty()){
      tmp_string2 = tmp_string2 + "          <td><pre id=\"s" + std::to_string(i) + "\" class=\"mb-0\"></pre></td>\n"; 
			//std::cout << "          <td><pre id=\"s" << std::to_string(i) << "\" class=\"mb-0\"></pre></td>\n"; 
    }
  }
    

  return tmp_string1 + framework_2 + tmp_string2 + framework_3;
}


// Based on single process http server
// we usually need a connecton (and connection manager) to read request and write simple reply
class connection : public std::enable_shared_from_this<connection> {

    public:
        connection(tcp::socket new_socket)//, boost::asio::io_service &n_ioservice)
            :connection_socket(std::move(new_socket)){
                //read_request();
            }
        void write_reply(std::string basic_reply);
        void read_request();
        tcp::socket connection_socket;
        std::array<char, 8192> data;
};
// Based on Console.cpp
// build multi connections to np server
// only write command and result to browser
class console : public std::enable_shared_from_this<console> {
    private:
        std::shared_ptr<connection> connection_ptr;
        tcp::socket tcp_socket_master{global_io_service};
        tcp::resolver tcp_resolver{global_io_service};
        std::fstream file;
        std::array<char,8192> data;
        tcp::resolver::query query;
        std::string session;

        
    public:
        console(std::shared_ptr<connection> used_connection, std::string host_name, std::string port, std::string sn, std::string file_name)
        : session(sn),
          query(host_name.c_str(), port.c_str()),
          connection_ptr(used_connection){
          file.open("test_case/" + file_name, std::ios::in);
       };
		    void do_connect_npserver();
        void do_read_from_npshell();
};

std::string copy_data(std::array<char, 8192> data, int length){
  std::string raw_request ;
  for(int i=0;i<length;i++){
      raw_request = raw_request + data.at(i);
  }
  return raw_request;
}

std::string parse1(std::string raw_request){
  std::regex GET_reg{"GET.*"};
  std::regex HOST_reg{"Host.*"};
  std::vector<std::string> http_request;
  boost::split(http_request, raw_request, boost::is_any_of("\r\n"), boost::algorithm::token_compress_on);

  std::string cgi_program;
  for(auto request_line:http_request){
    //std::cout<<request_line<<std::endl;
    if(std::regex_match(request_line, GET_reg)){
      std::vector<std::string> first_line;
      boost::split(first_line, request_line, boost::is_space(), boost::algorithm::token_compress_on);
      
      //std::cout<<first_line[1].c_str()<<std::endl;
      //setenv("REQUEST_URI", first_line[1].c_str(), 1);
      
      std::vector<std::string> query_line;
      boost::split(query_line, first_line[1], boost::is_any_of("?"), boost::algorithm::token_compress_on);

      cgi_program =  query_line[0];
      //std::cout<<cgi_program<<std::endl;

      /*
      if(query_line.size()>1){
        std::cout<<query_line[1].c_str()<<std::endl;
        //setenv("QUERY_STRING", query_line[1].c_str(), 1);
      }
      else {
        //setenv("QUERY_STRING", "", 1);
      }
      */

      //std::cout<<first_line[2].c_str()<<std::endl;
      //setenv("SERVER_PROTOCOL", first_line[2].c_str(), 1);
    }
    else if(std::regex_match(request_line, HOST_reg)){
      std::vector<std::string> host_line;
      boost::split(host_line, request_line, boost::is_any_of(":"), boost::algorithm::token_compress_on);
      //setenv("HTTP_HOST",host_line[1].c_str(), 1);
    }
  }
  //std::cout<<"Parse finished"<<std::endl;
  return cgi_program;
}

std::string parse2(std::string raw_request){
  std::regex GET_reg{"GET.*"};
  std::regex HOST_reg{"Host.*"};
  std::vector<std::string> http_request;
  boost::split(http_request, raw_request, boost::is_any_of("\r\n"), boost::algorithm::token_compress_on);

  std::string cgi_program;
  for(auto request_line:http_request){
    //std::cout<<request_line<<std::endl;
    if(std::regex_match(request_line, GET_reg)){
      std::vector<std::string> first_line;
      boost::split(first_line, request_line, boost::is_space(), boost::algorithm::token_compress_on);
      
      //std::cout<<first_line[1].c_str()<<std::endl;
      //setenv("REQUEST_URI", first_line[1].c_str(), 1);
      
      std::vector<std::string> query_line;
      boost::split(query_line, first_line[1], boost::is_any_of("?"), boost::algorithm::token_compress_on);

      cgi_program =  query_line[0];
      //std::cout<<cgi_program<<std::endl;

      if(query_line.size()>1){
        //std::cout<<query_line[1].c_str()<<std::endl;
        return query_line[1].c_str();
        //setenv("QUERY_STRING", query_line[1].c_str(), 1);
      }
      else {
        //setenv("QUERY_STRING", "", 1);
      }

      //std::cout<<first_line[2].c_str()<<std::endl;
      //setenv("SERVER_PROTOCOL", first_line[2].c_str(), 1);
    }
    else if(std::regex_match(request_line, HOST_reg)){
      std::vector<std::string> host_line;
      boost::split(host_line, request_line, boost::is_any_of(":"), boost::algorithm::token_compress_on);
      //setenv("HTTP_HOST",host_line[1].c_str(), 1);
    }
  }
  //std::cout<<"Parse finished"<<std::endl;
  return cgi_program;
}

std::string parse_request(std::array<char, 8192> data, int length){
  std::string raw_request = copy_data(data, length);
  return parse1(raw_request);
}

std::string parse_query(std::array<char, 8192> data, int length){
  std::string raw_request = copy_data(data, length);
  return parse2(raw_request);
}
void connection::read_request(){
    auto self(shared_from_this());
    connection_socket.async_read_some(
        boost::asio::buffer(data, 4096),
        [this, self](boost::system::error_code ec, std::size_t length){
            if(!ec){
                //std::cout<<"starting parsing\n"<<std::flush;
                std::string cgi_program = parse_request(data, length).substr(1);
                //std::cout<<"Parse success\n"<<std::flush;
                if(cgi_program=="panel.cgi"){
                    write_reply(get_panel_cgi());
                }
                else if(cgi_program=="console.cgi"){
                    std::string query_string = parse_query(data, length);
                    //std::cout<<query_string<<std::endl;
                    std::string console_part1 = get_console_cgi_upper();
                    std::string console_part2 = get_console_cgi_lower(query_string);
                    write_reply(console_part1 + console_part2);
                    for(int i=0;i<5;i++){
                        if(!np_server[i].host_name.empty()){
                        //boost::asio::ip::address addr;
                        std::string session = "s" + std::to_string(i);
                        //addr.from_string(np_server[i].host_name);
                        //unsigned short port = (unsigned short) stoi(np_server[i].host_port);
                        //tcp::endpoint endpoint(addr, port);
                        std::make_shared<console>(shared_from_this(), np_server[i].host_name, np_server[i].host_port, session, np_server[i].host_file)->do_connect_npserver();
                        }
                    }

                }
                //read_request();
            }
        }

    );
}

void connection::write_reply(std::string basic_reply){
    auto self(shared_from_this());
    boost::asio::async_write(connection_socket, boost::asio::buffer(basic_reply),
        [this, self](boost::system::error_code ec, std::size_t){
            if(!ec){
                //std::cout<<"Wrtie success\n";
            }
        }

    );
}
class http_server{
    private:
        tcp::socket tcp_socket;
        tcp::acceptor tcp_acceptor;

        void do_accept();
    public:
        http_server(unsigned short port)
        : tcp_acceptor(global_io_service, {tcp::v4(), port}),
          tcp_socket(global_io_service){
          do_accept();
        }
        void run();
};
void http_server::run(){
    global_io_service.run();
}
void http_server::do_accept(){
    tcp_acceptor.async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket new_socket){
            if(!ec){
                std::make_shared<connection>(std::move(new_socket))->read_request();
            }
            do_accept();
        }
    );
}





void console::do_connect_npserver(){
  auto self(shared_from_this());
  tcp_resolver.async_resolve(query, [this, self](boost::system::error_code ec, tcp::resolver::iterator it){
    if(!ec){
        tcp_socket_master.async_connect(*it, [this, self](boost::system::error_code ec1){
            if(!ec1){
                do_read_from_npshell();
            }
        });
        //do_connect_npserver(it);
    }
      
  });

}

void writehandler(const boost::system::error_code& error, std::size_t bytes_transferred); 


void console::do_read_from_npshell(){
  auto self(shared_from_this());
  tcp_socket_master.async_read_some(boost::asio::buffer(data),
    [this, self](boost::system::error_code ec, std::size_t length){
      if(!ec){
        std::string content = copy_data(data, length);
        boost::asio::async_write(
            connection_ptr->connection_socket,
            boost::asio::buffer(output_shell(session, content)), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});
        //std::regex end_line{".*% .*"};
        if(content.find("% ") != std::string::npos){
          std::string command;
          std::getline(file, command);
          command = command + '\n';


          // To transmit complete command of a line
          boost::asio::async_write(connection_ptr->connection_socket,
            boost::asio::buffer(output_command(session, command)), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});

          boost::asio::async_write(tcp_socket_master,
            boost::asio::buffer(command), [] (const boost::system::error_code& error, std::size_t bytes_transferred){});

          //tcp_socket_master.write_some(boost::asio::buffer(command));

        }

        do_read_from_npshell();
      }    
    
    });
}
int main(int argc, char * argv[]){
    //std::cout<<get_panel_cgi()<<std::endl;
    //boost::asio::io_service ioservice;
    http_server server((unsigned short)atoi(argv[1]));

    server.run();

}