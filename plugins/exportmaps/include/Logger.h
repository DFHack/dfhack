/*
  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

// You can always find the latest version of this plugin in Github
// https://github.com/ragundo/exportmaps

#ifndef EXPORTMAPS_LOGGER_H
#define EXPORTMAPS_LOGGER_H
#include "../../../library/include/ColorText.h"


class Logger
{
public:
  Logger(DFHack::color_ostream& out);
  void log(std::string st);
  void log_line(std::string st);
  void log_cr();
  void log_endl();
  void log_number(unsigned int i);
  void log_number(unsigned int i, unsigned int length);
protected:
  DFHack::color_ostream& _out;
};

#endif //EXPORTMAPS_LOGGER_H
