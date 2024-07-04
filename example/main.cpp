
#include <asio.hpp>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>

#include <iostream>
#include <chrono>
#include <thread>

std::vector<uint8_t> vBuffer(20 * 1024);

void GrabSomeData(asio::ip::tcp::socket& socket)
{
  socket.async_read_some(asio::buffer(vBuffer.data(), vBuffer.size()), 
    [&](std::error_code ec, std::size_t length){
        if(ec)
        {
          std::cout << "Error occured while grabbing data: " << std::endl << ec.message() << std::endl; 
          return;
        }

        std::cout << "Data Received:" << std::endl;
        for(int i=0;i<length;++i)
          std::cout << vBuffer[i];
        GrabSomeData(socket);
    }
  );
}

int main()
{
  asio::error_code ec;

   
  asio::io_context context;

  // create work for the context to not exit
  asio::io_context::work idleWork(context);

  // create new thread for the context to run in, so it dont hold the main execution
  std::thread thrContext = std::thread([&](){ context.run();});

  // create endpoint
  asio::ip::tcp::endpoint endpoint(asio::ip::make_address("192.168.0.102", ec), 80);

  asio::ip::tcp::socket socket(context);
  socket.connect(endpoint);

  if(!ec)
  {
    std::cout << "Connected" << std::endl;
  }else
    std::cout << "Failed to connect: " << ec.message() << std::endl;

  if(socket.is_open())
  {
    GrabSomeData(socket);
     std::string Request = 
      "GET /admin/login.php HTTP/1.1\r\n"
      "Host: pikachuszek\r\n"
      "Connection: close\r\n\r\n";
    
    socket.write_some(asio::buffer(Request.data(), Request.size()), ec);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(20s);
        
  }
}
