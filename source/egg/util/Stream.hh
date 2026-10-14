#pragma once

#include <Common.hh>

#include <cstring>

namespace Kinoko::EGG {

/// @brief A stream of data, abstracted to allow for continuous seeking.
class Stream {
public:
    /// @brief Constructor
    /// @details Initializes @ref m_endian to `std::endian::big` and @ref m_index to zero.
    Stream() : m_endian(std::endian::big), m_index(0) {}

    /// @brief Default virtual destructor
    virtual ~Stream() = default;

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
    virtual bool eof() const = 0;

    /// @brief Checks if it is safe to read/write the specified number of bytes from/to the stream
    /// @param size Number of bytes to check
    /// @return `true` if it is safe, `false` otherwise.
    virtual bool safe(u32 size) const = 0;

    /// @brief Checks if the stream is in an invalid state
    /// @return `true` if the stream is in an invalid state, `false` otherwise.
    virtual bool bad() const = 0;

    /// @brief Skips the specified number of bytes in the stream
    /// @param count Number of bytes to skip
    /// @warning The skip must not go beyond the end of the stream, otherwise it will put the stream
    /// in an invalid state.
    void skip(u32 count) {
        m_index += count;
        ASSERT(!bad());
    }

    /// @brief Jumps to the specified index in the stream
    /// @param index The index to jump to
    /// @warning The skip must not go beyond the end of the stream, otherwise it will put the stream
    /// in an invalid state.
    void jump(u32 index) {
        m_index = index;
        ASSERT(!bad());
    }

    /// @beginSetters

    /// @brief Sets the endianness of the stream
    /// @param endian The endianness to set
    void setEndian(std::endian endian) {
        m_endian = endian;
    }

    /// @endSetters

    /// @beginGetters

    /// @brief Gets the current index in the stream
    /// @return The current index in the stream, @ref m_index
    [[nodiscard]] u32 index() const {
        return m_index;
    }

    /// @endGetters

    /// @brief Reads a `u8` from the stream and advances the index to the next byte
    /// @return The `u8` value read from the stream at the current index
    [[nodiscard]] u8 read_u8() {
        return read<u8>();
    }

    /// @brief Reads a `u16` from the stream and jumps the index forward 2 bytes
    /// @return The `u16` value read from the stream at the current index
    [[nodiscard]] u16 read_u16() {
        return read<u16>();
    }

    /// @brief Reads a `u32` from the stream and jumps the index forward 4 bytes
    /// @return The `u32` value read from the stream at the current index
    [[nodiscard]] u32 read_u32() {
        return read<u32>();
    }

    /// @brief Reads a `u64` from the stream and jumps the index forward 8 bytes
    /// @return The `u64` value read from the stream at the current index
    [[nodiscard]] u64 read_u64() {
        return read<u64>();
    }

    /// @brief Reads a `s8` from the stream and advances the index to the next byte
    /// @return The `s8` value read from the stream at the current index
    [[nodiscard]] s8 read_s8() {
        return read<s8>();
    }

    /// @brief Reads a `s16` from the stream and jumps the index forward 2 bytes
    /// @return The `s16` value read from the stream at the current index
    [[nodiscard]] s16 read_s16() {
        return read<s16>();
    }

    /// @brief Reads a `s32` from the stream and jumps the index forward 4 bytes
    /// @return The `s32` value read from the stream at the current index
    [[nodiscard]] s32 read_s32() {
        return read<s32>();
    }

    /// @brief Reads a `s64` from the stream and jumps the index forward 8 bytes
    /// @return The `s64` value read from the stream at the current index
    [[nodiscard]] s64 read_s64() {
        return read<s64>();
    }

    /// @brief Reads a `f32` from the stream and jumps the index forward 4 bytes
    /// @return The `f32` value read from the stream at the current index
    [[nodiscard]] f32 read_f32() {
        return read<f32>();
    }

    /// @brief Reads a `f64` from the stream and jumps the index forward 8 bytes
    /// @return The `f64` value read from the stream at the current index
    [[nodiscard]] f64 read_f64() {
        return read<f64>();
    }

    /// @brief Writes the provided `u8` to the stream and advances the index to the next byte
    /// @param val The `u8` value to write to the stream
    void write_u8(u8 val) {
        return write(val);
    }

    /// @brief Writes the provided `u16` to the stream and jumps the index forward 2 bytes
    /// @param val The `u16` value to write to the stream
    void write_u16(u16 val) {
        return write(val);
    }

    /// @brief Writes the provided `u32` to the stream and jumps the index forward 4 bytes
    /// @param val The `u32` value to write to the stream
    void write_u32(u32 val) {
        return write(val);
    }

    /// @brief Writes the provided `u64` to the stream and jumps the index forward 8 bytes
    /// @param val The `u64` value to write to the stream
    void write_u64(u64 val) {
        return write(val);
    }

    /// @brief Writes the provided `s8` to the stream and advances the index to the next byte
    /// @param val The `s8` value to write to the stream
    void write_s8(s8 val) {
        return write(val);
    }

    /// @brief Writes the provided `s16` to the stream and jumps the index forward 2 bytes
    /// @param val The `s16` value to write to the stream
    void write_s16(s16 val) {
        return write(val);
    }

    /// @brief Writes the provided `s32` to the stream and jumps the index forward 4 bytes
    /// @param val The `s32` value to write to the stream
    void write_s32(s32 val) {
        return write(val);
    }

    /// @brief Writes the provided `s64` to the stream and jumps the index forward 8 bytes
    /// @param val The `s64` value to write to the stream
    void write_s64(s64 val) {
        return write(val);
    }

    /// @brief Writes the provided `f32` to the stream and jumps the index forward 4 bytes
    /// @param val The `f32` value to write to the stream
    void write_f32(f32 val) {
        return write(val);
    }

    /// @brief Writes the provided `f64` to the stream and jumps the index forward 8 bytes
    /// @param val The `f64` value to write to the stream
    void write_f64(f64 val) {
        return write(val);
    }

protected:
    std::endian m_endian; ///< The endianness to use when reading/writing data to the stream
    u32 m_index;          ///< The current index within the stream for the next read/write

private:
    /// @brief Reads a value of type `T` from the stream at the current @ref m_index and advances
    /// the index by the size of `T`
    /// @tparam T The type of value to read from the stream
    /// @return The value of type `T` read from the stream at the current index
    /// @warning The read must not go beyond the end of the stream, otherwise it will put the stream
    /// in an invalid state.
    template <ParseableType T>
    [[nodiscard]] T read() {
        ASSERT(safe(sizeof(T)));
        T val;
        read(&val, sizeof(val));
        m_index += sizeof(val);

        return parse<T>(val, m_endian);
    }

    /// @brief Writes a value of type `T` to the stream at the current @ref m_index and advances
    /// the index by the size of `T`
    /// @tparam T The type of value to write to the stream
    /// @param val The value of type `T` to write to the stream
    /// @warning The write must not go beyond the end of the stream, otherwise it will put the
    /// stream in an invalid state.
    template <ParseableType T>
    void write(T val) {
        ASSERT(safe(sizeof(T)));
        T parsedVal = parse<T>(val, m_endian);
        write(&parsedVal, sizeof(parsedVal));
        m_index += sizeof(parsedVal);
    }
};

/// @brief A stream of data stored in memory.
/// @details RamStream solves two problems:
/// 1. Inconsistent endianness across Kinoko clients.
/// 2. Having to maintain pointer arithmetic while reading in various data types.
/// We specify the endianness of the data in the stream, and the stream will handle the rest.
class RamStream : public Stream {
public:
    /// @brief Default constructor
    RamStream() = default;

    /// @brief Constructor that creates a @ref RamStream of length `size` from the provided `buffer`
    /// @param buffer The buffer of data to stream
    /// @param size The size of the buffer to stream
    RamStream(const void *buffer, u32 size) {
        setBufferAndSize(const_cast<void *>(buffer), size);
    }

    /// @brief Default virtual destructor
    ~RamStream() override = default;

    /// @copydoc Stream::read()
    /// @details Performs a `memcpy` from @ref m_buffer to the destination `output` buffer.
    /// @warning The read must not go beyond the end of the stream, otherwise it will put the stream
    /// in an invalid state.
    void read(void *output, u32 size) override {
        ASSERT(safe(size));
        std::memcpy(output, m_buffer.data() + m_index, size);
    }

    /// @copydoc Stream::write()
    /// @details Performs a `memcpy` from the source `input` buffer to @ref m_buffer.
    /// @warning The write must not go beyond the end of the stream, otherwise it will put the
    /// stream in an invalid state.
    void write(void *input, u32 size) override {
        ASSERT(safe(size));
        std::memcpy(m_buffer.data() + m_index, input, size);
    }

    /// @copydoc Stream::eof()
    /// @details The stream has reached its end if @ref m_index is equal to the size of @ref
    /// m_buffer.
    [[nodiscard]] bool eof() const override {
        return m_index == m_buffer.size();
    }

    /// @copydoc Stream::safe()
    /// @details The stream is safe for reading or writing `size` bytes if @ref m_index + `size` is
    /// less than or equal to the size of @ref m_buffer.
    [[nodiscard]] bool safe(u32 size) const override {
        return m_index + size <= m_buffer.size();
    }

    /// @copydoc Stream::bad()
    /// @details The stream is in a bad state if @ref m_index is greater than the size of @ref
    /// m_buffer.
    [[nodiscard]] bool bad() const override {
        return m_index > m_buffer.size();
    }

    /// @brief Reads a string from the stream and jumps the index forward based on the length of the
    /// string
    /// @details Expects a null-terminated char array, and moves @ref m_index past the null
    /// terminator.
    /// @warning The read must not go beyond the end of the stream, otherwise it will put the stream
    /// in an invalid state.
    [[nodiscard]] const char *read_string() {
        const char *ret = reinterpret_cast<char *>(m_buffer.data() + m_index);
        m_index += strlen(ret) + 1;
        ASSERT(!bad());
        return ret;
    }

    /// @brief Splits the current stream into two.
    /// @return A new @ref RamStream containing the data from the current index to `size` bytes
    /// after
    /// @details Segments the current stream at @ref m_index. The returned stream is the data
    /// from @ref m_index to size bytes after. `this` stream is then moved to `size` bytes after
    /// @ref m_index.
    /// @warning The split `size` must not go beyond the end of the stream, otherwise it will put
    /// the stream in an invalid state.
    [[nodiscard]] RamStream split(u32 size) {
        RamStream stream = RamStream(m_buffer.data() + m_index, size);
        m_index += size;
        ASSERT(!bad());
        return stream;
    }

    /// @brief Sets the stream's buffer and size based on the provided parameters
    /// @param buffer The buffer to set for the stream
    /// @param size The size of the buffer to stream
    void setBufferAndSize(void *buffer, u32 size) {
        m_buffer = std::span<u8>(reinterpret_cast<u8 *>(buffer), size);
    }

    /// @beginGetters

    /// @brief Gets a pointer to the buffer's data
    /// @return A pointer to the buffer's data
    [[nodiscard]] u8 *data() {
        return m_buffer.data();
    }

    /// @brief Gets a pointer to the buffer's data at the current index
    /// @return A pointer to the buffer's data at @ref m_index
    [[nodiscard]] u8 *dataAtIndex() {
        return m_buffer.data() + m_index;
    }

    /// @endGetters

private:
    std::span<u8> m_buffer; ///< Span of the stream's buffer and size
};

} // namespace Kinoko::EGG
