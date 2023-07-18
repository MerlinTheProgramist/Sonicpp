#pragma once

#include "message.h"
#include "queue.h"
#include "connection.h"

#include <fcntl.h>
#include <limits>
#include <memory>
#include <system_error>

namespace net_frame{

  template<typename T>
  class server_interface
  {
  public:
    using message = net_frame::message<T>;
    using Connection = net_frame::connection<T>;
    
    server_interface(uint16_t port)
      : m_asioAcceptor(m_asioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
    {
      
    }

    virtual ~server_interface()
    {
      Stop();
    }

    bool Start()
    {
      try{
        // give work of waiting for connection
        waitForClientConnection();

        m_threadContext = std::thread([this](){ m_asioContext.run();});
      }
      catch(std::exception& e)
      {
        std::cerr << "[SERVER] Exception: " << e.what() << std::endl;
        return false;
      }

      std::cout << "[SERVER] Started!" << std::endl;
      return true;
    }

    void Stop()
    {
      // Request context to close
      m_asioContext.stop();

      // Tidy up the context thread
      if(m_threadContext.joinable()) m_threadContext.join();
      
      std::cout << "[SERVER] Stopped!" << std::endl;
    }
    
    //@ASYNC - wait for connection
    void waitForClientConnection()
    {
      m_asioAcceptor.async_accept(
      [this](std::error_code ec, asio::ip::tcp::socket socket)
      {
          if(!ec)
          {
            std::cerr << "[SERVER] New Connection: " << socket.remote_endpoint() << std::endl;

            std::shared_ptr<connection<T>> newconn = 
              std::make_shared<connection<T>>(
                connection<T>::owner::server, 
                m_asioContext, 
                std::move(socket), 
                m_qMessagesIn
            );

            // Give the user server a chance to deny connection
            if(OnClientConnect(newconn))
            {
              // Add connection to active connections
              m_deqConnections.push_back(std::move(newconn));

              // allocate id for the connection
              m_deqConnections.back()->ConnectToClient(this, nIDCounter++);
              
            }
            else
            {
              std::cout << "[------] Connection Denied" << std::endl;
            }
          }
          else
          {
            std::cerr << "[SERVER] New Connection Error: " << ec.message() << std::endl; 
          }

          waitForClientConnection();
      });
    }

    void MessageClient(std::shared_ptr<connection<T>> client, const message& msg)
    {
      if(client && client->IsConnected())
      {
        client->Send(msg);
      }
      else
      {
        std::cout << "[" << client->GetID() << "] Disconnected" << std::endl;
        OnClientDisconnect(client);
        client.reset();
        
        m_deqConnections.erase(
          std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
      }
    }
    
    void MessageAllClients(const message& msg, std::shared_ptr<connection<T>> pIgnoreClient = nullptr)
    {
      // Flag to indicate that some clients died, to remove them all at once from the collection
      bool bInvalidClientExits = false;
      for(auto& client : m_deqConnections)
      {
        if(client && client->IsConnected())
        {
          if(client == pIgnoreClient)
            continue;

          client->Send(msg);
        }
        else
        {
          std::cout << "[" << client->GetID() << "] Disconnected" << std::endl;
          // The client couldnt be contacted, so it has disconnected
          OnClientDisconnect(client);
          client.reset();
          bInvalidClientExits = true;
        }
      }

      if(bInvalidClientExits)
        m_deqConnections.erase(
          std::remove(m_deqConnections.begin(), m_deqConnections.end(), nullptr), m_deqConnections.end());
    }

    void Update(const size_t nMaxMessages = std::numeric_limits<size_t>::max(), bool bWait = false)
    {
      if(bWait) m_qMessagesIn.wait();
      
      size_t nMessageCount = 0;
      while(nMessageCount < nMaxMessages && !m_qMessagesIn.is_empty())
      {
        // Grab the front message
        owned_message<T> msg = m_qMessagesIn.pop_front();

        // Pass to message handler
        OnMessage(msg.remote, msg.msg);
        
        nMessageCount++;
      }
    }
    
  // Functions that could be obertitten by the derrived class
  protected:
    // Called when a new client connects 
    virtual bool OnClientConnect(std::shared_ptr<connection<T>> client)
    {return false;}
    // Called when a client disconnects
    virtual void OnClientDisconnect(std::shared_ptr<connection<T>> client)
    {}
    // Called when a message arrives
    virtual void OnMessage(std::shared_ptr<connection<T>> client, message& msg)
    {}
  public: 
    // 
    virtual void OnClientValidated(std::shared_ptr<connection<T>> client)
    {}

  protected:
    // Thread safe Queue of incoming message packets
    tsqueue<owned_message<T>> m_qMessagesIn;

    // Container of active validated connections
    std::deque<std::shared_ptr<connection<T>>> m_deqConnections;
    
    asio::io_context m_asioContext;
    std::thread m_threadContext;

    asio::ip::tcp::acceptor m_asioAcceptor;

    uint32_t nIDCounter = 10000;
  };
  
}