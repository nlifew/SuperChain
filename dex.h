
#ifndef DEX_H
#define DEX_H

#include "types.h"

struct DexHeader
{
    static constexpr int kSHA1DigestLen = 20;
    static constexpr const char *MAGIC = "dex\n";

    u1  magic[4];           // 魔数
    u1  version[4];
    u4  checksum;           // adler 校验值
    u1  signature[kSHA1DigestLen]; // sha1 校验值
    u4  fileSize;           // DEX 文件大小
    u4  headerSize;         // DEX 文件头大小
    u4  endianTag;          // 字节序
    u4  linkSize;           // 链接段大小
    u4  linkOff;            // 链接段的偏移量
    u4  mapOff;             // DexMapList 偏移量
    u4  stringIdsSize;      // DexStringId 个数
    u4  stringIdsOff;       // DexStringId 偏移量
    u4  typeIdsSize;        // DexTypeId 个数
    u4  typeIdsOff;         // DexTypeId 偏移量
    u4  protoIdsSize;       // DexProtoId 个数
    u4  protoIdsOff;        // DexProtoId 偏移量
    u4  fieldIdsSize;       // DexFieldId 个数
    u4  fieldIdsOff;        // DexFieldId 偏移量
    u4  methodIdsSize;      // DexMethodId 个数
    u4  methodIdsOff;       // DexMethodId 偏移量
    u4  classDefsSize;      // DexCLassDef 个数
    u4  classDefsOff;       // DexClassDef 偏移量
    u4  dataSize;           // 数据段大小
    u4  dataOff;            // 数据段偏移量
};

struct DexStringId {
    u4 stringDataOff;
};


struct DexTypeId {
    u4 descriptorIdx;
};

struct DexProtoId {
    u4  shortyIdx;          /* index into stringIds for shorty descriptor */
    u4  returnTypeIdx;      /* index into typeIds list for return type */
    u4  parametersOff;      /* file offset to type_list for parameter types */
};

struct DexTypeItem {
    u2  typeIdx;            /* index into typeIds */
};

struct DexTypeList {
    u4  size;               /* #of entries in list */
    DexTypeItem list[1];    /* entries */
};

struct DexFieldId {
    u2  classIdx;           /* index into typeIds list for defining class */
    u2  typeIdx;            /* index into typeIds for field type */
    u4  nameIdx;            /* index into stringIds for field name */
};

struct DexMethodId {
    u2  classIdx;           /* index into typeIds list for defining class */
    u2  protoIdx;           /* index into protoIds for method prototype */
    u4  nameIdx;            /* index into stringIds for method name */
};

struct DexClassDef {
    u4  classIdx;           /* index into typeIds for this class */
    u4  accessFlags;
    u4  superclassIdx;      /* index into typeIds for superclass */
    u4  interfacesOff;      /* file offset to DexTypeList */
    u4  sourceFileIdx;      /* index into stringIds for source file name */
    u4  annotationsOff;     /* file offset to annotations_directory_item */
    u4  classDataOff;       /* file offset to class_data_item */
    u4  staticValuesOff;    /* file offset to DexEncodedArray */
};


#endif // DEX_H