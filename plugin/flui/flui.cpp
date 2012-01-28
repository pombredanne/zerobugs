// -*- tab-width: 4; indent-tabs-mode: nil;  -*-
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4 
//
// $Id$
//
// ZDK headers
#include "zdk/argv_util.h"
#include "zdk/check_ptr.h"

#include "flmenu.h"
#include "flui.h"

// C++ headers
#include <cassert>
#include <iostream>

// fltk headers
#include <FL/Fl.H>
#include <FL/Fl_Window.H>

using namespace std;

// properties
#define WINDOW_X        "flui.window.x"
#define WINDOW_Y        "flui.window.y"
#define WINDOW_W        "flui.window.width"
#define WINDOW_H        "flui.window.height"
#define WINDOW_TITLE    "ZeroBUGS"

static const int default_window_width = 800;
static const int default_window_height = 700;

/**
 * Advertise the interfaces supported by this plugin
 */
void query_plugin(InterfaceRegistry* registry)
{
    registry->update(DebuggerPlugin::_uuid());
}

////////////////////////////////////////////////////////////////
int32_t query_version(int32_t* minor)
{
    if (minor) *minor = ZDK_MINOR;
    return ZDK_MAJOR;
}


////////////////////////////////////////////////////////////////
Plugin* create_plugin(uuidref_t iid)
{
    Plugin* plugin = 0;
   
    if (uuid_equal(DebuggerPlugin::_uuid(), iid))
    {
        plugin = new Flui();
    }
    return plugin;
}


////////////////////////////////////////////////////////////////
Flui::Flui()
    : ui::Controller()
    , window_(nullptr)
{
}


////////////////////////////////////////////////////////////////
Flui::~Flui()
{
    delete window_;
}


////////////////////////////////////////////////////////////////
void Flui::release()
{
    delete this;
}


////////////////////////////////////////////////////////////////
/**
 * Parse command line and other initializing stuff
 */
bool Flui::initialize(

    Debugger*   debugger,
    int*        argc,
    char***     argv)
{
    ui::Controller::initialize( debugger, argc, argv );
    if (argc)
    {
        args_.push_back( (*argv)[0] );
    }

    /*BEGIN_ARG_PARSE(argc, argv)
    ON_ARG("--foo")
        {
            cout << __func__ << ": foon";
        }
    ON_ARG("--bar")
        {
            cout << __func__ << ": barn";
        }
    END_ARG_PARSE */
    return true;
}


////////////////////////////////////////////////////////////////
void Flui::init_main_window()
{
    assert(window_ == nullptr);

    Properties& prop = *debugger()->properties();

    // get coordinates and dimensions from saved properties
    const word_t x = prop.get_word(WINDOW_X, 0);
    const word_t y = prop.get_word(WINDOW_Y, 0);
    const word_t h = prop.get_word(WINDOW_H, default_window_height);
    const word_t w = prop.get_word(WINDOW_W, default_window_width);

    window_ = new Fl_Window(x, y, w, h, WINDOW_TITLE);
    window_->resizable(window_);
}


////////////////////////////////////////////////////////////////
ui::CompositeMenu* Flui::init_menu()
{
    assert(window_);

    return new FlMenuBar(*this, window_);
}


////////////////////////////////////////////////////////////////
ui::Layout* Flui::init_layout()
{
    auto layout = ui::Controller::init_layout();

    window_->end();
    window_->show();

    return layout;
}


////////////////////////////////////////////////////////////////
void Flui::lock()
{
    Fl::lock();
}


////////////////////////////////////////////////////////////////
void Flui::unlock()
{
    Fl::unlock();
}


////////////////////////////////////////////////////////////////
void Flui::notify_ui_thread()
{
    Fl::awake();
}


////////////////////////////////////////////////////////////////
int Flui::wait_for_event()
{
    return Fl::wait();
}


////////////////////////////////////////////////////////////////
const char* Flui::description() const
{
    return "Fltk-based user interface";
}


////////////////////////////////////////////////////////////////
const char* Flui::copyright() const
{
    return "";
}

