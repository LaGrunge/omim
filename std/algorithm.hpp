#pragma once

#ifdef new
#undef new
#endif

#include <algorithm>

using std::all_of;
using std::any_of;
using std::binary_search;
using std::copy;
using std::equal;
using std::equal_range;
using std::fill;
using std::find;
using std::find_first_of;
using std::find_if;
using std::for_each;
using std::is_sorted;
using std::iter_swap;
using std::lexicographical_compare;
using std::lower_bound;
using std::max;
using std::max_element;
using std::min;
using std::min_element;
using std::minmax_element;
using std::next_permutation;
using std::none_of;
using std::nth_element;
using std::partial_sort;
using std::remove_if;
using std::replace;
using std::reverse;
using std::set_intersection;
using std::set_union;
using std::sort;
using std::stable_partition;
using std::stable_sort;
using std::swap;
using std::unique;
using std::upper_bound;
using std::set_difference;
using std::distance;
using std::generate;
using std::pop_heap;
using std::push_heap;
using std::remove_copy_if;
using std::set_symmetric_difference;
using std::sort_heap;
using std::transform;

#ifdef DEBUG_NEW
#define new DEBUG_NEW
#endif
