#pragma once

// Private API.
// Memory shim.

/// Initialize memory allocation tracking.
/// @param cap Maximum number of allowed allocations (defaults to UINT_MAX).
void memory_shim_init(unsigned cap);

/// @return Count of memory allocations.
unsigned memory_shim_count_get(void);
