#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "df/world.h"
#include "df/world_raws.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "modules/World.h"

//#include "df/world.h"

using namespace DFHack;

DFHACK_PLUGIN("generated-creature-renamer");
REQUIRE_GLOBAL(world);

command_result rename_creatures(color_ostream &out, std::vector <std::string> & parameters);
command_result list_creatures(color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "rename-generated",
        "Renames generated creature tags to something friendlier to modders",
        rename_creatures,
        false, //allow non-interactive use
        "Renames generated creature tags to something friendlier to modders"
    ));
    commands.push_back(PluginCommand(
        "list-generated",
        "Prints a list of generated creature tokens. Use \"list-generated detailed\" to show descriptions.",
        list_creatures,
        false, //allow non-interactive use
        "Prints a list of generated creature tokens. Use \"list-generated detailed\" to show descriptions."
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}

#define NUM_DESC 223
std::string descriptors[NUM_DESC] = { "blob", "quadruped", "humanoid", "silverfish", "mayfly", "dragonfly",
"damselfly", "stonefly", "earwig", "grasshopper", "cricket", "stick insect", "cockroach", "termite",
"mantis", "louse", "thrips", "aphid", "cicada", "assassin bug", "wasp", "hornet", "tiger beetle",
"ladybug", "weevil", "darkling beetle", "click beetle", "firefly", "scarab beetle", "stag beetle",
"dung beetle", "rhinoceros beetle", "rove beetle", "snakefly", "lacewing", "antlion larva",
"mosquito", "flea", "scorpionfly", "caddisfly", "butterfly", "moth", "caterpillar", "maggot",
"spider", "tarantula", "scorpion", "tick", "mite", "shrimp", "lobster", "crab", "nematode",
"snail", "slug", "earthworm", "leech", "bristleworm", "ribbon worm", "flat worm", "toad", "frog",
"salamander", "newt", "alligator", "crocodile", "lizard", "chameleon", "iguana", "gecko", "skink",
"gila monster", "monitor", "serpent", "viper", "rattlesnake", "cobra", "python", "anaconda", "turtle",
"tortoise", "pterosaur", "dimetrodon", "sauropod", "theropod", "iguanodont", "hadrosaurid", "stegosaurid",
"ceratopsid", "ankylosaurid", "duck", "goose", "swan", "turkey", "grouse", "chicken", "quail", "pheasant",
"gull", "loon", "grebe", "albatross", "petrel", "penguin", "pelican", "stork", "vulture", "flamingo",
"falcon", "kestrel", "condor", "osprey", "buzzard", "eagle", "harrier", "kite", "crane", "dove",
"pigeon", "parrot", "cockatoo", "cuckoo", "nightjar", "swift", "hummingbird", "kingfisher",
"hornbill", "quetzal", "toucan", "woodpecker", "lyrebird", "thornbill", "honeyeater", "oriole",
"fantail", "shrike", "crow", "raven", "magpie", "kinglet", "lark", "swallow", "martin", "bushtit",
"warbler", "thrush", "oxpecker", "starling", "mockingbird", "wren", "nuthatch", "sparrow", "tanager",
"cardinal", "bunting", "finch", "titmouse", "chickadee", "waxwing", "flycatcher", "opossum", "koala",
"wombat", "kangaroo", "sloth", "anteater", "armadillo", "squirrel", "marmot", "beaver", "gopher",
"mouse", "porcupine", "chinchilla", "cavy", "capybara", "rabbit", "hare", "lemur", "loris", "monkey",
"hedgehog", "shrew", "mole", "fruit bat", "wolf", "coyote", "jackal", "raccoon", "coati",
"weasel", "otter", "badger", "skunk", "bear", "panda", "panther", "mongoose", "hyena", "civet",
"walrus", "pangolin", "elephant", "mammoth", "horse", "zebra", "tapir", "rhinoceros", "warthog",
"hippopotamus", "camel", "llama", "giraffe", "deer", "moose", "antelope", "sheep", "goat",
"bison", "buffalo", "bull", "ape", "ant" };

command_result rename_creatures(color_ostream &out, std::vector <std::string> & parameters)
{
    if (!parameters.empty())
        return CR_WRONG_USAGE;
    CoreSuspender suspend;

    int descriptorCount[NUM_DESC] = { 0 };

    if (World::GetPersistentData("AlreadyRenamedCreatures").isValid())
    {
        return CR_OK;
    }

    for (int i = 0; i < world->raws.creatures.all.size(); i++)
    {
        auto creatureRaw = world->raws.creatures.all[i];
        if (!creatureRaw->flags.is_set(df::enums::creature_raw_flags::GENERATED))
            continue;
        size_t minPos = std::string::npos;
        size_t foundIndex = std::string::npos;

        for (size_t j = 0; j < NUM_DESC; j++)
        {
            size_t pos = creatureRaw->caste[0]->description.find(descriptors[j]);
            if (pos < minPos)
            {
                minPos = pos;
                foundIndex = j;
            }
        }

        if (foundIndex < std::string::npos)
        {
            size_t digitPos = creatureRaw->creature_id.find_first_of("0123456789");
            if (digitPos > creatureRaw->creature_id.length())
                digitPos = creatureRaw->creature_id.length();

            creatureRaw->creature_id.replace(digitPos, std::string::npos, descriptors[foundIndex]);

            if (descriptorCount[foundIndex] > 0)
            {
                creatureRaw->creature_id.append(std::to_string(descriptorCount[foundIndex]));
            }

            descriptorCount[foundIndex]++;
        }
    }
    World::AddPersistentData("AlreadyRenamedCreatures");

    return CR_OK;
}

command_result list_creatures(color_ostream &out, std::vector <std::string> & parameters)
{
    bool detailed = false;
    if (!parameters.empty())
    {
        if (parameters.size() > 1)
            return CR_WRONG_USAGE;
        if(parameters[0].compare("detailed") != 0)
            return CR_WRONG_USAGE;
        detailed = true;
    }

    CoreSuspender suspend;
    for (int i = 0; i < world->raws.creatures.all.size(); i++)
    {
        auto creatureRaw = world->raws.creatures.all[i];
        if (!creatureRaw->flags.is_set(df::enums::creature_raw_flags::GENERATED))
            continue;
        out.print(creatureRaw->creature_id.c_str());
        if (detailed)
        {
            out.print("\t");
            out.print(creatureRaw->caste[0]->description.c_str());
        }
        out.print("\n");
    }
}

