import { createPendulumTrainer } from '../../src/index.js';

self.onmessage = async ({ data }) => {
  let trainer;
  try {
    trainer = await createPendulumTrainer({ seed: data.seed });
    while (!trainer.finished) {
      for (let i = 0; i < 100 && !trainer.finished; i++) trainer.step();
      self.postMessage({
        step: trainer.stepCount,
        limit: trainer.stepLimit,
        state: trainer.state,
        evaluations: trainer.evaluations,
        finished: trainer.finished
      });
    }
  } catch (error) {
    self.postMessage({ error: String(error) });
  } finally {
    trainer?.dispose();
  }
};
