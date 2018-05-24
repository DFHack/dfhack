# Helper to download files as needed

function(file_md5_if_exists FILE VAR)
  if(EXISTS "${FILE}")
    file(MD5 "${FILE}" "${VAR}")
    set(${VAR} "${${VAR}}" PARENT_SCOPE)
  else()
    set(${VAR} "" PARENT_SCOPE)
  endif()
endfunction()

function(search_downloads FILE_MD5 VAR)
  set(${VAR} "" PARENT_SCOPE)
  file(GLOB FILES ${CMAKE_SOURCE_DIR}/CMake/downloads/*)
  foreach(FILE ${FILES})
    file(MD5 "${FILE}" CUR_MD5)
    if("${CUR_MD5}" STREQUAL "${FILE_MD5}")
      set(${VAR} ${FILE} PARENT_SCOPE)
      return()
    endif()
  endforeach()
endfunction()

function(download_file URL DEST EXPECTED_MD5)
  get_filename_component(FILENAME "${URL}" NAME)
  file_md5_if_exists("${DEST}" CUR_MD5)

  if(NOT "${EXPECTED_MD5}" STREQUAL "${CUR_MD5}")
    search_downloads(${EXPECTED_MD5} DLPATH)
    if(NOT("${DLPATH}" STREQUAL ""))
      message("* Copying ${FILENAME} from ${DLPATH}")
      execute_process(COMMAND "${CMAKE_COMMAND}" -E copy
        "${DLPATH}"
        "${DEST}")
      return()
    endif()

    message("* Downloading ${FILENAME}")
    file(DOWNLOAD "${URL}" "${DEST}" EXPECTED_MD5 "${EXPECTED_MD5}" SHOW_PROGRESS)
  endif()
endfunction()

# Download a file and uncompress it
function(download_file_unzip URL ZIP_TYPE ZIP_DEST ZIP_MD5 UNZIP_DEST UNZIP_MD5)
  get_filename_component(FILENAME "${URL}" NAME)
  file_md5_if_exists("${UNZIP_DEST}" CUR_UNZIP_MD5)

  # Redownload if the MD5 of the uncompressed file doesn't match
  if(NOT "${UNZIP_MD5}" STREQUAL "${CUR_UNZIP_MD5}")
    download_file("${URL}" "${ZIP_DEST}" "${ZIP_MD5}")

    if(EXISTS "${ZIP_DEST}")
      message("* Decompressing ${FILENAME}")
      if("${ZIP_TYPE}" STREQUAL "gz")
        execute_process(COMMAND
          "${PERL_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/depends/gunzip.pl"
          "${ZIP_DEST}" --force)
      else()
        message(SEND_ERROR "Unknown ZIP_TYPE: ${ZIP_TYPE}")
      endif()
      if(NOT EXISTS "${UNZIP_DEST}")
        message(SEND_ERROR "File failed to unzip to ${UNZIP_DEST}")
      else()
        file(MD5 "${UNZIP_DEST}" CUR_UNZIP_MD5)
        if(NOT "${UNZIP_MD5}" STREQUAL "${CUR_UNZIP_MD5}")
          message(SEND_ERROR "MD5 mismatch: ${UNZIP_DEST}: expected ${UNZIP_MD5}, got ${CUR_UNZIP_MD5}")
        endif()
      endif()
    endif()
  endif()
endfunction()
