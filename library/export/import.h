#pragma once

// "import" of DFHack::type_identity for export.

#include "Core.h"
#include "DataDefs.h"
#include "DataIdentity.h"
#include "VersionInfo.h"

#include "label.h"

#pragma GCC diagnostic error "-Wall"
#pragma GCC diagnostic error "-Wextra"

namespace DFHack {
namespace Export {
class Platform {
  virtual Type &make_std_vector(Type &) = 0;
  virtual Type &make_std_string() = 0;
  template<typename T> Type &check(Type &t) {
    if (t.byte.size() != sizeof(T))
      die("platform type size mismatch");
    if (t.byte.alignment() != alignof(T))
      die("platform type alignment mismatch");
    return t;
  }
  Platform(const Platform &) = delete;
  Platform &operator=(const Platform &) = delete;
  Platform(Platform &&) = delete;
  Platform &operator=(Platform &&) = delete;
protected:
  Platform() = default;
public:
  ~Platform() = default;
  Type &std_vector(Type &t) {
    return check<std::vector<void *>>(make_std_vector(t));
  }
  Type &std_string() {
    return check<std::string>(make_std_string());
  }
};

class Linux : public Platform {
  Root &r;

  Type *string = NULL;

  Type &make_std_string() override {
    if (!string) {
      auto &st = r.new_struct(std::string{"std_string_Rep"});
      std::vector<StructField> fields;
      fields.push_back(StructField {
          std::string{"_M_length"}, &r.size_type, 0});
      fields.push_back(StructField {
          std::string{"_M_capacity"}, &r.size_type, sizeof(void *)});
      fields.push_back(StructField {
          std::string{"_M_refcount"}, &r.int_type, 2 * sizeof(void *)});
      st.set_fields(fields);
      string = &st.pointer();
    }
    return *string;
  }

  std::map<Type *, Type *> item_to_vector;

  Type &make_std_vector(Type &t) override {
    if (!item_to_vector.count(&t)) {
      auto &st = r.new_struct(t.name().insert(0, "std_vector_"));
      std::vector<StructField> fields;
      fields.push_back(StructField {
          std::string{"_M_start"}, &t.pointer(), 0});
      fields.push_back(StructField {
          std::string{"_M_finish"}, &t.pointer(), sizeof(void *)});
      fields.push_back(StructField {
          std::string{"_M_end_of_storage"}, &t.pointer(), 2 * sizeof(void *)});
      st.set_fields(fields);
      item_to_vector[&t] = &st;
    }
    return *item_to_vector.at(&t);
  }

public:
  Linux(Root &root) :r{root} {}
};

struct Import {
  Root r{};
  Namespace ns = Namespace::new_root("dfhack");
  const std::shared_ptr<const VersionInfo> vinfo = Core::getInstance().vinfo;

  explicit Import() {
    if (vinfo->getOS() == OS_LINUX)
      platform.reset(new Linux{r});
    else
      die("no platform types for your OS");
  }

  Json::Value to_json() {
    try {
      complete_vtts();
    } catch(const Error &e) {
      e.context("completing vtts");
    }
    Json::Value json;
    try {
      json["namespace"] = ns.to_json(r.xref);
    } catch(const Error &e) {
      e.context("converting namespaces to json");
    }
    try {
      json["types"] = r.to_json();
    } catch(const Error &e) {
      e.context("converting types to json");
    }
    return json;
  }

  Type &import(const type_identity &id) {
    const auto type_name = id.getFullName();
    if (!typedefs.count(type_name)) {
      if (typedefs.count(type_name))
        die("!!!");
      if (r.have(type_name))
        save_type_alias(type_name, r[type_name], id);
      else
        save_type_alias(type_name, import_inner(id), id);
    }
    // TBD: Make these have pointer identity. Please don't just match
    // regex the type names..
    if (type_name != "BitArray<>"
        && type_name != "vector<int8_t>"
        && type_name != "vector<int16_t>"
        && type_name != "vector<int32_t>"
        && type_name != "vector<int32_t>"
        && type_name != "int8_t[16]") {
      if (type_ids.at(type_name) != &id)
        die(type_name + " type_identity name collision");
    }
    return *typedefs[type_name];
  }

  Type &import(const type_identity *id) {
    if (!id)
      die("tried to import NULL");
    return import(*id);
  }

private:
  std::unique_ptr<Platform> platform;

  void save_type_alias(const std::string &n, Type &t,
                       const type_identity &id) {
    // if (typedefs.count(n))
    //   die(t.name() + " alias collision against " + typedefs[n]->name());
    typedefs[n] = &t;
    type_ids[n] = &id;
  }

  std::map<std::string, Type *> typedefs;
  std::map<std::string, const type_identity *> type_ids;

  Type &make_opaque(const std::string &name, size_t size) {
    auto &s = r.new_struct(name);
    Type *type;
    // At least if the type is ever used in a field the alignment
    // check there will catch a wrong guess here.
    if (size % 8 == 0)
      type = &r.undefined8_type.array(size / 8);
    else if (size % 4 == 0)
      type = &r.undefined4_type.array(size / 4);
    else if (size % 2 == 0)
      type = &r.undefined2_type.array(size / 2);
    else
      type = &r.undefined1_type.array(size);
    s.set_fields(std::vector<StructField>{StructField{
          std::string{"opaque_make_me_a_platform_type"}, type, 0}});
    return s;
  }

  Type &make_opaque(const type_identity &id) {
    return make_opaque(id.getFullName(), id.byte_size());
  }

  Type &import_inner(const type_identity &id) {
    const auto type_name = id.getFullName();
    try {
      switch (id.type()) {
      case IDTYPE_GLOBAL:
        return load_global(cast<global_identity>(id));

      case IDTYPE_FUNCTION:
        die("Import::import doesn't handle functions");

      case IDTYPE_PRIMITIVE:
        if (type_name == "string")
          return platform->std_string();
        break;

      case IDTYPE_POINTER: {
        auto ptr = cast<pointer_identity>(id);
        if (ptr.getTarget())
          return import(ptr.getTarget()).pointer();
        else
          return r.void_type.pointer();
      }

      case IDTYPE_CONTAINER: {
        auto &c = cast<df::stl_container_base_identity>(id);
        if (!c.name)
          die("unnamed STL type");
        auto &item_type = import(c.getItemType());
        if (!strcmp(c.name, "vector"))
          return platform->std_vector(item_type);

        // Would be nice to move these into Platform.
        if (!strcmp(c.name, "set"))
          return make_opaque(item_type.name().insert(0, "std_set_"),
                             c.byte_size());
        if (!strcmp(c.name, "deque"))
          return make_opaque(item_type.name().insert(0, "std_deque_"),
                             c.byte_size());

        if (!strcmp(c.name, "DfArray")) {
          auto &st = r.new_struct(std::string{"df_array_"}
                                    + item_type.name());
          std::vector<StructField> fields;
          fields.push_back(StructField{
              std::string{"m_data"}, &item_type.pointer(), 0});
          fields.push_back(StructField{
              std::string{"m_size"}, &r.ushort_type, sizeof(void *)});
          st.set_fields(fields);
          return st;
        }
        break;
      }

      case IDTYPE_PTR_CONTAINER:
        break;

      case IDTYPE_BIT_CONTAINER: {
        if (type_name == "BitArray<>") {
          auto &s = r.new_struct("bit_array");
          std::vector<StructField> fields;
          fields.push_back(StructField{
              std::string{"bits"}, &r.uint8_type.pointer(), 0});
          fields.push_back(StructField{
              std::string{"size"}, &r.uint32_type, sizeof(void *)});
          s.set_fields(fields);
          return s;
        } else if (type_name == "vector<bool>")
            return make_opaque(std::string{"std_vector_bool"}, id.byte_size());
        break;
      }

      case IDTYPE_BITFIELD: {
        auto &b = cast<bitfield_identity>(id);
        auto &bitted_type = import(b.base_type);
        std::vector<BitfieldField> fields;
        for (auto i = 0; i < b.getNumBits(); i++) {
          const auto &bi = b.getBits()[i];
          if (bi.size == 0) {
            if (bi.name)
              die("named zero bits");
            continue;
          }
          auto name = bi.name;
          if (!name)
            name = "";
          fields.push_back(BitfieldField{
              std::string{name}, bi.size, i});
        }
        auto &type = r.new_bitfield(type_name, std::ref(bitted_type));
        type.set_fields(fields);
        return type;
      }

      case IDTYPE_ENUM: {
        auto &e = cast<enum_identity>(id);
        Type *enumed_type;
        if (!e.getBaseType())
          enumed_type = &r.int32_type;
        else
          enumed_type = &import(e.getBaseType());
        std::vector<EnumField> fields;
        if (e.getComplex()) {
          for (int i = 0; i < e.getCount(); i++) {
            auto name = "";
            if (e.getKeys() && e.getKeys()[i])
              name = e.getKeys()[i];
            auto value = e.getComplex()->index_value_map[i];
            fields.push_back(EnumField{std::string{name}, value});
          }
        } else {
          for (int i = 0; i < e.getCount(); i++) {
            auto name = "";
            if (e.getKeys() && e.getKeys()[i])
              name = e.getKeys()[i];
            auto value = e.getFirstItem() + i;
            fields.push_back(EnumField{std::string{name}, value});
          }
        }
        auto &type = r.new_enum(type_name, std::ref(*enumed_type));
        type.set_fields(fields);
        return type;
      }

      case IDTYPE_STRUCT: {
        auto &sid = cast<struct_identity>(id);
        if (sid.is_union) {
          auto &u = r.new_union(type_name);
          std::vector<UnionField> fields;
          for (const auto &f : load_fields(sid))
            fields.push_back(UnionField{f.name, f.type});
          u.set_fields(fields);
          return u;
        } else {
          auto &s = r.new_struct(type_name);
          s.set_fields(load_fields(sid));
          return s;
        }
      }

      case IDTYPE_CLASS:
        return load_class(cast<virtual_identity>(id));

      case IDTYPE_BUFFER: {
        auto &b = cast<df::buffer_container_identity>(id);
        auto &item_type = import(b.getItemType());
        return item_type.array(b.getSize());
      }

      case IDTYPE_STL_PTR_VECTOR: {
        auto &v = cast<df::stl_ptr_vector_identity>(id);
        if (v.getItemType())
          return platform->std_vector(import(v.getItemType()).pointer());
        else
          return platform->std_vector(r.void_type.pointer());
      }

      case IDTYPE_OPAQUE:
        return make_opaque(id);
      }
      die("unhandled");
    } catch(const Error &e) {
      e.context(type_name + "=" + identity_type_str(id.type()));
    }
  }

  template<typename T> const T &cast(const type_identity &t) {
    const T *ptr = dynamic_cast<const T *>(&t);
    if (!ptr)
      die(t.getFullName() + " bad cast");
    return *ptr;
  }
  template<typename T> const T &cast(const Type &t) {
    const T *ptr = dynamic_cast<const T *>(&t);
    if (!ptr)
      die("bad cast");
    return *ptr;
  }
  template<typename T> const T &cast(const Type *t) {
    if (!t)
      die("null type");
    return cast<T>(*t);
  }

  Type &load_field_type(const struct_field_info &fi) {
    if (fi.custom)
      die("tried to make type of custom");

    switch(fi.mode) {
    case struct_field_info::END:
      die("unexpected end");

    case struct_field_info::PRIMITIVE:
      return import(fi.type);

    case struct_field_info::STATIC_STRING:
      if (fi.count != 0)
        return r.char_type.array(fi.count);
      else
        return r.char_type.pointer();

    case struct_field_info::POINTER:
      if (!fi.type)
        return r.void_type.pointer();
      else
        return import(fi.type).pointer();

    case struct_field_info::STATIC_ARRAY:
      return import(fi.type).array(fi.count);

    case struct_field_info::SUBSTRUCT:
    case struct_field_info::CONTAINER:
      return import(fi.type);

    case struct_field_info::STL_VECTOR_PTR:
      if (fi.type)
        return platform->std_vector(import(fi.type).pointer());
      else
        return platform->std_vector(r.void_type.pointer());

    case struct_field_info::OBJ_METHOD:
    case struct_field_info::CLASS_METHOD:
      break;
    }
    die("undecipherable");
  }

  std::vector<StructField> load_fields(const struct_identity &id,
                                       const bool check_byte=true) {
    const std::string type_name = id.getFullName();
    std::vector<StructField> sfs;
    for (const auto *fi = id.getFields();
         fi && fi->mode != struct_field_info::END;
         fi++) {
      if (fi->mode == struct_field_info::OBJ_METHOD
          || fi->mode == struct_field_info::CLASS_METHOD)
        continue;
      std::string name = "";
      if (fi->name)
        name = fi->name;
      const auto full_name = type_name + '.' + name;
      try {
        auto &type = load_field_type(*fi);
        if (check_byte && (fi->size != type.byte.size()
                           || fi->alignment != type.byte.alignment())) {
          auto fa = std::to_string(fi->alignment);
          auto fs = std::to_string(fi->size);
          auto ta = std::to_string(type.byte.alignment());
          auto ts = std::to_string(type.byte.size());
          die(type.name()+ ':' + typeid(type).name() + ' ' + ts + '/' + ta
              + " should be " + fs + '/' + fa);
        }
        sfs.push_back(StructField{name, &type, fi->offset});
      } catch (const Error &e) {
        e.context(full_name);
      }
    }
    return sfs;
  };

  Type &load_global(const global_identity &id) {
    // Global has a pointer's size/alignment for every field. TBD make
    // it not so since it'd be great to check the global sizes are
    // right.
    const auto sfs = load_fields(id, false);
    for (const auto &sf : sfs) {
      const auto address = vinfo->getAddress(sf.name);
      ns.labels.create(sf.name, sf.type, (char *)address);
    }
    return r.void_type;
  }

  // Assumes method name is <class-name>_<method-name>, and the
  // instance type name is <class-name>.
  Thiscall &own_method_type(Struct &instance, Pointer &parent_method_ptr) {
    auto &parent_method = cast<Thiscall>(parent_method_ptr.type);
    std::vector<Parameter> params = parent_method.fields();
    auto &parent_instance = cast<Struct>
      (cast<Pointer>(params.at(0).type).type);
    const auto parent_name = parent_instance.name();
    auto method_name = parent_method.name().substr(parent_name.size() + 1);
    auto &method = r.new_thiscall(instance.name() + "_" + method_name,
                                  std::ref(parent_method.type));
    params.at(0).type = &instance.pointer();
    method.set_fields(params);
    return method;
  }

  // Assumes vtt name is <class-name>_vtt.
  Type &complete_vtt(const virtual_identity &id) {
    const auto type_name = id.getFullName();
    const auto vtt_name = type_name + "_vtt";
    if (!r.have(vtt_name))
      r.new_struct(vtt_name);
    auto &vtt_type = const_cast<Struct &>(cast<Struct>(r[vtt_name]));
    if (vtt_type.complete())
      return vtt_type;
    auto &instance_type = const_cast<Struct &>(cast<Struct>(import(id)));
    const auto vtt_address = (char *)
      vinfo->getVTable(std::string{id.getOriginalName()})
      - vinfo->getRebaseDelta();

    auto *class_ns = &ns;
    std::vector<const virtual_identity *> ancestors;
    for (const auto *parent = id.getParent(); parent;
         parent = parent->getParent())
      ancestors.push_back(&cast<virtual_identity>(*parent));
    for (int i = ancestors.size() - 1; i >= 0; i--) {
      complete_vtt(*ancestors[i]);
      class_ns = &class_ns->namespaces[ancestors[i]->getFullName()];
    }
    class_ns = &class_ns->namespaces.create(type_name);

    std::vector<StructField> vtt_fields;
    size_t parent_method_count = 0;
    if (ancestors.size()) {
      auto &parent_type = import(ancestors.at(0));
      auto &parent_vtt = cast<Struct>(&r[parent_type.name() + "_vtt"]);
      parent_method_count = parent_vtt.fields().size();
      const auto &parent_id = cast<virtual_identity>(*id.getParent());
      for (size_t i = 0; i < parent_method_count; i++) {
        const auto own_ptr = id.get_original_vmethod_ptr(i);
        auto &parent_mptr = cast<Pointer>(parent_vtt.fields().at(i).type);
        const auto &own_mtype = own_method_type
          (instance_type, const_cast<Pointer &>(parent_mptr));
        if (own_ptr != parent_id.get_original_vmethod_ptr(i)) {
          class_ns->labels.create(own_mtype.name(),
                                  const_cast<Thiscall *>(&own_mtype),
                                  (char *) own_ptr);
        }
        vtt_fields.push_back(StructField{own_mtype.name(),
              &const_cast<Thiscall *>(&own_mtype)->pointer(),
              sizeof(void *) * vtt_fields.size()});
      }
    }

    const auto &oms = *id.own_method_signatures;
    const auto own_method_count = oms.size();
    for (size_t i = 0; i < own_method_count; i++) {
      std::string method_name = "";
      if (id.own_method_names[i])
        method_name = id.own_method_names[i];
      if (method_name.empty())
        method_name = type_name + "_"
          + std::to_string(parent_method_count + i + 1);
      Type *return_type = NULL;
      std::vector<Parameter> params;
      params.push_back(Parameter{std::string{"this"},
            &instance_type.pointer()});
      for (const auto &fi : oms.at(i)) {
        auto &param_type = load_field_type(fi);
        std::string param_name = "";
        if (fi.name && strcmp(fi.name, "$unnamed"))
          param_name = fi.name;
        if (param_name == "$return") {
          if (return_type)
            die(type_name + '.' + method_name + " multiple $returns");
          return_type = &param_type;
          continue;
        }
        params.push_back(Parameter{param_name, &param_type});
      }
      if (!return_type) {
        if (params.empty() && method_name.empty())
          return_type = &r.undefined_type;
        else
          return_type = &r.void_type;
      }
      auto &method_type = r.new_thiscall(type_name + '_' + method_name,
                                         std::ref(*return_type));
      method_type.set_fields(params);
      vtt_fields.push_back(StructField{method_name, &method_type.pointer(),
            sizeof(void *) * vtt_fields.size()});
      class_ns->labels.create(method_type.name(), &method_type,
                              (char *) id.get_original_vmethod_ptr
                              (parent_method_count + i));
    }
    vtt_type.set_fields(vtt_fields);
    class_ns->labels.create(vtt_name, &vtt_type, vtt_address);

    return vtt_type;
  }

  bool vtts_pending = false;
  std::set<const virtual_identity *> delayed_vtts;
  void delay_vtt(const virtual_identity *id) {
    vtts_pending = true;
    delayed_vtts.insert(id);
  }

  Type &load_class(const virtual_identity &id) {
    const auto type_name = id.getFullName();
    const auto vtt_name = type_name + "_vtt";

    if (!id.own_method_names || !id.own_method_signatures)
      die("null methods on class: a dfhack custom class?");

    auto &instance_type = r.new_struct(type_name);
    auto &vtt_type = r.new_struct(vtt_name);
    delay_vtt(&id);

    std::vector<StructField> instance_fields{
      StructField{std::string{"__vtt"}, &vtt_type.pointer(), 0}
    };
    auto sfs = load_fields(id);
    instance_fields.insert(instance_fields.end(), sfs.begin(), sfs.end());
    instance_type.set_fields(instance_fields);

    return instance_type;
  }

  void complete_vtts() {
    while (vtts_pending) {
      vtts_pending = false;
      for (auto id : delayed_vtts)
        complete_vtt(*id);
    }
  }
};

Import load_df() {
  Import import;
  for (const auto *top : compound_identity::getTopScope()) {
    if (top->getFullName().find("dfhack") == std::string::npos)
      import.import(*top);
  }
  return import;
}
} // namespace DFHack
} // namespace Export
