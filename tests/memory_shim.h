#pragma once

// Private API.
// Memory shim.
// Simulate allocation failure.

/// Reset memory allocation tracking.
void memory_shim_reset(void);

/// Reset memory allocation tracking, and configure to fail at @c nth allocation.
/// @note @c nth should be one or more.
void memory_shim_fail_at(unsigned nth);

/// @return Count of memory allocations since @c memory_shim_init.
unsigned memory_shim_count_get(void);
