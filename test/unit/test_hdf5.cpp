#include "../gtest.h"

#include <arbor/cable_cell.hpp>

#include <sonata/hdf5_lib.hpp>
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

TEST(hdf5_record, verify_nodes) {
    std::string datadir{DATADIR};

    auto filename0 = datadir + "/nodes_0.h5";
    auto filename1 = datadir + "/nodes_1.h5";
    auto filename2 = datadir + "/nodes_2.h5";

    auto f0 = std::make_shared<h5_file>(filename0);
    auto f1 = std::make_shared<h5_file>(filename1);
    auto f2 = std::make_shared<h5_file>(filename2);

    h5_record r({f0, f1, f2});

    EXPECT_TRUE(r.verify_nodes());
    EXPECT_THROW(r.verify_edges(), sonata_exception);

    EXPECT_EQ(std::vector<unsigned>({0u, 4u, 5u, 6u}), r.partitions());
    EXPECT_EQ(6, r.num_elements());

    auto pops = r.populations();
    auto map = r.map();

    EXPECT_EQ(pops.size(), 3);
    EXPECT_EQ("pop_e", pops[0].name());
    EXPECT_EQ("pop_i", pops[1].name());
    EXPECT_EQ("pop_ext", pops[2].name());

    EXPECT_EQ(0, map["pop_e"]);
    EXPECT_EQ(1, map["pop_i"]);
    EXPECT_EQ(2, map["pop_ext"]);

    EXPECT_EQ(r[0].name(), r["pop_e"].name());
    EXPECT_EQ(r[1].name(), r["pop_i"].name());
    EXPECT_EQ(r[2].name(), r["pop_ext"].name());

    for (unsigned i = 0; i < 4; i++) {
        auto l = r.localize(i);
        EXPECT_EQ("pop_e", l.pop_name);
        EXPECT_EQ(i, l.el_id);
    }
    auto l = r.localize(4);
    EXPECT_EQ("pop_i", l.pop_name);
    EXPECT_EQ(0, l.el_id);

    l = r.localize(5);
    EXPECT_EQ("pop_ext", l.pop_name);
    EXPECT_EQ(0, l.el_id);

    for (unsigned i = 0; i < 4; i++) {
        EXPECT_EQ(i, r.globalize({"pop_e", i}));
    }
    EXPECT_EQ(4, r.globalize({"pop_i", 0}));
    EXPECT_EQ(5, r.globalize({"pop_ext", 0}));
}

TEST(hdf5_record, verify_edges) {
    using int_pair = std::pair<int,int>;
    std::string datadir{DATADIR};

    auto filename0 = datadir + "/edges_0.h5";
    auto filename1 = datadir + "/edges_1.h5";
    auto filename2 = datadir + "/edges_2.h5";
    auto filename3 = datadir + "/edges_3.h5";

    auto f0 = std::make_shared<h5_file>(filename0);
    auto f1 = std::make_shared<h5_file>(filename1);
    auto f2 = std::make_shared<h5_file>(filename2);
    auto f3 = std::make_shared<h5_file>(filename3);

    h5_record r({f0, f1, f2, f3});

    EXPECT_TRUE(r.verify_edges());
    EXPECT_THROW(r.verify_nodes(), sonata_exception);

    EXPECT_EQ(std::vector<unsigned>({0u, 2u, 3u, 4u, 6u}), r.partitions());
    EXPECT_EQ(6, r.num_elements());

    auto pops = r.populations();
    auto map = r.map();

    EXPECT_EQ(pops.size(), 4);
    EXPECT_EQ("pop_e_i", pops[0].name());
    EXPECT_EQ("pop_i_e", pops[1].name());
    EXPECT_EQ("pop_e_e", pops[2].name());
    EXPECT_EQ("pop_ext_e", pops[3].name());

    EXPECT_EQ(0, map["pop_e_i"]);
    EXPECT_EQ(1, map["pop_i_e"]);
    EXPECT_EQ(2, map["pop_e_e"]);
    EXPECT_EQ(3, map["pop_ext_e"]);

    EXPECT_EQ(r[0].name(), r["pop_e_i"].name());
    EXPECT_EQ(r[1].name(), r["pop_i_e"].name());
    EXPECT_EQ(r[2].name(), r["pop_e_e"].name());
    EXPECT_EQ(r[3].name(), r["pop_ext_e"].name());

    EXPECT_EQ(2, r["pop_e_i"].dataset_size("edge_group_id"));
    EXPECT_EQ(0, r["pop_e_i"].get<int>("edge_group_id",0));
    EXPECT_EQ(1, r["pop_e_i"].get<int>("edge_group_id",1));

    EXPECT_EQ(2, r["pop_e_i"].dataset_size("edge_group_index"));
    EXPECT_EQ(0, r["pop_e_i"].get<int>("edge_group_index",0));
    EXPECT_EQ(0, r["pop_e_i"].get<int>("edge_group_index",1));

    EXPECT_EQ(2, r["pop_e_i"].dataset_size("edge_type_id"));
    EXPECT_EQ(103, r["pop_e_i"].get<int>("edge_type_id",0));
    EXPECT_EQ(103, r["pop_e_i"].get<int>("edge_type_id",1));

    EXPECT_NE(-1, r["pop_e_i"].find_group("0"));
    EXPECT_NE(-1, r["pop_e_i"].find_group("1"));

    EXPECT_NE(-1, r["pop_i_e"].find_group("0"));
    EXPECT_EQ(-1, r["pop_i_e"].find_group("1"));

    EXPECT_NE(-1, r["pop_e_e"].find_group("0"));
    EXPECT_EQ(-1, r["pop_e_e"].find_group("1"));

    EXPECT_EQ(0,   r["pop_e_i"]["0"].get<int>("afferent_section_id", 0));
    EXPECT_NEAR(0.4, r["pop_e_i"]["0"].get<double>("afferent_section_pos", 0), 1e-5);
    EXPECT_EQ(1,   r["pop_e_i"]["0"].get<int>("efferent_section_id", 0));
    EXPECT_NEAR(0.3, r["pop_e_i"]["0"].get<double>("efferent_section_pos", 0), 1e-5);

    EXPECT_EQ(2,   r["pop_e_i"]["1"].get<int>("afferent_section_id", 0));
    EXPECT_NEAR(0.1, r["pop_e_i"]["1"].get<double>("afferent_section_pos", 0), 1e-5);
    EXPECT_EQ(3,   r["pop_e_i"]["1"].get<int>("efferent_section_id", 0));
    EXPECT_NEAR(0.2, r["pop_e_i"]["1"].get<double>("efferent_section_pos", 0), 1e-5);

    EXPECT_EQ(0,   r["pop_i_e"]["0"].get<int>("afferent_section_id", 0));
    EXPECT_NEAR(0.5, r["pop_i_e"]["0"].get<double>("afferent_section_pos", 0), 1e-5);
    EXPECT_EQ(0,   r["pop_i_e"]["0"].get<int>("efferent_section_id", 0));
    EXPECT_NEAR(0.9, r["pop_i_e"]["0"].get<double>("efferent_section_pos", 0), 1e-5);

    EXPECT_EQ(5,   r["pop_e_e"]["0"].get<int>("afferent_section_id", 0));
    EXPECT_NEAR(0.6, r["pop_e_e"]["0"].get<double>("afferent_section_pos", 0), 1e-5);
    EXPECT_EQ(1,   r["pop_e_e"]["0"].get<int>("efferent_section_id", 0));
    EXPECT_NEAR(0.2, r["pop_e_e"]["0"].get<double>("efferent_section_pos", 0), 1e-5);

    EXPECT_EQ(std::make_pair(0,1), r["pop_e_i"]["indicies"]["source_to_target"].get<int_pair>("node_id_to_ranges", 0));
    EXPECT_EQ(std::make_pair(1,1), r["pop_e_i"]["indicies"]["source_to_target"].get<int_pair>("node_id_to_ranges", 1));
    EXPECT_EQ(std::make_pair(1,2), r["pop_e_i"]["indicies"]["source_to_target"].get<int_pair>("node_id_to_ranges", 2));
    EXPECT_EQ(std::make_pair(2,2), r["pop_e_i"]["indicies"]["source_to_target"].get<int_pair>("node_id_to_ranges", 3));

    EXPECT_EQ(std::make_pair(0,1), r["pop_e_i"]["indicies"]["source_to_target"].get<int_pair>("range_to_edge_id", 0));
    EXPECT_EQ(std::make_pair(1,2), r["pop_e_i"]["indicies"]["source_to_target"].get<int_pair>("range_to_edge_id", 1));

    EXPECT_EQ(0, r["pop_e_i"].get<int>("source_node_id", 0));
    EXPECT_EQ(0, r["pop_e_i"].get<int>("target_node_id", 0));
    EXPECT_EQ(2, r["pop_e_i"].get<int>("source_node_id", 1));
    EXPECT_EQ(0, r["pop_e_i"].get<int>("target_node_id", 1));
}
