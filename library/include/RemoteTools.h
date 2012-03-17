/*
https://github.com/peterix/dfhack
Copyright (c) 2009-2011 Petr Mrázek (peterix@gmail.com)

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
#include "Pragma.h"
#include "Export.h"
#include "RemoteServer.h"

#include "DataDefs.h"

#include "Basic.pb.h"

namespace df
{
    struct material;
}

namespace DFHack
{
    class MaterialInfo;

    using google::protobuf::RepeatedPtrField;

    DFHACK_EXPORT void strVectorToRepeatedField(RepeatedPtrField<std::string> *pf,
                                                const std::vector<std::string> &vec);

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

    /////

    using dfproto::BasicMaterialInfo;
    using dfproto::BasicMaterialInfoMask;

    DFHACK_EXPORT void describeMaterial(BasicMaterialInfo *info, df::material *mat,
                                        const BasicMaterialInfoMask *mask = NULL);
    DFHACK_EXPORT void describeMaterial(BasicMaterialInfo *info, const MaterialInfo &mat,
                                        const BasicMaterialInfoMask *mask = NULL);

    /////

    class CoreService : public RPCService {
        int suspend_depth;
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
    };
}
