#include "DVDFile.hh"

namespace Abstract {

DVDFile::DVDFile(void) : mData(nullptr), mSize(0) {}

DVDFile::DVDFile(const DVDFile &rhs) : DVDFile() {
    if (rhs.ok()) {
        mSize = rhs.mSize;
        mData = new u8[mSize];
    }
}

DVDFile::DVDFile(DVDFile &&rhs) {
    mData = rhs.mData;
    mSize = rhs.mSize;

    rhs.mData = nullptr;
    rhs.mSize = 0;
}

DVDFile &DVDFile::operator=(DVDFile &&rhs) {
    mData = rhs.mData;
    mSize = rhs.mSize;

    rhs.mData = nullptr;
    rhs.mSize = 0;

    return *this;
}

DVDFile::DVDFile(const char *path) : DVDFile() {
    ASSERT(path);
    load(path);
}

DVDFile::DVDFile(const wchar_t *path) : DVDFile() {
    ASSERT(path);
    load(path);
}

DVDFile::~DVDFile(void) {
    if (ok()) {
        unload();
    }
}

/* ================================================================ *
        See DVDFile.inl for DVDFile::load_( ... ).
 * ================================================================ */

void DVDFile::unload(void) {
    if (!ok()) {
        PANIC("DVDFile already unloaded!");
    }

    delete[] cast<u8>();
    mData = nullptr;
    mSize = 0;
}

bool DVDFile::ok(void) const {
    return mData;
}

bool DVDFile::empty(void) const {
    return !mData || mSize == 0;
}

} // namespace Abstract
