#pragma once

#include <Common.hh>

#include <span>

namespace Kinoko::EGG {

/// @brief A stream of data, abstracted to allow for continuous seeking.
class Stream {
public:
    Stream();
    virtual ~Stream();

    /// @brief Reads data from the stream into the specified output buffer
    /// @param output Buffer to store the read data
    /// @param size Number of bytes to read from the stream
    virtual void read(void *output, u32 size) = 0;

    /// @brief Writes data to the stream from the specified input buffer
    /// @param input Buffer containing the data to write
    /// @param size Number of bytes to write to the stream
    virtual void write(void *input, u32 size) = 0;

    /// @brief Checks if we have reached the end of the stream
    /// @return `true` if the stream has reached the end, `false` otherwise.
    [[nodiscard]] virtual bool eof() const = 0;

    /// @brief Checks if it is safe to read/write the specified number of bytes from/to the stream
    /// @param size Number of bytes to check
    /// @return `true` if it is safe, `false` otherwise.
    [[nodiscard]] virtual bool safe(u32 size) const = 0;

    /// @brief Checks if the stream is in an invalid state
    /// @return `true` if the stream is in an invalid state, `false` otherwise.
    [[nodiscard]] virtual bool bad() const = 0;

    void skip(u32 count);
    void jump(u32 index);

    /// @beginSetters

    void setEndian(std::endian endian);

    /// @endSetters

    /// @beginGetters

    [[nodiscard]] u32 index() const;

    /// @endGetters

    [[nodiscard]] u8 read_u8();
    [[nodiscard]] u16 read_u16();
    [[nodiscard]] u32 read_u32();
    [[nodiscard]] u64 read_u64();
    [[nodiscard]] s8 read_s8();
    [[nodiscard]] s16 read_s16();
    [[nodiscard]] s32 read_s32();
    [[nodiscard]] s64 read_s64();
    [[nodiscard]] f32 read_f32();
    [[nodiscard]] f64 read_f64();

    void write_u8(u8 val);
    void write_u16(u16 val);
    void write_u32(u32 val);
    void write_u64(u64 val);
    void write_s8(s8 val);
    void write_s16(s16 val);
    void write_s32(s32 val);
    void write_s64(s64 val);
    void write_f32(f32 val);
    void write_f64(f64 val);

protected:
    std::endian m_endian; ///< The endianness to use when reading/writing data to the stream
    u32 m_index;          ///< The current index within the stream for the next read/write

private:
    template <ParseableType T>
    [[nodiscard]] T read();

    template <ParseableType T>
    void write(T val);
};

/// @brief A stream of data stored in memory.
/// @details RamStream solves two problems:
/// 1. Inconsistent endianness across Kinoko clients.
/// 2. Having to maintain pointer arithmetic while reading in various data types.
/// We specify the endianness of the data in the stream, and the stream will handle the rest.
class RamStream : public Stream {
public:
    RamStream();
    RamStream(const void *buffer, u32 size);
    ~RamStream() override;

    void read(void *output, u32 size) override;
    void write(void *input, u32 size) override;
    [[nodiscard]] bool eof() const override;
    [[nodiscard]] bool safe(u32 size) const override;
    [[nodiscard]] bool bad() const override;

    [[nodiscard]] const char *read_string();
    [[nodiscard]] RamStream split(u32 size);

    void setBufferAndSize(void *buffer, u32 size);

    /// @beginGetters

    [[nodiscard]] u8 *data();
    [[nodiscard]] u8 *dataAtIndex();

    /// @endGetters

private:
    std::span<u8> m_buffer; ///< Span of the stream's buffer and size
};

} // namespace Kinoko::EGG
