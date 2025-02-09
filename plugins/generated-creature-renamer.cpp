#include "Console.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "df/world.h"
#include "df/creature_raw.h"
#include "df/caste_raw.h"
#include "modules/World.h"
#include "MemAccess.h"

//#include "df/world.h"

using namespace DFHack;
using std::endl;

DFHACK_PLUGIN("generated-creature-renamer");
REQUIRE_GLOBAL(world);

#define RENAMER_VERSION 3

command_result list_creatures(color_ostream &out, std::vector <std::string> & parameters);
command_result save_generated_raw(color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init(color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "list-generated",
        "Prints a list of generated creature tokens.",
        list_creatures));
    commands.push_back(PluginCommand(
        "save-generated-raws",
        "Exports a graphics raw file for the renamed generated creatures.",
        save_generated_raw));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown(color_ostream &out)
{
    return CR_OK;
}

DFHACK_PLUGIN_IS_ENABLED(is_enabled);

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    is_enabled = enable;
    return CR_OK;
}

std::vector<std::string> descriptors = {
    "blob",        "quadruped",       "humanoid",     "silverfish",    "mayfly",        "dragonfly",   "damselfly",    "stonefly",
    "earwig",      "grasshopper",     "cricket",      "stick insect",  "cockroach",     "termite",     "mantis",       "louse",
    "thrips",      "aphid",           "cicada",       "assassin bug",  "wasp",          "hornet",      "tiger beetle", "ladybug",
    "weevil",      "darkling beetle", "click beetle", "firefly",       "scarab beetle", "stag beetle", "dung beetle",  "rhinoceros beetle",
    "rove beetle", "snakefly",        "lacewing",     "antlion larva", "mosquito",      "flea",        "scorpionfly",  "caddisfly",
    "butterfly",   "moth",            "caterpillar",  "maggot",        "spider",        "tarantula",   "scorpion",     "tick",
    "mite",        "shrimp",          "lobster",      "crab",          "nematode",      "snail",       "slug",         "earthworm",
    "leech",       "bristleworm",     "ribbon worm",  "flat worm",     "toad",          "frog",        "salamander",   "newt",
    "alligator",   "crocodile",       "lizard",       "chameleon",     "iguana",        "gecko",       "skink",        "gila monster",
    "monitor",     "serpent",         "viper",        "rattlesnake",   "cobra",         "python",      "anaconda",     "turtle",
    "tortoise",    "pterosaur",       "dimetrodon",   "sauropod",      "theropod",      "iguanodont",  "hadrosaurid",  "stegosaurid",
    "ceratopsid",  "ankylosaurid",    "duck",         "goose",         "swan",          "turkey",      "grouse",       "chicken",
    "quail",       "pheasant",        "gull",         "loon",          "grebe",         "albatross",   "petrel",       "penguin",
    "pelican",     "stork",           "vulture",      "flamingo",      "falcon",        "kestrel",     "condor",       "osprey",
    "buzzard",     "eagle",           "harrier",      "kite",          "crane",         "dove",        "pigeon",       "parrot",
    "cockatoo",    "cuckoo",          "nightjar",     "swift",         "hummingbird",   "kingfisher",  "hornbill",     "quetzal",
    "toucan",      "woodpecker",      "lyrebird",     "thornbill",     "honeyeater",    "oriole",      "fantail",      "shrike",
    "crow",        "raven",           "magpie",       "kinglet",       "lark",          "swallow",     "martin",       "bushtit",
    "warbler",     "thrush",          "oxpecker",     "starling",      "mockingbird",   "wren",        "nuthatch",     "sparrow",
    "tanager",     "cardinal",        "bunting",      "finch",         "titmouse",      "chickadee",   "waxwing",      "flycatcher",
    "opossum",     "koala",           "wombat",       "kangaroo",      "sloth",         "anteater",    "armadillo",    "squirrel",
    "marmot",      "beaver",          "gopher",       "mouse",         "porcupine",     "chinchilla",  "cavy",         "capybara",
    "rabbit",      "hare",            "lemur",        "loris",         "monkey",        "hedgehog",    "shrew",        "mole",
    "fruit bat",   "wolf",            "coyote",       "jackal",        "raccoon",       "coati",       "weasel",       "otter",
    "badger",      "skunk",           "bear",         "panda",         "panther",       "mongoose",    "hyena",        "civet",
    "walrus",      "pangolin",        "elephant",     "mammoth",       "horse",         "zebra",       "tapir",        "rhinoceros",
    "warthog",     "hippopotamus",    "camel",        "llama",         "giraffe",       "deer",        "moose",        "antelope",
    "sheep",       "goat",            "bison",        "buffalo",       "bull",          "ape",         "ant",          "bat",
    "owl",         "pig",             "bee",          "fly",           "hawk",          "jay",         "rat",          "fox",
    "cat",         "ass",             "elk"
};

std::vector<std::string> prefixes = {
    "FORGOTTEN_BEAST_",
    "TITAN_",
    "DEMON_",
    "NIGHT_CREATURE_",
    "HF"
};


DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if (!is_enabled)
        return CR_OK;

    if (event != DFHack::SC_WORLD_LOADED)
        return CR_OK;

    std::vector<int> descriptorCount = std::vector<int>(descriptors.size());

    auto version = World::GetPersistentData("AlreadyRenamedCreatures");
    if (version.isValid() && version.ival(1) >= RENAMER_VERSION)
    {
        return CR_OK;
    }

    int creatureCount = 0;

    for (size_t i = 0; i < world->raws.creatures.all.size(); i++)
    {
        auto creatureRaw = world->raws.creatures.all[i];
        if (!creatureRaw->flags.is_set(df::enums::creature_raw_flags::GENERATED))
            continue;
        size_t minPos = std::string::npos;
        size_t foundIndex = -1;
        size_t prefixIndex = -1;

        for (size_t j = 0; j < prefixes.size(); j++)
        {
            if (creatureRaw->creature_id.compare(0, prefixes[j].length(), prefixes[j]) == 0)
            {
                prefixIndex = j;
            }
        }

        if (prefixIndex < 0)
            continue; //unrecognized generaed type.

        for (size_t j = 0; j < descriptors.size(); j++)
        {
            size_t pos = creatureRaw->caste[0]->description.find(" " + descriptors[j]);
            if (pos < minPos)
            {
                minPos = pos;
                foundIndex = j;
            }
        }

        if (foundIndex < 0)
            continue; //can't find a match.

        auto descriptor = descriptors[foundIndex];

        for (size_t j = 0; j < descriptor.size(); j++)
        {
            if (descriptor[j] == ' ')
                descriptor[j] = '_';
            else
                descriptor[j] = toupper(descriptor[j]);
        }

        auto prefix = prefixes[prefixIndex];
        if (prefix[prefix.length() - 1] != '_')
            prefix.append("_");

        creatureRaw->creature_id = prefixes[prefixIndex] + descriptor;

        if (descriptorCount[foundIndex] > 0)
        {
            creatureRaw->creature_id.append("_" + std::to_string(descriptorCount[foundIndex]));
        }
        descriptorCount[foundIndex]++;
        creatureCount++;
    }

    version = World::AddPersistentData("AlreadyRenamedCreatures");
    version.ival(1) = RENAMER_VERSION;

    out << "Renamed " << creatureCount << " generated creatures to have sensible names." << endl;


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

    for (size_t i = 0; i < world->raws.creatures.all.size(); i++)
    {
        auto creatureRaw = world->raws.creatures.all[i];
        if (!creatureRaw->flags.is_set(df::enums::creature_raw_flags::GENERATED))
            continue;
        out.print("%s",creatureRaw->creature_id.c_str());
        if (detailed)
        {
            out.print("\t%s",creatureRaw->caste[0]->description.c_str());
        }
        out.print("\n");
    }
    return CR_OK;
}

command_result save_generated_raw(color_ostream &out, std::vector <std::string> & parameters)
{
#ifdef LINUX_BUILD
    std::string pathSep =  "/";
#else
    std::string pathSep = "\\";
#endif
    int pageWidth = 16;
    int pageHeight = (descriptors.size() / pageWidth) + ((descriptors.size() % pageWidth > 0) ? 1 : 0);
    int tileWidth = 24;
    int tileHeight = 24;
    std::string fileName = "graphics_procedural_creatures";
    std::string pageName = "PROCEDURAL_FRIENDLY";
    size_t repeats = 128;

    std::ofstream outputFile(fileName + ".txt", std::ios::out | std::ios::trunc);

    outputFile << fileName << endl << endl;

    outputFile << "[OBJECT:GRAPHICS]" << endl << endl;

    outputFile << "[TILE_PAGE:" << pageName << "]" << endl;
    outputFile << "    [FILE:procedural_friendly.png]" << endl;
    outputFile << "    [TILE_DIM:" << tileWidth << ":" << tileHeight << "]" << endl;
    outputFile << "    [PAGE_DIM:" << pageWidth << ":" << pageHeight << "]" << endl << endl;

    for (size_t descIndex = 0; descIndex < descriptors.size(); descIndex++)
    {
        for (size_t prefIndex = 0; prefIndex < prefixes.size(); prefIndex++)
        {
            for (size_t rep = 0; rep < repeats; rep++)
            {
                auto descriptor = descriptors[descIndex];

                for (size_t j = 0; j < descriptor.size(); j++)
                {
                    if (descriptor[j] == ' ')
                        descriptor[j] = '_';
                    else
                        descriptor[j] = toupper(descriptor[j]);
                }

                auto prefix = prefixes[prefIndex];
                if (prefix[prefix.length() - 1] != '_')
                    prefix.append("_");

                std::string token = prefix + descriptor;
                if (rep > 0)
                    token.append("_" + std::to_string(rep));

                outputFile << "[CREATURE_GRAPHICS:" << token << "]" << endl;
                outputFile << "    [DEFAULT:" << pageName << ":" << descIndex % pageWidth << ":" << descIndex / pageWidth << ":ADD_COLOR]" << endl;
            }
            outputFile << endl;
        }
        outputFile << endl;
    }

    outputFile.close();

    return CR_OK;
}
