/**
 * @file resource_stdp_blifat.cpp
 * @brief Python bindings for resource stdp BLIFAT neuron.
 * @kaspersky_developer Vartenkov A.
 * @date 28.10.24
 * @license Apache 2.0
 * @copyright © 2024 AO Kaspersky Lab
 */


#include <knp/neuron-traits/all_traits.h>
using rsbn_params = knp::neuron_traits::neuron_parameters<knp::neuron_traits::SynapticResourceSTDPBLIFATNeuron>;

#if defined(_KNP_IN_NEURON_TRAITS)

#    include "common.h"

py::class_<rsbn_params, py::bases<bn_params>>(
    "SynapticResourceSTDPBLIFATNeuronParameters", "Structure for STDP Resource BLIFAT neuron parameters", py::no_init);

#endif
