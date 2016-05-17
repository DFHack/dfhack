
#include "modules/Filesystem.h"
#include "modules/Persistent.h"

#include <iostream>
#include <sstream>
#include <string>

using namespace DFHack;
using namespace Persistent;
using std::cerr;
using std::endl;

static Json::Value root(Json::objectValue);

void Persistent::writeToFile(const std::string& filename) {
    std::ofstream out(filename);
    //TODO: assert out
    out << root;
    out.flush();
    out.close();
}

void Persistent::readFromFile(const std::string& filename) {
    if ( DFHack::Filesystem::exists(filename) ) {
        std::ifstream in(filename);
        //TODO: assert in
        in >> root;
        in.close();
    } else {
        root = Json::Value();
    }
}

Json::Value& Persistent::getBase() {
    return root;
}

Json::Value& Persistent::get(const std::string& name) {
    return root[name];
}

void Persistent::erase(const std::string& name) {
    if ( root.isMember(name) )
        root[name].clear();
}

void Persistent::clear() {
    root.clear();
}
