/**
 * @file message_bus_impl.h
 * @brief Message bus ZeroMQ implementation header.
 * @author Artiom N.
 * @date 31.03.2023
 */

#pragma once

#include <knp/core/message_bus.h>

#include <spdlog/spdlog.h>

#include <string>

#include <zmq.hpp>


namespace knp::core
{

/**
 * @brief internal message bus class, not intended for user code.
 */
class MessageBus::MessageBusImpl
{
public:
    MessageBusImpl();

    /**
     * @brief send a message from one socket to another.
     */
    bool step();

    /**
     * @brief Creates an endpoint that can be used for message exchange.
     * @return a new endpoint.
     */
    [[nodiscard]] MessageEndpoint create_endpoint();

private:
    /**
     * @brief Router socket address.
     */
    std::string router_sock_address_;

    /**
     * @brief Publish socket address.
     */
    std::string publish_sock_address_;

    /**
     * @brief Messaging context.
     */
    zmq::context_t context_;

    /**
     * @brief Router socket.
     */
    zmq::socket_t router_socket_;

    /**
     * @brief Publish socket.
     */
    zmq::socket_t publish_socket_;
};


}  // namespace knp::core