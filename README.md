# Learn2Slither

See the app deployed as a static site to GH pages [znichola.github.io/Learn2Slither](https://znichola.github.io/Learn2Slither/).

> Reinforcement learning

A snake that moves around a board, it must collect green squares to grow, red squares shrink it. Bonus points for growing past 10 squares.

- https://en.wikipedia.org/wiki/Q-learning

## Project Structure

The reinforment learning algo is implemented in C++, which is then compiled to wasm, the frontend is written in Elm. `make` will launch the build for both and start a server to host the result. During the build, the Elm binary is downloaded to a local folder, and the emscripten docker contianer is used for the c++ to wasm. It's also possible to just compile with clang and obtain a cli tool `make snake`. Communicaiton between the wasm binary and the Elm app is done through the stdout/in of the program, there is js connection glue in `index.html` and `support.js`. This connects the stdout, stderr and a shim is used for stdin, and Elm with ports are use on the other end. This allows message to be sent between the two worlds.


## TODO

- [ ] `light-dark` is not supported on my phone :(
- [ ] actually make it do the thing
-  -  [ ] log the new state encounted in a run
-  -  [ ] refactor agent view compression, maybe remove it
- [ ] rename episode_done to RUN_DONE

## Training params and RL

Notes on a discussion with Pier form the Apero du Code, 5 days later so any mistakes are on my end.

```
Q(s,a) ← Q(s,a) + α * (r + γ * max_a' Q(s',a') - Q(s,a))
```

### Alpha

The rate at which the model trains, typically very very low (0.1), each iteraciton only adjusts the model by a little bit, only changing a bit is important if there is lots of variancebetween each learn session. You can pick at random some set of states and see how their action values evolve over time, if very volotile a low alpha means the model is more stable, but slower to learn.

### Gamma

Discount facotor, or impact of future reward. It should always be below 1, else the model values intermidiate useless moves before going for the reward. A low value means the agent is short signted and does not learn the value of taking a longer route for eventual reward.

### Reward

Scale matters, keep everything around 1 and below. Death should maybe be 0, it's already punched becasue no  future rewards can be had. (Idk how this fits in with the model as it is. Maybe 1 for eat apple, 0.2 for red apple, and death is -0.1?)

### Epsylon and epsylon decay

Random exploration vs exploitation, how likely the model is to pick at random can be usefull to explore a large number of states at first, and then fine tune to only the best actions. Also these are fine tuning methods and should be focused on once the basic model works, they should be used to optimise the training time.


## Training setup

```
-Kind of ok training with the single tile state reprisentation
Episodes 5000
Run Size 100
Max Steps 500
Frame Time (ms) 100
Board X 10
Board Y 10
Alpha 0.1
Gamma 0.95
Epsilon 0
Epsilon Decay 0.995
Epsilon Min 0
Reward Advance -1
Reward Green 10
Reward Red -2
Reward Death -10
```
