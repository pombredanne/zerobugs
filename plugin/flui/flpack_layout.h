#ifndef FLPACK_LAYOUT_H__C9EE8D01_7D07_4F48_9578_2355F6269D67
#define FLPACK_LAYOUT_H__C9EE8D01_7D07_4F48_9578_2355F6269D67
//
// -*- tab-width: 4; indent-tabs-mode: nil;  -*-
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
//
// $Id: flpack_layout.h 67464 2012-01-28 04:14:52Z unknown $
//
#include "code_view.h"

class Fl_Group;
class Fl_Input;
class Fl_Output;
class FlSplitter;


class FlPackLayout : public ui::Layout
{
    ~FlPackLayout() throw();

public:
    FlPackLayout(ui::Controller&, int x, int y, int w, int h);

    // View interface
    virtual void update(const ui::State&);

    // Layout interface
    virtual void show(ui::View&, bool);

protected:
    int code_height() const;

    CallbackPtr make_callback(ui::ViewType);

    void resize(int x, int y, int w, int h);

    void status_message(const std::string&);

private:
    void init_status_bar(int x, int y, int w, int h);
    void update_status(const ui::State&);

    FlSplitter* group_; // this group
    Fl_Group*   code_;
    Fl_Group*   bottomL_;
    Fl_Group*   bottomR_;
    Fl_Group*   right_;
    Fl_Output*  status_;
};


#endif // FLPACK_LAYOUT_H__C9EE8D01_7D07_4F48_9578_2355F6269D67

