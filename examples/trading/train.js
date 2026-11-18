import { createTradingTrainer } from './trainer.js';

const iterations = Number(process.argv[2] ?? 10);
const seed = Number(process.argv[3] ?? 0);
if (!Number.isSafeInteger(iterations) || iterations < 0) {
  throw new RangeError('iterations must be a non-negative integer');
}
const trainer = await createTradingTrainer({ seed });
try {
  for (let i = 0; i < iterations && !trainer.finished; i++) trainer.step();
  console.log(JSON.stringify(trainer.evaluate(), null, 2));
} finally {
  trainer.dispose();
}
