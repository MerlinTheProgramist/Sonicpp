#pragma once

#include "client.h"
#include "message.h"
#include "queue.h"
#include "server.h"
#include <iterator>
#include <memory>
#include <system_error>

#include <asio.hpp>

#define DEBUG

namespace sonicpp{

  // Forward declare
  template<typename T>
  class ServerInterface;
  template<typename T>
  class ClientIntefrace;
  
  template<typename T>
  class Connection : public std::enable_shared_from_this<Connection<T>>
  {
    friend ServerInterface<T>;
    friend ClientIntefrace<T>;

  public:
    enum class Owner
    {
      Server,
      Client
    };

    Connection(Owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn);
    
    virtual ~Connection(){}

    uint32_t GetID() const {return id;}
    
  private:
    void ConnectToClient(sonicpp::ServerInterface<T>* server, uint32_t uid = 0);
    void ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints);
    void Disconnect();
    bool IsConnected() const;
    void Send(const Message<T>& msg);
    
  private:
    // @ASYNC - Prime context ready to read a message header
    bool ReadHeader();
    // @ASYNC
    void ReadBody();
    // @ASYNC
    void WriteHeader();
    // @ASYNC
    void WriteBody();
    void AddToIncomingMessageQueue();
    // Encrypt data
    uint64_t scramble(uint64_t nInput);
    void WriteValidation();
    void ReadValidation(ServerInterface<T>* server = nullptr);
    
  protected:
    // Each connection has a unique socket to a remote 
    asio::ip::tcp::socket m_socket;

    // This context is shared with the while asio instance
    asio::io_context& m_asioContext;

    // This queue holds all messages to be sent to the remote side
    // of this connection
    tsqueue<Message<T>> m_qMessagesOut;


    // This queue holds all messages that have been recieved from
    // the remote side of this connection, Noteit is a reference
    // as the "owner" of this conneciton is expected to provide it
    tsqueue<owned_message<T>>& m_qMessagesIn;
    Message<T> m_msgTemporaryIn;
    // The owner decides how some of hte connection behaves
    const Owner m_nOwnerType = Owner::Server;
    uint32_t id = 0;

    // Handshake validation
    uint64_t m_nHandshakeOut = 0;
    uint64_t m_nHandshakeIn = 0;
    uint64_t m_nHandshakeCheck = 0;
  };

  // -------------------
  //  IMPLEMENTATIONS
  // -------------------
  
  template<typename T>
  Connection<T>::Connection(Owner parent, asio::io_context& asioContext, asio::ip::tcp::socket socket, tsqueue<owned_message<T>>& qIn)
    : m_socket(std::move(socket)), 
      m_asioContext(asioContext), 
      m_qMessagesIn(qIn),
      m_nOwnerType(parent)
  {
    if(m_nOwnerType == Owner::Server)
    {
      // Generate random handshake 
      m_nHandshakeOut = std::rand();
      // Precalculate the result 
      m_nHandshakeCheck = scramble(m_nHandshakeOut);
    }
    else
    {
      m_nHandshakeOut = 0;
      m_nHandshakeIn = 0;
    }
  }

  template<typename T>
  void Connection<T>::ReadValidation(ServerInterface<T>* server)
  {
    asio::async_read(m_socket, asio::buffer(&m_nHandshakeIn, sizeof(m_nHandshakeIn)),
      [this, server](std::error_code ec, std::size_t length)
      {
        if(!ec)
        {
          if(m_nOwnerType == Owner::Server)
          {
            // If client send correct anwser
            if(m_nHandshakeIn == m_nHandshakeCheck)
            {
              // Client has provided validation solution
              std::cout << "[SERVER] Client Validated" << std::endl;
              server->OnClientValidated(this->shared_from_this());

              // now prime the Read
              if(!ReadHeader())
                server->KickClient(this->shared_from_this());
            }
            else
            {
              std::cout << "Client Disconnected (Fail Validation)" << std::endl;
              server->KickClient(this->shared_from_this());
            }
          }
          else // if client
          {
            // Decode a the validation message
            m_nHandshakeOut = scramble(m_nHandshakeIn);
            // Send the result 
            WriteValidation();
          }
        }
        else
        {
          std::cerr << "Client Disconnected (ReadValidation)" << std::endl; 
          m_socket.close();
        }
      });
  }

    template<typename T>
    void Connection<T>::ConnectToClient(sonicpp::ServerInterface<T>* server, uint32_t uid)
    {
      if(m_nOwnerType == Owner::Server)
      {
        if(m_socket.is_open())
        {
          id = uid;
          // send handshake data to validate incoming connection is a valid client app
          WriteValidation();

          // prime wait for the connected client to anwser the validation
          ReadValidation(server);
        }
        else
          std::cerr << "[SERVER] Couldn't connect to client" << std::endl;
        
      }
    }
    template<typename T>
    void Connection<T>::ConnectToServer(const asio::ip::tcp::resolver::results_type& endpoints)
    {
      // Only clients can connect to servers
      if(m_nOwnerType == Owner::Client)
      {
        asio::async_connect(m_socket, endpoints,
          [this](std::error_code ec, asio::ip::tcp::endpoint endpoint)
          {
            if(!ec)
            {
              // Prime reading messages
              ReadValidation();
            }
            else
            {
              std::cerr << "[" << id << "] Connection to server Failed!" << std::endl;
            }
          }
        );
      }
    }
    
    template<typename T>
    void Connection<T>::Disconnect()
    {
      asio::post(m_asioContext,
        [this]()
        {
          m_socket.close();  
        });
    }
    template<typename T>
    bool Connection<T>::IsConnected() const
    {
      return m_socket.is_open();
    }

  
    template<typename T>
    void Connection<T>::Send(const Message<T>& msg)
    {
      asio::post(m_asioContext,
        [this, msg]()
        {
          bool bWritingMessage = !m_qMessagesOut.is_empty();
          m_qMessagesOut.push_back(msg);
          // Call writeHeader only if no onter messsages are processed now
          if(!bWritingMessage)
          {
            WriteHeader();
          }
        }  
      );  
    }
    template<typename T>
    bool Connection<T>::ReadHeader()
    {
      bool success{true};
      asio::async_read(m_socket, asio::buffer(&m_msgTemporaryIn.header, sizeof(message_header<T>)),
        [this, &success](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            if(m_msgTemporaryIn.header.size > 0)
            {
              m_msgTemporaryIn.body.resize(m_msgTemporaryIn.header.size);
              ReadBody();
            }
            else
            {
              // if no body, just add it as complete
              AddToIncomingMessageQueue();
            }
          }
          else
          {
            std::cout << "[" << id << "] Read Header Fail." << std::endl;
            Disconnect();
            success = false;
          }
        });
      return success;
    }
    template<typename T>
    void Connection<T>::ReadBody()
    {
      asio::async_read(m_socket, asio::buffer(m_msgTemporaryIn.body.data(), m_msgTemporaryIn.body.size()),
        [this](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            // add to quue as complete read message
            AddToIncomingMessageQueue();
          }
          else
          {
            std::cout << "[" << id << "] Read Body Fail!" << std::endl;
            m_socket.close();
          }
        });
    }

    
    // @ASYNC
    template<typename T>
    void Connection<T>::WriteHeader()
    {
        asio::async_write(m_socket, asio::buffer(&m_qMessagesOut.front().header, sizeof(message_header<T>)),
        [this](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            if(m_qMessagesOut.front().body.size() > 0)
            {
              WriteBody();
            }
            else
            {
              m_qMessagesOut.pop_front(); 

              if(!m_qMessagesOut.is_empty())
                WriteHeader();
            }
          }
          else
          {
            std::cout << "[" << id << "] Write Header Fail!" << std::endl;
            m_socket.close();
          }
        });
    }

    // @ASYNC
    template<typename T>
    void Connection<T>::WriteBody()
    {
        asio::async_write(m_socket, asio::buffer(m_qMessagesOut.front().body.data(), m_qMessagesOut.front().body.size()),
        [this](std::error_code ec, std::size_t length)
        {
          if(!ec)
          {
            m_qMessagesOut.pop_front(); 

            if(!m_qMessagesOut.is_empty())
              WriteHeader();
          }
          else
          {
            std::cout << "[" << id << "] Write Body Fail!" << std::endl;
            m_socket.close();
          }
        });
    }

    template<typename T>
    void Connection<T>::AddToIncomingMessageQueue()
    {
      if(m_nOwnerType == Owner::Server)
        m_qMessagesIn.push_back({this->shared_from_this(), m_msgTemporaryIn});
      else // owner == client
        // clients have only one connection so dont specify connection
        m_qMessagesIn.push_back({nullptr, m_msgTemporaryIn}); 

      // prime to read another header
      ReadHeader();
    }

    // Encrypt data
    template<typename T>
    uint64_t  Connection<T>::scramble(uint64_t nInput)
    {
      uint64_t out = nInput ^ 0x8084A1AEB05774E3;
      out = (out & 0xAFD6C6A5A4DA07B7) >> 4 | (out & 0xB2C0E02AA82ECB8F) << 4;
      return out ^ 0xDC91D19D4907AFE9;
    }
    
    template<typename T>
    void Connection<T>::WriteValidation()
    {
      asio::async_write(m_socket, asio::buffer(&m_nHandshakeOut, sizeof(m_nHandshakeOut)),
      [this](std::error_code ec, std::size_t length)
      {
          if(!ec)
          {
            // Validation sent, clients should sit and wait for a response, or a closure
            if(m_nOwnerType == Owner::Client)
              ReadHeader();
          }
          else
          {
            std::cout << "[" << id << "] Write Validation Fail!" << std::endl;
            m_socket.close();
          }
      });
    }
}


