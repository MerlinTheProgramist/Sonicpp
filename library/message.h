#pragma once

#include <cstdint>
#include <iostream>
#include <asio.hpp>
#include <chrono>
#include <type_traits>


namespace net_frame
{
  template <typename T>
  struct message_header
  {
      T id{};
      // size of the ertire message including this header
      uint32_t size = 0;
  };

  template <typename T>
  struct message
  {
    message_header<T> header{};
    std::vector<uint8_t> body;
    

    // print message
    friend std::ostream& operator<<(std::ostream& os, const message<T>& msg)
    {
      os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
      return os;
    }

    // pipe any data inside
    template<typename DataType>
    friend message<T>& operator<<(message<T>& msg, const DataType& data)
    {
      // Check if datatype can be serialized
      static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be serialized");
      
      size_t prev_size = msg.body.size();

      // resize vector
      msg.body.resize(msg.body.size() + sizeof(DataType));

      // copy data to vector
      std::memcpy(msg.body.data() + prev_size, &data, sizeof(DataType));

      // update message size
      msg.header.size = msg.body.size();

      return msg;
    }

    // extract data from message
    template<typename DataType>
    friend message<T>& operator>>(message<T>& msg, DataType& data)
    {
      // Check if datatype can be serialized
      static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be serialized");
      assert(msg.body.size()>= sizeof(DataType) && "Cant extract this datatype");
      
      size_t new_size = msg.body.size() - sizeof(DataType);

      // copy data
      std::memcpy(&data, msg.body.data() + new_size, sizeof(DataType));

      // update vector size 
      msg.body.resize(new_size);

      // update header
      msg.header.size = msg.body.size();

      return msg;
    }
  };

  // Forward declare the connection
  template<typename T>
  class connection;
  
  template<typename T>
  struct owned_message
  {
    std::shared_ptr<connection<T>> remote = nullptr;
    message<T> msg;
    
    // overloaf print message
    friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
    {
      os << msg;
      return os;
    }
  }; 

}
