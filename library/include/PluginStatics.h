#pragma once

#include "DataIdentity.h"

namespace DFHack {

// xlsxreader definitions
struct xlsx_file_handle_identity : public compound_identity {
    xlsx_file_handle_identity()
        :compound_identity(0, nullptr, nullptr, "xlsx_file_handle") {};
    DFHack::identity_type type() override { return IDTYPE_OPAQUE; }
};

struct xlsx_sheet_handle_identity : public compound_identity {
    xlsx_sheet_handle_identity()
        :compound_identity(0, nullptr, nullptr, "xlsx_sheet_handle") {};
    DFHack::identity_type type() override { return IDTYPE_OPAQUE; }
};

struct xlsx_file_handle {
    typedef struct xlsxio_read_struct* xlsxioreader;
    const xlsxioreader handle;
    xlsx_file_handle(xlsxioreader handle): handle(handle) {}
    DFHACK_IMPORT static xlsx_file_handle_identity _identity;
};

struct xlsx_sheet_handle {
    typedef struct xlsxio_read_sheet_struct* xlsxioreadersheet;
    const xlsxioreadersheet handle;
    xlsx_sheet_handle(xlsxioreadersheet handle): handle(handle) {}
    DFHACK_IMPORT static xlsx_sheet_handle_identity _identity;
};

}
