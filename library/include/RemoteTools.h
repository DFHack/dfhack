/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2012 Petr Mrázek (peterix@gmail.com)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

#pragma once
#include "Export.h"
#include "RemoteServer.h"

#include "DataDefs.h"

#include "Basic.pb.h"

namespace df
{
    struct material;
    struct unit;
    struct language_name;
}

namespace DFHack
{
    struct MaterialInfo;

    using google::protobuf::RepeatedField;
    using google::protobuf::RepeatedPtrField;

    DFHACK_EXPORT void strVectorToRepeatedField(RepeatedPtrField<std::string> *pf,
                                                const std::vector<std::string> &vec);

    using dfproto::StringListMessage;

    /**
     * Represent bitfield bits as a repeated string field.
     */
    template<class T>
    inline void bitfield_to_string(RepeatedPtrField<std::string> *pf, const T &val) {
        std::vector<std::string> tmp;
        bitfield_to_string<T>(&tmp, val);
        strVectorToRepeatedField(pf, tmp);
    }

    /**
     * Represent flagarray bits as a repeated string field.
     */
    template<class T>
    inline void flagarray_to_string(RepeatedPtrField<std::string> *pf, const BitArray<T> &val) {
        std::vector<std::string> tmp;
        flagarray_to_string<T>(&tmp, val);
        strVectorToRepeatedField(pf, tmp);
    }

    /**
     * Represent flagarray bits as a repeated int field.
     */
    template<class T>
    void flagarray_to_ints(RepeatedField<google::protobuf::int32> *pf, const BitArray<T> &val) {
        for (size_t i = 0; i < val.size*8; i++)
            if (val.is_set(T(i)))
                pf->Add(i);
    }

    using dfproto::EnumItemName;

    DFHACK_EXPORT void describeEnum(RepeatedPtrField<EnumItemName> *pf, int base,
                                    int size, const char* const *names);

    template<class T>
    void describe_enum(RepeatedPtrField<EnumItemName> *pf)
    {
        typedef df::enum_traits<T> traits;
        int base = traits::first_item;
        int size = (int)traits::last_item - base + 1;
        describeEnum(pf, base, size, traits::key_table);
    }

    DFHACK_EXPORT void describeBitfield(RepeatedPtrField<EnumItemName> *pf,
                                        int size, const bitfield_item_info *items);

    template<class T>
    void describe_bitfield(RepeatedPtrField<EnumItemName> *pf)
    {
        typedef df::bitfield_traits<T> traits;
        describeBitfield(pf, traits::bit_count, traits::bits);
    }

    /////

    using dfproto::BasicMaterialInfo;
    using dfproto::BasicMaterialInfoMask;

    DFHACK_EXPORT void describeMaterial(BasicMaterialInfo *info, df::material *mat,
                                        const BasicMaterialInfoMask *mask = NULL);
    DFHACK_EXPORT void describeMaterial(BasicMaterialInfo *info, const MaterialInfo &mat,
                                        const BasicMaterialInfoMask *mask = NULL);

    using dfproto::NameInfo;

    DFHACK_EXPORT void describeName(NameInfo *info, df::language_name *name);

    using dfproto::NameTriple;

    DFHACK_EXPORT void describeNameTriple(NameTriple *info, const std::string &name,
                                          const std::string &plural, const std::string &adj);

    using dfproto::BasicUnitInfo;
    using dfproto::BasicUnitInfoMask;

    DFHACK_EXPORT void describeUnit(BasicUnitInfo *info, df::unit *unit,
                                    const BasicUnitInfoMask *mask = NULL);

    /////

    class CoreService : public RPCService {
        int suspend_depth;
        CoreSuspender* coreSuspender;

        static int doRunLuaFunction(lua_State *L);
    public:
        CoreService();
        ~CoreService();

        command_result BindMethod(color_ostream &stream,
                                  const dfproto::CoreBindRequest *in,
                                  dfproto::CoreBindReply *out);
        command_result RunCommand(color_ostream &stream,
                                  const dfproto::CoreRunCommandRequest *in);

        // For batching
        command_result CoreSuspend(color_ostream &stream, const EmptyMessage*, IntMessage *cnt);
        command_result CoreResume(color_ostream &stream, const EmptyMessage*, IntMessage *cnt);

        command_result RunLua(color_ostream &stream,
                              const dfproto::CoreRunLuaRequest *in,
                              StringListMessage *out);
    };
}
