//
// -*- tab-width: 4; indent-tabs-mode: nil;  -*-
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
//
// $Id$
//
#include "flmenu.h"
#include <iostream>
#include <typeinfo>
#include <FL/Fl_Window.H>


FlMenuBar::FlMenuBar(

    ui::Controller& c,
    Fl_Window*      w

) : ui::CompositeMenu()
  , controller_(c)
  , menu_(new Fl_Menu_Bar(0, 0, w->w(), 30))
{
}


FlMenuBar::~FlMenuBar() throw()
{
}


void FlMenuBar::add(RefPtr<ui::Menu> menu)
{
    assert(menu);
    ui::CompositeMenu::add(menu);

    menu_->add(menu->name().c_str(), menu->shortcut(), exec_command, this);
}


void FlMenuBar::exec_command(Fl_Widget* w, void* p)
{
    FlMenuBar* menubar = reinterpret_cast<FlMenuBar*>(p);

    char path[100] = { 0 };
    assert(menubar->menu_);

    menubar->menu_->item_pathname(path, sizeof(path) - 1);
    menubar->exec_command(path);
}


void FlMenuBar::exec_command(const char* path)
{
    // find the menu item that issued the command
    for (auto mi = begin(children_); mi != end(children_); ++mi)
    {
        if (strcmp((*mi)->name().c_str(), path) == 0)
        {
            controller_.call_async_on_main_thread((*mi)->emit_command());
            break;
        }
    }
}

