
#ifndef ZIP_H
#define ZIP_H

#include <cstdio>
#include "types.h"

enum CompressMethod
{
    COMPRESS_STORE = 0,
    COMPRESS_DEFLATE = 8,
};

struct ZipEntry
{
    u2 versionMadeBy;
    u2 versionToExtract;
    u2 flag;
    u2 method;
    u2 mTime;
    u2 mDate;
    u4 crc32;
    u4 compressedSize;
    u4 unCompressedSize;
    u2 nameLength;
    u2 extraLength;
    u2 commentLength;
//    u2 diskNumberStart;
//    u2 internalAttributes;
//    u4 externalAttributes;
    u4 bytesOffset;

    const char *name;
    const void *extra;
    const char *comment;
};

class ZipFile
{
private:
    size_t mSize = 0;
    ZipEntry *mEntries = nullptr;
    FILE *mFile = nullptr;
    const char *mComment = nullptr;

public:
    explicit ZipFile() = default;
    ~ZipFile() noexcept { close(); }

    ZipFile(const ZipFile&) = delete;
    ZipFile& operator=(const ZipFile &) = delete;

    int open(const char *path) noexcept;

    void close() noexcept;

    [[nodiscard]]
    size_t size() const noexcept { return mSize; }

    [[nodiscard]]
    const char *comment() const noexcept { return mComment; }

    [[nodiscard]]
    const ZipEntry *entryAt(size_t index) const noexcept { return mEntries + index; }


    int uncompress(size_t index, void *buff) const noexcept;

    int uncompress(const ZipEntry *e, void *buff) const noexcept;
};

#endif // ZIP_H