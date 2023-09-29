#include "check-structures-sanity.h"

#include "LuaTools.h"
#include "LuaWrapper.h"

DFHACK_PLUGIN("check-structures-sanity");

static command_result command(color_ostream &, std::vector<std::string> &);

DFhackCExport command_result plugin_init(color_ostream &, std::vector<PluginCommand> & commands)
{
    commands.push_back(PluginCommand(
        "check-structures-sanity",
        "performs a sanity check on df-structures",
        command,
        false,
        "check-structures-sanity [-enums] [-sizes] [-lowmem] [-maxerrors n] [-failfast] [starting_point]\n"
        "\n"
        "-enums: report unexpected or unnamed enum or bitfield values.\n"
        "-sizes: report struct and class sizes that don't match structures. (requires sizecheck)\n"
        "-unnamed: report unnamed enum/bitfield values, not just undefined ones.\n"
        "-maxerrors n: set the maximum number of errors before bailing out.\n"
        "-failfast: crash if any error is encountered. useful only for debugging.\n"
        "-maybepointer: report integers that might actually be pointers.\n"
        "starting_point: a lua expression or a word like 'screen', 'item', or 'building'. (defaults to df.global)\n"
        "\n"
        "by default, check-structures-sanity reports invalid pointers, vectors, strings, and vtables."
    ));

    known_types_by_size.clear();
    build_size_table();

    return CR_OK;
}

// returns 0 if MALLOC_PERTURB_ is unset, or if set to 0, because 0 is not useful
uint8_t check_malloc_perturb()
{
    const size_t TEST_DATA_LEN = 5000;  // >1 4kb page
    std::unique_ptr<uint8_t[]> test_data{new uint8_t[TEST_DATA_LEN]};

    uint8_t expected_perturb = test_data[0];
    if (getenv("MALLOC_PERTURB_"))
        expected_perturb = 0xff ^ static_cast<uint8_t>(atoi(getenv("MALLOC_PERTURB_")));

    for (size_t i = 0; i < TEST_DATA_LEN; i++)
        if (expected_perturb != test_data[i])
            return 0;

    return expected_perturb;
}

static command_result command(color_ostream & out, std::vector<std::string> & parameters)
{
    uint8_t perturb_byte = check_malloc_perturb();
    if (!perturb_byte)
        out.printerr("check-structures-sanity: MALLOC_PERTURB_ not set. Some checks may be bypassed or fail.\n");

    CoreSuspender suspend;

    Checker checker(out);
    checker.perturb_byte = perturb_byte;

    // check parameters with values first
#define VAL_PARAM(name, expr_using_value) \
    auto name ## _idx = std::find(parameters.begin(), parameters.end(), "-" #name); \
    if (name ## _idx != parameters.end()) \
    { \
        if (name ## _idx + 1 == parameters.end()) \
        { \
            return CR_WRONG_USAGE; \
        } \
        try \
        { \
            auto value = std::move(*(name ## _idx + 1)); \
            parameters.erase((name ## _idx + 1)); \
            parameters.erase(name ## _idx); \
            checker.name = (expr_using_value); \
        } \
        catch (std::exception & ex) \
        { \
            out.printerr("check-structures-sanity: argument to -%s: %s\n", #name, ex.what()); \
            return CR_WRONG_USAGE; \
        } \
    }
    VAL_PARAM(maxerrors, std::stoul(value));
#undef VAL_PARAM

#define BOOL_PARAM(name) \
    auto name ## _idx = std::find(parameters.begin(), parameters.end(), "-" #name); \
    if (name ## _idx != parameters.end()) \
    { \
        checker.name = true; \
        parameters.erase(name ## _idx); \
    }
    BOOL_PARAM(enums);
    BOOL_PARAM(sizes);
    BOOL_PARAM(unnamed);
    BOOL_PARAM(failfast);
    BOOL_PARAM(noprogress);
    BOOL_PARAM(maybepointer);
#undef BOOL_PARAM

    if (parameters.size() > 1)
    {
        return CR_WRONG_USAGE;
    }

    if (parameters.empty())
    {
        checker.queue_globals();
    }
    else
    {
        using namespace DFHack::Lua;
        using namespace DFHack::Lua::Core;
        using namespace DFHack::LuaWrapper;

        StackUnwinder unwinder(State);
        PushModulePublic(out, "utils", "df_expr_to_ref");
        Push(parameters.at(0));
        if (!SafeCall(out, 1, 1))
        {
            return CR_FAILURE;
        }
        if (!lua_touserdata(State, -1))
        {
            return CR_WRONG_USAGE;
        }

        QueueItem item(parameters.at(0), get_object_ref(State, -1));
        lua_getfield(State, -1, "_type");
        lua_getfield(State, -1, "_identity");
        auto identity = reinterpret_cast<type_identity *>(lua_touserdata(State, -1));
        if (!identity)
        {
            out.printerr("could not determine type identity\n");
            return CR_FAILURE;
        }

        checker.queue_item(item, CheckedStructure(identity));
    }

    while (checker.process_queue())
    {
        if (!checker.noprogress)
        {
            out << "checked " << checker.checked_count << " fields\r" << std::flush;
        }
    }

    out << "checked " << checker.checked_count << " fields" << std::endl;

    return checker.error_count ? CR_FAILURE : CR_OK;
}
