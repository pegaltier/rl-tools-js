#pragma once

#include "trading_environment.h"

#include <rl_tools/operations/cpu_mux.h>
#include <rl_tools/nn/optimizers/adam/instance/operations_generic.h>
#include <rl_tools/nn/operations_cpu_mux.h>
#include <rl_tools/nn/layers/standardize/operations_generic.h>
#include <rl_tools/nn_models/mlp_unconditional_stddev/operations_generic.h>
#include <rl_tools/nn_models/sequential/operations_generic.h>
#include <rl_tools/nn/optimizers/adam/operations_generic.h>
#include <rl_tools/rl/algorithms/ppo/loop/core/config.h>
#include <rl_tools/rl/algorithms/ppo/loop/core/operations_generic.h>

namespace rlt = rl_tools;

using DEVICE = rlt::devices::DEVICE_FACTORY<>;
using RNG = DEVICE::SPEC::RANDOM::ENGINE<>;
using T = float;
using TYPE_POLICY = rlt::numeric_types::Policy<T>;
using TI = typename DEVICE::index_t;
using ENVIRONMENT = trading::Environment<trading::Specification<T, TI>>;

struct TRADING_LOOP_PARAMETERS: rlt::rl::algorithms::ppo::loop::core::DefaultParameters<TYPE_POLICY, TI, ENVIRONMENT> {
    static constexpr TI BATCH_SIZE = 128;
    struct PPO_PARAMETERS: rlt::rl::algorithms::ppo::DefaultParameters<TYPE_POLICY, TI, BATCH_SIZE> {
        static constexpr T ACTION_ENTROPY_COEFFICIENT = 0.005;
        static constexpr TI N_EPOCHS = 4;
        static constexpr T GAMMA = 0.99;
        static constexpr T LAMBDA = 0.95;
        static constexpr bool ADAPTIVE_LEARNING_RATE = false;
    };
    static constexpr TI ACTOR_HIDDEN_DIM = 32;
    static constexpr TI ACTOR_NUM_LAYERS = 2;
    static constexpr auto ACTOR_ACTIVATION_FUNCTION = rlt::nn::activation_functions::ActivationFunction::TANH;
    static constexpr TI CRITIC_HIDDEN_DIM = 32;
    static constexpr TI CRITIC_NUM_LAYERS = 2;
    static constexpr auto CRITIC_ACTIVATION_FUNCTION = rlt::nn::activation_functions::ActivationFunction::TANH;
    static constexpr TI ON_POLICY_RUNNER_STEPS_PER_ENV = ENVIRONMENT::EPISODE_STEP_LIMIT;
    static constexpr TI N_ENVIRONMENTS = 8;
    static constexpr TI TOTAL_STEP_LIMIT = 32768;
    static constexpr TI ITERATION_LIMIT = TOTAL_STEP_LIMIT / (ON_POLICY_RUNNER_STEPS_PER_ENV * N_ENVIRONMENTS);
    static constexpr TI STEP_LIMIT = ITERATION_LIMIT - 1;
    static constexpr TI EPISODE_STEP_LIMIT = ENVIRONMENT::EPISODE_STEP_LIMIT;
};

using TRADING_LOOP_CONFIG = rlt::rl::algorithms::ppo::loop::core::Config<
    TYPE_POLICY,
    TI,
    RNG,
    ENVIRONMENT,
    TRADING_LOOP_PARAMETERS,
    rlt::rl::algorithms::ppo::loop::core::ConfigApproximatorsSequential>;
using TRADING_LOOP_STATE = typename TRADING_LOOP_CONFIG::template State<TRADING_LOOP_CONFIG>;
using TRADING_ACTOR = typename TRADING_LOOP_CONFIG::NN::ACTOR_TYPE;
