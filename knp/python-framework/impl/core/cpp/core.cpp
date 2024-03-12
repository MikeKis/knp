/**
 * @file core.cpp
 * @brief Python core bindings.
 * @author Artiom N.
 * @date 01.02.2024
 */

#include <knp/core/core.h>

#include <filesystem>

#include "any_converter.h"
#include "common.h"
#include "message_endpoint.h"
#include "optional_converter.h"
#include "population.h"
#include "projection.h"
#include "spike_message.h"
#include "tuple_converter.h"
#include "uid.h"

// knp::neuron_traits::neuron_parameters<knp::neuron_traits::BLIFATNeuron>

// py::object my_tf(size_t index)
//{
//     std::cout << "Index ok = " << index << std::endl;
//     return py::object();
// }

std::string get_py_class_name(const py::object &obj_class)
{
    const std::string class_name = boost::python::extract<std::string>(obj_class.attr("__class__").attr("__name__"));
    if (class_name != "class")
    {
        PyErr_SetString(PyExc_TypeError, "Passed object is not a class!");
        py::throw_error_already_set();
        throw std::runtime_error("Not a class");
    }

    return boost::python::extract<std::string>(obj_class.attr("__name__"));
}


BOOST_PYTHON_MODULE(KNP_FULL_LIBRARY_NAME)
{
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));

    //    py::to_python_converter<std::any, to_python_any>();
    //    from_python_any();
    py::class_<core::TagMap>("TagMap", "The TagMap class is a definition of tags used by entity and their values.")
        //        .def("get_tag", static_cast<std::any&(core::TagMap::*)(const std::string&)>(&core::TagMap::get_tag),
        //             "Get tag value by tag name.")
        ;  // NOLINT

    py::class_<core::BaseData>("BaseData", "Common parameters for several different entities.");

    py::implicitly_convertible<core::UID, boost::uuids::uuid>();

    // py::to_python_converter<std::optional<int>, to_python_optional<int>>();
    //    Py_Initialize();

    // Need for import.
    //    PyObject* sysPath = PySys_GetObject("path");
    //    PyList_Insert(sysPath, 0, PyUnicode_FromString(absolute(std::filesystem::current_path()).string().c_str()));
    //
    //    boost::python::import("libknp_python_framework_neuron_traits");

#define _KNP_IN_CORE
#include "backend.cpp"                  // NOLINT
#include "device.cpp"                   // NOLINT
#include "message_bus.cpp"              // NOLINT
#include "message_endpoint.cpp"         // NOLINT
#include "message_header.cpp"           // NOLINT
#include "messaging.cpp"                // NOLINT
#include "population.cpp"               // NOLINT
#include "projection.cpp"               // NOLINT
#include "spike_message.cpp"            // NOLINT
#include "subscription.cpp"             // NOLINT
#include "synaptic_impact_message.cpp"  // NOLINT
#include "uid.cpp"                      // NOLINT
#undef _KNP_IN_CORE
}