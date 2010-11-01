#ifndef AUTO_H__74E18692_0247_48DA_BF64_02F194B2F3D3
#define AUTO_H__74E18692_0247_48DA_BF64_02F194B2F3D3
//
// $Id: auto.h 714 2010-10-17 10:03:52Z root $
//
// -------------------------------------------------------------------------
// This file is part of ZeroBugs, Copyright (c) 2010 Cristian L. Vlasceanu
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// -------------------------------------------------------------------------

#include <boost/utility.hpp>
#include "generic/export.h"


/// A class that cannot be instantiated on the free store
class ZDK_LOCAL Automatic : private boost::noncopyable
{
protected:
    void* operator new(size_t);
    void operator delete(void*) {}
};
#endif // AUTO_H__74E18692_0247_48DA_BF64_02F194B2F3D3
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
