# Learn2Slither

See the app deployed as a static site to GH pages [znichola.github.io/Learn2Slither](https://znichola.github.io/Learn2Slither/).

> Reinforcement learning

A snake that moves around a board, it must collect green squares to grow, red squares shrink it. Bonus points for growing past 10 squares.

- https://en.wikipedia.org/wiki/Q-learning

## Project Structure

The reinforment learning algo is implemented in C++, which is then compiled to wasm, the frontend is written in Elm. `make` will launch the build for both and start a server to host the result. During the build, the Elm binary is downloaded to a local folder, and the emscripten docker contianer is used for the c++ to wasm. It's also possible to just compile with clang and obtain a cli tool `make snake`. Communicaiton between the wasm binary and the Elm app is done through the stdout/in of the program, there is js connection glue in `index.html` and `support.js`. This connects the stdout, stderr and a shim is used for stdin, and Elm with ports are use on the other end. This allows message to be sent between the two worlds.
