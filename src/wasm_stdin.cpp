#if defined(EMSCRIPTEN)
#include <emscripten/emscripten.h>
#include <stdio.h>
#include <unistd.h>

EM_ASYNC_JS(void, wait_for_stdin, (), {
    await window._waitForStdin();
});

extern "C" {
    // Try wrapping both — whichever is actually called will log
    char* __real_fgets(char* str, int num, FILE* stream);
    char* __wrap_fgets(char* str, int num, FILE* stream) {
        EM_ASM({ console.log('[WRAP] fgets called'); });
        if (stream == stdin) wait_for_stdin();
        return __real_fgets(str, num, stream);
    }

    ssize_t __real_read(int fd, void* buf, size_t count);
    ssize_t __wrap_read(int fd, void* buf, size_t count) {
        EM_ASM({ console.log('[WRAP] read called, fd:', $0); }, fd);
        if (fd == 0) wait_for_stdin();
        return __real_read(fd, buf, count);
    }
}
#endif