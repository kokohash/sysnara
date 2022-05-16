#ifndef __UTIL_H
#define __UTIL_H

// Type definition for de-allocator function, e.g. free().
typedef void (*free_function)(void *);

// Type definition for read-only callback for single-value containers,
// used by e.g. print functions.
typedef void (*inspect_callback)(const void *);

// Ditto for dual-value containers.
typedef void (*inspect_callback_pair)(const void *, const void *);

// Type definition for comparison function, used by e.g. table.
//
// Comparison functions should return values that indicate the order
// of the arguments. If the first argument is considered less/lower
// than the second, a negative value should be returned. If the first
// argument is considered more/higher than the second, a positive value
// should be returned. If the arguments are considered equal, a zero
// value should be returned.
typedef int compare_function(const void *,const void *);

#endif
