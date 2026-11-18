import { createTradingTrainer } from './trainer.js';

self.onmessage = async ({ data: { seed, iterations } }) => {
  let trainer;
  try {
    trainer = await createTradingTrainer({ seed });
    for (let i = 0; i < iterations && !trainer.finished; i++) {
      trainer.step();
      self.postMessage({ progress: trainer.iteration, limit: trainer.iterationLimit });
    }
    self.postMessage({ result: trainer.evaluate() });
  } catch (error) {
    self.postMessage({ error: error.message });
  } finally {
    trainer?.dispose();
  }
};
