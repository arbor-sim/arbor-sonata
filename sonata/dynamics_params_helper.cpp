#include <iostream>
#include <unordered_map>

#include <sonata/json/json_params.hpp>
#include <sonata/sonata_exceptions.hpp>
#include <sonata/density_mech_helper.hpp>

namespace sonata{
arb::mechanism_desc read_dynamics_params_point(std::string fname) {
    auto json = sup::read_json_file(fname);
    auto mechs = json.get<std::unordered_map<std::string, nlohmann::json>>();

    if (mechs.size() > 1) {
        throw sonata_file_exception("Point mechanism description must contains only one mechanism, file: ", fname);
    }

    std::string mech_name = mechs.begin()->first;
    nlohmann::json mech_params = mechs.begin()->second;

    auto syn = arb::mechanism_desc(mech_name);
    auto params = mech_params.get<std::unordered_map<std::string, double>>();
    for (auto p: params) {
        syn.set(p.first, p.second);
    }

    return syn;
}

std::unordered_map<std::string, mech_groups> read_dynamics_params_density_base(std::string fname) {
    auto json = sup::read_json_file(fname);

    auto mech_definitions = json.get<std::unordered_map<std::string, nlohmann::json>>();

    std::unordered_map<std::string, mech_groups> mech_map;

    for (const auto& [mech_id, mech_features]: mech_definitions) {
        std::unordered_map<std::string, double> variables; // key -value pairs, can be overwritten
        std::vector<mech_params> mech_details;

        for (const auto& params: mech_features) { //iterate through json
            for (auto it = params.begin(); it != params.end(); ++it) {
                if(!it->is_structured()) {
                    variables[it.key()] = it.value();
                } else {
                    // Mechanism instance
                    std::string section_name;
                    std::string mech_name;
                    std::unordered_map<std::string, double> mech_params;
                    std::unordered_map<std::string, std::string> mech_aliases;

                    for (auto mech_it = it->begin(); mech_it != it->end(); mech_it++) {
                        if (mech_it.key() == "section") {
                            section_name = (mech_it.value()).get<std::string>();
                        } else if (mech_it.key() == "mech") {
                            mech_name = (mech_it.value()).get<std::string>();
                        } else if ((*mech_it).type() == nlohmann::json::value_t::string) {
                            mech_aliases[mech_it.key()] = (mech_it.value()).get<std::string>();
                        } else if (mech_it->type() == nlohmann::json::value_t::number_float
                                || mech_it->type() == nlohmann::json::value_t::number_unsigned
                                || mech_it->type() == nlohmann::json::value_t::number_integer) {
                            mech_params[mech_it.key()] = (mech_it.value()).get<double>();
                        } else {
                            throw std::runtime_error{"While reading mechanism parameters: No idea how to handle entry. id=" + mech_id
                                                   + " key=" + mech_it.key()};
                        }
                    }

                    auto base = arb::mechanism_desc(mech_name);
                    for (const auto& [k, v]: mech_params) base.set(k, v);

                    mech_details.emplace_back(section_name, mech_aliases, base);
                }
            }
        }
        mech_map.insert({mech_id, mech_groups(variables, mech_details)});
    }
    return mech_map;
}

std::unordered_map<std::string, variable_map> read_dynamics_params_density_override(std::string fname) {
    auto json = sup::read_json_file(fname);
    auto mech_overrides = json.get<std::unordered_map<std::string, nlohmann::json>>();

    std::unordered_map<std::string, variable_map> var_overrides;

    for (auto mech_def: mech_overrides) {
        std::string mech_id = mech_def.first; //key
        variable_map mech_variables = (mech_def.second).get<variable_map>();

        var_overrides.insert({mech_id, mech_variables});
    }
    return var_overrides;
}
} // namespace sonata
