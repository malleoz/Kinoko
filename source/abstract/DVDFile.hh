#pragma once

#include <Common.hh>

#include <type_traits>

namespace Abstract {

/* ================================================================ *
        DVDFile loaded into memory, not mapped to a given type.
 * ================================================================ */
class DVDFile {
public:
    /* ================================================================ *
            Default constructor. Initializes to an invalid state.
            No allocation.
     * ================================================================ */
    DVDFile(void);

    /* ================================================================ *
            Copy constructor. Initializes to identical state to rhs.
     * ================================================================ */
    DVDFile(const DVDFile &rhs);

    /* ================================================================ *
            Move constructor. Leaves rhs in invalid state.
     * ================================================================ */
    DVDFile(DVDFile &&rhs);

    /* ================================================================ *
            Move assignment operator. Leaves rhs in invalid state.
     * ================================================================ */
    DVDFile &operator=(DVDFile &&rhs);

    /* ================================================================ *
            Path constructor. Loads the requested file.
     * ================================================================ */
    DVDFile(const char *path);
    DVDFile(const wchar_t *path);

    /* ================================================================ *
            Destructor. Unloads the file.
     * ================================================================ */
    ~DVDFile(void);

    /* ================================================================ *
            Loads the file with the provided path.
            Panics if starting in an valid state.
     * ================================================================ */
    void load(const char *path) {
        return load_(path);
    }

    void load(const wchar_t *path) {
        return load_(path);
    }

private:
    /* ================================================================ *
            INTERNAL: Loads the file with the provided path.
     * ================================================================ */
    template <typename TChar>
        requires std::is_same_v<TChar, char> || std::is_same_v<TChar, wchar_t>
    void load_(const TChar *path);

public:
    /* ================================================================ *
            Unloads the file.
            Panics if starting in an invalid state.
    * ================================================================ */
    void unload();

    /* ================================================================ *
            Checks if the file is valid.
     * ================================================================ */
    bool ok(void) const;

    /* ================================================================ *
            Checks if the file is empty. Returns true if invalid.
     * ================================================================ */
    bool empty(void) const;

    /* ================================================================ *
            Built-in reinterpret cast.
     * ================================================================ */
    template <typename T>
    const T *cast(void) const {
        return reinterpret_cast<const T *>(mData);
    }

    template <typename T>
    T *cast(void) {
        return reinterpret_cast<T *>(mData);
    }

    /* ================================================================ *
            Gets the raw data for the file.
     * ================================================================ */
    const void *data(void) const {
        return mData;
    }

    void *data(void) {
        return mData;
    }

    /* ================================================================ *
            Gets the size of the file.
     * ================================================================ */
    size_t size(void) const {
        return mSize;
    }

private:
    void *mData;
    size_t mSize;
};

/* ================================================================ *
        DVDFile loaded into memory, mapped to a given type.
 * ================================================================ */
template <typename TFormat>
class TDVDFile : public DVDFile {
public:
    /* ================================================================ *
            Deleted built-in reinterpret cast -- use data( )!!
     * ================================================================ */
    template <typename T>
    const T *cast(void) const {
        PANIC("Casting is disabled when type is specified!");
        return nullptr;
    }

    template <typename T>
    T *cast(void) {
        PANIC("Casting is disabled with type is specified!");
        return nullptr;
    }

    /* ================================================================ *
            Gets the mapped data for the file.
     * ================================================================ */
    const TFormat *data(void) const {
        return reinterpret_cast<const TFormat *>(mData);
    }

    TFormat *data(void) {
        return reinterpret_cast<TFormat *>(mData);
    }

private:
    // So long as this compiles, DVDFile -> TDVDFile< TFormat > is safe
    STATIC_ASSERT(sizeof(TDVDFile<TFormat>) == sizeof(DVDFile));
};

} // namespace Abstract

#include "abstract/DVDFile.inl"
