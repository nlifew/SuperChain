#include <cstdio>
#include <memory>
#include <algorithm>
#include <unordered_map>
#include <string>
#include <regex>
#include <vector>

#include "types.h"
#include "dex.h"
#include "zip.h"
#include "log.h"


struct DexFile
{
    DexHeader header;
    DexStringId *stringPool;
    DexTypeId *typePool;
    DexClassDef *classes;
    DexFieldId *fields;
    const u1 *data;
    size_t dataCapacity;
    std::string tag;

    std::unordered_map<std::string, DexClassDef *> nameToClassMap;

    [[nodiscard]]
    const char *getTypeName(u2 indexToTypePool) const noexcept
    {
        auto &typeId = typePool[indexToTypePool];
        auto &string = stringPool[typeId.descriptorIdx];
        auto buff = data + string.stringDataOff;
        return (char *) (buff + 1);
    }

    [[nodiscard]]
    const char *getStringAt(u4 index) const noexcept
    {
        auto &string = stringPool[index];
        auto buff = data + string.stringDataOff;
        return (char *) (buff + 1);
    }

    int readFrom(BytesInput &file) noexcept
    {
        file >> header;
        if (memcmp((char *) header.magic, DexHeader::MAGIC, sizeof(header.magic)) != 0) {
            printf("invalid magic\n");
            return -1;
        }
        if (header.endianTag != 0x12345678) {
            printf("invalid endian\n");
            return -1;
        }

        data = (u1 *) file.data();
        dataCapacity = file.length();
        stringPool = (DexStringId *) (data + header.stringIdsOff);
        typePool = (DexTypeId *) (data + header.typeIdsOff);
        classes = (DexClassDef *) (data + header.classDefsOff);
        fields = (DexFieldId *) (data + header.fieldIdsOff);

        for (size_t i = 0, n = header.classDefsSize; i < n; ++i) {
            const char *name = getTypeName(classes[i].classIdx);
            nameToClassMap[name] = &classes[i];
        }

        return 0;
    }

    [[nodiscard]]
    DexClassDef *findClassByName(const char *name) const noexcept
    {
        const auto &it = nameToClassMap.find(name);
        return it == nameToClassMap.end() ? nullptr : it->second;
    }
};

struct Modifier
{
    static constexpr u4 ACC_PUBLIC      = 0x0001;
    static constexpr u4 ACC_PRIVATE     = 0x0002;
    static constexpr u4 ACC_PROTECTED   = 0x0004;
    static constexpr u4 ACC_STATIC      = 0x0008;
    static constexpr u4 ACC_FINAL       = 0x0010;
    static constexpr u4 ACC_VOLATILE    = 0x0040;
    static constexpr u4 ACC_TRANSIENT   = 0x0080;
    static constexpr u4 ACC_SYNTHETIC   = 0x1000;
};

struct DexField {

    u4 fieldIdx;    /* index to a field_id_item */
    u4 accessFlags;
};

//struct DexMethod {
//    u4 methodIdx;    /* index to a method_id_item */
//    u4 accessFlags;
//    u4 codeOff;      /* file offset to a code_item */
//};

struct DexClassData {
    u4 staticFieldsSize;
    u4 instanceFieldsSize;
    u4 directMethodsSize;
    u4 virtualMethodsSize;

//    std::vector<DexField> staticFields;
    std::vector<DexField> instanceFields;
//    std::vector<DexMethod> directMethods;
//    std::vector<DexMethod> virtualMethods;

    static u4 readAsULEB128(BytesInput &in) noexcept
    {
        u4 result = 0, count = 0;
        u1 cur;
        do {
            in >> cur;
            result |= (cur & 0x7f) << ((count ++) * 7);
        } while ((cur & 0x80) == 128 && count < 5);
        return result;
    }


    int readFrom(BytesInput &in) noexcept
    {
//        LOGI("start: 0x%08zx\n", in.where());

        staticFieldsSize = readAsULEB128(in);
        instanceFieldsSize = readAsULEB128(in);
        directMethodsSize = readAsULEB128(in);
        virtualMethodsSize = readAsULEB128(in);

        u4 off = 0;

//        LOGI("read 4 size: 0x%08zx\n", in.where());

//        staticFields.resize(staticFieldsSize);
        for (size_t i = 0; i < staticFieldsSize; ++i) {
//            staticFields[i] = {
                    readAsULEB128(in);
                    readAsULEB128(in);
//            };
        }
//        LOGI("staticFields: 0x%08zx\n", in.where());

        off = 0;
        instanceFields.resize(instanceFieldsSize);
        for (size_t i = 0; i < instanceFieldsSize; ++i) {
            auto fieldIdx = readAsULEB128(in);
            auto accessFlag = readAsULEB128(in);
            instanceFields[i] = {
                    off + fieldIdx,
                    accessFlag,
            };
            off += fieldIdx;
        }
//        LOGI("instanceFields: 0x%08zx\n", in.where());

//        directMethods.resize(directMethodsSize);
        for (size_t i = 0; i < directMethodsSize; i ++) {
//            directMethods[i] = {
                    readAsULEB128(in);
                    readAsULEB128(in);
                    readAsULEB128(in);
//            };
        }
//        LOGI("directMethods: 0x%08zx\n", in.where());

//        virtualMethods.resize(virtualMethodsSize);
        for (size_t i = 0; i < virtualMethodsSize; ++i) {
//            virtualMethods[i] = {
                    readAsULEB128(in);
                    readAsULEB128(in);
                    readAsULEB128(in);
//            };
        }
//        LOGI("virtualMethods: 0x%08zx\n", in.where());

        return 0;
    }
};

class ApkFile
{
public:
    struct ResolvedField;
    using ScanResultPair = std::pair<ResolvedField, ResolvedField>;


    struct ResolvedField
    {
        u4 accessFlag;
        const char *name;
        const char *type;
        const char *declaredClassName;
//        DexClassDef *declaredClass;
//        DexFile *declaredDex;

        static int compare(const ResolvedField& p, const ResolvedField &q) noexcept
        {
            if (&p == &q) return 0;

            int cmp = strcmp(p.name, q.name);
            if (cmp != 0) return cmp;

            cmp = strcmp(p.type, q.type);
            return cmp;
        }
    };

private:
    using Buffer = std::vector<u1>;
    std::vector<Buffer> mBufferVec;
    std::vector<DexFile> mDexVec;

    using ResolvedFieldTable = std::vector<ResolvedField>;
    std::unordered_map<DexClassDef*, ResolvedFieldTable> mResolvedClassMap;

    std::pair<DexFile*, DexClassDef *> findClassByName(const char *name) noexcept
    {
        DexFile *superDex = nullptr;
        DexClassDef *superClassDef = nullptr;

        for (auto &dexIt : mDexVec) {
            auto tmp = dexIt.findClassByName(name);
            if (tmp != nullptr) {
                superDex = &dexIt;
                superClassDef = tmp;
                break;
            }
        }
        return std::make_pair(superDex, superClassDef);
    }

    static ResolvedFieldTable generateFieldTable(DexFile &dex, DexClassDef &classDef) noexcept
    {
        // 如果偏移量为 0，则说明这个类没有这一项数据（比如接口）
        if (classDef.classDataOff == 0) {
            return {};
        }

        BytesInput input(dex.data, dex.dataCapacity);
        input.seek(classDef.classDataOff);
        DexClassData dexClassData {};
        dexClassData.readFrom(input);

        ResolvedFieldTable resolvedFieldTable;

        for (size_t i = 0, n = dexClassData.instanceFieldsSize; i < n; i ++) {
            const auto &dexField = dexClassData.instanceFields[i];
            u4 whiteList = Modifier::ACC_PRIVATE | Modifier::ACC_STATIC | Modifier::ACC_SYNTHETIC;
            if ((whiteList & dexField.accessFlags) != 0) {
                continue;
            }

            const auto &fieldId = dex.fields[dexField.fieldIdx];
            ResolvedField field = {
                    .accessFlag = dexField.accessFlags,
                    .name = dex.getStringAt(fieldId.nameIdx),
                    .type = dex.getTypeName(fieldId.typeIdx),
                    .declaredClassName = dex.getTypeName(classDef.classIdx),
//                    .declaredClass = &classDef,
//                    .declaredDex = &dex,
            };
            resolvedFieldTable.push_back(field);
        }

        std::sort(resolvedFieldTable.begin(), resolvedFieldTable.end(), [](const auto &p, const auto &q) {
            return ResolvedField::compare(p, q) < 0;
        });
        return resolvedFieldTable;
    }
    
    static void findIntersection(
            std::vector<ScanResultPair> *vec,
            const ResolvedFieldTable &self,
            const ResolvedFieldTable &super) noexcept
    {
        size_t i = 0, j = 0;

        while (i < self.size() && j < super.size()) {
            int cmp = ResolvedField::compare(self[i], super[j]);
            if (cmp < 0) {
                i += 1;
            }
            else if (cmp > 0) {
                j += 1;
            }
            else {
                vec->emplace_back(self[i ++], super[j ++]);
            }
        }
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
    void resolveClass(DexFile &dex, DexClassDef &classDef, std::vector<ScanResultPair> *outVec) noexcept
    {
        if (mResolvedClassMap.find(&classDef) != mResolvedClassMap.end()) {
            return;
        }
        LOGD("for class '%s' in dex '%s'\n", dex.getTypeName(classDef.classIdx), dex.tag.c_str());

        // 先保证父类能正常解析完成
        const char *superClassName = dex.getTypeName(classDef.superclassIdx);
        auto [superDex, superClassDef] = findClassByName(superClassName);
        if (superDex != nullptr && superClassDef != nullptr) {
            resolveClass(*superDex, *superClassDef, outVec);
        }

        // 遍历所有的字段，将私有/静态字段排除，放进 table 里
        ResolvedFieldTable fieldTable = generateFieldTable(dex, classDef);

        // 如果能找到父类，则比较字段表和父类的字段表，寻找交集
        if (superClassDef != nullptr) {
            const auto &parentTable = mResolvedClassMap[superClassDef];
            findIntersection(outVec, fieldTable, parentTable);

            for (const auto &it : parentTable) {
                fieldTable.push_back(it);
            }
            std::sort(fieldTable.begin(), fieldTable.end(), [](const auto &p, const auto &q) {
                return ResolvedField::compare(p, q) < 0;
            });
        }

        mResolvedClassMap[&classDef] = fieldTable;
    }
#pragma clang diagnostic pop

public:
    explicit ApkFile() noexcept = default;
    NO_COPY(ApkFile)

    int open(const char *path) noexcept
    {
        LOGD("open zip file: '%s'\n", path);

        ZipFile zipFile;
        if (zipFile.open(path) == -1) {
            PLOGE("failed to open zip file '%s'\n", path);
            return -1;
        }

        std::regex reg("^classes\\d*.dex$");

        for (size_t i = 0, n = zipFile.size(); i < n; ++i) {
            auto e = zipFile.entryAt(i);
            if (! std::regex_match(e->name, reg)) {
                continue;
            }
            LOGD("unzip entry '%s' at index '%zu', size = '%u;\n", e->name, i, e->unCompressedSize);
            Buffer &buffer = mBufferVec.emplace_back(e->unCompressedSize);
            if (zipFile.uncompress(e, &buffer[0]) == -1) {
                LOGE("failed to unzip entry '%s' at index '%zu', ignore ...\n", e->name, i);
                return -1;
            }
            BytesInput input(buffer.data(), buffer.size());
            DexFile dexFile {};
            if (dexFile.readFrom(input) == -1) {
                LOGE("entry '%s' at '%zu' is NOT a .dex file\n", e->name, i);
                return -1;
            }
            dexFile.tag = e->name;
//            mBufferVec.push_back(buffer);
            mDexVec.push_back(dexFile);
        }
        return 0;
    }

    std::vector<ScanResultPair> scanAll() noexcept
    {
        std::vector<ScanResultPair> vec;

        for (auto &dex : mDexVec) {
            LOGD("here %s\n", dex.tag.c_str());
            for (size_t i = 0, n = dex.header.classDefsSize; i < n; ++i) {
                auto &klass = dex.classes[i];
                resolveClass(dex, klass, &vec);
            }
        }
        return vec;
    }
};


int main(int argc, const char *argv[])
{
    if (argc <= 1) {
        LOGI("usage: %s [apkPath]\n", argv[0]);
        return 1;
    }

    ApkFile apkFile;
    if (apkFile.open(argv[1]) < 0) {
        return 1;
    }
    for (const auto &it : apkFile.scanAll()) {
        const auto &p = it.first;
        const auto  &q = it.second;

        LOGI("%s->%s:%s' <==> '%s->%s:%s\n",
             p.declaredClassName, p.name, p.type,
             q.declaredClassName, q.name, q.type);
    }

    return 0;
}
