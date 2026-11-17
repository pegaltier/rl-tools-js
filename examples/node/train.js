import { createPendulumTrainer } from '../../src/index.js';

const steps = Number(process.argv[2] ?? 1000);
const seed = Number(process.argv[3] ?? 0);
if (!Number.isSafeInteger(steps) || steps < 0) throw new RangeError('steps must be a non-negative integer');

const trainer = await createPendulumTrainer({ seed });
try {
  for (let i = 0; i < steps && !trainer.finished; i++) trainer.step();
  console.log(JSON.stringify({
    step: trainer.stepCount,
    finished: trainer.finished,
    state: trainer.state,
    evaluations: trainer.evaluations,
    actionForUpright: trainer.action([1, 0, 0])
  }, null, 2));
} finally {
  trainer.dispose();
}
