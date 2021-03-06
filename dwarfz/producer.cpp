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

#include "private/factory.h"
#include "producer.h"


void Dwarf::register_creator
(
    Dwarf_Half dwarf_tag,
    std::shared_ptr<Die>(*fn)(Dwarf_Debug, Dwarf_Die)
)
{
    Dwarf::Factory::instance().register_creator(dwarf_tag, fn);
}

// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
