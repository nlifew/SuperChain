

#include <memory>
#include <algorithm>
#include <iostream>
#include <vector>
#include <zlib.h>

#include "zip.h"

static size_t readFully(FILE *fp, void *dst, size_t size) noexcept
{
    size_t consumed = 0;
    while (consumed < size && !feof(fp) && !ferror(fp)) {
        auto bytes = fread((char *) dst + consumed, 1, size - consumed, fp);
        if (bytes == -1) break;
        consumed += bytes;
    }
    return consumed;
}

struct EOCD
{
    static constexpr u4 MAGIC = 0x06054b50;
    u4 magic;
    u2 diskNumber;
    u2 startDiskNumber;
    u2 entriesOnDisk;
    u2 entriesInDirectory;
    u4 directorySize;
    u4 directoryOffset;
    u2 commentLength;
    char comment[0];
} __attribute__((packed));

struct CDE
{
    static constexpr u4 MAGIC = 0x02014b50;

    u4 magic;
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
    u2 diskNumberStart;
    u2 internalAttributes;
    u4 externalAttributes;
    u4 headerOffset;

    void copyTo(ZipEntry *e) const noexcept
    {
        e->versionMadeBy = versionMadeBy;
        e->versionToExtract = versionToExtract;
        e->flag = flag;
        e->method = method;
        e->mTime = mTime;
        e->mDate = mDate;
        e->crc32 = crc32;
        e->compressedSize = compressedSize;
        e->unCompressedSize = unCompressedSize;
        e->nameLength = nameLength;
        e->extraLength = extraLength;
        e->commentLength = commentLength;
    }
}  __attribute__((packed));

struct LFH
{
    static constexpr u4 MAGIC = 0x04034b50;
    u4 magic;
    u2 version;
    u2 flag;
    u2 method;
    u2 mTime;
    u2 mDate;
    u4 crc32;
    u4 compressedSize;
    u4 uncompressedSize;
    u2 nameLength;
    u2 extraLength;
} __attribute__((packed));

static inline const char *readString(FILE *file, u2 len) noexcept
{
    if (len == 0) {
        return nullptr;
    }
    auto buff = new char[len + 1];
    fread(buff, 1, len, file);
    buff[len] = '\0';
    return buff;
}

int ZipFile::open(const char *path) noexcept
{
    if ((mFile = fopen(path, "rb")) == nullptr) {
        return -1;
    }
    fseek(mFile, 0, SEEK_END);
    auto fileSize = ftell(mFile);

    long buffLength = 64 * 1024 + sizeof(EOCD);
    auto buff = std::make_unique<char[]>(buffLength);

    fseek(mFile, - std::min(buffLength, fileSize), SEEK_CUR);
    buffLength = (long) fread(buff.get(), 1, buffLength, mFile);

    EOCD *eocd = nullptr;

    for (long i = buffLength - sizeof(EOCD); i >= 0; --i) {
        auto tmp = (EOCD *) (buff.get() + i);
        if (tmp->magic == EOCD::MAGIC
            && tmp->diskNumber == 0
            && tmp->startDiskNumber == 0
            && tmp->entriesOnDisk == tmp->entriesInDirectory) {
            eocd = tmp;
            break;
        }
    }
    if (eocd == nullptr) {
        return -1;
    }

    mSize = eocd->entriesOnDisk;
    mEntries = new ZipEntry[mSize];

    if (eocd->commentLength > 0) {
        mComment = new char[eocd->commentLength + 1];
        memcpy((void *) mComment, eocd->comment, eocd->commentLength);
    }

    fseek(mFile, eocd->directoryOffset, SEEK_SET);

    // 遍历 cde 里的每个 entry，收集数据
    CDE cde{};
    for (int i = 0; i < eocd->entriesOnDisk; i ++) {
        fread(&cde, 1, sizeof(CDE), mFile);
        if (cde.magic != CDE::MAGIC) {
            return -1;
        }
        auto e = mEntries + i;
        cde.copyTo(e);
        e->bytesOffset = cde.headerOffset;
        e->name = readString(mFile, e->nameLength);
        e->extra = readString(mFile, e->extraLength);
        e->comment = readString(mFile, e->commentLength);
        cde.magic = 0;
    }
    // 再次遍历每个 entry，修正 cde
    LFH lfh {};
    for (int i = 0; i < eocd->entriesOnDisk; i ++) {
        auto e = mEntries + i;
        fseek(mFile, e->bytesOffset, SEEK_SET);

        fread(&lfh, 1, sizeof(LFH), mFile);
        if (lfh.magic != LFH::MAGIC) {
            return -1;
        }
        fseek(mFile, lfh.extraLength + lfh.nameLength, SEEK_CUR);
        e->bytesOffset = ftell(mFile);
    }

    return 0;
}

void ZipFile::close() noexcept
{
    if (mFile == nullptr) {
        return;
    }
    fclose(mFile);
    mFile = nullptr;

    delete[] mComment;
    mComment = nullptr;

    for (size_t i = 0; i < mSize; i ++) {
        auto e = mEntries + i;
        delete[] e->name;
        delete[] (char *) e->extra;
        delete[] e->comment;
    }
    delete[] mEntries;
    mEntries = nullptr;
    mSize = 0;
}

int ZipFile::uncompress(size_t index, void *buff) const noexcept
{
    return uncompress(entryAt(index), buff);
}

static int uncompressRaw(void *dst, size_t dstLen, const void *src, size_t srcLen) noexcept
{
    z_stream stream{};
    stream.next_in = (Bytef*) src;
    stream.avail_in = (uInt) srcLen;
    stream.next_out = (Bytef *) dst;
    stream.avail_out = (uInt) dstLen;
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK) {
        return -1;
    }
    std::unique_ptr<z_stream, void (*)(z_stream *)> streamGuard(
            &stream, [](z_stream *p) { inflateEnd(p); });

    auto result = inflate(&stream, Z_FINISH);
    if (result != Z_STREAM_END) {
        return -1;
    }
    return 0;
}

int ZipFile::uncompress(const ZipEntry *e, void *out) const noexcept
{
    if (e->flag != 0) {
        return -1;
    }

    uLong crc = 0;
    fseek(mFile, e->bytesOffset, SEEK_SET);

    if (e->method == COMPRESS_STORE) {
        auto consumed = readFully(mFile, out, e->unCompressedSize);
        crc = crc32(crc, (Bytef *) out, consumed);
    }
    else if (e->method == COMPRESS_DEFLATE) {
        auto in = std::make_unique<char[]>(e->compressedSize);
        auto inLen = readFully(mFile, in.get(), e->compressedSize);
        uncompressRaw(out, e->unCompressedSize, in.get(), inLen);
        crc = crc32(crc, (Bytef *) out, e->unCompressedSize);
    }

    if (crc != e->crc32) {
        return -1;
    }
    return 0;
}