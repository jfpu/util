#ifndef _TIME_CACHE_H_
#define _TIME_CACHE_H_

#include <list>
#include <ext/hash_map>
#include <stddef.h>
#include <unistd.h>
#include <stdint.h>

#include "oss_thread_lock.h"
#include "thread.h"
#include "log.h"

using std::list;
using __gnu_cxx::hash_map;

using wbl::thread::CGuard;
using wbl::thread::CMutex;

using indexer::Thread;

namespace tre {
namespace server {

const uint32_t k_expire_secs = 60;
const uint32_t k_bucket_num = 10;

template<typename K= uint64_t, typename V = uint64_t>
class TimeCacheMap : public Thread {
  typedef hash_map<K, V> Time_Map;
  typedef list<Time_Map*> Buckets;

  uint32_t num_;
  uint32_t expire_time_;
  CMutex mutex_;
  Buckets buckets_;
  Time_Map* deleted_map_;

  void RealExecute() {
    while (Running()) {
      pthread_testcancel();
      sleep(expire_time_);

      {
        // LogInfo("TimeCacheMap timeout");
        CGuard<CMutex> lock(mutex_);

        if (NULL != deleted_map_) {
          deleted_map_->clear();
          delete deleted_map_;
          deleted_map_ = NULL;
        }

        deleted_map_ = buckets_.back();
        buckets_.pop_back();
        buckets_.push_front(new Time_Map());
      }
    }
  }

 public:
  TimeCacheMap()
    : num_(k_bucket_num), expire_time_(k_expire_secs), deleted_map_(NULL) {}

  explicit TimeCacheMap(uint32_t numBuckets, uint32_t expireSecs)
    : num_(numBuckets), expire_time_(expireSecs), deleted_map_(NULL) {}

  void Init(uint32_t timeout, uint32_t numBuckets) {
    if (timeout > 0) {
      expire_time_ = timeout;
    }

    if (numBuckets > 0) {
      num_ = numBuckets;
    }

    for (uint32_t i = 0; i < num_; ++i) {
      Time_Map* pmap = new Time_Map();
      if (NULL == pmap) {
        continue;
      }
      buckets_.push_front(pmap);
    }
    Start();
  }

  void Start() {
    Run();
  }

  void Terminate() {
    Stop();

    typename Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      Time_Map* pmap = *it;

      if (NULL != pmap) {
        pmap->clear();
        delete pmap;
      }
    }

    buckets_.clear();
  }

  void GetNewestData(Time_Map& tm) {
    CGuard<CMutex> lock(mutex_);
    if (buckets_.empty()) {
      LogInfo("newest blacklist is null");
      return;
    }

    typename Buckets::iterator it = buckets_.begin();
    Time_Map* pmap = *it;
    if (NULL == pmap) {
      LogInfo("map is null");
      return;
    }
    tm = *pmap;
  } 

  bool ContainsKey(K& key) {
    CGuard<CMutex> lock(mutex_);

    typename Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      Time_Map* pmap = *it;

      if (pmap->end() != pmap->find(key))
        return true;
    }

    return false;
  }

  bool Get(K& key, V& val) {
    CGuard<CMutex> lock(mutex_);

    typename Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      Time_Map* pmap = *it;

      typename Time_Map::iterator it_map = pmap->find(key);
      if (pmap->end() != it_map) {
        val = it_map->second;
        return true;
      }
    }
    return false;
  }

  void Insert(const K& key, const V& value) {
    CGuard<CMutex> lock(mutex_);

    typename Buckets::iterator it = buckets_.begin();
    Time_Map& map = **it;
    map[key] = value;
    ++it;
    for (; it != buckets_.end(); ++it) {
      (*it)->erase(key);
    }
  }

  void Erase(K& key) {
    CGuard<CMutex> lock(mutex_);
    typename Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      (*it)->erase(key);
    }
  }

  void Clean() {
    CGuard<CMutex> lock(mutex_);
    typename Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      (*it)->clear();
    }
  }

  size_t Size() {
    CGuard<CMutex> lock(mutex_);

    size_t size = 0;
    typename Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      size += (*it)->size();
    }

    return size;
  }

  void Oldest(Time_Map& map) {
    CGuard<CMutex> lock(mutex_);

    map = *deleted_map_;
  }
};

}
}

#endif
