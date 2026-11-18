// Shared by the browser worker and the Node.js example.
export async function createTradingTrainer({ seed = 0 } = {}) {
  if (!Number.isInteger(seed) || seed < 0 || seed > 0x7fffffff) {
    throw new RangeError('seed must be an integer between 0 and 2147483647');
  }
  const isNode = typeof process !== 'undefined' && !!process.versions?.node;
  const factory = isNode
    ? (await import('../../dist/trading-node.cjs')).default
    : (await import('../../dist/trading-browser.js')).default;
  const options = isNode
    ? { wasmBinary: await (await import('node:fs/promises')).readFile(new URL('../../dist/trading-node.wasm', import.meta.url)) }
    : {};
  const module = await factory(options);
  const pointer = module._trading_create(seed);
  if (!pointer) throw new Error('Failed to allocate the trading trainer');
  return new TradingTrainer(module, pointer);
}

class TradingTrainer {
  #module;
  #pointer;
  #finished = false;

  constructor(module, pointer) {
    this.#module = module;
    this.#pointer = pointer;
  }

  #check() {
    if (!this.#pointer) throw new Error('Trainer has been disposed');
  }

  get iteration() { this.#check(); return this.#module._trading_iteration(this.#pointer); }
  get iterationLimit() { this.#check(); return this.#module._trading_step_limit(); }
  get finished() { this.#check(); return this.#finished; }

  step() {
    this.#check();
    if (!this.#finished) this.#finished = Boolean(this.#module._trading_step(this.#pointer));
    return this.#finished;
  }

  evaluate() {
    this.#check();
    const m = this.#module;
    const p = this.#pointer;
    m._trading_evaluate(p);
    const count = m._trading_eval_count(p);
    return {
      iteration: this.iteration,
      finished: this.finished,
      totalReturn: m._trading_eval_return(p),
      trajectory: Array.from({ length: count }, (_, i) => ({
        price: m._trading_eval_price(p, i),
        equity: m._trading_eval_equity(p, i),
        position: m._trading_eval_position(p, i)
      }))
    };
  }

  dispose() {
    if (this.#pointer) {
      this.#module._trading_destroy(this.#pointer);
      this.#pointer = 0;
    }
  }
}
