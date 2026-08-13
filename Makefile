# ==========================================
# 自作OS用 Makefile (UEFI + C++)
# ==========================================

# コンパイルターゲット名
TARGET = BOOTX64.EFI

# ツールチェーン
CXX      = clang++
LD       = lld-link

# ソースファイル
SRCS     = boot.cpp kernel.cpp
OBJS     = $(SRCS:.cpp=.o)

# コンパイルオプション (ベアメタル / UEFI用)
CXXFLAGS = -target x86_64-unknown-windows \
           -ffreestanding \
           -fno-builtin \
           -fno-exceptions \
           -fno-rtti \
           -std=c++17 \
           -Wall \
           -Wextra \
           -O2

# リンクオプション (UEFI Application形式)
LDFLAGS  = -subsystem:efi_application \
           -entry:efi_main \
           -nodefaultlib

# QEMU / OVMF 設定 (QEMUでテストする場合)
QEMU     = qemu-system-x86_64
OVMF     = /usr/share/ovmf/OVMF.fd   # 環境に合わせてパスを変更してください
DISK_DIR = disk_img

.PHONY: all clean run image

# デフォルトターゲット ( .efi のビルド )
all: $(TARGET)

# .cpp から .o へのコンパイル
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# .o をリンクして .efi を生成
$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) -out:$@ $(OBJS)

# QEMU用仮想ディスクディレクトリの作成
image: $(TARGET)
	mkdir -p $(DISK_DIR)/EFI/BOOT
	cp $(TARGET) $(DISK_DIR)/EFI/BOOT/BOOTX64.EFI

# QEMUで実行してテスト起動
run: image
	$(QEMU) -bios $(OVMF) -drive format=raw,file=fat:rw:$(DISK_DIR)

# クリーンアップ
clean:
	rm -f $(OBJS) $(TARGET)
	rm -rf $(DISK_DIR)
