/*
 * This file and the companion PluginStatics.cpp contain static structures used
 * by DFHack plugins. Linking them here, into the dfhack library, instead of
 * into the plugins themselves allows the plugins to be freely unloaded and
 * reloaded without fear of causing cached references to static data becoming
 * corrupted.
 */

#pragma once

#include "DataIdentity.h"

namespace DFHack {

// xlsxreader definitions
struct DFHACK_EXPORT xlsx_file_handle_identity : public compound_identity {
    xlsx_file_handle_identity()
        :compound_identity(0, nullptr, nullptr, "xlsx_file_handle") {};
    DFHack::identity_type type() override { return IDTYPE_OPAQUE; }
};

struct DFHACK_EXPORT xlsx_sheet_handle_identity : public compound_identity {
    xlsx_sheet_handle_identity()
        :compound_identity(0, nullptr, nullptr, "xlsx_sheet_handle") {};
    DFHack::identity_type type() override { return IDTYPE_OPAQUE; }
};

struct DFHACK_EXPORT xlsx_file_handle {
    typedef struct xlsxio_read_struct* xlsxioreader;
    const xlsxioreader handle;
    xlsx_file_handle(xlsxioreader handle): handle(handle) {}
    static xlsx_file_handle_identity _identity;
};

struct DFHACK_EXPORT xlsx_sheet_handle {
    typedef struct xlsxio_read_sheet_struct* xlsxioreadersheet;
    const xlsxioreadersheet handle;
    xlsx_sheet_handle(xlsxioreadersheet handle): handle(handle) {}
    static xlsx_sheet_handle_identity _identity;
};

}
