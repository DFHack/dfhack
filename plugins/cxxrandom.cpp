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

#include "Error.h"
#include "Core.h"
#include "DataFuncs.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>

using namespace DFHack;
DFHACK_PLUGIN("cxxrandom");
#define PLUGIN_VERSION 2.0
color_ostream *cout = nullptr;

DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{
    cout = &out;
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    return CR_OK;
}


class EnginesKeeper
{
private:
    EnginesKeeper() {}
    std::unordered_map<uint16_t, std::mt19937_64> m_engines;
    uint16_t counter = 0;
public:
    static EnginesKeeper& Instance()
    {
        static EnginesKeeper instance;
        return instance;
    }
    uint16_t NewEngine( uint64_t seed )
    {
        std::mt19937_64 engine( seed != 0 ? seed : std::chrono::system_clock::now().time_since_epoch().count() );
        m_engines[++counter] = engine;
        return counter;
    }
    void DestroyEngine( uint16_t id )
    {
        m_engines.erase( id );
    }
    void NewSeed( uint16_t id, uint64_t seed )
    {
        CHECK_INVALID_ARGUMENT( m_engines.find( id ) != m_engines.end() );
        m_engines[id].seed( seed != 0 ? seed : std::chrono::system_clock::now().time_since_epoch().count() );
    }
    std::mt19937_64& RNG( uint16_t id )
    {
        CHECK_INVALID_ARGUMENT( m_engines.find( id ) != m_engines.end() );
        return m_engines[id];
    }
};


uint16_t GenerateEngine( uint64_t seed )
{
    return EnginesKeeper::Instance().NewEngine( seed );
}

void DestroyEngine( uint16_t id )
{
    EnginesKeeper::Instance().DestroyEngine( id );
}

void NewSeed( uint16_t id, uint64_t seed )
{
    EnginesKeeper::Instance().NewSeed( id, seed );
}


int      rollInt(uint16_t id, int min, int max)
{
    std::uniform_int_distribution<int> ND(min, max);
    return ND(EnginesKeeper::Instance().RNG(id));
}

double   rollDouble(uint16_t id, double min, double max)
{
    std::uniform_real_distribution<double> ND(min, max);
    return ND(EnginesKeeper::Instance().RNG(id));
}

double   rollNormal(uint16_t id, double mean, double stddev)
{
    std::normal_distribution<double> ND(mean, stddev);
    return ND(EnginesKeeper::Instance().RNG(id));
}

bool     rollBool(uint16_t id, float p)
{
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
    NumberSequence( int64_t start, int64_t end )
    {
        for( int64_t i = start; i <= end; ++i )
        {
            m_numbers.push_back( i );
        }
    }
    void Add( int64_t num ) { m_numbers.push_back( num ); }
    void Reset()            { m_numbers.clear(); }
    int64_t Next()
    {
        if(m_position >= m_numbers.size())
        {
            m_position = 0;
        }
        return m_numbers[m_position++];
    }
    void Shuffle( uint16_t id )
    {
        std::shuffle( std::begin( m_numbers ), std::end( m_numbers ), EnginesKeeper::Instance().RNG( id ) );
    }
    void Print()
    {
        for( auto v : m_numbers )
        {
            cout->print( "%" PRId64 " ", v );
        }
    }
};

class SequenceKeeper
{
private:
    SequenceKeeper() {}
    std::unordered_map<uint16_t, NumberSequence> m_sequences;
    uint16_t counter = 0;
public:
    static SequenceKeeper& Instance()
    {
        static SequenceKeeper instance;
        return instance;
    }
    uint16_t MakeNumSequence( int64_t start, int64_t end )
    {
        m_sequences[++counter] = NumberSequence( start, end );
        return counter;
    }
    uint16_t MakeNumSequence()
    {
        m_sequences[++counter] = NumberSequence();
        return counter;
    }
    void DestroySequence( uint16_t id )
    {
        m_sequences.erase( id );
    }
    void AddToSequence( uint16_t id, int64_t num )
    {
        CHECK_INVALID_ARGUMENT( m_sequences.find( id ) != m_sequences.end() );
        m_sequences[id].Add( num );
    }
    void Shuffle( uint16_t id, uint16_t rng_id )
    {
        CHECK_INVALID_ARGUMENT( m_sequences.find( id ) != m_sequences.end() );
        m_sequences[id].Shuffle( rng_id );
    }
    int64_t NextInSequence( uint16_t id )
    {
        CHECK_INVALID_ARGUMENT( m_sequences.find( id ) != m_sequences.end() );
        return m_sequences[id].Next();
    }
    void PrintSequence( uint16_t id )
    {
        CHECK_INVALID_ARGUMENT( m_sequences.find( id ) != m_sequences.end() );
        auto seq = m_sequences[id];
        seq.Print();
    }
};


uint16_t MakeNumSequence( int64_t start, int64_t end )
{
    if( start == end )
    {
        return SequenceKeeper::Instance().MakeNumSequence();
    }
    return SequenceKeeper::Instance().MakeNumSequence( start, end );
}

void     DestroyNumSequence( uint16_t id )
{
    SequenceKeeper::Instance().DestroySequence( id );
}

void     AddToSequence( uint16_t id, int64_t num )
{
    SequenceKeeper::Instance().AddToSequence( id, num );
}

void     ShuffleSequence( uint16_t rngID, uint16_t id )
{
    SequenceKeeper::Instance().Shuffle( id, rngID );
}

int64_t  NextInSequence( uint16_t id )
{
    return SequenceKeeper::Instance().NextInSequence( id );
}

void DebugSequence( uint16_t id )
{
    SequenceKeeper::Instance().PrintSequence( id );
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
