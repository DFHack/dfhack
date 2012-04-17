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

#include <ruby.h>

using namespace DFHack;



// DFHack stuff


static void df_rubythread(void*);
static command_result df_rubyload(color_ostream &out, std::vector <std::string> & parameters);
static command_result df_rubyeval(color_ostream &out, std::vector <std::string> & parameters);
static void ruby_bind_dfhack(void);

// inter-thread communication stuff
enum RB_command {
    RB_IDLE,
    RB_INIT,
    RB_DIE,
    RB_LOAD,
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

    commands.push_back(PluginCommand("r",
                "Ruby interpreter dev. Eval() a ruby string.",
                df_rubyeval));

    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    m_mutex->lock();
    if (!r_thread)
        return CR_OK;

    r_type = RB_DIE;
    r_command = 0;
    m_irun->unlock();

    r_thread->join();

    delete r_thread;
    r_thread = 0;
    delete m_irun;
    m_mutex->unlock();
    delete m_mutex;

    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( color_ostream &out )
{
    if (!onupdate_active)
        return CR_OK;

    command_result ret;

    m_mutex->lock();
    if (!r_thread)
        return CR_OK;

    r_type = RB_EVAL;
    r_command = "DFHack.onupdate";
    m_irun->unlock();

    while (r_type != RB_IDLE)
        tthread::this_thread::yield();

    ret = r_result;

    m_irun->lock();
    m_mutex->unlock();

    return ret;
}

static command_result df_rubyload(color_ostream &out, std::vector <std::string> & parameters)
{
    command_result ret;

    if (parameters.size() == 1 && (parameters[0] == "help" || parameters[0] == "?"))
    {
        out.print("This command loads the ruby script whose path is given as parameter, and run it.\n");
        return CR_OK;
    }

    // serialize 'accesses' to the ruby thread
    m_mutex->lock();
    if (!r_thread)
        // raced with plugin_shutdown ?
        return CR_OK;

    r_type = RB_LOAD;
    r_command = parameters[0].c_str();
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

static command_result df_rubyeval(color_ostream &out, std::vector <std::string> & parameters)
{
    command_result ret;

    if (parameters.size() == 1 && (parameters[0] == "help" || parameters[0] == "?"))
    {
        out.print("This command executes an arbitrary ruby statement.\n");
        return CR_OK;
    }

    std::string full = "";
    full += "DFHack.puts((";

    for (unsigned i=0 ; i<parameters.size() ; ++i) {
        full += parameters[i];
        full += " ";
    }

    full += ").inspect)";

    m_mutex->lock();
    if (!r_thread)
        return CR_OK;

    r_type = RB_EVAL;
    r_command = full.c_str();
    m_irun->unlock();

    while (r_type != RB_IDLE)
        tthread::this_thread::yield();

    ret = r_result;

    m_irun->lock();
    m_mutex->unlock();

    return ret;
}



// ruby stuff

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

// ruby thread main loop
static void df_rubythread(void *p)
{
    int state, running;

    // initialize the ruby interpreter
    ruby_init();
    ruby_init_loadpath();
    // default value for the $0 "current script name"
    ruby_script("dfhack");

    // create the ruby objects to map DFHack to ruby methods
    ruby_bind_dfhack();

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

        case RB_LOAD:
            state = 0;
            rb_load_protect(rb_str_new2(r_command), Qfalse, &state);
            if (state)
                dump_rb_error();
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
    onupdate_active = (val == Qtrue || val == INT2FIX(1)) ? 1 : 0;
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
    // TODO color_ostream proxy yadda yadda
    //getcore().con.print("%s", rb_string_value_ptr(&s));
    Core::printerr("%s", rb_string_value_ptr(&s));
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
    rb_raise(rb_eRuntimeError, "not implemented");
}

static VALUE rb_dfget_global_address(VALUE self, VALUE name)
{
    uint32_t addr = Core::getInstance().vinfo->getAddress(rb_string_value_ptr(&name));
    return rb_uint2inum(addr);
}

static VALUE rb_dfget_vtable(VALUE self, VALUE name)
{
    uint32_t addr = (uint32_t)Core::getInstance().vinfo->getVTable(rb_string_value_ptr(&name));
    return rb_uint2inum(addr);
}

static VALUE rb_dfget_rtti_classname(VALUE self, VALUE objptr)
{
    void **ptr = (void**)rb_num2ulong(objptr);
    return rb_str_new2(typeid(*ptr).name());
}




// raw memory access
// used by the ruby class definitions
// WARNING: may cause game crash ! double-check your addresses !

static VALUE rb_dfmalloc(VALUE self, VALUE len)
{
    return rb_uint2inum((long)malloc(FIX2INT(len)));
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
static VALUE rb_dfmemory_read_stlstring(VALUE self, VALUE addr)
{
    std::string *s = (std::string*)rb_num2ulong(addr);
    return rb_str_new(s->c_str(), s->length());
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
static VALUE rb_dfmemory_write_stlstring(VALUE self, VALUE addr, VALUE val)
{
    std::string *s = (std::string*)rb_num2ulong(addr);
    int strlen = FIX2INT(rb_funcall(val, rb_intern("length"), 0));
    s->assign(rb_string_value_ptr(&val), strlen);
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


// vector access
// vector<uint8>
static VALUE rb_dfmemory_vec8_length(VALUE self, VALUE addr)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->size());
}
static VALUE rb_dfmemory_vec8_at(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->at(FIX2INT(idx)));
}
static VALUE rb_dfmemory_vec8_set(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint8_t> *v = (std::vector<uint8_t>*)rb_num2ulong(addr);
    v->at(FIX2INT(idx)) = rb_num2ulong(val);
    return Qtrue;
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
static VALUE rb_dfmemory_vec16_at(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->at(FIX2INT(idx)));
}
static VALUE rb_dfmemory_vec16_set(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint16_t> *v = (std::vector<uint16_t>*)rb_num2ulong(addr);
    v->at(FIX2INT(idx)) = rb_num2ulong(val);
    return Qtrue;
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
static VALUE rb_dfmemory_vec32_at(VALUE self, VALUE addr, VALUE idx)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    return rb_uint2inum(v->at(FIX2INT(idx)));
}
static VALUE rb_dfmemory_vec32_set(VALUE self, VALUE addr, VALUE idx, VALUE val)
{
    std::vector<uint32_t> *v = (std::vector<uint32_t>*)rb_num2ulong(addr);
    v->at(FIX2INT(idx)) = rb_num2ulong(val);
    return Qtrue;
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
    rb_define_singleton_method(rb_cDFHack, "register_dfcommand", RUBY_METHOD_FUNC(rb_dfregister), 2);
    rb_define_singleton_method(rb_cDFHack, "print_str", RUBY_METHOD_FUNC(rb_dfprint_str), 1);
    rb_define_singleton_method(rb_cDFHack, "print_err", RUBY_METHOD_FUNC(rb_dfprint_err), 1);
    rb_define_singleton_method(rb_cDFHack, "malloc", RUBY_METHOD_FUNC(rb_dfmalloc), 1);
    rb_define_singleton_method(rb_cDFHack, "free", RUBY_METHOD_FUNC(rb_dffree), 1);
    rb_define_const(rb_cDFHack, "REBASE_DELTA", rb_dfrebase_delta());

    rb_define_singleton_method(rb_cDFHack, "memory_read", RUBY_METHOD_FUNC(rb_dfmemory_read), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_read_stlstring", RUBY_METHOD_FUNC(rb_dfmemory_read_stlstring), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int8",  RUBY_METHOD_FUNC(rb_dfmemory_read_int8),  1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int16", RUBY_METHOD_FUNC(rb_dfmemory_read_int16), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_int32", RUBY_METHOD_FUNC(rb_dfmemory_read_int32), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_read_float", RUBY_METHOD_FUNC(rb_dfmemory_read_float), 1);

    rb_define_singleton_method(rb_cDFHack, "memory_write", RUBY_METHOD_FUNC(rb_dfmemory_write), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_stlstring", RUBY_METHOD_FUNC(rb_dfmemory_write_stlstring), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int8",  RUBY_METHOD_FUNC(rb_dfmemory_write_int8),  2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int16", RUBY_METHOD_FUNC(rb_dfmemory_write_int16), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_int32", RUBY_METHOD_FUNC(rb_dfmemory_write_int32), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_write_float", RUBY_METHOD_FUNC(rb_dfmemory_write_float), 2);

    rb_define_singleton_method(rb_cDFHack, "memory_vector8_length",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_at",      RUBY_METHOD_FUNC(rb_dfmemory_vec8_at), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_set",     RUBY_METHOD_FUNC(rb_dfmemory_vec8_set), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_insert",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector8_delete",  RUBY_METHOD_FUNC(rb_dfmemory_vec8_delete), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_length", RUBY_METHOD_FUNC(rb_dfmemory_vec16_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_at",     RUBY_METHOD_FUNC(rb_dfmemory_vec16_at), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_set",    RUBY_METHOD_FUNC(rb_dfmemory_vec16_set), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_insert", RUBY_METHOD_FUNC(rb_dfmemory_vec16_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector16_delete", RUBY_METHOD_FUNC(rb_dfmemory_vec16_delete), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_length", RUBY_METHOD_FUNC(rb_dfmemory_vec32_length), 1);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_at",     RUBY_METHOD_FUNC(rb_dfmemory_vec32_at), 2);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_set",    RUBY_METHOD_FUNC(rb_dfmemory_vec32_set), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_insert", RUBY_METHOD_FUNC(rb_dfmemory_vec32_insert), 3);
    rb_define_singleton_method(rb_cDFHack, "memory_vector32_delete", RUBY_METHOD_FUNC(rb_dfmemory_vec32_delete), 2);

    // load the default ruby-level definitions
    int state=0;
    rb_load_protect(rb_str_new2("./hack/ruby.rb"), Qfalse, &state);
    if (state)
        dump_rb_error();
}
