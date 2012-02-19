//
// -*- tab-width: 4; indent-tabs-mode: nil;  -*-
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
//
// $Id: $
//
#include "code_view.h"
#include "zdk/align.h"
#include "zdk/check_ptr.h"
#include "zdk/zero.h"
#include "command.h"
#include "controller.h"
#include "flcode_table.h"
#include <fstream>
#include <iostream>


using namespace std;
static const size_t ASM_WINDOW_SIZE = 256;


ui::CodeView::CodeView(ui::Controller& controller)
    : ui::View(controller)
{
}


ui::CodeView::~CodeView() throw()
{
}


////////////////////////////////////////////////////////////////
ui::SourceView::SourceView(Controller& controller)
    : ui::CodeView(controller)
    , maxLineWidth_(0)
{
}


ui::SourceView::~SourceView() throw()
{
}


bool ui::SourceView::refresh(

    const RefPtr<Thread>&,
    const RefPtr<Symbol>& sym)
{
    current_ = sym;

    const char* filename = sym->file()->c_str();
    if (filename_ == filename)
    {
        return false;
    }
    read_file(filename);
    return true;
}


void ui::SourceView::read_file(const char* filename)
{
    assert(filename_ != filename);
    filename_ = filename;

    lines_.clear();
    maxLineWidth_ = 0;

    // if source code lines exceed this size, though luck
    vector<char> buf(2048);

    ifstream fin(filename);
    if (fin)
    {
        filename_ = filename;

        while (fin.getline(&buf[0], buf.size()))
        {
            lines_.push_back(&buf[0]);
            if (lines_.back().length() > size_t(maxLineWidth_))
            {
                maxLineWidth_ = lines_.back().length();
            }
        }
    }
}


const char* ui::SourceView::current_file() const
{
    return filename_.c_str();
}


size_t ui::SourceView::current_line() const
{
    return current_ ? current_->line() : 0;
}


////////////////////////////////////////////////////////////////
ui::MultiCodeView::MultiCodeView(ui::Controller& controller)
    : ui::CodeView(controller)
{
}


ui::MultiCodeView::~MultiCodeView() throw()
{
}


void ui::MultiCodeView::update(const ui::State& s)
{
    RefPtr<Symbol> sym;

    RefPtr<Thread> t = s.current_thread();
    if (t)
    {
        if (RefPtr<Frame> f = t->stack_trace()->selection())
        {
            sym = f->function();
        }
    }

    if (sym)
    {
        SharedStringPtr filename = sym->file();

        auto i = views_.find(filename);

        if (i == views_.end())
        {
            CodeViewPtr cv = make_view(*sym);
            i = views_.insert(make_pair(filename, cv)).first;

            Layout::CallbackPtr cb(make_callback());
            assert(cb);

            cv->insert_self(*cb);
        }
        current_ = sym;
        i->second->show(t, sym);
        make_visible(i->second);
    }
}

////////////////////////////////////////////////////////////////
ui::AsmView::AsmView(ui::Controller& c) 
    : CodeView(c)
    , maxLineWidth_(0)
{
}


ui::AsmView::~AsmView() throw()
{
}


void ui::AsmView::add_listing_line(

    addr_t          addr,
    const string&   line )

{
    int lineNum = lines_.size();
    
    lines_.push_back(line);
    set_row_count(lines_.size());

    // build addr <-> line mappings
    addrToLine_[addr] = lineNum;
    lineToAddr_.insert(make_pair(lineNum, addr));
}


const char* ui::AsmView::current_file() const
{
    return current_ ? current_->file()->c_str() : "";
}


size_t ui::AsmView::current_line() const
{
    return current_ ? current_->line() : 0;
}


/**
 * In order to disassemble a portion of the debug target
 * we need to read its memory -- which must happen on the
 * main debugger thread. This thread affinity restriction
 * comes from the fact that only the thread who called the
 * initial PTRACE_ATTACH operation can perform subsequent
 * ptrace calls successfully on that target.
 */
class DisasmCommand : public ui::Command, Disassembler::OutputCallback
{
public:
    DisasmCommand(RefPtr<ui::AsmView> view, RefPtr<Thread> t, RefPtr<Symbol> s)
        : view_(view)
        , thread_(t)
        , start_(s)
        , complete_(false)
    { }

protected:
    ~DisasmCommand() throw()
    { }

    void execute_on_main_thread()
    {
        const size_t nwords = round_to_word(ASM_WINDOW_SIZE);
        buffer_.resize(nwords * sizeof(word_t));
        word_t* wordBuf = reinterpret_cast<word_t*>(&buffer_[0]);
        thread_->read_code(start_->addr(), wordBuf, nwords);

        Debugger* debugger = CHKPTR(thread_->debugger());
        debugger->disassemble(
            thread_.get(), 
            start_.get(),
            buffer_.size(),
            true,   /* include source code in listing */
            &buffer_[0],
            this);
    }

    void continue_on_ui_thread(ui::Controller&)
    {
        complete_ = true;
    }

    bool is_complete() const
    {
        return complete_;
    }
    //
    // Disassembler::OutputCallback interface
    //
    bool notify(addr_t addr, const char* line, size_t lineLength)
    {
        view_->add_listing_line(addr, string(line, lineLength));
        return true;
    }

    bool tabstops(size_t*, size_t*) const
    {
        return true;
    }

private:
    RefPtr<ui::AsmView> view_;
    RefPtr<Thread>      thread_;
    RefPtr<Symbol>      start_;
    vector<uint8_t>     buffer_;
    bool                complete_;
};


bool ui::AsmView::refresh(

    const RefPtr<Thread>&   t,
    const RefPtr<Symbol>&   s )

{
    if (s == current_)
    {
        return false;
    }

    lines_.clear();         // reset listing lines
    addrToLine_.clear();    // reset addr / line mapping
    lineToAddr_.clear();

    maxLineWidth_ = 0;
    current_ = s;

    addr_t addr = s->addr();

    // disassemble before the symbol in view
    // so that the user can scroll up and down
    addr -= ASM_WINDOW_SIZE / 2;

    RefPtr<SymbolMap> symbols = CHKPTR(t)->symbols();
    RefPtr<Symbol> start = CHKPTR(symbols)->lookup_symbol(addr);
    if (start)
    {
        // try and align at beginning of the function
        RefPtr<Symbol> tmp = symbols->lookup_symbol(start->addr() - start->offset());
        if (tmp)
        {
            start = tmp;
        }
    }
    else
    {
        start = s;
    }

    RefPtr<ui::Command> disassemble = new DisasmCommand(this, t, start);
    controller().call_async_on_main_thread(disassemble);

    return true;
}

