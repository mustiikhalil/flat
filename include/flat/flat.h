#ifndef flat_h
#define flat_h

#include <cctype>
#include <iostream>
#include <string>

#include "JSON.h"
#include "variables.h"

namespace flat {

struct PrintScalarTag {};
struct PrintPointerTag {};
template<typename T> struct PrintTag { typedef PrintScalarTag type; };
template<> struct PrintTag<const void *> { typedef PrintPointerTag type; };

class FLAT {
 private:
  Parser parser;
  std::string gen_code;
  int inden_lvl = 0;

  void incrementIndentation() { inden_lvl += 2; }
  void decrementIndentation() {
    if (inden_lvl) inden_lvl -= 2;
  }

  void write(const std::string field_name, std::string &text,
             const std::string value) {
    write(field_name, text, value, "- ", ": ", "\n", inden_lvl);
  }

  void write(const std::string field_name, std::string &text,
             const std::string value, const std::string start,
             const std::string separator, const std::string new_line,
             int indent_by) {
    text += (std::string(indent_by, ' ') + start + field_name + separator +
             value + new_line);
  }

  /// Taken from  flatbuffers writing format to json
  template<typename T> std::string PrintScalar(T val, const Type &type) {
    if (IsBool(type.base_type)) { return val != 0 ? "true" : "false"; }
    if (type.enum_def) {
      const auto &enum_def = *type.enum_def;
      if (auto ev = enum_def.ReverseLookup(static_cast<int64_t>(val))) {
        return ev->name;
      }
    }
    return flatbuffers::NumToString(val);
  }

  template<typename T>
  std::string PrintVector(const void *val, const Type &type, int indent,
                          const uint8_t *prev_val) {
    typedef flatbuffers::Vector<T> Container;
    typedef typename PrintTag<typename Container::return_type>::type tag;
    auto &vec = *reinterpret_cast<const Container *>(val);
    return PrintContainer<Container>(tag(), vec, vec.size(), type, indent,
                                     prev_val);
  }

  template<typename Container>
  std::string PrintContainer(PrintScalarTag, const Container &c, size_t size,
                             const Type &type, int indent, const uint8_t *) {
    std::string text = "\n";
    write("", text, "", "[", "", "\n", indent);
    for (uoffset_t i = 0; i < size; i++) {
      std::string txt = PrintScalar(c[i], type);
      write("", text, txt, ":: ", "", ",\n", indent + 2);
    }
    write("", text, "", "]", "", "\n", indent);
    return text;
  }

  template<typename Container>
  std::string PrintContainer(PrintPointerTag, const Container &c, size_t size,
                             const Type &type, int indent,
                             const uint8_t *prev_val) {
    std::string text = "\n";
    const auto is_struct = IsStruct(type);
    write("", text, "", "[", "", "\n", indent);
    incrementIndentation();
    for (uoffset_t i = 0; i < size; i++) {
      auto ptr = is_struct ? reinterpret_cast<const void *>(
                                 c.Data() + type.struct_def->bytesize * i)
                           : c[i];
      std::string txt =
          ReadFields(ptr, type, prev_val, static_cast<soffset_t>(i), false);
      write("", text, txt, ":: ", "", ",\n", indent + 2);
    }
    decrementIndentation();
    write("", text, "", "]", "", "\n", indent);
    return text;
  }

  template<typename T> static T GetFieldDefault(const FieldDef &fd) {
    T val;
    auto check = flatbuffers::StringToNumber(fd.value.constant.c_str(), &val);
    (void)check;
    return val;
  }

  // Generate text for a scalar field.
  template<typename T>
  std::string GenField(const FieldDef &fd, const Table *table, bool fixed) {
    return PrintScalar(
        fixed ? reinterpret_cast<const Struct *>(table)->GetField<T>(
                    fd.value.offset)
              : table->GetField<T>(fd.value.offset, GetFieldDefault<T>(fd)),
        fd.value.type);
  }

  std::string ReadScalarFields(const FieldDef &fd, const Table *table,
                               const StructDef &struct_def,
                               const uint8_t *prev_val, const bool fixed) {
    switch (fd.value.type.base_type) {
      case flatbuffers::BASE_TYPE_NONE:
        return GenField<uint8_t>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_UTYPE:
        return GenField<uint8_t>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_BOOL: return GenField<bool>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_CHAR:
        return GenField<int8_t>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_UCHAR:
        return GenField<uint8_t>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_SHORT:
        return GenField<short>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_USHORT:
        return GenField<ushort>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_INT: return GenField<int>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_UINT: return GenField<uint>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_LONG:
        return GenField<int64_t>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_ULONG:
        return GenField<uint64_t>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_FLOAT:
        return GenField<float>(fd, table, fixed);
      case flatbuffers::BASE_TYPE_DOUBLE:
        return GenField<double>(fd, table, fixed);
      default: return GetOffset(fd, table, struct_def, prev_val, fixed);
    }
  }

  std::string ReadVectorContants(const void *val, const Type &type, int indent,
                                 const uint8_t *prev_val) {
    switch (type.base_type) {
      case flatbuffers::BASE_TYPE_NONE:
        return PrintVector<uint8_t>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_UTYPE:
        return PrintVector<uint8_t>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_BOOL:
        return PrintVector<bool>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_CHAR:
        return PrintVector<int8_t>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_UCHAR:
        return PrintVector<uint8_t>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_SHORT:
        return PrintVector<short>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_USHORT:
        return PrintVector<ushort>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_INT:
        return PrintVector<int>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_UINT:
        return PrintVector<uint>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_LONG:
        return PrintVector<int64_t>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_ULONG:
        return PrintVector<uint64_t>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_FLOAT:
        return PrintVector<float>(val, type, indent, prev_val);
      case flatbuffers::BASE_TYPE_DOUBLE:
        return PrintVector<double>(val, type, indent, prev_val);
      default:
        return PrintVector<flatbuffers::Offset<void>>(val, type, indent,
                                                      prev_val);
    }
  }

  std::string GetOffset(const FieldDef &fd, const Table *table,
                        const StructDef &struct_def, const uint8_t *prev_val,
                        const bool fixed) {
    const void *val = nullptr;
    if (fixed) {
      val = reinterpret_cast<const Struct *>(table)->GetStruct<const void *>(
          fd.value.offset);
    } else {
      val = IsStruct(fd.value.type)
                ? table->GetStruct<const void *>(fd.value.offset)
                : table->GetPointer<const void *>(fd.value.offset);
    }
    return ReadFields(val, fd.value.type, prev_val, -1);
  }

  std::string ReadFields(const void *val, const Type &type,
                         const uint8_t *prev_val,
                         flatbuffers::soffset_t vector_index,
                         bool should_include_separator = true) {
    std::string text;
    switch (type.base_type) {
      case flatbuffers::BASE_TYPE_STRING: {
        auto s = reinterpret_cast<const String *>(val);
        flatbuffers::EscapeString(s->c_str(), s->size(), &text, false, false);
        return text;
      }
      case flatbuffers::BASE_TYPE_STRUCT: {
        std::string text = "\n";
        std::string type_ = flatbuffers::IsStruct(type) ? "Struct" : "Table";
        incrementIndentation();
        auto name = type.struct_def->name;
        if (should_include_separator) {
          write(type_, text, name + " -");
        } else {
          text = "";
          write(type_, text, name, "", ": ", "\n", 0);
        }
        GenerateBody(reinterpret_cast<const Table *>(val), *type.struct_def,
                     text);
        decrementIndentation();
        return text.substr(0, text.size() - 1) + "";
      }
      case flatbuffers::BASE_TYPE_UNION: {
        auto union_type_byte = *prev_val;
        if (vector_index >= 0) {
          auto type_vec =
              reinterpret_cast<const flatbuffers::Vector<uint8_t> *>(
                  prev_val +
                  flatbuffers::ReadScalar<flatbuffers::uoffset_t>(prev_val));
          union_type_byte =
              type_vec->Get(static_cast<flatbuffers::uoffset_t>(vector_index));
        }
        auto enum_val = type.enum_def->ReverseLookup(union_type_byte, true);
        if (enum_val) {
          return ReadFields(val, enum_val->union_type, nullptr, -1);
        } else {
          return "UNKNOWN";
        }
      }
      case flatbuffers::BASE_TYPE_ARRAY: {
        // TODO: - Currently not needed however if required later on it should be implemented
        return "[]";
      }
      case flatbuffers::BASE_TYPE_VECTOR: {
        const auto vec_type = type.VectorType();
        return ReadVectorContants(val, vec_type, inden_lvl + 2, prev_val);
      };
      default: return "UNKNOWN";
    }
  }

  const Table *GetRoot(const void *flatbuffer) {
    return parser.opts.size_prefixed
               ? flatbuffers::GetSizePrefixedRoot<Table>(flatbuffer)
               : flatbuffers::GetRoot<Table>(flatbuffer);
  }

  void GenerateBody(const Table *table, const StructDef &struct_def,
                    std::string &text) {
    incrementIndentation();
    const uint8_t *ptr = nullptr;
    for (auto it = struct_def.fields.vec.begin();
         it < struct_def.fields.vec.end(); ++it) {
      FieldDef &field = **it;
      auto is_present =
          struct_def.fixed || table->CheckField(field.value.offset);
      if (is_present && !field.deprecated) {
        auto value =
            ReadScalarFields(field, table, struct_def, ptr, struct_def.fixed);
        write(field.name, text, value);
      }
      if (struct_def.fixed) {
        ptr = reinterpret_cast<const uint8_t *>(table) + field.value.offset;
      } else {
        ptr = table->GetAddressOf(field.value.offset);
      }
    }
    decrementIndentation();
  }

  void GenerateFlat(const Table *table, const StructDef &struct_def) {
    std::string text;
    write("RootType", text, struct_def.name + " -");
    GenerateBody(table, struct_def, text);
    gen_code += text;
    decrementIndentation();
    gen_code += "----";
  }

 public:
  FLAT(const std::string &table);
  std::string parse(const void *flatbuffer);
};

}  // namespace flat
#endif /* flat_h */
