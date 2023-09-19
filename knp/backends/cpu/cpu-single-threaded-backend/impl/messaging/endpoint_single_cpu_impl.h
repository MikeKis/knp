//
// Created by an_vartenkov on 19.09.23.
//

#ifndef KNPRUNARNI_ENDPOINT_SINGLE_CPU_IMPL_H
#    define KNPRUNARNI_ENDPOINT_SINGLE_CPU_IMPL_H

#endif  // KNPRUNARNI_ENDPOINT_SINGLE_CPU_IMPL_H
/**
 * @file message_bus_single_cpu_impl.h
 * @brief Single-threaded CPU message bus implementation header.
 * @author Vartenkov A.
 * @date 18.09.2023
 */
#pragma once

#include <knp/core/message_endpoint.h>

#include <algorithm>
#include <utility>
#include <vector>

/**
 * @brief Namespace for single-threaded backend.
 */
namespace knp::backends::single_threaded_cpu
{

class MessageEndpointSingleCPUImpl : public core::MessageEndpointImpl
{
public:
    void send_message(const knp::core::messaging::MessageVariant &message) override
    {
        messages_to_send_.push_back(message);
    };

    ~MessageEndpointSingleCPUImpl() = default;

    /**
     * @brief Reads all the messages queued to be sent, then clears message container.
     * @return a vector of messages to be sent to other endpoints.
     */
    [[nodiscard]] std::vector<knp::core::messaging::MessageVariant> get_sent_messages()
    {
        auto result = std::move(messages_to_send_);
        messages_to_send_.clear();
        return result;
    }

    // Embarrassingly inefficient: all endpoints basically receive all messages by copying them. Should do better.
    void add_received_messages(const std::vector<knp::core::messaging::MessageVariant> &incoming_messages)
    {
        if (received_messages_.capacity() < received_messages_.size() + incoming_messages.size())
            received_messages_.reserve(std::max(2 * received_messages_.size(), 2 * incoming_messages.size()));

        received_messages_.insert(received_messages_.end(), incoming_messages.begin(), incoming_messages.end());
    }

    void add_received_message(knp::core::messaging::MessageVariant &&incoming)
    {
        received_messages_.emplace_back(incoming);
    }

    void add_received_message(const knp::core::messaging::MessageVariant &incoming)
    {
        received_messages_.push_back(incoming);
    }

    std::optional<knp::core::messaging::MessageVariant> receive_message() override
    {
        if (received_messages_.empty()) return {};
        auto result = std::move(received_messages_.back());
        received_messages_.pop_back();
        return result;
    }

private:
    std::vector<knp::core::messaging::MessageVariant> messages_to_send_;
    std::vector<knp::core::messaging::MessageVariant> received_messages_;
};

}  // namespace knp::backends::single_threaded_cpu
