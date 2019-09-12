#pragma once

#ifdef new
#undef new
#endif

#include <unordered_map>
using std::unordered_map;
using std::unordered_multimap;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif

#if defined(__GNUC__) && !defined(__clang__)
#include <experimental/unordered_map>

namespace std
{
using std::experimental::erase_if;
}
#else
namespace std
{
template <class Key, class T, class Hash, class KeyEqual, class Alloc, class Pred>
void erase_if(std::unordered_map<Key, T, Hash, KeyEqual, Alloc> & c, Pred pred)
{
  for (auto i = c.begin(), last = c.end(); i != last;)
  {
    if (pred(*i))
    {
      i = c.erase(i);
    }
    else
    {
      ++i;
    }
  }
}
}  // namespace
#endif
