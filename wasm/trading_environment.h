#pragma once

#include <rl_tools/operations/cpu_mux.h>
#include <cmath>
#include <rl_tools/rl/environments/environments.h>

namespace trading {
    template <typename T_T, typename T_TI>
    struct Observation {
        using T = T_T;
        using TI = T_TI;
        static constexpr TI DIM = 4;
    };

    template <typename T_T, typename T_TI>
    struct State {
        using T = T_T;
        using TI = T_TI;
        TI time = 0;
        T position = 0;
    };

    template <typename T_T, typename T_TI>
    struct Parameters {
        using T = T_T;
        using TI = T_TI;
        TI series_seed = 0;
    };

    template <typename T_T, typename T_TI>
    struct Specification {
        using T = T_T;
        using TI = T_TI;
        using Parameters = trading::Parameters<T, TI>;
    };

    template <typename T_SPEC>
    struct Environment: rl_tools::rl::environments::Environment<typename T_SPEC::T, typename T_SPEC::TI> {
        using SPEC = T_SPEC;
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        using State = trading::State<T, TI>;
        using Parameters = typename SPEC::Parameters;
        using Observation = trading::Observation<T, TI>;
        using ObservationPrivileged = Observation;
        static constexpr TI ACTION_DIM = 1;
        static constexpr TI EPISODE_STEP_LIMIT = 64;
        static constexpr T POSITION_THRESHOLD = (T)0.25;
        static constexpr T TRANSACTION_COST = (T)0.0005;
    };

    template <typename T>
    RL_TOOLS_FUNCTION_PLACEMENT T position_from_action(T raw_action, T threshold) {
        return raw_action > threshold ? (T)1 : (raw_action < -threshold ? (T)-1 : (T)0);
    }

    template <typename T>
    RL_TOOLS_FUNCTION_PLACEMENT T market_return(int time, int seed, bool held_out = false) {
        if (held_out) {
            return (T)0.0018 * (T)std::sin((T)(time + seed % 83) * (T)0.083)
                 + (T)0.0006 * (T)std::sin((T)(time + seed % 47) * (T)0.157)
                 + (T)0.0005 * (T)std::sin((T)time * (T)1.414 + (T)(seed % 29));
        }
        return (T)0.002 * (T)std::sin((T)(time + seed % 97) * (T)0.071)
             + (T)0.0007 * (T)std::sin((T)(time + seed % 53) * (T)0.193)
             + (T)0.00045 * (T)std::sin((T)time * (T)1.618 + (T)(seed % 31));
    }
}

namespace rl_tools {
    template <typename DEVICE, typename SPEC>
    RL_TOOLS_FUNCTION_PLACEMENT void malloc(DEVICE&, const trading::Environment<SPEC>&) {}
    template <typename DEVICE, typename SPEC>
    RL_TOOLS_FUNCTION_PLACEMENT void free(DEVICE&, const trading::Environment<SPEC>&) {}
    template <typename DEVICE, typename SPEC>
    RL_TOOLS_FUNCTION_PLACEMENT void init(DEVICE&, const trading::Environment<SPEC>&) {}

    template <typename DEVICE, typename SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void sample_initial_parameters(DEVICE& device, const trading::Environment<SPEC>&, typename trading::Environment<SPEC>::Parameters& parameters, RNG& rng) {
        parameters.series_seed = random::uniform_int_distribution(device.random, 0, 0x7fffffff, rng);
    }

    template <typename DEVICE, typename SPEC>
    RL_TOOLS_FUNCTION_PLACEMENT void initial_parameters(DEVICE&, const trading::Environment<SPEC>&, typename trading::Environment<SPEC>::Parameters&) {}

    template <typename DEVICE, typename SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void sample_initial_state(DEVICE&, const trading::Environment<SPEC>&, typename trading::Environment<SPEC>::Parameters&, typename trading::Environment<SPEC>::State& state, RNG&) {
        state.time = 0;
        state.position = 0;
    }

    template <typename DEVICE, typename SPEC, typename ACTION_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT typename SPEC::T step(DEVICE&, const trading::Environment<SPEC>&, typename trading::Environment<SPEC>::Parameters& parameters, const typename trading::Environment<SPEC>::State& state, const Matrix<ACTION_SPEC>& action, typename trading::Environment<SPEC>::State& next_state, RNG&) {
        using T = typename SPEC::T;
        const T position = trading::position_from_action(get(action, 0, 0), trading::Environment<SPEC>::POSITION_THRESHOLD);
        next_state.time = state.time + 1;
        next_state.position = position;
        return (T)1;
    }

    template <typename DEVICE, typename SPEC, typename ACTION_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT typename SPEC::T reward(DEVICE&, const trading::Environment<SPEC>&, typename trading::Environment<SPEC>::Parameters& parameters, const typename trading::Environment<SPEC>::State& state, const Matrix<ACTION_SPEC>& action, const typename trading::Environment<SPEC>::State&, RNG&) {
        using T = typename SPEC::T;
        const T position = trading::position_from_action(get(action, 0, 0), trading::Environment<SPEC>::POSITION_THRESHOLD);
        const T transaction_cost = trading::Environment<SPEC>::TRANSACTION_COST * (T)std::abs(position - state.position);
        return position * trading::market_return<T>((int)state.time + 1, (int)parameters.series_seed) - transaction_cost;
    }

    template <typename DEVICE, typename SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT bool terminated(DEVICE&, const trading::Environment<SPEC>&, typename trading::Environment<SPEC>::Parameters&, const typename trading::Environment<SPEC>::State& state, RNG&) {
        return state.time >= trading::Environment<SPEC>::EPISODE_STEP_LIMIT;
    }

    template <typename DEVICE, typename SPEC, typename OBS_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void observe(DEVICE&, const trading::Environment<SPEC>&, const typename trading::Environment<SPEC>::Parameters& parameters, const typename trading::Environment<SPEC>::State& state, const typename trading::Environment<SPEC>::Observation&, Matrix<OBS_SPEC>& observation, RNG&) {
        using T = typename SPEC::T;
        const auto return_at = [&](int time) -> T {
            if (time <= 0) return 0;
            return trading::market_return<T>(time, (int)parameters.series_seed);
        };
        const T latest = return_at(state.time);
        T average = 0;
        for (int lag = 0; lag < 5; lag++) average += return_at((int)state.time - lag);
        average /= (T)5;
        const T momentum = return_at((int)state.time) + return_at((int)state.time - 1) + return_at((int)state.time - 2);
        set(observation, 0, 0, latest * (T)100);
        set(observation, 0, 1, average * (T)100);
        set(observation, 0, 2, momentum * (T)(100.0 / 3.0));
        set(observation, 0, 3, state.position);
    }
}
