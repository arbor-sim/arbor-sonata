#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>

#include <arbor/assert_macro.hpp>
#include <arbor/common_types.hpp>
#include <arbor/context.hpp>
#include <arbor/load_balance.hpp>
#include <arbor/cable_cell.hpp>
#include <arbor/spike_source_cell.hpp>
#include <arbor/profile/meter_manager.hpp>
#include <arbor/profile/profiler.hpp>
#include <arbor/simple_sampler.hpp>
#include <arbor/simulation.hpp>
#include <arbor/recipe.hpp>
#include <arbor/version.hpp>

// #include <arbor/cable_cell.hpp>
// #include <arbor/cable_cell_param.hpp>
#include <arborio/label_parse.hpp>

#include <arborenv/concurrency.hpp>
#include <arborenv/gpu_env.hpp>

#include <sonata/sonata_io.hpp>
#include <sonata/data_management_lib.hpp>
#include <sonata/sonata_cell.hpp>

#ifdef ARB_MPI_ENABLED
#include <mpi.h>
#include <arborenv/with_mpi.hpp>
#endif

using arb::cell_gid_type;
using arb::cell_lid_type;
using arb::cell_size_type;
using arb::cell_member_type;
using arb::cell_kind;
using arb::time_type;

using namespace arborio::literals;

namespace sonata {
// Generate a cell.
arb::cable_cell sonata_cell(
        arb::decor dec,
        arb::morphology morph,
        std::unordered_map<section_kind, std::vector<arb::mechanism_desc>> mechs,
        std::vector<std::pair<arb::mlocation, double>> detectors,
        std::vector<std::pair<arb::mlocation, arb::mechanism_desc>> synapses) {
    arb::label_dict ld;
    // arb::decor dec;

    using arb::reg::tagged;
    ld.set("soma", tagged(1));
    ld.set("axon", tagged(2));
    ld.set("dend", join(tagged(3), tagged(4)));

    for (auto mech: mechs[section_kind::soma]) {
        dec.paint("soma"_lab, arb::density(mech));
    }
    for (auto mech: mechs[section_kind::dend]) {
        dec.paint("dend"_lab, arb::density(mech));
    }
    for (auto mech: mechs[section_kind::axon]) {
        dec.paint("axon"_lab, arb::density(mech));
    }

    // Add spike threshold detector at the soma.
    // for (const auto& [k,v]: detectors) {
    for (int i=0; i < detectors.size(); i++) {
        dec.place(detectors[i].first, arb::threshold_detector{detectors[i].second}, std::string{"detector@"}+std::to_string(i));
    }

    // for (const auto& [k,v]: synapses) {
    for (int i=0; i < synapses.size(); i++) {
        dec.place(synapses[i].first, arb::synapse(synapses[i].second), std::string{"synapse@"}+std::to_string(i)); //,cell_tag_type
    }

    dec.set_default(arb::cv_policy_fixed_per_branch(200));

    arb::cable_cell cell = arb::cable_cell(morph, dec, ld);

    return cell;
}
} // namespace sonata