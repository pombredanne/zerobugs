//
// $Id$
//
// -------------------------------------------------------------------------
// This file is part of ZeroBugs, Copyright (c) 2010 Cristian L. Vlasceanu
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// -------------------------------------------------------------------------

#include "zdk/stdexcept.h"
#include <assert.h>
#include <iostream>
#include <stack>
#include <dwarf.h>
#include <pthread.h>
#include "private/log.h"
#include "public/addr_ops.h"
#include "public/abi.h"
#include "public/error.h"
#include "public/impl.h"
#include "public/location.h"
#include "public/unwind.h"
#include "generic/lock.h"


using namespace std;
using namespace Dwarf;
using namespace Dwarf::ABI;


namespace
{
    /**
     * Stack for evaluating location operations
     */
    CLASS Stack : public stack<Dwarf_Addr>
    {
    public:
        void push(Dwarf_Addr addr)
        {
            stack<Dwarf_Addr>::push(addr);
            LOG_DEBUG(2) << "PUSH " << hex << addr << dec << endl;
        }

        void pop()
        {
            stack<Dwarf_Addr>::pop();
            LOG_DEBUG(2) << "POP" << endl;
        }
    };
}

static pthread_key_t addrOpsKey;

static void delete_key(void*)
{
    LOG_DEBUG(0) << __func__ << endl;
    pthread_key_delete(addrOpsKey);
}

namespace Dwarf
{
    int init = pthread_key_create(&addrOpsKey, delete_key);
}

/**
 * note: the client must ensure thread-safety
 */
AddrOps* Dwarf::set_addr_operations(AddrOps* ops)
{
    AddrOps* prev = (AddrOps*)pthread_getspecific(addrOpsKey);
    pthread_setspecific(addrOpsKey, ops);
    return prev;
}


AddrOps* Dwarf::get_addr_operations()
{
    AddrOps* addrOps = (AddrOps*)pthread_getspecific(addrOpsKey);
    assert(addrOps);

    return addrOps;
}


Location::Location(Dwarf_Debug dbg, Dwarf_Attribute attr)
    : dbg_(dbg), list_(0), size_(0), isValue_(false)
{
    assert(dbg);
    Dwarf_Error err = 0;

    if (dwarf_loclist_n(attr, &list_, &size_, &err) == DW_DLV_ERROR)
    {
        THROW_ERROR(dbg, err);
    }
}


Location::~Location()
{
    if (list_)
    {
        for (Dwarf_Signed i(0); i != size_; ++i)
        {
            dwarf_dealloc(dbg_, list_[i]->ld_s, DW_DLA_LOC_BLOCK);
            dwarf_dealloc(dbg_, list_[i], DW_DLA_LOCDESC);
        }
        dwarf_dealloc(dbg_, list_, DW_DLA_LIST);
    }
}


bool Location::is_register(Dwarf_Addr pc, Dwarf_Addr base) const
{
    bool result = false;
    if (list_)
    {
        assert(size_  > 0);

        for (Dwarf_Signed j = 0; (j < size_) && !result; ++j)
        {
            const Dwarf_Locdesc* desc = list_[j];
            if (pc < desc->ld_lopc + base)
            {
                continue;
            }
            // -1 means unlimited to the right
            if ((desc->ld_hipc != Dwarf_Addr(-1)) && (pc >= desc->ld_hipc + base))
            {
                continue;
            }
            if (desc->ld_cents == 1)
            {
                const Dwarf_Loc& loc = desc->ld_s[0];

                if (loc.lr_atom == DW_OP_regx)
                {
                    result = true;
                }
                else if (loc.lr_atom >= DW_OP_reg0 && loc.lr_atom < DW_OP_reg31)
                {
                    result = true;
                }
            }
        }
    }
    return result;
}


Dwarf_Addr Location::eval(Dwarf_Addr frame,
                          Dwarf_Addr moduleBase,
                          Dwarf_Addr unitBase,
                          Dwarf_Addr pc) const
{
    LOG_DEBUG(2) << __func__ << " unit base=" << hex << unitBase << dec << endl;

    Dwarf_Addr result = 0;
    isValue_ = false;

    if (list_)
    {
        assert(size_ > 0);
        for (Dwarf_Signed i = 0; i != size_; ++i)
        {
            const Dwarf_Locdesc* loc = list_[i];
            Dwarf_Addr lopc = loc->ld_lopc;
            if (loc->ld_lopc)
            {
                lopc += moduleBase + unitBase;
            }
            Dwarf_Addr hipc = loc->ld_hipc;
            if (hipc != static_cast<Dwarf_Addr>(-1))
            {
                hipc += moduleBase + unitBase;
            }

            LOG_DEBUG(2) << "loc[" << i << "]=" << hex << lopc /* loc->ld_lopc */
                         << "-" << hipc << " pc=" << pc << dec << endl;

            if ((pc >= lopc) && (pc < hipc))
            {
                LOG_DEBUG(2) << "evaluating loc[" << i << "]" << endl;
                result = eval(dbg_, pc, frame, moduleBase, loc, isValue_);

                LOG_DEBUG(2) << __func__ << "=" << hex << result << dec << endl;
                break;
            }
        }
    }
    return result;
}


static void handle_empty_stack(const string& func)
{
    LOG_WARN << func << ": stack empty" << endl;

    if (getenv("ZERO_DWARF_ABORT_EMPTY_STACK"))
    {
        abort();
    }
    throw runtime_error(func + ": state machine stack empty");
}


/**
 * The DW_OP_deref operation pops the top stack entry and
 * treats it as an address. The value retrieved from that
 * address is pushed
 */
static void op_deref(Stack& stack)
{
    AddrOps* addrOps = get_addr_operations();
    if (!addrOps)
    {
        throw runtime_error("op_deref: memory operations not set");
    }
    else
    {
        if (stack.empty())
        {
            handle_empty_stack(__func__);
        }
        else
        {
            LOG_DEBUG(2) << "deref: " << hex << stack.top() << dec << endl;
            Dwarf_Addr addr = addrOps->read_mem(stack.top());

            stack.pop();
            stack.push(addr);
        }
    }
}


/**
 * The DW_OP_plus_uconst operation pops the top stack entry, adds
 * it to the unsigned LEB128 constant operand and pushes the result
 */
static void op_plus_uconst(Stack& stack, Dwarf_Unsigned operand)
{
    if (stack.empty())
    {
        handle_empty_stack(__func__);
    }
    else
    {
        Dwarf_Addr addr = stack.top();
        LOG_DEBUG(2) << hex << addr << "+=" << operand << dec << endl;

        addr += operand;

        stack.pop();
        stack.push(addr);
    }
}


/**
 * The DW_OP_minus operations pops the top two stack values,
 * subtracts the former top of the stack from the former second
 * entry, and pushes the result.
 */
static void op_minus(Stack& stack)
{
    if (stack.size() < 2)
    {
        LOG_ERROR << __func__ << ": not enough operands" << endl;
    }
    else
    {
        Dwarf_Addr op2 = stack.top();
        stack.pop();

        Dwarf_Addr op1 = stack.top();
        stack.pop();

        const Dwarf_Addr res = op1 - op2;

        LOG_DEBUG(2) << hex << op1 << '-' << op2 << '=' << res << dec << endl;
        stack.push(res);
    }
}


static void op_plus(Stack& stack)
{
    if (stack.size() < 2)
    {
        LOG_ERROR << __func__ << ": not enough operands" << endl;
    }
    else
    {
        Dwarf_Addr op2 = stack.top();
        stack.pop();

        Dwarf_Addr op1 = stack.top();
        stack.pop();

        Dwarf_Addr result = op1 + op2;

        LOG_DEBUG(2) << hex << op1 << "+" << op2
                     << "=" << result << dec << endl;

        stack.push(result);
    }
}


/**
 * Evaluate an address using a simple stack machine
 */
Dwarf_Addr
Location::eval(Dwarf_Debug dbg,
               Dwarf_Addr pc,
               Dwarf_Addr frameBase,
               Dwarf_Addr moduleBase,
               const Dwarf_Locdesc* desc,
               bool& isValue)
{
    AddrOps* addrOps = get_addr_operations();
    if (!addrOps)
    {
        throw runtime_error("eval: memory operations not set");
    }
    assert(desc);
    Stack stack;

    LOG_DEBUG(2) << "moduleBase=" << hex << moduleBase << dec << endl;
    if (frameBase)
    {
        LOG_DEBUG(2) << "frameBase=" << hex << frameBase << dec << endl;

        stack.push(frameBase);
    }

    for (size_t i(0); i != desc->ld_cents; ++i)
    {
        const Dwarf_Loc& loc = desc->ld_s[i];
        const int atom = loc.lr_atom;

        LOG_DEBUG(2) << "opcode=0x" << hex << atom << dec << endl;

        switch (atom)
        {
        case DW_OP_addr:
            LOG_DEBUG(2) << "addr " << hex << loc.lr_number
                         << " + " << moduleBase << dec << endl;

            // adjust the operand to the address where
            // the module is loaded in memory

            stack.push(loc.lr_number + moduleBase);
            break;

        case DW_OP_lit0:
        case DW_OP_lit1:
        case DW_OP_lit2:
        case DW_OP_lit3:
        case DW_OP_lit4:
        case DW_OP_lit5:
        case DW_OP_lit6:
        case DW_OP_lit7:
        case DW_OP_lit8:
        case DW_OP_lit9:
        case DW_OP_lit10:
        case DW_OP_lit11:
        case DW_OP_lit12:
        case DW_OP_lit13:
        case DW_OP_lit14:
        case DW_OP_lit15:
        case DW_OP_lit16:
        case DW_OP_lit17:
        case DW_OP_lit18:
        case DW_OP_lit19:
        case DW_OP_lit20:
        case DW_OP_lit21:
        case DW_OP_lit22:
        case DW_OP_lit23:
        case DW_OP_lit24:
        case DW_OP_lit25:
        case DW_OP_lit26:
        case DW_OP_lit27:
        case DW_OP_lit28:
        case DW_OP_lit29:
        case DW_OP_lit30:
        case DW_OP_lit31:
            LOG_DEBUG(2) << "literal: " << atom - DW_OP_lit0 << endl;
            //
            // note: assume that literals from 0 thru 31 are
            // defined in contiguous, increasing order
            //
            stack.push(atom - DW_OP_lit0);
            break;

        case DW_OP_deref:
            op_deref(stack);
            break;

        case DW_OP_dup:
            LOG_DEBUG(2) << "dup" << endl;
            //
            // DW_OP_dup duplicates the value at the top of the stack
            //
            if (stack.empty())
            {
                handle_empty_stack("DW_OP_dup");
            }
            else
            {
                stack.push(stack.top());
            }
            break;

        case DW_OP_reg0:
        case DW_OP_reg1:
        case DW_OP_reg2:
        case DW_OP_reg3:
        case DW_OP_reg4:
        case DW_OP_reg5:
        case DW_OP_reg6:
        case DW_OP_reg7:
        case DW_OP_reg8:
        case DW_OP_reg9:
        case DW_OP_reg10:
        case DW_OP_reg11:
        case DW_OP_reg12:
        case DW_OP_reg13:
        case DW_OP_reg14:
        case DW_OP_reg15:
        case DW_OP_reg16:
        case DW_OP_reg17:
        case DW_OP_reg18:
        case DW_OP_reg19:
        case DW_OP_reg20:
        case DW_OP_reg21:
        case DW_OP_reg22:
        case DW_OP_reg23:
        case DW_OP_reg24:
        case DW_OP_reg25:
        case DW_OP_reg26:
        case DW_OP_reg27:
        case DW_OP_reg28:
        case DW_OP_reg29:
        case DW_OP_reg30:
        case DW_OP_reg31:
            {
                const size_t nreg = atom - DW_OP_reg0;
                const int ureg = user_reg_index(nreg);
                LOG_DEBUG(2) << "read_cpu_reg " << nreg << " " << ureg
                             << " (" << user_reg_name(nreg) << ")" << endl;

                const Dwarf_Addr addr = addrOps->read_cpu_reg(ureg);
                LOG_DEBUG(2) << "read_cpu_reg=" << hex << addr << dec << endl;
                stack.push(addr);
            }
            break;

        case DW_OP_regx:
            {
                const int ureg = user_reg_index(loc.lr_number);
                const Dwarf_Addr addr = addrOps->read_cpu_reg(ureg);

                LOG_DEBUG(2) << "read_cpu_regx=" << hex << addr << dec << endl;
                stack.push(addr);
            }
            break;

        case DW_OP_fbreg:
            //
            // The DW_OP_fbreg operation provides a signed offset from the
            // address specified by the location description in the
            // DW_AT_frame_base attribute of the current function.
            //
            LOG_DEBUG(2) << "DW_OP_fbreg: " << hex << loc.lr_number << dec << endl;

            stack.push(frameBase + loc.lr_number);
            break;

        case DW_OP_plus_uconst:
            op_plus_uconst(stack, loc.lr_number);
            break;

        case DW_OP_minus: op_minus(stack); break;
        case DW_OP_plus: op_plus(stack); break;

        case DW_OP_breg0:
        case DW_OP_breg1:
        case DW_OP_breg2:
        case DW_OP_breg3:
        case DW_OP_breg4:
        case DW_OP_breg5:
        case DW_OP_breg6:
        case DW_OP_breg7:
        case DW_OP_breg8:
        case DW_OP_breg9:
        case DW_OP_breg10:
        case DW_OP_breg11:
        case DW_OP_breg12:
        case DW_OP_breg13:
        case DW_OP_breg14:
        case DW_OP_breg15:
        case DW_OP_breg16:
        case DW_OP_breg17:
        case DW_OP_breg18:
        case DW_OP_breg19:
        case DW_OP_breg20:
        case DW_OP_breg21:
        case DW_OP_breg22:
        case DW_OP_breg23:
        case DW_OP_breg24:
        case DW_OP_breg25:
        case DW_OP_breg26:
        case DW_OP_breg27:
        case DW_OP_breg28:
        case DW_OP_breg29:
        case DW_OP_breg30:
        case DW_OP_breg31:
            {
                const size_t nreg = atom - DW_OP_breg0;
                const int ureg = user_reg_index(nreg);
                LOG_DEBUG(2) << "DW_OP_breg" << nreg << endl;
                LOG_DEBUG(2) << "Read_cpu_reg " << nreg << " " << ureg
                    << " (" << user_reg_name(nreg) << ")" << endl;

                const Dwarf_Addr addr = addrOps->read_cpu_reg(ureg);
                LOG_DEBUG(2) << "Read_cpu_reg=" << hex << addr << dec << endl;

                LOG_DEBUG(2) << "loc.lr_num=" << hex << loc.lr_number << dec << endl;
                stack.push(addr + loc.lr_number);
            }
            break;

        case DW_OP_piece:
            // log it but don't do anything else

            /*  DWARF 4 Standard: 2.6.1.2 Composite Location Descriptions
                "Each piece is described by a composition operation, which 
                "does not compute a value nor store any result on the DWARF stack"*/ 
            LOG_DEBUG(0) << __func__ << ": opcode=0x"
                         << hex << static_cast<int>(loc.lr_atom)
                         << " operand=" << dec << loc.lr_number << endl;

            /* if (loc.lr_number < sizeof (Dwarf_Addr))
            {
                assert(!stack.empty());
                Dwarf_Addr v = stack.top();

                LOG_DEBUG(2) << "stack_top=" << hex << v << dec << endl;
            } */
            break;

        case DW_OP_const1u:
        case DW_OP_const2u:
        case DW_OP_const4u:
        case DW_OP_const8u:
        case DW_OP_constu:
            LOG_DEBUG(2) << "const: " << hex << loc.lr_number << dec << endl;
            stack.push(loc.lr_number);
            break;

        case DW_OP_const1s:
        case DW_OP_const2s:
        case DW_OP_const4s:
        case DW_OP_const8s:
        case DW_OP_consts:
            LOG_DEBUG(2) << "const: " << (Dwarf_Signed)loc.lr_number << endl;
            stack.push(loc.lr_number);
            break;

        case DW_OP_call_frame_cfa:  // DWARF 3f
            {   // temporary stack unwinder for computing
                // the Canonical Frame Address
                Unwind unwind(dbg);
                stack.push(unwind.compute_cfa(pc, *addrOps));
            }
            break;

        case DW_OP_stack_value:     // DWARF 4
            assert(!stack.empty());
            isValue = true;
            break;

        default:
            LOG_WARN << __func__ << ": unhandled opcode=0x"
                     << hex << (int)loc.lr_atom << dec << endl;
            break;
        }
    }
    const Dwarf_Addr res = stack.empty() ? 0 : stack.top();
    LOG_DEBUG(2) << __func__ << "=" << hex << res << dec << endl;
    return res;
}


VTableElemLocation::VTableElemLocation(Dwarf_Debug dbg, Dwarf_Attribute attr)
    : Location(dbg, attr)
{
}


/**
 * GCC emits the index in the vtable.
 * Other compilers emit the absolute address in the vtable; in the latter
 * case, adapt the result to a GCC-like, index in the vtable. Assume that
 * 'base' is where the object begins, and that is where the .vptr
 */
Dwarf_Addr
VTableElemLocation::eval( Dwarf_Addr base,
                          Dwarf_Addr modBase,
                          Dwarf_Addr unitBase,
                          Dwarf_Addr pc
                        ) const
{
    Dwarf_Addr addr = Location::eval(base, modBase, unitBase, pc);

    if (addr >= base)
    {
        if (AddrOps* addrOps = get_addr_operations())
        {
            addr = addrOps->read_mem(base);
            addr = Location::eval(addr, modBase, unitBase, pc) - addr;
            addr /= sizeof(void*);
        }
        else
        {
            throw runtime_error("VTableElemLocation: memory operations not set");
        }
    }
    return addr;
}
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4

