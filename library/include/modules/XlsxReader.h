/*
 * Wrapper for xlsxio_read library functions.
 */

#pragma once

#include <string>
#include <vector>

#include "Export.h"

/**
 * \defgroup grp_xlsx_reader Xlsx file reader
 * @ingroup grp_modules
 */
namespace DFHack
{
namespace XlsxReader
{

typedef void* xlsx_file_handle;
typedef void* xlsx_sheet_handle;

// returns NULL on error
DFHACK_EXPORT xlsx_file_handle open_xlsx_file(std::string filename);
DFHACK_EXPORT void close_xlsx_file(xlsx_file_handle file_handle);
DFHACK_EXPORT std::vector<std::string> list_sheets(xlsx_file_handle file_handle);

// returns XLSXIOReaderSheet object or NULL on error
DFHACK_EXPORT xlsx_sheet_handle open_sheet(
        xlsx_file_handle file_handle, std::string sheet_name);
DFHACK_EXPORT void close_sheet(xlsx_sheet_handle sheet_handle);

// start reading the next row of data; must be called before GetNextCell .
// returns false if there is no next row to get.
DFHACK_EXPORT bool get_next_row(xlsx_sheet_handle sheet_handle);

// fills the value param with the contents of the cell in the next column cell
// in the current row.
// returns false if there are no more cells in this row.
DFHACK_EXPORT bool get_next_cell(
        xlsx_sheet_handle sheet_handle, std::string& value);

}
}
