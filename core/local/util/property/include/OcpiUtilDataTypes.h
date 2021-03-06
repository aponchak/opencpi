
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Non-API datatype declarations, for protocols and properties etc.
 */
#ifndef OCPI_UTIL_DATA_TYPES_H
#define OCPI_UTIL_DATA_TYPES_H

#include "ezxml.h"
#include <stdarg.h>
#include <string>
#include "OcpiUtilDataTypesApi.h"

namespace OCPI {
  namespace Util {
    // Since arrays are part of the declarator, and not the type, a member with both
    // sequence and array characteristics is a sequence of arrays, not an array of sequences.
    // I.e. if it is an array of sequences, then the member must refer to a subsidiary type,
    // roughly like a typedef. (the BaseType is then OCPI_Type)
    class Member;
    // The class that defines the type of a data value.  Could have been called DataType.
    class ValueType {
    public:
      OCPI::API::BaseType m_baseType; // The basic type - scalar, string, struct or typedef
      size_t
        m_arrayRank,                  // If > 0, we have an array
	m_nMembers;                   // For structs
      size_t 
	m_dataAlign,                  // Alignment of data for this type (i.e. not sequence count)
	m_align,                      // The alignment requirement for this type
	m_nBits;
      bool m_isSequence;              // Are we a sequence?
      size_t
        m_nBytes,                     // Total bytes (if bounded), minimum bytes if unbounded
	*m_arrayDimensions,           // != NULL for an array, m_arrayRank long
        m_stringLength,               // maximum strlen (null not included, like strlen)
	m_sequenceLength;             // maximum length for sequences, zero is unbounded
      Member *m_members;              // if a struct, these are the members
      Member *m_type;                 // if a recursive type, that type
      const char **m_enums;
      size_t m_nEnums;
      size_t m_nItems;                // total number of fixed items
      std::string m_typeDef;          // If we were created from a typedef
      std::string m_format;
      // unions and enums
      ValueType(OCPI::API::BaseType bt = OCPI::API::OCPI_none, bool isSequence = false);
      ~ValueType();
      bool isSequence() const { return m_isSequence; }
      // Return whether this value is fixed in size
      // If top == true, its ok for it to be a sequence
      bool isFixed(bool top = true) const;
    };
    const unsigned testMaxStringLength = 10;
    const unsigned maxDataTypeAlignment = 16;
    union WriteDataPtr {
      const uint8_t *data;
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,u,bits,run,pretty,store) const run *p##pretty;
#define OCPI_DATA_TYPE_S(sca,corba,u,bits,run,pretty,store) const run p##pretty;
      OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
    };
    union ReadDataPtr {
      uint8_t *data;
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE(sca,corba,u,buts,run,pretty,store) run *p##pretty;
#define OCPI_DATA_TYPE_S(sca,corba,u,buts,run,pretty,store) run p##pretty;
      OCPI_DATA_TYPES
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE
    };
    class Writer {
    protected:
      Writer();
      virtual ~Writer();
    public:      
      virtual void 
	writeOpcode(const char *name, uint8_t opcode),
	beginSequence(Member &m, size_t nElements) = 0,
	beginArray(Member &m, size_t nItems),
	endArray(Member &m ),
	endSequence(Member &m),
	beginStruct(Member &m),
	endStruct(Member &m),
	beginType(Member &m),
	endType(Member &m),
	writeString(Member &m, WriteDataPtr p, size_t strLen, bool start, bool top) = 0,
	writeData(Member &m, WriteDataPtr p, size_t nBytes, size_t nElements) = 0,
	end();
    };
    class Reader {
    protected:
      Reader();
      virtual ~Reader();
    public:      
      virtual size_t
	beginSequence(Member &m) = 0,
	beginString(Member &m, const char *&chars, bool first) = 0;
      virtual void 
	beginArray(Member &m, size_t nItems),
	endArray(Member &m ),
	endSequence(Member &m),
	endString(Member &m),
	beginStruct(Member &m),
	endStruct(Member &m),
	beginType(Member &m),
	endType(Member &m),
	readData(Member &m, ReadDataPtr p, size_t nBytes, size_t nElements) = 0,
	end();
    };
    // There are the data type attributes allowed for members
#define OCPI_UTIL_MEMBER_ATTRS						\
    "Name", "Type", "StringLength", "SequenceLength", "ArrayLength", "ArrayDimensions", "Key", "Enums"

    // A "member" is used for structure members, operation arguments, exception members, 
    // and properties.  Members have names, and offsets in their group, and possibly a
    // default value
    class Value;
    class Member : public ValueType {
    public:
      std::string m_name, m_abbrev;
      std::string m_description;
      size_t m_offset;              // in group
      bool m_isIn, m_isOut, m_isKey;  // for arguments (could use another class, but not worth it)
      Value *m_default;               // A default value, if one is appropriate and there is one
      unsigned m_ordinal;             // ordinal within group
      Member();
      Member(const char *name, const char *abbrev, const char *description, OCPI::API::BaseType type,
	     bool isSequence, const char *defaultValue);
      virtual ~Member();
      void printAttrs(FILE *f, const char *tag, unsigned indent = 0);
      void printChildren(FILE *f, const char *tag, unsigned indent = 0);
      void printXML(FILE *f, const char *tag, unsigned indent);
      void write(Writer &writer, const uint8_t *&data, size_t &length, bool topSeq = false);
      void read(Reader &reader, uint8_t *&data, size_t &length);
      void generate(const char *name, unsigned ordinal = 0, unsigned depth = 0);
      const char *
	parse(ezxml_t xp, bool isFixed, bool hasName, const char *hasDefault, unsigned ordinal);
      const char *
      offset(size_t &maxAlign, size_t &argOffset,
	     size_t &minSize, bool &diverseSizes, bool &sub32, bool &unBounded, bool isTop = false);
      static const char *
      parseMembers(ezxml_t prop, size_t &nMembers, Member *&members,
		   bool isFixed, const char *tag, const char *vtag);
      static const char *
      alignMembers(Member *m, size_t nMembers,
		   size_t &maxAlign, size_t &myOffset,
		   size_t &minSize, bool &diverseSizes, bool &sub32, bool &unBounded, bool isTop = false);
    };
    // These two are indexed by the BaseType
    extern const char *baseTypeNames[];
    extern const char *idlTypeNames[];
    extern unsigned baseTypeSizes[];
  }
}
#endif
