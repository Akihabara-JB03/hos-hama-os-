// UEFIのややこしい画面出力をC++のクラスで綺麗にする
class UefiConsole {
private:
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* con_out;

public:
    UefiConsole(EFI_SYSTEM_TABLE* sys_table) {
        con_out = sys_table->ConOut;
    }

    // C++っぽいシンプルな描画メソッド
    void print(const wchar_t* msg) {
        con_out->OutputString(con_out, (const uint16_t*)msg);
    }
};

// カーネル本体（エントリーポイント）
extern "C" EFI_STATUS EFIAPI efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE* sys_table) {
    // クラスインスタンスを作って呼び出すだけ！
    UefiConsole console(sys_table);
    console.print(L"Hello, C++ World!\r\n");

    while (1) {
        asm volatile ("hlt");
    }

    return 0;
}
