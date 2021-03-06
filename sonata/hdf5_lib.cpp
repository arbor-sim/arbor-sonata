#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <hdf5.h>

#include <sonata/sonata_exceptions.hpp>
#include <sonata/hdf5_lib.hpp>

#define MAX_NAME 1024

///h5_dataset methods
namespace sonata {
h5_dataset::h5_dataset(hid_t parent, std::string name): parent_id_(parent), name_(name) {
    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    hid_t dspace = H5Dget_space(id);

    const int ndims = H5Sget_simple_extent_ndims(dspace);

    hsize_t dims[ndims];
    H5Sget_simple_extent_dims(dspace, dims, NULL);

    size_ = dims[0];

    H5Sclose(dspace);
    H5Dclose(id);
}

h5_dataset::h5_dataset(hid_t parent, std::string name, std::vector<int> data): parent_id_(parent), name_(name) {
    hsize_t size = data.size();
    auto dspace = H5Screate_simple(1, &size, NULL);

    auto id = H5Dcreate(parent_id_, name.c_str(), H5T_NATIVE_INT, dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    int arr[size];
    std::copy(data.begin(), data.end(), arr);
    H5Dwrite(id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, arr);

    H5Sclose(dspace);
    H5Dclose(id);
}

h5_dataset::h5_dataset(hid_t parent, std::string name, std::vector<double> data): parent_id_(parent), name_(name) {
    hsize_t size = data.size();
    auto dspace = H5Screate_simple(1, &size, NULL);

    auto id = H5Dcreate(parent_id_, name.c_str(), H5T_NATIVE_DOUBLE, dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    double arr[size];
    std::copy(data.begin(), data.end(), arr);
    H5Dwrite(id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, arr);

    H5Sclose(dspace);
    H5Dclose(id);
}

h5_dataset::h5_dataset(hid_t parent, std::string name, std::vector<std::vector<int>> data): parent_id_(parent), name_(name) {
    hsize_t dims_data[2] = {data.size(), data.front().size()};
    auto dspace = H5Screate_simple(2, dims_data, NULL);

    auto id = H5Dcreate(parent_id_, name.c_str(), H5T_NATIVE_INT, dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    int arr[data.size()][data.front().size()];
    for (unsigned i = 0; i < data.size(); i++) {
        std::copy(data[i].begin(), data[i].end(), arr[i]);
    }
    H5Dwrite(id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, arr);

    H5Sclose(dspace);
    H5Dclose(id);
}

h5_dataset::h5_dataset(hid_t parent, std::string name, std::vector<std::vector<double>> data): parent_id_(parent), name_(name) {
    hsize_t dims_data[2] = {data.size(), data.front().size()};
    auto dspace = H5Screate_simple(2, dims_data, NULL);

    auto id = H5Dcreate(parent_id_, name.c_str(), H5T_NATIVE_DOUBLE, dspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    double arr[data.size()][data.front().size()];
    for (unsigned i = 0; i < data.size(); i++) {
        std::copy(data[i].begin(), data[i].end(), arr[i]);
    }
    H5Dwrite(id, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT, arr);

    H5Sclose(dspace);
    H5Dclose(id);
}

std::string h5_dataset::name() {
    return name_;
}

int h5_dataset::size() {
    return size_;
}

template<>
auto h5_dataset::get<int>(const int i) {
    const hsize_t idx = (hsize_t)i;

    // Output
    int *out = new int[1];

    // Output dimensions 1x1
    hsize_t dims = 1;
    hsize_t dim_sizes[] = {1};

    // Output size
    hsize_t num_elements = 1;

    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    hid_t dspace = H5Dget_space(id);

    H5Sselect_elements(dspace, H5S_SELECT_SET, num_elements, &idx);

    hid_t out_mem = H5Screate_simple(dims, dim_sizes, NULL);

    auto status = H5Dread(id, H5T_NATIVE_INT, out_mem, dspace, H5P_DEFAULT, out);

    H5Sclose(dspace);
    H5Sclose(out_mem);
    H5Dclose(id);

    if (status < 0 ) {
        throw sonata_dataset_exception(name_, (unsigned)i);
    }

    int r = out[0];
    delete [] out;

    return r;
}

template<>
auto h5_dataset::get<double>(const int i) {
    const hsize_t idx = (hsize_t)i;

    // Output
    double *out = new double[1];

    // Output dimensions 1x1
    hsize_t dims = 1;
    hsize_t dim_sizes[] = {1};

    // Output size
    hsize_t num_elements = 1;

    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    hid_t dspace = H5Dget_space(id);

    H5Sselect_elements(dspace, H5S_SELECT_SET, num_elements, &idx);
    hid_t out_mem = H5Screate_simple(dims, dim_sizes, NULL);

    auto status = H5Dread(id, H5T_NATIVE_DOUBLE, out_mem, dspace, H5P_DEFAULT, out);

    H5Sclose(dspace);
    H5Sclose(out_mem);
    H5Dclose(id);

    if (status < 0) {
        throw sonata_dataset_exception(name_, (unsigned)i);
    }

    double r = out[0];
    delete [] out;

    return r;
}

template<>
auto h5_dataset::get<std::string>(const int i) {
    const hsize_t idx = (hsize_t)i;
    hsize_t dims = 1;
    hsize_t dim_sizes[] = {1};
    size_t  sdim;

    char *out;

    auto dset =  H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    auto filetype = H5Dget_type(dset);

    // Initialize sdim with size of string + null terminator
    sdim = H5Tget_size(filetype) + 1;

    // Initialize output buffer
    out = (char *) malloc (sdim * sizeof (char));

    H5Dclose(dset);
    H5Tclose(filetype);

    // Open dataset and dataspace
    dset =  H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    auto dspace = H5Dget_space(dset);

    // Select element to read
    H5Sselect_elements(dspace, H5S_SELECT_SET, 1, &idx);
    hid_t out_mem = H5Screate_simple(dims, dim_sizes, NULL);

    // Select right datatype
    auto memtype = H5Tcopy(H5T_C_S1);
    H5Tset_size (memtype, sdim);

    auto status = H5Dread(dset, memtype, out_mem, dspace, H5P_DEFAULT, out);

    if (status < 0) {
        throw sonata_dataset_exception(name_, (unsigned)i);
    }

    std::string ret(out);

    free (out);
    H5Dclose(dset);
    H5Sclose(dspace);
    H5Tclose(memtype);

    return ret;
}

template<>
auto h5_dataset::get<std::vector<int>>(const int i, const int j) {
    hsize_t offset = i;
    hsize_t count = j-i;
    hsize_t stride = 1;
    hsize_t  block = 1;
    hsize_t dimsm = count;

    int rdata[count];

    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    hid_t dspace = H5Dget_space(id);

    hid_t out_mem = H5Screate_simple(1, &dimsm, NULL);

    H5Sselect_hyperslab(dspace, H5S_SELECT_SET, &offset, &stride, &count, &block);
    auto status = H5Dread(id, H5T_NATIVE_INT, out_mem, dspace, H5P_DEFAULT, rdata);

    H5Sclose(dspace);
    H5Sclose(out_mem);
    H5Dclose(id);

    if (status < 0) {
        throw sonata_dataset_exception(name_, (unsigned)i, (unsigned)j);
    }

    std::vector<int> out(rdata, rdata + count);

    return out;
}

template <>
auto h5_dataset::get<std::vector<double>>(const int i, const int j) {
    hsize_t offset = i;
    hsize_t count = j-i;
    hsize_t stride = 1;
    hsize_t  block = 1;
    hsize_t dimsm = count;

    double rdata[count];

    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    hid_t dspace = H5Dget_space(id);

    hid_t out_mem = H5Screate_simple(1, &dimsm, NULL);

    H5Sselect_hyperslab(dspace, H5S_SELECT_SET, &offset, &stride, &count, &block);

    auto status = H5Dread(id, H5T_NATIVE_DOUBLE, out_mem, dspace, H5P_DEFAULT, rdata);

    H5Sclose(dspace);
    H5Sclose(out_mem);
    H5Dclose(id);

    if (status < 0) {
        throw sonata_dataset_exception(name_, (unsigned)i, (unsigned)j);
    }

    std::vector<double> out(rdata, rdata + count);

    return out;
}

template <>
auto h5_dataset::get<std::pair<int,int>>(const int i) {
    const hsize_t idx_0[2] = {(hsize_t)i, (hsize_t)0};

    // Output
    int out_0, out_1;

    // Output dimensions 1x1
    hsize_t dims = 1;
    hsize_t dim_sizes[] = {1};

    // Output size
    hsize_t num_elements = 1;

    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);
    hid_t dspace = H5Dget_space(id);

    H5Sselect_elements(dspace, H5S_SELECT_SET, num_elements, idx_0);
    hid_t out_mem_0 = H5Screate_simple(dims, dim_sizes, NULL);

    auto status0 = H5Dread(id, H5T_NATIVE_INT, out_mem_0, dspace, H5P_DEFAULT, &out_0);

    const hsize_t idx_1[2] = {(hsize_t)i, (hsize_t)1};

    H5Sselect_elements(dspace, H5S_SELECT_SET, num_elements, idx_1);
    hid_t out_mem_1 = H5Screate_simple(dims, dim_sizes, NULL);

    auto status1 = H5Dread(id, H5T_NATIVE_INT, out_mem_1, dspace, H5P_DEFAULT, &out_1);

    H5Sclose(dspace);
    H5Sclose(out_mem_0);
    H5Sclose(out_mem_1);
    H5Dclose(id);

    if (status0 < 0 || status1 < 0) {
        throw sonata_dataset_exception(name_, (unsigned)i);
    }

    return std::make_pair(out_0, out_1);
}
template <>
auto h5_dataset::get<std::vector<int>>() {
    int out_a[size_];
    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);

    auto status = H5Dread(id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
            out_a);
    H5Dclose(id);

    if (status < 0) {
        throw sonata_dataset_exception(name_);
    }

    std::vector<int> out(out_a, out_a + size_);

    return out;
}

template <>
auto h5_dataset::get<std::vector<std::pair<int, int>>>() {
    int out_a[size_][2];
    auto id = H5Dopen(parent_id_, name_.c_str(), H5P_DEFAULT);

    auto status = H5Dread(id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT, out_a);

    H5Dclose(id);

    if (status < 0) {
        throw sonata_dataset_exception(name_);
    }

    std::vector<std::pair<int, int>> out(size_);
    for (unsigned i = 0; i < size_; i++) {
        out[i] = std::make_pair(out_a[i][0], out_a[i][1]);
    }

    return out;
}

///h5_group methods

h5_group::h5_group(hid_t parent, std::string name): parent_id_(parent), name_(name), group_h_(parent_id_, name_) {

    hsize_t nobj;
    H5Gget_num_objs(group_h_.id, &nobj);

    char memb_name[MAX_NAME];

    groups_.reserve(nobj);

    for (unsigned i = 0; i < nobj; i++) {
        H5Gget_objname_by_idx(group_h_.id, (hsize_t)i, memb_name, (size_t)MAX_NAME);
        hid_t otype = H5Gget_objtype_by_idx(group_h_.id, (size_t)i);
        if (otype == H5G_GROUP) {
            groups_.emplace_back(std::make_shared<h5_group>(group_h_.id, memb_name));
        }
        else if (otype == H5G_DATASET) {
            datasets_.emplace_back(std::make_shared<h5_dataset>(group_h_.id, memb_name));
        }
    }
}

std::shared_ptr<h5_group> h5_group::add_group(std::string name) {
    auto new_group = std::make_shared<h5_group>(group_h_.id, name);
    groups_.emplace_back(new_group);
    return new_group;
}

template <typename T>
void h5_group::add_dataset(std::string name, std::vector<T> dset) {
    auto new_dataset = std::make_shared<h5_dataset>(group_h_.id, name, dset);
    datasets_.emplace_back(new_dataset);
}

std::string h5_group::name() {
    return name_;
}

///h5_file methods
h5_file::h5_file(std::string name, bool new_file):
        name_(name),
        file_h_(name, new_file),
        top_group_(std::make_shared<h5_group>(file_h_.id, "/")) {}

std::string h5_file::name() {
    return name_;
}

void print_group(std::ostream& out, const std::shared_ptr<h5_group>& group, int indent) {
    for (const auto& g: group->groups_) {
        out << std::string(indent, '\t') << g->name() << std::endl;
        print_group(out, g, indent+1);
    }
    for (const auto& d: group->datasets_) {
        out << std::string(indent, '\t') << d->name() << "(" << d->size() << ")" << std::endl;
    }
}

void h5_file::print() {
    std::cout << top_group_->name() << std::endl;
    print_group(std::cout, top_group_, 1);
}


///h5_wrapper methods

h5_wrapper::h5_wrapper() {}

h5_wrapper::h5_wrapper(const std::shared_ptr<h5_group>& g): ptr_(g) {
    unsigned i = 0;
    for (auto d: ptr_->datasets_) {
        dset_map_[d->name()] = i++;
    }
    i = 0;
    for (auto g: ptr_->groups_) {
        member_map_[g->name()] = i++;
        members_.emplace_back(g);
    }
}

int h5_wrapper::size() {
    return members_.size();
}

int h5_wrapper::find_group(std::string name) const {
    if (member_map_.find(name) != member_map_.end()) {
        return member_map_.at(name);
    }
    return -1;
}

int h5_wrapper::find_dataset(std::string name) const {
    if (dset_map_.find(name) != dset_map_.end()) {
        return dset_map_.at(name);
    }
    return -1;
}

int h5_wrapper::dataset_size(std::string name) const {
    if (dset_map_.find(name) != dset_map_.end()) {
        return ptr_->datasets_.at(dset_map_.at(name))->size();
    }
    return -1;
}

template <typename T>
T h5_wrapper::get(std::string name, unsigned i) const {
    if (find_dataset(name) != -1) {
        return ptr_->datasets_.at(dset_map_.at(name))->get<T>(i);
    }
    throw sonata_dataset_exception(name);
}

template <typename T>
T h5_wrapper::get(std::string name, unsigned i, unsigned j) const {
    if (find_dataset(name)!= -1) {
        return ptr_->datasets_.at(dset_map_.at(name))->get<T>(i, j);
    }
    throw sonata_dataset_exception(name);
}

template <typename T>
T h5_wrapper::get(std::string name) const {
    if (find_dataset(name)!= -1) {
        return ptr_->datasets_.at(dset_map_.at(name))->get<T>();
    }
    throw sonata_dataset_exception(name);
}

const h5_wrapper& h5_wrapper::operator [](unsigned i) const {
    if (i < members_.size() && i >= 0) {
        return members_.at(i);
    }
    throw sonata_exception("h5_wrapper index out of range");
}

const h5_wrapper& h5_wrapper::operator [](std::string name) const {
    auto gid = find_group(name);
    if (gid != -1) {
        return members_.at(gid);
    }
    throw sonata_exception("h5_wrapper index out of range");
}

std::string h5_wrapper::name() const {
    return ptr_->name();
}

///h5_record methods

h5_record::h5_record(const std::vector<std::shared_ptr<h5_file>>& files) : files_(files) {
    unsigned idx = 0;
    partition_.push_back(0);
    for (auto f: files) {
        if (f->top_group_->groups_.size() != 1) {
            throw sonata_exception("file hierarchy wrong\n");
        }

        for (auto g: f->top_group_->groups_.front()->groups_) {
            pop_names_.emplace_back(g->name());
            map_[g->name()] = idx++;
            populations_.emplace_back(g);
        }

        for (auto &p: f->top_group_->groups_.front()->groups_) {
            for (auto &d: p->datasets_) {
                if (d->name().find("type_id") != std::string::npos) {
                    num_elements_ += d->size();
                    partition_.push_back(num_elements_);
                }
            }
        }
    }
}

bool h5_record::verify_edges() {
    for (auto& p: populations()) {
        if (p.find_group("indicies") == -1) {
            throw sonata_exception("indicies group must be available in all edge population groups ");
        }
        if (p.find_dataset("edge_type_id") == -1) {
            throw sonata_exception("edge_type_id dataset not provided in edge populations");
        }
        if (p.find_dataset("edge_id") != -1) {
            throw sonata_exception("edge_id datasets not supported; "
                                        "ids are automatically assigned contiguously starting from 0");
        }
    }
    return true;
}

bool h5_record::verify_nodes() {
    for (auto& p: populations()) {
        if (p.find_dataset("node_type_id") == -1) {
            throw sonata_exception("node_type_id dataset not provided in node populations");
        }
        if (p.find_dataset("node_id") != -1) {
            throw sonata_exception("node_id datasets not supported; "
                                        "ids are automatically assigned contiguously starting from 0");
        }
    }
    return true;
}

local_element h5_record::localize(unsigned gid) const {
    for (unsigned i = 0; i < partitions().size(); i++) {
        if (gid < partitions()[i]) {
            return {pop_names()[i-1], gid - partitions()[i-1]};
        }
    }
    return local_element();
}

unsigned h5_record::globalize(local_element n) const {
    return n.el_id + partitions()[map_.at(n.pop_name)];
}

int h5_record::num_elements() const {
    return num_elements_;
}

const h5_wrapper& h5_record::operator [](std::string name) const {
    if (map_.find(name) == map_.end()) {
        throw sonata_exception("population not present in hdf5 file");
    }
    return populations_[map_.at(name)];
}

const h5_wrapper& h5_record::operator [](int i) const {
    return populations_[i];
}

bool h5_record::find_population(std::string name) const {
    if(map_.find(name) != map_.end()) {
        return true;
    }
    return false;
}


std::vector<h5_wrapper> h5_record::populations() const {
    return populations_;
}

std::vector<unsigned> h5_record::partitions() const {
    return partition_;
}

std::unordered_map<std::string, unsigned> h5_record::map() const {
    return map_;
}

std::vector<std::string> h5_record::pop_names() const {
    return pop_names_;
}

template void h5_group::add_dataset<int>(std::string, std::vector<int>);
template void h5_group::add_dataset<double>(std::string, std::vector<double>);
template void h5_group::add_dataset<std::vector<int>>(std::string, std::vector<std::vector<int>>);
template void h5_group::add_dataset<std::vector<double>>(std::string, std::vector<std::vector<double>>);

template int h5_wrapper::get<int>(std::string, unsigned) const;
template double h5_wrapper::get<double>(std::string, unsigned) const;
template std::string h5_wrapper::get<std::string>(std::string, unsigned) const;
template std::pair<int,int> h5_wrapper::get<std::pair<int,int>>(std::string, unsigned) const;

template std::vector<int> h5_wrapper::get<std::vector<int>>(std::string name, unsigned i, unsigned j) const;
template std::vector<double> h5_wrapper::get<std::vector<double>>(std::string name, unsigned i, unsigned j) const;

template std::vector<int> h5_wrapper::get<std::vector<int>>(std::string) const;
template std::vector<std::pair<int,int>> h5_wrapper::get<std::vector<std::pair<int,int>>>(std::string) const;
} // namespace sonata