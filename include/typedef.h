#pragma once

#include <cstdint>
#include <cstdlib>

#include <string>
#include <vector>
#include <tuple>
#include <memory>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

using std::pair;
using std::tuple;

using std::string;
using std::string_view;
using std::vector;

using std::shared_ptr;
using std::unique_ptr;
using std::weak_ptr;

using std::make_pair;
using std::make_shared;
using std::make_tuple;
using std::make_unique;

template <typename T>
using ptr = T*;

struct Object;
using Obj = ptr<Object>;

template <typename T>
using ObjPtr = ptr<T>;

template <typename T, typename U>
ptr<T> ptrcast(ptr<U> p) {
  return std::reinterpret_pointer_cast<T>(p);
}

template <typename T>
using Vec = std::vector<T>;

using StringVector = Vec<string>;
