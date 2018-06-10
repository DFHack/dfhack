/**
Copyright © 2018 Pauli <suokkos@gmail.com>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
   must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
 */

#include "Core.h"
#include "PluginManager.h"
#include "DebugManager.h"
#include "Debug.h"
#include "modules/Filesystem.h"

#include <jsoncpp-ex.h>

#include <algorithm>
#include <array>
#include <map>
#include <set>
#include <mutex>
#include <regex>
#include <cwchar>

DFHACK_PLUGIN("debug");

DBG_DECLARE(debug,filter);
DBG_DECLARE(debug,init);
DBG_DECLARE(debug,command);
DBG_DECLARE(debug,ui);

namespace serialization {

template<typename T>
struct nvp : public std::pair<const char*, T*> {
    using parent_t = std::pair<const char*, T*>;
    nvp(const char* name, T& value) :
        parent_t{name, &value}
    {}
};

template<typename T>
nvp<T> make_nvp(const char* name, T& value) {
    return {name, value};
}

}

#define NVP(variable) serialization::make_nvp(#variable, variable)

namespace Json {
template<typename ET>
typename std::enable_if<std::is_enum<ET>::value, ET>::type
get(Json::Value& ar, const std::string &key, const ET& default_)
{
    return static_cast<ET>(as<UInt64>(ar.get(key, static_cast<uint64_t>(default_))));
}
}

namespace DFHack { namespace debugPlugin {

using JsonArchive = Json::Value;

//! Helper to map types to Json::Value::as*() calls
//! This could be moved to Json::Value to allow use from everywhere
template<typename T, int Void>
T as(JsonArchive&);

template<typename T,
    typename std::enable_if<std::is_integral<T>::value, int>::type = 0>
T as(JsonArchive& ar)
{
    if (sizeof(T) == 8) {
        if (std::is_unsigned<T>::value)
            return ar.asUInt64();
        else
            return ar.asInt64();
    } else {
        if (std::is_unsigned<T>::value)
            return ar.asUInt();
        else
            return ar.asInt();
    }
}

template<typename T,
    typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
T as(JsonArchive& ar)
{
    if (sizeof(T) == 4)
        return ar.asFloat();
    else
        return ar.asDouble();
}

template<typename T,
    typename std::enable_if<std::is_enum<T>::value, int>::type = 0>
T as(JsonArchive& ar)
{
    std::stringstream ss(ar.asString());
    size_t rv;
    ss >> rv;
    return static_cast<T>(rv);
}

template<>
bool as<bool>(JsonArchive& ar)
{
    return ar.asBool();
}

template<>
std::string as<std::string, 0>(JsonArchive& ar)
{
    return ar.asString();
}

//! Write a named and type value to Json::Value. enable_if makes sure this is
//! only available for types that Json::Value supports directly.
template<typename T,
    typename std::enable_if<std::is_constructible<Json::Value, T>::value, int>::type = 0>
JsonArchive& operator<<(JsonArchive& ar, const serialization::nvp<T>& target)
{
    ar[target.first] = *target.second;
    return ar;
}

//! Read a named and typed value from Json::Value
template<typename T>
JsonArchive& operator>>(JsonArchive& ar, const serialization::nvp<T>& target)
{
    *target.second = Json::get(ar, target.first, T{});
    return ar;
}

/*!
 * Default regex flags optimized for matching speed because objects are stored
 * long time in memory.
 */
static constexpr auto defaultRegex =
    std::regex::optimize | std::regex::nosubs;

static const char* const commandHelp =
    "  Manage runtime debug print filters.\n"
    "\n"
    "  debugfilter category [<category regex> [<plugin regex>]]\n"
    "    List categories matching regular expressions.\n"
    "  debugfilter filter [<filter id>]\n"
    "    List active filters or show detailed information for a filter.\n"
    "  debugfilter set [persistent] <level> [<category regex> [<plugin regex>]]\n"
    "    Set a filter level to categories matching regular expressions.\n"
    "  debugfilter unset <filter id> [<filter id> ...]\n"
    "    Unset filters matching space separated list of ids from 'filter'.\n"
    "  debugfilter disable <filter id> [<filter id> ...]\n"
    "    Disable filters matching space separated list of ids from 'filter'.\n"
    "  debugfilter enable <filter id> [<filter id> ...]\n"
    "    Enable filters matching space separated list of ids from 'filter'.\n"
    "  debugfilter ui\n"
    "    Show interactive user interface in Dwarf Fortress.\n"
    "  debugfilter help [<subcommand>]\n"
    "    Show detailed help for a command or this help.\n";
static const char* const commandCategory =
    "  category [<category regex> [<plugin regex>]]\n"
    "    List categories with optional filters. Parameters are passed to\n"
    "    std::regex to limit which once are shown. The first regular\n"
    "    expression is used to match category and the second is used match\n"
    "    plugin name.\n";
static const char* const commandSet =
    "  set [persistent] <level> [<category regex> [<plugin regex>]]\n"
    "    Set filtering level for matching categories. 'level' must be one of\n"
    "    trace, debug, info, warning and error. The 'level' parameter sets\n"
    "    the lowest message level that will be shown. The command doesn't\n"
    "    allow filters to disable any error messages.\n"
    "    Default filter life time is until Dwarf Fortress process exists or\n"
    "    plugin is unloaded. Passing 'persistent' as second parameter tells\n"
    "    the plugin to store the filter to dfhack-config. Stored filters\n"
    "    will be active until always when the plugin is loaded. 'unset'\n"
    "    command can be used to remove persistent filters.\n"
    "    Filters are applied FIFO order. The latest filter will override any\n"
    "    older filter that also matches.\n";
static const char* const commandFilters =
    "  filter [<filter id>]\n"
    "    Show the list of active filters. The first column is 'id' which can\n"
    "    be used to deactivate filters using 'unset' command.\n"
    "    Filters are printed in same order as applied - the oldest first.\n";
static const char* const commandUnset =
    "  unset <filter id> [<filter id> ...]\n"
    "    'unset' takes space separated list of filter ids from 'filter'.\n"
    "    It will reset any matching category back to the default 'warning'\n"
    "    level or any other still active matching filter level.\n";
static const char* const commandDisable =
    "  disable <filter id> [<filter id> ...]\n"
    "    'disable' takes space separated list of filter ids from 'filter'.\n"
    "    It will reset any matching category back to the default 'warning'\n"
    "    level or any other still active matching filter level.\n"
    "    'disable' will print red filters that were already disabled.\n";
static const char* const commandEnable =
    "  enable <filter id> [<filter id> ...]\n"
    "    'enable' takes space separated list of filter ids from 'filter'.\n"
    "    It will reset any matching category back to the default 'warning'\n"
    "    level or any other still active matching filter level.\n"
    "    'enable' will print red filters that were already enabled.\n";
static const char* const commandUI =
    "  ui\n"
    "    Show interactive user interface in the Dwarf Fortress window. It\n"
    "    has same features as commands but might be easier to use in some\n"
    "    cases.\n";
static const char* const commandHelpDetails =
    "  help [<subcommand>]\n"
    "    Show help for any of subcommands. Without any parameters it shows\n"
    "    short help for all subcommands.\n";

//! Helper type to hold static dispatch table for subcommands
struct CommandDispatch {
    //! Store handler function pointer and help message for commands
    struct Command {
        using handler_t = command_result(*)(color_ostream&,std::vector<std::string>&);
        Command(handler_t handler, const char* help) :
            handler_(handler),
            help_(help)
        {}

        command_result operator()(color_ostream& out,
                std::vector<std::string>& parameters) const noexcept
        {
            return handler_(out, parameters);
        }

        handler_t handler() const noexcept
        { return handler_; }

        const char* help() const noexcept
        { return help_; }
    private:
        handler_t handler_;
        const char* help_;
    };
    using dispatch_t = const std::map<std::string, Command>;
    //! Name to handler function and help message mapping
    static dispatch_t dispatch;
};

//! List of DebugCategory::level's in human readable form
static const std::array<const std::string,5> levelNames{
    "Trace",
    "Debug",
    "Info",
    "Warning",
    "Error",
};

/*!
 * Filter applies a runtime filter to matching  DFHack::DebugCategory 's.
 * Filters are stored in DFHack::debugPlugin::FilterManager which applies
 * filters to dynamically added categories. The manager also stores and loads
 * persistent Filters from the configuration file.
 */
struct Filter {
    explicit Filter(DebugCategory::level level,
            const std::string& categoryText,
            const std::regex& category,
            const std::string& pluginText,
            const std::regex& plugin,
            bool persistent = true,
            bool enabled = true) noexcept;

    explicit Filter(DebugCategory::level level,
            const std::string& categoryText,
            const std::string& pluginText,
            bool persistent = true,
            bool enabled = true);

    explicit Filter() = default;

    //! Apply the filter to a category if regex's match
    void apply(DFHack::DebugCategory& cat) noexcept;
    //! Apply the filter to a category that has been already matched
    bool applyAgain(DFHack::DebugCategory& cat) const noexcept;
    //! Remove the category from matching count if regex's match
    //! \return true if regex's matched
    bool remove(const DFHack::DebugCategory& cat) noexcept;

    //! Query if filter is enabled
    bool enabled() const noexcept {return enabled_;}
    //! Set the enable status of filter
    void enabled(bool enable) noexcept;
    //! Query if filter is persistent
    bool persistent() const noexcept {return persistent_;}
    //! Query if the filter level
    bool level() const noexcept {return level_;}
    //! Query number of matching categories
    size_t matches() const noexcept {return matches_;}
    //! Add matches count for the initial category filter matching
    void addMatch() noexcept {++matches_;}
    //! Return the category filter text
    const std::string& categoryText() const noexcept {return categoryText_;}
    //! Return the plugin filter text
    const std::string& pluginText() const noexcept {return pluginText_;}

    //! Load Filter from configuration file. Second parameter would be version
    //! number if format changes in future to include more fields. Then new
    //! fields would have to be loaded conditionally
    void load(JsonArchive& ar, const unsigned int)
    {
        ar >> NVP(categoryText_)
            >> NVP(pluginText_)
            >> NVP(enabled_)
            >> NVP(level_);
        TRACE(filter) << "Loading filter cat: " << categoryText_
            << " plug: " << pluginText_
            << " ena " << enabled_
            << " level: " << level_
            << std::endl;
        persistent_ = true;
        matches_ = 0;
        category_ = std::regex{categoryText_};
        plugin_ = std::regex{pluginText_};
    }

    //! Save the Filter to json configuration file
    void save(JsonArchive& ar, const unsigned int) const
    {
        ar << NVP(categoryText_)
            << NVP(pluginText_)
            << NVP(enabled_)
            << NVP(level_);
    }

private:
    std::regex category_;
    std::regex plugin_;
    DebugCategory::level level_;
    size_t matches_;
    bool persistent_;
    bool enabled_;
    std::string categoryText_;
    std::string pluginText_;
};

Filter::Filter(DebugCategory::level level,
        const std::string& categoryText,
        const std::regex& category,
        const std::string& pluginText,
        const std::regex& plugin,
        bool persistent,
        bool enabled) noexcept :
    category_{category},
    plugin_{plugin},
    level_{level},
    matches_{0},
    persistent_{persistent},
    enabled_{enabled},
    categoryText_{categoryText},
    pluginText_{pluginText}
{}

Filter::Filter(DebugCategory::level level,
        const std::string& categoryText,
        const std::string& pluginText,
        bool persistent,
        bool enabled) :
    Filter{
        level,
        categoryText,
        std::regex{categoryText, defaultRegex},
        pluginText,
        std::regex{pluginText, defaultRegex},
        persistent,
        enabled,
    }
{}

void Filter::enabled(bool enable) noexcept
{
    if (enable == enabled_)
        return;
    enabled_ = enable;
}

bool Filter::applyAgain(DebugCategory& cat) const noexcept
{
    if (!enabled_)
        return false;
    if (!std::regex_search(cat.category(), category_))
        return false;
    if (!std::regex_search(cat.category(), plugin_))
        return false;
    TRACE(filter) << "apply " << cat.plugin() << ':' << cat.category() << " matches '"
        << pluginText() << "' '" << categoryText() << std::endl;
    cat.allowed(level_);
    return true;
}

void Filter::apply(DebugCategory& cat) noexcept
{
    if (applyAgain(cat))
        ++matches_;
}

bool Filter::remove(const DebugCategory& cat) noexcept
{
    if (!enabled_)
        return false;
    if (!std::regex_search(cat.category(), category_))
        return false;
    if (!std::regex_search(cat.category(), plugin_))
        return false;
    TRACE(filter) << "remove " << cat.plugin() << ':' << cat.category() << " matches '"
        << pluginText() << "' '" << categoryText() << std::endl;
    --matches_;
    return true;
}

/**
 * Storage for enabled and disabled filters. It uses ordered map because common
 * case is to iterate filters from oldest to the newest.
 *
 * Data races: any reader and writer must lock
 * DFHack::DebugManager::access_lock_. Sharing DebugManager make sense because
 * most of functionality related to filters requires locking DebugManager too.
 * Remaining functionality can share same lock because they are rarely called
 * functions.
 */
struct FilterManager : public std::map<size_t, Filter>
{
    using parent_t = std::map<size_t, Filter>;
    //! Current configuration version implemented by the code
    constexpr static size_t configVersion{1};
    //! Path to the configuration file
    constexpr static const char* configPath{"dfhack-config/runtime-debug.json"};

    //! Get reference to the singleton
    static FilterManager& getInstance() noexcept
    {
        static FilterManager instance;
        return instance;
    }

    //! Add a new filter which gets automatically a new id
    template<typename... Args>
    std::pair<iterator,bool> emplaceNew( Args&&... args ) {
        return emplace(std::piecewise_construct,
                std::forward_as_tuple(nextId_++),
                std::forward_as_tuple(std::forward<Args>(args)...));
    }

    //! Load state from the configuration file
    DFHack::command_result loadConfig(DFHack::color_ostream& out) noexcept;
    //! Save state to the configuration file
    DFHack::command_result saveConfig(DFHack::color_ostream& out) const noexcept;

    //! Connect FilterManager to DFHack::DebugManager::categorySignal
    void connectTo(DebugManager::categorySignal_t& signal) noexcept;
    //! Disonnect FilterManager from DFHack::DebugManager::categorySignal
    void disconnectFrom(DebugManager::categorySignal_t& signal) noexcept;
    //! Temporary block DFHack::DebugManager::categorySignal
    DebugManager::categorySignal_t::BlockGuard blockSlot(DebugManager::categorySignal_t& signal) noexcept;

    ~FilterManager();

    FilterManager(const FilterManager&) = delete;
    FilterManager(FilterManager&&) = delete;
    FilterManager& operator=(FilterManager) = delete;
    FilterManager& operator=(FilterManager&&) = delete;

    void load(JsonArchive& ar)
    {
        size_t version = -1;
        ar >> serialization::make_nvp("configVersion", version);
        if (version > configVersion) {
            std::stringstream ss;
            ss << "The saved config version ("
               << version
               << ") is newer than code supported version ("
               << configVersion
               << ")";
            throw std::runtime_error{ss.str()};
        }
        ar >> NVP(nextId_);
        JsonArchive& children = ar["children"];
        for (auto iter = children.begin(); iter != children.end(); ++iter) {
            Filter child;
            child.load(*iter, version);
            std::stringstream ss(iter.name());
            size_t id;
            ss >> id;
            insert(std::make_pair(id, child));
        }
    }

    void save(JsonArchive& ar) const
    {
        JsonArchive children{Json::objectValue};
        for (const auto& filterPair: *this) {
            if (!filterPair.second.persistent())
                continue;
            std::stringstream ss;
            ss << filterPair.first;
            filterPair.second.save(children[ss.str()], configVersion);
        }
        auto configVersion = FilterManager::configVersion;
        ar << NVP(configVersion)
            << NVP(nextId_);
        ar["children"] = children;
    }

private:
    FilterManager() = default;
    //! The next integer to use as id to have each with unique number
    size_t nextId_;
    DebugManager::categorySignal_t::Connection connection_;
};

FilterManager::~FilterManager()
{
    auto& catMan = DFHack::DebugManager::getInstance();
    std::lock_guard<std::mutex> lock(catMan.access_mutex_);
    disconnectFrom(catMan.categorySignal);
}

command_result FilterManager::loadConfig(DFHack::color_ostream& out) noexcept
{
    nextId_ = 1;
    if (!Filesystem::isfile(configPath))
        return CR_OK;
    try {
        DEBUG(command, out) << "Load config from '" << configPath << "'" << std::endl;
        JsonArchive archive;
        std::ifstream ifs(configPath);
        if (!ifs.good())
            throw std::runtime_error{"Failed to open configuration file for reading"};
        ifs >> archive;
        load(archive);
    } catch(std::exception& e) {
        ERR(command, out) << "Serializing filters from '" << configPath << "' failed: "
            << e.what() << std::endl;
        return CR_FAILURE;
    }
    return CR_OK;
}

command_result FilterManager::saveConfig(DFHack::color_ostream& out) const noexcept
{
    try {
        DEBUG(command, out) << "Save config to '" << configPath << "'" << std::endl;
        JsonArchive archive;
        save(archive);
        std::ofstream ofs(configPath);
        if (!ofs.good())
            throw std::runtime_error{"Failed to open configuration file for writing"};
        ofs << archive;
    } catch(std::exception e) {
        ERR(command, out) << "Serializing filters to '" << configPath << "' failed: "
            << e.what() << std::endl;
        return CR_FAILURE;
    }
    return CR_OK;
}

void FilterManager::connectTo(DebugManager::categorySignal_t& signal) noexcept
{
    connection_ = signal.connect(
            [this](DebugManager::signalType t, DebugCategory& cat) {
                TRACE(filter) << "sig type: " <<  t << ' '
                    << cat.plugin() << ':'
                    << cat.category() << std::endl;
                switch (t) {
                case DebugManager::CAT_ADD:
                    for (auto& filterPair: *this)
                        filterPair.second.apply(cat);
                    break;
                case DebugManager::CAT_REMOVE:
                    for (auto& filterPair: *this)
                        filterPair.second.remove(cat);
                    break;
                case DebugManager::CAT_MODIFIED:
                    break;
                }
            });
}

void FilterManager::disconnectFrom(DebugManager::categorySignal_t& signal) noexcept
{
    connection_.disconnect(signal);
}

DebugManager::categorySignal_t::BlockGuard FilterManager::blockSlot(DebugManager::categorySignal_t& signal) noexcept
{
    TRACE(filter) << "Temporary disable FilterManager::connection_" << std::endl;
    return {signal, connection_};
}

//! \brief Helper to parse optional regex string safely
static command_result parseRegexParam(std::regex& target,
        color_ostream& out,
        std::vector<std::string>& parameters,
        size_t pos)
{
    if (parameters.size() <= pos)
        return CR_OK;
    try {
        std::regex temp{parameters[pos], defaultRegex};
        target = std::move(temp);
    } catch(std::regex_error e) {
        ERR(command,out) << "Failed to parse regular expression '"
            << parameters[pos] << "'\n";
        ERR(command,out) << "Parser message: " <<  e.what() << std::endl;
        return CR_WRONG_USAGE;
    }
    return CR_OK;
}

/*!
 * "Algorithm" to apply category filters based on optional regex parameters.
 * \param out The output stream where errors should be written
 * \param paramters The list of parameters for the command
 * \param pos The position where first optional regular expression parameter is
 * \param header The callback after locking DebugManager and before loop
 * \param categoryMatch The callback for each matching category
 */
template<typename Callable1, typename Callable2, typename Callable3>
static command_result applyCategoryFilters(color_ostream& out,
        std::vector<std::string>& parameters,
        size_t pos, Callable1 header,
        Callable2 categoryMatch,
        Callable3 listComplete)
{
    std::regex pluginRegex{".", defaultRegex};
    std::regex categoryRegex{".", defaultRegex};
    command_result rv = CR_OK;

    DEBUG(command,out) << "applying category filters '"
        << (parameters.size() >= pos + 1 ? parameters[pos] : "")
        << "' and plugin filter '"
        << (parameters.size() >= pos + 2 ? parameters[pos+1] : "")
        << '\'' << std::endl;
    // Parse parameters
    if ((rv = parseRegexParam(categoryRegex, out, parameters, pos)) != CR_OK)
        return rv;
    if ((rv = parseRegexParam(pluginRegex, out, parameters, pos+1)) != CR_OK)
        return rv;
    // Lock the manager to have consistent view of categories
    auto& manager = DebugManager::getInstance();
    std::lock_guard<std::mutex> lock(manager.access_mutex_);
    out << std::left;
    auto guard = header(manager, categoryRegex, pluginRegex);
    for (auto* category: manager) {
        DebugCategory::cstring_ref p = category->plugin();
        DebugCategory::cstring_ref c = category->category();
        // Apply filters to the category and plugin names
        if (!std::regex_search(c, categoryRegex))
            continue;
        if (!std::regex_search(p, pluginRegex))
            continue;
        categoryMatch(*category);
    }
    out << std::flush << std::right;
    out.color(COLOR_RESET);
    return listComplete();
}

static void printCategoryListHeader(color_ostream& out)
{
    // Output the header.
    out.color(COLOR_GREEN);
    out << std::setw(12) << "Plugin"
        << std::setw(12) << "Category"
        << std::setw(18) << "Lowest printed" << '\n';
}

static void printCategoryListEntry(color_ostream& out,
        unsigned& line,
        DebugCategory& cat,
        DebugCategory::level old = static_cast<DebugCategory::level>(-1))
{
    if ((line & 31) == 0)
        printCategoryListHeader(out);
    // Output matching categories.
    out.color((line++ & 1) == 0 ? COLOR_CYAN : COLOR_LIGHTCYAN);
    const std::string& level = (old != static_cast<DebugCategory::level>(-1)) ?
        levelNames[static_cast<unsigned>(old)] + "->" +
            levelNames[static_cast<unsigned>(cat.allowed())] :
        levelNames[static_cast<unsigned>(cat.allowed())];
    out << std::setw(12) << cat.plugin()
        << std::setw(12) << cat.category()
        << std::setw(18) << level << '\n';
}

//! Handler for debugfilter category
static command_result listCategories(color_ostream& out,
        std::vector<std::string>& parameters)
{
    unsigned line = 0;
    return applyCategoryFilters(out, parameters, 1u,
            [](DebugManager&, const std::regex&, const std::regex&) {
                return 0;
            },
            [&out, &line](DebugCategory& cat) {
                printCategoryListEntry(out, line, cat);
            },
            []() {return CR_OK;});
}

//! Type that prints parameter string in center of output stream field
template<typename CT, typename TT = std::char_traits<CT>>
struct center {
    using string = std::basic_string<CT, TT>;
    center(const string& str) :
        str_(str)
    {}

    const string& str_;
};

/*!
 * Helper to construct a center object to print fields centered
 * \code{.cpp}
 * out << std::setw(20) << centered("centered"_s);
 * \endcode
 */
template<typename ST>
center<typename ST::value_type, typename ST::traits_type> centered(const ST& str)
{
    return {str};
}

//! c++14 string conversion literal to std::string
std::string operator "" _s(const char* cstr, size_t len)
{
    return {cstr, len};
}

//! Output centered string, the stream must be using std::ios::right
//! \sa DFHack::debugPlugin::centered
template<typename CT, typename TT>
std::basic_ostream<CT, TT>& operator<<(std::basic_ostream<CT, TT>& os, const center<CT, TT>& toCenter)
{
    std::streamsize w = os.width();
    const auto& str = toCenter.str_;
    mbstate_t ps{};
    std::streamsize ccount = 0;
    auto iter = str.cbegin();
    // Check multibyte character length. It will break when mbrlen find the '\0'
    for (;ccount < w; ++ccount) {
        const size_t n = std::distance(iter, str.cend());
        ssize_t bytes = mbrlen(&*iter, n, &ps);
        if (bytes <= 0) /* Check for errors and null */
            break;
        std::advance(iter, bytes);
    }

    if (ccount < w) {
        // Center the character when there is less than the width
        // fillw = w - count
        // cw = w - (fillw)/2 = (2w - w + count)/2
        // Extra one for rounding half results up
        std::streamsize cw = (w + ccount + 1)/2;
        os << std::setw(cw) << str
            << std::setw(w - cw) << "";
    } else {
        // Truncate characters to the width of field
        os.write(&str[0], std::distance(str.begin(), iter));
        // Reset the field width because we wrote the string with write
        os << std::setw(0);
    }
    return os;
}

static FilterManager::iterator parseFilterId(color_ostream& out,
        const std::string& parameter)
{
    unsigned long id = 0;
    try {
        id = std::stoul(parameter);
    } catch(...) {
    }
    auto& filMan = FilterManager::getInstance();
    auto iter = filMan.find(id);
    if (iter == filMan.end()) {
        WARN(command,out) << "The optional parameter ("
            << parameter << ") must be an filter id." << std::endl;
    }
    return iter;
}

static void printFilterListEntry(color_ostream& out,
        unsigned line,
        color_value lineColor,
        size_t id,
        const Filter& filter)
{
    if ((line & 31) == 0) {
        out.color(COLOR_GREEN);
        out << std::setw(4) << "ID"
            << std::setw(8) << "enabled"
            << std::setw(8) << "persist"
            << std::setw(9) << centered("level"_s)
            << ' '
            << std::setw(15) << centered("category"_s)
            << ' '
            << std::setw(15) << centered("plugin"_s)
            << std::setw(8) << "matches"
            << '\n';
    }
    out.color(lineColor);
    out << std::setw(4) << id
        << std::setw(8) << centered(filter.enabled() ? "X"_s:""_s)
        << std::setw(8) << centered(filter.persistent() ? "X"_s:""_s)
        << std::setw(9) << centered(levelNames[filter.level()])
        << ' '
        << std::setw(15) << centered(filter.categoryText())
        << ' '
        << std::setw(15) << centered(filter.pluginText())
        << std::setw(8) << filter.matches()
        << '\n';
}

//! Handler for debugfilter filter
static command_result listFilters(color_ostream& out,
        std::vector<std::string>& parameters)
{
    if (1u < parameters.size()) {
        auto& catMan = DebugManager::getInstance();
        std::lock_guard<std::mutex> lock(catMan.access_mutex_);
        auto iter = parseFilterId(out, parameters[1]);
        if (iter == FilterManager::getInstance().end())
            return CR_WRONG_USAGE;

        auto id = iter->first;
        Filter& filter = iter->second;

        out << std::left
            << std::setw(10) << "ID:" << id << '\n'
            << std::setw(10) << "Enabled:" << (filter.enabled() ? "Yes"_s:"No"_s) << '\n'
            << std::setw(10) << "Persist:" << (filter.persistent() ? "Yes"_s:"No"_s) << '\n'
            << std::setw(10) << "Level:" <<  levelNames[filter.level()] << '\n'
            << std::setw(10) << "category:" << filter.categoryText() << '\n'
            << std::setw(10) << "plugin:" << filter.pluginText() << '\n'
            << std::setw(10) << "matches:" << filter.matches() << '\n'
            << std::right
            << std::endl;
        return CR_OK;
    }
    auto& catMan = DebugManager::getInstance();
    {
        std::lock_guard<std::mutex> lock(catMan.access_mutex_);
        auto& filMan = FilterManager::getInstance();
        unsigned line = 0;
        for (auto& filterPair: filMan) {
            size_t id = filterPair.first;
            const Filter& filter = filterPair.second;

            color_value c = (line & 1) == 0 ? COLOR_CYAN : COLOR_LIGHTCYAN;
            printFilterListEntry(out,line++,c,id,filter);
        }
    }
    out.color(COLOR_RESET);
    out.flush();
    return CR_OK;
}

static const std::string persistent("persistent");

//! Handler for debugfilter set
static command_result setFilter(color_ostream& out,
        std::vector<std::string>& parameters)
{
    bool persist = false;
    size_t pos = 1u;
    if (pos < parameters.size() &&
            parameters[pos] == persistent) {
        pos++;
        persist = true;
    }
    if (pos >= parameters.size()) {
        ERR(command,out).print("set requires at least the level parameter\n");
        return CR_WRONG_USAGE;
    }
    const std::string& level = parameters[pos];
    auto iter = std::find_if(levelNames.begin(), levelNames.end(),
            [&level](const std::string& v) -> bool {
                // This is std::equal but it is c++14 feature
                auto iter2 = level.begin();
                for (auto iter1 = v.begin(); iter1 != v.end(); ++iter1, ++iter2)
                    if (std::tolower(*iter1) != std::tolower(*iter2)) return false;
                // null terminate strings compare nulls as last character
                return true;
            });
    if (iter == levelNames.end()) {
        ERR(command,out).print("level ('%s') parameter must be one of "
                "trace, debug, info, warning, error.\n",
                parameters[pos].c_str());
        return CR_WRONG_USAGE;
    }

    DebugCategory::level filterLevel = static_cast<DebugCategory::level>(
            iter - levelNames.begin());

    unsigned line = 0;
    Filter* newFilter = nullptr;
    return applyCategoryFilters(out, parameters, pos + 1,
            [&parameters, pos, filterLevel,persist,&newFilter](DebugManager& catMan, const std::regex& catRegex, const std::regex& pluginRegex)
            {
                auto& filMan = FilterManager::getInstance();
                newFilter = &filMan.emplaceNew(filterLevel,
                        pos+1 < parameters.size()?parameters[pos+1]:".",
                        catRegex,
                        pos+2 < parameters.size()?parameters[pos+2]:".",
                        pluginRegex,
                        persist).first->second;
                return filMan.blockSlot(catMan.categorySignal);
            },
            [filterLevel, &line, &out, &newFilter](DebugCategory& cat) {
                auto old = cat.allowed();
                cat.allowed(filterLevel);
                newFilter->addMatch();
                printCategoryListEntry(out, line, cat, old);
            },
            [&out,&persist]() {
                if (persist)
                    return FilterManager::getInstance().saveConfig(out);
                return CR_OK;
            });
}

template<typename HighlightRed,typename ListComplete>
static command_result applyFilterIds(color_ostream& out,
        std::vector<std::string>& parameters,
        const char* name,
        HighlightRed hlRed,
        ListComplete listComplete)
{
    if (1u >= parameters.size()) {
        ERR(command,out) << name << " requires at least a filter id" << std::endl;
        return CR_WRONG_USAGE;
    }
    command_result rv = CR_OK;
    {
        auto& catMan = DebugManager::getInstance();
        std::lock_guard<std::mutex> lock(catMan.access_mutex_);
        auto& filMan = FilterManager::getInstance();
        unsigned line = 0;
        for (size_t pos = 1; pos < parameters.size(); ++pos) {
            const std::string& p = parameters[pos];
            auto iter = parseFilterId(out, p);
            if (iter == filMan.end())
                continue;
            color_value c = (line & 1) == 0 ? COLOR_CYAN : COLOR_LIGHTCYAN;
            if (hlRed(iter))
                c = COLOR_RED;
            printFilterListEntry(out,line++,c,iter->first,iter->second);
        }
        rv = listComplete();
    }
    out.color(COLOR_RESET);
    out.flush();
    return rv;
}

//! Handler for debugfilter disable
static command_result disableFilter(color_ostream& out,
        std::vector<std::string>& parameters)
{
    std::set<DebugCategory*> modified;
    bool mustSave = false;
    return applyFilterIds(out,parameters,"disable",
            [&modified,&mustSave](FilterManager::iterator& iter) -> bool {
                Filter& filter = iter->second;
                bool enabled = filter.enabled();
                if (enabled == false)
                    return true;
                auto& catMan = DebugManager::getInstance();
                for (DebugCategory* cat: catMan) {
                    if (filter.remove(*cat))
                        modified.emplace(cat);
                }
                filter.enabled(false);
                mustSave = mustSave || filter.persistent();
                return false;
            },
            [&modified,&mustSave,&out]() {
                for (DebugCategory* cat: modified) {
                    // Reset filtering back to default
                    cat->allowed(DebugCategory::LWARNING);
                    auto& filMan = FilterManager::getInstance();
                    // Reapply all remaining filters
                    for (auto& filterPair: filMan)
                        filterPair.second.applyAgain(*cat);
                }
                if (mustSave)
                    return FilterManager::getInstance().saveConfig(out);
                return CR_OK;
            });
}

//! Handler for debugfilter enable
static command_result enableFilter(color_ostream& out,
        std::vector<std::string>& parameters)
{
    std::set<DebugCategory*> modified;
    bool mustSave = false;
    return applyFilterIds(out,parameters,"enable",
            [&modified,&mustSave](FilterManager::iterator& iter) -> bool {
                Filter& filter = iter->second;
                bool enabled = filter.enabled();
                if (enabled == true)
                    return true;
                filter.enabled(true);
                auto& catMan = DebugManager::getInstance();
                for (DebugCategory* cat: catMan) {
                    if (filter.applyAgain(*cat)) {
                        modified.emplace(cat);
                        filter.addMatch();
                    }
                }
                mustSave = mustSave || filter.persistent();
                return false;
            },
            [&modified,&mustSave,&out]() {
                for (DebugCategory* cat: modified) {
                    // Reset filtering back to default
                    cat->allowed(DebugCategory::LWARNING);
                    auto& filMan = FilterManager::getInstance();
                    // Reapply all remaining filters
                    for (auto& filterPair: filMan)
                        filterPair.second.applyAgain(*cat);
                }
                if (mustSave)
                    return FilterManager::getInstance().saveConfig(out);
                return CR_OK;
            });
}

//! Handler for debugfilter unset
static command_result unsetFilter(color_ostream& out,
        std::vector<std::string>& parameters)
{
    std::set<DebugCategory*> modified;
    std::vector<FilterManager::iterator> toErase;
    return applyFilterIds(out,parameters,"unset",
            [&modified, &toErase](FilterManager::iterator& iter) -> bool {
                Filter& filter = iter->second;
                if (filter.enabled()) {
                    auto& catMan = DebugManager::getInstance();
                    for (DebugCategory* cat: catMan) {
                        if (filter.remove(*cat))
                            modified.emplace(cat);
                    }
                }
                toErase.emplace_back(iter);
                return false;
            },
            [&modified,&toErase,&out]() {
                auto& filMan = FilterManager::getInstance();
                bool mustSave = false;
                for (auto iter: toErase) {
                    mustSave = mustSave || iter->second.persistent();
                    filMan.erase(iter);
                }

                for (DebugCategory* cat: modified) {
                    // Reset filtering back to default
                    cat->allowed(DebugCategory::LWARNING);
                    // Reapply all remaining filters
                    for (auto& filterPair: filMan)
                        filterPair.second.applyAgain(*cat);
                }
                if (mustSave)
                    return FilterManager::getInstance().saveConfig(out);
                return CR_OK;
            });
}

//! Handler for debugfilter ui
static command_result showUI(color_ostream& out,
        std::vector<std::string>&)
{
    ERR(command, out) << "Interactive UI is not yet implemented" << std::endl;
    return CR_OK;
}

using DFHack::debugPlugin::CommandDispatch;

static command_result printHelp(color_ostream& out,
        std::vector<std::string>& parameters)
{
    const char* help = commandHelp;
    auto iter = CommandDispatch::dispatch.end();
    if (1u < parameters.size())
        iter = CommandDispatch::dispatch.find(parameters[1]);
    if (iter != CommandDispatch::dispatch.end())
        help = iter->second.help();
    out.print(help);
    return CR_OK;
}

CommandDispatch::dispatch_t CommandDispatch::dispatch {
    {"category", {listCategories,commandCategory}},
    {"filter", {listFilters,commandFilters}},
    {"set", {setFilter,commandSet}},
    {"unset", {unsetFilter,commandUnset}},
    {"enable", {enableFilter,commandEnable}},
    {"disable", {disableFilter,commandDisable}},
    {"ui", {showUI,commandUI}},
    {"help", {printHelp,commandHelpDetails}},
};

//! Dispatch command handling to the subcommand or help
static command_result commandDebugFilter(color_ostream& out,
        std::vector<std::string>& parameters)
{
    DEBUG(command,out).print("debugfilter %s, parameter count %zu\n",
            parameters.size() > 0 ? parameters[0].c_str() : "",
            parameters.size());
    auto handler = printHelp;
    auto iter = CommandDispatch::dispatch.end();
    if (0u < parameters.size())
        iter = CommandDispatch::dispatch.find(parameters[0]);
    if (iter != CommandDispatch::dispatch.end())
        handler = iter->second.handler();
    return (handler)(out, parameters);
}

} } /* namespace debug */

DFhackCExport DFHack::command_result plugin_init(DFHack::color_ostream& out,
        std::vector<DFHack::PluginCommand>& commands)
{
    commands.emplace_back(
            "debugfilter",
            "Manage runtime debug print filters",
            DFHack::debugPlugin::commandDebugFilter,
            false,
            DFHack::debugPlugin::commandHelp);
    auto& filMan = DFHack::debugPlugin::FilterManager::getInstance();
    DFHack::command_result rv = DFHack::CR_OK;
    if ((rv = filMan.loadConfig(out)) != DFHack::CR_OK)
        return rv;
    auto& catMan = DFHack::DebugManager::getInstance();
    std::lock_guard<std::mutex> lock(catMan.access_mutex_);
    for (auto* cat: catMan) {
        for (auto& filterPair: filMan) {
            DFHack::debugPlugin::Filter& filter = filterPair.second;
            filter.apply(*cat);
        }
    }
    INFO(init,out).print("plugin_init with %zu commands, %zu filters and %zu categories\n",
            commands.size(), filMan.size(), catMan.size());
    filMan.connectTo(catMan.categorySignal);
    return rv;
}

DFhackCExport DFHack::command_result plugin_shutdown(DFHack::color_ostream& out)
{
    INFO(init,out).print("plugin_shutdown\n");
    return DFHack::CR_OK;
}
