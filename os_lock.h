#ifndef _OS_LOCK_H_
#define _OS_LOCK_H_

#include <pthread.h>
#include <time.h>
#include <unistd.h>

namespace util {

class CMutex {
  pthread_mutex_t lock_;

 public:
  CMutex(pthread_mutexattr_t* attrs = 0) {
    pthread_mutex_init(&lock_,attrs);
  }

  ~CMutex() {
    pthread_mutex_destroy(&lock_);
  }

  int acquire() {
    return pthread_mutex_lock(&lock_);
  }

  int acquire(const struct timespec* _time) {
    return pthread_mutex_timedlock(&lock_, _time);
  }

  int release() {
    return  pthread_mutex_unlock(&lock_);
  }

  pthread_mutex_t& lock() {
    return lock_;
  }
};

class CNullMutex {
  int lock_;        // A dummy lock.

 public:
  CNullMutex (pthread_mutexattr_t* attrs = 0) : lock_(0) {}

  ~CNullMutex () {}

  int acquire() {
    return 0;
  }

  int acquire(const struct timespec* _time) {
    errno = ETIME;
    return -1;
  }

  int release() {
    return 0;
  }

  int& lock() {
    return lock_;
  }
};

template <class MyLOCK>
class CGuard {
  MyLOCK* m_lock;
  int m_owner;

  CGuard();
  CGuard (const CGuard<MyLOCK> &);
  void operator= (const CGuard<MyLOCK> &);

 public:
  CGuard(MyLOCK& lock) : m_lock(&lock) {
    acquire();
  }

  ~CGuard() {
    release();
  }    

  int acquire () {
    return m_owner = m_lock->acquire();
  }

  int release() {
    if(m_owner == -1)
      return -1;
    else {
      m_owner = -1;
      return m_lock->release();
    }
  }

  int locked() const {
    return m_owner != -1;
  }
};

template<>
class CGuard<CNullMutex> {
  CGuard();
  CGuard (const CGuard<CNullMutex> &);
  void operator= (const CGuard<CNullMutex> &);

 public:
  CGuard(CNullMutex& lock) {}

  ~CGuard() {}

  int acquire(){ 
    return 0;
  }

  int release() {
    return 0;
  }

  int locked() const {
    return 1;
  }
};


template <class MUTEX>
class CCondition {
  void operator= (const CCondition<MUTEX> &);
  CCondition (const CCondition<MUTEX> &);

 protected:
  pthread_cond_t cond_;
  MUTEX &mutex_;

 public:
  CCondition (MUTEX &m) : mutex_(m) {
    pthread_condattr_t attr;
    pthread_condattr_init(&attr);

    if (pthread_cond_init(&this->cond_, &attr) != 0) {
      printf("call pthread_cond_init failed\n");
    }

    pthread_condattr_destroy(&attr);
  }

  ~CCondition () {
    if (this->remove() == -1) {
      printf("remove CCondition failed\n");
    }
  }

  int wait () {
    return pthread_cond_wait (&this->cond_, &this->mutex_.lock());
  }

  int wait (const struct timespec* _time) {
    return pthread_cond_timedwait(&this->cond_, &this->mutex_.lock(),_time);
  }


  int signal (){
    return pthread_cond_signal (&this->cond_);
  }


  int broadcast (void){
    return pthread_cond_broadcast(&this->cond_);
  }

  int remove (void){
    int result = 0;

    while ((result = pthread_cond_destroy(&this->cond_)) == -1 && errno == EBUSY) {
      pthread_cond_broadcast (&this->cond_);
      usleep(10000);
    }

    return result;
  }

  MUTEX &mutex (void){
    return this->mutex_;
  }
};

}

#endif
