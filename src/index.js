function validSeed(seed) {
  if (!Number.isInteger(seed) || seed < 0 || seed > 0x7fffffff) {
    throw new RangeError('seed must be an integer between 0 and 2147483647');
  }
}

export async function createPendulumTrainer({ seed = 0 } = {}) {
  validSeed(seed);
  const isNode = typeof process !== 'undefined' && !!process.versions?.node;
  const factory = isNode
    ? (await import('../dist/pendulum-node.cjs')).default
    : (await import('../dist/pendulum-browser.js')).default;
  const options = isNode
    ? { wasmBinary: await (await import('node:fs/promises')).readFile(new URL('../dist/pendulum-node.wasm', import.meta.url)) }
    : {};
  const module = await factory(options);
  const pointer = module._trainer_create(seed);
  if (!pointer) throw new Error('Failed to allocate the training state');
  return new PendulumTrainer(module, pointer);
}

class PendulumTrainer {
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

  get stepCount() { this.#check(); return this.#module._trainer_get_step(this.#pointer); }
  get stepLimit() { this.#check(); return this.#module._trainer_step_limit(); }
  get finished() { this.#check(); return this.#finished; }

  step() {
    this.#check();
    this.#finished = Boolean(this.#module._trainer_step(this.#pointer));
    return this.#finished;
  }

  get state() {
    this.#check();
    if (this.stepCount === 0) return null;
    return { theta: this.#module._trainer_theta(this.#pointer), thetaDot: this.#module._trainer_theta_dot(this.#pointer) };
  }

  get evaluations() {
    this.#check();
    const count = this.#module._trainer_evaluation_count(this.#pointer);
    const interval = this.#module._trainer_evaluation_interval();
    return Array.from({ length: count }, (_, i) => ({
      step: i * interval,
      meanReturn: this.#module._trainer_evaluation_return(this.#pointer, i)
    }));
  }

  action(observation) {
    this.#check();
    if (!Array.isArray(observation) || observation.length !== 3 ||
        !observation.every(value => Number.isFinite(value) && Number.isFinite(Math.fround(value)))) {
      throw new TypeError('observation must be [cos(theta), sin(theta), thetaDot] with finite float32 values');
    }
    return this.#module._trainer_action(this.#pointer, ...observation);
  }

  dispose() {
    if (this.#pointer) {
      this.#module._trainer_destroy(this.#pointer);
      this.#pointer = 0;
    }
  }
}
