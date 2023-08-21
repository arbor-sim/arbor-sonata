#include "../gtest.h"

#include <sonata/dynamics_params_helper.hpp>

using namespace sonata;

TEST(dynamics_params_helper, point_mech) {
    std::string datadir{DATADIR};
    std::string expsyn_file = datadir + "/expsyn.json";
    std::string exp2syn_file = datadir + "/exp2syn.json";

    auto expsyn = read_dynamics_params_point(expsyn_file);

    EXPECT_EQ("expsyn", expsyn.name());
    EXPECT_EQ(0, expsyn.values().at("e"));
    EXPECT_EQ(2.0, expsyn.values().at("tau"));

    auto exp2syn = read_dynamics_params_point(exp2syn_file);

    EXPECT_EQ("exp2syn", exp2syn.name());
    EXPECT_EQ(0.1, exp2syn.values().at("e"));
    EXPECT_EQ(0.5, exp2syn.values().at("tau1"));
    EXPECT_EQ(0.6, exp2syn.values().at("tau2"));
}

TEST(dynamics_params_helper, density_mech) {
    std::string datadir{DATADIR};
    std::string pas_hh_file = datadir + "/set_pas.json";

    auto pas_hh = read_dynamics_params_density_base(pas_hh_file);

    // Check we found two correctly name mechanisms
    EXPECT_EQ(2, pas_hh.size());
    EXPECT_TRUE(pas_hh.count("pas_0"));
    EXPECT_TRUE(pas_hh.count("hh_0"));

    // Check the parameters are to spec
    const auto& pas_0_group = pas_hh.at("pas_0");
    EXPECT_EQ(2, pas_0_group.variables.size());

    EXPECT_TRUE(pas_0_group.variables.count("e_pas"));
    EXPECT_TRUE(pas_0_group.variables.count("g_pas"));

    EXPECT_EQ(0.002, pas_0_group.variables.at("g_pas"));
    EXPECT_EQ(-70, pas_0_group.variables.at("e_pas"));

    // Now recurse into the sub-sections
    // First, look at pas
    auto pas_0_details = pas_0_group.mech_details;
    EXPECT_EQ(2, pas_0_details.size());

    auto pas_soma = std::find_if(pas_0_details.begin(), pas_0_details.end(),
                                 [](const auto& d) { return d.section == section_kind::soma; });
    EXPECT_NE(pas_soma, pas_0_details.end());
    auto pas_dend = std::find_if(pas_0_details.begin(), pas_0_details.end(),
                                 [](const auto& d) { return d.section == section_kind::dend; });
    EXPECT_NE(pas_dend, pas_0_details.end());

    EXPECT_EQ(section_kind::soma, pas_soma->section);
    {
        EXPECT_EQ("pas", pas_soma->mech.name());
        EXPECT_EQ(0, pas_soma->param_alias.size());
        const auto& values = pas_soma->mech.values();
        EXPECT_EQ(2, values.size());
        EXPECT_TRUE(values.count("e"));
        EXPECT_TRUE(values.count("g"));
        EXPECT_EQ(-65, values.at("e"));
        EXPECT_EQ(0, values.at("g"));
    }

    EXPECT_EQ(section_kind::dend, pas_dend->section);
    {
        EXPECT_EQ(1, pas_dend->param_alias.size());
        EXPECT_TRUE(pas_dend->param_alias.count("e"));
        EXPECT_EQ("e_pas", pas_dend->param_alias.at("e"));
        EXPECT_EQ("pas", pas_dend->mech.name());
        EXPECT_EQ(1, pas_dend->mech.values().size());
        EXPECT_EQ(0.001, pas_dend->mech.values().at("g"));
    }
    // then HH
    auto hh_0_group = pas_hh.at("hh_0");
    EXPECT_EQ(2, hh_0_group.variables.size());

    EXPECT_TRUE(hh_0_group.variables.count("el_hh"));
    EXPECT_TRUE(hh_0_group.variables.count("gl_hh"));

    EXPECT_EQ(0.002, hh_0_group.variables.at("gl_hh"));
    EXPECT_EQ(-54, hh_0_group.variables.at("el_hh"));

    auto hh_0_details = hh_0_group.mech_details;
    EXPECT_EQ(1, hh_0_details.size());

    auto hh_soma = hh_0_details[0];
    EXPECT_EQ(section_kind::soma, hh_soma.section);
    EXPECT_EQ(2, hh_soma.param_alias.size());
    EXPECT_TRUE(hh_soma.param_alias.count("el"));
    EXPECT_EQ("el_hh", hh_soma.param_alias.at("el"));
    EXPECT_TRUE(hh_soma.param_alias.count("gl"));
    EXPECT_EQ("gl_hh", hh_soma.param_alias.at("gl"));
    EXPECT_EQ("hh", hh_soma.mech.name());
    EXPECT_EQ(0, hh_soma.mech.values().size());
}

TEST(dynamics_params_helper, override_density_mech) {
    std::string datadir{DATADIR};
    std::string pas_file = datadir + "/pas.json";
    std::string pas_hh_file = datadir + "/pas_hh.json";

    auto pas_override = read_dynamics_params_density_override(pas_file);
    auto pas_hh_override = read_dynamics_params_density_override(pas_hh_file);

    EXPECT_EQ(1, pas_override.size());
    EXPECT_TRUE(pas_override.find("pas_0") != pas_override.end());
    EXPECT_EQ(1, pas_override.at("pas_0").size());
    EXPECT_EQ(0.001, pas_override.at("pas_0").at("g_pas"));

    EXPECT_EQ(2, pas_hh_override.size());
    EXPECT_TRUE(pas_hh_override.find("pas_0") != pas_hh_override.end());
    EXPECT_EQ(2, pas_hh_override.at("pas_0").size());
    EXPECT_EQ(0.001, pas_hh_override.at("pas_0").at("g_pas"));
    EXPECT_EQ(-65, pas_hh_override.at("pas_0").at("e_pas"));

    EXPECT_TRUE(pas_hh_override.find("hh_0") != pas_hh_override.end());
    EXPECT_EQ(1, pas_hh_override.at("hh_0").size());
    EXPECT_EQ(0.003, pas_hh_override.at("hh_0").at("gl_hh"));
}
