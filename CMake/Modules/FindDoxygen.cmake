# - This module looks for Doxygen and the path to Graphviz's dot
# Doxygen is a documentation generation tool.  Please see
# http://www.doxygen.org
#
# This module accepts the following optional variables:
#
#   DOXYGEN_SKIP_DOT       = If true this module will skip trying to find Dot
#                            (an optional component often used by Doxygen)
#
# This modules defines the following variables:
#
#   DOXYGEN_EXECUTABLE     = The path to the doxygen command.
#   DOXYGEN_FOUND          = Was Doxygen found or not?
#
#   DOXYGEN_DOT_EXECUTABLE = The path to the dot program used by doxygen.
#   DOXYGEN_DOT_FOUND      = Was Dot found or not?
#   DOXYGEN_DOT_PATH       = The path to dot not including the executable
#
#

#Copyright 2000-2009 Kitware, Inc., Insight Software Consortium
#All rights reserved.
#
#Redistribution and use in source and binary forms, with or without
#modification, are permitted provided that the following conditions
#are met:
#
#* Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#* Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#* Neither the names of Kitware, Inc., the Insight Software Consortium,
#  nor the names of their contributors may be used to endorse or promote
#  products derived from this software without specific prior written
#  permission.
#
#THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#-----------------------------------------------------------------------------
#
#The above copyright and license notice applies to distributions of
#CMake in source and binary form.  Some source files contain additional
#notices of original copyright by their contributors; see each source
#for details.  Third-party software packages supplied with CMake under
#compatible licenses provide their own copyright notices documented in
#corresponding subdirectories.
#
#-----------------------------------------------------------------------------
#
#CMake was initially developed by Kitware with the following sponsorship:
#
# * National Library of Medicine at the National Institutes of Health
#   as part of the Insight Segmentation and Registration Toolkit (ITK).
#
# * US National Labs (Los Alamos, Livermore, Sandia) ASC Parallel
#   Visualization Initiative.
#
# * National Alliance for Medical Image Computing (NAMIC) is funded by the
#   National Institutes of Health through the NIH Roadmap for Medical Research,
#   Grant U54 EB005149.
#
# * Kitware, Inc.


# For backwards compatibility support
IF(Doxygen_FIND_QUIETLY)
  SET(DOXYGEN_FIND_QUIETLY TRUE)
ENDIF(Doxygen_FIND_QUIETLY)

# ===== Rationale for OS X AppBundle mods below =====
#     With the OS X GUI version, Doxygen likes to be installed to /Applications and
#     it contains the doxygen executable in the bundle. In the versions I've 
#     seen, it is located in Resources, but in general, more often binaries are 
#     located in MacOS.
#
#     NOTE: The official Doxygen.app that is distributed for OS X uses non-standard 
#     conventions.  Instead of the command-line "doxygen" tool being placed in
#     Doxygen.app/Contents/MacOS, "Doxywizard" is placed there instead and 
#     "doxygen" is placed in Contents/Resources.  This is most likely done
#     so that something happens when people double-click on the Doxygen.app
#     package.  Unfortunately, CMake gets confused by this as when it sees the
#     bundle it uses "Doxywizard" as the executable to use instead of
#     "doxygen".  Therefore to work-around this issue we temporarily disable
#     the app-bundle feature, just for this CMake module:
if(APPLE)
    #  Save the old setting
    SET(TEMP_DOXYGEN_SAVE_CMAKE_FIND_APPBUNDLE ${CMAKE_FIND_APPBUNDLE})
    # Disable the App-bundle detection feature
    SET(CMAKE_FIND_APPBUNDLE "NEVER")
endif()
#     FYI:
#     In the older versions of OS X Doxygen, dot was included with the 
#     Doxygen bundle. But the new versions require you to download
#     Graphviz.app which contains "dot" in it's bundle.
# ============== End OSX stuff ================

#
# Find Doxygen...
#

FIND_PROGRAM(DOXYGEN_EXECUTABLE
  NAMES doxygen
  PATHS
    "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\doxygen_is1;Inno Setup: App Path]/bin"
    /Applications/Doxygen.app/Contents/Resources
    /Applications/Doxygen.app/Contents/MacOS
  DOC "Doxygen documentation generation tool (http://www.doxygen.org)"
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Doxygen DEFAULT_MSG DOXYGEN_EXECUTABLE)

#
# Find Dot...
#

IF(NOT DOXYGEN_SKIP_DOT)
  FIND_PROGRAM(DOXYGEN_DOT_EXECUTABLE
    NAMES dot
    PATHS 
      "$ENV{ProgramFiles}/Graphviz2.26.3/bin"
      "C:/Program Files/Graphviz2.26.3/bin"
      "$ENV{ProgramFiles}/Graphviz 2.21/bin"
      "C:/Program Files/Graphviz 2.21/bin"
      "$ENV{ProgramFiles}/ATT/Graphviz/bin"
      "C:/Program Files/ATT/Graphviz/bin"
      [HKEY_LOCAL_MACHINE\\SOFTWARE\\ATT\\Graphviz;InstallPath]/bin
      /Applications/Graphviz.app/Contents/MacOS
      /Applications/Doxygen.app/Contents/Resources
      /Applications/Doxygen.app/Contents/MacOS
    DOC "Graphviz Dot tool for using Doxygen"
  )
  
  if(DOXYGEN_DOT_EXECUTABLE)
    set(DOXYGEN_DOT_FOUND TRUE)
    # The Doxyfile wants the path to Dot, not the entire path and executable
    get_filename_component(DOXYGEN_DOT_PATH "${DOXYGEN_DOT_EXECUTABLE}" PATH CACHE)
  endif()
  
endif(NOT DOXYGEN_SKIP_DOT)

#
# Backwards compatibility...
#

if(APPLE)
  # Restore the old app-bundle setting setting
  SET(CMAKE_FIND_APPBUNDLE ${TEMP_DOXYGEN_SAVE_CMAKE_FIND_APPBUNDLE})
endif()

# Maintain the _FOUND variables as "YES" or "NO" for backwards compatibility
# (allows people to stuff them directly into Doxyfile with configure_file())
if(DOXYGEN_FOUND)
  set(DOXYGEN_FOUND "YES")
else()
  set(DOXYGEN_FOUND "NO")
endif()
if(DOXYGEN_DOT_FOUND)
  set(DOXYGEN_DOT_FOUND "YES")
else()
  set(DOXYGEN_DOT_FOUND "NO")
endif()

# For backwards compatibility support
SET (DOXYGEN ${DOXYGEN_EXECUTABLE} )
SET (DOT ${DOXYGEN_DOT_EXECUTABLE} )

MARK_AS_ADVANCED(
  DOXYGEN_EXECUTABLE
  DOXYGEN_DOT_EXECUTABLE
  DOXYGEN_DOT_PATH
  )
