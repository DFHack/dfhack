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

#include "Core.h"
#include "DataFuncs.h"
#include <Console.h>
#include <Export.h>
#include <PluginManager.h>


using namespace DFHack;
DFHACK_PLUGIN("cxxrandom");
#define PLUGIN_VERSION 1.0


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


std::default_random_engine& RNG()
{
    static std::default_random_engine instance(std::chrono::system_clock::now().time_since_epoch().count());
    return instance;
}

void    seedRNG(unsigned short seed)
{
    RNG() = std::default_random_engine(seed);
}



class SimpleNumDistribution
{
private:
    unsigned short m_position = 0;
    std::vector<unsigned short> m_distribution;

public:
    SimpleNumDistribution(unsigned short N)
    {
        m_position = 0;
        m_distribution.reserve(N);
        for(int i = 1; i <= N; ++i)
        {
            m_distribution.push_back(i);
        }
        Reset();
    }

    void Reset()
    {
        std::shuffle(std::begin(m_distribution), std::end(m_distribution), RNG());
    }

    unsigned short Length() const { return m_distribution.size(); }

    unsigned short Next()
    {
        if(m_position >= m_distribution.size())
        {
            m_position = 0;
            Reset();
        }
        return m_distribution[m_position++];
    }
};



typedef std::unordered_map<std::string, SimpleNumDistribution> DistributionContainer;
DistributionContainer& GetDistribContainer()
{
    static DistributionContainer instance;
    return instance;
}

void    resetIndexRolls(std::string ref, unsigned short N)
{
    DistributionContainer& ND_index = GetDistribContainer();
    auto iter = ND_index.find(ref);
    if(iter == ND_index.end() || iter->second.Length() != N )
    {
        if(iter != ND_index.end())
            ND_index.erase(iter);
        
        iter = ND_index.emplace(ref, SimpleNumDistribution(N)).first;
    }
    iter->second.Reset();
}

int     rollIndex(std::string ref, unsigned short N)
{
    DistributionContainer& ND_index = GetDistribContainer();
    auto iter = GetDistribContainer().find(ref);
    if(iter == ND_index.end() || iter->second.Length() != N )
    {
        if(iter != ND_index.end())
            ND_index.erase(iter);
        
        iter = ND_index.emplace(ref, SimpleNumDistribution(N)).first;
    }
    return iter->second.Next();
}


int     rollInt(int min, int max)
{
    std::uniform_int_distribution<int> ND(min, max);
    return ND(RNG());
}
        
double  rollDouble(double min, double max)
{
    std::uniform_real_distribution<double> ND(min, max);
    return ND(RNG());
}
        
double  rollNormal(double mean, double stddev)
{
    std::normal_distribution<double> ND(mean, stddev);
    return ND(RNG());
}
        
bool    rollBool(float p)
{
    std::bernoulli_distribution ND(p);
    return ND(RNG());
}


DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(resetIndexRolls),
    DFHACK_LUA_FUNCTION(rollIndex),
    DFHACK_LUA_FUNCTION(seedRNG),
    DFHACK_LUA_FUNCTION(rollInt),
    DFHACK_LUA_FUNCTION(rollDouble),
    DFHACK_LUA_FUNCTION(rollNormal),
    DFHACK_LUA_FUNCTION(rollBool),
    DFHACK_LUA_END
};

