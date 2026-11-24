#pragma once
// Pure Win32 64-bit Memory Engine for PES 2021 / SP Football Life.
// Zero-allocation stack typed reads, fast chunked AOB scanner with wildcards,
// FlushInstructionCache, near-module VirtualAllocEx (strictly within +-1.8GB),
// UTF-8 safe string writes, and process auto-attach with elevation detection.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

namespace pes {

class Win32Memory {
public:
    HANDLE handle = nullptr;
    uint32_t pid = 0;
    std::string exe_name;
    uint64_t module_base = 0;
    uint32_t module_size = 0;
    bool access_denied = false;

    Win32Memory() = default;
    ~Win32Memory() { detach(); }

    Win32Memory(const Win32Memory&) = delete;
    Win32Memory& operator=(const Win32Memory&) = delete;

    bool is_alive() const;
    bool attached();
    bool attach();
    void detach();

    bool read_into(uint64_t addr, void* dst, size_t size) const {
        if (!handle || addr <= 0x10000ULL || !dst || size == 0) return false;
        SIZE_T n = 0;
        return ReadProcessMemory(handle, reinterpret_cast<LPCVOID>(addr), dst, size, &n) != 0 && n == size;
    }

    bool write_from(uint64_t addr, const void* src, size_t size) const {
        if (!handle || addr <= 0x10000ULL || !src || size == 0) return false;
        SIZE_T n = 0;
        return WriteProcessMemory(handle, reinterpret_cast<LPVOID>(addr), src, size, &n) != 0 && n == size;
    }

    template <typename T>
    T read_t(uint64_t addr, T fallback = T{}) const {
        T val = fallback;
        if (!read_into(addr, &val, sizeof(T))) return fallback;
        return val;
    }

    template <typename T>
    bool write_t(uint64_t addr, T val) const {
        return write_from(addr, &val, sizeof(T));
    }

    uint8_t  read_u8(uint64_t a) const  { return read_t<uint8_t>(a, 0); }
    uint16_t read_u16(uint64_t a) const { return read_t<uint16_t>(a, 0); }
    uint32_t read_u32(uint64_t a) const { return read_t<uint32_t>(a, 0); }
    int32_t  read_i32(uint64_t a) const { return read_t<int32_t>(a, 0); }
    uint64_t read_u64(uint64_t a) const { return read_t<uint64_t>(a, 0); }
    float    read_f32(uint64_t a) const { return read_t<float>(a, 0.0f); }

    bool write_u8(uint64_t a, uint8_t v) const   { return write_t<uint8_t>(a, v); }
    bool write_u16(uint64_t a, uint16_t v) const { return write_t<uint16_t>(a, v); }
    bool write_u32(uint64_t a, uint32_t v) const { return write_t<uint32_t>(a, v); }
    bool write_i32(uint64_t a, int32_t v) const  { return write_t<int32_t>(a, v); }
    bool write_u64(uint64_t a, uint64_t v) const { return write_t<uint64_t>(a, v); }
    bool write_f32(uint64_t a, float v) const    { return write_t<float>(a, v); }

    bool write_code_bytes(uint64_t addr, const void* data, size_t size) const;
    void flush_icache(uint64_t addr, size_t size) const;

    uint64_t alloc_near(uint64_t target_addr, size_t size = 0x2000) const;
    bool free_mem(uint64_t addr) const;

    std::string read_str(uint64_t addr, size_t max_len = 46) const;
    bool write_str(uint64_t addr, std::string_view text, size_t max_len) const;

    uint32_t get_bits(uint64_t addr, int bit_start, int bit_len) const;
    bool set_bits(uint64_t addr, int bit_start, int bit_len, uint32_t new_val) const;

    uint64_t aob_scan_region(uint64_t start_addr, uint64_t end_addr, std::string_view pattern) const;
};

} // namespace pes
