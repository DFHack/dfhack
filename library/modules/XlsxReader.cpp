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

#include <xlsxio_read.h>

#include "modules/XlsxReader.h"

using namespace DFHack;


// returns NULL on error
DFHACK_EXPORT XlsxReader::xlsx_file_handle XlsxReader::open_xlsx_file(
        std::string filename)
{
    return xlsxioread_open(filename.c_str());
}

DFHACK_EXPORT void XlsxReader::close_xlsx_file(
        XlsxReader::xlsx_file_handle file_handle)
{
    xlsxioread_close((xlsxioreader)file_handle);
}

static int list_callback(const XLSXIOCHAR* name, void* cbdata)
{
    auto sheetNames = (std::vector<std::string> *)cbdata;
    sheetNames->push_back(name);
    return 0;
}

DFHACK_EXPORT std::vector<std::string> XlsxReader::list_sheets(
    XlsxReader::xlsx_file_handle file_handle)
{
    auto sheetNames = std::vector<std::string>();
    xlsxioread_list_sheets(
            (xlsxioreader)file_handle, list_callback, &sheetNames);
    return sheetNames;
}

// returns XLSXIOReaderSheet object or NULL on error
DFHACK_EXPORT XlsxReader::xlsx_sheet_handle XlsxReader::open_sheet(
    XlsxReader::xlsx_file_handle file_handle, std::string sheet_name)
{
    if (file_handle == NULL)
        return NULL;
    return xlsxioread_sheet_open(
        (xlsxioreader)file_handle, sheet_name.c_str(), XLSXIOREAD_SKIP_NONE);
}

DFHACK_EXPORT void XlsxReader::close_sheet(
        XlsxReader::xlsx_sheet_handle sheet_handle)
{
    xlsxioread_sheet_close((xlsxioreadersheet)sheet_handle);
}

// start reading the next row of data; must be called before GetNextCell .
// returns false if there is no next row to get.
DFHACK_EXPORT bool XlsxReader::get_next_row(
        XlsxReader::xlsx_sheet_handle sheet_handle)
{
    return xlsxioread_sheet_next_row((xlsxioreadersheet)sheet_handle) != 0;
}

// fills the value param with the contents of the cell in the next column cell
// in the current row.
// returns false if there are no more cells in this row.
DFHACK_EXPORT bool XlsxReader::get_next_cell(
    XlsxReader::xlsx_sheet_handle sheet_handle, std::string& value)
{
    char* result;
    if (!xlsxioread_sheet_next_cell_string((xlsxioreadersheet)sheet_handle,
                                           &result)) {
        value.clear();
        return false;
    }
    value.assign(result);
    free(result);
    return true;
}
