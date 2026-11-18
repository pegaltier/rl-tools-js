#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
: "${RL_TOOLS_DIR:?Set RL_TOOLS_DIR to an RLtools checkout at the revision documented in README.md}"
: "${JSON_INCLUDE_DIR:?Set JSON_INCLUDE_DIR to the nlohmann/json single_include directory}"
command -v em++ >/dev/null || { echo "em++ not found; activate the Emscripten SDK" >&2; exit 1; }
mkdir -p "$root/dist"
exports="['_trainer_create','_trainer_destroy','_trainer_step','_trainer_get_step','_trainer_step_limit','_trainer_evaluation_interval','_trainer_evaluation_count','_trainer_evaluation_return','_trainer_theta','_trainer_theta_dot','_trainer_action']"
common=(-std=c++17 -O3 -msimd128 -I"$RL_TOOLS_DIR/include" -I"$RL_TOOLS_DIR/src/rl/environments/pendulum/sac/wasm" -s "EXPORTED_FUNCTIONS=$exports" -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1)
em++ "${common[@]}" -s EXPORT_ES6=1 -s ENVIRONMENT=web,worker "$root/wasm/pendulum.cpp" -o "$root/dist/pendulum-browser.js"
em++ "${common[@]}" -s ENVIRONMENT=node "$root/wasm/pendulum.cpp" -o "$root/dist/pendulum-node.cjs"
trading_exports="['_trading_create','_trading_destroy','_trading_step','_trading_iteration','_trading_step_limit','_trading_evaluate','_trading_eval_count','_trading_eval_price','_trading_eval_equity','_trading_eval_position','_trading_eval_return']"
trading_common=(-std=c++17 -O3 -msimd128 -I"$RL_TOOLS_DIR/include" \
  -s "EXPORTED_FUNCTIONS=$trading_exports" -s MODULARIZE=1 -s ALLOW_MEMORY_GROWTH=1)
em++ "${trading_common[@]}" -s EXPORT_ES6=1 -s ENVIRONMENT=web,worker \
  "$root/wasm/trading.cpp" -o "$root/dist/trading-browser.js"
sed -i 's/[[:space:]]*$//' "$root/dist/trading-browser.js"
em++ "${trading_common[@]}" -s ENVIRONMENT=node \
  "$root/wasm/trading.cpp" -o "$root/dist/trading-node.cjs"
car_exports="['_proxy_create','_proxy_step','_proxy_step_message','_proxy_destroy','_proxy_num_messages','_proxy_pop_message','_proxy_delete_message','_malloc','_free']"
em++ -std=c++17 -O3 -msimd128 -I"$RL_TOOLS_DIR/include" -I"$JSON_INCLUDE_DIR" \
  -s "EXPORTED_FUNCTIONS=$car_exports" -s "EXPORTED_RUNTIME_METHODS=['UTF8ToString','stringToUTF8','lengthBytesUTF8']" \
  -s MODULARIZE=1 -s EXPORT_ES6=1 -s ENVIRONMENT=web,worker -s ALLOW_MEMORY_GROWTH=1 \
  "$root/wasm/wasm_interface.cpp" -o "$root/dist/car-browser.js"
