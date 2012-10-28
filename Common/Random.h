#pragma once
#include "Types.h"

#define randInt() xor128() 

u32 xor128(void) {
  static u32 x = 123456789;
  static u32 y = 362436069;
  static u32 z = 521288629;
  static u32 w = 88675123;
  u32 t;
 
  t = x ^ (x << 11);
  x = y; y = z; z = w;
  return w = w ^ (w >> 19) ^ (t ^ (t >> 8));
}