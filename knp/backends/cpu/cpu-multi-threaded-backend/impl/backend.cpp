/**
 * @file backend.cpp
 * @brief Multi-threaded CPU backend class implementation.
 * @author Artiom N.
 * @date 21.06.2023
 */

#include <knp/backends/cpu-library/blifat_population.h>
#include <knp/backends/cpu-library/delta_synapse_projection.h>
#include <knp/backends/cpu-multi-threaded/backend.h>
#include <knp/backends/thread_pool/thread_pool.h>
#include <knp/core/core.h>
#include <knp/devices/cpu.h>
#include <knp/meta/stringify.h>

#include <spdlog/spdlog.h>

#include <functional>
#include <optional>
#include <vector>

#include <boost/mp11.hpp>


namespace knp::backends::multi_threaded_cpu
{
MultiThreadedCPUBackend::MultiThreadedCPUBackend(size_t thread_count)
    : message_endpoint_{message_bus_.create_endpoint()},
      calc_pool_(std::make_unique<ThreadPool>(thread_count ? thread_count : std::thread::hardware_concurrency()))
{
    SPDLOG_INFO("MT CPU backend instance created, threads count = {}...", thread_count);
}


MultiThreadedCPUBackend::~MultiThreadedCPUBackend()
{
    // stop() and join() will be called in the thread_pool destructor.
}


std::shared_ptr<MultiThreadedCPUBackend> MultiThreadedCPUBackend::create()
{
    SPDLOG_DEBUG("Creating MT CPU backend instance...");
    return std::make_shared<MultiThreadedCPUBackend>();
}


std::vector<std::string> MultiThreadedCPUBackend::get_supported_neurons() const
{
    return knp::meta::get_supported_type_names<knp::neuron_traits::AllNeurons, SupportedNeurons>(
        knp::neuron_traits::neurons_names);
}


std::vector<std::string> MultiThreadedCPUBackend::get_supported_synapses() const
{
    return knp::meta::get_supported_type_names<knp::synapse_traits::AllSynapses, SupportedSynapses>(
        knp::synapse_traits::synapses_names);
}


void MultiThreadedCPUBackend::calculate_populations_pre_impact()
{
    for (auto &population : populations_)
    {
        auto pop_size = std::visit([](auto &pop) { return pop.size(); }, population);
        for (size_t neuron_index = 0; neuron_index < pop_size; neuron_index += population_part_size_)
        {
            std::visit(
                [this, neuron_index](auto &pop)
                {
                    // Check if population is supported by backend. We don't need to repeat it.
                    using T = std::decay_t<decltype(pop)>;
                    if constexpr (
                        boost::mp11::mp_find<SupportedPopulations, T>{} == boost::mp11::mp_size<SupportedPopulations>{})
                        static_assert(
                            knp::core::always_false_v<T>, "Population isn't supported by the CPU MT backend!");

                    // Start threads.
                    calc_pool_->post(calculate_neurons_state_part, std::ref(pop), neuron_index, population_part_size_);
                },
                population);
        }
    }
    // Wait for all threads to finish their work.
    calc_pool_->join();
}


void MultiThreadedCPUBackend::calculate_populations_impact()
{
    for (auto &population : populations_)
    {
        auto uid = std::visit([](auto &population) { return population.get_uid(); }, population);
        auto messages = message_endpoint_.unload_messages<knp::core::messaging::SynapticImpactMessage>(uid);
        std::visit(
            [this, &messages](auto &pop) { calc_pool_->post(process_inputs, std::ref(pop), messages); }, population);
    }
    calc_pool_->join();
}


std::vector<knp::core::messaging::SpikeMessage> MultiThreadedCPUBackend::calculate_populations_post_impact()
{
    std::vector<knp::core::messaging::SpikeMessage> spike_container(populations_.size());
    for (size_t pop_index = 0; pop_index < populations_.size(); ++pop_index)
    {
        auto &population = populations_[pop_index];
        auto &message = spike_container[pop_index];
        message.header_.send_time_ = get_step();
        message.header_.sender_uid_ = std::visit([](auto &population) { return population.get_uid(); }, population);

        size_t population_size = std::visit([](auto &population) { return population.size(); }, population);
        for (size_t neuron_index = 0; neuron_index < population_size; neuron_index += population_part_size_)
        {
            std::visit(
                [this, &message, neuron_index](auto &pop)
                {
                    calc_pool_->post(
                        calculate_neurons_post_input_state_part, std::ref(pop), std::ref(message), neuron_index,
                        population_part_size_, std::ref(ep_mutex_));
                },
                population);
        }
    }
    calc_pool_->join();
    return spike_container;
}


void MultiThreadedCPUBackend::calculate_populations()
{
    SPDLOG_DEBUG("Calculating populations");
    calculate_populations_pre_impact();

    calculate_populations_impact();

    auto spike_messages = calculate_populations_post_impact();

    // Sending non-empty messages.
    for (const auto &message : spike_messages)
    {
        if (message.neuron_indexes_.empty()) continue;
        message_endpoint_.send_message(message);
    }
}


void MultiThreadedCPUBackend::calculate_projections()
{
    SPDLOG_DEBUG("Calculating projections");
    for (auto &projection : projections_)
    {
        auto uid = std::visit([](auto &proj) { return proj.get_uid(); }, projection.arg_);
        auto msg_buf = message_endpoint_.unload_messages<knp::core::messaging::SpikeMessage>(uid);

        // We might want to add some preliminary function before, even if delta proj doesn't require it.
        if (msg_buf.empty()) continue;

        // Looping over synapses.
        auto message_data = convert_spikes(msg_buf[0]);
        auto proj_size = std::visit([](const auto &proj) { return proj.size(); }, projection.arg_);
        for (size_t synapse_index = 0; synapse_index < proj_size; synapse_index += projection_part_size_)
        {
            std::visit(
                [this, synapse_index, &message_data, &projection](auto &proj)
                {
                    calc_pool_->post(
                        calculate_projection_part, std::ref(proj), message_data, std::ref(projection.messages_),
                        get_step(), synapse_index, projection_part_size_, std::ref(ep_mutex_));
                },
                projection.arg_);
        }
        calc_pool_->join();
    }
    // Sending messages. It might be possible to parallelize this as well if we use more than one endpoint.
    for (auto &projection : projections_)
    {
        auto &msg_queue = projection.messages_;
        auto msg_iter = msg_queue.find(get_step());
        if (msg_iter != msg_queue.end())
        {
            message_endpoint_.send_message(std::move(msg_iter->second));
            msg_queue.erase(msg_iter);
        }
    }
}


std::vector<size_t> MultiThreadedCPUBackend::get_supported_projection_indexes() const
{
    return knp::meta::get_supported_type_indexes<core::AllProjections, SupportedProjections>();
}


std::vector<size_t> MultiThreadedCPUBackend::get_supported_population_indexes() const
{
    return knp::meta::get_supported_type_indexes<core::AllPopulations, SupportedPopulations>();
}


void MultiThreadedCPUBackend::step()
{
    SPDLOG_DEBUG("Starting step #{}.", get_step());
    calculate_populations();
    message_bus_.route_messages();
    message_endpoint_.receive_all_messages();
    calculate_projections();
    message_bus_.route_messages();
    message_endpoint_.receive_all_messages();
    gad_step();
    SPDLOG_DEBUG("Step #{} finished.", get_step());
}


void MultiThreadedCPUBackend::load_populations(const std::vector<PopulationVariants> &populations)
{
    SPDLOG_DEBUG("Loading populations");
    populations_.clear();
    populations_.reserve(populations.size());

    for (const auto &p : populations) populations_.push_back(p);
    SPDLOG_DEBUG("All populations loaded");
}


void MultiThreadedCPUBackend::load_projections(const std::vector<ProjectionVariants> &projections)
{
    SPDLOG_DEBUG("Loading projections");
    projections_.clear();
    projections_.reserve(projections.size());

    for (const auto &p : projections)
    {
        projections_.push_back(ProjectionWrapper{p});
    }

    SPDLOG_DEBUG("All projections loaded");
}


std::vector<std::unique_ptr<knp::core::Device>> MultiThreadedCPUBackend::get_devices() const
{
    std::vector<std::unique_ptr<knp::core::Device>> result;
    auto processors{knp::devices::cpu::list_processors()};

    result.reserve(processors.size());

    for (auto &&cpu : processors)
    {
        SPDLOG_DEBUG("Device CPU \"{}\"", cpu.get_name());
        result.push_back(std::make_unique<knp::devices::cpu::CPU>(std::move(cpu)));
    }

    SPDLOG_DEBUG("CPUs count = {}", result.size());
    return result;
}


void MultiThreadedCPUBackend::init()
{
    SPDLOG_DEBUG("Initializing...");

    for (const auto &p : projections_)
    {
        const auto [pre_uid, post_uid, this_uid] = std::visit(
            [](const auto &proj)
            { return std::make_tuple(proj.get_presynaptic(), proj.get_postsynaptic(), proj.get_uid()); },
            p.arg_);

        if (pre_uid) message_endpoint_.subscribe<knp::core::messaging::SpikeMessage>(this_uid, {pre_uid});
        if (post_uid) message_endpoint_.subscribe<knp::core::messaging::SynapticImpactMessage>(post_uid, {this_uid});
    }
    SPDLOG_DEBUG("Initializing finished...");
}


MultiThreadedCPUBackend::PopulationIterator MultiThreadedCPUBackend::begin_populations()
{
    return populations_.begin();
}


MultiThreadedCPUBackend::PopulationConstIterator MultiThreadedCPUBackend::begin_populations() const
{
    return populations_.cbegin();
}


MultiThreadedCPUBackend::PopulationIterator MultiThreadedCPUBackend::end_populations()
{
    return populations_.end();
}


MultiThreadedCPUBackend::PopulationConstIterator MultiThreadedCPUBackend::end_populations() const
{
    return populations_.cend();
}


MultiThreadedCPUBackend::ProjectionIterator MultiThreadedCPUBackend::begin_projections()
{
    return projections_.begin();
}


MultiThreadedCPUBackend::ProjectionConstIterator MultiThreadedCPUBackend::begin_projections() const
{
    return projections_.cbegin();
}


MultiThreadedCPUBackend::ProjectionIterator MultiThreadedCPUBackend::end_projections()
{
    return projections_.end();
}


MultiThreadedCPUBackend::ProjectionConstIterator MultiThreadedCPUBackend::end_projections() const
{
    return projections_.cend();
}


}  // namespace knp::backends::multi_threaded_cpu
