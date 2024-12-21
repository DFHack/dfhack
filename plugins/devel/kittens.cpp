#include <array>
#include <atomic>
#include <vector>
#include <random>
#include <string>
#include <thread>

#include "Console.h"
#include "Core.h"
#include "Debug.h"
#include "Export.h"
#include "MiscUtils.h"
#include "PluginManager.h"
#include "Signal.hpp"

#include "modules/Gui.h"
#include "modules/Items.h"
#include "modules/Maps.h"

#include "df/caste_raw.h"
#include "df/creature_raw.h"
#include "df/plotinfost.h"
#include "df/world.h"

// for MSVC alignas(64) issues
#ifdef WIN32
#define _DISABLE_EXTENDED_ALIGNED_STORAGE
#endif

using std::vector;
using std::string;
using std::endl;
using namespace DFHack;

DFHACK_PLUGIN("kittens");
DFHACK_PLUGIN_IS_ENABLED(is_enabled);
REQUIRE_GLOBAL(plotinfo);
REQUIRE_GLOBAL(world);

namespace DFHack {
DBG_DECLARE(kittens,command);
}

std::atomic<bool> shutdown_flag{false};
std::atomic<bool> final_flag{true};
std::atomic<bool> timering{false};
std::atomic<bool> trackmenu_flg{false};
std::atomic<uint8_t> trackpos_flg{0};
std::atomic<uint8_t> statetrack{0};
int32_t last_designation[3] = {-30000, -30000, -30000};
int32_t last_mouse[2] = {-1, -1};
df::ui_sidebar_mode last_menu = df::ui_sidebar_mode::Default;
uint64_t timeLast = 0;

command_result kittens (color_ostream &out, vector <string> & parameters);
command_result ktimer (color_ostream &out, vector <string> & parameters);
command_result trackmenu (color_ostream &out, vector <string> & parameters);
command_result trackpos (color_ostream &out, vector <string> & parameters);
command_result trackstate (color_ostream &out, vector <string> & parameters);
command_result colormods (color_ostream &out, vector <string> & parameters);
command_result sharedsignal (color_ostream &out, vector <string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand("nyan","NYAN CAT INVASION!",kittens));
    commands.push_back(PluginCommand("ktimer","Measure time between game updates and console lag.",ktimer));
    commands.push_back(PluginCommand("trackmenu","Track menu ID changes (toggle).",trackmenu));
    commands.push_back(PluginCommand("trackpos","Track mouse and designation coords (toggle).",trackpos));
    commands.push_back(PluginCommand("trackstate","Track world and map state (toggle).",trackstate));
    commands.push_back(PluginCommand("colormods","Dump colormod vectors.",colormods));
    commands.push_back(PluginCommand("sharedsignal","Test Signal with signal_shared_tag",sharedsignal));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    shutdown_flag = true;
    while(!final_flag)
    {
        Core::getInstance().getConsole().msleep(60);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    if(!statetrack)
        return CR_OK;
    switch (event) {
        case SC_MAP_LOADED:
            out << "Map loaded" << endl;
            break;
        case SC_MAP_UNLOADED:
            out << "Map unloaded" << endl;
            break;
        case SC_WORLD_LOADED:
            out << "World loaded" << endl;
            break;
        case SC_WORLD_UNLOADED:
            out << "World unloaded" << endl;
            break;
        case SC_VIEWSCREEN_CHANGED:
            out << "Screen changed" << endl;
            break;
        default:
            out << "Something else is happening, nobody knows what..." << endl;
            break;
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if(timering)
    {
        uint64_t time2 = GetTimeMs64();
        uint64_t delta = time2-timeLast;
        timeLast = time2;
        out.print("Time delta = %d ms\n", int(delta));
    }
    if(trackmenu_flg)
    {
        if (last_menu != plotinfo->main.mode)
        {
            last_menu = plotinfo->main.mode;
            out.print("Menu: %d\n",last_menu);
        }
    }
    if(trackpos_flg)
    {
        int32_t desig_x, desig_y, desig_z;
        Gui::getDesignationCoords(desig_x,desig_y,desig_z);
        if(desig_x != last_designation[0] || desig_y != last_designation[1] || desig_z != last_designation[2])
        {
            last_designation[0] = desig_x;
            last_designation[1] = desig_y;
            last_designation[2] = desig_z;
            out.print("Designation: %d %d %d\n",desig_x, desig_y, desig_z);
        }
        df::coord mousePos = Gui::getMousePos();
        if(mousePos.x != last_mouse[0] || mousePos.y != last_mouse[1])
        {
            last_mouse[0] = mousePos.x;
            last_mouse[1] = mousePos.y;
            out.print("Mouse: %d %d\n",mousePos.x, mousePos.y);
        }
    }
    return CR_OK;
}

command_result trackmenu (color_ostream &out, vector <string> & parameters)
{
    bool is_running = trackmenu_flg.exchange(false);
    if(is_running)
    {
        return CR_OK;
    }
    else
    {
        is_enabled = true;
        last_menu = plotinfo->main.mode;
        out.print("Menu: %d\n",last_menu);
        trackmenu_flg = true;
        return CR_OK;
    }
}
command_result trackpos (color_ostream &out, vector <string> & parameters)
{
    trackpos_flg.fetch_xor(1);
    is_enabled = true;
    return CR_OK;
}

command_result trackstate ( color_ostream& out, vector< string >& parameters )
{
    statetrack.fetch_xor(1);
    return CR_OK;
}

command_result colormods (color_ostream &out, vector <string> & parameters)
{
    auto & vec = world->raws.creatures.alphabetic;
    for(df::creature_raw* rawlion : vec)
    {
        df::caste_raw * caste = rawlion->caste[0];
        out.print("%s\nCaste addr %p\n",rawlion->creature_id.c_str(), &caste->color_modifiers);
        for(size_t j = 0; j < caste->color_modifiers.size();j++)
        {
            out.print("mod %zd: %p\n", j, caste->color_modifiers[j]);
        }
    }
    return CR_OK;
}

command_result ktimer (color_ostream &out, vector <string> & parameters)
{
    bool is_running = timering.exchange(false);
    if(is_running)
    {
        return CR_OK;
    }
    uint64_t timestart = GetTimeMs64();
    {
        uint64_t timeend = GetTimeMs64();
        timeLast = timeend;
        timering = true;
        out.print("Time to suspend = %d ms\n", int(timeend - timestart));
    }
    is_enabled = true;
    return CR_OK;
}

struct Connected;
using shared = std::shared_ptr<Connected>;
using weak = std::weak_ptr<Connected>;

static constexpr std::chrono::microseconds delay{1};

template<typename Derived>
struct ClearMem : public ConnectedBase {
    ~ClearMem()
    {
        memset(reinterpret_cast<void*>(this), 0xDE, sizeof(Derived));
    }
};

struct Connected : public ClearMem<Connected> {
    using Sig = Signal<void(uint32_t), signal_shared_tag>;
    std::array<Sig::Connection,4> con;
    Sig signal;
    weak other;
    Sig::weak_ptr other_sig;
    color_ostream *out;
    int id;
    uint32_t count;
    uint32_t caller;
    alignas(64) std::atomic<uint32_t> callee;
    Connected() = default;
    Connected(int id) :
        Connected{}
    {
        this->id = id;
    }
    void connect(color_ostream& o, shared& b, size_t pos, uint32_t c)
    {
        out = &o;
        count = c*2;
        other = b;
        other_sig = b->signal.weak_from_this();
        // Externally synchronized object destruction is only safe to this
        // connect.
        con[pos] = b->signal.connect(
                [this](uint32_t) {
                    uint32_t old = callee.fetch_add(1);
                    assert(old != 0xDEDEDEDE); (void)old;
                    std::this_thread::sleep_for(delay);
                    assert(callee != 0xDEDEDEDE);
                });
        // Shared object managed object with possibility of destruction while
        // other threads calling emit must pass the shared_ptr to connect.
        Connected *bptr = b.get();
        b->con[pos] = signal.connect(b,
                [bptr](int) {
                    uint32_t old = bptr->callee.fetch_add(1);
                    assert(old != 0xDEDEDEDE); (void)old;
                    std::this_thread::sleep_for(delay);
                    assert(bptr->callee != 0xDEDEDEDE);
                });
    }
    void reconnect(size_t pos) {
        auto b = other.lock();
        if (!b)
            return;
        // Not required to use Sig::lock because other holds strong reference to
        // Signal. But this just shows how weak_ref could be used.
        auto sig = Sig::lock(other_sig);
        if (!sig)
            return;
        con[pos] = sig->connect(b,
                [this](uint32_t) {
                    uint32_t old = callee.fetch_add(1);
                    assert(old != 0xDEDEDEDE); (void)old;
                    std::this_thread::sleep_for(delay);
                    assert(callee != 0xDEDEDEDE);
                });
    }
    void connect(color_ostream& o, shared& a, shared& b,size_t pos, uint32_t c)
    {
        out = &o;
        count = c;
        con[pos] = b->signal.connect(a,
                [this](uint32_t) {
                    uint32_t old = callee.fetch_add(1);
                    assert(old != 0xDEDEDEDE); (void)old;
                    std::this_thread::sleep_for(delay);
                    assert(callee != 0xDEDEDEDE);
                });
    }
    Connected* operator->() noexcept
    {
        return this;
    }
    ~Connected() {
        INFO(command,*out).print("Connected %d had %d count. "
                "It was caller %d times. "
                "It was callee %d times.\n",
                id, count, caller, callee.load());
    }
};

command_result sharedsignal (color_ostream &out, vector <string> & parameters)
{
    using rng_t = std::linear_congruential_engine<uint32_t, 747796405U, 2891336453U, 0>;
    rng_t rng(std::random_device{}());
    size_t count = 10;
    if (0 < parameters.size()) {
        std::stringstream ss(parameters[0]);
        ss >> count;
        DEBUG(command, out) << "Parsed " << count
            << " from paramters[0] '" << parameters[0] << '\'' << std::endl;
    }


    std::uniform_int_distribution<uint32_t>  dis(4096,8192);
    out << "Running signal_shared_tag destruction test "
        << count << " times" << std::endl;
    for (size_t nr = 0; nr < count; ++nr) {
        std::array<std::thread,4> t{};
        // Make an object which destruction is protected by std::thread::join()
        Connected external{static_cast<int>(t.size())};
        TRACE(command, out) << "begin " << std::endl;
        {
            int id = 0;
            // Make objects that are automatically protected using weak_ptr
            // references that are promoted to shared_ptr when Signal is
            // accessed.
            std::array<shared,4> c = {
                std::make_shared<Connected>(id++),
                std::make_shared<Connected>(id++),
                std::make_shared<Connected>(id++),
                std::make_shared<Connected>(id++),
            };
            assert(t.size() == c.size());
            for (unsigned i = 1; i < c.size(); ++i) {
                c[0]->connect(out, c[0], c[i], i - 1, dis(rng));
                c[i]->connect(out, c[i], c[0], 0, dis(rng));
            }
            external.connect(out, c[1], 1, dis(rng));
            auto thr = [&out](shared c) {
                TRACE(command, out) << "Thread " << c->id << " started." << std::endl;
                weak ref = c;
                for (;c->caller < c->count; ++c->caller) {
                    c->signal(std::move(c->caller));
                }
                TRACE(command, out) << "Thread " << c->id << " resets shared." << std::endl;
                c.reset();
                while((c = ref.lock())) {
                    ++c->caller;
                    c->signal(std::move(c->caller));
                    c.reset();
                    std::this_thread::sleep_for(delay*25);
                }
            };
            for (unsigned i = 0; i < c.size(); ++i) {
                TRACE(command, out) << "start thread " << i << std::endl;
                t[i] = std::thread{thr, c[i]};
            }
        }
        TRACE(command, out) << "running " << std::endl;
        for (;external->caller < external->count; ++external->caller) {
            external->signal(std::move(external->caller));
            external->reconnect(1);
        }
        TRACE(command, out) << "join " << std::endl;
        for (unsigned i = 0; i < t.size(); ++i)
            t[i].join();
    }
    return CR_OK;
}

command_result kittens (color_ostream &out, vector <string> & parameters)
{
    if (parameters.size() >= 1)
    {
        if (parameters[0] == "stop")
        {
            shutdown_flag = true;
            while(!final_flag)
            {
                Core::getInstance().getConsole().msleep(60);
            }
            shutdown_flag = false;
            return CR_OK;
        }
    }
    final_flag = false;
    if (!out.is_console())
        return CR_FAILURE;
    Console &con = static_cast<Console&>(out);
    // http://evilzone.org/creative-arts/nyan-cat-ascii/
    const char * nyan []=
    {
        "NYAN NYAN NYAN NYAN NYAN NYAN NYAN",
        "+      o     +              o   ",
        "    +             o     +       +",
        "o          +",
        "    o  +           +        +",
        "+        o     o       +        o",
        "-_-_-_-_-_-_-_,------,      o ",
        "_-_-_-_-_-_-_-|   /\\_/\\  ",
        "-_-_-_-_-_-_-~|__( ^ .^)  +     +  ",
        "_-_-_-_-_-_-_-\"\"  \"\"      ",
        "+      o         o   +       o",
        "    +         +",
        "o        o         o      o     +",
        "    o           +",
        "+      +     o        o      +    ",
        "NYAN NYAN NYAN NYAN NYAN NYAN NYAN",
        0
    };
    const char VARIABLE_IS_NOT_USED * kittenz1 []=
    {
        "   ____",
        "  (.   \\",
        "    \\  |  ",
        "     \\ |___(\\--/)",
        "   __/    (  . . )",
        "  \"'._.    '-.O.'",
        "       '-.  \\ \"|\\",
        "          '.,,/'.,,mrf",
        0
    };
    con.cursor(false);
    con.clear();
    Console::color_value color = COLOR_BLUE;
    while(1)
    {
        if(shutdown_flag || !con.isInited())
        {
            final_flag = true;
            con.reset_color();
            con << std::endl << "NYAN!" << std::endl << std::flush;
            return CR_OK;
        }
        con.color(color);
        int index = 0;
        const char * kit = nyan[index];
        con.gotoxy(1,1);
        //con << "Your DF is now full of kittens!" << std::endl;
        while (kit != 0)
        {
            con.gotoxy(1,1+index);
            con << kit << std::endl;
            index++;
            kit = nyan[index];
        }
        con.flush();
        con.msleep(60);
        color = Console::color_value(int(color) + 1);
        if(color > COLOR_MAX)
            color = COLOR_BLUE;
    }
}
