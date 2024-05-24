/**
 * @file data_storage_common.cpp
 * @brief File for common functions for saving and loading data.
 * @author An. Vartenkov
 * @date 24.04.2024
 */

#include "data_storage_common.h"

#include <knp/core/messaging/messaging.h>

#include <unordered_map>
#include <utility>
#include <vector>


namespace knp::framework::storage::native
{
std::vector<knp::core::messaging::SpikeMessage> convert_node_time_arrays_to_messages(
    const std::vector<int64_t> &nodes, const std::vector<float> &timestamps, const knp::core::UID &uid,
    float time_per_step)
{
    if (nodes.size() != timestamps.size()) throw std::runtime_error("Different array sizes: nodes and timestamps.");
    using Message = knp::core::messaging::SpikeMessage;
    std::unordered_map<size_t, Message> message_map;  // This is a result buffer.
    for (size_t i = 0; i < timestamps.size(); ++i)
    {
        auto step = static_cast<knp::core::Step>(timestamps[i] / time_per_step);
        auto index = nodes[i];
        auto iterator = message_map.find(step);
        if (iterator == message_map.end())
        {
            Message message{{uid, step}, std::vector<knp::core::messaging::SpikeIndex>(1, index)};
            message_map.insert({step, std::move(message)});
        }
        else
            iterator->second.neuron_indexes_.push_back(index);
    }

    // Moving messages from map to vector, O(N_messages).
    std::vector<Message> result;
    result.reserve(message_map.size());
    std::transform(
        message_map.begin(), message_map.end(), std::back_inserter(result),
        [](auto &val) { return std::move(val.second); });

    // Sort message vector by message step.
    std::sort(
        result.begin(), result.end(),
        [](const Message &msg1, const Message &msg2) { return msg1.header_.send_time_ < msg2.header_.send_time_; });
    return result;
}
}  // namespace knp::framework::storage::native