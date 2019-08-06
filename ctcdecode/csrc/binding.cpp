#include <torch/torch.h>
#include <pybind11/stl.h>
#include <pybind11/complex.h>
#include <pybind11/functional.h>
#include <pybind11/chrono.h>
#include "binding.h"

using namespace pybind11::literals;

/**
 * Some hackery that lets pybind11 handle shared_ptr<void> (for LMStatePtr).
 * See: https://github.com/pybind/pybind11/issues/820
 */
PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);

/**
 * A pybind11 "alias type" for abstract class LM, allowing one to subclass LM
 * with a custom LM defined purely in Python. For those who don't want to build
 * with KenLM, or have their own custom LM implementation.
 * See: https://pybind11.readthedocs.io/en/stable/advanced/classes.html
 *
 * Currently this works in principle, but is very slow due to
 * decoder calling `compareState` a huge number of times.
 *
 * TODO: ensure this works. Last time I tried this there were slicing issues,
 * see https://github.com/pybind/pybind11/issues/1546 for workarounds.
 * This is low-pri since we assume most people can just build with KenLM.
 */
class PyLM : public LM
{
    using LM::LM;

    // needed for pybind11 or else it won't compile
    using LMOutput = std::pair<LMStatePtr, float>;

    LMStatePtr start(bool startWithNothing) override
    {
        PYBIND11_OVERLOAD_PURE(LMStatePtr, LM, start, startWithNothing);
    }

    LMOutput score(const LMStatePtr &state, const int usrTokenIdx) override
    {
        PYBIND11_OVERLOAD_PURE(LMOutput, LM, score, state, usrTokenIdx);
    }

    LMOutput finish(const LMStatePtr &state) override
    {
        PYBIND11_OVERLOAD_PURE(LMOutput, LM, finish, state);
    }

    int compareState(const LMStatePtr &state1, const LMStatePtr &state2)
        const override
    {
        PYBIND11_OVERLOAD_PURE(int, LM, compareState, state1, state2);
    }
};

LMStatePtr to_shared_ptr(py::object const &object)
{
    return std::make_shared<py::object>(object);
}

py::object *to_py_object(LMStatePtr &ptr)
{
    return static_cast<py::object *>(ptr.get());
}

LMStatePtr start(LMPtr lm) { return lm->start(0); }

std::tuple<at::Tensor, at::Tensor, at::Tensor, at::Tensor> beam_decoder(const at::Tensor log_probs,
                                                                        const at::Tensor seq_lengths,
                                                                        int blank_id,
                                                                        int beam_size,
                                                                        int num_processes,
                                                                        double cutoff_prob,
                                                                        int cutoff_top_n,
                                                                        LMPtr scorer)
{
    AT_ASSERT(log_probs.is_contiguous())
    AT_ASSERT(seq_lengths.is_contiguous())

    const int64_t batch_size = log_probs.size(0);
    const int64_t max_time = log_probs.size(1);
    const int64_t num_classes = log_probs.size(2);

    const int64_t seq_lengths_size = seq_lengths.size(0);

    std::vector<std::vector<Output>> batch_results = ctc_beam_search_decoder_batch(
        log_probs.data<float>(),
        batch_size,
        max_time,
        num_classes,
        seq_lengths.data<int>(),
        seq_lengths_size,
        blank_id,
        beam_size,
        num_processes,
        log(cutoff_prob),
        cutoff_top_n,
        scorer);

    at::Tensor output = torch::empty({batch_size, beam_size, max_time}, torch::dtype(torch::kInt32).device(torch::kCPU));
    at::Tensor timesteps = torch::empty({batch_size, beam_size, max_time}, torch::dtype(torch::kInt32).device(torch::kCPU));
    at::Tensor scores = torch::empty({batch_size, beam_size}, torch::dtype(torch::kFloat32).device(torch::kCPU));
    at::Tensor output_length = torch::empty({batch_size, beam_size}, torch::dtype(torch::kInt32).device(torch::kCPU));

    auto output_a = output.accessor<int32_t, 3>();
    auto timesteps_a = timesteps.accessor<int32_t, 3>();

    auto output_length_a = output_length.accessor<int32_t, 2>();
    auto scores_a = scores.accessor<float, 2>();

    for (int b = 0; b < batch_results.size(); ++b)
    {
        std::vector<Output> results = batch_results[b];
        for (int p = 0; p < results.size(); ++p)
        {
            Output out = results[p];
            auto output_tokens = out.tokens;
            auto output_timesteps = out.timesteps;
            for (int t = 0; t < output_tokens.size(); ++t)
            {
                output_a[b][p][t] = output_tokens[t];
                timesteps_a[b][p][t] = output_timesteps[t];
            }
            scores_a[b][p] = out.probability;
            output_length_a[b][p] = output_tokens.size();
        }
    }
    return std::make_tuple(output, scores, timesteps, output_length);
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m)
{

    py::class_<std::shared_ptr<void>>(m, "encapsulated_data");

    py::class_<LM, LMPtr, PyLM>(m, "LM")
        .def(py::init<>())
        .def("start", &LM::start, "start_with_nothing"_a)
        .def("score", &LM::score, "state"_a, "token_index"_a)
        .def("finish", &LM::finish, "state"_a)
        .def("compare_state", &LM::compareState, "state1"_a, "state2"_a)
        .def_readwrite("alpha", &LM::alpha)
        .def_readwrite("beta", &LM::beta);

    m.def("beam_decoder", &beam_decoder, "beam_decoder");
    m.def("to_shared_ptr", &to_shared_ptr, "to_shared_ptr");
    m.def("to_py_object", &to_shared_ptr, "to_py_object");
    m.def("start", &start, "start");
}
