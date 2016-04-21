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

#include <iomanip>
#include "../include/Logger.h"

//----------------------------------------------------------------------------//
// Constructor
// Store a reference to the DFHack console
//----------------------------------------------------------------------------//
Logger::Logger(DFHack::color_ostream& out) : _out(out) {};

//----------------------------------------------------------------------------//
// Logs a string to the DFHack console without adding a newline
//----------------------------------------------------------------------------//
void Logger::log(std::string st)
{
  _out << st;
}

//----------------------------------------------------------------------------//
// Logs a string to the DFHack console adding a newline
//----------------------------------------------------------------------------//
void Logger::log_line(std::string st)
{
  _out << st << std::endl;
}

//----------------------------------------------------------------------------//
// Logs a newline to the DFHack console
//----------------------------------------------------------------------------//
void Logger::log_endl()
{
  _out << std::endl;
}

//----------------------------------------------------------------------------//
// Logs a carriage return to the DFHack console
//----------------------------------------------------------------------------//
void Logger::log_cr()
{
  _out << "\r" << std::flush;
}

//----------------------------------------------------------------------------//
// Logs a integer number to the DFHack console
//----------------------------------------------------------------------------//
void Logger::log_number(unsigned int i)
{
  _out << i;
}

//----------------------------------------------------------------------------//
// Logs a integer number to the console using a maximum width
//----------------------------------------------------------------------------//
void Logger::log_number(unsigned int i, unsigned int width)
{
  _out << std::setw(width) << i;
}
