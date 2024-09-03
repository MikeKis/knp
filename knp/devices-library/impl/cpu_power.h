/**
 * @file cpu_power.h
 * @brief CPU power consumption received via Intel PCM library.
 * @author Artiom N.
 * @date 20.02.2023
 * @license Apache 2.0
 * @copyright © 2024 AO Kaspersky Lab
 */

#pragma once

#include <knp/core/impexp.h>

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wdocumentation"
#endif

#include <pcm/src/cpucounters.h>

#if defined(__clang__)
#    pragma clang diagnostic pop
#endif

#include <chrono>
#include <cinttypes>
#include <vector>


namespace knp::devices::cpu
{

void check_pcm_status(const pcm::PCM::ErrorCode &status);

/**
 * @brief Power meter via Intel PCM.
 */
class KNP_DECLSPEC CpuPower
{
public:
    explicit CpuPower(uint32_t cpu_sock_no);
    float get_power();

private:
    // cppcheck-suppress unusedStructMember
    uint32_t cpu_sock_no_;
    std::chrono::time_point<std::chrono::steady_clock> time_start_;
    pcm::PCM *pcm_instance_ = nullptr;
    // cppcheck-suppress unusedStructMember
    pcm::SocketCounterState sktstate1_, sktstate2_;
    // cppcheck-suppress unusedStructMember
    // std::vector<pcm::CoreCounterState> /* cstates1, */ cstates2_;
    // cppcheck-suppress unusedStructMember
    // pcm::SystemCounterState sstate1_, sstate2_;
};
}  // namespace knp::devices::cpu
