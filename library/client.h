#pragma once

#include "queue.h"
#include "connection.h"
#include "message.h"

#include <asio/ip/address.hpp>
#include <memory>

#include <asio.hpp>

namespace net_frame{

  template<typename T>
  class client_intefrace
  {
  public:
    client_intefrace() : m_socket(m_context)
    {

    }

    virtual ~client_intefrace()
    {
      Disconnect();
    }
    
  public:
    using message = net_frame::message<T>;

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
        m_connection = std::make_unique<connection<T>>(
            connection<T>::owner::client,
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

    tsqueue<owned_message<T>>& Incoming()
    {
      return m_qMessagesIn;
    }

    void Send(message& msg)
    {
      m_connection->Send(msg);
    }
    
  protected:
    // context for handling data transfer
    asio::io_context m_context;
    // thread for the context to execute in separately from other stuff
    std::thread thrContext;
    // hardware socket that is connected to the interface
    asio::ip::tcp::socket m_socket;
    // instance of connection object, whitch handles data trasfer
    std::unique_ptr<connection<T>> m_connection;
  private:
    // This is the thread safe queue of incoming messages from the server
    tsqueue<owned_message<T>> m_qMessagesIn;
  };
}