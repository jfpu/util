#ifndef _NO_COPY_H_
#define _NO_COPY_H_

namespace util {

class NoCopy {
  NoCopy(const NoCopy&);
  NoCopy& operator=(const NoCopy&);
 public:
  NoCopy() {}
  virtual ~NoCopy() {}
#if 0
 public:
  NoCopy() = default;
  ~NoCopy() = default;
  NoCopy(const NoCopy&) = delete;
  NoCopy& operator=(const NoCopy&) = delete;
#endif
};

}

#endif
