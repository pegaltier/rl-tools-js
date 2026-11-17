export interface Evaluation {
  step: number;
  meanReturn: number;
}

export interface PendulumState {
  theta: number;
  thetaDot: number;
}

export interface PendulumTrainer {
  readonly stepCount: number;
  readonly stepLimit: number;
  readonly finished: boolean;
  readonly state: PendulumState | null;
  readonly evaluations: Evaluation[];
  step(): boolean;
  action(observation: [number, number, number]): number;
  dispose(): void;
}

export function createPendulumTrainer(options?: { seed?: number }): Promise<PendulumTrainer>;
