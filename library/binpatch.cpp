/*
https://github.com/peterix/dfhack
Copyright (c) 2011 Petr Mrázek <peterix@gmail.com>

A thread-safe logging console with a line editor for windows.

Based on linenoise win32 port,
copyright 2010, Jon Griffiths <jon_p_griffiths at yahoo dot com>.
All rights reserved.
Based on linenoise, copyright 2010, Salvatore Sanfilippo <antirez at gmail dot com>.
The original linenoise can be found at: http://github.com/antirez/linenoise

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
  * Neither the name of Redis nor the names of its contributors may be used
    to endorse or promote products derived from this software without
    specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/


#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <string>
#include <stdint.h>

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <vector>

#include <memory>

#include <md5wrapper.h>

using std::cout;
using std::cerr;
using std::endl;

typedef unsigned char patch_byte;

struct BinaryPatch {
    struct Byte {
        unsigned offset;
        patch_byte old_val, new_val;
    };
    enum State {
        Conflict = 0,
        Unapplied = 1,
        Applied = 2,
        Partial = 3
    };

    std::vector<Byte> entries;

    bool loadDIF(std::string name);
    State checkState(const patch_byte *ptr, size_t len);

    void apply(patch_byte *ptr, size_t len, bool newv);
};

inline bool is_hex(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

bool BinaryPatch::loadDIF(std::string name)
{
    entries.clear();

    std::ifstream infile(name);
    if(infile.bad())
    {
        cerr << "Cannot open file: " << name << endl;
        return false;
    }

    std::string s;
    while(std::getline(infile, s))
    {
        // Parse lines that begin with "[0-9a-f]+:"
        size_t idx = s.find(':');
        if (idx == std::string::npos || idx == 0 || idx > 8)
            continue;

        bool ok = true;
        for (size_t i = 0; i < idx; i++)
            if (!is_hex(s[i]))
                ok = false;
        if (!ok)
            continue;

        unsigned off, oval, nval;
        int nchar = 0;
        int cnt = sscanf(s.c_str(), "%x: %x %x%n", &off, &oval, &nval, &nchar);

        if (cnt < 3)
        {
            cerr << "Could not parse: " << s << endl;
            return false;
        }

        for (size_t i = nchar; i < s.size(); i++)
        {
            if (!isspace(s[i]))
            {
                cerr << "Garbage at end of line: " << s << endl;
                return false;
            }
        }

        if (oval >= 256 || nval >= 256)
        {
            cerr << "Invalid byte values: " << s << endl;
            return false;
        }

        Byte bv = { off, patch_byte(oval), patch_byte(nval) };
        entries.push_back(bv);
    }

    if (entries.empty())
    {
        cerr << "No lines recognized." << endl;
        return false;
    }

    return true;
}

BinaryPatch::State BinaryPatch::checkState(const patch_byte *ptr, size_t len)
{
    int state = 0;

    for (size_t i = 0; i < entries.size(); i++)
    {
        Byte &bv = entries[i];

        if (bv.offset >= len)
        {
            cerr << "Offset out of range: 0x" << std::hex << bv.offset << std::dec << endl;
            return Conflict;
        }

        patch_byte cv = ptr[bv.offset];
        if (bv.old_val == cv)
            state |= Unapplied;
        else if (bv.new_val == cv)
            state |= Applied;
        else
        {
            cerr << std::hex << bv.offset << ": "
                 << unsigned(bv.old_val) << " " << unsigned(bv.new_val)
                 << ", but currently " << unsigned(cv) << std::dec << endl;
            return Conflict;
        }
    }

    return State(state);
}

void BinaryPatch::apply(patch_byte *ptr, size_t len, bool newv)
{
    for (size_t i = 0; i < entries.size(); i++)
    {
        Byte &bv = entries[i];
        assert (bv.offset < len);

        ptr[bv.offset] = (newv ? bv.new_val : bv.old_val);
    }
}

bool load_file(std::vector<patch_byte> *pvec, std::string fname)
{
    FILE *f = fopen(fname.c_str(), "rb");
    if (!f)
    {
        cerr << "Cannot open file: " << fname << endl;
        return false;
    }

    fseek(f, 0, SEEK_END);
    pvec->resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    size_t cnt = fread(pvec->data(), 1, pvec->size(), f);
    fclose(f);

    return cnt == pvec->size();
}

bool save_file(const std::vector<patch_byte> &pvec, std::string fname)
{
    FILE *f = fopen(fname.c_str(), "wb");
    if (!f)
    {
        cerr << "Cannot open file: " << fname << endl;
        return false;
    }

    size_t cnt = fwrite(pvec.data(), 1, pvec.size(), f);
    fclose(f);

    return cnt == pvec.size();
}

std::string compute_hash(const std::vector<patch_byte> &pvec)
{
    md5wrapper md5;
    return md5.getHashFromBytes(pvec.data(), pvec.size());
}

int main (int argc, char *argv[])
{
    if (argc <= 3)
    {
        cerr << "Usage: binpatch check|apply|remove <exe> <patch>" << endl;
        return 2;
    }

    std::string cmd = argv[1];

    if (cmd != "check" && cmd != "apply" && cmd != "remove")
    {
        cerr << "Invalid command: " << cmd << endl;
        return 2;
    }

    std::string exe_file = argv[2];
    std::vector<patch_byte> bindata;
    if (!load_file(&bindata, exe_file))
        return 2;

    BinaryPatch patch;
    if (!patch.loadDIF(argv[3]))
        return 2;

    BinaryPatch::State state = patch.checkState(bindata.data(), bindata.size());
    if (state == BinaryPatch::Conflict)
        return 1;

    if (cmd == "check")
    {
        switch (state)
        {
        case BinaryPatch::Unapplied:
            cout << "Currently not applied." << endl;
            break;
        case BinaryPatch::Applied:
            cout << "Currently applied." << endl;
            break;
        case BinaryPatch::Partial:
            cout << "Currently partially applied." << endl;
            break;
        default:
            break;
        }

        return 0;
    }
    else if (cmd == "apply")
    {
        if (state == BinaryPatch::Applied)
        {
            cout << "Already applied." << endl;
            return 0;
        }

        patch.apply(bindata.data(), bindata.size(), true);
    }
    else if (cmd == "remove")
    {
        if (state == BinaryPatch::Unapplied)
        {
            cout << "Already removed." << endl;
            return 0;
        }

        patch.apply(bindata.data(), bindata.size(), false);
    }

    if (!save_file(bindata, exe_file + ".bak"))
    {
        cerr << "Could not create backup." << endl;
        return 1;
    }

    if (!save_file(bindata, exe_file))
        return 1;

    cout << "Patched " << patch.entries.size()
         << " bytes, new hash: " << compute_hash(bindata) << endl;
    return 0;
}
