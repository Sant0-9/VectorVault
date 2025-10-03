#pragma once

#include <string>
#include <span>
#include <vector>
#include <stdexcept>
#include <cstddef>
#include <cstdint>

namespace vectorvault {

// Memory-mapped file wrapper for efficient I/O
class MMapFile {
public:
    MMapFile() = default;
    ~MMapFile();
    
    // Non-copyable but movable
    MMapFile(const MMapFile&) = delete;
    MMapFile& operator=(const MMapFile&) = delete;
    MMapFile(MMapFile&& other) noexcept;
    MMapFile& operator=(MMapFile&& other) noexcept;
    
    // Open file for reading
    bool open_read(const std::string& path);
    
    // Create/open file for writing with specified size
    bool open_write(const std::string& path, size_t size);
    
    // Close the mapping
    void close();
    
    // Get mapped data
    const uint8_t* data() const { return data_; }
    uint8_t* data() { return data_; }
    size_t size() const { return size_; }
    
    bool is_open() const { return data_ != nullptr; }
    
private:
    uint8_t* data_ = nullptr;
    size_t size_ = 0;
    
#ifdef _WIN32
    void* file_handle_ = nullptr;
    void* mapping_handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

// Simple CRC32 for data integrity
uint32_t compute_crc32(const uint8_t* data, size_t length);

// Helper for serialization
class BinaryWriter {
public:
    explicit BinaryWriter(std::vector<uint8_t>& buffer) : buffer_(buffer) {}
    
    void write_uint32(uint32_t value);
    void write_uint64(uint64_t value);
    void write_int32(int32_t value);
    void write_float(float value);
    void write_bytes(const void* data, size_t size);
    
    template<typename T>
    void write_vector(const std::vector<T>& vec) {
        write_uint64(vec.size());
        write_bytes(vec.data(), vec.size() * sizeof(T));
    }
    
private:
    std::vector<uint8_t>& buffer_;
};

// Helper for deserialization
class BinaryReader {
public:
    explicit BinaryReader(const uint8_t* data, size_t size) 
        : data_(data), size_(size), pos_(0) {}
    
    uint32_t read_uint32();
    uint64_t read_uint64();
    int32_t read_int32();
    float read_float();
    void read_bytes(void* dest, size_t size);
    
    template<typename T>
    std::vector<T> read_vector() {
        uint64_t count = read_uint64();
        std::vector<T> result(count);
        read_bytes(result.data(), count * sizeof(T));
        return result;
    }
    
    size_t position() const { return pos_; }
    size_t remaining() const { return size_ - pos_; }
    
private:
    const uint8_t* data_;
    size_t size_;
    size_t pos_;
};

} // namespace vectorvault
