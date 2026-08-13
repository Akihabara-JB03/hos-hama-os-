// boot.cpp - UEFI Bootloader (Once compiled, never touch again!)

#include <stdint.h>

// --- UEFI 基本定義 ---
typedef void* EFI_HANDLE;
typedef uint64_t EFI_STATUS;
#define EFI_SUCCESS 0
#define EFIAPI __attribute__((ms_abi))

typedef struct {
    uint32_t Data1;
    uint16_t Data2;
    uint16_t Data3;
    uint8_t  Data4[8];
} EFI_GUID;

// --- カーネルに渡す画面情報構造体 ---
struct FrameBufferConfig {
    uint8_t* frame_buffer;
    uint32_t pixels_per_scan_line;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format; // 0: RGB, 1: BGR
};

// --- GOP (Graphics Output Protocol) 関連定義 ---
typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBlinkOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    uint32_t Version;
    uint32_t HorizontalResolution;
    uint32_t VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT PixelFormat;
    uint32_t PixelInformation[4];
    uint32_t PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    uint32_t MaxMode;
    uint32_t Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info;
    uint64_t SizeOfInfo;
    uint64_t FrameBufferBase;
    uint64_t FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    void* QueryMode;
    void* SetMode;
    void* Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* Mode;
};

// --- UEFI Boot Services 定義 ---
typedef struct {
    char Header[24];
    void* RaiseTPL;
    void* RestoreTPL;
    void* AllocatePages;
    void* FreePages;
    EFI_STATUS (EFIAPI *GetMemoryMap)(uint64_t* MemoryMapSize, void* MemoryMap, uint64_t* MapKey, uint64_t* DescriptorSize, uint32_t* DescriptorVersion);
    void* AllocatePool;
    void* FreePool;
    void* CreateEvent;
    void* SetTimer;
    void* WaitForEvent;
    void* SignalEvent;
    void* CloseEvent;
    void* CheckEvent;
    void* InstallProtocolInterface;
    void* ReinstallProtocolInterface;
    void* UninstallProtocolInterface;
    void* HandleProtocol;
    void* Reserved;
    void* RegisterProtocolNotify;
    EFI_STATUS (EFIAPI *LocateProtocol)(EFI_GUID* Protocol, void* Registration, void** Interface);
    // 省略 (必要最低限)
    void* Padding[9];
    EFI_STATUS (EFIAPI *ExitBootServices)(EFI_HANDLE ImageHandle, uint64_t MapKey);
} EFI_BOOT_SERVICES;

typedef struct {
    char Header[24];
    void* FirmwareVendor;
    uint32_t FirmwareRevision;
    void* ConsoleInHandle;
    void* ConIn;
    void* ConsoleOutHandle;
    void* ConOut;
    void* StandardErrorHandle;
    void* StdErr;
    void* RuntimeServices;
    EFI_BOOT_SERVICES* BootServices;
} EFI_SYSTEM_TABLE;

// GOP の GUID (9042a9de-23dc-4a38-96fb-7afd01a9d822)
static EFI_GUID gEfiGraphicsOutputProtocolGuid = {
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xfd, 0x01, 0xa9, 0xd8, 0x22}
};

// kernel.cpp 側のメイン関数を宣言
extern "C" void kernel_main(const FrameBufferConfig* config);

// エントリーポイント
extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* sys_table) {
    // 1. GOP (画面出力プロトコル) を取得
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = nullptr;
    EFI_STATUS status = sys_table->BootServices->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, nullptr, (void**)&gop);

    if (status != EFI_SUCCESS || !gop) {
        while (1) asm volatile ("hlt"); // GOP取得失敗時は停止
    }

    // 2. 画面情報 (FrameBuffer) の詰め込み
    FrameBufferConfig config;
    config.frame_buffer = (uint8_t*)gop->Mode->FrameBufferBase;
    config.pixels_per_scan_line = gop->Mode->Info->PixelsPerScanLine;
    config.horizontal_resolution = gop->Mode->Info->HorizontalResolution;
    config.vertical_resolution = gop->Mode->Info->VerticalResolution;
    config.pixel_format = (uint32_t)gop->Mode->Info->PixelFormat;

    // 3. UEFI を終了してOS完全自律化へ (ExitBootServices)
    char memory_map[4096 * 4];
    uint64_t map_size = sizeof(memory_map);
    uint64_t map_key = 0;
    uint64_t descriptor_size = 0;
    uint32_t descriptor_version = 0;

    // GetMemoryMap で最新の map_key を取得して即座に ExitBootServices を呼ぶ
    sys_table->BootServices->GetMemoryMap(&map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
    status = sys_table->BootServices->ExitBootServices(image_handle, map_key);

    if (status != EFI_SUCCESS) {
        // 失敗した場合はもう一度 GetMemoryMap してリトライする（UEFIの定石）
        map_size = sizeof(memory_map);
        sys_table->BootServices->GetMemoryMap(&map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
        sys_table->BootServices->ExitBootServices(image_handle, map_key);
    }

    // 4. kernel.cpp のメイン関数へジャンプ！ (もう二度とここには戻らない)
    kernel_main(&config);

    while (1) {
        asm volatile ("hlt");
    }

    return 0;
}
