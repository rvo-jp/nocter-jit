


// 実行可能メモリ
typedef void (*func_t)();

func_t compile(const char* path) {
    // compile
    bytecode code = {0};
    variables vars = {0};

    uint8_t prologue[] = {
        0x55,                       // push rbp
        0x48, 0x89, 0xE5,           // mov rbp, rsp
        0x48, 0x81, 0xEC, 0,0,0,0   // sub rsp, imm32(frame_size)
    };
    append(&code, prologue, sizeof(prologue));

    char abs_path[1024];
    if (GetFullPathNameA(path, 1024, abs_path, NULL) == 0) {
        puts("error: file not found");
        exit(-1);
    }

    script src = loadscript(abs_path);
    skip(&src);
    while (*src.p != '\0') parse_statement_single(&src, &code, &vars);
    free(src.head);

    uint8_t epilogue[] = {
        0x48, 0x89, 0xEC,   // mov rsp, rbp
        0x5D,               // pop rbp
        0xC3                // ret
    };
    append(&code, epilogue, sizeof(epilogue));

    // 変数の最大数を表示
    printf("@vars.max = %zu\n", vars.max);

    // フレームサイズ
    int frame_size = 32 + vars.max * 8;
    frame_size = (frame_size + 15) & ~15;  // 16バイト境界に丸める
    *(int32_t*)(code.main.mem + 7) = frame_size; // 埋め込み
    free(vars.mem);

    // db埋め込み
    uint8_t jmp[] = {
        0xE9, 0,0,0,0,     // jmp +N
    };
    *(int32_t*)(jmp + 1) = code.db.size;
    dbappend(&code, jmp, sizeof(jmp));



    // 実行可能メモリを作成
    uint8_t* execode = (uint8_t*)VirtualAlloc(
        NULL, code.main.size + code.db.size,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!execode) return NULL;

    memcpy(execode, code.db.mem, code.db.size);
    memcpy(execode + code.db.size, code.main.mem, code.main.size);
    delete_code(code);

    for (int i = 0; i < code.main.size + code.db.size; i ++) printf("%x ", execode[i]);
    puts("");

    return (func_t)execode;
}

int main() {
    puts("@version 1.0.0");

    func_t exe = compile("./test.nct");
    puts("@compile done\n");

    exe();
    return 0;
}
