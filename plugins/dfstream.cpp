#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/graphic.h"
#include "df/enabler.h"
#include "df/renderer.h"

#include <vector>
#include <string>
#include "PassiveSocket.h"
#include "tinythread.h"

using namespace DFHack;
using namespace df::enums;

using std::string;
using std::vector;

DFHACK_PLUGIN("dfstream");
REQUIRE_GLOBAL(gps);
REQUIRE_GLOBAL(enabler);

// Owns the thread that accepts TCP connections and forwards messages to clients;
// has a mutex
class client_pool {
    typedef tthread::mutex mutex;

    mutex clients_lock;
    std::vector<CActiveSocket *> clients;

    // TODO - delete this at some point
    tthread::thread * accepter;

    static void accept_clients(void * client_pool_pointer) {
        client_pool * p = reinterpret_cast<client_pool *>(client_pool_pointer);
        CPassiveSocket socket;
        socket.Initialize();
        if (socket.Listen((const uint8_t *)"0.0.0.0", 8008)) {
            std::cout << "Listening on a socket" << std::endl;
        } else {
            std::cout << "Not listening: " << socket.GetSocketError() << std::endl;
            std::cout << socket.DescribeError() << std::endl;
        }
        while (true) {
            CActiveSocket * client = socket.Accept();
            if (client != 0) {
                lock l(*p);
                p->clients.push_back(client);
            }
        }
    }

public:
    class lock {
        tthread::lock_guard<mutex> l;
    public:
        lock(client_pool & p)
            : l(p.clients_lock)
        {
        }
    };
    friend class client_pool::lock;

    client_pool() {
        accepter = new tthread::thread(accept_clients, this);
    }

    // MUST have lock
    bool has_clients() {
        return !clients.empty();
    }

    // MUST have lock
    void add_client(CActiveSocket * sock) {
        clients.push_back(sock);
    }

    // MUST have lock
    void broadcast(const std::string & message) {
        unsigned int sz = htonl(message.size());
        for (size_t i = 0; i < clients.size(); ++i) {
            clients[i]->Send(reinterpret_cast<const uint8_t *>(&sz), sizeof(sz));
            clients[i]->Send((const uint8_t *) message.c_str(), message.size());
        }
    }
};

// A decorator (in the design pattern sense) of the DF renderer class.
// Sends the screen contents to a client_pool.
class renderer_decorator : public df::renderer {
    // the renderer we're decorating
    df::renderer * inner;

    // how many frames have passed since we last sent a frame
    int framesNotPrinted;

    // set to false in the destructor
    bool * alive;

    // clients to which we send the frame
    client_pool clients;

    // The following three methods facilitate copying of state to the inner object
    void set_to_null() {
        screen = NULL;
        screentexpos = NULL;
        screentexpos_addcolor = NULL;
        screentexpos_grayscale = NULL;
        screentexpos_cf = NULL;
        screentexpos_cbr = NULL;
        screen_old = NULL;
        screentexpos_old = NULL;
        screentexpos_addcolor_old = NULL;
        screentexpos_grayscale_old = NULL;
        screentexpos_cf_old = NULL;
        screentexpos_cbr_old = NULL;
    }

    void copy_from_inner() {
        screen = inner->screen;
        screentexpos = inner->screentexpos;
        screentexpos_addcolor = inner->screentexpos_addcolor;
        screentexpos_grayscale = inner->screentexpos_grayscale;
        screentexpos_cf = inner->screentexpos_cf;
        screentexpos_cbr = inner->screentexpos_cbr;
        screen_old = inner->screen_old;
        screentexpos_old = inner->screentexpos_old;
        screentexpos_addcolor_old = inner->screentexpos_addcolor_old;
        screentexpos_grayscale_old = inner->screentexpos_grayscale_old;
        screentexpos_cf_old = inner->screentexpos_cf_old;
        screentexpos_cbr_old = inner->screentexpos_cbr_old;
    }

    void copy_to_inner() {
        inner->screen = screen;
        inner->screentexpos = screentexpos;
        inner->screentexpos_addcolor = screentexpos_addcolor;
        inner->screentexpos_grayscale = screentexpos_grayscale;
        inner->screentexpos_cf = screentexpos_cf;
        inner->screentexpos_cbr = screentexpos_cbr;
        inner->screen_old = screen_old;
        inner->screentexpos_old = screentexpos_old;
        inner->screentexpos_addcolor_old = screentexpos_addcolor_old;
        inner->screentexpos_grayscale_old = screentexpos_grayscale_old;
        inner->screentexpos_cf_old = screentexpos_cf_old;
        inner->screentexpos_cbr_old = screentexpos_cbr_old;
    }

public:
    renderer_decorator(df::renderer * inner, bool * alive)
        : inner(inner)
        , framesNotPrinted(0)
        , alive(alive)
    {
        copy_from_inner();
    }
    virtual void update_tile(int x, int y) {
        copy_to_inner();
        inner->update_tile(x, y);
    }
    virtual void update_all() {
        copy_to_inner();
        inner->update_all();
    }
    virtual void render() {
        copy_to_inner();
        inner->render();

        ++framesNotPrinted;
        int gfps = enabler->calculated_gfps;
        if (gfps == 0) gfps = 1;
        // send a frame roughly every 128 mibiseconds (1 second = 1024 mibiseconds)
        if ((framesNotPrinted * 1024) / gfps <= 128) return;

        client_pool::lock lock(clients);
        if (!clients.has_clients()) return;
        framesNotPrinted = 0;
        std::stringstream frame;
        frame << gps->dimx << ' ' << gps->dimy << " 0 0 " << gps->dimx << ' ' << gps->dimy << '\n';
        unsigned char * sc_ = gps->screen;
        for (int y = 0; y < gps->dimy; ++y) {
            unsigned char * sc = sc_;
            for (int x = 0; x < gps->dimx; ++x) {
                unsigned char ch   = sc[0];
                unsigned char bold = (sc[3] != 0) * 8;
                unsigned char translate[] =
                { 0, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15 };
                unsigned char fg   = translate[(sc[1] + bold) % 16];
                unsigned char bg   = translate[sc[2] % 16]*16;
                frame.put(ch);
                frame.put(fg+bg);
                sc += 4*gps->dimy;
            }
            sc_ += 4;
        }
        clients.broadcast(frame.str());
    }
    virtual void set_fullscreen() { inner->set_fullscreen(); }
    virtual void zoom(df::zoom_commands cmd) {
        copy_to_inner();
        inner->zoom(cmd);
    }
    virtual void resize(int w, int h) {
        copy_to_inner();
        inner->resize(w, h);
        copy_from_inner();
    }
    virtual void grid_resize(int w, int h) {
        copy_to_inner();
        inner->grid_resize(w, h);
        copy_from_inner();
    }
    virtual ~renderer_decorator() {
        *alive = false;
        if (inner) {
            copy_to_inner();
            delete inner;
            inner = 0;
        }
        set_to_null();
    }
    virtual bool get_mouse_coords(int *x, int *y) { return inner->get_mouse_coords(x, y); }
    virtual bool uses_opengl() { return inner->uses_opengl(); }

    static renderer_decorator * hook(df::renderer *& ptr, bool * alive) {
        renderer_decorator * r = new renderer_decorator(ptr, alive);
        ptr = r;
        return r;
    }

    static void unhook(df::renderer *& ptr, renderer_decorator * dec, color_ostream & out) {
        dec->copy_to_inner();
        ptr = dec->inner;
        dec->inner = 0;
        delete dec;
    }
};

inline df::renderer *& active_renderer() {
    return enabler->renderer;
}

// This class is a smart pointer around a renderer_decorator.
// It should only be assigned r_d pointers that use the alive-pointer of this
// instance.
// If the r_d has been deleted by an external force, this smart pointer doesn't
// redelete it.
class auto_renderer_decorator {
    renderer_decorator * p;
public:
    // pass this member to the ctor of renderer_decorator
    bool alive;

    auto_renderer_decorator()
        : p(0)
    {
    }

    ~auto_renderer_decorator() {
        reset();
    }

    void reset() {
        if (*this) {
            delete p;
            p = 0;
        }
    }

    operator bool() {
        return (p != 0) && alive;
    }

    auto_renderer_decorator & operator=(renderer_decorator *p) {
        reset();
        this->p = p;
        return *this;
    }

    renderer_decorator * get() {
        return p;
    }

    renderer_decorator * operator->() {
        return get();
    }
};

auto_renderer_decorator decorator;

DFhackCExport command_result plugin_init ( color_ostream &out, vector <PluginCommand> &commands)
{
    if (!df::renderer::_identity.can_instantiate())
    {
        out.printerr("Cannot allocate a renderer\n");
        return CR_OK;
    }
    if (!decorator) {
        decorator = renderer_decorator::hook(active_renderer(), &decorator.alive);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if (decorator && active_renderer() == decorator.get())
    {
        renderer_decorator::unhook(active_renderer(), decorator.get(), out);
    }
    decorator.reset();
    return CR_OK;
}
// vim:set sw=4 sts=4 et:
