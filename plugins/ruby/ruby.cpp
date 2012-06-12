// blindly copied imports from fastdwarf
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "VersionInfo.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/unit.h"

#include "tinythread.h"

using namespace DFHack;



// DFHack stuff


static int df_loadruby(void);
static void df_unloadruby(void);
static void df_rubythread(void*);
static command_result df_rubyload(color_ostream &out, std::vector <std::string> & parameters);
static command_result df_rubyeval(color_ostream &out, std::vector <std::string> & parameters);
static void ruby_bind_dfhack(void);

// inter-thread communication stuff
enum RB_command {
    RB_IDLE,
    RB_INIT,
    RB_DIE,
    RB_EVAL,
    RB_CUSTOM,
};
tthread::mutex *m_irun;
tthread::mutex *m_mutex;
static RB_command r_type;
static const char *r_command;
static command_result r_result;
static tthread::thread *r_thread;
static int onupdate_active;

DFHACK_PLUGIN("ruby")

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    // fail silently instead of spamming the console with 'failed to initialize' if libruby is not present
    // the error is still logged in stderr.log
    if (!df_loadruby())
        return CR_OK;

    m_irun = new tthread::mutex();
    m_mutex = new tthread::mutex();
    r_type = RB_INIT;

    r_thread = new tthread::thread(df_rubythread, 0);

    while (r_type != RB_IDLE)
        tthread::this_thread::yield();

    m_irun->lock();

    if (r_result == CR_FAILURE)
        return CR_FAILURE;

    onupdate_active = 0;

    commands.push_back(PluginCommand("rb_load",
                "Ruby interpreter. Loads the given ruby script.",
                df_rubyload));

    commands.push_back(PluginCommand("rb_eval",
                "Ruby interpreter. Eval() a ruby string.",
                df_rubyeval));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    if (!r_thread)
        return CR_OK;

    m_mutex->lock();

    r_type = RB_DIE;
    r_command = 0;
    m_irun->unlock();

    r_thread->join();

    delete r_thread;
    r_thread = 0;
    delete m_irun;
    m_mutex->unlock();
    delete m_mutex;

    df_unloadruby();

    return CR_OK;
}

// send a single ruby line to be evaluated by the ruby thread
static command_result plugin_eval_rb(const char *command)
{
    command_result ret;

    // serialize 'accesses' to the ruby thread
    m_mutex->lock();
    if (!r_thread)
        // raced with plugin_shutdown ?
        return CR_OK;

    r_type = RB_EVAL;
    r_command = command;
    m_irun->unlock();

    // could use a condition_variable or something...
    while (r_type != RB_IDLE)
        tthread::this_thread::yield();

    // XXX non-atomic with previous r_type change check
    ret = r_result;

    m_irun->lock();
    m_mutex->unlock();

    return ret;
}

static command_result plugin_eval_rb(std::string &command)
{
    return plugin_eval_rb(command.c_str());
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if (!r_thread)
        return CR_OK;

    if (!onupdate_active)
        return CR_OK;

    return plugin_eval_rb("DFHack.onupdate");
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
        SCASE(BEGIN_UNLOAD);
#undef SCASE
    }

    return plugin_eval_rb(cmd);
}

static command_result df_rubyload(color_ostream &out, std::vector <std::string> & parameters)
{
    if (parameters.size() == 1 && (parameters[0] == "help" || parameters[0] == "?"))
    {
        out.print("This command loads the ruby script whose path is given as parameter, and run it.\n");
        return CR_OK;
    }

    std::string cmd = "load '";
    cmd += parameters[0];       // TODO escape singlequotes
    cmd += "'";

    return plugin_eval_rb(cmd);
}

static command_result df_rubyeval(color_ostream &out, std::vector <std::string> & parameters)
{
    command_result ret;

    if (parameters.size() == 1 && (parameters[0] == "help" || parameters[0] == "?"))
    {
        out.print("This command executes an arbitrary ruby statement.\n");
        return CR_OK;
    }

    std::string full = "";

    for (unsigned i=0 ; i<parameters.size() ; ++i) {
        full += parameters[i];
        if (i != parameters.size()-1)
            full += " ";
    }

    return plugin_eval_rb(full);
}



// ruby stuff

// ruby-dev on windows is messy
// ruby.h on linux 64 is broken
// so we dynamically load libruby instead of linking it at compile time
// lib path can be set in dfhack.ini to use the system libruby, but by
// default we'll embed our own (downloaded by cmake)

// these ruby definitions are invalid for windows 64bit
typedef unsigned long VALUE;
typedef unsigned long ID;

#define Qfalse ((VALUE)0)
#define Qtrue  ((VALUE)2)
#define Qnil   ((VALUE)4)

#define INT2FIX(i) ((VALUE)((((long)i) << 1) | 1))
#define FIX2INT(i) (((long)i) >> 1)
#define RUBY_METHOD_FUNC(func) ((VALUE(*)(...))func)

VALUE *rb_eRuntimeError;

void (*ruby_sysinit)(int *, const char ***);
void (*ruby_init)(void);
void (*ruby_init_loadpath)(void);
void (*ruby_script)(const char*);
void (*ruby_finalize)(void);
ID (*rb_intern)(const char*);
VALUE (*rb_raise)(VALUE, const char*, ...);
VALUE (*rb_funcall)(VALUE, ID, int, ...);
VALUE (*rb_define_module)(const char*);
void (*rb_define_singleton_method)(VALUE, const char*, VALUE(*)(...), int);
void (*rb_define_const)(VALUE, const char*, VALUE);
void (*rb_load_protect)(VALUE, int, int*);
VALUE (*rb_gv_get)(const char*);
VALUE (*rb_str_new)(const char*, long);
VALUE (*rb_str_new2)(const char*);
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
#ifdef WIN32
        "./libruby.dll";
#else
        "hack/libruby.so";
#endif

    libruby_handle = OpenPlugin(libpath);
    if (!libruby_handle) {
        fprintf(stderr, "Cannot initialize ruby plugin: failed to load %s\n", libpath);
        return 0;
    }

    if (!(rb_eRuntimeError = (VALUE*)LookupPlugin(libruby_handle, "rb_eRuntimeError")))
        return 0;

    // XXX does msvc support decltype ? might need a #define decltype typeof
    // or just assign to *(void**)(&s) = ...
    // ruby_sysinit is optional (ruby1.9 only)
    ruby_sysinit = (decltype(ruby_sysinit))LookupPlugin(libruby_handle, "ruby_sysinit");
#define rbloadsym(s) if (!(s = (decltype(s))LookupPlugin(libruby_handle, #s))) return 0
    rbloadsym(ruby_init);
    rbloadsym(ruby_init_loadpath);
    rbloadsym(ruby_script);
    rbloadsym(ruby_finalize);
    rbloadsym(rb_intern);
    rbloadsym(rb_raise);
    rbloadsym(rb_funcall);
    rbloadsym(rb_define_module);
    rbloadsym(rb_define_singleton_method);
    rbloadsym(rb_define_const);
    rbloadsym(rb_load_protect);
    rbloadsym(rb_gv_get);
    rbloadsym(rb_str_new);
    rbloadsym(rb_str_new2);
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


// ruby thread code
static void dump_rb_error(void)
{
    VALUE s, err;

    err = rb_gv_get("$!");

    s = rb_funcall(err, rb_intern("class"), 0);
    s = rb_funcall(s, rb_intern("name"), 0);
    Core::printerr("E: %s: ", rb_string_value_ptr(&s));

    s = rb_funcall(err, rb_intern("message"), 0);
    Core::printerr("%s\n", rb_string_value_ptr(&s));

    err = rb_funcall(err, rb_intern("backtrace"), 0);
    for (int i=0 ; i<8 ; ++i)
        if ((s = rb_ary_shift(err)) != Qnil)
            Core::printerr(" %s\n", rb_string_value_ptr(&s));
}

static color_ostream_proxy *console_proxy;

// ruby thread main loop
static void df_rubythread(void *p)
{
    int state, running;

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

    r_result = CR_OK;
    r_type = RB_IDLE;

    running = 1;
    while (running) {
        // wait for new command
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

        case RB_CUSTOM:
            // TODO handle ruby custom commands
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

// enable/disable calls to DFHack.onupdate()
static VALUE rb_dfonupdateactive(VALUE self)
{
    if (onupdate_active)
        return Qtrue;
    else
        return Qfalse;
}

static VALUE rb_dfonupdateactiveset(VALUE self, VALUE val)
{
    onupdate_active = (BOOL_ISFALSE(val) ? 0 : 1);
    return Qtrue;
}

static VALUE rb_dfresume(VALUE self)
{
    Core::getInstance().Resume();
    return Qtrue;
}

static VALUE rb_dfsuspend(VALUE self)
{
    Core::getInstance().Suspend();
    return Qtrue;
}

// returns the delta to apply to dfhack xml addrs wrt actual memory addresses
// usage: real_addr = addr_from_xml + this_delta;
static VALUE rb_dfrebase_delta(void)
{
    uint32_t expected_base_address;
    uint32_t actual_base_address = 0;
#ifdef WIN32
    expected_base_address = 0x00400000;
    actual_base_address = (uint32_t)GetModuleHandle(0);
#else
    expected_base_address = 0x08048000;
    FILE *f = fopen("/proc/self/maps", "r");
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, "libs/Dwarf_Fortress")) {
            actual_base_address = strtoul(line, 0, 16);
            break;
        }
    }
#endif

    return rb_int2inum(actual_base_address - expected_base_address);
}

static VALUE rb_dfprint_str(VALUE self, VALUE s)
{
    console_proxy->print("%s", rb_string_value_ptr(&s));
    return Qnil;
}

static VALUE rb_dfprint_err(VALUE self, VALUE s)
{
    Core::printerr("%s", rb_string_value_ptr(&s));
    return Qnil;
}

/* TODO needs main dfhack support
   this needs a custom DFHack::Plugin subclass to pass the cmdname to invoke(), to match the ruby callback
// register a ruby method as dfhack console command
// usage: DFHack.register("moo", "this commands prints moo on the console") { DFHack.puts "moo !" }
static VALUE rb_dfregister(VALUE self, VALUE name, VALUE descr)
{
    commands.push_back(PluginCommand(rb_string_value_ptr(&name),
                rb_string_value_ptr(&descr),
                df_rubycustom));

    return Qtrue;
}
*/
static VALUE rb_dfregister(VALUE self, VALUE name, VALUE descr)
{
    rb_raise(*rb_eRuntimeError, "not implemented");
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
    return rb_str_new2(typestring);
#endif
}

static VALUE rb_dfget_vtable_ptr(VALUE self, VALUE objptr)
{
    // actually, rb_dfmemory_read_int32
    return rb_uint2inum(*(uint32_t*)rb_num2ulong(objptr));
}




// raw memory access
// used by the ruby class definitions
// XXX may cause game crash ! double-check your addresses !

static VALUE rb_dfmalloc(VALUE self, VALUE len)
{
    char *ptr = (char*)malloc(FIX2INT(len));
    if (!ptr)
        rb_raise(*rb_eRuntimeError, "no memory");
    memset(ptr, 0, FIX2INT(len));
    return rb_uint2inum((long)ptr);
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


// stl::string
static VALUE rb_dfmemory_stlstring_init(VALUE self, VALUE addr)
{
    // XXX THIS IS TERRIBLE
    std::string *ptr = new std::string;
    memcpy((void*)rb_num2ulong(addr), (void*)ptr, sizeof(*ptr));
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
static VALUE rb_dfmemory_vec_init(VALUE self, VALUE addr)
{
    std::vector<uint8_t> *ptr = new std::vector<uint8_t>;
    memcpy((void*)rb_num2ulong(addr), (void*)ptr, sizeof(*ptr));
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
static VALUE rb_dfmemory_vec8_insert(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), rb_num2ulong(val));
    return Qtrue;
}
static VALUE rb_dfmemory_vec8_delete(VALUE self, VALUE addr, VALUE idx)
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
static VALUE rb_dfmemory_vec16_insert(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), rb_num2ulong(val));
    return Qtrue;
}
static VALUE rb_dfmemory_vec16_delete(VALUE self, VALUE addr, VALUE idx)
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
static VALUE rb_dfmemory_vec32_insert(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), rb_num2ulong(val));
    return Qtrue;
}
static VALUE rb_dfmemory_vec32_delete(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    v->erase(v->begin()+FIX2INT(idx));
    return Qtrue;
}

// vector<bool>
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
static VALUE rb_dfmemory_vecbool_insert(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<bool> *v = (std::vector<bool>*)rb_num2ulong(addr);
    v->insert(v->begin()+FIX2INT(idx), (BOOL_ISFALSE(val) ? 0 : 1));
    return Qtrue;
}
static VALUE rb_dfmemory_vecbool_delete(VALUE self, VALUE addr, VALUE idx)
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


/* call an arbitrary object virtual method */
static VALUE rb_dfvcall(VALUE self, VALUE cppobj, VALUE cppvoff, VALUE a0, VALUE a1, VALUE a2, VALUE a3)
{
#ifdef WIN32
    __thiscall
#endif
    int (*fptr)(char **me, int, int, int, int);
    char **that = (char**)rb_num2ulong(cppobj);
    int ret;
    fptr = (decltype(fptr))*(void**)(*that + rb_num2ulong(cppvoff));
    ret = fptr(that, rb_num2ulong(a0), rb_num2ulong(a1), rb_num2ulong(a2), rb_num2ulong(a3));
    return rb_int2inum(ret);
}



// define module DFHack and its methods
static void ruby_bind_dfhack(void) {
    rb_cDFHack = rb_define_module("DFHack");

    // global DFHack commands
    rb_define_singleton_method(rb_cDFHack, "onupdate_active", RUBY_METHOD_FUNC(rb_dfonupdateactive), 0);
    rb_define_singleton_method(rb_cDFHack, "onupdate_active=", RUBY_METHOD_FUNC(rb_dfonupdateactiveset), 1);
    rb_define_singleton_method(rb_cDFHack, "resume", RUBY_METHOD_FUNC(rb_dfresume), 0);
    rb_define_singleton_method(rb_cDFHack, "do_suspend", RUBY_METHOD_FUNC(rb_dfsuspend), 0);
    rb_define_singleton_method(rb_cDFHack, "get_global_address", RUBY_METHOD_FUNC(rb_dfget_global_address), 1);
    rb_define_singleton_method(rb_cDFHack, "get_vtable", RUBY_METHOD_FUNC(rb_dfget_vtable), 1);
    rb_define_singleton_method(rb_cDFHack, "get_rtti_classname", RUBY_METHOD_FUNC(rb_dfget_rtti_classname), 1);
    rb_define_singleton_method(rb_cDFHack, "get_vtable_ptr", RUBY_METHOD_FUNC(rb_dfget_vtable_ptr), 1);
    rb_define_singleton_method(rb_cDFHack, "register_dfcommand", RUBY_METHOD_FUNC(rb_dfregister), 2);
    rb_define_singleton_method(rb_cDFHack, "print_str", RUBY_METHOD_FUNC(rb_dfprint_str), 1);
    rb_define_singleton_method(rb_cDFHack, "print_err", RUBY_METHOD_FUNC(rb_dfprint_err), 1);
    rb_define_singleton_method(rb_cDFHack, "malloc", RUBY_METHOD_FUNC(rb_dfmalloc), 1);
    rb_define_singleton_method(rb_cDFHack, "free", RUBY_METHOD_FUNC(rb_dffree), 1);
    rb_define_singleton_method(rb_cDFHack, "vmethod_do_call", RUBY_METHOD_FUNC(rb_dfvcall), 6);
    rb_define_const(rb_cDFHack, "REBASE_DELTA", rb_dfrebase_delta());

    rb_define_singleton_method(rb_cDFHack, "memory_read", RUBY_METHOD_FUNC(rb_dfmemory_read), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int8",  RUBY_METHOD_FUNC(rb_dfmemory_read_int8),  1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int16", RUBY_METHOD_FUNC(rb_dfmemory_read_int16), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int32", RUBY_METHOD_FUNC(rb_dfmemory_read_int32), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_float", RUBY_METHOD_FUNC(rb_dfmemory_read_float), 1);

    rb_define_singleton_method(rb_cDFHack, "memory_write", RUBY_METHOD_FUNC(rb_dfmemory_write), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int8",  RUBY_METHOD_FUNC(rb_dfmemory_write_int8),  2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int16", RUBY_METHOD_FUNC(rb_dfmemory_write_int16), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int32", RUBY_METHOD_FUNC(rb_dfmemory_write_int32), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_float", RUBY_METHOD_FUNC(rb_dfmemory_write_float), 2);

    rb_define_singleton_method(rb_cDFHack, "memory_stlstring_init",  RUBY_METHOD_FUNC(rb_dfmemory_stlstring_init), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_stlstring", RUBY_METHOD_FUNC(rb_dfmemory_read_stlstring), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_write_stlstring", RUBY_METHOD_FUNC(rb_dfmemory_write_stlstring), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector_init",  RUBY_METHOD_FUNC(rb_dfmemory_vec_init), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_length",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_ptrat",   RUBY_METHOD_FUNC(rb_dfmemory_vec8_ptrat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_insert",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_delete",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_delete), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_length", RUBY_METHOD_FUNC(rb_dfmemory_vec16_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_ptrat",  RUBY_METHOD_FUNC(rb_dfmemory_vec16_ptrat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_insert", RUBY_METHOD_FUNC(rb_dfmemory_vec16_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_delete", RUBY_METHOD_FUNC(rb_dfmemory_vec16_delete), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_length", RUBY_METHOD_FUNC(rb_dfmemory_vec32_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_ptrat",  RUBY_METHOD_FUNC(rb_dfmemory_vec32_ptrat), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_insert", RUBY_METHOD_FUNC(rb_dfmemory_vec32_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_delete", RUBY_METHOD_FUNC(rb_dfmemory_vec32_delete), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_length", RUBY_METHOD_FUNC(rb_dfmemory_vecbool_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_at",     RUBY_METHOD_FUNC(rb_dfmemory_vecbool_at), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_setat",  RUBY_METHOD_FUNC(rb_dfmemory_vecbool_setat), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_insert", RUBY_METHOD_FUNC(rb_dfmemory_vecbool_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vectorbool_delete", RUBY_METHOD_FUNC(rb_dfmemory_vecbool_delete), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_length", RUBY_METHOD_FUNC(rb_dfmemory_bitarray_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_resize", RUBY_METHOD_FUNC(rb_dfmemory_bitarray_resize), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_isset",  RUBY_METHOD_FUNC(rb_dfmemory_bitarray_isset), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_bitarray_set",    RUBY_METHOD_FUNC(rb_dfmemory_bitarray_set), 3);

    // load the default ruby-level definitions
    int state=0;
    rb_load_protect(rb_str_new2("./hack/ruby.rb"), Qfalse, &state);
    if (state)
        dump_rb_error();
}
