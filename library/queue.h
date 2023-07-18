#pragma once

#include <condition_variable>
#include <mutex>
#include <deque>
#include <thread>


namespace net_frame
{

  template<typename T>
  class tsqueue
  {
  public:
    tsqueue() = default;
    tsqueue(const tsqueue<T>&) = delete;
    virtual ~tsqueue() { clear(); }

    const T& front()
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      return deqQueue.front();
    }
    const T& back()
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      return deqQueue.back();
    }
    void push_back(const T& item)
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      deqQueue.push_back(item);
      
      std::unique_lock<std::mutex> ul(muxBlocking);
      cvBlocking.notify_one();
    }
    
    void push_front(const T& item)
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      deqQueue.push_front(item);

      std::unique_lock<std::mutex> ul(muxBlocking);
      cvBlocking.notify_one();
    }
    T pop_back()
    {
      auto item = back();
      std::lock_guard<std::mutex> lock(muxQueue);
      deqQueue.pop_back();
      return item;
    }
    
    T pop_front()
    {
      auto item = front();
      std::lock_guard<std::mutex> lock(muxQueue);
      deqQueue.pop_front();
      return item;
    }
    
    size_t count()
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      return deqQueue.size();
    }
    
    bool is_empty()
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      return deqQueue.empty();
    }
    
    void clear()
    {
      std::lock_guard<std::mutex> lock(muxQueue);
      deqQueue.clear();
    }

    void wait()
    {
      while(is_empty())
      {
        // wait until push to queue
        std::unique_lock<std::mutex> ul(muxBlocking);
        cvBlocking.wait(ul);
      }
    }
  
  protected:
    std::mutex muxQueue;
    std::deque<T> deqQueue;  

    std::condition_variable cvBlocking;
    std::mutex muxBlocking;
  };

}
