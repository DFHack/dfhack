/* Plugin for exporting C++11 random number functionality
*Exports functions for random number generation
*Functions:
- seedRNG(seed)
- rollInt(min, max)
- rollDouble(min, max)
- rollNormal(mean, std_deviation)
- rollBool(chance_for_true)
- resetIndexRolls(string, array_length)  --String identifies the instance of SimpleNumDistribution to reset
- rollIndex(string, array_length)        --String identifies the instance of SimpleNumDistribution to use
                                         --(Shuffles a vector of indices, Next() increments through then reshuffles when end() is reached)
Author:  Josh Cooper
Created: Dec. 13 2017
Updated: Dec. 21 2017
*/


#include <random>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cinttypes>
#include <stdio.h>
#include <stdlib.h>

#include "DataFuncs.h"
#include "PluginManager.h"
#include "PluginLua.h"

using namespace DFHack;
DFHACK_PLUGIN("cxxrandom");
#define PLUGIN_VERSION 2.0
color_ostream *cout = nullptr;

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands) {
    cout = &out;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out) {
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event) {
    return CR_OK;
}

#define EK_ID_BASE (1ll << 40)

class EnginesKeeper
{
private:
    EnginesKeeper() = default;
    std::unordered_map<uint64_t, std::mt19937_64> m_engines;
    uint64_t id_counter = EK_ID_BASE;
public:
    static EnginesKeeper& Instance() {
        static EnginesKeeper instance;
        return instance;
    }
    uint64_t NewEngine( uint64_t seed ) {
        auto id = ++id_counter;
        CHECK_INVALID_ARGUMENT(m_engines.count(id) == 0);
        std::mt19937_64 engine( seed != 0 ? seed : std::chrono::system_clock::now().time_since_epoch().count() );
        m_engines[id] = engine;
        return id;
    }
    void DestroyEngine( uint64_t id ) {
        m_engines.erase( id );
    }
    void NewSeed( uint64_t id, uint64_t seed ) {
        CHECK_INVALID_ARGUMENT( m_engines.find( id ) != m_engines.end() );
        m_engines[id].seed( seed != 0 ? seed : std::chrono::system_clock::now().time_since_epoch().count() );
    }
    std::mt19937_64& RNG( uint64_t id ) {
        CHECK_INVALID_ARGUMENT( m_engines.find( id ) != m_engines.end() );
        return m_engines[id];
    }
};


uint64_t GenerateEngine( uint64_t seed ) {
    return EnginesKeeper::Instance().NewEngine( seed );
}

void DestroyEngine( uint64_t id ) {
    EnginesKeeper::Instance().DestroyEngine( id );
}

void NewSeed( uint64_t id, uint64_t seed ) {
    EnginesKeeper::Instance().NewSeed( id, seed );
}


int      rollInt(uint64_t id, int min, int max) {
    std::uniform_int_distribution<int> ND(min, max);
    return ND(EnginesKeeper::Instance().RNG(id));
}

double   rollDouble(uint64_t id, double min, double max) {
    std::uniform_real_distribution<double> ND(min, max);
    return ND(EnginesKeeper::Instance().RNG(id));
}

double   rollNormal(uint64_t id, double mean, double stddev) {
    std::normal_distribution<double> ND(mean, stddev);
    return ND(EnginesKeeper::Instance().RNG(id));
}

bool     rollBool(uint64_t id, float p) {
    std::bernoulli_distribution ND(p);
    return ND(EnginesKeeper::Instance().RNG(id));
}


class NumberSequence
{
private:
    unsigned short m_position = 0;
    std::vector<int64_t> m_numbers;
public:
    NumberSequence(){}
    NumberSequence( int64_t start, int64_t end ) {
        for( int64_t i = start; i <= end; ++i ) {
            m_numbers.push_back( i );
        }
    }
    void Add( int64_t num ) { m_numbers.push_back( num ); }
    void Reset() { m_numbers.clear(); }
    int64_t Next() {
        if(m_position >= m_numbers.size()) {
            m_position = 0;
        }
        return m_numbers[m_position++];
    }
    void Shuffle( uint64_t engID ) {
        std::shuffle( std::begin( m_numbers ), std::end( m_numbers ), EnginesKeeper::Instance().RNG(engID));
    }
    void Print() {
        for( auto v : m_numbers ) {
            cout->print( "%" PRId64 " ", v );
        }
    }
};

#define SK_ID_BASE 0

class SequenceKeeper
{
private:
    SequenceKeeper() = default;
    std::unordered_map<uint64_t, NumberSequence> m_sequences;
    uint64_t id_counter = SK_ID_BASE;
public:
    static SequenceKeeper& Instance() {
        static SequenceKeeper instance;
        return instance;
    }
    uint64_t MakeNumSequence( int64_t start, int64_t end ) {
        auto id = ++id_counter;
        CHECK_INVALID_ARGUMENT(m_sequences.count(id) == 0);
        m_sequences[id] = NumberSequence(start, end);
        return id;
    }
    uint64_t MakeNumSequence() {
        auto id = ++id_counter;
        CHECK_INVALID_ARGUMENT(m_sequences.count(id) == 0);
        m_sequences[id] = NumberSequence();
        return id;
    }
    void DestroySequence( uint64_t seqID ) {
        m_sequences.erase(seqID);
    }
    void AddToSequence(uint64_t seqID, int64_t num ) {
        CHECK_INVALID_ARGUMENT(m_sequences.find(seqID) != m_sequences.end());
        m_sequences[seqID].Add(num);
    }
    void Shuffle(uint64_t seqID, uint64_t engID ) {
        uint64_t sid = seqID >= SK_ID_BASE ? seqID : engID;
        uint64_t eid = engID >= EK_ID_BASE ? engID : seqID;
        CHECK_INVALID_ARGUMENT(m_sequences.find(sid) != m_sequences.end());
        m_sequences[sid].Shuffle(eid);
    }
    int64_t NextInSequence( uint64_t seqID ) {
        CHECK_INVALID_ARGUMENT(m_sequences.find(seqID) != m_sequences.end());
        return m_sequences[seqID].Next();
    }
    void PrintSequence( uint64_t seqID ) {
        CHECK_INVALID_ARGUMENT(m_sequences.find(seqID) != m_sequences.end());
        auto seq = m_sequences[seqID];
        seq.Print();
    }
};


uint64_t MakeNumSequence( int64_t start, int64_t end ) {
    if (start == end) {
        return SequenceKeeper::Instance().MakeNumSequence();
    }
    return SequenceKeeper::Instance().MakeNumSequence(start, end);
}

void     DestroyNumSequence( uint64_t seqID ) {
    SequenceKeeper::Instance().DestroySequence(seqID);
}

void     AddToSequence(uint64_t seqID, int64_t num ) {
    SequenceKeeper::Instance().AddToSequence(seqID, num);
}

void     ShuffleSequence(uint64_t seqID, uint64_t engID ) {
    SequenceKeeper::Instance().Shuffle(seqID, engID);
}

int64_t  NextInSequence( uint64_t seqID ) {
    return SequenceKeeper::Instance().NextInSequence(seqID);
}

void DebugSequence( uint64_t seqID ) {
    SequenceKeeper::Instance().PrintSequence(seqID);
}


DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(GenerateEngine),
    DFHACK_LUA_FUNCTION(DestroyEngine),
    DFHACK_LUA_FUNCTION(NewSeed),
    DFHACK_LUA_FUNCTION(rollInt),
    DFHACK_LUA_FUNCTION(rollDouble),
    DFHACK_LUA_FUNCTION(rollNormal),
    DFHACK_LUA_FUNCTION(rollBool),
    DFHACK_LUA_FUNCTION(MakeNumSequence),
    DFHACK_LUA_FUNCTION(DestroyNumSequence),
    DFHACK_LUA_FUNCTION(AddToSequence),
    DFHACK_LUA_FUNCTION(ShuffleSequence),
    DFHACK_LUA_FUNCTION(NextInSequence),
    DFHACK_LUA_FUNCTION(DebugSequence),
    DFHACK_LUA_END
};
