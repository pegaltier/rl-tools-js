#include "trading_ppo.h"
#include <emscripten.h>

#include <array>
#include <cmath>
#include <cstdint>

namespace {
    constexpr int EVALUATION_TRANSITIONS = ENVIRONMENT::EPISODE_STEP_LIMIT;
    constexpr int EVALUATION_POINTS = EVALUATION_TRANSITIONS + 1;
    constexpr float INITIAL_PRICE = 100.0f;

    struct Trading {
        DEVICE device;
        TRADING_LOOP_STATE state;
        TRADING_ACTOR::template Buffer<true> inference_buffer;
        RNG inference_rng;
        int seed = 0;
        int evaluation_count = 0;
        bool finished = false;
        std::array<double, EVALUATION_POINTS> prices{};
        std::array<double, EVALUATION_POINTS> equities{};
        std::array<int, EVALUATION_POINTS> positions{};
        double evaluation_return = 0;
    };

    float evaluation_return_at(int time, int seed) {
        return trading::market_return<float>(time, seed, true);
    }

    void set_observation(Trading& trainer, int time, int seed, int position) {
        using INPUT_SHAPE = typename TRADING_ACTOR::INPUT_SHAPE;
        using OUTPUT_SHAPE = typename TRADING_ACTOR::OUTPUT_SHAPE;
        rlt::Tensor<rlt::tensor::Specification<T, TI, INPUT_SHAPE, false>> input;
        rlt::Tensor<rlt::tensor::Specification<T, TI, OUTPUT_SHAPE, false>> output;
        const auto return_at = [seed](int index) {
            return index <= 0 ? 0.0f : evaluation_return_at(index, seed);
        };
        T average = 0;
        for (int lag = 0; lag < 5; lag++) average += return_at(time - lag);
        average /= 5.0f;
        const T latest = return_at(time);
        const T momentum = return_at(time) + return_at(time - 1) + return_at(time - 2);
        rlt::set(trainer.device, input, latest * 100.0f, 0, 0, 0);
        rlt::set(trainer.device, input, average * 100.0f, 0, 0, 1);
        rlt::set(trainer.device, input, momentum * (100.0f / 3.0f), 0, 0, 2);
        rlt::set(trainer.device, input, (T)position, 0, 0, 3);
        rlt::evaluate(trainer.device, rlt::get_actor(trainer.state), input, output,
                      trainer.inference_buffer, trainer.inference_rng,
                      rlt::Mode<rlt::mode::Evaluation<>>{});
        trainer.positions[time + 1] = (int)trading::position_from_action(
            rlt::get(trainer.device, output, 0, 0, 0), ENVIRONMENT::POSITION_THRESHOLD);
    }
}

extern "C" {
EMSCRIPTEN_KEEPALIVE Trading* trading_create(int seed) {
    auto* trainer = new Trading{};
    trainer->seed = seed;
    rlt::malloc(trainer->device, trainer->state);
    rlt::malloc(trainer->device, trainer->inference_buffer);
    rlt::init(trainer->device, trainer->state, seed);
    rlt::init(trainer->device, trainer->inference_rng, seed ^ 0x5f3759df);
    return trainer;
}

EMSCRIPTEN_KEEPALIVE void trading_destroy(Trading* trainer) {
    if (!trainer) return;
    rlt::free(trainer->device, trainer->inference_buffer);
    rlt::free(trainer->device, trainer->state);
    delete trainer;
}

EMSCRIPTEN_KEEPALIVE int trading_step(Trading* trainer) {
    if (!trainer->finished) trainer->finished = rlt::step(trainer->device, trainer->state);
    return trainer->finished ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE int trading_iteration(Trading* trainer) {
    return trainer->state.step;
}

EMSCRIPTEN_KEEPALIVE int trading_step_limit() {
    return TRADING_LOOP_PARAMETERS::ITERATION_LIMIT;
}

EMSCRIPTEN_KEEPALIVE int trading_evaluate(Trading* trainer) {
    const int held_out_seed = trainer->seed ^ 0x6d2b79f5;
    RNG evaluation_rng;
    rlt::init(trainer->device, evaluation_rng, held_out_seed ^ 0x5f3759df);
    trainer->inference_rng = evaluation_rng;

    trainer->prices[0] = INITIAL_PRICE;
    trainer->equities[0] = 1.0;
    trainer->positions[0] = 0;
    int current_position = 0;
    for (int time = 0; time < EVALUATION_TRANSITIONS; time++) {
        set_observation(*trainer, time, held_out_seed, current_position);
        current_position = trainer->positions[time + 1];
        const float gross_return = evaluation_return_at(time + 1, held_out_seed);
        const double cost = ENVIRONMENT::TRANSACTION_COST * std::abs(current_position - trainer->positions[time]);
        trainer->prices[time + 1] = trainer->prices[time] * std::exp(gross_return);
        trainer->equities[time + 1] = trainer->equities[time] *
            std::exp(current_position * gross_return - cost);
    }
    trainer->evaluation_return = trainer->equities[EVALUATION_TRANSITIONS] - 1.0;
    trainer->evaluation_count = EVALUATION_POINTS;
    return EVALUATION_POINTS;
}

EMSCRIPTEN_KEEPALIVE int trading_eval_count(Trading* trainer) {
    return trainer->evaluation_count;
}

EMSCRIPTEN_KEEPALIVE double trading_eval_price(Trading* trainer, int index) {
    return trainer->evaluation_count && index >= 0 && index < EVALUATION_POINTS ? trainer->prices[index] : 0.0;
}

EMSCRIPTEN_KEEPALIVE double trading_eval_equity(Trading* trainer, int index) {
    return trainer->evaluation_count && index >= 0 && index < EVALUATION_POINTS ? trainer->equities[index] : 0.0;
}

EMSCRIPTEN_KEEPALIVE int trading_eval_position(Trading* trainer, int index) {
    return trainer->evaluation_count && index >= 0 && index < EVALUATION_POINTS ? trainer->positions[index] : 0;
}

EMSCRIPTEN_KEEPALIVE double trading_eval_return(Trading* trainer) {
    return trainer->evaluation_count ? trainer->evaluation_return : 0.0;
}
}
