#pragma once

#include "types.h"

namespace foundation
{
	// [MikeLest] TODO: Implement Perfect Murmur hashing for compile-time known keys
	// http://bitsquid.blogspot.com/2009/09/simple-perfect-murmur-hashing.html

	/// Implementation of the 64 bit MurmurHash2 function
	/// http://murmurhash.googlepages.com/
	uint64_t murmur_hash_64(const void *key, uint32_t len, uint64_t seed);
}