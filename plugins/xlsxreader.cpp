/*
 * Wrapper for xlsxio_read library functions.
 *
 * Sample usage:
 *
 *  std::string filename = "sample_file.xlsx";
 *  xlsxioreader xlsxfile = XlsxReader::open_xlsx_file(filename);
 *  if (xlsxfile == NULL) {
 *      printf("cannot open file: '%s'", filename.c_str());
 *      return false;
 *  }
 *  auto sheetNames = XlsxReader::list_sheets(xlsxfile);
 *  for (auto sheetName = sheetNames.begin();
 *       sheetName != sheetNames.end();
 *       ++sheetName) {
 *      printf("reading sheet: %s\n", sheetName->c_str());
 *      xlsxioreadersheet xlsxsheet =
 *              XlsxReader::open_sheet(xlsxfile, *sheetName);
 *      if (xlsxsheet == NULL) {
 *          printf("cannot open sheet: '%s'", sheetName->c_str());
 *          continue;
 *      }
 *      std::string value;
 *      int row_num = 1;
 *      while (XlsxReader::GetNextRow(xlsxsheet)) {
 *          std::string s;
 *          printf("%d:\t", row_num);
 *          while (XlsxReader::GetNextCell(xlsxsheet, s)) {
 *              printf("%s\t", s.c_str());
 *          }
 *          printf("\n");
 *          ++row_num;
 *      }
 *      XlsxReader::close_sheet(xlsxsheet);
 *  }
 *  XlsxReader::close_xlsx_file(xlsxfile);
 *  return true;
 */

#define BUILD_XLSXIO_STATIC
#include <xlsxio_read.h>

#include "DataFuncs.h"
#include "LuaTools.h"
#include "PluginManager.h"

using namespace DFHack;

DFHACK_PLUGIN("xlsxreader");

struct xlsx_file_handle_identity : public compound_identity {
    xlsx_file_handle_identity()
        :compound_identity(0, nullptr, nullptr, "handle") {};
    DFHack::identity_type type() override { return IDTYPE_OPAQUE; }
};

struct xlsx_sheet_handle_identity : public compound_identity {
    xlsx_sheet_handle_identity()
        :compound_identity(0, nullptr, nullptr, "handle") {};
    DFHack::identity_type type() override { return IDTYPE_OPAQUE; }
};

struct xlsx_file_handle {
    const xlsxioreader handle;
    xlsx_file_handle(xlsxioreader handle): handle(handle) {}
    static xlsx_file_handle_identity _identity;
};

struct xlsx_sheet_handle {
    const xlsxioreadersheet handle;
    xlsx_sheet_handle(xlsxioreadersheet handle): handle(handle) {}
    static xlsx_sheet_handle_identity _identity;
};

xlsx_file_handle_identity xlsx_file_handle::_identity;
xlsx_sheet_handle_identity xlsx_sheet_handle::_identity;

// returns NULL on error
xlsx_file_handle* open_xlsx_file(std::string filename) {
    return new xlsx_file_handle(xlsxioread_open(filename.c_str()));
}

void close_xlsx_file(xlsx_file_handle *file_handle) {
    CHECK_NULL_POINTER(file_handle);
    xlsxioread_close(file_handle->handle);
    delete(file_handle);
}

// returns XLSXIOReaderSheet object or NULL on error
xlsx_sheet_handle* open_sheet(xlsx_file_handle *file_handle,
                              std::string sheet_name) {
    CHECK_NULL_POINTER(file_handle);
    return new xlsx_sheet_handle(
        xlsxioread_sheet_open(file_handle->handle,
                              sheet_name.c_str(), XLSXIOREAD_SKIP_NONE));
}

void close_sheet(xlsx_sheet_handle *sheet_handle) {
    CHECK_NULL_POINTER(sheet_handle);
    xlsxioread_sheet_close(sheet_handle->handle);
    delete(sheet_handle);
}

static int list_callback(const XLSXIOCHAR* name, void* cbdata) {
    auto sheetNames = (std::vector<std::string> *)cbdata;
    sheetNames->push_back(name);
    return 0;
}

// start reading the next row of data; must be called before get_next_cell.
// returns false if there is no next row to get.
static bool get_next_row(xlsx_sheet_handle *sheet_handle) {
    return xlsxioread_sheet_next_row(sheet_handle->handle) != 0;
}

// fills the value param with the contents of the cell in the next column cell
// in the current row.
// returns false if there are no more cells in this row.
static bool get_next_cell(xlsx_sheet_handle *sheet_handle, std::string& value) {
    char* result;
    if (!xlsxioread_sheet_next_cell_string(sheet_handle->handle, &result)) {
        value.clear();
        return false;
    }
    value.assign(result);
    free(result);
    return true;
}

// internal function to extract a handle from the first stack param
static void * get_xlsxreader_handle(lua_State *L) {
    if (lua_gettop(L) < 1 || lua_isnil(L, 1)) {
        luaL_error(L, "invalid xlsxreader handle");
    }
    luaL_checktype(L, 1, LUA_TUSERDATA);
    return LuaWrapper::get_object_ref(L, 1);
}

// takes a file handle and returns a table-list of sheet names
int list_sheets(lua_State *L) {
    auto file_handle = (xlsx_file_handle *)get_xlsxreader_handle(L);
    auto sheetNames = std::vector<std::string>();
    xlsxioread_list_sheets(file_handle->handle, list_callback, &sheetNames);
    Lua::PushVector(L, sheetNames, true);
    return 1;
}

// takes the sheet handle and returns a table-list of strings, or nil if we
// already processed the last row in the file.
int get_row(lua_State *L) {
    auto sheet_handle = (xlsx_sheet_handle *)get_xlsxreader_handle(L);
    bool ok = get_next_row(sheet_handle);
    if (!ok) {
        lua_pushnil(L);
    } else {
        std::string value;
        auto cells = std::vector<std::string>();
        while (get_next_cell(sheet_handle, value)) {
            cells.push_back(value);
        }
        Lua::PushVector(L, cells, true);
    }

    return 1;
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(open_xlsx_file),
    DFHACK_LUA_FUNCTION(close_xlsx_file),
    DFHACK_LUA_FUNCTION(open_sheet),
    DFHACK_LUA_FUNCTION(close_sheet),
    DFHACK_LUA_END
};

DFHACK_PLUGIN_LUA_COMMANDS{
    DFHACK_LUA_COMMAND(list_sheets),
    DFHACK_LUA_COMMAND(get_row),
    DFHACK_LUA_END
};

DFhackCExport command_result plugin_init(color_ostream &,
                                         std::vector<PluginCommand> &) {
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &) {
    return CR_OK;
}
