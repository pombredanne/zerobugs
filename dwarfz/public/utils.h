#ifndef UTILS_H__6A0BEF97_4723_4D2E_BC92_3F7B61E7F4CE
#define UTILS_H__6A0BEF97_4723_4D2E_BC92_3F7B61E7F4CE
//
// $Id: utils.h 714 2010-10-17 10:03:52Z root $
//
// -------------------------------------------------------------------------
// This file is part of ZeroBugs, Copyright (c) 2010 Cristian L. Vlasceanu
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// -------------------------------------------------------------------------

#include <libelf.h>
#include <libdwarf.h>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include "funfwd.h"
#include "interface.h"

namespace Dwarf
{
    class Debug;
    class Die;
    class Location;
    class Type;

    CLASS Utils
    {
        template<typename T> friend class IterationTraits;

    public:
        template<typename T, Dwarf_Unsigned Type>
        CLASS AutoDealloc : private boost::noncopyable
        {
            Dwarf_Debug dbg_;
            T* ptr_;

        public:
            AutoDealloc(Dwarf_Debug dbg, T* ptr) : dbg_(dbg), ptr_(ptr)
            { }

            ~AutoDealloc() throw()
            {
                if (ptr_) dwarf_dealloc(dbg_, ptr_, Type);
            }
        };

        /**
         * Get the TAG of the given die
         */
        static Dwarf_Half tag(Dwarf_Debug, Dwarf_Die);
        static Dwarf_Half tag(Dwarf_Debug, Dwarf_Die, std::nothrow_t) throw();

        /* Get the die describing the type of the given
           die; works for dice of type DW_TAG_member, DW_TAG_variable,
           DW_TAG_formal_parameter, DW_TAG_constant, etc */
        static boost::shared_ptr<Type> type(Dwarf_Debug, Dwarf_Die);

        static boost::shared_ptr<Type> type(const Dwarf::Die&);

        static boost::shared_ptr<Type> containing_type(Dwarf_Debug, Dwarf_Die);

        /* Get the location, for a die that has a DW_AT_location
           attribute */
        static boost::shared_ptr<Location> loc(Dwarf_Debug, Dwarf_Die);

        static boost::shared_ptr<Location> loc(
            Dwarf_Debug, Dwarf_Die, Dwarf_Half loc_attr);

        static bool has_attr(Dwarf_Debug, Dwarf_Die, Dwarf_Half);

        static Dwarf_Unsigned byte_size(Dwarf_Debug, Dwarf_Die);
        static Dwarf_Unsigned bit_size(Dwarf_Debug, Dwarf_Die);
        static Dwarf_Off bit_offset(Dwarf_Debug, Dwarf_Die);

        // static Dwarf_Off offset(Dwarf_Debug, Dwarf_Die);

        static Dwarf_Die get_parent(Dwarf_Debug, Dwarf_Die);

        //static Dwarf_Off get_cu_offset(Dwarf_Debug, Dwarf_Die);
        //static Dwarf_Off get_cu_offset(Dwarf_Die);

        static bool has_child(const Die&, Dwarf_Half);

        static Dwarf_Die first_child(
            Dwarf_Debug,
            Dwarf_Die,
            const Dwarf_Half*,
            size_t,
            Dwarf_Half* retTag);

        static Dwarf_Die next_sibling(
            Dwarf_Debug,
            Dwarf_Die,
            const Dwarf_Half*,
            size_t,
            Dwarf_Half* retTag);

        static void dump_attributes(const Dwarf::Die&, std::ostream& outs);

        static void dump_children(const Dwarf::Die&, std::ostream& outs);

        static boost::shared_ptr<Function>
            lookup_function(const FunList& funcs,
                            Dwarf_Addr addr,
                            const char* linkage = NULL);

    private:

        static Dwarf_Die first_child(Dwarf_Debug, Dwarf_Die, Dwarf_Half);
        static Dwarf_Die next_sibling(Dwarf_Debug, Dwarf_Die, Dwarf_Half);
    };
}
#endif // UTILS_H__6A0BEF97_4723_4D2E_BC92_3F7B61E7F4CE
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
