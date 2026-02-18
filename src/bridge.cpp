#ifdef __EMSCRIPTEN__

#include <emscripten.h>
#include <string>

#include "bridge.hpp"

// Stub that Asyncify will instrument — the real logic lives in JS.
// Without this, the linker has nothing to link against.
EM_ASYNC_JS(void, js_wait_for_input, (), {
    await new Promise((resolve) => {
        Module._pendingResume = resolve;
    });
});

// JS-side hook — declared here, defined in JS via addFunction or just a 
// global that Asyncify knows to treat as a suspension point.
extern "C" {
    extern void js_wait_for_input();    // listed in ASYNCIFY_IMPORTS
}

// Shared slot: JS writes here before resuming.
static std::string g_pending_input;

// Called by JS to deliver data and wake the WASM back up.
// Export this so JS can call Module._bridge_deliver(ptr, len).
extern "C" EMSCRIPTEN_KEEPALIVE
void bridge_deliver(const char* data, int len) {
    g_pending_input.assign(data, len);
}

namespace Bridge {
    std::string waitForInput() {
        g_pending_input.clear();
        js_wait_for_input();   // suspends here via Asyncify
        return g_pending_input;
    }
}
#endif  
