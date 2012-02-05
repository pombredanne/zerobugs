#ifndef Flui_H__70E93067_9421_4550_8C81_B35381DC19F6
#define Flui_H__70E93067_9421_4550_8C81_B35381DC19F6
//
// -*- tab-width: 4; indent-tabs-mode: nil;  -*-
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
//
// $Id$
//
#include "zdk/version_info_impl.h"
#include "dharma/sarray.h"
#include "menu.h"

class Fl_Double_Window;
class Fl_Window;


/**
 * Fltk-based user interface plugin.
 */
class Flui : public ui::Controller
    , VersionInfoImpl<ZERO_API_MAJOR, ZERO_API_MINOR>
{
public:
    Flui();

protected:
    virtual ~Flui();

    BEGIN_INTERFACE_MAP(Flui)
        INTERFACE_ENTRY(DebuggerPlugin)
        INTERFACE_ENTRY(VersionInfo)
    END_INTERFACE_MAP()

    int x() const;
    int y() const;
    int w() const;
    int h() const;

    // --- RefCounted
    virtual void release();

    // --- DebuggerPlugin interface
    /**
     * Pass pointer to debugger and the command line params
     * to plug-in module.
     */
    virtual bool initialize(Debugger*, int* argc, char*** argv);

    // -- VersionInfo interface
    const char* description() const;

    const char* copyright() const;

private:
    virtual ui::CodeView*       init_code_view();
    virtual ui::Layout*         init_layout();
    virtual void                init_main_window();
    virtual ui::CompositeMenu*  init_menu();

    virtual void    lock();
    virtual void    unlock();
    virtual void    notify_ui_thread();

    virtual int     wait_for_event();

private:
    SArray              args_;
    Fl_Double_Window*   window_;
};


#endif // Flui_H__70E93067_9421_4550_8C81_B35381DC19F6
