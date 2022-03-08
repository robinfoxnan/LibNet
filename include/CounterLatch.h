// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef LibNet_BASE_COUNTDOWNLATCH_H
#define LibNet_BASE_COUNTDOWNLATCH_H

#include <mutex>
#include <thread>
#include <condition_variable>
using namespace std;
namespace LibNet
{
  class CountDownLatch 
  {

  public:
    explicit CountDownLatch(int count) :  count_(count)
    {
    }

    void wait()
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (count_ > 0)
      {
        condition_.wait(lock);
      }
    }

    void countDown()
    {
     std::lock_guard<std::mutex> guard(mutex_);
      --count_;
      if (count_ == 0)
      {
        condition_.notify_all();;
      }
    }

    int getCount() const
    {
      std::lock_guard<std::mutex> guard(mutex_);
      return count_;
    }

  private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    int count_;
  };

} // namespace LibNet
#endif // LibNet_BASE_COUNTDOWNLATCH_H
