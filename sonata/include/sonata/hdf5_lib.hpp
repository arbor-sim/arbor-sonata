#pragma once

#include <iostream>
#include <fstream>
#include <string>

#include <hdf5.h>

namespace sonata {
/// Class for reading from hdf5 datasets
/// Datasets are opened and closed every time they are read
class h5_dataset {
public:
    // Constructor from parent (hdf5 group) id and dataset name - finds size of the dataset
    h5_dataset(hid_t parent, std::string name);

    // Constructor from parent (hdf5 group) id and dataset name - creates andf writes dataset `data`
    h5_dataset(hid_t parent, std::string name, std::vector<int> data);

    h5_dataset(hid_t parent, std::string name, std::vector<double> data);

    h5_dataset(hid_t parent, std::string name, std::vector<std::vector<int>> data);

    h5_dataset(hid_t parent, std::string name, std::vector<std::vector<double>> data);

    // returns name of dataset
    std::string name();

    // returns number of elements in a dataset
    int size();

    // Read all dataset
    template <typename T>
    auto get();

    // Read at index `i`; throws exception if out of bounds
    template <typename T>
    auto get(const int i);

    // Read range  between indices `i` and `j`; throws exception if out of bounds
    template <typename T>
    auto get(const int i, const int j);

private:
    // id of parent group
    hid_t parent_id_;

    // name of dataset
    std::string name_;

    // First dimension of dataset
    size_t size_;
};


/// Class for keeping track of what's in an hdf5 group (groups and datasets with pointers to each)
class h5_group {
public:
    // Constructor from parent (hdf5 group) id and group name
    // Builds tree of groups, each with it's own sub-groups and datasets
    h5_group(hid_t parent, std::string name);

    // Returns name of group
    std::string name();

    // Add a new group
    std::shared_ptr<h5_group> add_group(std::string name);

    // Add a new int dataset
    template <typename T>
    void add_dataset(std::string name, std::vector<T> dset);

    // hdf5 groups belonging to group
    std::vector<std::shared_ptr<h5_group>> groups_;

    // hdf5 datasets belonging to group
    std::vector<std::shared_ptr<h5_dataset>> datasets_;

private:
    // RAII to handle recursive opening/closing groups
    struct group_handle {
        group_handle(hid_t parent_id, std::string name): name(name) {
            if (H5Lexists(parent_id, name.c_str(), H5P_DEFAULT)) {
                id = H5Gopen(parent_id, name.c_str(), H5P_DEFAULT);
            } else {
                id = H5Gcreate(parent_id, name.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            }
        }
        ~group_handle() {
            H5Gclose(id);
        }
        hid_t id;
        std::string name;
    };

    // id of parent group
    hid_t parent_id_;

    // name of group
    std::string name_;

    // Handles group opening/closing
    group_handle group_h_;
};


/// Class for an hdf5 file, holding a pointer to the top level group in the file
class h5_file {
private:
    // RAII to handle opening/closing files
    struct file_handle {
        file_handle(std::string file, bool new_file = false): name(file) {
            if (new_file) {
                id = H5Fcreate(file.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            }
            else {
                id = H5Fopen(file.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
            }
        }
        ~file_handle() {
            H5Fclose(id);
        }
        hid_t id;
        std::string name;
    };

    // name of file
    std::string name_;

    // Handles file opening/closing
    file_handle file_h_;

public:
    // Constructor from file name
    h5_file(std::string name, bool new_file=false);

    // Returns file name
    std::string name();

    // Debugging function
    void print();

    // Pointer to top level group in file
    std::shared_ptr<h5_group> top_group_;
};

/// Class that wraps an h5_group
/// Provides direct read access to datasets in the group
/// Provides access to sub-groups of the group
class h5_wrapper {
public:
    h5_wrapper();

    h5_wrapper(const std::shared_ptr<h5_group>& g);

    // Returns number of sub-groups in the wrapped h5_group
    int size();

    // Returns index of sub-group with name `name`; returns -1 if sub-group not found
    int find_group(std::string name) const;

    // Returns index of dataset with name `name`; returns -1 if dataset not found
    int find_dataset(std::string name) const;

    // Returns size of dataset with name `name`; returns -1 if dataset not found
    int dataset_size(std::string name) const;

    // Returns value at index i of dataset with name `name`; throws exception if dataset not found
    template <typename T>
    T get(std::string name, unsigned i) const;

    // Returns values between indices i and j of dataset with name `name`; throws exception if dataset not found
    template <typename T>
    T get(std::string name, unsigned i, unsigned j) const;

    // Returns full content of 1D dataset with name `name`; throws exception if dataset not found
    template <typename T>
    T get(std::string name) const;

    // Returns h5_wrapper of group at index i in members_
    const h5_wrapper& operator[] (unsigned i) const;

    // Returns h5_wrapper of group with name `name` in members_
    const h5_wrapper& operator[] (std::string name) const;

    // Returns name of the wrapped h5_group
    std::string name() const;

private:
    // Pointer to the h5_group wrapped in h5_wrapper
    std::shared_ptr<h5_group> ptr_;

    // Vector of h5_wrappers around sub_groups of the h5_group
    std::vector<h5_wrapper> members_;

    // Map from name of dataset to index in vector of datasets in wrapped h5_group
    std::unordered_map<std::string, unsigned> dset_map_;

    // Map from name of sub-group to index in vector of sub-groups in wrapped h5_group
    std::unordered_map<std::string, unsigned> member_map_;
};

struct local_element{
    std::string pop_name;
    unsigned el_id;
};

/// Class that stores sonata specific information about a collection of hdf5 files
class h5_record {
public:
    h5_record(const std::vector<std::shared_ptr<h5_file>>& files);

    // Verifies that the hdf5 files contain sonata edge information
    bool verify_edges();

    // Verifies that the hdf5 files contain sonata node information
    bool verify_nodes();

    // If gid < num_elements, return population and local id in population, otherwise return empty struct
    local_element localize(unsigned gid) const;

    // Given a population and local id in it, return gid
    unsigned globalize(local_element n) const;

    // Returns total number of nodes/edges
    int num_elements() const;

    // Returns the population with name `name` in populations_
    const h5_wrapper& operator [](std::string name) const;

    // Returns the population at index `i` in populations_
    const h5_wrapper& operator [](int i) const;

    // Returns true if `name` present in map_
    bool find_population(std::string name) const;

    // Returns names of all populations_
    std::vector<std::string> pop_names() const;

    // Returns all populations
    std::vector<h5_wrapper> populations() const;

    // Returns partitioned sizes of every population in the h5_record
    std::vector<unsigned> partitions() const;

    // Returns map_
    std::unordered_map<std::string, unsigned> map() const;

private:
    // Total number of nodes/ edges
    int num_elements_ = 0;

    // Keep the original hdf5 files, to guarantee they remain open for the lifetime of a record
    std::vector<std::shared_ptr<h5_file>> files_;

    // Population names
    std::vector<std::string> pop_names_;

    // Partitioned sizes of the populations
    std::vector<unsigned> partition_;

    // Wrapped h5_group representing the top levels of every population
    std::vector<h5_wrapper> populations_;

    // Map from population name to index in populations_
    std::unordered_map<std::string, unsigned> map_;
};
} // namespace sonata