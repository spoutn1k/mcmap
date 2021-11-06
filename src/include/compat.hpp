#pragma once
#include <map>

/* Loosely inspired from lower_bound in
 * /usr/include/c++/11.1.0/bits/stl_algobase.h
 * Returns a const_iterator to the highest integer that is inferior or equal to
 * `version` from the keys of a map<int, F> */
template <typename F>
typename std::map<int, F>::const_iterator
compatible(const std::map<int, F> &hash, int version) {
  typedef typename std::map<int, F>::const_iterator _Iterator;
  typedef typename _Iterator::difference_type _DistanceType;

  _Iterator __first = hash.begin(), __end = hash.end();
  _DistanceType __len = std::distance(__first, __end);

  while (__len > 1) {
    _DistanceType __half = __len >> 1;
    _Iterator __middle = __first;
    std::advance(__middle, __half);

    if (__middle->first > version)
      __end = __middle;
    else
      __first = __middle;

    __len = __len - __half;
  }

  return __first;
}
