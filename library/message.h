#pragma once

#include <asio/generic/datagram_protocol.hpp>
#include <cstddef>
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
  struct Message
  {
    message_header<T> header{};
    std::vector<uint8_t> body{};

    Message() = default;
    Message(T id):header{id,0}{}

    T GetType() const {
      return this->header.id;
    }    
    
    // print message
    friend std::ostream& operator<<(std::ostream& os, const Message<T>& msg)
    {
      os << "ID:" << int(msg.header.id) << " Size:" << msg.header.size;
      return os;
    }

    // pipe any data inside
    template<typename DataType>
    friend Message<T>& operator<<(Message<T>& msg, const DataType& data)
    {
      // DataType cannot be a string
      static_assert(std::is_same<std::string, std::decay<DataType>>::value == false, "String has its own implementation, this is an misuse");
      
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

    // same as above but only for std::string
    friend Message<T>& operator<<(Message<T>& msg, const std::string& str)
    {
      // std::cout << "accual string size: " << str.size() << std::endl;
      size_t prev_size = msg.body.size();
      // resize vector
      msg.body.resize(msg.body.size() + str.size() + sizeof(std::string::size_type));
      // copy data to vector
      std::memcpy(msg.body.data() + prev_size, str.c_str(), str.size());
      // additionaly add size_t indicating string size
      auto str_size = str.size();
      std::memcpy(msg.body.data() + prev_size + str.size(), &str_size, sizeof(std::string::size_type));
      // update message size
      msg.header.size = msg.body.size();

      // std::cout << "send string size: " << *(size_t*)(msg.body.data() + msg.body.size() - sizeof(std::string::size_type)) << std::endl;
      
      return msg;
    }
    
    // same as above but only for std::vector
    template<typename Type>
    friend Message<T>& operator<<(Message<T>& msg, const std::vector<Type>& vec)
    {
      using size_type = typename std::vector<Type>::size_type;
      size_t prev_size = msg.body.size();

      size_t vec_byte_size = vec.size()*sizeof(Type);
      // resize vector
      msg.body.resize(msg.body.size() + vec_byte_size + sizeof(size_type));
      // copy data to vector
      std::memcpy(msg.body.data() + prev_size, vec.data(), vec_byte_size);
      // additionaly add size_t indicating string size
      size_type vec_size = vec.size();
      std::memcpy(msg.body.data() + prev_size + vec_byte_size, &vec_size, sizeof(size_type));
      // update message size
      msg.header.size = msg.body.size();
      
      return msg;
    }

    // extract data from message
    template<typename DataType>
    friend Message<T>& operator>>(Message<T>& msg, DataType& data)
    {
      // DataType cannot be a string
      static_assert(std::is_same<std::string, std::decay<DataType>>::value == false, "String has its own implementation, this is an misuse");

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
    // same as above but only for std::string
    friend Message<T>& operator>>(Message<T>& msg, std::string& str)
    {
      // firstly extract size of the string
      std::string::size_type str_size = *(std::string::size_type*)(msg.body.data() + msg.body.size() - sizeof(std::string::size_type));
      // std::cout << "string size: " << str_size << std::endl;
      size_t new_size = msg.body.size() - str_size - sizeof(std::string::size_type);
      // std::cout << "new_size: " << new_size << std::endl;
      
      // copy data
      // str.resize(str_size);
      // str.c_str() = *(msg.body.data() + new_size);
      str = std::string(reinterpret_cast<const char*>(msg.body.data() + new_size), str_size);

      // update vector size 
      msg.body.resize(new_size);

      // update header
      msg.header.size = msg.body.size();

      return msg;
    }
    
    // same as above but only for std::vector
    template<typename Type>
    friend Message<T>& operator>>(Message<T>& msg, std::vector<Type>& vec)
    {
      using size_type = typename std::vector<Type>::size_type;
      // firstly extract size of the vector
      size_type vec_size = *(size_type*)(msg.body.data() + msg.body.size() - sizeof(size_type));
      // std::cout << "vector size: " << vec_size << std::endl;
      size_t new_size = msg.body.size() - vec_size*sizeof(Type) - sizeof(size_type);
      // std::cout << "new_size: " << new_size << std::endl;
      
      // copy data
      // str.resize(str_size);
      // str.c_str() = *(msg.body.data() + new_size);
      vec.resize(vec_size);
      std::memcpy(vec.data(), msg.body.data() + new_size, vec_size*sizeof(Type));
      // vec = std::vector<Type>(reinterpret_cast<const Type*>(msg.body.data() + new_size), reinterpret_cast<const Type*>(msg.body.data() + new_size + vec_size));

      // update vector size 
      msg.body.resize(new_size);

      // update header
      msg.header.size = msg.body.size();

      return msg;
    }
  };

  // Forward declare the connection
  template<typename T>
  class Connection;
  
  template<typename T>
  struct owned_message
  {
    std::shared_ptr<Connection<T>> remote = nullptr;
    Message<T> msg;
    
    // overloaf print message
    friend std::ostream& operator<<(std::ostream& os, const owned_message<T>& msg)
    {
      os << msg;
      return os;
    }
  }; 

}
