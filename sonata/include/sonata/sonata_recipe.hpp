#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <any>

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

#include <arborenv/concurrency.hpp>
#include <arborenv/gpu_env.hpp>

#include "sonata_io.hpp"
#include "sonata_cell.hpp"
#include "data_management_lib.hpp"

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

namespace sonata {
class sonata_recipe: public arb::recipe {
public:
    arb::cable_cell_global_properties gprop;
    sonata_recipe(sonata_params params):
            model_desc_(params.network.nodes,
                       params.network.edges,
                       params.network.nodes_types,
                       params.network.edges_types),
            io_desc_(params.network.nodes,
                        params.spikes_input,
                        params.current_clamps,
                        params.probes_info),
            run_params_(params.run),
            sim_cond_(params.conditions),
            probe_info_(params.probes_info),
            num_cells_(model_desc_.num_cells()) {
                gprop.default_parameters = arb::neuron_parameter_defaults;
                gprop.default_parameters.axial_resistivity = 100;
                gprop.default_parameters.temperature_K = sim_cond_.temp_c + 273.15;
                gprop.default_parameters.init_membrane_potential = sim_cond_.v_init;
                gprop.default_parameters.reversal_potential_method["k"] = "nernst/k";
                gprop.default_parameters.reversal_potential_method["na"] = "nernst/na";
            }

    cell_size_type num_cells() const override {
        return num_cells_;
    }

    void build_local_maps(const arb::domain_decomposition& decomp) {
        std::lock_guard<std::mutex> l(mtx_);
        model_desc_.build_source_and_target_maps(decomp.groups());
    }

    arb::util::unique_any get_cell_description(cell_gid_type gid) const override {
        if (get_cell_kind(gid) == cell_kind::cable) {
            std::vector<arb::mlocation> src_locs;
            std::vector<std::pair<arb::mlocation, arb::mechanism_desc>> tgt_types;

            std::lock_guard<std::mutex> l(mtx_);
            auto morph = model_desc_.get_cell_morphology(gid);
            auto mechs = model_desc_.get_density_mechs(gid);

            model_desc_.get_sources_and_targets(gid, src_locs, tgt_types);

            std::vector<std::pair<arb::mlocation, double>> src_types;
            for (auto s: src_locs) {
                src_types.push_back(std::make_pair(s, run_params_.threshold));
            }

            auto decor = arb::decor();

            auto stims = io_desc_.get_current_clamps(gid);
            // for (auto s: stims) {
            for (int i=0; i < stims.size(); i++) {
                auto s = stims[i];
                arb::i_clamp stim(s.delay, s.duration, s.amplitude);
                decor.place(s.stim_loc, stim, std::string{"i_clamp"} + std::to_string(i));
            }

            return sonata_cell(gprop.catalogue, decor, morph, mechs, src_types, tgt_types);
        }
        else if (get_cell_kind(gid) == cell_kind::spike_source) {
            std::lock_guard<std::mutex> l(mtx_);
            std::vector<double> time_sequence = io_desc_.get_spikes(gid);
            return arb::util::unique_any(arb::spike_source_cell{"det@0",arb::explicit_schedule(time_sequence)});
        }
        return {};
    }

    cell_kind get_cell_kind(cell_gid_type gid) const override {
        std::lock_guard<std::mutex> l(mtx_);
        return model_desc_.get_cell_kind(gid);
    }

    // cell_size_type num_sources(cell_gid_type gid) const override {
    //     std::lock_guard<std::mutex> l(mtx_);
    //     return model_desc_.num_sources(gid);
    // }

    // cell_size_type num_targets(cell_gid_type gid) const override {
    //     std::lock_guard<std::mutex> l(mtx_);
    //     return model_desc_.num_targets(gid);
    // }

    std::vector<arb::cell_connection> connections_on(cell_gid_type gid) const override {
        std::vector<arb::cell_connection> conns;

        std::lock_guard<std::mutex> l(mtx_);
        model_desc_.get_connections(gid, conns);

        for (const auto& conn : conns){
            std::cout << gid << " " << conn.target.tag << " " << conn.source.label.tag << " " << conn.source.gid << " " << "\n";
        }

        return conns;
    }

    std::vector<arb::event_generator> event_generators(cell_gid_type gid) const override {
        std::vector<arb::event_generator> gens;
        return gens;
    }

    std::vector<trace_index_and_info> get_probes_info(cell_gid_type gid) const {
        return io_desc_.get_probes(gid);
    }

    std::vector<arb::probe_info> get_probes(cell_gid_type gid) const override {
        std::vector<arb::probe_info> probes;
        std::vector<trace_index_and_info> pbs = io_desc_.get_probes(gid);

        for (auto p: pbs) {
            if (p.info.is_voltage) {
                probes.push_back(arb::probe_info{arb::cable_probe_membrane_voltage{p.info.loc}});
            } else {
                probes.push_back(arb::probe_info{arb::cable_probe_axial_current{p.info.loc}});
            }
        }
        return probes;
    }

    std::unordered_map<std::string, std::vector<cell_member_type>> get_probe_groups() {
        return io_desc_.get_probe_groups();
    }

    std::any get_global_properties(cell_kind k) const override {
        return gprop;
    }

    std::vector<unsigned> get_pop_partitions() const {
        return model_desc_.pop_partitions();
    }

    std::vector<std::string> get_pop_names() const {
        return model_desc_.pop_names();
    }

private:
    mutable std::mutex mtx_;
    mutable model_desc model_desc_;
    mutable io_desc io_desc_;

    run_params run_params_;
    sim_conditions sim_cond_;
    std::vector<probe_info> probe_info_;

    cell_size_type num_cells_;
};
} // namespace sonata