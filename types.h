
#ifndef TYPES_H
#define TYPES_H

#include <cstdint>

using u1 = uint8_t;
using u2 = uint16_t;
using u4 = uint32_t;
using u8 = uint64_t;

#define NO_COPY(x) \
    x(x &&) = delete; \
    x(const x&) = delete; \
    x& operator=(const x&) = delete;

class BytesInput
{
private:
    const char *mBuff = nullptr;
    size_t mLength = 0;
    size_t mCursor = 0;

public:
    BytesInput(const void *buff, size_t buffLength) noexcept
    {
        mBuff = (const char *) buff;
        mLength = buffLength;
    }

    NO_COPY(BytesInput)

    ~BytesInput() noexcept = default;

    [[nodiscard]]
    const void *data() const noexcept { return mBuff; }

    [[nodiscard]]
    size_t where() const noexcept { return mCursor; }

    void seek(size_t off) noexcept { mCursor = std::min(off, mLength); }

    [[nodiscard]]
    bool eof() const noexcept { return mCursor >= mLength; }

    size_t write(void *buff, size_t size) noexcept
    {
        size_t consumed = std::min(mLength - mCursor, size);
        memcpy(buff, mBuff + mCursor, consumed);
        mCursor += consumed;
        return consumed;
    }

    template<typename T>
    BytesInput& operator>>(T &value) noexcept
    {
        write(&value, sizeof(T));
        return *this;
    }

    [[nodiscard]]
    size_t length() const noexcept { return mLength; }
};

#endif // TYPES_H