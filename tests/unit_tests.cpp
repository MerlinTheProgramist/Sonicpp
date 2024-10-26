#include "../library/client.h"
#include "../library/server.h"

#include "gtest/gtest.h"
#include <cstdint>
#include <gtest/gtest.h>
#include <optional>
#include <queue>
#include <thread>

namespace {
  using namespace std::literals;
  const std::chrono::duration<float> LOCAL_MAX_TIMEOUT = 500ms;
  enum class TestMessageType{
    UwU, 
    MakeMeCoffee,
    ForTheEmperor
  };
  using Message = sonicpp::Message<TestMessageType>;
  class TestClient : public sonicpp::ClientIntefrace<::TestMessageType>{
  public:
    std::optional<Message> get_msg(std::chrono::duration<float> timeout = LOCAL_MAX_TIMEOUT){
      if(auto msg = AwaitNextMessage(timeout)){
        return msg;
      }
      return std::nullopt;
    }
  };
  class TestServer : public sonicpp::ServerInterface<::TestMessageType>{
  public:
    TestServer(uint16_t p):sonicpp::ServerInterface<::TestMessageType>(p){}
    size_t client_count{0};
    std::unordered_map<uint32_t, std::queue<::Message>> messages_q{};
  private:
    bool OnClientConnect(std::shared_ptr<Connection> client) override{
      return true;
    }
    void OnClientDisconnect(std::shared_ptr<Connection> client) override{
      client_count--;
    }
    void OnClientValidated(std::shared_ptr<Connection> client) override{
      client_count++;
      std::cout << "Client connected!" << std::endl;
    }

    void OnMessage(std::shared_ptr<Connection> client, Message &msg) override{
      messages_q[client->GetID()].push(msg);
      if(msg.GetType()==TestMessageType::MakeMeCoffee){
        Message msg_ans{TestMessageType::ForTheEmperor}; 
        MessageClient(client, msg_ans);
      }
    }
  };
  class CommunicationTest : public testing::Test{
  protected:
    const uint16_t PORT = 8080;
    ::TestClient test_client{};
    ::TestServer test_server{PORT};

  };
  TEST_F(CommunicationTest, Connecting){
    EXPECT_TRUE(test_client.Connect("127.0.0.1", PORT));
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(test_server.client_count, 1);
    EXPECT_EQ(test_server.m_deqConnections.back()->GetID(), *test_client.GetID());
    test_client.Disconnect();
  }
  TEST_F(CommunicationTest, Server_sending_message){
    EXPECT_TRUE(test_client.Connect("127.0.0.1", PORT));
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(test_server.m_deqConnections.size(), 1);
    EXPECT_EQ(test_server.m_deqConnections.back()->GetID(), *test_client.GetID());

    ::Message sent_msg{TestMessageType::UwU};
    test_server.MessageAllClients(sent_msg);

    ::Message recv_msg;
    EXPECT_NO_THROW(recv_msg = test_client.get_msg().value());
    EXPECT_EQ(recv_msg.GetType(), TestMessageType::UwU);
  }
  TEST_F(CommunicationTest, Client_sending_message){
    EXPECT_TRUE(test_client.Connect("127.0.0.1", PORT));
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(test_server.m_deqConnections.size(), 1);
    EXPECT_EQ(test_server.m_deqConnections.back()->GetID(), *test_client.GetID());

    ::Message sent_msg{TestMessageType::UwU};
    test_client.Send(sent_msg);

    ::Message recv_msg;
    EXPECT_NO_THROW(recv_msg = test_server.messages_q[*test_client.GetID()].back());
    EXPECT_EQ(recv_msg.GetType(), TestMessageType::UwU);
  }
  TEST_F(CommunicationTest, Anwsering){
    EXPECT_TRUE(test_client.Connect("127.0.0.1", PORT));
    std::this_thread::sleep_for(300ms);
    EXPECT_EQ(test_server.m_deqConnections.size(), 1);
    EXPECT_EQ(test_server.m_deqConnections.back()->GetID(), *test_client.GetID());

    ::Message sent_msg{TestMessageType::MakeMeCoffee};
    test_client.Send(sent_msg);

    ::Message recv_msg;
    EXPECT_NO_THROW(recv_msg = test_client.get_msg().value());
    EXPECT_EQ(recv_msg.GetType(), TestMessageType::ForTheEmperor);
  }
}
