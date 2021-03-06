#ifndef DEBUG_REGS_BASE_H__9C75AEFB_E778_43FD_A2A4_1C46E24539B2
#define DEBUG_REGS_BASE_H__9C75AEFB_E778_43FD_A2A4_1C46E24539B2
//
// -*- tab-width: 4; indent-tabs-mode: nil;  -*-
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
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
#include <iosfwd>
#include "zdk/debug_reg.h"
#include "zdk/zerofwd.h"



CLASS DebugRegsBase : public DebugRegs
{
public:
    virtual ~DebugRegsBase() { }

    virtual void dump(std::ostream&) const = 0;

    /**
     * @return the owning thread
     */
    virtual const Thread& thread() const = 0;
};

#endif // DEBUG_REGS_BASE_H__9C75AEFB_E778_43FD_A2A4_1C46E24539B2
