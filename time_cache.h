#ifndef _TIME_CACHE_H_
#define _TIME_CACHE_H_

#include <list>
#include <ext/hash_map>
#include <cstddef>
#include <cunistd>
#include <stdint.h>

#include "oss_thread_lock.h"
#include "thread.h"

using std::list;
using __gnu_cxx::hash_map;

using wbl::thread::CGuard;
using wbl::thread::CMutex;

namespace tre {
namespace server {

const uint32_t k_expire_secs = 60;
const uint32_t k_bucket_num = 10;

// Key: Adid. Value: Timestamp
class TimeCacheMap : public Thread {
  typedef hash_map<uint64_t, uint64_t> Adid_Time_Map;
  typedef list<Adid_Time_Map*> Buckets;

  uint32_t num_;
  uint32_t expire_time_;
  CMutex mutex_;
  Buckets buckets_;
  Adid_Time_Map* deleted_map_;

  void RealExecute() {
    while (Running()) {
      pthread_testcancel();
      sleep(expire_time_);

      {
        CGuard<CMutex> lock(mutex_);

        if (NULL != deleted_map_) {
          delete deleted_map_;
          deleted_map_ = NULL;
        }

        deleted_map_ = buckets_.back();
        buckets_.pop_back();
        buckets_.push_front(new Adid_Time_Map());
      }
    }
  }

 public:
  TimeCacheMap()
    : num_(k_bucket_num), expire_time_(k_expire_secs), deleted_map_(NULL) {}

  explicit TimeCacheMap(uint32_t numBuckets, uint32_t expireSecs)
    : num_(numBuckets), expire_time_(expireSecs), deleted_map_(NULL) {}

  void Init(uint32_t numBuckets = num_) {
    buckets_.assign(numBuckets, NULL);
    for (uint32_t i = 0; i < numBuckets; ++i) {
      Adid_Time_Map* pmap = new Adid_Time_Map();
      buckets_.push_front(pmap);
    }

    Start();
  }

  void Start() {
    Run();
  }

  void Terminate() {
    Stop();
  }

  bool ContainsKey(uint64_t key) const {
    CGuard<CMutex> lock(mutex_);

    Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      Adid_Time_Map* pmap = *it;

      if (pmap->end() != pmap->find(key))
        return true;
    }

    return false;
  }

  uint64_t Get(uint64_t key) const {
    CGuard<CMutex> lock(mutex_);

    Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      Adid_Time_Map* pmap = *it;

      Adid_Time_Map::iterator it_map = pmap->find(key);
      if (pmap->end() != it_map)
        return it_map->second;
    }

    return 0;
  }

  void Insert(uint64_t key, uint64_t value) {
    CGuard<CMutex> lock(mutex_);

    Buckets::iterator it = buckets_.begin();
    (*it)[key] = value;

    ++it;
    for (; it != buckets_.end(); ++it) {
      it->erase(key);
    }
  }

  size_t Size() {
    CGuard<CMutex> lock(mutex_);

    size_t size = 0;
    Buckets::iterator it = buckets_.begin();
    for (; it != buckets_.end(); ++it) {
      size += it->size();
    }

    return size;
  }

  void Oldest(Adid_Time_Map& map) const {
    CGuard<CMutex> lock(mutex_);

    return map = *deleted_map_;
  }
};

}
}

#endif
