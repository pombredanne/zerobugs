// -------------------------------------------------------------------------
// This file is part of ZeroBugs, Copyright (c) 2010 Cristian L. Vlasceanu
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// -------------------------------------------------------------------------
//
// $Id$
//
#include "addr_operations.h"


Mutex& Dwarf::get_operations_mutex()
{
    static Mutex mx;
    return mx;
}


Mutex* _init_mutex = &Dwarf::get_operations_mutex();


// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
