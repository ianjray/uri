#pragma once

// Private API.
// Memory shim.
// Simulate allocation failure.

/// Initialize memory allocation tracking.
/// @param n Fail at Nth allocation (defaults to UINT_MAX).
void memory_shim_init(unsigned n);

/// @return Count of memory allocations since @c memory_shim_init.
unsigned memory_shim_count_get(void);
