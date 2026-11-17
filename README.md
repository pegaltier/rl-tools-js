# rl-tools-js

**Train and run an RLtools pendulum agent in Node.js or the browser, powered by WebAssembly.**

`rl-tools-js` wraps the [RLtools](https://github.com/rl-tools/rl-tools) Pendulum-v1 Soft Actor-Critic (SAC) demo and includes browser versions of the pendulum and racing-car demos from [rl.tools](https://rl.tools/). The compiled WASM is included: you do not need a C++ toolchain to try it.

> **Experimental:** This is not a general-purpose binding for all RLtools algorithms. The pendulum actor is trained from scratch; pretrained checkpoint loading and saving are not supported. The car example is browser-only.

## Get started

You need Node.js 18+ with WebAssembly SIMD support. No npm dependencies are required to run the included examples.

```sh
git clone https://github.com/pegaltier/rl-tools-js.git
cd rl-tools-js
npm run example:node -- 1000 0
```

The optional arguments are the number of training steps and the seed. The example prints the latest state, evaluation returns and an action from the current policy.

To use the library from another Node.js project, install this checkout locally (the package is not published to npm):

```sh
npm install /path/to/rl-tools-js
```

```js
import { createPendulumTrainer } from 'rl-tools-js';

const trainer = await createPendulumTrainer({ seed: 0 });

try {
  for (let i = 0; i < 1000 && !trainer.finished; i++) {
    trainer.step();
  }

  console.log(trainer.state);       // { theta, thetaDot }
  console.log(trainer.evaluations); // [{ step, meanReturn }, ...]
  console.log(trainer.action([1, 0, 0]));
} finally {
  trainer.dispose();
}
```

## Browser demos

Serve the repository root over HTTP:

```sh
python3 -m http.server 8000
```

| Demo | URL | What it does |
| --- | --- | --- |
| Pendulum SAC | [localhost:8000/examples/browser/](http://localhost:8000/examples/browser/) | Trains in a Web Worker, animates the pendulum and plots evaluation returns. |
| Racing car PPO | [localhost:8000/examples/car/](http://localhost:8000/examples/car/) | Draw or use the default track, save it, then drive with arrow keys or start training. |

Car training is substantially more expensive than pendulum training. Stopping the pendulum demo terminates its worker and releases its WASM memory.

Browser applications can also import `src/index.js` as an ES module. Serve it and the accompanying `dist` files at their relative paths over HTTP; `file://` loading is not supported. When using a bundler, ensure the generated JavaScript and WASM assets are emitted and accessible.

## Pendulum API

`await createPendulumTrainer({ seed })` creates an independent trainer. The seed defaults to `0` and must be an integer from `0` to `2147483647`.

| Member | Description |
| --- | --- |
| `step()` | Synchronously perform a training step; returns `true` once training is finished. |
| `stepCount` / `stepLimit` | Number of environment transitions performed and the configured limit. |
| `finished` | Whether training has finished. |
| `state` | Latest training environment state, `{ theta, thetaDot }`; `null` before the first step. |
| `evaluations` | Completed deterministic evaluations as `{ step, meanReturn }` records. |
| `action([cosTheta, sinTheta, thetaDot])` | Evaluate the current actor deterministically; returns a normalized action in `[-1, 1]`. |
| `dispose()` | Release the trainer's WASM allocations. Safe to call more than once. |

The upstream loop performs its final evaluation on the call **after** reaching `stepLimit`; that call sets `finished`. Inference uses its own RNG and does not advance training. Always dispose of trainers when finished. TypeScript declarations are included.

## Rebuild the WASM

The generated browser and Node binaries are committed in `dist/`. To rebuild them, install and activate Emscripten. The upstream demo documents version 4.0.17; the included binaries were built with 3.1.6.

```sh
git clone https://github.com/rl-tools/rl-tools.git
git -C rl-tools checkout b32d9985c65a5e098a6bbf190fd994962d288b99
git clone --branch v3.11.3 --depth 1 https://github.com/nlohmann/json.git

RL_TOOLS_DIR="$PWD/rl-tools" JSON_INCLUDE_DIR="$PWD/json/single_include" npm run build:wasm
npm test
```

The complete RLtools checkout is required because the pendulum binding includes its `full_training.h`. Builds target browser/worker and Node separately and use SIMD.

## Project layout

- `src/` — JavaScript pendulum API and TypeScript declarations.
- `wasm/` — C++ bindings and the adapted car training configuration.
- `dist/` — generated JavaScript loaders and WASM binaries.
- `examples/` — browser demos and a Node.js example.
- `test/` — Node.js integration tests.

## Attribution and license

The bindings use [RLtools](https://github.com/rl-tools/rl-tools) at commit [`b32d998`](https://github.com/rl-tools/rl-tools/tree/b32d9985c65a5e098a6bbf190fd994962d288b99). RLtools is MIT licensed; see [LICENSE](LICENSE). The car build also uses nlohmann/json; its MIT license is in [wasm/LICENSE-nlohmann-json](wasm/LICENSE-nlohmann-json).
