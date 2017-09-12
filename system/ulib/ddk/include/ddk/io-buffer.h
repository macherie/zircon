// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/assert.h>
#include <zircon/compiler.h>
#include <zircon/syscalls.h>
#include <zircon/types.h>

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

__BEGIN_CDECLS;

// Sentinel value for io_buffer_t's |phys| field for when it is not valid.
#define IO_BUFFER_INVALID_PHYS UINT64_MAX

typedef struct {
    zx_handle_t vmo_handle;
    size_t size;
    zx_off_t offset;
    void* virt;
    // Points to the physical page backing the start of the VMO, if this
    // io buffer was created with the IO_BUFFER_CONTIG flag.
    zx_paddr_t phys;

    // This is used for storing the addresses of the physical pages backing non
    // contiguous buffers and is set by io_buffer_physmap().
    // Each entry in the list represents a whole page and the first entry
    // points to the page containing 'offset'.
    zx_paddr_t* phys_list;
    uint64_t phys_count;
} io_buffer_t;

enum {
    IO_BUFFER_RO         = (0 << 0),
    IO_BUFFER_RW         = (1 << 0),
    IO_BUFFER_CONTIG     = (1 << 1),
    IO_BUFFER_FLAGS_MASK = IO_BUFFER_RW | IO_BUFFER_CONTIG,
};

// Initializes a new io_buffer.  If this call fails, it is still safe to call
// io_buffer_release on |buffer|.
zx_status_t io_buffer_init(io_buffer_t* buffer, size_t size, uint32_t flags);
// An alignment of zero is interpreted as requesting page alignment.
// Requesting a specific alignment is not supported for non-contiguous buffers,
// pass zero for |alignment_log2| if not passing IO_BUFFER_CONTIG.
zx_status_t io_buffer_init_aligned(io_buffer_t* buffer, size_t size, uint32_t alignment_log2, uint32_t flags);

// Initializes an io_buffer base on an existing VMO.
// duplicates the provided vmo_handle - does not take ownership
zx_status_t io_buffer_init_vmo(io_buffer_t* buffer, zx_handle_t vmo_handle,
                               zx_off_t offset, uint32_t flags);

// Initializes an io_buffer that maps a given physical address
zx_status_t io_buffer_init_physical(io_buffer_t* buffer, zx_paddr_t addr, size_t size,
                                    zx_handle_t resource, uint32_t cache_policy);

zx_status_t io_buffer_cache_op(io_buffer_t* buffer, const uint32_t op,
                               const zx_off_t offset, const size_t size);

// io_buffer_cache_flush() performs a cache flush on a range of memory in the buffer
zx_status_t io_buffer_cache_flush(io_buffer_t* buffer, zx_off_t offset, size_t length);

// io_buffer_cache_flush_invalidate() performs a cache flush and invalidate on a range of memory
// in the buffer
zx_status_t io_buffer_cache_flush_invalidate(io_buffer_t* buffer, zx_off_t offset, size_t length);

// Looks up the physical pages backing this buffer's vm object.
// This is used for non contiguous buffers.
// The 'phys_list' and 'phys_count' fields are set if this function succeeds.
zx_status_t io_buffer_physmap(io_buffer_t* buffer);

// Releases an io_buffer
void io_buffer_release(io_buffer_t* buffer);

// Set the BTI used by io_buffers for getting physical addresses
void io_buffer_set_default_bti(zx_handle_t bti);

static inline bool io_buffer_is_valid(io_buffer_t* buffer) {
    return (buffer->vmo_handle != ZX_HANDLE_INVALID);
}

static inline void* io_buffer_virt(io_buffer_t* buffer) {
    return (void*)(((uintptr_t)buffer->virt) + buffer->offset);
}

static inline zx_paddr_t io_buffer_phys(io_buffer_t* buffer) {
    ZX_DEBUG_ASSERT(buffer->phys != IO_BUFFER_INVALID_PHYS);
    return buffer->phys + buffer->offset;
}

static inline zx_status_t io_buffer_physmap_range(io_buffer_t* buffer, zx_off_t offset,
                                                  size_t length, size_t phys_count,
                                                  zx_paddr_t* physmap) {
    // TODO(teisenbe): We need to figure out how to integrate lifetime
    // management of this pin into the io_buffer API...
    extern zx_handle_t io_default_bti;
    if (io_default_bti == ZX_HANDLE_INVALID) {
        return zx_vmo_op_range(buffer->vmo_handle, ZX_VMO_OP_LOOKUP, offset, length,
                               physmap, phys_count * sizeof(*physmap));
    } else {
        size_t actual_entries;
        const size_t sub_offset = offset & (PAGE_SIZE - 1);
        const size_t pin_offset = offset - sub_offset;
        const size_t pin_length = (length + sub_offset + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
        zx_status_t status = zx_bti_pin(io_default_bti, buffer->vmo_handle, pin_offset,
                                        pin_length,
                                        ZX_VM_FLAG_PERM_READ | ZX_VM_FLAG_PERM_WRITE,
                                        physmap, phys_count, &actual_entries);
        if (status != ZX_OK) {
            return status;
        }
        // We should remove this assert by tracking the length of this array
        // explicitly in buffer.
        if (actual_entries != phys_count) {
            // TODO(teisenbe): Support non-4K min contig
            ZX_DEBUG_ASSERT(phys_count == pin_length / PAGE_SIZE);
            ZX_DEBUG_ASSERT(actual_entries == 1);
            for (size_t i = 1; i < phys_count; ++i) {
                physmap[i] = physmap[i - 1] + PAGE_SIZE;
            }
        }
        physmap[0] += sub_offset;
        return ZX_OK;
    }
}

// Returns the buffer size available after the given offset, relative to the
// io_buffer vmo offset.
static inline size_t io_buffer_size(io_buffer_t* buffer, size_t offset) {
    size_t remaining = buffer->size - buffer->offset - offset;
    // May overflow.
    if (remaining > buffer->size) {
        remaining = 0;
    }
    return remaining;
}

__END_CDECLS;
