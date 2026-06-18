#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <stdafx.h>
#ifdef __x86_64__
#include <cpuid.h>
#endif
#include <cpu/guest_thread.h>
#include <gpu/video.h>
#include <kernel/function.h>
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/xam.h>
#include <kernel/save_system.h>
#include <kernel/io/file_system.h>
#include <kernel/vfs.h>
#include <file.h>
#include <vector>
#include <image.h>
#include <xex.h>
#ifdef __SWITCH__
#include <aes.hpp>
#endif
#include <apu/audio.h>
#include <hid/hid.h>
#include <user/config.h>
#include <user/paths.h>
#include <user/registry.h>
#include <kernel/xdbf.h>
#include <install/installer.h>
#include <install/platform_paths.h>
#include <install/update_checker.h>
#include <os/logger.h>
#include <os/process.h>
#include <os/registry.h>
#include <ui/game_window.h>
#include <ui/installer_wizard.h>
#include <mod/mod_loader.h>
#include <preload_executable.h>
#include <iostream>
#include <app.h>

#ifdef _WIN32
#include <timeapi.h>
#endif

#ifdef __SWITCH__
#include <sys/stat.h>

extern "C" {
uint32_t fsdevMountSdmc(void);
int fsdevUnmountDevice(const char* name);
void* fsdevGetDeviceFileSystem(const char* name);
uint32_t romfsMountSelf(const char* name);
uint32_t romfsUnmount(const char* name);
uint32_t svcOutputDebugString(const char* str, uint64_t size);
void svcSleepThread(int64_t ns);
bool envIsNso(void);
void LibertySwitchShowAuditDiagnostic(const char* title, const char* status, const char* detailPath, const char* logPath);
}

static constexpr const char* SWITCH_AUDIT_LOG_PATH = "sdmc:/switch/LibertyRecomp/LibertyRecomp.log";
static constexpr const char* SWITCH_AUDIT_CONTENT_ROOT = "sdmc:/switch/LibertyRecomp";
static constexpr const char* SWITCH_AUDIT_GAME_ROOT = "sdmc:/switch/LibertyRecomp/game";
static constexpr const char* SWITCH_AUDIT_MODULE_PATH = "sdmc:/switch/LibertyRecomp/game/default.xex";

static void SwitchAuditDebugRaw(const char* message)
{
    svcOutputDebugString(message, strlen(message));
}

__attribute__((constructor(101)))
static void SwitchAuditPremainProbe()
{
    SwitchAuditDebugRaw("[Switch] premain constructor 101 entered.\n");
}

static bool SwitchAuditSdmcAvailable()
{
    if (fsdevGetDeviceFileSystem("sdmc") != nullptr)
        return true;

    struct stat sdmcStat;
    return stat("sdmc:/", &sdmcStat) == 0;
}

static bool SwitchAuditMountSdmc(uint32_t& mountResult, bool& mountedHere)
{
    mountedHere = false;
    if (SwitchAuditSdmcAvailable())
    {
        mountResult = 0;
        return true;
    }

    mountResult = fsdevMountSdmc();
    if (mountResult == 0)
    {
        mountedHere = true;
        return true;
    }

    return SwitchAuditSdmcAvailable();
}

static void SwitchAuditLog(const char* message, const std::filesystem::path& path = {})
{
    mkdir("sdmc:/switch", 0777);
    mkdir("sdmc:/switch/LibertyRecomp", 0777);
    mkdir("sdmc:/switch/LibertyRecomp/game", 0777);

    std::string line = std::string("[Switch] ") + message;
    if (!path.empty())
        line += " " + path.string();
    line += "\n";

    FILE* log = fopen(SWITCH_AUDIT_LOG_PATH, "a");
    if (log != nullptr)
    {
        fputs(line.c_str(), log);
        fclose(log);
    }

    svcOutputDebugString(line.c_str(), static_cast<uint64_t>(line.size()));
    fputs(line.c_str(), stderr);
}

static void SwitchAuditUnmount(bool romfsMounted, bool sdmcMounted)
{
    if (romfsMounted)
        romfsUnmount("romfs");

    if (sdmcMounted)
        fsdevUnmountDevice("sdmc");
}

static bool SwitchAuditFileExists(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == nullptr)
        return false;

    fclose(file);
    return true;
}

#if defined(__SWITCH__)
static std::filesystem::path SwitchSelectAudioRoot(const std::filesystem::path& gameRoot)
{
    std::error_code ec;
    const std::filesystem::path audioRoot = gameRoot / "audio";
    if (std::filesystem::is_directory(audioRoot, ec))
        return audioRoot;

    const std::filesystem::path platformAudioRoot = gameRoot / "xbox360" / "audio";
    if (std::filesystem::is_directory(platformAudioRoot, ec))
        return platformAudioRoot;

    return audioRoot;
}
#endif

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONTENT_LAYOUT_CHECK) || defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_VFS_PREFLIGHT) || defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT)
static bool SwitchAuditFileExists(const std::filesystem::path& path)
{
    const std::string pathString = path.string();
    return SwitchAuditFileExists(pathString.c_str());
}

static bool SwitchAuditDirectoryExists(const std::filesystem::path& path)
{
    const std::string pathString = path.string();
    struct stat pathStat;
    if (stat(pathString.c_str(), &pathStat) != 0)
        return false;

    return S_ISDIR(pathStat.st_mode);
}

static void SwitchAuditLogPresence(const char* label, const std::filesystem::path& path, bool present)
{
    std::string message = std::string(label) + (present ? " present:" : " missing:");
    SwitchAuditLog(message.c_str(), path);
}

static void SwitchAuditLogf(const char* format, ...)
{
    char message[512];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    SwitchAuditLog(message);
}

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT)
struct SwitchAuditImageDosHeader
{
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res[4];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2[10];
    uint32_t e_lfanew;
};

struct SwitchAuditImageFileHeader
{
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

struct SwitchAuditImageDataDirectory
{
    uint32_t VirtualAddress;
    uint32_t Size;
};

struct SwitchAuditImageOptionalHeader32
{
    uint16_t Magic;
    uint8_t MajorLinkerVersion;
    uint8_t MinorLinkerVersion;
    uint32_t SizeOfCode;
    uint32_t SizeOfInitializedData;
    uint32_t SizeOfUninitializedData;
    uint32_t AddressOfEntryPoint;
    uint32_t BaseOfCode;
    uint32_t BaseOfData;
    uint32_t ImageBase;
    uint32_t SectionAlignment;
    uint32_t FileAlignment;
    uint16_t MajorOperatingSystemVersion;
    uint16_t MinorOperatingSystemVersion;
    uint16_t MajorImageVersion;
    uint16_t MinorImageVersion;
    uint16_t MajorSubsystemVersion;
    uint16_t MinorSubsystemVersion;
    uint32_t Win32VersionValue;
    uint32_t SizeOfImage;
    uint32_t SizeOfHeaders;
    uint32_t CheckSum;
    uint16_t Subsystem;
    uint16_t DllCharacteristics;
    uint32_t SizeOfStackReserve;
    uint32_t SizeOfStackCommit;
    uint32_t SizeOfHeapReserve;
    uint32_t SizeOfHeapCommit;
    uint32_t LoaderFlags;
    uint32_t NumberOfRvaAndSizes;
    SwitchAuditImageDataDirectory DataDirectory[16];
};

struct SwitchAuditImageNtHeaders32
{
    uint32_t Signature;
    SwitchAuditImageFileHeader FileHeader;
    SwitchAuditImageOptionalHeader32 OptionalHeader;
};

struct SwitchAuditImageSectionHeader
{
    uint8_t Name[8];
    union
    {
        uint32_t PhysicalAddress;
        uint32_t VirtualSize;
    } Misc;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};

struct SwitchAuditSectionRange
{
    char name[9];
    uint32_t virtualAddress;
    uint32_t virtualSize;
};

struct SwitchAuditBasicBlockRange
{
    size_t imageOffset;
    size_t sourceOffset;
    size_t dataSize;
    size_t zeroSize;
};

struct SwitchAuditStagedImageView
{
    const uint8_t* srcData{};
    size_t sourceSize{};
    size_t imageSize{};
    uint32_t imageBase{};
    std::vector<SwitchAuditBasicBlockRange> blocks{};
};

static bool SwitchAuditRangeWithin(size_t totalSize, size_t offset, size_t length)
{
    return offset <= totalSize && length <= totalSize - offset;
}

static size_t SwitchAuditBoundedStringLength(const char* text, size_t maxLength)
{
    size_t length = 0;
    while (length < maxLength && text[length] != '\0')
        length++;
    return length;
}

static bool SwitchAuditReadStagedImage(
    const std::vector<SwitchAuditBasicBlockRange>& blocks,
    const uint8_t* srcData,
    size_t imageSize,
    size_t offset,
    void* output,
    size_t readSize)
{
    if (!SwitchAuditRangeWithin(imageSize, offset, readSize))
        return false;

    auto* writeData = static_cast<uint8_t*>(output);
    size_t remaining = readSize;
    size_t readOffset = offset;
    while (remaining > 0)
    {
        bool foundRange = false;
        for (const SwitchAuditBasicBlockRange& block : blocks)
        {
            const size_t blockSize = block.dataSize + block.zeroSize;
            if (readOffset < block.imageOffset || readOffset >= block.imageOffset + blockSize)
                continue;

            const size_t blockOffset = readOffset - block.imageOffset;
            const size_t availableInBlock = blockSize - blockOffset;
            const size_t copySize = remaining < availableInBlock ? remaining : availableInBlock;
            if (blockOffset < block.dataSize)
            {
                const size_t dataCopySize = copySize < (block.dataSize - blockOffset) ? copySize : (block.dataSize - blockOffset);
                memcpy(writeData, srcData + block.sourceOffset + blockOffset, dataCopySize);
                if (dataCopySize < copySize)
                    memset(writeData + dataCopySize, 0, copySize - dataCopySize);
            }
            else
            {
                memset(writeData, 0, copySize);
            }

            writeData += copySize;
            readOffset += copySize;
            remaining -= copySize;
            foundRange = true;
            break;
        }

        if (!foundRange)
            return false;
    }

    return true;
}

static bool SwitchAuditFindImageOffset(
    const std::vector<SwitchAuditSectionRange>& sections,
    size_t imageSize,
    uint32_t imageBase,
    uint32_t address,
    size_t readSize,
    size_t& imageOffset)
{
    for (const SwitchAuditSectionRange& section : sections)
    {
        const uint32_t sectionBase = imageBase + section.virtualAddress;
        const uint32_t sectionSize = section.virtualSize;
        if (address < sectionBase || address - sectionBase > sectionSize)
            continue;

        const size_t sectionOffset = static_cast<size_t>(address - sectionBase);
        imageOffset = static_cast<size_t>(section.virtualAddress) + sectionOffset;
        if (sectionOffset <= sectionSize && readSize <= static_cast<size_t>(sectionSize) - sectionOffset &&
            SwitchAuditRangeWithin(imageSize, imageOffset, readSize))
        {
            return true;
        }
    }

    return false;
}

static bool SwitchAuditPublishStagedImageView(
    SwitchAuditStagedImageView* stagedImage,
    const uint8_t* srcData,
    size_t sourceSize,
    size_t imageSize,
    uint32_t imageBase,
    const std::vector<SwitchAuditBasicBlockRange>& imageBlocks)
{
    if (stagedImage == nullptr)
        return true;

    if (srcData == nullptr || imageSize == 0 || imageBlocks.empty() || imageSize > UINT32_MAX)
    {
        SwitchAuditLog("Switch module-load preflight audit: staged image publish failed: invalid staged view.");
        return false;
    }

    stagedImage->srcData = srcData;
    stagedImage->sourceSize = sourceSize;
    stagedImage->imageSize = imageSize;
    stagedImage->imageBase = imageBase;
    stagedImage->blocks = imageBlocks;
    SwitchAuditLogf(
        "Switch module-load preflight audit: staged image view published base=0x%08X size=0x%llX blocks=%llu.",
        imageBase,
        static_cast<unsigned long long>(imageSize),
        static_cast<unsigned long long>(imageBlocks.size()));
    return true;
}

static bool SwitchAuditProbeXexFullParsePhases(uint8_t* data, size_t dataSize, SwitchAuditStagedImageView* stagedImage)
{
    SwitchAuditLog("Switch module-load preflight audit: full-parse phase probe begin.");

    if (!SwitchAuditRangeWithin(dataSize, 0, sizeof(Xex2Header)))
    {
        SwitchAuditLog("Switch module-load preflight audit: full-parse phase probe failed: missing XEX header.");
        return false;
    }

    const auto* header = reinterpret_cast<const Xex2Header*>(data);
    const uint32_t headerSize = header->headerSize;
    const uint32_t securityOffset = header->securityOffset;
    if (!SwitchAuditRangeWithin(dataSize, securityOffset, sizeof(Xex2SecurityInfo)) ||
        headerSize > dataSize)
    {
        SwitchAuditLog("Switch module-load preflight audit: full-parse phase probe failed: invalid XEX bounds.");
        return false;
    }

    const auto* security = reinterpret_cast<const Xex2SecurityInfo*>(data + securityOffset);
    const auto* fileFormatInfo = reinterpret_cast<const Xex2OptFileFormatInfo*>(getOptHeaderPtr(data, XEX_HEADER_FILE_FORMAT_INFO));
    if (fileFormatInfo == nullptr)
    {
        SwitchAuditLog("Switch module-load preflight audit: full-parse phase probe failed: missing file-format info.");
        return false;
    }

    const uint32_t imageBase = [&]() {
        const auto* imageBasePtr = reinterpret_cast<const be<uint32_t>*>(getOptHeaderPtr(data, XEX_HEADER_IMAGE_BASE_ADDRESS));
        return imageBasePtr != nullptr ? static_cast<uint32_t>(*imageBasePtr) : static_cast<uint32_t>(security->loadAddress);
    }();
    const size_t payloadSize = dataSize - headerSize;
    const uint16_t encryptionType = static_cast<uint16_t>(fileFormatInfo->encryptionType);
    const uint16_t compressionType = static_cast<uint16_t>(fileFormatInfo->compressionType);

    SwitchAuditLogf(
        "Switch module-load preflight audit: phase setup payload=%llu imageSize=0x%X enc=%u comp=%u infoSize=0x%X imageBase=0x%08X",
        static_cast<unsigned long long>(payloadSize),
        static_cast<uint32_t>(security->imageSize),
        static_cast<unsigned>(encryptionType),
        static_cast<unsigned>(compressionType),
        static_cast<uint32_t>(fileFormatInfo->infoSize),
        imageBase);

    const uint8_t* srcData = data + headerSize;
    if (encryptionType == XEX_ENCRYPTION_NORMAL)
    {
        if ((payloadSize % 16) != 0)
        {
            SwitchAuditLog("Switch module-load preflight audit: decrypt phase failed: payload is not AES block aligned.");
            return false;
        }

        constexpr uint32_t KeySize = 16;
        AES_ctx aesContext;
        uint8_t decryptedKey[KeySize];
        memcpy(decryptedKey, security->aesKey, KeySize);

        SwitchAuditLog("Switch module-load preflight audit: decrypt key phase begin.");
        AES_init_ctx_iv(&aesContext, Xex2RetailKey, AESBlankIV);
        AES_CBC_decrypt_buffer(&aesContext, decryptedKey, KeySize);
        SwitchAuditLog("Switch module-load preflight audit: decrypt key phase complete.");

        SwitchAuditLog("Switch module-load preflight audit: decrypt image phase begin using in-place payload buffer.");
        AES_init_ctx_iv(&aesContext, decryptedKey, AESBlankIV);
        constexpr size_t DecryptChunkSize = 256 * 1024;
        size_t decryptedOffset = 0;
        while (decryptedOffset < payloadSize)
        {
            const size_t remaining = payloadSize - decryptedOffset;
            const size_t chunkSize = remaining < DecryptChunkSize ? remaining : DecryptChunkSize;
            SwitchAuditLogf(
                "Switch module-load preflight audit: decrypt image chunk begin offset=0x%llX size=0x%llX.",
                static_cast<unsigned long long>(decryptedOffset),
                static_cast<unsigned long long>(chunkSize));
            AES_CBC_decrypt_buffer(&aesContext, data + headerSize + decryptedOffset, chunkSize);
            decryptedOffset += chunkSize;
            SwitchAuditLogf(
                "Switch module-load preflight audit: decrypt image chunk complete offset=0x%llX.",
                static_cast<unsigned long long>(decryptedOffset));
        }
        srcData = data + headerSize;
        SwitchAuditLog("Switch module-load preflight audit: decrypt image phase complete.");
    }
    else if (encryptionType == XEX_ENCRYPTION_NONE)
    {
        SwitchAuditLog("Switch module-load preflight audit: decrypt image phase skipped: image is not encrypted.");
    }
    else
    {
        SwitchAuditLogf("Switch module-load preflight audit: decrypt phase failed: unsupported encryption type %u.", static_cast<unsigned>(encryptionType));
        return false;
    }

    std::vector<SwitchAuditBasicBlockRange> imageBlocks;
    size_t imageSize = static_cast<uint32_t>(security->imageSize);
    if (compressionType == XEX_COMPRESSION_NONE)
    {
        if (imageSize > payloadSize)
        {
            SwitchAuditLog("Switch module-load preflight audit: uncompressed copy phase failed: image exceeds payload.");
            return false;
        }

        imageBlocks.push_back({ 0, 0, imageSize, 0 });
        SwitchAuditLog("Switch module-load preflight audit: uncompressed staged image view ready.");
    }
    else if (compressionType == XEX_COMPRESSION_BASIC)
    {
        const size_t infoSize = static_cast<uint32_t>(fileFormatInfo->infoSize);
        if (infoSize < sizeof(Xex2FileBasicCompressionInfo) || (infoSize % sizeof(Xex2FileBasicCompressionInfo)) != 0)
        {
            SwitchAuditLog("Switch module-load preflight audit: basic decompression failed: invalid block info size.");
            return false;
        }

        const auto* blocks = reinterpret_cast<const Xex2FileBasicCompressionBlock*>(fileFormatInfo + 1);
        const size_t numBlocks = (infoSize / sizeof(Xex2FileBasicCompressionInfo)) - 1;
        size_t expectedImageSize = 0;
        size_t compressedBytes = 0;
        imageBlocks.reserve(numBlocks);
        for (size_t i = 0; i < numBlocks; i++)
        {
            const size_t dataBytes = static_cast<uint32_t>(blocks[i].dataSize);
            const size_t zeroBytes = static_cast<uint32_t>(blocks[i].zeroSize);
            if (dataBytes > payloadSize - compressedBytes)
            {
                SwitchAuditLogf("Switch module-load preflight audit: basic decompression failed: block %llu exceeds payload.", static_cast<unsigned long long>(i));
                return false;
            }
            imageBlocks.push_back({ expectedImageSize, compressedBytes, dataBytes, zeroBytes });
            compressedBytes += dataBytes;
            expectedImageSize += dataBytes + zeroBytes;
        }

        SwitchAuditLogf(
            "Switch module-load preflight audit: staged basic decompression view ready blocks=%llu compressedBytes=%llu expectedImageSize=0x%llX.",
            static_cast<unsigned long long>(numBlocks),
            static_cast<unsigned long long>(compressedBytes),
            static_cast<unsigned long long>(expectedImageSize));

        imageSize = expectedImageSize;
    }
    else
    {
        SwitchAuditLogf("Switch module-load preflight audit: decompression phase failed: unsupported compression type %u.", static_cast<unsigned>(compressionType));
        return false;
    }

    if (!SwitchAuditRangeWithin(imageSize, 0, sizeof(SwitchAuditImageDosHeader)))
    {
        SwitchAuditLog("Switch module-load preflight audit: PE scan failed: image too small for DOS header.");
        return false;
    }

    SwitchAuditLog("Switch module-load preflight audit: PE section scan phase begin.");
    SwitchAuditImageDosHeader dosHeader{};
    if (!SwitchAuditReadStagedImage(imageBlocks, srcData, imageSize, 0, &dosHeader, sizeof(dosHeader)) ||
        dosHeader.e_magic != 0x5A4D ||
        !SwitchAuditRangeWithin(imageSize, dosHeader.e_lfanew, sizeof(SwitchAuditImageNtHeaders32)))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: PE scan failed: magic=0x%04X e_lfanew=0x%X imageSize=0x%llX.",
            dosHeader.e_magic,
            dosHeader.e_lfanew,
            static_cast<unsigned long long>(imageSize));
        return false;
    }

    SwitchAuditImageNtHeaders32 ntHeaders{};
    if (!SwitchAuditReadStagedImage(imageBlocks, srcData, imageSize, dosHeader.e_lfanew, &ntHeaders, sizeof(ntHeaders)))
    {
        SwitchAuditLog("Switch module-load preflight audit: PE scan failed: NT headers could not be read.");
        return false;
    }

    const size_t sectionTableOffset = static_cast<size_t>(dosHeader.e_lfanew) + sizeof(uint32_t) + sizeof(SwitchAuditImageFileHeader) + ntHeaders.FileHeader.SizeOfOptionalHeader;
    const size_t sectionTableSize = static_cast<size_t>(ntHeaders.FileHeader.NumberOfSections) * sizeof(SwitchAuditImageSectionHeader);
    if (ntHeaders.Signature != 0x00004550 ||
        !SwitchAuditRangeWithin(imageSize, sectionTableOffset, sectionTableSize))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: PE scan failed: signature=0x%08X sectionOffset=0x%llX sectionBytes=0x%llX.",
            ntHeaders.Signature,
            static_cast<unsigned long long>(sectionTableOffset),
            static_cast<unsigned long long>(sectionTableSize));
        return false;
    }

    std::vector<SwitchAuditSectionRange> sections;
    for (uint16_t i = 0; i < ntHeaders.FileHeader.NumberOfSections; i++)
    {
        SwitchAuditImageSectionHeader section{};
        const size_t sectionOffset = sectionTableOffset + static_cast<size_t>(i) * sizeof(SwitchAuditImageSectionHeader);
        if (!SwitchAuditReadStagedImage(imageBlocks, srcData, imageSize, sectionOffset, &section, sizeof(section)))
        {
            SwitchAuditLogf("Switch module-load preflight audit: PE section %u read failed.", static_cast<unsigned>(i));
            return false;
        }

        SwitchAuditSectionRange range{};
        memcpy(range.name, section.Name, sizeof(section.Name));
        range.name[sizeof(range.name) - 1] = '\0';
        range.virtualAddress = section.VirtualAddress;
        range.virtualSize = section.Misc.VirtualSize;
        sections.push_back(range);

        SwitchAuditLogf(
            "Switch module-load preflight audit: PE section %u name=%s va=0x%X size=0x%X raw=0x%X.",
            static_cast<unsigned>(i),
            range.name,
            section.VirtualAddress,
            section.Misc.VirtualSize,
            section.SizeOfRawData);
    }
    SwitchAuditLogf(
        "Switch module-load preflight audit: PE section scan phase complete sections=%u.",
        static_cast<unsigned>(ntHeaders.FileHeader.NumberOfSections));

    SwitchAuditLog("Switch module-load preflight audit: import thunk scan phase begin.");
    const auto* imports = reinterpret_cast<const Xex2ImportHeader*>(getOptHeaderPtr(data, XEX_HEADER_IMPORT_LIBRARIES));
    if (imports == nullptr)
    {
        SwitchAuditLog("Switch module-load preflight audit: import thunk scan phase skipped: no import header.");
        return SwitchAuditPublishStagedImageView(stagedImage, srcData, payloadSize, imageSize, imageBase, imageBlocks);
    }

    const size_t importsOffset = reinterpret_cast<const uint8_t*>(imports) - data;
    const uint32_t importCount = static_cast<uint32_t>(imports->numImports);
    const uint32_t stringTableSize = static_cast<uint32_t>(imports->sizeOfStringTable);
    if (!SwitchAuditRangeWithin(dataSize, importsOffset, sizeof(Xex2ImportHeader) + stringTableSize))
    {
        SwitchAuditLog("Switch module-load preflight audit: import thunk scan failed: import header/string table out of range.");
        return false;
    }

    const char* stringTable = reinterpret_cast<const char*>(imports + 1);
    size_t paddedStringOffset = 0;
    size_t totalThunkDescriptors = 0;
    size_t missingThunkTargets = 0;
    const uint8_t* libraryPtr = reinterpret_cast<const uint8_t*>(imports) + sizeof(Xex2ImportHeader) + stringTableSize;
    for (uint32_t i = 0; i < importCount; i++)
    {
        if (paddedStringOffset >= stringTableSize)
        {
            SwitchAuditLogf("Switch module-load preflight audit: import thunk scan failed: string table ended before library %u.", i);
            return false;
        }

        const char* libraryName = stringTable + paddedStringOffset;
        const size_t libraryNameLength = SwitchAuditBoundedStringLength(libraryName, stringTableSize - paddedStringOffset);
        if (libraryNameLength >= stringTableSize - paddedStringOffset)
        {
            SwitchAuditLogf("Switch module-load preflight audit: import thunk scan failed: unterminated library name %u.", i);
            return false;
        }
        paddedStringOffset += ((libraryNameLength + 1) + 3) & ~static_cast<size_t>(3);

        const size_t libraryOffset = libraryPtr - data;
        if (!SwitchAuditRangeWithin(dataSize, libraryOffset, sizeof(Xex2ImportLibrary)))
        {
            SwitchAuditLogf("Switch module-load preflight audit: import thunk scan failed: library %u out of range.", i);
            return false;
        }

        const auto* library = reinterpret_cast<const Xex2ImportLibrary*>(libraryPtr);
        const uint16_t numberOfImports = static_cast<uint16_t>(library->numberOfImports);
        const size_t descriptorBytes = static_cast<size_t>(numberOfImports) * sizeof(Xex2ImportDescriptor);
        const size_t descriptorOffset = libraryOffset + sizeof(Xex2ImportLibrary);
        if (!SwitchAuditRangeWithin(dataSize, descriptorOffset, descriptorBytes))
        {
            SwitchAuditLogf("Switch module-load preflight audit: import thunk scan failed: descriptors for %s out of range.", libraryName);
            return false;
        }

        size_t libraryFunctionThunks = 0;
        size_t libraryMissingThunks = 0;
        const auto* descriptors = reinterpret_cast<const Xex2ImportDescriptor*>(libraryPtr + sizeof(Xex2ImportLibrary));
        for (uint16_t descriptorIndex = 0; descriptorIndex < numberOfImports; descriptorIndex++)
        {
            const uint32_t firstThunk = static_cast<uint32_t>(descriptors[descriptorIndex].firstThunk);
            size_t thunkImageOffset = 0;
            if (!SwitchAuditFindImageOffset(sections, imageSize, imageBase, firstThunk, sizeof(uint32_t), thunkImageOffset))
            {
                libraryMissingThunks++;
                missingThunkTargets++;
                continue;
            }

            uint32_t thunkData = 0;
            if (!SwitchAuditReadStagedImage(imageBlocks, srcData, imageSize, thunkImageOffset, &thunkData, sizeof(thunkData)))
            {
                libraryMissingThunks++;
                missingThunkTargets++;
                continue;
            }
            thunkData = ByteSwap(thunkData);
            const uint32_t thunkType = (thunkData >> 24) & 0xFF;
            if (thunkType != XEX_THUNK_VARIABLE)
                libraryFunctionThunks++;
        }

        totalThunkDescriptors += numberOfImports;
        SwitchAuditLogf(
            "Switch module-load preflight audit: import library %u name=%s descriptors=%u functionThunks=%llu missingThunkTargets=%llu.",
            i,
            libraryName,
            static_cast<unsigned>(numberOfImports),
            static_cast<unsigned long long>(libraryFunctionThunks),
            static_cast<unsigned long long>(libraryMissingThunks));

        libraryPtr += sizeof(Xex2ImportLibrary) + descriptorBytes;
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: import thunk scan phase complete libraries=%u descriptors=%llu missingThunkTargets=%llu.",
        static_cast<unsigned>(importCount),
        static_cast<unsigned long long>(totalThunkDescriptors),
        static_cast<unsigned long long>(missingThunkTargets));

    return SwitchAuditPublishStagedImageView(stagedImage, srcData, payloadSize, imageSize, imageBase, imageBlocks);
}
#endif
#endif
#endif

#if defined(_WIN32) && defined(LIBERTY_RECOMP_D3D12)
static std::array<std::string_view, 3> g_D3D12RequiredModules =
{
    "D3D12/D3D12Core.dll",
    "dxcompiler.dll",
    "dxil.dll"
};
#endif

const size_t XMAIOBegin = 0x7FEA0000;
const size_t XMAIOEnd = XMAIOBegin + 0x0000FFFF;

#ifndef __SWITCH__
static std::filesystem::path SelectAudioRoot(const std::filesystem::path& gameRoot)
{
    const std::filesystem::path topLevelAudio = gameRoot / "audio";
    const std::filesystem::path xbox360Audio = gameRoot / "xbox360" / "audio";
    std::error_code ec;

    if (std::filesystem::exists(topLevelAudio, ec))
        return topLevelAudio;

    ec.clear();
    if (std::filesystem::exists(xbox360Audio, ec))
        return xbox360Audio;

    return topLevelAudio;
}

static void LogWindowsContentPreflight(const std::filesystem::path& gameRoot)
{
    auto logPresence = [](const char* label, const std::filesystem::path& path)
    {
        std::error_code ec;
        const bool exists = std::filesystem::exists(path, ec);
        printf("[Main] Windows VFS preflight: %s %s -> %s\n",
               label, exists ? "FOUND" : "MISSING", path.string().c_str());
        fflush(stdout);
        LOGF_IMPL(Utility, "Main", "Windows VFS preflight: {} {} -> {}",
                  label, exists ? "FOUND" : "MISSING", path.string());
    };

    logPresence("game root", gameRoot);
    logPresence("default.xex", gameRoot / "default.xex");
    logPresence("common extracted dir", gameRoot / "common");
    logPresence("platform extracted dir", gameRoot / "xbox360");
    logPresence("platform textures dir", gameRoot / "xbox360" / "textures");
    logPresence("audio extracted dir", gameRoot / "audio");
    logPresence("xbox360 audio dir", gameRoot / "xbox360" / "audio");
    logPresence("common.rpf source archive", gameRoot / "common.rpf");
    logPresence("xbox360.rpf source archive", gameRoot / "xbox360.rpf");
    logPresence("audio.rpf source archive", gameRoot / "audio.rpf");
    logPresence("install aes_key.bin", PlatformPaths::GetAesKeyPath());
    logPresence("bundled aes_key.bin", PlatformPaths::GetBundledAesKeyPath());
}
#endif

Memory g_memory;
Heap g_userHeap;
XDBFWrapper g_xdbfWrapper;
std::unordered_map<uint16_t, GuestTexture*> g_xdbfTextureCache;

#if defined(__SWITCH__) && defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT)
static bool SwitchAuditGuestRangeEnd(uint32_t start, uint32_t size, uint64_t& endExclusive) noexcept
{
    if (size == 0)
        return false;

    const uint64_t end = static_cast<uint64_t>(start) + static_cast<uint64_t>(size);
    if (end > PPC_MEMORY_SIZE)
        return false;

    endExclusive = end;
    return true;
}

static bool SwitchAuditTouchGuestAddress(const char* label, const char* point, uint32_t address)
{
    SwitchAuditLogf(
        "Switch module-load preflight audit: touching guest range %s %s addr=0x%08X.",
        label,
        point,
        address);

    auto* ptr = static_cast<volatile uint8_t*>(g_memory.Translate(address));
    const uint8_t original = *ptr;
    const uint8_t probe = static_cast<uint8_t>(original ^ 0xA5u);
    *ptr = probe;
    const uint8_t observed = *ptr;
    *ptr = original;

    if (observed != probe)
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: guest range %s %s touch verify failed addr=0x%08X expected=0x%02X observed=0x%02X.",
            label,
            point,
            address,
            static_cast<unsigned>(probe),
            static_cast<unsigned>(observed));
        return false;
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: guest range %s %s touch ok addr=0x%08X.",
        label,
        point,
        address);
    return true;
}

static bool SwitchAuditProbeGuestRange(const char* label, uint32_t start, uint32_t size)
{
    uint64_t endExclusive = 0;
    if (!SwitchAuditGuestRangeEnd(start, size, endExclusive))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: guest range %s invalid start=0x%08X size=0x%08X.",
            label,
            start,
            size);
        return false;
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: guest range %s planned start=0x%08X end=0x%llX size=0x%08X.",
        label,
        start,
        static_cast<unsigned long long>(endExclusive),
        size);

    if (g_memory.base == nullptr)
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: guest range %s touch skipped because guest memory is disabled.",
            label);
        return true;
    }

    if (!SwitchAuditTouchGuestAddress(label, "begin", start))
        return false;

    const uint32_t lastAddress = static_cast<uint32_t>(endExclusive - 1);
    if (lastAddress != start && !SwitchAuditTouchGuestAddress(label, "end", lastAddress))
        return false;

    return true;
}

static bool SwitchAuditProbeLdrGuestRanges(
    uint32_t imageBase,
    uint32_t imageSize,
    uint32_t resourceOffset,
    uint32_t resourceSize)
{
    SwitchAuditLogf(
        "Switch module-load preflight audit: guest memory base=%p.",
        static_cast<void*>(g_memory.base));

    bool ok = true;
    ok = SwitchAuditProbeGuestRange("image copy", imageBase, imageSize) && ok;

    if (resourceOffset != 0 && resourceSize != 0)
        ok = SwitchAuditProbeGuestRange("resource", resourceOffset, resourceSize) && ok;
    else
        SwitchAuditLog("Switch module-load preflight audit: resource range skipped because metadata is absent.");

    ok = SwitchAuditProbeGuestRange("collision zero", 0x82003880, 0x80) && ok;
    ok = SwitchAuditProbeGuestRange("stream struct", 0x82003890, 0x1C) && ok;
    ok = SwitchAuditProbeGuestRange("worker globals", 0x830F5000, 0x3000) && ok;

    if (ok)
        SwitchAuditLog("Switch module-load preflight audit: guest range translate/touch probe complete.");
    else
        SwitchAuditLog("Switch module-load preflight audit: guest range translate/touch probe failed.");

    return ok;
}

static bool SwitchAuditVerifyZeroFill(const uint8_t* data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        if (data[i] != 0)
            return false;
    }

    return true;
}

static bool SwitchAuditMaterializeStagedImageToGuest(const SwitchAuditStagedImageView& stagedImage)
{
    if (stagedImage.srcData == nullptr || stagedImage.sourceSize == 0 || stagedImage.imageSize == 0 ||
        stagedImage.imageSize > UINT32_MAX || stagedImage.blocks.empty())
    {
        SwitchAuditLog("Switch module-load preflight audit: staged image materialization failed: invalid staged view.");
        return false;
    }

    if (g_memory.base == nullptr)
    {
        SwitchAuditLog("Switch module-load preflight audit: staged image materialization failed: guest memory is disabled.");
        return false;
    }

    uint64_t imageEnd = 0;
    if (!SwitchAuditGuestRangeEnd(stagedImage.imageBase, static_cast<uint32_t>(stagedImage.imageSize), imageEnd))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: staged image materialization failed: invalid image span base=0x%08X size=0x%llX.",
            stagedImage.imageBase,
            static_cast<unsigned long long>(stagedImage.imageSize));
        return false;
    }

    auto* imageDest = static_cast<uint8_t*>(g_memory.Translate(stagedImage.imageBase));
    SwitchAuditLogf(
        "Switch module-load preflight audit: staged image materialization begin base=0x%08X end=0x%llX size=0x%llX blocks=%llu.",
        stagedImage.imageBase,
        static_cast<unsigned long long>(imageEnd),
        static_cast<unsigned long long>(stagedImage.imageSize),
        static_cast<unsigned long long>(stagedImage.blocks.size()));

    size_t dataBytes = 0;
    size_t zeroBytes = 0;
    for (size_t i = 0; i < stagedImage.blocks.size(); i++)
    {
        const SwitchAuditBasicBlockRange& block = stagedImage.blocks[i];
        if (block.zeroSize > SIZE_MAX - block.dataSize)
        {
            SwitchAuditLogf("Switch module-load preflight audit: staged image materialization failed: block %llu size overflow.", static_cast<unsigned long long>(i));
            return false;
        }

        const size_t blockSize = block.dataSize + block.zeroSize;
        if (!SwitchAuditRangeWithin(stagedImage.imageSize, block.imageOffset, blockSize) ||
            !SwitchAuditRangeWithin(stagedImage.sourceSize, block.sourceOffset, block.dataSize))
        {
            SwitchAuditLogf(
                "Switch module-load preflight audit: staged image materialization failed: block %llu out of range imageOffset=0x%llX sourceOffset=0x%llX data=0x%llX zero=0x%llX.",
                static_cast<unsigned long long>(i),
                static_cast<unsigned long long>(block.imageOffset),
                static_cast<unsigned long long>(block.sourceOffset),
                static_cast<unsigned long long>(block.dataSize),
                static_cast<unsigned long long>(block.zeroSize));
            return false;
        }

        uint8_t* const blockDest = imageDest + block.imageOffset;
        const uint8_t* const blockSource = stagedImage.srcData + block.sourceOffset;
        SwitchAuditLogf(
            "Switch module-load preflight audit: staged image materialization block %llu imageOffset=0x%llX data=0x%llX zero=0x%llX.",
            static_cast<unsigned long long>(i),
            static_cast<unsigned long long>(block.imageOffset),
            static_cast<unsigned long long>(block.dataSize),
            static_cast<unsigned long long>(block.zeroSize));

        if (block.dataSize != 0)
        {
            memcpy(blockDest, blockSource, block.dataSize);
            if (memcmp(blockDest, blockSource, block.dataSize) != 0)
            {
                SwitchAuditLogf(
                    "Switch module-load preflight audit: staged image materialization failed: block %llu data verify mismatch.",
                    static_cast<unsigned long long>(i));
                return false;
            }
            dataBytes += block.dataSize;
        }

        if (block.zeroSize != 0)
        {
            uint8_t* const zeroDest = blockDest + block.dataSize;
            memset(zeroDest, 0, block.zeroSize);
            if (!SwitchAuditVerifyZeroFill(zeroDest, block.zeroSize))
            {
                SwitchAuditLogf(
                    "Switch module-load preflight audit: staged image materialization failed: block %llu zero-fill verify mismatch.",
                    static_cast<unsigned long long>(i));
                return false;
            }
            zeroBytes += block.zeroSize;
        }
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: staged image materialization complete blocks=%llu dataBytes=%llu zeroBytes=%llu imageSize=0x%llX.",
        static_cast<unsigned long long>(stagedImage.blocks.size()),
        static_cast<unsigned long long>(dataBytes),
        static_cast<unsigned long long>(zeroBytes),
        static_cast<unsigned long long>(stagedImage.imageSize));
    return true;
}

static bool SwitchAuditValidateXdbfResource(const uint8_t* resourceData, uint32_t resourceSize)
{
    if (resourceData == nullptr || resourceSize <= sizeof(XDBFHeader))
    {
        SwitchAuditLog("Switch module-load preflight audit: XDBF resource validation failed: resource is too small.");
        return false;
    }

    const auto* header = reinterpret_cast<const XDBFHeader*>(resourceData);
    const uint32_t signature = static_cast<uint32_t>(header->Signature);
    if (signature != XDBF_SIGNATURE)
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: XDBF resource validation failed: signature=0x%08X.",
            signature);
        return false;
    }

    const uint32_t entryCount = static_cast<uint32_t>(header->EntryCount);
    const uint32_t freeSpaceTableLength = static_cast<uint32_t>(header->FreeSpaceTableLength);
    if (entryCount > SIZE_MAX / sizeof(XDBFEntry) ||
        freeSpaceTableLength > SIZE_MAX / sizeof(XDBFFreeSpaceEntry))
    {
        SwitchAuditLog("Switch module-load preflight audit: XDBF resource validation failed: table size overflow.");
        return false;
    }

    const size_t entryBytes = static_cast<size_t>(entryCount) * sizeof(XDBFEntry);
    const size_t freeBytes = static_cast<size_t>(freeSpaceTableLength) * sizeof(XDBFFreeSpaceEntry);
    const size_t headerBytes = sizeof(XDBFHeader);
    if (entryBytes > SIZE_MAX - headerBytes ||
        freeBytes > SIZE_MAX - headerBytes - entryBytes ||
        headerBytes + entryBytes + freeBytes > resourceSize)
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: XDBF resource validation failed: entries=%u free=%u resourceSize=0x%08X.",
            entryCount,
            freeSpaceTableLength,
            resourceSize);
        return false;
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: XDBF resource validation ok entries=%u freeTable=%u resourceSize=0x%08X.",
        entryCount,
        freeSpaceTableLength,
        resourceSize);
    return true;
}

static bool SwitchAuditZeroGuestRange(const char* label, uint32_t start, uint32_t size)
{
    uint64_t endExclusive = 0;
    if (!SwitchAuditGuestRangeEnd(start, size, endExclusive))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: side-effect range %s invalid start=0x%08X size=0x%08X.",
            label,
            start,
            size);
        return false;
    }

    auto* ptr = static_cast<uint8_t*>(g_memory.Translate(start));
    memset(ptr, 0, size);
    if (!SwitchAuditVerifyZeroFill(ptr, size))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: side-effect range %s zero verify failed start=0x%08X size=0x%08X.",
            label,
            start,
            size);
        return false;
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: side-effect range %s zeroed start=0x%08X end=0x%llX size=0x%08X.",
        label,
        start,
        static_cast<unsigned long long>(endExclusive),
        size);
    return true;
}

static bool SwitchAuditApplyLdrSideEffects(uint32_t resourceOffset, uint32_t resourceSize)
{
    if (g_memory.base == nullptr)
    {
        SwitchAuditLog("Switch module-load preflight audit: side-effect audit failed: guest memory is disabled.");
        return false;
    }

    SwitchAuditLog("Switch module-load preflight audit: side-effect audit begin.");

    if (resourceOffset == 0 || resourceSize == 0)
    {
        SwitchAuditLog("Switch module-load preflight audit: side-effect audit failed: XDBF resource metadata is absent.");
        return false;
    }

    uint64_t resourceEnd = 0;
    if (!SwitchAuditGuestRangeEnd(resourceOffset, resourceSize, resourceEnd))
    {
        SwitchAuditLogf(
            "Switch module-load preflight audit: side-effect audit failed: invalid resource range start=0x%08X size=0x%08X.",
            resourceOffset,
            resourceSize);
        return false;
    }

    auto* resourceData = static_cast<uint8_t*>(g_memory.Translate(resourceOffset));
    if (!SwitchAuditValidateXdbfResource(resourceData, resourceSize))
        return false;

    g_xdbfWrapper = XDBFWrapper(resourceData, resourceSize);
    if (g_xdbfWrapper.pBuffer != resourceData)
    {
        SwitchAuditLog("Switch module-load preflight audit: side-effect audit failed: XDBFWrapper rejected resource.");
        return false;
    }

    SwitchAuditLogf(
        "Switch module-load preflight audit: XDBF wrapper initialized resource=0x%08X end=0x%llX size=0x%08X.",
        resourceOffset,
        static_cast<unsigned long long>(resourceEnd),
        resourceSize);

    if (!SwitchAuditZeroGuestRange("collision zero", 0x82003880, 0x80))
        return false;

    auto* streamPtr = reinterpret_cast<be<uint32_t>*>(g_memory.Translate(0x82003890));
    for (size_t i = 0; i < 7; i++)
        streamPtr[i] = 0;

    for (size_t i = 0; i < 7; i++)
    {
        if (static_cast<uint32_t>(streamPtr[i]) != 0)
        {
            SwitchAuditLogf(
                "Switch module-load preflight audit: stream struct verify failed index=%llu.",
                static_cast<unsigned long long>(i));
            return false;
        }
    }

    SwitchAuditLog("Switch module-load preflight audit: stream struct initialized start=0x82003890 size=0x0000001C fields=7.");

    if (!SwitchAuditZeroGuestRange("worker globals", 0x830F5000, 0x3000))
        return false;

    SwitchAuditLog("Switch module-load preflight audit: side-effect audit complete.");
    return true;
}
#endif

void HostStartup()
{
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    hid::Init();
}

// Forward declarations from imports.cpp
void InitKernelMainThread();

// Xenon memory initialization
#include <kernel/xenon_memory.h>

// Name inspired from nt's entry point
void KiSystemStartup()
{
    // Initialize main thread ID for SDL event safety
    InitKernelMainThread();
    
    if (g_memory.base == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("System_MemoryAllocationFailed").c_str(), GameWindow::s_pWindow);
        std::_Exit(1);
    }

    InitializeReXGlueMMIOBridge();

    g_userHeap.Init();
    
    // CRITICAL: Initialize Xbox 360 Xenon memory regions per memory contract
    // This zeros all regions the game assumes are pre-allocated and zeroed on boot.
    // Must happen BEFORE any game code executes to prevent corruption.
    InitializeXenonMemoryRegions(g_memory.base);

#if defined(__SWITCH__) && defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    SwitchAuditLog("KiSystemStartup heap and Xenon fixed memory regions initialized; returning before save/content/audio startup.");
    return;
#endif

    // Initialize save system early - creates directories and registers save content
    SaveSystem::Initialize();

    const auto gameContent = XamMakeContent(XCONTENTTYPE_RESERVED, "Game");
    const std::string gamePath = (const char*)(GetGamePath() / "game").u8string().c_str();

    BuildPathCache(gamePath);

    // Also cache extracted assets living under "RPF DUMP" (nested RPFS, audio packs, etc.).
    // This improves hit rate when the title requests files that are not present in game/.
    const std::string rpfDumpPath = (const char*)(GetGamePath() / "RPF DUMP").u8string().c_str();
    if (std::filesystem::exists(rpfDumpPath))
        BuildPathCache(rpfDumpPath);

    XamRegisterContent(gameContent, gamePath);

    // Register and mount update content (CRITICAL for file override)
    const auto updateContent = XamMakeContent(XCONTENTTYPE_RESERVED, "Update");
    const std::filesystem::path updateRoot = GetGamePath() / "update";
    const std::string updatePath = (const char*)updateRoot.u8string().c_str();

    if (std::filesystem::exists(updateRoot))
    {
        XamRegisterContent(updateContent, updatePath);
        XamContentCreateEx(0, "update", &updateContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);
        
        // Create root mappings for update paths (case variations)
        XamRootCreate("update", updatePath);
        XamRootCreate("Update", updatePath);
        
        LOGF_IMPL(Utility, "Main", "Registered update: -> {}", updatePath);
    }
    else
    {
        LOGF_IMPL(Utility, "Main", "No update directory found (this is normal if no Title Update installed)");
    }

    // Note: Save system initialization already handled by SaveSystem::Initialize() above
    // This section is kept for backwards compatibility with old save file format
    const auto saveFilePath = GetSaveFilePath(true);
    bool saveFileExists = std::filesystem::exists(saveFilePath);

    if (!saveFileExists)
    {
        // Copy base save data to modded save as fallback.
        std::error_code ec;
        std::filesystem::create_directories(saveFilePath.parent_path(), ec);

        if (!ec)
        {
            std::filesystem::copy_file(GetSaveFilePath(false), saveFilePath, ec);
            saveFileExists = !ec;
        }
    }

    if (saveFileExists)
    {
        std::u8string savePathU8 = saveFilePath.parent_path().u8string();
        XamRegisterContent(XamMakeContent(XCONTENTTYPE_SAVEDATA, "GTA4SaveData.bin"), (const char*)(savePathU8.c_str()));
    }

    // Mount game
    XamContentCreateEx(0, "game", &gameContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);

    // OS mounts game data to D:
    XamContentCreateEx(0, "D", &gameContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);

    // GTA IV uses "common:" and "platform:" root paths
    // All game files are in: GetGamePath() / "game" / (common/, xbox360/, audio/)
    // Structure: ~/Library/Application Support/LibertyRecomp/game/common/, etc.
    const auto gameRoot = GetGamePath() / "game";
    const std::string commonPath = (const char*)(gameRoot / "common").u8string().c_str();
    const std::string platformPath = (const char*)(gameRoot / "xbox360").u8string().c_str();
#if defined(__SWITCH__)
    const std::string audioPath = (const char*)SwitchSelectAudioRoot(gameRoot).u8string().c_str();
#else
    LogWindowsContentPreflight(gameRoot);
    const std::string audioPath = (const char*)SelectAudioRoot(gameRoot).u8string().c_str();
#endif
    
    // Register main root paths
    XamRootCreate("common", commonPath);
    XamRootCreate("platform", platformPath);
    XamRootCreate("audio", audioPath);
    
    // Also register alternate names the game might use
    XamRootCreate("xbox360", platformPath);  // Some code uses xbox360: instead of platform:
    
    LOGF_IMPL(Utility, "Main", "Game root: {}", gameRoot.string());
    LOGF_IMPL(Utility, "Main", "Registered common: -> {}", commonPath);
    LOGF_IMPL(Utility, "Main", "Registered platform: -> {}", platformPath);
    LOGF_IMPL(Utility, "Main", "Registered audio: -> {}", audioPath);

    // Initialize Virtual File System for direct file serving
    // This bypasses complex RPF offset-based reading that causes stream issues
    VFS::Initialize(gameRoot);
    LOGF_IMPL(Utility, "Main", "VFS initialized with root: {}", gameRoot.string());

    std::error_code ec;
    for (auto& file : std::filesystem::directory_iterator(GetGamePath() / "dlc", ec))
    {
        if (file.is_directory())
        {
            std::u8string fileNameU8 = file.path().filename().u8string();
            std::u8string filePathU8 = file.path().u8string();
            const char* fileName = (const char*)(fileNameU8.c_str());
            const char* filePath = (const char*)(filePathU8.c_str());
            
            // Register DLC content
            XamRegisterContent(XamMakeContent(XCONTENTTYPE_DLC, fileName), filePath);
            
            // Mount DLC to virtual path
            auto dlcContent = XamMakeContent(XCONTENTTYPE_DLC, fileName);
            XamContentCreateEx(0, fileName, &dlcContent, OPEN_EXISTING, nullptr, nullptr, 0, 0, nullptr);
            
            // Create root mapping for DLC-specific paths
            XamRootCreate(fileName, filePath);
            
            LOGF_IMPL(Utility, "Main", "Registered DLC: {} -> {}", fileName, filePath);
        }
    }

    XAudioInitializeSystem();
}

uint32_t LdrLoadModule(const std::filesystem::path &path)
{
    const auto loadResult = LoadFile(path);
    if (loadResult.empty())
    {
        assert("Failed to load module" && false);
        return 0;
    }

    const auto image = Image::ParseImage(loadResult.data(), loadResult.size());

    memcpy(g_memory.Translate(image.base), image.data.get(), image.size);
    g_xdbfWrapper = XDBFWrapper(static_cast<uint8_t*>(g_memory.Translate(image.resource_offset)), image.resource_size);

    // GTA IV Memory Layout Collision Fix
    // Address 0x82003890 aliases with "common.rpf" string in image .rdata section.
    // The game expects this to be heap-backed stream object storage.
    // Xbox 360 kept these regions separate, but native recompilation does not.
    // Fix: Zero the collision range and fully initialize stream struct.
    {
        uint8_t* collisionBase = static_cast<uint8_t*>(g_memory.Translate(0x82003880));
        memset(collisionBase, 0, 0x80);  // Zero 0x82003880 - 0x82003900
        
        // Fully initialize stream struct at 0x82003890
        // Stream struct layout (28 bytes / 7 dwords):
        //   [0] = Object pointer (vtable holder)
        //   [1] = Context/parameter
        //   [2] = Buffer pointer
        //   [3] = File position
        //   [4] = Buffer cursor
        //   [5] = Buffer end/limit
        //   [6] = Buffer capacity
        be<uint32_t>* streamPtr = reinterpret_cast<be<uint32_t>*>(g_memory.Translate(0x82003890));
        streamPtr[0] = 0;  // Null object - stream ops will return early gracefully
        streamPtr[1] = 0;           // Context (null is safe)
        streamPtr[2] = 0;           // Buffer (null = no buffer)
        streamPtr[3] = 0;           // File position
        streamPtr[4] = 0;           // Buffer cursor
        streamPtr[5] = 0;           // Buffer end (0 = empty)
        streamPtr[6] = 0;           // Capacity (0 = no buffer)
    }
    
    // Worker Thread Global Memory Initialization
    // Address range 0x830F5000-0x830F8000 contains worker-related global structures.
    // Workers read semaphore handles from this region (e.g., 0x830F7684 = base+9860).
    // Without zeroing, workers read stale data from previous runs causing infinite polling.
    // Fix: Zero worker globals so uninitialized handles read as NULL.
    {
        uint8_t* workerGlobals = static_cast<uint8_t*>(g_memory.Translate(0x830F5000));
        if (workerGlobals) {
            memset(workerGlobals, 0, 0x3000);  // Zero 12KB of worker globals
            printf("[LdrLoadModule] Zeroed worker globals 0x830F5000-0x830F8000\n");
        }
    }

    return image.entry_point;
}

#ifdef __x86_64__
__attribute__((constructor(101), target("no-avx,no-avx2"), noinline))
void init()
{
    uint32_t eax, ebx, ecx, edx;

    // Execute CPUID for processor info and feature bits.
    __get_cpuid(1, &eax, &ebx, &ecx, &edx);

    // Check for AVX support.
    if ((ecx & (1 << 28)) == 0)
    {
        printf("[*] CPU does not support the AVX instruction set.\n");

#ifdef _WIN32
        MessageBoxA(nullptr, "Your CPU does not meet the minimum system requirements.", "Liberty Recompiled", MB_ICONERROR);
#endif

        std::_Exit(1);
    }
}
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

#ifdef __SWITCH__
    SwitchAuditDebugRaw("[Switch] main entered.\n");
    bool switchRomfsMounted = false;
    bool switchSdmcMounted = false;

    const bool switchRunningAsNso = envIsNso();
    uint32_t romfsResult = 0;
    if (switchRunningAsNso)
    {
        SwitchAuditDebugRaw("[Switch] envIsNso reported true; skipping romfsMountSelf for SD-only audit package.\n");
    }
    else
    {
        SwitchAuditDebugRaw("[Switch] mounting romfs.\n");
        romfsResult = romfsMountSelf("romfs");
        if (romfsResult != 0)
            fprintf(stderr, "[Switch] romfsInit failed: 0x%08X\n", romfsResult);
        else
            switchRomfsMounted = true;
    }
    {
        char status[128];
        snprintf(status, sizeof(status), "[Switch] romfsMountSelf status 0x%08X (nso=%u).\n", romfsResult, switchRunningAsNso ? 1u : 0u);
        SwitchAuditDebugRaw(status);
    }

    SwitchAuditDebugRaw("[Switch] ensuring sdmc.\n");
    uint32_t sdmcResult = 0;
    const bool switchSdmcUsable = SwitchAuditMountSdmc(sdmcResult, switchSdmcMounted);
    if (!switchSdmcUsable)
    {
        fprintf(stderr, "[Switch] fsdevMountSdmc failed and sdmc is unavailable: 0x%08X\n", sdmcResult);
        char status[128];
        snprintf(status, sizeof(status), "fsdevMountSdmc failed and sdmc is unavailable: 0x%08X", sdmcResult);
        SwitchAuditDebugRaw(status);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        for (;;)
            svcSleepThread(1000000000);
    }

    if (sdmcResult != 0)
    {
        char status[128];
        snprintf(status, sizeof(status), "fsdevMountSdmc returned 0x%08X; existing sdmc device is usable", sdmcResult);
        SwitchAuditDebugRaw(status);
    }

    SwitchAuditLog("Switch audit package startup; continuing to content preflight.");

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONFIG_LOAD)
    SwitchAuditLog("Switch config audit: before Config::Load.");
    Config::Load();
    SwitchAuditLog("Switch config audit: Config::Load returned; stopping before content preflight, host startup, and guest code.");
    LibertySwitchShowAuditDiagnostic(
        "Config audit",
        "Config::Load returned.\n"
        "Content preflight, host startup, module loading, and guest code were skipped.",
        SWITCH_AUDIT_CONTENT_ROOT,
        SWITCH_AUDIT_LOG_PATH);
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
    return 0;
#endif

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_INSTALL_CHECK)
    {
        SwitchAuditLog("Switch install-check audit: before Config::Load.");
        Config::Load();
        SwitchAuditLog("Switch install-check audit: Config::Load returned.");

        const std::filesystem::path gamePath = GetGamePath();
        SwitchAuditLog("Switch install-check audit: game path:", gamePath);

        std::filesystem::path modulePath;
        const bool isGameInstalled = Installer::checkGameInstall(gamePath, modulePath);
        const std::string installStatus = isGameInstalled
            ? "Switch install-check audit: Installer::checkGameInstall found module:"
            : "Switch install-check audit: Installer::checkGameInstall did not find module:";
        SwitchAuditLog(installStatus.c_str(), modulePath);
        SwitchAuditLog("Switch install-check audit: stopping before host startup, module loading, and guest code.");

        const std::string modulePathText = modulePath.string();
        LibertySwitchShowAuditDiagnostic(
            isGameInstalled ? "Content check" : "Missing game content",
            isGameInstalled
                ? "default.xex was found.\nHost startup, module loading, and guest code were skipped."
                : "default.xex was not found.\nHost startup, module loading, and guest code were skipped.",
            modulePathText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_CONTENT_LAYOUT_CHECK)
    {
        SwitchAuditLog("Switch content-layout audit: before Config::Load.");
        Config::Load();
        SwitchAuditLog("Switch content-layout audit: Config::Load returned.");

        const std::filesystem::path contentRoot = GetGamePath();
        const std::filesystem::path gameRoot = contentRoot / "game";
        const std::filesystem::path commonRoot = gameRoot / "common";
        const std::filesystem::path xbox360Root = gameRoot / "xbox360";
        const std::filesystem::path audioRoot = gameRoot / "audio";
        const std::filesystem::path platformAudioRoot = xbox360Root / "audio";
        const std::filesystem::path commonRpf = gameRoot / "common.rpf";
        const std::filesystem::path xbox360Rpf = gameRoot / "xbox360.rpf";
        const std::filesystem::path audioRpf = gameRoot / "audio.rpf";
        const std::filesystem::path legacyRpfDump = contentRoot / "RPF DUMP";
        const std::filesystem::path dlcRoot = contentRoot / "dlc";

        SwitchAuditLog("Switch content-layout audit: content root:", contentRoot);
        SwitchAuditLog("Switch content-layout audit: game root:", gameRoot);

        std::filesystem::path modulePath;
        const bool modulePresent = Installer::checkGameInstall(contentRoot, modulePath);
        SwitchAuditLogPresence("Switch content-layout audit: game/default.xex", modulePath, modulePresent);

        const bool gameRootPresent = SwitchAuditDirectoryExists(gameRoot);
        const bool commonPresent = SwitchAuditDirectoryExists(commonRoot);
        const bool xbox360Present = SwitchAuditDirectoryExists(xbox360Root);
        const bool audioPresent = SwitchAuditDirectoryExists(audioRoot);
        const bool platformAudioPresent = SwitchAuditDirectoryExists(platformAudioRoot);
        const bool commonRpfPresent = SwitchAuditFileExists(commonRpf);
        const bool xbox360RpfPresent = SwitchAuditFileExists(xbox360Rpf);
        const bool audioRpfPresent = SwitchAuditFileExists(audioRpf);
        const bool legacyRpfDumpPresent = SwitchAuditDirectoryExists(legacyRpfDump);
        const bool dlcPresent = SwitchAuditDirectoryExists(dlcRoot);

        SwitchAuditLogPresence("Switch content-layout audit: game directory", gameRoot, gameRootPresent);
        SwitchAuditLogPresence("Switch content-layout audit: extracted common directory", commonRoot, commonPresent);
        SwitchAuditLogPresence("Switch content-layout audit: extracted xbox360 directory", xbox360Root, xbox360Present);
        SwitchAuditLogPresence("Switch content-layout audit: extracted audio directory", audioRoot, audioPresent);
        SwitchAuditLogPresence("Switch content-layout audit: platform audio directory", platformAudioRoot, platformAudioPresent);
        SwitchAuditLogPresence("Switch content-layout audit: source common.rpf", commonRpf, commonRpfPresent);
        SwitchAuditLogPresence("Switch content-layout audit: source xbox360.rpf", xbox360Rpf, xbox360RpfPresent);
        SwitchAuditLogPresence("Switch content-layout audit: source audio.rpf", audioRpf, audioRpfPresent);
        SwitchAuditLogPresence("Switch content-layout audit: legacy RPF DUMP directory", legacyRpfDump, legacyRpfDumpPresent);
        SwitchAuditLogPresence("Switch content-layout audit: optional DLC directory", dlcRoot, dlcPresent);

        const bool extractedAudioReady = audioPresent || platformAudioPresent;
        const bool extractedReady = commonPresent && xbox360Present && extractedAudioReady;
        const bool sourceArchivesPresent = commonRpfPresent && xbox360RpfPresent && audioRpfPresent;
        const char* summary = "content layout incomplete";
        const char* screenStatus =
            "Content layout is incomplete.\n"
            "Host startup, module loading, and guest code were skipped.";

        if (!modulePresent)
        {
            summary = "missing game/default.xex";
            screenStatus =
                "game/default.xex is missing.\n"
                "Host startup, module loading, and guest code were skipped.";
        }
        else if (extractedReady)
        {
            if (audioPresent)
            {
                summary = "required extracted content directories are present";
                screenStatus =
                    "default.xex and extracted content directories are present.\n"
                    "Host startup, module loading, and guest code were skipped.";
            }
            else
            {
                summary = "required extracted content directories are present with platform audio fallback";
                screenStatus =
                    "default.xex, common, xbox360, and xbox360/audio are present.\n"
                    "Host startup, module loading, and guest code were skipped.";
            }
        }
        else if (sourceArchivesPresent)
        {
            summary = "source RPF archives are present but extracted directories are missing";
            screenStatus =
                "RPF archives are present, but extracted content directories are missing.\n"
                "Host startup, module loading, and guest code were skipped.";
        }
        else if (legacyRpfDumpPresent)
        {
            summary = "legacy RPF DUMP directory is present but current game layout is incomplete";
            screenStatus =
                "Legacy RPF DUMP exists, but current game/common, game/xbox360, and game/audio layout is incomplete.\n"
                "Host startup, module loading, and guest code were skipped.";
        }

        SwitchAuditLog("Switch content-layout audit summary:", summary);
        SwitchAuditLog("Switch content-layout audit: stopping before host startup, module loading, and guest code.");

        const std::string gameRootText = gameRoot.string();
        LibertySwitchShowAuditDiagnostic(
            "Content layout audit",
            screenStatus,
            gameRootText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_VFS_PREFLIGHT)
    {
        SwitchAuditLog("Switch VFS preflight audit: before Config::Load.");
        Config::Load();
        SwitchAuditLog("Switch VFS preflight audit: Config::Load returned.");

        const std::filesystem::path contentRoot = GetGamePath();
        const std::filesystem::path gameRoot = contentRoot / "game";
        const std::filesystem::path commonRoot = gameRoot / "common";
        const std::filesystem::path xbox360Root = gameRoot / "xbox360";
        const std::filesystem::path audioRoot = SwitchSelectAudioRoot(gameRoot);

        std::filesystem::path modulePath;
        const bool modulePresent = Installer::checkGameInstall(contentRoot, modulePath);
        SwitchAuditLogPresence("Switch VFS preflight audit: game/default.xex", modulePath, modulePresent);
        SwitchAuditLog("Switch VFS preflight audit: game root:", gameRoot);
        SwitchAuditLog("Switch VFS preflight audit: common root:", commonRoot);
        SwitchAuditLog("Switch VFS preflight audit: platform root:", xbox360Root);
        SwitchAuditLog("Switch VFS preflight audit: audio root:", audioRoot);

        if (!modulePresent)
        {
            SwitchAuditLog("Switch VFS preflight audit: missing module; stopping before VFS initialization.");
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "VFS preflight",
                "game/default.xex is missing.\n"
                "VFS initialization, module loading, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 0;
        }

        const std::string commonPath = (const char*)commonRoot.u8string().c_str();
        const std::string platformPath = (const char*)xbox360Root.u8string().c_str();
        const std::string audioPath = (const char*)audioRoot.u8string().c_str();

        SwitchAuditLog("Switch VFS preflight audit: building path cache.");
        const size_t pathCacheEntries = BuildPathCache(gameRoot.string());
        char pathCacheMessage[128];
        snprintf(
            pathCacheMessage,
            sizeof(pathCacheMessage),
            "Switch VFS preflight audit: BuildPathCache returned entries=%llu",
            static_cast<unsigned long long>(pathCacheEntries));
        SwitchAuditLog(pathCacheMessage);

        SwitchAuditLog("Switch VFS preflight audit: registering XAM roots.");
        XamRootCreate("common", commonPath);
        XamRootCreate("platform", platformPath);
        XamRootCreate("xbox360", platformPath);
        XamRootCreate("audio", audioPath);
        SwitchAuditLog("Switch VFS preflight audit: XAM roots registered; initializing VFS with index scan.");
        VFS::Initialize(gameRoot);
        SwitchAuditLog("Switch VFS preflight audit: VFS::Initialize returned.");

        const VFS::Stats stats = VFS::GetStats();
        char statsMessage[192];
        snprintf(
            statsMessage,
            sizeof(statsMessage),
            "Switch VFS preflight audit: VFS stats files=%llu dirs=%llu bytes=%llu",
            static_cast<unsigned long long>(stats.totalFiles),
            static_cast<unsigned long long>(stats.totalDirectories),
            static_cast<unsigned long long>(stats.totalBytes));
        SwitchAuditLog(statsMessage);

        struct ProbePath
        {
            const char* label;
            const char* guestPath;
        };

        const ProbePath probes[] =
        {
            { "common script image", "common:\\data\\cdimages\\script.img" },
            { "platform vehicles image", "platform:\\models\\cdimages\\vehicles.img" },
            { "audio resident pack", "audio:\\sfx\\resident.rpf" },
        };

        bool allResolved = true;
        for (const ProbePath& probe : probes)
        {
            const std::filesystem::path resolved = FileSystem::ResolvePath(probe.guestPath, false);
            const bool present = !resolved.empty() && SwitchAuditFileExists(resolved);
            allResolved = allResolved && present;

            std::string message = std::string("Switch VFS preflight audit: ") + probe.label;
            message += present ? " resolved:" : " missing:";
            SwitchAuditLog(message.c_str(), resolved);

            const std::filesystem::path vfsResolved = VFS::Resolve(probe.guestPath);
            const bool vfsPresent = !vfsResolved.empty() && SwitchAuditFileExists(vfsResolved);
            std::string vfsMessage = std::string("Switch VFS preflight audit: VFS ") + probe.label;
            vfsMessage += vfsPresent ? " resolved:" : " missing:";
            SwitchAuditLog(vfsMessage.c_str(), vfsResolved);
        }

        SwitchAuditLog(
            allResolved
                ? "Switch VFS preflight audit summary: representative paths resolved; stopping before host startup, module loading, and guest code."
                : "Switch VFS preflight audit summary: one or more representative paths are missing; stopping before host startup, module loading, and guest code.");

        const std::string gameRootText = gameRoot.string();
        LibertySwitchShowAuditDiagnostic(
            "VFS preflight",
            allResolved
                ? "Representative content paths resolved.\nHost startup, module loading, and guest code were skipped."
                : "One or more representative content paths are missing.\nHost startup, module loading, and guest code were skipped.",
            gameRootText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

#if defined(LIBERTY_RECOMP_SWITCH_AUDIT_STOP_AFTER_MODULE_LOAD_PREFLIGHT)
    {
        SwitchAuditLog("Switch module-load preflight audit: before Config::Load.");
        Config::Load();
        SwitchAuditLog("Switch module-load preflight audit: Config::Load returned.");

        const std::filesystem::path contentRoot = GetGamePath();
        const std::filesystem::path gameRoot = contentRoot / "game";
        const std::filesystem::path commonRoot = gameRoot / "common";
        const std::filesystem::path xbox360Root = gameRoot / "xbox360";
        const std::filesystem::path audioRoot = SwitchSelectAudioRoot(gameRoot);

        std::filesystem::path modulePath;
        const bool modulePresent = Installer::checkGameInstall(contentRoot, modulePath);
        SwitchAuditLogPresence("Switch module-load preflight audit: game/default.xex", modulePath, modulePresent);
        SwitchAuditLog("Switch module-load preflight audit: game root:", gameRoot);
        SwitchAuditLog("Switch module-load preflight audit: common root:", commonRoot);
        SwitchAuditLog("Switch module-load preflight audit: platform root:", xbox360Root);
        SwitchAuditLog("Switch module-load preflight audit: audio root:", audioRoot);

        if (!modulePresent)
        {
            SwitchAuditLog("Switch module-load preflight audit: missing module; stopping before VFS and module parsing.");
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module preflight",
                "game/default.xex is missing.\n"
                "VFS initialization, module parsing, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 0;
        }

        const std::string commonPath = (const char*)commonRoot.u8string().c_str();
        const std::string platformPath = (const char*)xbox360Root.u8string().c_str();
        const std::string audioPath = (const char*)audioRoot.u8string().c_str();

        SwitchAuditLog("Switch module-load preflight audit: building path cache.");
        const size_t pathCacheEntries = BuildPathCache(gameRoot.string());
        char pathCacheMessage[128];
        snprintf(
            pathCacheMessage,
            sizeof(pathCacheMessage),
            "Switch module-load preflight audit: BuildPathCache returned entries=%llu",
            static_cast<unsigned long long>(pathCacheEntries));
        SwitchAuditLog(pathCacheMessage);

        SwitchAuditLog("Switch module-load preflight audit: registering XAM roots.");
        XamRootCreate("common", commonPath);
        XamRootCreate("platform", platformPath);
        XamRootCreate("xbox360", platformPath);
        XamRootCreate("audio", audioPath);
        SwitchAuditLog("Switch module-load preflight audit: XAM roots registered; initializing VFS with index scan.");
        VFS::Initialize(gameRoot);
        SwitchAuditLog("Switch module-load preflight audit: VFS::Initialize returned.");

        const VFS::Stats stats = VFS::GetStats();
        char statsMessage[192];
        snprintf(
            statsMessage,
            sizeof(statsMessage),
            "Switch module-load preflight audit: VFS stats files=%llu dirs=%llu bytes=%llu",
            static_cast<unsigned long long>(stats.totalFiles),
            static_cast<unsigned long long>(stats.totalDirectories),
            static_cast<unsigned long long>(stats.totalBytes));
        SwitchAuditLog(statsMessage);

        SwitchAuditLog("Switch module-load preflight audit: reading module:", modulePath);
        auto loadResult = LoadFile(modulePath);
        if (loadResult.empty())
        {
            SwitchAuditLog("Switch module-load preflight audit: LoadFile returned no data; stopping before guest-memory writes:", modulePath);
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module preflight failed",
                "default.xex could not be read.\n"
                "Guest-memory writes and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        char moduleSizeMessage[160];
        snprintf(
            moduleSizeMessage,
            sizeof(moduleSizeMessage),
            "Switch module-load preflight audit: module bytes=%llu",
            static_cast<unsigned long long>(loadResult.size()));
        SwitchAuditLog(moduleSizeMessage);

        const bool xexMagicPresent = loadResult.size() >= 4 &&
            loadResult[0] == 'X' && loadResult[1] == 'E' && loadResult[2] == 'X' && loadResult[3] == '2';
        if (!xexMagicPresent)
        {
            SwitchAuditLog("Switch module-load preflight audit: module is not XEX2; stopping before Image::ParseImage:", modulePath);
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module preflight failed",
                "default.xex did not start with XEX2 magic.\n"
                "Image parsing, guest-memory writes, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        const auto* xexHeader = reinterpret_cast<const Xex2Header*>(loadResult.data());
        const uint32_t headerSize = xexHeader->headerSize;
        const uint32_t securityOffset = xexHeader->securityOffset;
        const uint32_t headerCount = xexHeader->headerCount;
        const uint32_t minimumOptionalHeaderEnd = sizeof(Xex2Header) + headerCount * sizeof(Xex2OptHeader);
        if (headerSize > loadResult.size() ||
            securityOffset + sizeof(Xex2SecurityInfo) > loadResult.size() ||
            minimumOptionalHeaderEnd > headerSize)
        {
            SwitchAuditLog("Switch module-load preflight audit: XEX header bounds are invalid; stopping before Image::ParseImage:", modulePath);
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module preflight failed",
                "default.xex header bounds were invalid.\n"
                "Image parsing, guest-memory writes, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        const auto* securityInfo = reinterpret_cast<const Xex2SecurityInfo*>(loadResult.data() + securityOffset);
        const auto* fileFormatInfo = reinterpret_cast<const Xex2OptFileFormatInfo*>(getOptHeaderPtr(loadResult.data(), XEX_HEADER_FILE_FORMAT_INFO));
        const auto* imageBasePtr = reinterpret_cast<const be<uint32_t>*>(getOptHeaderPtr(loadResult.data(), XEX_HEADER_IMAGE_BASE_ADDRESS));
        const auto* entryPointPtr = reinterpret_cast<const be<uint32_t>*>(getOptHeaderPtr(loadResult.data(), XEX_HEADER_ENTRY_POINT));
        const auto* resourceInfo = reinterpret_cast<const Xex2ResourceInfo*>(getOptHeaderPtr(loadResult.data(), XEX_HEADER_RESOURCE_INFO));
        const auto* importsInfo = reinterpret_cast<const Xex2ImportHeader*>(getOptHeaderPtr(loadResult.data(), XEX_HEADER_IMPORT_LIBRARIES));

        const uint32_t imageBase = imageBasePtr != nullptr ? static_cast<uint32_t>(*imageBasePtr) : static_cast<uint32_t>(securityInfo->loadAddress);
        const uint32_t entryPoint = entryPointPtr != nullptr ? static_cast<uint32_t>(*entryPointPtr) : 0;
        const uint32_t resourceOffset = resourceInfo != nullptr ? static_cast<uint32_t>(resourceInfo->offset) : 0;
        const uint32_t resourceSize = resourceInfo != nullptr ? static_cast<uint32_t>(resourceInfo->sizeOfData) : 0;
        const int32_t encryptionType = fileFormatInfo != nullptr ? static_cast<int32_t>(static_cast<uint16_t>(fileFormatInfo->encryptionType)) : -1;
        const int32_t compressionType = fileFormatInfo != nullptr ? static_cast<int32_t>(static_cast<uint16_t>(fileFormatInfo->compressionType)) : -1;
        const uint32_t importCount = importsInfo != nullptr ? static_cast<uint32_t>(importsInfo->numImports) : 0;

        char imageMessage[384];
        snprintf(
            imageMessage,
            sizeof(imageMessage),
            "Switch module-load preflight audit: xex moduleFlags=0x%08X headerSize=0x%X security=0x%X optHeaders=%u imageSize=0x%X load=0x%08X imageBase=0x%08X entry=0x%08X resource=0x%08X+0x%08X fileFormat enc=%d comp=%d imports=%u pages=%u",
            static_cast<uint32_t>(xexHeader->moduleFlags),
            headerSize,
            securityOffset,
            headerCount,
            static_cast<uint32_t>(securityInfo->imageSize),
            static_cast<uint32_t>(securityInfo->loadAddress),
            imageBase,
            entryPoint,
            resourceOffset,
            resourceSize,
            encryptionType,
            compressionType,
            importCount,
            static_cast<uint32_t>(securityInfo->pageDescriptorCount));
        SwitchAuditLog(imageMessage);

        SwitchAuditStagedImageView stagedImage{};
        const bool parseProbeOk = SwitchAuditProbeXexFullParsePhases(loadResult.data(), loadResult.size(), &stagedImage);
        if (!parseProbeOk)
        {
            SwitchAuditLog("Switch module-load preflight audit summary: XEX full-parse phase probe failed; stopping before Image::ParseImage, LdrLoadModule guest-memory writes, and GuestThread::Start.");
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module preflight failed",
                "default.xex full-parse phase probe failed.\n"
                "Image::ParseImage, guest-memory writes, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        const bool guestRangeProbeOk = SwitchAuditProbeLdrGuestRanges(
            imageBase,
            static_cast<uint32_t>(securityInfo->imageSize),
            resourceOffset,
            resourceSize);
        if (!guestRangeProbeOk)
        {
            SwitchAuditLog("Switch module-load preflight audit summary: guest-memory range probe failed; stopping before Image::ParseImage, LdrLoadModule guest-memory writes, and GuestThread::Start.");
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module memory preflight failed",
                "default.xex guest-memory range probe failed.\n"
                "Image::ParseImage, real module writes, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        const bool materializeOk = SwitchAuditMaterializeStagedImageToGuest(stagedImage);
        if (!materializeOk)
        {
            SwitchAuditLog("Switch module-load preflight audit summary: staged image materialization failed; stopping before XDBF setup, LdrLoadModule side effects, and GuestThread::Start.");
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module materialization failed",
                "default.xex staged image materialization failed.\n"
                "XDBF setup, real loader side effects, and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        const bool sideEffectsOk = SwitchAuditApplyLdrSideEffects(resourceOffset, resourceSize);
        if (!sideEffectsOk)
        {
            SwitchAuditLog("Switch module-load preflight audit summary: loader side-effect audit failed; stopping before GuestThread::Start.");
            const std::string modulePathText = modulePath.string();
            LibertySwitchShowAuditDiagnostic(
                "Module side effects failed",
                "default.xex loader side-effect audit failed.\n"
                "GuestThread::Start and guest code were skipped.",
                modulePathText.c_str(),
                SWITCH_AUDIT_LOG_PATH);
            SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
            return 1;
        }

        SwitchAuditLog("Switch module-load preflight audit summary: staged XEX image materialization and loader side-effect audit completed; stopping before GuestThread::Start.");

        const std::string modulePathText = modulePath.string();
        LibertySwitchShowAuditDiagnostic(
            "Module preflight",
            "default.xex materialization and side-effect audit completed.\n"
            "GuestThread::Start and guest code were skipped.",
            modulePathText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

#if defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP)
    if (g_memory.base != nullptr)
    {
        SwitchAuditLog("Switch guest memory audit mapped successfully; stopping before content preflight and host startup.");
        LibertySwitchShowAuditDiagnostic(
            "Guest memory audit",
            "Guest memory mapped successfully.\n"
            "Content preflight, host startup, and guest code were skipped.",
            SWITCH_AUDIT_CONTENT_ROOT,
            SWITCH_AUDIT_LOG_PATH);
    }
    else
    {
        SwitchAuditLog("Switch guest memory audit failed; Memory::base is null; stopping before content preflight and host startup.");
        LibertySwitchShowAuditDiagnostic(
            "Guest memory audit failed",
            "Memory::base is null after startup.\n"
            "Content preflight, host startup, and guest code were skipped.",
            SWITCH_AUDIT_CONTENT_ROOT,
            SWITCH_AUDIT_LOG_PATH);
    }
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
    return 0;
#endif

#if defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    if (g_memory.base == nullptr)
    {
        SwitchAuditLog("Switch startup-memory audit failed; Memory::base is null; stopping before host startup.");
        LibertySwitchShowAuditDiagnostic(
            "Startup memory audit failed",
            "Memory::base is null after startup.\n"
            "Host startup and guest code were skipped.",
            SWITCH_AUDIT_CONTENT_ROOT,
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }

    SwitchAuditLog("Switch startup-memory audit mapped guest memory; running KiSystemStartup heap/Xenon initialization only.");
    KiSystemStartup();
    SwitchAuditLog("KiSystemStartup heap/Xenon memory initialization completed; stopping before host config, content, module load, and guest code.");
    LibertySwitchShowAuditDiagnostic(
        "Startup memory audit",
        "KiSystemStartup heap/Xenon memory initialization completed.\n"
        "Host config, content checks, module loading, and guest code were skipped.",
        SWITCH_AUDIT_CONTENT_ROOT,
        SWITCH_AUDIT_LOG_PATH);
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
    return 0;
#else
    if (!SwitchAuditFileExists(SWITCH_AUDIT_MODULE_PATH))
    {
        SwitchAuditLog("Early preflight missing game executable:", SWITCH_AUDIT_MODULE_PATH);
        SwitchAuditLog("Expected SD layout root:", SWITCH_AUDIT_CONTENT_ROOT);
        SwitchAuditLog("Expected game content root:", SWITCH_AUDIT_GAME_ROOT);
        LibertySwitchShowAuditDiagnostic(
            "Missing game content",
            "Expected SD root: sdmc:/switch/LibertyRecomp\n"
            "Expected module: game/default.xex\n"
            "Host startup and guest code were skipped.",
            SWITCH_AUDIT_MODULE_PATH,
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

    if (g_memory.base == nullptr)
    {
        SwitchAuditLog("default.xex was found, but Switch guest memory is disabled; stopping before host startup:", SWITCH_AUDIT_MODULE_PATH);
        LibertySwitchShowAuditDiagnostic(
            "Guest memory disabled",
            "default.xex is present, but this audit build has no guest memory backend.\n"
            "Host startup and guest code were skipped.",
            SWITCH_AUDIT_MODULE_PATH,
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif

    os::process::CheckConsole();

    if (!os::registry::Init())
        LOGN_WARNING("OS does not support registry.");

    os::logger::Init();

    PreloadContext preloadContext;
    preloadContext.PreloadExecutable();

    bool forceInstaller = false;
    bool forceDLCInstaller = false;
    bool useDefaultWorkingDirectory = false;
    bool forceInstallationCheck = false;
    bool graphicsApiRetry = false;
    const char *sdlVideoDriver = nullptr;

    for (uint32_t i = 1; i < argc; i++)
    {
        forceInstaller = forceInstaller || (strcmp(argv[i], "--install") == 0);
        forceDLCInstaller = forceDLCInstaller || (strcmp(argv[i], "--install-dlc") == 0);
        useDefaultWorkingDirectory = useDefaultWorkingDirectory || (strcmp(argv[i], "--use-cwd") == 0);
        forceInstallationCheck = forceInstallationCheck || (strcmp(argv[i], "--install-check") == 0);
        graphicsApiRetry = graphicsApiRetry || (strcmp(argv[i], "--graphics-api-retry") == 0);
        App::s_isSkipLogos = App::s_isSkipLogos || (strcmp(argv[i], "--skip-logos") == 0);

        if (strcmp(argv[i], "--sdl-video-driver") == 0)
        {
            if ((i + 1) < argc)
                sdlVideoDriver = argv[++i];
            else
                LOGN_WARNING("No argument was specified for --sdl-video-driver. Option will be ignored.");
        }
    }

    if (!useDefaultWorkingDirectory)
    {
        // Set the current working directory to the executable's path.
        std::error_code ec;
        std::filesystem::current_path(os::process::GetExecutablePath().parent_path(), ec);
    }

    Config::Load();

    if (forceInstallationCheck)
    {
        // Create the console to show progress to the user, otherwise it will seem as if the game didn't boot at all.
        os::process::ShowConsole();

        Journal journal;
        double lastProgressMiB = 0.0;
        double lastTotalMib = 0.0;
        Installer::checkInstallIntegrity(GAME_INSTALL_DIRECTORY, journal, [&]()
        {
            constexpr double MiBDivisor = 1024.0 * 1024.0;
            constexpr double MiBProgressThreshold = 128.0;
            double progressMiB = double(journal.progressCounter) / MiBDivisor;
            double totalMiB = double(journal.progressTotal) / MiBDivisor;
            if (journal.progressCounter > 0)
            {
                if ((progressMiB - lastProgressMiB) > MiBProgressThreshold)
                {
                    fprintf(stdout, "Checking files: %0.2f MiB / %0.2f MiB\n", progressMiB, totalMiB);
                    lastProgressMiB = progressMiB;
                }
            }
            else
            {
                if ((totalMiB - lastTotalMib) > MiBProgressThreshold)
                {
                    fprintf(stdout, "Scanning files: %0.2f MiB\n", totalMiB);
                    lastTotalMib = totalMiB;
                }
            }

            return true;
        });

        char resultText[512];
        uint32_t messageBoxStyle;
        if (journal.lastResult == Journal::Result::Success)
        {
            snprintf(resultText, sizeof(resultText), "%s", Localise("IntegrityCheck_Success").c_str());
            fprintf(stdout, "%s\n", resultText);
            messageBoxStyle = SDL_MESSAGEBOX_INFORMATION;
        }
        else
        {
            snprintf(resultText, sizeof(resultText), Localise("IntegrityCheck_Failed").c_str(), journal.lastErrorMessage.c_str());
            fprintf(stderr, "%s\n", resultText);
            messageBoxStyle = SDL_MESSAGEBOX_ERROR;
        }

        SDL_ShowSimpleMessageBox(messageBoxStyle, GameWindow::GetTitle(), resultText, GameWindow::s_pWindow);
        std::_Exit(int(journal.lastResult));
    }

#if defined(_WIN32) && defined(LIBERTY_RECOMP_D3D12)
    for (auto& dll : g_D3D12RequiredModules)
    {
        if (!std::filesystem::exists(g_executableRoot / dll))
        {
            char text[512];
            snprintf(text, sizeof(text), Localise("System_Win32_MissingDLLs").c_str(), dll.data());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), text, GameWindow::s_pWindow);
            std::_Exit(1);
        }
    }
#endif

    // Check the time since the last time an update was checked. Store the new time if the difference is more than six hours.
    constexpr double TimeBetweenUpdateChecksInSeconds = 6 * 60 * 60;
    time_t timeNow = std::time(nullptr);
    double timeDifferenceSeconds = difftime(timeNow, Config::LastChecked);
    if (timeDifferenceSeconds > TimeBetweenUpdateChecksInSeconds)
    {
        UpdateChecker::initialize();
        UpdateChecker::start();
        Config::LastChecked = timeNow;
        Config::Save();
    }

    if (Config::ShowConsole)
        os::process::ShowConsole();
    LOGN_WARNING("Host Startup");
    HostStartup();

    std::filesystem::path modulePath;
    bool isGameInstalled = Installer::checkGameInstall(GetGamePath(), modulePath);
    bool runInstallerWizard = forceInstaller || forceDLCInstaller || !isGameInstalled;

#ifdef __SWITCH__
#if !defined(LIBERTY_RECOMP_SWITCH_GUEST_MEMORY_AUDIT_STOP_AFTER_XENON_INIT)
    if (!isGameInstalled)
    {
        SwitchAuditLog("Game install is missing; expected module:", modulePath);
        SwitchAuditLog("Expected SD layout root: sdmc:/switch/LibertyRecomp");
        SwitchAuditLog("Expected game content root: sdmc:/switch/LibertyRecomp/game");
        SwitchAuditLog("This NRO is an audit container only and will exit before guest startup.");
        const std::string modulePathText = modulePath.string();
        LibertySwitchShowAuditDiagnostic(
            "Missing game content",
            "default.xex was not found. Guest startup was skipped.",
            modulePathText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 0;
    }
#endif
#endif

    // TEMPORARY: Force installer UI to always show for preview
    // TODO: Remove this line after UI preview is done
    // runInstallerWizard = true;  // DISABLED - respect actual install check
    
     if (runInstallerWizard)
     {
         if (!Video::CreateHostDevice(sdlVideoDriver, graphicsApiRetry))
         {
             SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("Video_BackendError").c_str(), GameWindow::s_pWindow);
             std::_Exit(1);
         }

         if (!InstallerWizard::Run(GetGamePath(), isGameInstalled && forceDLCInstaller))
         {
             std::_Exit(0);
         }
     }

    // ModLoader::Init();

    printf("[Main] Calling KiSystemStartup...\n"); fflush(stdout);
    KiSystemStartup();
    printf("[Main] KiSystemStartup done\n"); fflush(stdout);

    printf("[Main] Loading module: %s\n", modulePath.string().c_str()); fflush(stdout);
    uint32_t entry = LdrLoadModule(modulePath);
    printf("[Main] Module loaded, entry=0x%08X\n", entry); fflush(stdout);

#ifdef __SWITCH__
    if (entry == 0)
    {
        SwitchAuditLog("Failed to load guest module; stopping before GuestThread::Start:", modulePath);
        const std::string modulePathText = modulePath.string();
        LibertySwitchShowAuditDiagnostic(
            "Guest module load failed",
            "LdrLoadModule returned entry=0. Guest startup was skipped.",
            modulePathText.c_str(),
            SWITCH_AUDIT_LOG_PATH);
        SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
        return 1;
    }
#endif

    if (!runInstallerWizard)
    {
        printf("[Main] Creating video device...\n"); fflush(stdout);
        if (!Video::CreateHostDevice(sdlVideoDriver, graphicsApiRetry))
        {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("Video_BackendError").c_str(), GameWindow::s_pWindow);
            std::_Exit(1);
        }
        printf("[Main] Video device created\n"); fflush(stdout);
    }
    LOGN_WARNING("Start Guest Thread");
    LOGN_WARNING(modulePath.string());
    // Video::StartPipelinePrecompilation();

    GuestThread::Start({ entry, 0, 0, 0 });

#ifdef __SWITCH__
    SwitchAuditUnmount(switchRomfsMounted, switchSdmcMounted);
#endif

    return 0;
}

GUEST_FUNCTION_STUB(__imp__vsprintf);
GUEST_FUNCTION_STUB(__imp___vsnprintf);
GUEST_FUNCTION_STUB(__imp__sprintf);
GUEST_FUNCTION_STUB(__imp___snprintf);
GUEST_FUNCTION_STUB(__imp___snwprintf);
GUEST_FUNCTION_STUB(__imp__vswprintf);
GUEST_FUNCTION_STUB(__imp___vscwprintf);
GUEST_FUNCTION_STUB(__imp__swprintf);
