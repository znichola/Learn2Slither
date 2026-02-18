#pragma once

#ifdef __EMSCRIPTEN__

#include <string>

namespace Bridge {
    // Suspends WASM and waits for JS to call resume() with data.
    // Returns the string that JS provided.
    std::string waitForInput();
}



#endif