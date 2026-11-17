#include "full_training.h"
#include <emscripten.h>

struct Trainer {
    DEVICE device;
    LOOP_STATE state;
    LOOP_CORE_CONFIG::EVAL_ACTOR_TYPE::Buffer<true> inference_buffer;
    RNG inference_rng;
    int evaluations = 0;
    bool finished = false;
};

extern "C" {
EMSCRIPTEN_KEEPALIVE Trainer* trainer_create(int seed) {
    auto* trainer = new Trainer{};
    rlt::malloc(trainer->device, trainer->state);
    rlt::malloc(trainer->device, trainer->inference_buffer);
    rlt::init(trainer->device, trainer->state, seed);
    rlt::init(trainer->device, trainer->inference_rng, seed);
    return trainer;
}

EMSCRIPTEN_KEEPALIVE void trainer_destroy(Trainer* trainer) {
    rlt::free(trainer->device, trainer->inference_buffer);
    rlt::free(trainer->device, trainer->state);
    delete trainer;
}

EMSCRIPTEN_KEEPALIVE int trainer_step(Trainer* trainer) {
    if (!trainer->finished) {
        const auto step = trainer->state.step;
        trainer->finished = rlt::step(trainer->device, trainer->state);
        if (step % LOOP_EVAL_PARAMETERS::EVALUATION_INTERVAL == 0 &&
            step / LOOP_EVAL_PARAMETERS::EVALUATION_INTERVAL < LOOP_EVAL_PARAMETERS::N_EVALUATIONS) {
            trainer->evaluations++;
        }
    }
    return trainer->finished;
}

EMSCRIPTEN_KEEPALIVE int trainer_get_step(Trainer* trainer) { return trainer->state.step; }
EMSCRIPTEN_KEEPALIVE int trainer_step_limit() { return LOOP_CORE_PARAMETERS::STEP_LIMIT; }
EMSCRIPTEN_KEEPALIVE int trainer_evaluation_interval() { return LOOP_EVAL_PARAMETERS::EVALUATION_INTERVAL; }
EMSCRIPTEN_KEEPALIVE int trainer_evaluation_count(Trainer* trainer) { return trainer->evaluations; }
EMSCRIPTEN_KEEPALIVE double trainer_evaluation_return(Trainer* trainer, int index) {
    return rlt::get(trainer->state.evaluation_results, 0, index).returns_mean;
}
EMSCRIPTEN_KEEPALIVE double trainer_theta(Trainer* trainer) {
    return rlt::get(trainer->state.off_policy_runner.states, 0, 0).theta;
}
EMSCRIPTEN_KEEPALIVE double trainer_theta_dot(Trainer* trainer) {
    return rlt::get(trainer->state.off_policy_runner.states, 0, 0).theta_dot;
}
EMSCRIPTEN_KEEPALIVE double trainer_action(Trainer* trainer, float cosine, float sine, float velocity) {
    using ACTOR = LOOP_CORE_CONFIG::EVAL_ACTOR_TYPE;
    rlt::Tensor<rlt::tensor::Specification<T, TI, ACTOR::INPUT_SHAPE, false>> input;
    rlt::Tensor<rlt::tensor::Specification<T, TI, ACTOR::OUTPUT_SHAPE, false>> output;
    rlt::set(trainer->device, input, cosine, 0, 0, 0);
    rlt::set(trainer->device, input, sine, 0, 0, 1);
    rlt::set(trainer->device, input, velocity, 0, 0, 2);
    rlt::evaluate(trainer->device, rlt::get_actor(trainer->state), input, output,
                  trainer->inference_buffer, trainer->inference_rng, rlt::Mode<rlt::mode::Evaluation<>>{});
    return rlt::get(trainer->device, output, 0, 0, 0);
}
}
