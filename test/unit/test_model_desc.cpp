#include "../gtest.h"

#include <arbor/cable_cell.hpp>

#include <sonata/hdf5_lib.hpp>
#include <sonata/csv_lib.hpp>
#include <sonata/data_management_lib.hpp>
#include <sonata/sonata_exceptions.hpp>

// pop_ext         n5
//           ______|_______
//          |       _______|________
//          |      |       |        |
//          v      |       v        v
//  pop_e:  n0     n1      n2       n3
//          |      ^       |
//          |      |       |
//          |      |       |
//  pop_i:  |____> n4 <____|

using namespace sonata;

model_desc simple_network() {
    std::string datadir{DATADIR};

    auto nodes0 = datadir + "/nodes_0.h5";
    auto nodes1 = datadir + "/nodes_1.h5";
    auto nodes2 = datadir + "/nodes_2.h5";

    auto n0 = std::make_shared<h5_file>(nodes0);
    auto n1 = std::make_shared<h5_file>(nodes1);
    auto n2 = std::make_shared<h5_file>(nodes2);

    h5_record nodes({n0, n1, n2});


    auto edges0 = datadir + "/edges_0.h5";
    auto edges1 = datadir + "/edges_1.h5";
    auto edges2 = datadir + "/edges_2.h5";
    auto edges3 = datadir + "/edges_3.h5";

    auto e0 = std::make_shared<h5_file>(edges0);
    auto e1 = std::make_shared<h5_file>(edges1);
    auto e2 = std::make_shared<h5_file>(edges2);
    auto e3 = std::make_shared<h5_file>(edges3);

    h5_record edges({e0, e1, e2, e3});

    auto edges4 = datadir + "/edges.csv";
    auto ef = csv_file(edges4);
    auto e_base = csv_edge_record({ef});

    auto nodes3 = datadir + "/nodes.csv";
    auto nf = csv_file(nodes3);
    auto n_base = csv_node_record({nf});

    model_desc md(nodes, edges, n_base, e_base);

    return std::move(md);
}

TEST(model_desc, helper_functions) {

    auto md = simple_network();

    EXPECT_EQ(6, md.num_cells());

    EXPECT_EQ(std::vector<unsigned>({0,4,5,6}), md.pop_partitions());
    EXPECT_EQ(std::vector<std::string>({"pop_e", "pop_i" ,"pop_ext"}), md.pop_names());

    EXPECT_EQ("pop_e", md.population_of(0));
    EXPECT_EQ("pop_e", md.population_of(1));
    EXPECT_EQ("pop_e", md.population_of(2));
    EXPECT_EQ("pop_e", md.population_of(3));
    EXPECT_EQ("pop_i", md.population_of(4));
    EXPECT_EQ("pop_ext", md.population_of(5));

    EXPECT_EQ(0, md.population_id_of(0));
    EXPECT_EQ(1, md.population_id_of(1));
    EXPECT_EQ(2, md.population_id_of(2));
    EXPECT_EQ(3, md.population_id_of(3));
    EXPECT_EQ(0, md.population_id_of(4));
    EXPECT_EQ(0, md.population_id_of(5));
}

TEST(model_desc, source_target_maps) {
    auto md = simple_network();

    auto verify_src_tgt = [&md](const std::vector<arb::group_description>& decomp) {
        md.build_source_and_target_maps(decomp);

        std::vector<arb::mlocation> srcs;
        std::vector<std::pair<arb::mlocation, arb::mechanism_desc>> tgts;
        md.get_sources_and_targets(0, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(1, srcs[0].branch);
        EXPECT_NEAR(0.3, srcs[0].pos, 1e-5);

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(0, tgts[0].first.branch);
        EXPECT_NEAR(0, tgts[0].first.pos, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());

        srcs.clear();
        tgts.clear();
        md.get_sources_and_targets(1, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(1, srcs[0].branch);
        EXPECT_NEAR(0.2, srcs[0].pos, 1e-5);

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(0, tgts[0].first.branch);
        EXPECT_NEAR(0.5, tgts[0].first.pos, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());
        EXPECT_NEAR(0.51, tgts[0].second.values().at("e"), 1e-5);


        srcs.clear();
        tgts.clear();
        md.get_sources_and_targets(2, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(3, srcs[0].branch);
        EXPECT_NEAR(0.2, srcs[0].pos, 1e-5);

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(0, tgts[0].first.branch);
        EXPECT_NEAR(0, tgts[0].first.pos, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());

        srcs.clear();
        tgts.clear();
        md.get_sources_and_targets(3, srcs, tgts);

        EXPECT_EQ(0, srcs.size());

        EXPECT_EQ(1, tgts.size());
        EXPECT_EQ(5, tgts[0].first.branch);
        EXPECT_NEAR(0.6, tgts[0].first.pos, 1e-5);
        EXPECT_EQ("expsyn", tgts[0].second.name());

        srcs.clear();
        tgts.clear();
        md.get_sources_and_targets(4, srcs, tgts);

        EXPECT_EQ(1, srcs.size());
        EXPECT_EQ(0, srcs[0].branch);
        EXPECT_NEAR(0.9, srcs[0].pos, 1e-5);

        EXPECT_EQ(2, tgts.size());
        EXPECT_EQ(0, tgts[0].first.branch);
        EXPECT_NEAR(0.4, tgts[0].first.pos, 1e-5);
        EXPECT_EQ("exp2syn", tgts[0].second.name());
        EXPECT_EQ(2, tgts[1].first.branch);
        EXPECT_NEAR(0.1, tgts[1].first.pos, 1e-5);
        EXPECT_EQ("exp2syn", tgts[1].second.name());
    };

    auto decomp = arb::group_description(arb::cell_kind::cable, {0,1,2,3,4,5}, arb::backend_kind::multicore);
    verify_src_tgt({decomp});
}

TEST(model_desc, connections) {
    auto md = simple_network();

    auto decomp = arb::group_description(arb::cell_kind::cable, {0,1,2,3,4,5}, arb::backend_kind::multicore);
    md.build_source_and_target_maps({decomp});

    std::vector<arb::cell_connection> conns;
    md.get_connections(0, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(5,conns[0].source.gid);
    EXPECT_EQ("detector@"+arb::cell_tag_type{"0"},conns[0].source.label.tag);
    // EXPECT_EQ(0,conns[0].dest.gid);
    EXPECT_EQ("synapse@"+arb::cell_tag_type{"0"},conns[0].target.tag);
    EXPECT_NEAR(0.01,conns[0].weight,1e-5);
    EXPECT_NEAR(0.1,conns[0].delay,1e-5);

    conns.clear();
    md.get_connections(1, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(4,conns[0].source.gid);
    EXPECT_EQ("detector@"+arb::cell_tag_type{"0"},conns[0].source.label.tag);
    // EXPECT_EQ(1,conns[0].dest.gid);
    EXPECT_EQ("synapse@"+arb::cell_tag_type{"0"},conns[0].target.tag);
    EXPECT_NEAR(-0.02,conns[0].weight,1e-5);
    EXPECT_NEAR(0.1,conns[0].delay,1e-5);

    conns.clear();
    md.get_connections(2, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(5,conns[0].source.gid);
    EXPECT_EQ("detector@"+arb::cell_tag_type{"0"},conns[0].source.label.tag);
    // EXPECT_EQ(2,conns[0].dest.gid);
    EXPECT_EQ("synapse@"+arb::cell_tag_type{"0"},conns[0].target.tag);
    EXPECT_NEAR(0.01,conns[0].weight,1e-5);
    EXPECT_NEAR(0.1,conns[0].delay,1e-5);

    conns.clear();
    md.get_connections(3, conns);
    EXPECT_EQ(1, conns.size());
    EXPECT_EQ(1,conns[0].source.gid);
    EXPECT_EQ("detector@"+arb::cell_tag_type{"0"},conns[0].source.label.tag);
    // EXPECT_EQ(3,conns[0].dest.gid);
    EXPECT_EQ("synapse@"+arb::cell_tag_type{"0"},conns[0].target.tag);
    EXPECT_NEAR(0.05,conns[0].weight,1e-5);
    EXPECT_NEAR(0.2,conns[0].delay,1e-5);

    conns.clear();
    md.get_connections(4, conns);
    EXPECT_EQ(2, conns.size());
    EXPECT_EQ(0,conns[0].source.gid);
    EXPECT_EQ("detector@"+arb::cell_tag_type{"0"},conns[0].source.label.tag);
    // EXPECT_EQ(4,conns[0].dest.gid);
    EXPECT_EQ("synapse@"+arb::cell_tag_type{"0"},conns[0].target.tag);
    EXPECT_NEAR(0.0235,conns[0].weight,1e-5);
    EXPECT_NEAR(0.3,conns[0].delay,1e-5);

    EXPECT_EQ(2,conns[1].source.gid);
    EXPECT_EQ("detector@"+arb::cell_tag_type{"0"},conns[1].source.label.tag);
    // EXPECT_EQ(4,conns[1].dest.gid);
    EXPECT_EQ("synapse@"+arb::cell_tag_type{"1"},conns[1].target.tag);
    EXPECT_NEAR(0.04,conns[1].weight,1e-5);
    EXPECT_NEAR(0.3,conns[1].delay,1e-5);

}

/*TEST(model_desc, morphologies) {
    auto md = simple_network();

    for (auto i = 0; i < 4; i++) {
        auto morph = md.get_cell_morphology(i);
        EXPECT_DOUBLE_EQ(6.30785, morph.soma.r);
        EXPECT_EQ(1, morph.num_branches());
    }

    auto morph = md.get_cell_morphology(4);
    EXPECT_DOUBLE_EQ(3.5, morph.soma.r);
    EXPECT_EQ(0, morph.num_branches());

    EXPECT_THROW(md.get_cell_morphology(5), sonata_exception);
}*/

TEST(model_desc, cell_kinds) {
    auto md = simple_network();

    for (unsigned i = 0; i < 5; i++) {
        auto kind = md.get_cell_kind(i);
        EXPECT_EQ(arb::cell_kind::cable, kind);
    }
    auto kind = md.get_cell_kind(5);
    EXPECT_EQ(arb::cell_kind::spike_source, kind);
}

TEST(model_desc, density_mechs) {
    auto md = simple_network();

    for (unsigned i = 0; i < 5; i++) {
        auto mechs_per_sec = md.get_density_mechs(i);
        EXPECT_EQ(2, mechs_per_sec.size());
        EXPECT_TRUE(mechs_per_sec.find(section_kind::soma) != mechs_per_sec.end());
        EXPECT_TRUE(mechs_per_sec.find(section_kind::dend) != mechs_per_sec.end());

        auto mech_soma = mechs_per_sec.at(section_kind::soma);
        auto mech_dend = mechs_per_sec.at(section_kind::dend);

        EXPECT_EQ(2, mech_soma.size());
        EXPECT_EQ(1, mech_dend.size());

        auto hh_soma = std::find_if(mech_soma.begin(), mech_soma.end(),
                                    [](auto& m) { return m.name() == "hh"; });
        auto pas_soma = std::find_if(mech_soma.begin(), mech_soma.end(),
                                     [](auto& m) { return m.name() == "pas"; });

        EXPECT_NE(hh_soma, mech_soma.end());
        EXPECT_NE(pas_soma, mech_soma.end());

        auto pas_dend = std::find_if(mech_dend.begin(), mech_dend.end(),
                                     [](auto& m) { return m.name() == "pas"; });

        EXPECT_NE(pas_dend, mech_dend.end());

        if (i < 4) {
            EXPECT_NEAR(0.0003, hh_soma->values().at("gl"), 1e-5);
            EXPECT_NEAR(-54.3, hh_soma->values().at("el"), 1e-5);
            EXPECT_NEAR(0, pas_soma->values().at("g"), 1e-5);
            EXPECT_NEAR(-65, pas_soma->values().at("e"), 1e-5);

            EXPECT_NEAR(0.001, pas_dend->values().at("g"), 1e-5);
            EXPECT_NEAR(-65.1, pas_dend->values().at("e"), 1e-5);
        } else {
            EXPECT_NEAR(0.003, hh_soma->values().at("gl"), 1e-5);
            EXPECT_NEAR(-54, hh_soma->values().at("el"), 1e-5);
            EXPECT_NEAR(0, pas_soma->values().at("g"), 1e-5);
            EXPECT_NEAR(-65, pas_soma->values().at("e"), 1e-5);

            EXPECT_NEAR(0.001, pas_dend->values().at("g"), 1e-5);
            EXPECT_NEAR(-65, pas_dend->values().at("e"), 1e-5);
        }
    }

    auto mechs_per_sec = md.get_density_mechs(5);
    EXPECT_EQ(0, mechs_per_sec.size());
}
