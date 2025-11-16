// File  : RegisterFile.h
// Author: Cole Schwandt

#ifndef REGISTER_FILE_H
#define REGISTER_FILE_H

#include <cstdint>
#include <cassert>

class RegisterFile
{
  public:
    // structors
    RegisterFile()
    {
        reset();
    }

    // register getters
    uint32_t readU(uint8_t i) const
    {
        assert(i < 32);
        return x_[i];
    }

    int32_t readS(uint8_t i) const
    {
        return static_cast<int32_t>(readU(i));
    }

    // register setters
    void writeU(uint8_t i, uint32_t v)
    {
        assert(i < 32);
        
        if (i != 0) // silently prevent overwrite of $0
            x_[i] = v;
    }

    void writeS(uint8_t i, int32_t v)
    {
        writeU(i, static_cast< uint32_t >(v));
    }

    // hi/lo getters
    uint32_t hiU() const { return hi_; }
    int32_t  hiS() const { return static_cast< int32_t >(hi_); }
    uint32_t loU() const { return lo_; }
    int32_t  loS() const { return static_cast< int32_t >(lo_); }

    // hi/lo setters
    void write_hiU(uint32_t v) { hi_ = v; }
    void write_hiS(int32_t v)  { hi_ = static_cast< uint32_t >(v); }
    void write_loU(uint32_t v) { lo_ = v; }
    void write_loS(int32_t v)  { lo_ = static_cast< uint32_t >(v); }
    
    // init
    void reset()
    {
        for (auto & r : x_)
        {
            r = 0;
        }
        hi_ = lo_ = 0;
    }
    
  private:
    uint32_t x_[32];
    uint32_t hi_;
    uint32_t lo_;
};

#endif
