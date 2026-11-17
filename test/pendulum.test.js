import test from 'node:test';
import assert from 'node:assert/strict';
import { createPendulumTrainer } from '../src/index.js';

test('trainers expose deterministic training, evaluations and policy actions', async () => {
  const a = await createPendulumTrainer({ seed: 7 });
  const b = await createPendulumTrainer({ seed: 7 });
  try {
    assert.equal(a.stepCount, 0);
    assert.equal(a.state, null);
    assert.deepEqual(a.evaluations, []);
    assert.equal(a.stepLimit, 100000);
    const action = a.action([1, 0, 0]);
    assert.ok(Number.isFinite(action) && Math.abs(action) <= 1);
    assert.equal(action, a.action([1, 0, 0]));
    assert.throws(() => a.action([0, NaN, 0]), TypeError);
    assert.throws(() => a.action([0, 1, 1e300]), TypeError);
    assert.throws(() => a.action([0, 1]), TypeError);
    for (let i = 0; i < 1101; i++) {
      assert.equal(a.step(), false);
      assert.equal(b.step(), false);
    }
    assert.equal(a.stepCount, 1101);
    assert.deepEqual(a.state, b.state);
    assert.ok(Number.isFinite(a.state.theta) && Number.isFinite(a.state.thetaDot));
    assert.deepEqual(a.evaluations, b.evaluations);
    assert.equal(a.evaluations.length, 2);
    assert.equal(a.evaluations[0].step, 0);
    assert.equal(a.evaluations[1].step, 1000);
    assert.ok(Number.isFinite(a.evaluations[0].meanReturn));
    assert.equal(a.action([1, 0, 0]), b.action([1, 0, 0]));
  } finally {
    a.dispose();
    b.dispose();
  }
  a.dispose();
  assert.throws(() => a.step(), /disposed/);
  assert.throws(() => a.action([1, 0, 0]), /disposed/);
});

test('rejects invalid seeds before loading WASM', async () => {
  for (const seed of [-1, 0.5, 2147483648, NaN]) {
    await assert.rejects(createPendulumTrainer({ seed }), RangeError);
  }
});
