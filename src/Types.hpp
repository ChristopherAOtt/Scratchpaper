#pragma once

#include <stdint.h>

typedef uint8_t  Bytes1;
typedef uint16_t Bytes2;
typedef uint32_t Bytes4;
typedef uint64_t Bytes8;

// Petty? Yes. 
typedef uint8_t  Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int8_t   Int8;
typedef int16_t  Int16;
typedef int32_t  Int32;
typedef int64_t  Int64;

typedef Int32 Leniency;
constexpr Leniency IMMEDIATE_ACTION_REQUIRED = 0;  // No leniency. The job cannot be deferred. 

typedef Uint64 Fingerprint;
constexpr Fingerprint INVALID_FINGERPRINT = 0;
