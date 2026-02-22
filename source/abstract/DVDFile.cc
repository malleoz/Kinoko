#include "DVDFile.hh"

#include <cstring>

namespace Abstract {

DVDFile::DVDFile(void) : mData(nullptr), mSize(0) {}

DVDFile::DVDFile(const DVDFile &rhs) : DVDFile() {
    if (rhs.ok()) {
        mPath = rhs.mPath;
        mSize = rhs.mSize;
        mData = new u8[mSize];
        memcpy(mData, rhs.mData, mSize);
    }
}

DVDFile::DVDFile(DVDFile &&rhs) {
    mPath = rhs.mPath;
    mData = rhs.mData;
    mSize = rhs.mSize;

    rhs.mPath.clear();
    rhs.mData = nullptr;
    rhs.mSize = 0;
}

DVDFile &DVDFile::operator=(DVDFile &&rhs) {
    mPath = rhs.mPath;
    mData = rhs.mData;
    mSize = rhs.mSize;

    rhs.mPath.clear();
    rhs.mData = nullptr;
    rhs.mSize = 0;

    return *this;
}

DVDFile::DVDFile(const std::filesystem::path &path) : DVDFile() {
    mPath = path;
    load(path.native().data());
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
    mPath.clear();
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
