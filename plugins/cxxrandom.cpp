/* Plugin for exporting C++11 random number functionality
*Exports functions for random number generation
*Functions:
- RollInt(min, max)
- RollDouble(min, max)
- RollNormal(mean, std_deviation)
- RollBool(chance_for_true)
- ResetIndexRolls(string, array_length)  --String identifies the unique Index Roller to reset, or create if it doesn't already exist
- RollIndex(string, array_length)        --String identifies the unique Index Roller to be used
                                         --(Shuffles a vector of indices, Next() increments through then reshuffles when end() is reached)
Author:  Josh Cooper
Created: Dec. 13 2017
Updated: Dec. 20 2017
*/


#include <random>
#include <chrono>
#include <unordered_map>
#include <vector>
#include <algorithm>

#include "Core.h"
#include "DataFuncs.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>


using namespace DFHack;
DFHACK_PLUGIN("cxxrandom");
#define PLUGIN_VERSION 1.0


class SimpleNumDistribution
{
protected:
    unsigned short m_position = 0;
    std::vector<unsigned short> m_distribution;

public:
    SimpleNumDistribution(unsigned short N);
    void Reset();
    unsigned short Length();
    unsigned short Next();
};

class CXXRNG
{
private:
    CXXRNG() { RNG = std::default_random_engine(std::chrono::system_clock::now().time_since_epoch().count()); }

protected:
    std::default_random_engine                                           RNG;
    std::unordered_map<int, std::uniform_int_distribution<int>>          ND_int;
    std::unordered_map<double, std::uniform_real_distribution<double>>   ND_double;
    std::unordered_map<double, std::normal_distribution<double>>         ND_normal;
    std::unordered_map<float, std::bernoulli_distribution>               ND_bool;
    std::unordered_map<std::string, SimpleNumDistribution>               ND_index;

public:
    static CXXRNG& Instance()
    {
        static CXXRNG instance;
        return instance;
    }
    void ResetIndexDistribution(std::string ref, unsigned short N);
    unsigned short GenIndex(std::string ref, unsigned short N);
    int GenInteger(short min, short max);
    float GenDouble(float min, float max);
    float GenNormal(float mean, float stddev);
    bool GenBool(float p);
    void BlastDistributions();
};

//command_result cxxrandom (color_ostream &out, std::vector <std::string> & parameters);
DFhackCExport command_result plugin_init (color_ostream &out, std::vector <PluginCommand> &commands)
{

	/*
    commands.push_back(PluginCommand(
        "cxxrandom", "C++xx Random Numbers", cxxrandom, false,
        "  This plugin cannot be used on the commandline.\n"
        "  Import into a lua script to have access to exported RNG functions:\n"
        "    local rng = require('plugins.cxxrandom')\n\n"
        "  Exported Functions:\n"
        "    rng.ResetIndexRolls(string_ref, total_indices)\n"
        "    rng.RollIndex(string_ref, total_indices)\n"
        "    rng.RollInt(min, max)\n"
        "    rng.RollDouble(min, max)\n"
        "    rng.RollNormal(mean, std_deviation)\n"
        "    rng.RollBool(chance_of_true)\n"
        "    rng.BlastDistributions()\n\n"
    ));*/
    CXXRNG::Instance();
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown (color_ostream &out)
{
    CXXRNG::Instance().BlastDistributions();
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    return CR_OK;
}

union TwoShorts
{
    struct
    {
        short A;
        short B;
    };
    int value;
};

union TwoFloats
{
    struct
    {
        float A;
        float B;
    };
    double value;
};

SimpleNumDistribution::SimpleNumDistribution(unsigned short N)
{
    m_position = 0;
    m_distribution.reserve(N);
    for(int i = 1; i <= N; ++i)
    {
        m_distribution.push_back(i);
    }
    Reset();
}

void SimpleNumDistribution::Reset()
{
    std::default_random_engine rng(std::chrono::system_clock::now().time_since_epoch().count());
    std::shuffle(std::begin(m_distribution), std::end(m_distribution), rng);
}

unsigned short SimpleNumDistribution::Length()
{
    return m_distribution.size();
}

unsigned short SimpleNumDistribution::Next()
{
    if(m_position >= Length())
    {
        m_position = 0;
        Reset();
    }
    return m_distribution[m_position++];
}


void CXXRNG::ResetIndexDistribution(std::string ref, unsigned short N)
{
    auto iter = ND_index.find(ref);
    if(iter == ND_index.end() || iter->second.Length() != N )
    {
        if(iter != ND_index.end())
            ND_index.erase(iter);

        iter = ND_index.emplace(ref, SimpleNumDistribution(N)).first;
    }
    iter->second.Reset();
}

unsigned short CXXRNG::GenIndex(std::string ref, unsigned short N)
{
    auto iter = ND_index.find(ref);
    if(iter == ND_index.end() || iter->second.Length() != N )
    {
        if(iter != ND_index.end())
            ND_index.erase(iter);

        iter = ND_index.emplace(ref, SimpleNumDistribution(N)).first;
    }
    return iter->second.Next();
}

int CXXRNG::GenInteger(short min, short max)
{
    TwoShorts merger;
    merger.A = min;
    merger.B = max;
    int ref = merger.value;
    auto iter = ND_int.find(ref);
    if(iter == ND_int.end() || iter->second.min() != min || iter->second.max() != max)
    {
        if(iter != ND_int.end())
            ND_int.erase(iter);

        iter = ND_int.emplace(ref, std::uniform_int_distribution<int>(min, max)).first;
    }
    return iter->second(RNG);
}

float CXXRNG::GenDouble(float min, float max)
{
    TwoFloats merger;
    merger.A = min;
    merger.B = max;
    double ref = merger.value;
    auto iter = ND_double.find(ref);
    if(iter == ND_double.end() || iter->second.min() != min || iter->second.max() != max)
    {
        if(iter != ND_double.end())
            ND_double.erase(iter);

        iter = ND_double.emplace(ref, std::uniform_real_distribution<double>(min, max)).first;
    }
    return iter->second(RNG);
}

float CXXRNG::GenNormal(float mean, float stddev)
{
    TwoFloats merger;
    merger.A = mean;
    merger.B = stddev;
    double ref = merger.value;
    auto iter = ND_normal.find(ref);
    if(iter == ND_normal.end() || iter->second.mean() != mean || iter->second.stddev() != stddev)
    {
        if(iter != ND_normal.end())
            ND_normal.erase(iter);

        iter = ND_normal.emplace(ref, std::normal_distribution<double>(mean, stddev)).first;
    }
    return iter->second(RNG);
}

bool CXXRNG::GenBool(float p)
{
    auto iter = ND_bool.find(p);
    if(iter == ND_bool.end() || iter->second.p() != p)
    {
        if(iter != ND_bool.end())
            ND_bool.erase(iter);

        iter = ND_bool.emplace(p, std::bernoulli_distribution(p)).first;
    }
    return iter->second(RNG);
}

void CXXRNG::BlastDistributions()
{
    ND_int.clear();
    ND_double.clear();
    ND_normal.clear();
    ND_bool.clear();
    ND_index.clear();
}


void ResetIndexRolls(std::string ref, unsigned short N)
{
    CXXRNG::Instance().ResetIndexDistribution(ref, N);
}

int RollIndex(std::string ref, unsigned short N)
{
    return CXXRNG::Instance().GenIndex(ref, N);
}

int RollInt(short min, short max)
{
    return CXXRNG::Instance().GenInteger((short)min, (short)max);
}

float RollDouble(float min, float max)
{
    return CXXRNG::Instance().GenDouble(min, max);
}

float RollNormal(float mean, float stddev)
{
    return CXXRNG::Instance().GenNormal(mean, stddev);
}

bool RollBool(float p)
{
    return CXXRNG::Instance().GenBool(p);
}

void BlastDistributions()
{
    CXXRNG::Instance().BlastDistributions();
}

DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(ResetIndexRolls),
    DFHACK_LUA_FUNCTION(RollIndex),
    DFHACK_LUA_FUNCTION(RollInt),
    DFHACK_LUA_FUNCTION(RollDouble),
    DFHACK_LUA_FUNCTION(RollNormal),
    DFHACK_LUA_FUNCTION(RollBool),
    DFHACK_LUA_FUNCTION(BlastDistributions),
    DFHACK_LUA_END
};
/*
static command_result cxxrandom (color_ostream &out, std::vector <std::string> & parameters)
{
    return CR_WRONG_USAGE;
}*/
