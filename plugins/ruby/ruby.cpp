// blindly copied imports from fastdwarf
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "VersionInfo.h"
#include "MemAccess.h"

#include "DataDefs.h"
#include "df/global_objects.h"

#include "tinythread.h"

using namespace DFHack;



// DFHack stuff


static int df_loadruby(void);
static void df_unloadruby(void);
static void df_rubythread(void*);
static command_result df_rubyeval(color_ostream &out, std::vector <std::string> & parameters);
static void ruby_bind_dfhack(void);

// inter-thread communication stuff
enum RB_command {
    RB_IDLE,
    RB_INIT,
    RB_DIE,
    RB_EVAL,
};
tthread::mutex *m_irun;
tthread::mutex *m_mutex;
static volatile RB_command r_type;
static volatile command_result r_result;
static color_ostream *r_console;       // color_ostream given as argument, if NULL resort to console_proxy
static const char *r_command;
static tthread::thread *r_thread;
static int onupdate_active;
static int onupdate_minyear, onupdate_minyeartick=-1, onupdate_minyeartickadv=-1;
static color_ostream_proxy *console_proxy;
static std::vector<std::string> *dfhack_run_queue;


DFHACK_PLUGIN("ruby")


DFhackDataExport bool plugin_is_enabled = true;

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    plugin_is_enabled = enable;
    return CR_OK;
}


DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    onupdate_active = 0;

    // fail silently instead of spamming the console with 'failed to initialize'
    // if libruby is not present, the error is still logged in stderr.log
    if (!df_loadruby())
        return CR_OK;

    // the ruby thread sleeps trying to lock this
    // when it gets it, it runs according to r_type
    // when finished, it sets r_type to IDLE and unlocks
    m_irun = new tthread::mutex();

    // when any thread is going to request something to the ruby thread,
    // lock this before anything, and release when everything is done
    m_mutex = new tthread::mutex();

    // list of dfhack commands to run when the current ruby run is done (once locks are released)
    dfhack_run_queue = new std::vector<std::string>;

    r_type = RB_INIT;

    // create the dedicated ruby thread
    // df_rubythread starts the ruby interpreter and goes to type=IDLE when done
    r_thread = new tthread::thread(df_rubythread, 0);

    // wait until init phase 1 is done
    while (r_type != RB_IDLE)
        tthread::this_thread::yield();

    // ensure the ruby thread sleeps until we have a command to handle
    m_irun->lock();

    // check return value from rbinit
    if (r_result == CR_FAILURE)
        return CR_FAILURE;

    commands.push_back(PluginCommand("rb_eval",
                "Ruby interpreter. Eval() a ruby string.",
                df_rubyeval));

    commands.push_back(PluginCommand("rb",
                "Ruby interpreter. Eval() a ruby string (alias for rb_eval).",
                df_rubyeval));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    // if dlopen failed
    if (!r_thread)
        return CR_OK;

    // ensure ruby thread is idle
    m_mutex->lock();

    r_type = RB_DIE;
    r_command = NULL;
    // start ruby thread
    m_irun->unlock();

    // wait until ruby thread ends after RB_DIE
    r_thread->join();

    // cleanup everything
    delete r_thread;
    r_thread = 0;
    delete m_irun;
    // we can release m_mutex, other users will check r_thread
    m_mutex->unlock();
    delete m_mutex;
    delete dfhack_run_queue;

    // dlclose libruby
    df_unloadruby();

    return CR_OK;
}

static command_result do_plugin_eval_ruby(color_ostream &out, const char *command)
{
    command_result ret;

    // ensure ruby thread is idle
    m_mutex->lock();
    if (!r_thread)
        // raced with plugin_shutdown
        return CR_OK;

    r_type = RB_EVAL;
    r_command = command;
    r_console = &out;
    // wake ruby thread up
    m_irun->unlock();

    // semi-active loop until ruby thread is done
    while (r_type != RB_IDLE)
        tthread::this_thread::yield();

    ret = r_result;
    r_console = NULL;

    // block ruby thread
    m_irun->lock();
    // let other plugin_eval_ruby run
    m_mutex->unlock();

    return ret;
}

// send a single ruby line to be evaluated by the ruby thread
DFhackCExport command_result plugin_eval_ruby( color_ostream &out, const char *command)
{
    command_result ret;

    // if dlopen failed
    if (!r_thread)
    {
        out.printerr("Failed to load ruby library.\n");
        return CR_FAILURE;
    }

    if (!strncmp(command, "nolock ", 7)) {
        // debug only!
        // run ruby commands without locking the main thread
        // useful when the game is frozen after a segfault
        ret = do_plugin_eval_ruby(out, command+7);
    } else {
        // wrap all ruby code inside a suspend block
        // if we dont do that and rely on ruby code doing it, we'll deadlock in
        // onupdate
        CoreSuspender suspend;
        ret = do_plugin_eval_ruby(out, command);
    }

    // if any dfhack command is queued for run, do it now
    while (!dfhack_run_queue->empty()) {
        std::string cmd = dfhack_run_queue->at(0);
        // delete before running the command, which may be ruby and cause infinite loops
        dfhack_run_queue->erase(dfhack_run_queue->begin());
        Core::getInstance().runCommand(out, cmd);
    }

    return ret;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if (!r_thread)
        return CR_OK;

    // ruby sets this flag when needed, to avoid lag running ruby code every
    // frame if not necessary
    if (!onupdate_active)
        return CR_OK;

    if (df::global::cur_year && (*df::global::cur_year < onupdate_minyear))
        return CR_OK;
    if (df::global::cur_year_tick && onupdate_minyeartick >= 0 &&
            (*df::global::cur_year == onupdate_minyear &&
             *df::global::cur_year_tick < onupdate_minyeartick))
        return CR_OK;
    if (df::global::cur_year_tick_advmode && onupdate_minyeartickadv >= 0 &&
            (*df::global::cur_year == onupdate_minyear &&
             *df::global::cur_year_tick_advmode < onupdate_minyeartickadv))
        return CR_OK;

    return plugin_eval_ruby(out, "DFHack.onupdate");
}

DFhackCExport command_result plugin_onstatechange ( color_ostream &out, state_change_event e)
{
    if (!r_thread)
        return CR_OK;

    std::string cmd = "DFHack.onstatechange ";
    switch (e) {
#define SCASE(s) case SC_ ## s : cmd += ":" # s ; break
        SCASE(WORLD_LOADED);
        SCASE(WORLD_UNLOADED);
        SCASE(MAP_LOADED);
        SCASE(MAP_UNLOADED);
        SCASE(VIEWSCREEN_CHANGED);
        SCASE(CORE_INITIALIZED);
        // if we go through plugin_eval at BEGIN_UNLOAD, it'll
        // try to get the suspend lock and deadlock at df exit
        case SC_BEGIN_UNLOAD : return CR_OK;
        SCASE(PAUSED);
        SCASE(UNPAUSED);
#undef SCASE
    }

    return plugin_eval_ruby(out, cmd.c_str());
}

static command_result df_rubyeval(color_ostream &out, std::vector <std::string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "help" || parameters[0] == "?"))
    {
        out.print("This command executes an arbitrary ruby statement.\n");
        return CR_OK;
    }

    // reconstruct the text from dfhack console line
    std::string full = "";

    for (unsigned i=0 ; i<parameters.size() ; ++i) {
        full += parameters[i];
        if (i != parameters.size()-1)
            full += " ";
    }

    return plugin_eval_ruby(out, full.c_str());
}



// ruby stuff

// - ruby-dev on windows is messy
// - ruby.h with gcc -m32 on linux 64 is broken
// so we dynamically load libruby with dlopen/LoadLibrary
// lib path is hardcoded here, and by default downloaded by cmake
// this code should work with ruby1.9, but ruby1.9 doesn't like running
// in a dedicated non-main thread, so use ruby1.8 binaries only for now

// these ruby definitions are invalid for windows 64bit (need long long)
typedef unsigned long VALUE;
typedef unsigned long ID;

#define Qfalse ((VALUE)0)
#define Qtrue  ((VALUE)2)
#define Qnil   ((VALUE)4)

#define INT2FIX(i) ((VALUE)((((long)i) << 1) | 1))
#define FIX2INT(i) (((long)i) >> 1)
#define RUBY_METHOD_FUNC(func) ((VALUE(*)(...))func)

void (*ruby_init_stack)(VALUE*);
void (*ruby_sysinit)(int *, const char ***);
void (*ruby_init)(void);
void (*ruby_init_loadpath)(void);
void (*ruby_script)(const char*);
void (*ruby_finalize)(void);
ID (*rb_intern)(const char*);
VALUE (*rb_funcall)(VALUE, ID, int, ...);
VALUE (*rb_define_module)(const char*);
void (*rb_define_singleton_method)(VALUE, const char*, VALUE(*)(...), int);
VALUE (*rb_gv_get)(const char*);
VALUE (*rb_str_new)(const char*, long);
char* (*rb_string_value_ptr)(VALUE*);
VALUE (*rb_eval_string_protect)(const char*, int*);
VALUE (*rb_ary_shift)(VALUE);
VALUE (*rb_float_new)(double);
double (*rb_num2dbl)(VALUE);
VALUE (*rb_int2inum)(long);
VALUE (*rb_uint2inum)(unsigned long);
unsigned long (*rb_num2ulong)(VALUE);
// end of rip(ruby.h)

DFHack::DFLibrary *libruby_handle;

// load the ruby library, initialize function pointers
static int df_loadruby(void)
{
    const char *libpath =
#if defined(WIN32)
        "./libruby.dll";
#elif defined(__APPLE__)
        "hack/libruby.dylib";
#else
        "hack/libruby.so";
#endif

    libruby_handle = OpenPlugin(libpath);
    if (!libruby_handle) {
        fprintf(stderr, "Cannot initialize ruby plugin: failed to load %s\n", libpath);
        return 0;
    }

    // ruby_sysinit is optional (ruby1.9 only)
    ruby_sysinit = (decltype(ruby_sysinit))LookupPlugin(libruby_handle, "ruby_sysinit");
#define rbloadsym(s) if (!(s = (decltype(s))LookupPlugin(libruby_handle, #s))) return 0
    rbloadsym(ruby_init_stack);
    rbloadsym(ruby_init);
    rbloadsym(ruby_init_loadpath);
    rbloadsym(ruby_script);
    rbloadsym(ruby_finalize);
    rbloadsym(rb_intern);
    rbloadsym(rb_funcall);
    rbloadsym(rb_define_module);
    rbloadsym(rb_define_singleton_method);
    rbloadsym(rb_gv_get);
    rbloadsym(rb_str_new);
    rbloadsym(rb_string_value_ptr);
    rbloadsym(rb_eval_string_protect);
    rbloadsym(rb_ary_shift);
    rbloadsym(rb_float_new);
    rbloadsym(rb_num2dbl);
    rbloadsym(rb_int2inum);
    rbloadsym(rb_uint2inum);
    rbloadsym(rb_num2ulong);
#undef rbloadsym

    return 1;
}

static void df_unloadruby(void)
{
    if (libruby_handle) {
        ClosePlugin(libruby_handle);
        libruby_handle = 0;
    }
}

static void printerr(const char* fmt, const char *arg)
{
    if (r_console)
        r_console->printerr(fmt, arg);
    else
        Core::printerr(fmt, arg);
}

// ruby thread code
static void dump_rb_error(void)
{
    VALUE s, err;

    err = rb_gv_get("$!");

    s = rb_funcall(err, rb_intern("class"), 0);
    s = rb_funcall(s, rb_intern("name"), 0);
    printerr("E: %s: ", rb_string_value_ptr(&s));

    s = rb_funcall(err, rb_intern("message"), 0);
    printerr("%s\n", rb_string_value_ptr(&s));

    err = rb_funcall(err, rb_intern("backtrace"), 0);
    for (int i=0 ; i<8 ; ++i)
        if ((s = rb_ary_shift(err)) != Qnil)
            printerr(" %s\n", rb_string_value_ptr(&s));
}

// ruby thread main loop
static void df_rubythread(void *p)
{
    int state, running;

    // may need to be run from df main thread?
    VALUE foo;
    ruby_init_stack(&foo);

    if (ruby_sysinit) {
        // ruby1.9 specific API
        static int argc;
        static const char *argv[] = { "dfhack", 0 };
        ruby_sysinit(&argc, (const char ***)&argv);
    }

    // initialize the ruby interpreter
    ruby_init();
    ruby_init_loadpath();
    // default value for the $0 "current script name"
    ruby_script("dfhack");

    // create the ruby objects to map DFHack to ruby methods
    ruby_bind_dfhack();

    console_proxy = new color_ostream_proxy(Core::getInstance().getConsole());

    // ensure noone bothers us while we load data defs in the background
    m_mutex->lock();

    // tell the main thread our initialization is finished
    r_result = CR_OK;
    r_type = RB_IDLE;

    // load the default ruby-level definitions in the background
    state=0;
    rb_eval_string_protect("require './hack/ruby/ruby'", &state);
    if (state)
        dump_rb_error();

    // ready to go
    m_mutex->unlock();

    running = 1;
    while (running) {
        // sleep waiting for new command
        m_irun->lock();

        switch (r_type) {
        case RB_IDLE:
        case RB_INIT:
            break;

        case RB_DIE:
            running = 0;
            ruby_finalize();
            break;

        case RB_EVAL:
            state = 0;
            rb_eval_string_protect(r_command, &state);
            if (state)
                dump_rb_error();
            break;
        }

        r_result = CR_OK;
        r_type = RB_IDLE;
        m_irun->unlock();
        tthread::this_thread::yield();
    }
}


#define BOOL_ISFALSE(v) ((v) == Qfalse || (v) == Qnil || (v) == INT2FIX(0))

// main DFHack ruby module
static VALUE rb_cDFHack;


// DFHack module ruby methods, binds specific dfhack methods

// df-dfhack version (eg "0.34.11-r2")
static VALUE rb_dfversion(VALUE self)
{
    const char *dfhack_version = Version::dfhack_version();
    return rb_str_new(dfhack_version, strlen(dfhack_version));
}

// enable/disable calls to DFHack.onupdate()
static VALUE rb_dfonupdate_active(VALUE self)
{
    if (onupdate_active)
        return Qtrue;
    else
        return Qfalse;
}

static VALUE rb_dfonupdate_active_set(VALUE self, VALUE val)
{
    onupdate_active = (BOOL_ISFALSE(val) ? 0 : 1);
    return Qtrue;
}

static VALUE rb_dfonupdate_minyear(VALUE self)
{
    return rb_uint2inum(onupdate_minyear);
}

static VALUE rb_dfonupdate_minyear_set(VALUE self, VALUE val)
{
    onupdate_minyear = rb_num2ulong(val);
    return Qtrue;
}

static VALUE rb_dfonupdate_minyeartick(VALUE self)
{
    return rb_uint2inum(onupdate_minyeartick);
}

static VALUE rb_dfonupdate_minyeartick_set(VALUE self, VALUE val)
{
    onupdate_minyeartick = rb_num2ulong(val);
    return Qtrue;
}

static VALUE rb_dfonupdate_minyeartickadv(VALUE self)
{
    return rb_uint2inum(onupdate_minyeartickadv);
}

static VALUE rb_dfonupdate_minyeartickadv_set(VALUE self, VALUE val)
{
    onupdate_minyeartickadv = rb_num2ulong(val);
    return Qtrue;
}

static VALUE rb_dfprint_str(VALUE self, VALUE s)
{
    if (r_console)
        r_console->print("%s", rb_string_value_ptr(&s));
    else
        console_proxy->print("%s", rb_string_value_ptr(&s));
    return Qnil;
}

static VALUE rb_dfprint_color(VALUE self, VALUE c, VALUE s)
{
    if (r_console) {
        color_value old_col = r_console->color();
        r_console->color(color_value(rb_num2ulong(c)));
        r_console->print("%s", rb_string_value_ptr(&s));
        r_console->color(old_col);
    } else
        console_proxy->print("%s", rb_string_value_ptr(&s));
    return Qnil;
}

static VALUE rb_dfprint_err(VALUE self, VALUE s)
{
    printerr("%s", rb_string_value_ptr(&s));
    return Qnil;
}

static VALUE rb_dfget_global_address(VALUE self, VALUE name)
{
    return rb_uint2inum(Core::getInstance().vinfo->getAddress(rb_string_value_ptr(&name)));
}

static VALUE rb_dfget_vtable(VALUE self, VALUE name)
{
    return rb_uint2inum((uint32_t)Core::getInstance().vinfo->getVTable(rb_string_value_ptr(&name)));
}

// read the c++ class name from a vtable pointer, inspired from doReadClassName
// XXX virtual classes only! dark pointer arithmetic, use with caution !
static VALUE rb_dfget_rtti_classname(VALUE self, VALUE vptr)
{
    char *ptr = (char*)rb_num2ulong(vptr);
#ifdef WIN32
    char *rtti = *(char**)(ptr - 0x4);
    char *typeinfo = *(char**)(rtti + 0xC);
    // skip the .?AV, trim @@ from end
    return rb_str_new(typeinfo+0xc, strlen(typeinfo+0xc)-2);
#else
    char *typeinfo = *(char**)(ptr - 0x4);
    char *typestring = *(char**)(typeinfo + 0x4);
    while (*typestring >= '0' && *typestring <= '9')
        typestring++;
    return rb_str_new(typestring, strlen(typestring));
#endif
}

static VALUE rb_dfget_vtable_ptr(VALUE self, VALUE objptr)
{
    // actually, rb_dfmemory_read_int32
    return rb_uint2inum(*(uint32_t*)rb_num2ulong(objptr));
}

// run a dfhack command, as if typed from the dfhack console
static VALUE rb_dfhack_run(VALUE self, VALUE cmd)
{
    std::string s;
    int strlen = FIX2INT(rb_funcall(cmd, rb_intern("length"), 0));
    s.assign(rb_string_value_ptr(&cmd), strlen);
    dfhack_run_queue->push_back(s);
    return Qtrue;
}



// raw memory access
// used by the ruby class definitions
// XXX may cause game crash ! double-check your addresses !

static VALUE rb_dfmalloc(VALUE self, VALUE len)
{
    char *ptr = (char*)malloc(FIX2INT(len));
    if (!ptr)
        return Qnil;
    memset(ptr, 0, FIX2INT(len));
    return rb_uint2inum((uint32_t)ptr);
}

static VALUE rb_dffree(VALUE self, VALUE ptr)
{
    free((void*)rb_num2ulong(ptr));
    return Qtrue;
}

// memory reading (buffer)
static VALUE rb_dfmemory_read(VALUE self, VALUE addr, VALUE len)
{
    return rb_str_new((char*)rb_num2ulong(addr), rb_num2ulong(len));
}

// memory reading (integers/floats)
static VALUE rb_dfmemory_read_int8(VALUE self, VALUE addr)
{
    return rb_int2inum(*(char*)rb_num2ulong(addr));
}
static VALUE rb_dfmemory_read_int16(VALUE self, VALUE addr)
{
    return rb_int2inum(*(short*)rb_num2ulong(addr));
}
static VALUE rb_dfmemory_read_int32(VALUE self, VALUE addr)
{
    return rb_int2inum(*(int*)rb_num2ulong(addr));
}

static VALUE rb_dfmemory_read_float(VALUE self, VALUE addr)
{
    return rb_float_new(*(float*)rb_num2ulong(addr));
}

static VALUE rb_dfmemory_read_double(VALUE self, VALUE addr)
{
    return rb_float_new(*(double*)rb_num2ulong(addr));
}


// memory writing (buffer)
static VALUE rb_dfmemory_write(VALUE self, VALUE addr, VALUE raw)
{
    // no stable api for raw.length between rb1.8/rb1.9 ...
    int strlen = FIX2INT(rb_funcall(raw, rb_intern("length"), 0));

    memcpy((void*)rb_num2ulong(addr), rb_string_value_ptr(&raw), strlen);

    return Qtrue;
}

// memory writing (integers/floats)
static VALUE rb_dfmemory_write_int8(VALUE self, VALUE addr, VALUE val)
{
    *(char*)rb_num2ulong(addr) = rb_num2ulong(val);
    return Qtrue;
}
static VALUE rb_dfmemory_write_int16(VALUE self, VALUE addr, VALUE val)
{
    *(short*)rb_num2ulong(addr) = rb_num2ulong(val);
    return Qtrue;
}
static VALUE rb_dfmemory_write_int32(VALUE self, VALUE addr, VALUE val)
{
    *(int*)rb_num2ulong(addr) = rb_num2ulong(val);
    return Qtrue;
}

static VALUE rb_dfmemory_write_float(VALUE self, VALUE addr, VALUE val)
{
    *(float*)rb_num2ulong(addr) = rb_num2dbl(val);
    return Qtrue;
}

static VALUE rb_dfmemory_write_double(VALUE self, VALUE addr, VALUE val)
{
    *(double*)rb_num2ulong(addr) = rb_num2dbl(val);
    return Qtrue;
}

// return memory permissions at address (eg "rx", nil if unmapped)
static VALUE rb_dfmemory_check(VALUE self, VALUE addr)
{
    void *ptr = (void*)rb_num2ulong(addr);
    std::vector<t_memrange> ranges;
    Core::getInstance().p->getMemRanges(ranges);

    unsigned i = 0;
    while (i < ranges.size() && ranges[i].end <= ptr)
        i++;

    if (i >= ranges.size() || ranges[i].start > ptr || !ranges[i].valid)
        return Qnil;

    std::string perm = "";
    if (ranges[i].read)
        perm += "r";
    if (ranges[i].write)
        perm += "w";
    if (ranges[i].execute)
        perm += "x";
    if (ranges[i].shared)
        perm += "s";

    return rb_str_new(perm.c_str(), perm.length());
}

// memory write (tmp override page permissions, eg patch code)
static VALUE rb_dfmemory_patch(VALUE self, VALUE addr, VALUE raw)
{
    int strlen = FIX2INT(rb_funcall(raw, rb_intern("length"), 0));
    bool ret;

    ret = Core::getInstance().p->patchMemory((void*)rb_num2ulong(addr),
            rb_string_value_ptr(&raw), strlen);

    return ret ? Qtrue : Qfalse;
}

// allocate memory pages
static VALUE rb_dfmemory_pagealloc(VALUE self, VALUE len)
{
    void *ret = Core::getInstance().p->memAlloc(rb_num2ulong(len));

    return (ret == (void*)-1) ? Qnil : rb_uint2inum((uint32_t)ret);
}

// free memory from pagealloc
static VALUE rb_dfmemory_pagedealloc(VALUE self, VALUE ptr, VALUE len)
{
    int ret = Core::getInstance().p->memDealloc((void*)rb_num2ulong(ptr), rb_num2ulong(len));

    return ret ? Qfalse : Qtrue;
}

// change memory page permissions
// ptr must be page-aligned
// prot is a String, eg 'rwx', 'r', 'x'
static VALUE rb_dfmemory_pageprotect(VALUE self, VALUE ptr, VALUE len, VALUE prot_str)
{
    int ret, prot=0;
    char *prot_p = rb_string_value_ptr(&prot_str);

    if (*prot_p == 'r') {
        prot |= Process::MemProt::READ;
        ++prot_p;
    }
    if (*prot_p == 'w') {
        prot |= Process::MemProt::WRITE;
        ++prot_p;
    }
    if (*prot_p == 'x') {
        prot |= Process::MemProt::EXEC;
        ++prot_p;
    }

    Core::printerr("pageprot %x %x %x\n", rb_num2ulong(ptr), rb_num2ulong(len), prot);
    ret = Core::getInstance().p->memProtect((void*)rb_num2ulong(ptr), rb_num2ulong(len), prot);

    return ret ? Qfalse : Qtrue;
}


// stl::string
static VALUE rb_dfmemory_stlstring_new(VALUE self)
{
    std::string *ptr = new std::string;
    return rb_uint2inum((uint32_t)ptr);
}
static VALUE rb_dfmemory_stlstring_delete(VALUE self, VALUE addr)
{
    std::string *ptr = (std::string*)rb_num2ulong(addr);
    if (ptr)
        delete ptr;
    return Qtrue;
}
static VALUE rb_dfmemory_stlstring_init(VALUE self, VALUE addr)
{
    new((void*)rb_num2ulong(addr)) std::string();
    return Qtrue;
}
static VALUE rb_dfmemory_read_stlstring(VALUE self, VALUE addr)
{
    std::string *s = (std::string*)rb_num2ulong(addr);
    return rb_str_new(s->c_str(), s->length());
}
static VALUE rb_dfmemory_write_stlstring(VALUE self, VALUE addr, VALUE val)
{
    std::string *s = (std::string*)rb_num2ulong(addr);
    int strlen = FIX2INT(rb_funcall(val, rb_intern("length"), 0));
    s->assign(rb_string_value_ptr(&val), strlen);
    return Qtrue;
}


// vector access
static VALUE rb_dfmemory_vec_new(VALUE self)
{
    std::vector<uint8_t> *ptr = new std::vector<uint8_t>;
    return rb_uint2inum((uint32_t)ptr);
}
static VALUE rb_dfmemory_vec_delete(VALUE self, VALUE addr)
{
    std::vector<uint8_t> *ptr = (std::vector<uint8_t>*)rb_num2ulong(addr);
    if (ptr)
        delete ptr;
    return Qtrue;
}
static VALUE rb_dfmemory_vec_init(VALUE self, VALUE addr)
{
    new((void*)rb_num2ulong(addr)) std::vector<uint8_t>();
    return Qtrue;
}
// vector<uint8>
static VALUE rb_dfmemory_vec8_length(VALUE self, VALUE addr)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->size());
}
static VALUE rb_dfmemory_vec8_ptrat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    return rb_uint2inum((uint32_t)&v->at(FIX2INT(idx)));
}
static VALUE rb_dfmemory_vec8_insertat(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), rb_num2ulong(val));
    return Qtrue;
}
static VALUE rb_dfmemory_vec8_deleteat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    v->erase(v->begin()+FIX2INT(idx));
    return Qtrue;
}

// vector<uint16>
static VALUE rb_dfmemory_vec16_length(VALUE self, VALUE addr)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->size());
}
static VALUE rb_dfmemory_vec16_ptrat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    return rb_uint2inum((uint32_t)&v->at(FIX2INT(idx)));
}
static VALUE rb_dfmemory_vec16_insertat(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), rb_num2ulong(val));
    return Qtrue;
}
static VALUE rb_dfmemory_vec16_deleteat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    v->erase(v->begin()+FIX2INT(idx));
    return Qtrue;
}

// vector<uint32>
static VALUE rb_dfmemory_vec32_length(VALUE self, VALUE addr)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->size());
}
static VALUE rb_dfmemory_vec32_ptrat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    return rb_uint2inum((uint32_t)&v->at(FIX2INT(idx)));
}
static VALUE rb_dfmemory_vec32_insertat(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), rb_num2ulong(val));
    return Qtrue;
}
static VALUE rb_dfmemory_vec32_deleteat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    v->erase(v->begin()+FIX2INT(idx));
    return Qtrue;
}

// vector<bool>
static VALUE rb_dfmemory_vecbool_new(VALUE self)
{
    std::vector<bool> *ptr = new std::vector<bool>;
    return rb_uint2inum((uint32_t)ptr);
}
static VALUE rb_dfmemory_vecbool_delete(VALUE self, VALUE addr)
{
    std::vector<bool> *ptr = (std::vector<bool>*)rb_num2ulong(addr);
    if (ptr)
        delete ptr;
    return Qtrue;
}
static VALUE rb_dfmemory_vecbool_init(VALUE self, VALUE addr)
{
    new((void*)rb_num2ulong(addr)) std::vector<bool>();
    return Qtrue;
}
static VALUE rb_dfmemory_vecbool_length(VALUE self, VALUE addr)
{
    std::vector<bool> *v = (std::vector<bool>*)rb_num2ulong(addr);
    return rb_uint2inum(v->size());
}
static VALUE rb_dfmemory_vecbool_at(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<bool> *v = (std::vector<bool>*)rb_num2ulong(addr);
    return v->at(FIX2INT(idx)) ? Qtrue : Qfalse;
}
static VALUE rb_dfmemory_vecbool_setat(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<bool> *v = (std::vector<bool>*)rb_num2ulong(addr);
    v->at(FIX2INT(idx)) = (BOOL_ISFALSE(val) ? 0 : 1);
    return Qtrue;
}
static VALUE rb_dfmemory_vecbool_insertat(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<bool> *v = (std::vector<bool>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), (BOOL_ISFALSE(val) ? 0 : 1));
    return Qtrue;
}
static VALUE rb_dfmemory_vecbool_deleteat(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<bool> *v = (std::vector<bool>*)rb_num2ulong(addr);
    v->erase(v->begin()+FIX2INT(idx));
    return Qtrue;
}

// BitArray
static VALUE rb_dfmemory_bitarray_length(VALUE self, VALUE addr)
{
    DFHack::BitArray<int> *b = (DFHack::BitArray<int>*)rb_num2ulong(addr);
    return rb_uint2inum(b->size*8);     // b->size is in bytes
}
static VALUE rb_dfmemory_bitarray_resize(VALUE self, VALUE addr, VALUE sz)
{
    DFHack::BitArray<int> *b = (DFHack::BitArray<int>*)rb_num2ulong(addr);
    b->resize(rb_num2ulong(sz));
    return Qtrue;
}
static VALUE rb_dfmemory_bitarray_isset(VALUE self, VALUE addr, VALUE idx)
{
    DFHack::BitArray<int> *b = (DFHack::BitArray<int>*)rb_num2ulong(addr);
    return b->is_set(rb_num2ulong(idx)) ? Qtrue : Qfalse;
}
static VALUE rb_dfmemory_bitarray_set(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    DFHack::BitArray<int> *b = (DFHack::BitArray<int>*)rb_num2ulong(addr);
    b->set(rb_num2ulong(idx), (BOOL_ISFALSE(val) ? 0 : 1));
    return Qtrue;
}

// add basic support for std::set<uint32> used for passing keyboard keys to viewscreens
#include <set>
static VALUE rb_dfmemory_set_new(VALUE self)
{
    std::set<unsigned long> *ptr = new std::set<unsigned long>;
    return rb_uint2inum((uint32_t)ptr);
}

static VALUE rb_dfmemory_set_delete(VALUE self, VALUE set)
{
    std::set<unsigned long> *ptr = (std::set<unsigned long>*)rb_num2ulong(set);
    if (ptr)
        delete ptr;
    return Qtrue;
}

static VALUE rb_dfmemory_set_set(VALUE self, VALUE set, VALUE key)
{
    std::set<unsigned long> *ptr = (std::set<unsigned long>*)rb_num2ulong(set);
    ptr->insert(rb_num2ulong(key));
    return Qtrue;
}

static VALUE rb_dfmemory_set_isset(VALUE self, VALUE set, VALUE key)
{
    std::set<unsigned long> *ptr = (std::set<unsigned long>*)rb_num2ulong(set);
    return ptr->count(rb_num2ulong(key)) ? Qtrue : Qfalse;
}

static VALUE rb_dfmemory_set_deletekey(VALUE self, VALUE set, VALUE key)
{
    std::set<unsigned long> *ptr = (std::set<unsigned long>*)rb_num2ulong(set);
    ptr->erase(rb_num2ulong(key));
    return Qtrue;
}

static VALUE rb_dfmemory_set_clear(VALUE self, VALUE set)
{
    std::set<unsigned long> *ptr = (std::set<unsigned long>*)rb_num2ulong(set);
    ptr->clear();
    return Qtrue;
}


/* call an arbitrary object virtual method */
#ifdef WIN32
__declspec(naked) static int raw_vcall(void *that, void *fptr, unsigned long a0,
        unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5)
{
    // __thiscall requires that the callee cleans up the stack
    // here we dont know how many arguments it will take, so
    // we simply fix esp across the funcall
    __asm {
        push ebp
        mov ebp, esp

        push a5
        push a4
        push a3
        push a2
        push a1
        push a0

        mov ecx, that

        call fptr

        mov esp, ebp
        pop ebp
        ret
    }
}
#else
static int raw_vcall(void *that, void *fptr, unsigned long a0,
        unsigned long a1, unsigned long a2, unsigned long a3, unsigned long a4, unsigned long a5)
{
    int (*t_fptr)(void *me, int, int, int, int, int, int);
    t_fptr = (decltype(t_fptr))fptr;
    return t_fptr(that, a0, a1, a2, a3, a4, a5);
}
#endif

// call an arbitrary vmethod, convert args/ret to native values for raw_vcall
static VALUE rb_dfvcall(VALUE self, VALUE cppobj, VALUE fptr, VALUE a0, VALUE a1, VALUE a2, VALUE a3, VALUE a4, VALUE a5)
{
    return rb_int2inum(raw_vcall((void*)rb_num2ulong(cppobj), (void*)rb_num2ulong(fptr),
                rb_num2ulong(a0), rb_num2ulong(a1),
                rb_num2ulong(a2), rb_num2ulong(a3),
                rb_num2ulong(a4), rb_num2ulong(a5)));
}


// define module DFHack and its methods
static void ruby_bind_dfhack(void) {
    rb_cDFHack = rb_define_module("DFHack");

    rb_define_singleton_method(rb_cDFHack, "onupdate_active", RUBY_METHOD_FUNC(rb_dfonupdate_active), 0);
    rb_define_singleton_method(rb_cDFHack, "onupdate_active=", RUBY_METHOD_FUNC(rb_dfonupdate_active_set), 1);
    rb_define_singleton_method(rb_cDFHack, "onupdate_minyear", RUBY_METHOD_FUNC(rb_dfonupdate_minyear), 0);
    rb_define_singleton_method(rb_cDFHack, "onupdate_minyear=", RUBY_METHOD_FUNC(rb_dfonupdate_minyear_set), 1);
    rb_define_singleton_method(rb_cDFHack, "onupdate_minyeartick", RUBY_METHOD_FUNC(rb_dfonupdate_minyeartick), 0);
    rb_define_singleton_method(rb_cDFHack, "onupdate_minyeartick=", RUBY_METHOD_FUNC(rb_dfonupdate_minyeartick_set), 1);
    rb_define_singleton_method(rb_cDFHack, "onupdate_minyeartickadv", RUBY_METHOD_FUNC(rb_dfonupdate_minyeartickadv), 0);
    rb_define_singleton_method(rb_cDFHack, "onupdate_minyeartickadv=", RUBY_METHOD_FUNC(rb_dfonupdate_minyeartickadv_set), 1);
    rb_define_singleton_method(rb_cDFHack, "get_global_address", RUBY_METHOD_FUNC(rb_dfget_global_address), 1);
    rb_define_singleton_method(rb_cDFHack, "get_vtable", RUBY_METHOD_FUNC(rb_dfget_vtable), 1);
    rb_define_singleton_method(rb_cDFHack, "get_rtti_classname", RUBY_METHOD_FUNC(rb_dfget_rtti_classname), 1);
    rb_define_singleton_method(rb_cDFHack, "get_vtable_ptr", RUBY_METHOD_FUNC(rb_dfget_vtable_ptr), 1);
    rb_define_singleton_method(rb_cDFHack, "dfhack_run", RUBY_METHOD_FUNC(rb_dfhack_run), 1);
    rb_define_singleton_method(rb_cDFHack, "print_str", RUBY_METHOD_FUNC(rb_dfprint_str), 1);
    rb_define_singleton_method(rb_cDFHack, "print_color", RUBY_METHOD_FUNC(rb_dfprint_color), 2);
    rb_define_singleton_method(rb_cDFHack, "print_err", RUBY_METHOD_FUNC(rb_dfprint_err), 1);
    rb_define_singleton_method(rb_cDFHack, "malloc", RUBY_METHOD_FUNC(rb_dfmalloc), 1);
    rb_define_singleton_method(rb_cDFHack, "free", RUBY_METHOD_FUNC(rb_dffree), 1);
    rb_define_singleton_method(rb_cDFHack, "pagealloc", RUBY_METHOD_FUNC(rb_dfmemory_pagealloc), 1);
    rb_define_singleton_method(rb_cDFHack, "pagedealloc", RUBY_METHOD_FUNC(rb_dfmemory_pagedealloc), 2);
    rb_define_singleton_method(rb_cDFHack, "pageprotect", RUBY_METHOD_FUNC(rb_dfmemory_pageprotect), 3);
    rb_define_singleton_method(rb_cDFHack, "vmethod_do_call", RUBY_METHOD_FUNC(rb_dfvcall), 8);
    rb_define_singleton_method(rb_cDFHack, "version", RUBY_METHOD_FUNC(rb_dfversion), 0);

    rb_define_singleton_method(rb_cDFHack, "memory_read", RUBY_METHOD_FUNC(rb_dfmemory_read), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int8",  RUBY_METHOD_FUNC(rb_dfmemory_read_int8),  1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int16", RUBY_METHOD_FUNC(rb_dfmemory_read_int16), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int32", RUBY_METHOD_FUNC(rb_dfmemory_read_int32), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_float", RUBY_METHOD_FUNC(rb_dfmemory_read_float), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_double", RUBY_METHOD_FUNC(rb_dfmemory_read_double), 1);

    rb_define_singleton_method(rb_cDFHack, "memory_write", RUBY_METHOD_FUNC(rb_dfmemory_write), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int8",  RUBY_METHOD_FUNC(rb_dfmemory_write_int8),  2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int16", RUBY_METHOD_FUNC(rb_dfmemory_write_int16), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int32", RUBY_METHOD_FUNC(rb_dfmemory_write_int32), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_float", RUBY_METHOD_FUNC(rb_dfmemory_write_float), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_double", RUBY_METHOD_FUNC(rb_dfmemory_write_double), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_check", RUBY_METHOD_FUNC(rb_dfmemory_check), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_patch", RUBY_METHOD_FUNC(rb_dfmemory_patch), 2);

    rb_define_singleton_method(rb_cDFHack, "memory_stlstring_new",  RUBY_METHOD_FUNC(rb_dfmemory_stlstring_new), 0);
    rb_define_singleton_method(rb_cDFHack, "memory_stlstring_delete",  RUBY_METHOD_FUNC(rb_dfmemory_stlstring_delete), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_stlstring_init",  RUBY_METHOD_FUNC(rb_dfmemory_stlstring_init), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_stlstring", RUBY_METHOD_FUNC(rb_dfmemory_read_stlstring), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_write_stlstring", RUBY_METHOD_FUNC(rb_dfmemory_write_stlstring), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector_new",  RUBY_METHOD_FUNC(rb_dfmemory_vec_new), 0);
    rb_define_singleton_method(rb_cDFHack, "memory_vector_delete",  RUBY_METHOD_FUNC(rb_dfmemory_vec_delete), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector_init",  RUBY_METHOD_FUNC(rb_dfmemory_vec_init), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_length",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_ptrat",   RUBY_METHOD_FUNC(rb_dfmemory_vec8_ptrat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_insertat",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_insertat), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_deleteat",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_deleteat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_length", RUBY_METHOD_FUNC(rb_dfmemory_vec16_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_ptrat",  RUBY_METHOD_FUNC(rb_dfmemory_vec16_ptrat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_insertat", RUBY_METHOD_FUNC(rb_dfmemory_vec16_insertat), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_deleteat", RUBY_METHOD_FUNC(rb_dfmemory_vec16_deleteat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_length", RUBY_METHOD_FUNC(rb_dfmemory_vec32_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_ptrat",  RUBY_METHOD_FUNC(rb_dfmemory_vec32_ptrat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_insertat", RUBY_METHOD_FUNC(rb_dfmemory_vec32_insertat), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_deleteat", RUBY_METHOD_FUNC(rb_dfmemory_vec32_deleteat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_new",  RUBY_METHOD_FUNC(rb_dfmemory_vecbool_new), 0);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_delete",  RUBY_METHOD_FUNC(rb_dfmemory_vecbool_delete), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_init",  RUBY_METHOD_FUNC(rb_dfmemory_vecbool_init), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_length", RUBY_METHOD_FUNC(rb_dfmemory_vecbool_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_at",     RUBY_METHOD_FUNC(rb_dfmemory_vecbool_at), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_setat",  RUBY_METHOD_FUNC(rb_dfmemory_vecbool_setat), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_insertat", RUBY_METHOD_FUNC(rb_dfmemory_vecbool_insertat), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_deleteat", RUBY_METHOD_FUNC(rb_dfmemory_vecbool_deleteat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_length", RUBY_METHOD_FUNC(rb_dfmemory_bitarray_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_resize", RUBY_METHOD_FUNC(rb_dfmemory_bitarray_resize), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_isset",  RUBY_METHOD_FUNC(rb_dfmemory_bitarray_isset), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_set",    RUBY_METHOD_FUNC(rb_dfmemory_bitarray_set), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_stlset_new",       RUBY_METHOD_FUNC(rb_dfmemory_set_new), 0);
    rb_define_singleton_method(rb_cDFHack, "memory_stlset_delete",    RUBY_METHOD_FUNC(rb_dfmemory_set_delete), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_stlset_set",       RUBY_METHOD_FUNC(rb_dfmemory_set_set), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_stlset_isset",     RUBY_METHOD_FUNC(rb_dfmemory_set_isset), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_stlset_deletekey", RUBY_METHOD_FUNC(rb_dfmemory_set_deletekey), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_stlset_clear",     RUBY_METHOD_FUNC(rb_dfmemory_set_clear), 1);
}
