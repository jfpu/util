#ifndef _THREAD_H_
#define _THREAD_H_

#include <iostream>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <exception>

#include "runable.h"
#include "no_copy.h"

namespace util {

// Success  0x0000
#define SYS_SUCCESS 0
#define RET_SUCCESS SYS_SUCCESS

#if defined __GNUC__
#define likely(x)     __builtin_expect(!!(x), 1)
#define unlikely(x)   __builtin_expect(!!(x), 0)
#else
#define likely(x)     (x)
#define unlikely(x)   (x)
#endif
  
class Thread : public Runable {
  pthread_t thread_;
  bool createRunnerThread_;     // Whether main routine create runner thread

  static void* __ThreadExecute(void* data) {
    Runable* runner = reinterpret_cast<Runable*>(data);
  
    try {
      if (likely(NULL != runner)) {
        runner->Execute();
      }
    } catch(std::exception& e) {
      std::cout << e.what() << std::endl;
      return NULL;
    }
  
    return NULL;
  }

 public:
  Thread() : Runable(), createRunnerThread_(false) {}

  virtual ~Thread() {
    Stop();
  }

  void Run() {
    if (unlikely(RET_SUCCESS != pthread_create(&thread_, NULL, &__ThreadExecute, this))) {
      throw strerror(errno);
      return;
    }

    createRunnerThread_ = true;
  }

  void Stop() {
    if (unlikely(!createRunnerThread_ || Stopped()))
      return;

    // In order to store valid thread id after thread was canceled
    pthread_t th = thread_;
    pthread_cancel(thread_);
    pthread_join(th, NULL);

    createRunnerThread_ = false;
  }

  // The actual running function
  virtual void RealExecute() = 0;
};

}

#endif
