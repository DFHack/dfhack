#pragma once

// Namespaced labels and their export.

#include "type.h"

#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"

namespace DFHack {
namespace Export {
struct Label {
  const std::string name;
  Type *type; // non-null
  const char *address;

  bool operator<(const Label &b) const {
    return address < b.address;
  }

  Json::Value to_json(Xref &x) const {
    try {
      Json::Value json;
      json["name"] = name;
      json["type"] = x[type];
      json["address"] = (long) address;
      return json;
    } catch(const Error &e) {
      e.context(name);
    }
  }
};

template<typename T> class Holder : Namer {
  using Map = std::map<std::string, std::unique_ptr<T>>;
  Map name_to_item;
public:
  template<typename ... Cs> T &create(const std::string &n, Cs... cs) {
    auto name = n.empty() ? unnamed() : n;
    if (have(name))
      die(n + " name collision");
    std::unique_ptr<T> ptr{new T{name, cs...}};
    name_to_item[name] = std::move(ptr);
    return *name_to_item.at(name);
  }
  bool have(const std::string &n) const { return name_to_item.count(n); }
  T &operator[](const std::string &n) const {
    if (!have(n))
      die(n + " does not exist");
    return *name_to_item.at(n);
  }
  const Map &all() const {
    return name_to_item;
  }
};

class Namespace {
  friend Holder<Namespace>;
  explicit Namespace(const std::string &n) :name{n} {}
protected:
  std::vector<const Label *> all_labels() {
    std::vector<const Label *> ls;
    for (const auto &it : labels.all())
      ls.push_back(it.second.get());
    for (const auto &it : namespaces.all()) {
      auto cs = it.second->all_labels();
      ls.insert(ls.end(), cs.begin(), cs.end());
    }
    return ls;
  }
  void check_none_overlap() {
    auto all = all_labels();
    std::sort(all.begin(), all.end());
    const char *last = 0;
    for (const auto *l : all) {
      if (l->address <= last)
        die(l->name + " label overlap");
      last = l->address + (l->type->byte.size() || 1);
    }
  }
public:
  static Namespace new_root(const char *name) {
    return Namespace{std::string{name}};
  }
  const std::string name;
  Holder<Namespace> namespaces;
  Holder<Label> labels;
  Json::Value to_json(Xref &x) const {
    try {
      Json::Value json;
      json["name"] = name;
      for (const auto &it : labels.all())
        json["labels"].append(it.second->to_json(x));
      for (const auto &it : namespaces.all())
        json["namespaces"].append(it.second->to_json(x));
      return json;
    } catch(const Error &e) {
      e.context(name);
    }
  }
};
} // namespace Export
} // namespace DFHack
