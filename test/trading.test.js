import test from 'node:test';
import assert from 'node:assert/strict';
import { createTradingTrainer } from '../examples/trading/trainer.js';

test('trading training and held-out evaluation are reproducible', async () => {
  const a = await createTradingTrainer({ seed: 7 });
  const b = await createTradingTrainer({ seed: 7 });
  try {
    const initial = a.evaluate();
    assert.deepEqual(initial, a.evaluate());
    assert.equal(initial.trajectory.length, 65);
    assert.deepEqual(initial.trajectory[0], { price: 100, equity: 1, position: 0 });
    for (const point of initial.trajectory) {
      assert.ok(Number.isFinite(point.price) && Number.isFinite(point.equity));
      assert.ok([-1, 0, 1].includes(point.position));
    }
    a.step();
    a.evaluate();
    b.step();
    a.step();
    b.step();
    const result = a.evaluate();
    assert.deepEqual(result, b.evaluate());
    assert.equal(result.iteration, 2);
    assert.ok(Number.isFinite(result.totalReturn));
    assert.ok(Math.abs(result.totalReturn - (result.trajectory.at(-1).equity - 1)) < 1e-12);
    for (let i = 1; i < result.trajectory.length; i++) {
      const previous = result.trajectory[i - 1];
      const current = result.trajectory[i];
      const logReturn = Math.log(current.price / previous.price);
      const cost = 0.0005 * Math.abs(current.position - previous.position);
      assert.ok(Math.abs(current.equity - previous.equity * Math.exp(current.position * logReturn - cost)) < 1e-7);
    }
  } finally {
    a.dispose();
    b.dispose();
  }
  a.dispose();
  assert.throws(() => a.step(), /disposed/);
  assert.throws(() => a.evaluate(), /disposed/);
});

test('trading trainer rejects invalid seeds', async () => {
  for (const seed of [-1, 0.5, 2147483648, NaN]) {
    await assert.rejects(createTradingTrainer({ seed }), RangeError);
  }
});
