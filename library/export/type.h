#pragma once

// Type representation and its export.

#include <limits>
#include <map>
#include <memory>
#include <string>

#include <json/json.h>

#include "error.h"

#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"
#pragma GCC diagnostic ignored "-Wparentheses"

namespace DFHack {
namespace Export {
class ByteSizeAlignment {
  size_t _size = 0;
  size_t _alignment = 0;

  void check_set(bool should_be_set) const {
    if (is_set() != should_be_set) {
      if (!is_set())
        die("tried to get size/alignment of incomplete");
      else
        die("tried to set size/alignment twice");
    }
  }

public:
  bool is_set() const {
    // Assumes both must be set at once.
    return _alignment != 0;
  }

  size_t size() const {
    check_set(true);
    return _size;
  }
  size_t alignment() const {
    check_set(true);
    return _alignment;
  }

  void set(size_t size, size_t alignment) {
    if (alignment & alignment - 1 || !alignment)
      die("alignment must be a power of two: ");
    if (size % alignment != 0)
      die("size must be a multiple of alignment");
    check_set(false);
    _size = size;
    _alignment = alignment;
  }
  void set(ByteSizeAlignment b) {
    check_set(false);
    if (b.is_set())
      set(b.size(), b.alignment());
  }

  ByteSizeAlignment() = default;
  ByteSizeAlignment(size_t size, size_t alignment) {
    set(size, alignment);
  }
};

class Type;

class Xref {
  std::map<const Type *, size_t> type_to_id;
  std::vector<const Type *> id_to_type;
public:
  size_t operator[](Type *t) {
    if (!type_to_id.count(t)) {
      type_to_id[t] = id_to_type.size();
      id_to_type.push_back(t);
    }
    return type_to_id.at(t);
  }
  const std::vector<const Type *> &types() const { return id_to_type; }
};

class Pointer;
class Array;

class Type {
  Type(const Type &) = delete;
  Type &operator=(const Type &) = delete;
  Type(Type &&) = delete;
  Type &operator=(Type &&) = delete;
  std::map<size_t, std::unique_ptr<Array>> length_to_array;
  std::unique_ptr<Pointer> _pointer;
protected:
  Type() = default;
public:
  virtual ~Type() = default;
  ByteSizeAlignment byte;
  Pointer &pointer();
  Array &array(size_t element_count);
  virtual std::string name() const = 0;

  virtual Json::Value to_json(Xref &x) const = 0;
  virtual const char *kind() const = 0;
  virtual Json::Value base_json(Xref &) const {
    Json::Value json;
    json["kind"] = kind();
    json["complete"] = byte.is_set();
    if (byte.is_set()) {
      json["size"] = byte.size();
      json["alignment"] = byte.alignment();
    }
    return json;
  }
};

class Pointer : public Type {
  friend Type;
  explicit Pointer(Type &t)
    :type{t}
  {
    byte.set(sizeof(void *), alignof(void *));
  }
public:
  Type &type;
  std::string name() const override {
    return type.name().insert(0, "p_");
  }
  const char *kind() const override { return "pointer"; }
  Json::Value to_json(Xref &x) const override {
    auto json = base_json(x);
    json["type"] = x[&type];
    return json;
  }
};

Pointer &Type::pointer() {
  if (!_pointer)
    _pointer.reset(new Pointer{*this});
  return *_pointer;
}

class Array : public Type {
  friend Type;
  Array(Type &t, size_t c)
    :type{t}, count{c}
  {
    byte.set(count * t.byte.size(), t.byte.alignment());
  }
public:
  Type &type;
  const size_t count;
  std::string name() const override {
    return std::string{'a'} + std::to_string(count) + '_' + type.name();
  }
  const char *kind() const override { return "array"; }
  Json::Value to_json(Xref &x) const override {
    auto json = base_json(x);
    json["type"] = x[&type];
    json["count"] = count;
    return json;
  }
};

Array &Type::array(size_t l) {
  if (!length_to_array.count(l))
    length_to_array[l] = std::unique_ptr<Array>{new Array{*this, l}};
  return *length_to_array.at(l);
}

class Named : public Type {
  const std::string _name;
protected:
  explicit Named(const std::string &n) :_name{n} {}
public:
  std::string name() const override { return _name; }
  Json::Value base_json(Xref &x) const override {
    auto json = Type::base_json(x);
    json["name"] = name();
    return json;
  }
};

class Root;

class Basic : public Named {
  friend Root;
  Basic(const std::string &n, ByteSizeAlignment b)
    :Named{n} {
    byte.set(b);
  }
  const char *kind() const override { return "basic"; }
  Json::Value to_json(Xref &x) const override { return base_json(x); }
};

class Namer {
  int n;
public:
  std::string unnamed() {
    return std::string{"unnamed_"} + std::to_string(++n);
  }
};

template<typename T> class Fielded : public Named, public Namer {
  std::vector<T> _fields;
  bool fields_set = false;
  virtual void on_fields_set() {};
protected:
  using Named::Named;
  size_t alignment() const {
    size_t alignment = 1;
    for (const auto &f : fields()) {
      if (f.type->byte.alignment() > alignment)
        alignment = f.type->byte.alignment();
    }
    return alignment;
  }
public:
  const std::vector<T> &fields() const {
    if (!fields_set)
      die(name().insert(0, "tried to get unset fields of "));
    return _fields;
  }
  void set_fields(const std::vector<T> &fs) {
    if (fields_set)
      die("tried to reset set fields");
    std::set<std::string> used;
    _fields = fs;
    for (auto &f : _fields) {
      if (f.name == "")
        f.name = unnamed();
      if (used.count(f.name))
        die(f.name + " field name collision");
      used.insert(f.name);
    }
    fields_set = true;
    on_fields_set();
  }
  bool complete() const { return fields_set; }
  Json::Value to_json(Xref &x) const override {
    auto json = base_json(x);
    json["fields"] = Json::arrayValue;
    for (auto &f : fields()) {
      json["fields"].append(f.to_json(x));
    }
    return json;
  }
};

struct StructField {
  std::string name;
  // Must be non-null. Pointer only because juggling references inside
  // containers is a pain.
  Type *type;
  size_t offset;

  Json::Value to_json(Xref &x) const {
    Json::Value json;
    json["name"] = name;
    json["type"] = x[type];
    json["offset"] = offset;
    return json;
  }
};

class Struct : public Fielded<StructField> {
  friend Root;
  using Fielded::Fielded;
  void on_fields_set() override {
    if (!fields().size()) {
      byte.set(0, 1);
    } else {
      const auto &last = fields().back();
      auto size = last.offset + last.type->byte.size();
      auto align = alignment();
      if (size % align != 0)
        size += align - size % align;
      byte.set(size, align);
    }
  }
  const char *kind() const override { return "struct"; }
};

struct UnionField {
  std::string name;
  Type *type; // non-null

  Json::Value to_json(Xref &x) const {
    Json::Value json;
    json["name"] = name;
    json["type"] = x[type];
    return json;
  }
};

class Union : public Fielded<UnionField> {
  friend Root;
  using Fielded::Fielded;
  void on_fields_set() override {
    size_t size = 0;
    for (const auto &f : fields()) {
      if (f.type->byte.size() > size)
        size = f.type->byte.size();
    }
    byte.set(size, alignment());
  }
  const char *kind() const override { return "union"; }
};

template<bool copy_byte, typename T> class FieldedTyped : public Fielded<T> {
protected:
  FieldedTyped(const std::string &n, Type &t)
    :Fielded<T>{n}, type{t}
  {
    if (copy_byte)
      this->byte.set(type.byte.size(), type.byte.alignment());
  }
public:
  Type &type;
  Json::Value to_json(Xref &x) const override {
    auto json = Fielded<T>::to_json(x);
    json["type"] = x[&type];
    return json;
  }
};

struct EnumField {
  std::string name;
  long value;

  Json::Value to_json(Xref &) const {
    Json::Value json;
    json["name"] = name;
    json["value"] = value;
    return json;
  }
};

class Enum : public FieldedTyped<true, EnumField> {
  friend Root;
  using FieldedTyped::FieldedTyped;
  const char *kind() const override { return "enum"; }
};

struct Parameter {
  std::string name;
  Type *type; // non-null

  Json::Value to_json(Xref &x) const {
    Json::Value json;
    json["name"] = name;
    json["type"] = x[type];
    return json;
  }
};

class Thiscall : public FieldedTyped<false, Parameter> {
  friend Root;
  using FieldedTyped::FieldedTyped;
  const char *kind() const override { return "thiscall"; }
};

struct BitfieldField {
  std::string name;
  int size;
  int offset;

  Json::Value to_json(Xref &) const {
    Json::Value json;
    json["name"] = name;
    json["size"] = size;
    json["offset"] = offset;
    return json;
  }
};

class Bitfield : public FieldedTyped<true, BitfieldField> {
  friend Root;
  using FieldedTyped::FieldedTyped;
  const char *kind() const override { return "bitfield"; }
};

class Root : Namer {
  std::map<std::string, std::unique_ptr<Type>> name_to_type;
  template<typename T, typename ... Cs>
  T &create(const std::string &n, Cs... cs) {
    try {
      auto name = n.empty() ? unnamed() : n;
      if (name_to_type.count(name) != 0)
        die(name + " name collision");
      std::unique_ptr<T> ptr{new T{name, cs...}};
      name_to_type[name] = std::move(ptr);
      return *dynamic_cast<T *>(name_to_type.at(name).get());
    } catch(const Error &e) {
      e.context(typeid(T).name());
    }
  }
  Basic &basic(const char *name, ByteSizeAlignment b) {
    return create<Basic>(std::string{name}, b);
  }
  template<typename T> Basic &basic(const char *name) {
    return basic(name, ByteSizeAlignment{sizeof(T), alignof(T)});
  }
public:
  Xref xref;

  Basic &void_type = basic("void", ByteSizeAlignment{});
  Basic &undefined_type = basic("undefined", ByteSizeAlignment{});
  Basic &undefined1_type = basic("undefined1", ByteSizeAlignment{1, 1});
  Basic &undefined2_type = basic("undefined2", ByteSizeAlignment{2, 2});
  Basic &undefined4_type = basic("undefined4", ByteSizeAlignment{4, 4});
  Basic &undefined8_type = basic("undefined8", ByteSizeAlignment{8, 8});
  Basic &int8_type = basic<int8_t>("int8_t");
  Basic &int16_type = basic<int16_t>("int16_t");
  Basic &int32_type = basic<int32_t>("int32_t");
  Basic &int64_type = basic<int64_t>("int64_t");
  Basic &uint8_type = basic<uint8_t>("uint8_t");
  Basic &uint16_type = basic<uint16_t>("uint16_t");
  Basic &uint32_type = basic<uint32_t>("uint32_t");
  Basic &uint64_type = basic<uint64_t>("uint64_t");
  Basic &size_type = basic<size_t>("size_t");
  Basic &bool_type = basic<bool>("bool");
  Basic &char_type = basic<char>("char");
  Basic &short_type = basic<short>("short");
  Basic &int_type = basic<int>("int");
  Basic &long_type = basic<long>("long");
  Basic &uchar_type = basic<unsigned char>("uchar");
  Basic &ushort_type = basic<unsigned short>("ushort");
  Basic &uint_type = basic<unsigned int>("uint");
  Basic &ulong_type = basic<unsigned long>("ulong");
  Basic &float_type = basic<float>("float");
  Basic &double_type = basic<double>("double");

  // In C++14 these could be template aliases.
  template<typename ... Cs>
  Struct &new_struct(const std::string &name, Cs... cs) {
    return create<Struct>(name, cs...);
  }
  template<typename ... Cs>
  Union &new_union(const std::string &name, Cs... cs) {
    return create<Union>(name, cs...);
  }
  template<typename ... Cs>
  Enum &new_enum(const std::string &name, Cs... cs) {
    return create<Enum>(name, cs...);
  }
  template<typename ... Cs>
  Thiscall &new_thiscall(const std::string &name, Cs... cs) {
    return create<Thiscall>(name, cs...);
  }
  template<typename ... Cs>
  Bitfield &new_bitfield(const std::string &name, Cs... cs) {
    return create<Bitfield>(name, cs...);
  }

  bool have(const std::string &name) {
    return name_to_type.count(name);
  }

  Type &operator[](const std::string &name) {
    if (!have(name))
      die(name + " does not exist");
    return *name_to_type.at(name);
  }

  Json::Value to_json() {
    Json::Value json;
    size_t i = 0;
    while (i < xref.types().size())
      json.append(xref.types()[i++]->to_json(xref));
    return json;
  }
};
} // namespace Export
} // namespace DFHack
