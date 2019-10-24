#pragma once

#include "Error.h"

namespace DFHack {
namespace Export {
class Error : public DFHack::Error::All {
  using DFHack::Error::All::All;
public:
  [[noreturn]] void context(std::string c) const {
    throw Error{c + ": " + full};
  }
  [[noreturn]] void context(const char *c) const {
    context(std::string{c});
  }
};

[[noreturn]] void die(const std::string &full) {
  throw Error{full};
}
[[noreturn]] void die(const char *full) {
  die(std::string{full});
}
}
}
