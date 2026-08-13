// kernel.cpp

#include <stdint.h>

// boot.cpp から受け取る構造体（ boot.cpp 側と定義を合わせる ）
struct FrameBufferConfig {
    uint8_t* frame_buffer;
    uint32_t pixels_per_scan_line;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixel_format;
};

// boot.cpp から呼ばれるエントリーポイント
extern "C" void kernel_main(const FrameBufferConfig* config) {
    // -------------------------------------------------------------
    // ここから先は完全に自由な C++ の世界！
    // config->frame_buffer にアクセスすれば直接画面のピクセルを塗れる
    // -------------------------------------------------------------

    for (uint32_t y = 0; y < 980; ++y) {
        for (uint32_t x = 0; x < 1920; ++x) {
            uint32_t index = (y * config->pixels_per_scan_line + x) * 4;
            config->frame_buffer[index + 0] = 82; // Blue
            config->frame_buffer[index + 1] = 74; // Green
            config->frame_buffer[index + 2] = 140; // Red
        }
    }
    for (uint32_t y = 980; y < 1080; ++y) {
        for (uint32_t x = 0; x < 1920; ++x) {
            uint32_t index = (y * config->pixels_per_scan_line + x) * 4;
            config->frame_buffer[index + 0] = 0; // Blue
            config->frame_buffer[index + 1] = 0; // Green
            config->frame_buffer[index + 2] = 0; // Red
        }
    }
  

    while (1) {
        asm volatile ("hlt");
    }
}
