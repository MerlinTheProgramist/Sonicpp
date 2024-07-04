#pragma once

#include "queue.h"
#include "connection.h"
#include "message.h"
#include "server.h"

#include <asio/ip/address.hpp>
#include <memory>

#include <asio.hpp>
#include <optional>

namespace net_frame{

  template<typename T>
  class ClientIntefrace
  {
  protected:
    // specific Message alias
    using Message = net_frame::Message<T>;

    // context for handling data transfer
    asio::io_context m_context;
    // thread for the context to execute in separately from other stuff
    std::thread thrContext;
    // hardware socket that is connected to the interface
    asio::ip::tcp::socket m_socket;
    // instance of connection object, whitch handles data trasfer
    std::unique_ptr<Connection<T>> m_connection;
  private:
    // This is the thread safe queue of incoming messages from the server
    tsqueue<owned_message<T>> m_qMessagesIn;
    
  public:
    ClientIntefrace() : m_socket(m_context)
    {

    }

    virtual ~ClientIntefrace()
    {
      Disconnect();
    }
    
    // Connect socket
    bool Connect(const std::string& host, const uint16_t port)
    {
      try
      {          

        // Resolve hostname/ip-address accordingly
        asio::ip::tcp::resolver resolver(m_context);
        asio::ip::tcp::resolver::results_type endpoints = 
          resolver.resolve(host, std::to_string(port));

        // create connection
        m_connection = std::make_unique<Connection<T>>(
            Connection<T>::Owner::Client,
            m_context,
            asio::ip::tcp::socket(m_context), 
            m_qMessagesIn
          );  
        
        // Tell the connection object to connect to server
        m_connection->ConnectToServer(endpoints);

        // Start Context Thread
        thrContext = std::thread([this](){m_context.run();});
      }
      catch(std::exception& e)
      {  
        std::cout << "Client Expection: " << e.what() << std::endl;
        return false;
      }
      return IsConnected();
    }

    // Disconnect socket
    void Disconnect()
    {
      if(IsConnected())
      {
        m_connection->Disconnect();
      }

      // stop context
      m_context.stop();

      // stop thread
      if(thrContext.joinable())
        thrContext.join();

      // Destroy connection
      m_connection.release();      
    }
    
    // Check if client is still connected
    bool IsConnected() const 
    {
      if(m_connection)
        return m_connection->IsConnected();
      else
        return false; 
    }

    std::optional<Message> NextMessage()
    {
      return (m_qMessagesIn.is_empty())?
        std::nullopt :
        std::make_optional(m_qMessagesIn.pop_front().msg);
    }

    void Send(Message& msg)
    {
      m_connection->Send(msg);
    }
  };
}
