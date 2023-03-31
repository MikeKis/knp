/**
 * @file network.h
 * @brief Network interface.
 * @author Artiom N.
 * @date 22.03.2023
 */

#pragma once

#include <knp/core/core.h>
#include <knp/core/impexp.h>
#include <knp/core/population.h>
#include <knp/core/projection.h>
#include <knp/framework/coordinates/generator.h>
#include <knp/neuron-traits/all_traits.h>
#include <knp/synapse-traits/all_traits.h>

#include <variant>
#include <vector>


/**
 * @brief Framework namespace.
 */
namespace knp::framework
{

/**
 * @brief The Network class is a neural network containing populations and projections.
 */
class Network
{
public:
    using AllPopulations = boost::mp11::mp_transform<knp::core::Population, knp::neuron_traits::AllNeurons>;
    using AllProjections = boost::mp11::mp_transform<knp::core::Projection, knp::synapse_traits::AllSynapses>;

    using AllPopulationVariants = boost::mp11::mp_rename<AllPopulations, std::variant>;
    using AllProjectionVariants = boost::mp11::mp_rename<AllProjections, std::variant>;

public:
    using PopulationContainer = std::vector<AllPopulationVariants>;
    using ProjectionContainer = std::vector<AllProjectionVariants>;

    using PopulationIterator = PopulationContainer::iterator;
    using PopulationConstIterator = PopulationContainer::const_iterator;
    using ProjectionIterator = ProjectionContainer::iterator;
    using ProjectionConstIterator = ProjectionContainer::const_iterator;

public:
    Network() = default;

public:
    /**
     * @brief add_population add population to network.
     * @param population new population.
     */
    void add_population(AllPopulationVariants &&population);
    /**
     * @copydoc add_population
     */
    template <typename PopulationType>
    void add_population(PopulationType &&population);
    /**
     * @brief get_population get population from network.
     * @param population_uid population UID.
     * @throw std::logic_error if population was not found.
     * @return existing population.
     */
    template <typename PopulationType>
    PopulationType &get_population(const knp::core::UID &population_uid);
    /**
     * @copydoc get_population
     */
    template <typename PopulationType>
    const PopulationType &get_population(const knp::core::UID &population_uid) const;
    /**
     * @brief remove_population remove population from network.
     * @param population_uid population UID.
     */
    void remove_population(const knp::core::UID &population_uid);

public:
    /**
     * @brief add_projection add projection to network.
     * @param projection new projection.
     */
    void add_projection(AllProjectionVariants &&projection);
    /**
     * @copydoc add_projection
     */
    template <typename ProjectionType>
    void add_projection(ProjectionType &&projection);
    /**
     * @brief get_projection get projection from network.
     * @param projection_uid projection UID.
     * @throw std::logic_error if projection was not found.
     * @return existing projection.
     */
    template <typename ProjectionType>
    ProjectionType &get_projection(const knp::core::UID &projection_uid);
    /**
     * @copydoc get_projection
     */
    template <typename ProjectionType>
    const ProjectionType &get_projection(const knp::core::UID &projection_uid) const;
    /**
     * @brief remove_projection remove projection from network.
     * @param projection_uid projection UID.
     */
    void remove_projection(const knp::core::UID &projection_uid);

public:
    PopulationIterator begin_populations();
    PopulationConstIterator begin_populations() const;
    PopulationIterator end_populations();
    PopulationConstIterator end_populations() const;

    ProjectionIterator begin_projections();
    ProjectionConstIterator begin_projections() const;
    ProjectionIterator end_projections();
    ProjectionConstIterator end_projections() const;

public:
    size_t populations_count() const { return populations_.size(); }
    size_t projections_count() const { return projections_.size(); }

public:
    /**
     * @brief Get the network UID.
     * @return UID.
     */
    [[nodiscard]] const knp::core::UID &get_uid() const { return base_.uid_; }
    /**
     * @brief Get tags used by the network.
     * @return tag map.
     * @see TagMap.
     */
    [[nodiscard]] auto &get_tags() { return base_.tags_; }

private:
    template <typename T, typename VT>
    typename std::vector<VT>::iterator find_elem(const knp::core::UID &uid, std::vector<VT> &container);
    template <typename Ts, typename VT>
    typename std::vector<VT>::iterator find_variant(const knp::core::UID &uid, std::vector<VT> &container);

private:
    knp::core::BaseData base_;
    PopulationContainer populations_;
    ProjectionContainer projections_;
};

}  // namespace knp::framework
