#include "Stream.hh"

#include <cstring>

namespace Kinoko::EGG {

/// @brief Constructor
/// @details Initializes @ref m_endian to `std::endian::big` and @ref m_index to zero.
Stream::Stream() : m_endian(std::endian::big), m_index(0) {}

/// @brief Default virtual destructor
Stream::~Stream() = default;

/// @brief Skips the specified number of bytes in the stream
/// @param count Number of bytes to skip
/// @warning The skip must not go beyond the end of the stream, otherwise it will put the stream
/// in an invalid state.
void Stream::skip(u32 count) {
    m_index += count;
    ASSERT(!bad());
}

/// @brief Jumps to the specified index in the stream
/// @param index The index to jump to
/// @warning The skip must not go beyond the end of the stream, otherwise it will put the stream
/// in an invalid state.
void Stream::jump(u32 index) {
    m_index = index;
    ASSERT(!bad());
}

/// @brief Sets the endianness of the stream
/// @param endian The endianness to set
void Stream::setEndian(std::endian endian) {
    m_endian = endian;
}

/// @brief Gets the current index in the stream
/// @return The current index in the stream, @ref m_index
u32 Stream::index() const {
    return m_index;
}

/// @brief Reads a `u8` from the stream and advances the index to the next byte
/// @return The `u8` value read from the stream at the current index
u8 Stream::read_u8() {
    return read<u8>();
}

/// @brief Reads a `u16` from the stream and jumps the index forward 2 bytes
/// @return The `u16` value read from the stream at the current index
u16 Stream::read_u16() {
    return read<u16>();
}

/// @brief Reads a `u32` from the stream and jumps the index forward 4 bytes
/// @return The `u32` value read from the stream at the current index
u32 Stream::read_u32() {
    return read<u32>();
}

/// @brief Reads a `u64` from the stream and jumps the index forward 8 bytes
/// @return The `u64` value read from the stream at the current index
u64 Stream::read_u64() {
    return read<u64>();
}

/// @brief Reads a `s8` from the stream and advances the index to the next byte
/// @return The `s8` value read from the stream at the current index
s8 Stream::read_s8() {
    return read<s8>();
}

/// @brief Reads a `s16` from the stream and jumps the index forward 2 bytes
/// @return The `s16` value read from the stream at the current index
s16 Stream::read_s16() {
    return read<s16>();
}

/// @brief Reads a `s32` from the stream and jumps the index forward 4 bytes
/// @return The `s32` value read from the stream at the current index
s32 Stream::read_s32() {
    return read<s32>();
}

/// @brief Reads a `s64` from the stream and jumps the index forward 8 bytes
/// @return The `s64` value read from the stream at the current index
s64 Stream::read_s64() {
    return read<s64>();
}

/// @brief Reads a `f32` from the stream and jumps the index forward 4 bytes
/// @return The `f32` value read from the stream at the current index
f32 Stream::read_f32() {
    return read<f32>();
}

/// @brief Reads a `f64` from the stream and jumps the index forward 8 bytes
/// @return The `f64` value read from the stream at the current index
f64 Stream::read_f64() {
    return read<f64>();
}

/// @brief Writes the provided `u8` to the stream and advances the index to the next byte
/// @param val The `u8` value to write to the stream
void Stream::write_u8(u8 val) {
    return write(val);
}

/// @brief Writes the provided `u16` to the stream and jumps the index forward 2 bytes
/// @param val The `u16` value to write to the stream
void Stream::write_u16(u16 val) {
    return write(val);
}

/// @brief Writes the provided `u32` to the stream and jumps the index forward 4 bytes
/// @param val The `u32` value to write to the stream
void Stream::write_u32(u32 val) {
    return write(val);
}

/// @brief Writes the provided `u64` to the stream and jumps the index forward 8 bytes
/// @param val The `u64` value to write to the stream
void Stream::write_u64(u64 val) {
    return write(val);
}

/// @brief Writes the provided `s8` to the stream and advances the index to the next byte
/// @param val The `s8` value to write to the stream
void Stream::write_s8(s8 val) {
    return write(val);
}

/// @brief Writes the provided `s16` to the stream and jumps the index forward 2 bytes
/// @param val The `s16` value to write to the stream
void Stream::write_s16(s16 val) {
    return write(val);
}

/// @brief Writes the provided `s32` to the stream and jumps the index forward 4 bytes
/// @param val The `s32` value to write to the stream
void Stream::write_s32(s32 val) {
    return write(val);
}

/// @brief Writes the provided `s64` to the stream and jumps the index forward 8 bytes
/// @param val The `s64` value to write to the stream
void Stream::write_s64(s64 val) {
    return write(val);
}

/// @brief Writes the provided `f32` to the stream and jumps the index forward 4 bytes
/// @param val The `f32` value to write to the stream
void Stream::write_f32(f32 val) {
    return write(val);
}

/// @brief Writes the provided `f64` to the stream and jumps the index forward 8 bytes
/// @param val The `f64` value to write to the stream
void Stream::write_f64(f64 val) {
    return write(val);
}

/// @brief Reads a value of type `T` from the stream at the current @ref m_index and advances
/// the index by the size of `T`
/// @tparam T The type of value to read from the stream
/// @return The value of type `T` read from the stream at the current index
/// @warning The read must not go beyond the end of the stream, otherwise it will put the stream
/// in an invalid state.
template <ParseableType T>
T Stream::read() {
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
void Stream::write(T val) {
    ASSERT(safe(sizeof(T)));
    T parsedVal = parse<T>(val, m_endian);
    write(&parsedVal, sizeof(parsedVal));
    m_index += sizeof(parsedVal);
}

/// @brief Default constructor
RamStream::RamStream() = default;

/// @brief Constructor that creates a @ref RamStream of length `size` from the provided `buffer`
/// @param buffer The buffer of data to stream
/// @param size The size of the buffer to stream
RamStream::RamStream(const void *buffer, u32 size) {
    setBufferAndSize(const_cast<void *>(buffer), size);
}

/// @brief Default virtual destructor
RamStream::~RamStream() = default;

/// @copydoc Stream::read()
/// @details Performs a `memcpy` from @ref m_buffer to the destination `output` buffer.
/// @warning The read must not go beyond the end of the stream, otherwise it will put the stream
/// in an invalid state.
void RamStream::read(void *output, u32 size) {
    ASSERT(safe(size));
    std::memcpy(output, m_buffer.data() + m_index, size);
}

/// @copydoc Stream::write()
/// @details Performs a `memcpy` from the source `input` buffer to @ref m_buffer.
/// @warning The write must not go beyond the end of the stream, otherwise it will put the
/// stream in an invalid state.
void RamStream::write(void *input, u32 size) {
    ASSERT(safe(size));
    std::memcpy(m_buffer.data() + m_index, input, size);
}

/// @copydoc Stream::eof()
/// @details The stream has reached its end if @ref m_index is equal to the size of @ref
/// m_buffer.
bool RamStream::eof() const {
    return m_index == m_buffer.size();
}

/// @copydoc Stream::safe()
/// @details The stream is safe for reading or writing `size` bytes if @ref m_index + `size` is
/// less than or equal to the size of @ref m_buffer.
bool RamStream::safe(u32 size) const {
    return m_index + size <= m_buffer.size();
}

/// @copydoc Stream::bad()
/// @details The stream is in a bad state if @ref m_index is greater than the size of @ref
/// m_buffer.
bool RamStream::bad() const {
    return m_index > m_buffer.size();
}

/// @brief Reads a string from the stream and jumps the index forward based on the length of the
/// string
/// @details Expects a null-terminated char array, and moves @ref m_index past the null
/// terminator.
/// @warning The read must not go beyond the end of the stream, otherwise it will put the stream
/// in an invalid state.
const char *RamStream::read_string() {
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
RamStream RamStream::split(u32 size) {
    RamStream stream = RamStream(m_buffer.data() + m_index, size);
    m_index += size;
    ASSERT(!bad());
    return stream;
}

/// @brief Sets the stream's buffer and size based on the provided parameters
/// @param buffer The buffer to set for the stream
/// @param size The size of the buffer to stream
void RamStream::setBufferAndSize(void *buffer, u32 size) {
    m_buffer = std::span<u8>(reinterpret_cast<u8 *>(buffer), size);
}

/// @brief Gets a pointer to the buffer's data
/// @return A pointer to the buffer's data
u8 *RamStream::data() {
    return m_buffer.data();
}

/// @brief Gets a pointer to the buffer's data at the current index
/// @return A pointer to the buffer's data at @ref m_index
u8 *RamStream::dataAtIndex() {
    return m_buffer.data() + m_index;
}

} // namespace Kinoko::EGG
