# Learn2Slither

> Reinforcement learning

A snake that moves around a board, it must collect green squares to gorw, red squares shrink it. Bonus points for growing past 10 squares.

- https://en.wikipedia.org/wiki/Q-learning

## Project Structure

The reinforment learning algo is implemented in C++, which is then compiled to wasm, the frontend is written in Elm. `make` will launch the build for both, the eml binary is downloaded to a local folder, and the emscripten dockerfile is used for the c++ to wasm. It's also possible to just compile with clang and obtain a cli tool. Communicaiton between the wasm binary and js app is done trough the stdout of the program, there is a thin js connection make int he `index.html` which passes the stdout to the elm app using ports. 
