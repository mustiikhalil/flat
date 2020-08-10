#include <string>

#include "flatbuffers/base.h"
#include "flatbuffers/flatbuffers.h"
#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#ifndef variables_h
#  define variables_h

namespace flat {

typedef flatbuffers::soffset_t soffset_t;
typedef flatbuffers::uoffset_t uoffset_t;
typedef flatbuffers::Parser Parser;
typedef flatbuffers::Table Table;
typedef flatbuffers::StructDef StructDef;
typedef flatbuffers::FieldDef FieldDef;
typedef flatbuffers::Type Type;
typedef flatbuffers::Struct Struct;
typedef flatbuffers::String String;

}  // namespace flat

#endif
