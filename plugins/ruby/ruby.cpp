// blindly copied imports from fastdwarf
#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"

#include "DataDefs.h"
#include "df/world.h"
#include "df/unit.h"

#include "tinythread.h"

#include <ruby.h>

using std::string;
using std::vector;
using namespace DFHack;


static void df_rubythread(void*);
static command_result df_rubyload(color_ostream &out, vector <string> & parameters);
static command_result df_rubyeval(color_ostream &out, vector <string> & parameters);
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

// dfhack interface
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

static command_result df_rubyload(color_ostream &out, vector <string> & parameters)
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

static command_result df_rubyeval(color_ostream &out, vector <string> & parameters)
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



// ruby classes
static VALUE rb_cDFHack;
static VALUE rb_c_WrapData;


// DFHack methods
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

/*
static VALUE rb_dfgetversion(VALUE self)
{
    return rb_str_new2(getcore().vinfo->getVersion().c_str());
}
*/

// TODO color_ostream proxy yadda yadda
static VALUE rb_dfprint_str(VALUE self, VALUE s)
{
    //getcore().con.print("%s", rb_string_value_ptr(&s));
    Core::printerr("%s", rb_string_value_ptr(&s));
    return Qnil;
}

static VALUE rb_dfprint_err(VALUE self, VALUE s)
{
    Core::printerr("%s", rb_string_value_ptr(&s));
    return Qnil;
}

// raw memory access
// WARNING: may cause game crash ! double-check your addresses !
static VALUE rb_dfmemread(VALUE self, VALUE addr, VALUE len)
{
    return rb_str_new((char*)rb_num2ulong(addr), rb_num2ulong(len));
}

static VALUE rb_dfmemwrite(VALUE self, VALUE addr, VALUE raw)
{
    // no stable api for raw.length between rb1.8/rb1.9 ...
    int strlen = FIX2INT(rb_funcall(raw, rb_intern("length"), 0));

    memcpy((void*)rb_num2ulong(addr), rb_string_value_ptr(&raw), strlen);

    return Qtrue;
}

static VALUE rb_dfmalloc(VALUE self, VALUE len)
{
    return rb_uint2inum((long)malloc(FIX2INT(len)));
}

static VALUE rb_dffree(VALUE self, VALUE ptr)
{
    free((void*)rb_num2ulong(ptr));
    return Qtrue;
}

// raw c++ wrappers
// return the nth element of a vector
static VALUE rb_dfvectorat(VALUE self, VALUE vect_addr, VALUE idx)
{
    vector<uint32_t> *v = (vector<uint32_t>*)rb_num2ulong(vect_addr);
    return rb_uint2inum(v->at(FIX2INT(idx)));
}

// return a c++ string as a ruby string (nul-terminated)
static VALUE rb_dfreadstring(VALUE self, VALUE str_addr)
{
    string *s = (string*)rb_num2ulong(str_addr);
    return rb_str_new2(s->c_str());
}




/* XXX this needs a custom DFHack::Plugin subclass to pass the cmdname to invoke(), to match the ruby callback
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


// return the address of the struct in DF memory (for raw memread/write)
static VALUE rb_memaddr(VALUE self)
{
    void *data;
    Data_Get_Struct(self, void, data);

    return rb_uint2inum((uint32_t)data);
}




// BEGIN GENERATED SECTION

// begin generated T_cursor binding
static VALUE rb_c_T_cursor;

static VALUE rb_m_T_cursor_x(VALUE self) {
    struct df::global::T_cursor *var;
    Data_Get_Struct(self, struct df::global::T_cursor, var);
    return rb_uint2inum(var->x);
}
static VALUE rb_m_T_cursor_x_set(VALUE self, VALUE val) {
    struct df::global::T_cursor *var;
    Data_Get_Struct(self, struct df::global::T_cursor, var);
    var->x = rb_num2ulong(val);
    return Qtrue;
}

static VALUE rb_m_T_cursor_y(VALUE self) {
    struct df::global::T_cursor *var;
    Data_Get_Struct(self, struct df::global::T_cursor, var);
    return rb_uint2inum(var->y);
}
static VALUE rb_m_T_cursor_y_set(VALUE self, VALUE val) {
    struct df::global::T_cursor *var;
    Data_Get_Struct(self, struct df::global::T_cursor, var);
    var->y = rb_num2ulong(val);
    return Qtrue;
}

static VALUE rb_m_T_cursor_z(VALUE self) {
    struct df::global::T_cursor *var;
    Data_Get_Struct(self, struct df::global::T_cursor, var);
    return rb_uint2inum(var->z);
}
static VALUE rb_m_T_cursor_z_set(VALUE self, VALUE val) {
    struct df::global::T_cursor *var;
    Data_Get_Struct(self, struct df::global::T_cursor, var);
    var->z = rb_num2ulong(val);
    return Qtrue;
}

// link methods to the class
static void ruby_bind_T_cursor(void) {
    // create a class, child of WrapData, in module DFHack
    rb_c_T_cursor = rb_define_class_under(rb_cDFHack, "T_cursor", rb_c_WrapData);

    // reader for 'x' (0 = no arg)
    rb_define_method(rb_c_T_cursor, "x",  RUBY_METHOD_FUNC(rb_m_T_cursor_x), 0);
    // writer for 'x' (1 arg)
    rb_define_method(rb_c_T_cursor, "x=", RUBY_METHOD_FUNC(rb_m_T_cursor_x_set), 1);
    rb_define_method(rb_c_T_cursor, "y",  RUBY_METHOD_FUNC(rb_m_T_cursor_y), 0);
    rb_define_method(rb_c_T_cursor, "y=", RUBY_METHOD_FUNC(rb_m_T_cursor_y_set), 1);
    rb_define_method(rb_c_T_cursor, "z",  RUBY_METHOD_FUNC(rb_m_T_cursor_z), 0);
    rb_define_method(rb_c_T_cursor, "z=", RUBY_METHOD_FUNC(rb_m_T_cursor_z_set), 1);
}


// create an instance of T_cursor from global::cursor
// this method is linked to DFHack.cursor in ruby_bind_dfhack()
static VALUE rb_global_cursor(VALUE self) {
    return Data_Wrap_Struct(rb_c_T_cursor, 0, 0, df::global::cursor);
}


// begin generated unit binding
static VALUE rb_c_unit;

static VALUE rb_m_unit_id(VALUE self) {
    struct df::unit *var;
    Data_Get_Struct(self, struct df::unit, var);

    return rb_uint2inum(var->id);
}
static VALUE rb_m_unit_id_set(VALUE self, VALUE val) {
    struct df::unit *var;
    Data_Get_Struct(self, struct df::unit, var);
    var->id = rb_num2ulong(val);
    return Qtrue;
}

static void ruby_bind_unit(void) {
    // ruby class name must begin with uppercase
    rb_c_unit = rb_define_class_under(rb_cDFHack, "Unit", rb_c_WrapData);

    rb_define_method(rb_c_unit, "id",  RUBY_METHOD_FUNC(rb_m_unit_id), 0);
    rb_define_method(rb_c_unit, "id=", RUBY_METHOD_FUNC(rb_m_unit_id_set), 1);
}


// begin generated world binding
static VALUE rb_c_world;
static VALUE rb_c_world_T_units;

static VALUE rb_m_world_T_units_all(VALUE self) {
    struct df::world::T_units *var;
    Data_Get_Struct(self, struct df::world::T_units, var);

    // read a vector
    VALUE ret = rb_ary_new();
    for (unsigned i=0 ; i<var->all.size() ; ++i)
        rb_ary_push(ret, Data_Wrap_Struct(rb_c_unit, 0, 0, var->all[i]));

    return ret;
}

static VALUE rb_m_world_units(VALUE self) {
    struct df::world *var;
    Data_Get_Struct(self, struct df::world, var);
    return Data_Wrap_Struct(rb_c_world_T_units, 0, 0, &var->units);
}

static void ruby_bind_world(void) {
    rb_c_world = rb_define_class_under(rb_cDFHack, "World", rb_c_WrapData);
    rb_c_world_T_units = rb_define_class_under(rb_c_world, "T_units", rb_c_WrapData);

    rb_define_method(rb_c_world, "units", RUBY_METHOD_FUNC(rb_m_world_units), 0);
}

static VALUE rb_global_world(VALUE self) {
    return Data_Wrap_Struct(rb_c_world, 0, 0, df::global::world);
}

/*
static VALUE rb_dfcreatures(VALUE self)
{
    OffsetGroup *ogc = getcore().vinfo->getGroup("Creatures");
    vector <df_creature*> *v = (vector<df_creature*>*)ogc->getAddress("vector");

    VALUE ret = rb_ary_new();
    for (unsigned i=0 ; i<v->size() ; ++i)
        rb_ary_push(ret, Data_Wrap_Struct(rb_cCreature, 0, 0, v->at(i)));

    return ret;
}

static VALUE rb_getlaborname(VALUE self, VALUE idx)
{
    return rb_str_new2(getcore().vinfo->getLabor(FIX2INT(idx)).c_str());
}

static VALUE rb_getskillname(VALUE self, VALUE idx)
{
    return rb_str_new2(getcore().vinfo->getSkill(FIX2INT(idx)).c_str());
}

static VALUE rb_mapblock(VALUE self, VALUE x, VALUE y, VALUE z)
{
    Maps *map;
    Data_Get_Struct(self, Maps, map);
    df_block *block;

    block = map->getBlock(FIX2INT(x)/16, FIX2INT(y)/16, FIX2INT(z));
    if (!block)
        return Qnil;

    return Data_Wrap_Struct(rb_cMapBlock, 0, 0, block);
}
*/




// define module DFHack and its methods
static void ruby_bind_dfhack(void) {
    rb_cDFHack = rb_define_module("DFHack");

    // global DFHack commands
    rb_define_singleton_method(rb_cDFHack, "onupdate_active", RUBY_METHOD_FUNC(rb_dfonupdateactive), 0);
    rb_define_singleton_method(rb_cDFHack, "onupdate_active=", RUBY_METHOD_FUNC(rb_dfonupdateactiveset), 1);
    rb_define_singleton_method(rb_cDFHack, "resume", RUBY_METHOD_FUNC(rb_dfresume), 0);
    rb_define_singleton_method(rb_cDFHack, "do_suspend", RUBY_METHOD_FUNC(rb_dfsuspend), 0);
    rb_define_singleton_method(rb_cDFHack, "resume", RUBY_METHOD_FUNC(rb_dfresume), 0);
    //rb_define_singleton_method(rb_cDFHack, "version", RUBY_METHOD_FUNC(rb_dfgetversion), 0);
    rb_define_singleton_method(rb_cDFHack, "print_str", RUBY_METHOD_FUNC(rb_dfprint_str), 1);
    rb_define_singleton_method(rb_cDFHack, "print_err", RUBY_METHOD_FUNC(rb_dfprint_err), 1);
    rb_define_singleton_method(rb_cDFHack, "memread", RUBY_METHOD_FUNC(rb_dfmemread), 2);
    rb_define_singleton_method(rb_cDFHack, "memwrite", RUBY_METHOD_FUNC(rb_dfmemwrite), 2);
    rb_define_singleton_method(rb_cDFHack, "malloc", RUBY_METHOD_FUNC(rb_dfmalloc), 1);
    rb_define_singleton_method(rb_cDFHack, "free", RUBY_METHOD_FUNC(rb_dffree), 1);
    rb_define_singleton_method(rb_cDFHack, "vectorat", RUBY_METHOD_FUNC(rb_dfvectorat), 2);
    rb_define_singleton_method(rb_cDFHack, "readstring", RUBY_METHOD_FUNC(rb_dfreadstring), 1);
    rb_define_singleton_method(rb_cDFHack, "register_dfcommand", RUBY_METHOD_FUNC(rb_dfregister), 2);

    // accessors for dfhack globals
    rb_define_singleton_method(rb_cDFHack, "cursor", RUBY_METHOD_FUNC(rb_global_cursor), 0);
    rb_define_singleton_method(rb_cDFHack, "world", RUBY_METHOD_FUNC(rb_global_world), 0);

    // parent class for all wrapped objects
    rb_c_WrapData = rb_define_class_under(rb_cDFHack, "WrapData", rb_cObject);
    rb_define_method(rb_c_WrapData, "memaddr", RUBY_METHOD_FUNC(rb_memaddr), 0);

    // call generated bindings
    ruby_bind_T_cursor();
    ruby_bind_unit();
    ruby_bind_world();

    // load the default ruby-level definitions
    int state=0;
    rb_load_protect(rb_str_new2("./hack/plugins/ruby.rb"), Qfalse, &state);
    if (state)
        dump_rb_error();
}
