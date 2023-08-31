#pragma once

#include <array>
#include <exception>
#include <optional>
#include <string>
#include <fstream>

#include <nlohmann/json.hpp>

namespace sup {

// Search a json object for an entry with a given name.
// If found, return the value and remove from json object.
template <typename T>
std::optional<T> find_and_remove_json(const char* name, nlohmann::json& j) {
    auto it = j.find(name);
    if (it==j.end()) {
        return std::nullopt;
    }
    T value = std::move(*it);
    j.erase(name);
    return std::move(value);
}

template <typename T>
void param_from_json(T& x, const char* name, nlohmann::json& j) {
    if (auto o = find_and_remove_json<T>(name, j)) {
        x = *o;
    }
}

template <typename T, size_t N>
void param_from_json(std::array<T, N>& x, const char* name, nlohmann::json& j) {
    std::vector<T> y;
    if (auto o = find_and_remove_json<std::vector<T>>(name, j)) {
        y = *o;
        if (y.size()!=N) {
            throw std::runtime_error("parameter "+std::string(name)+" requires "+std::to_string(N)+" values");
        }
        std::copy(y.begin(), y.end(), x.begin());
    }
}

inline
nlohmann::json read_json_file(const std::string& fn) {
    std::ifstream fd(fn);
    if (!fd.good()) throw std::runtime_error("Unable to open input file: " + fn);
    nlohmann::json json;
    fd >> json;
    return json;
}

template <typename T>
T json_get_value(nlohmann::json& json, const std::string& k) {
    if (!json.contains(k)) throw std::runtime_error("JSON missing required field: " + k);
    try {
        return json[k].get<T>();
    } catch (nlohmann::json::type_error& e) {
        throw std::runtime_error("Couldn't obtain field '" + k + "' due to type error: " + e.what());
    }
}

} // namespace sup
