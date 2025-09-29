// File  : RegisterFile.h
// Author: Cole Schwandt

#ifndef REGISTER_FILE
#define REGISTER_FILE

#include <cstdint>

class RegisterFile
{
  public:
    int32_t operator[](int32_t i) const;
    int32_t & operator[](int32_t i);
    int32_t hi() const;
    int32_t & hi();
    int32_t lo() const;
    int32_t & lo();
  private:
    int32_t x_[32];
    int32_t hi_;
    int32_t lo_;
};
// shld any of these be uint32_t?

#endif
