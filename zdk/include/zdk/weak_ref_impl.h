#ifndef WEAK_REF_IMPL_H__C4178524_195B_47E8_8102_C31BEFA8ADD2
#define WEAK_REF_IMPL_H__C4178524_195B_47E8_8102_C31BEFA8ADD2
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
//
#include <cassert>
#include <memory>
#include <boost/type_traits.hpp>
#include "generic/lock.h"
#include "zdk/atomic.h"
#include "zdk/mutex.h"
#include "zdk/weak_ref.h"

// allocate WeakRef objects as needed
#define WEAK_REF_LAZY 1

template<typename T> class WeakPtr;

inline void ZDK_LOCAL weak_ref_debug() // breakpoint hook
{
}


//
// NOTE: For now this class should NOT be ZDK_LOCAL, since it has
// static members (noref and nil) that must be shared. I have tried
// making those explicitly ZDK_EXPORT and the parent class ZDK_LOCAL
// but the code as generated by G++ 4.2.1 (Suse Linux) crashes with a SEGV.
//
template<typename T>
class SupportsWeakRef : public T
{
    SupportsWeakRef(const SupportsWeakRef&); // non-copyable
    SupportsWeakRef& operator=(const SupportsWeakRef&); // non-assignable

    template<typename U>
    static T* cast_(U* ptr)
    {
        typedef SupportsWeakRef<T> my_type;
        typedef typename boost::remove_cv<U>::type strip_type;

        return static_cast<my_type*>(const_cast<strip_type*>(ptr));
    }

    /***********************************************************
     * This is an indirect reference to T. A pointer to T is
     * obtained by calling get_ptr(). The WeakRefImpl object
     * itself is reference-counted.
     * Here is how it all works: an object that supports weak
     * referencing owns an instance of a WeakRefImpl. Weak pointers
     * to that object share the ref-counted WeakRefImpl instance.
     * The object's destructor calls WeakRef::clear(), setting ptr_
     * to NULL. All outstanding weak pointers to the object will now
     * also return NULL when dereferenced. When all outstanding weak
     * ptrs go out of scope, the reference count of the WeakRefImpl
     * instance goes down to zero and the WeakRefImpl is deleted.
     */
    class WeakRefImpl : public WeakRef
    {
        T* ptr_;
        mutable Mutex mutex_;

    protected:
        atomic_t count_;

    public:
        template<typename U>
        explicit WeakRefImpl(U* ptr) : ptr_(cast_(ptr)), count_(1)
        {
        }
        virtual ~WeakRefImpl() throw()
        {
        }
        void clear()
        {
            Lock<Mutex> lock(mutex_);
            ptr_ = NULL;
            release();
        }
        virtual long count() const volatile
        {
            return atomic_read(count_);
        }
        virtual void add_ref()
        {
            atomic_inc(count_);
        }
        virtual struct Unknown* get_ptr() const volatile
        {
            Lock<Mutex> lock(mutex_);
            if (ptr_)
            {
                if (ptr_->ref_count() == 0) // about to be deleted
                {
                    return NULL;
                }
                static_cast<SupportsWeakRef<T>*>(ptr_)->inc_ref();
            }
            return ptr_;
        }
        virtual void release()
        {
            if (atomic_dec_and_test(count_))
            {
                delete this;
            }
        }
        virtual void enter() { add_ref(); mutex_.enter(); }
        virtual void leave() { mutex_.leave(); release(); }

        T* get() const { return ptr_; }
    }; // WeakRefImpl
    /**********************************************************/

    class NullRefImpl : public WeakRefImpl
    {
    public:
        NullRefImpl() : WeakRefImpl((T*)0) { }

        void release() { }
        void add_ref() { }
    };

private:
    mutable WeakRefImpl* ref_;

protected:
    virtual ~SupportsWeakRef() throw()
    {
        if (ref_)
        {
            ref_->clear();
        }
    }
#ifdef WEAK_REF_LAZY
    static ZDK_EXPORT WeakRefImpl* nil;
    static ZDK_EXPORT WeakRefImpl* noref;

    //
    // Do not allocate a WeakRefImpl object until it is needed
    //
    SupportsWeakRef() : ref_(0) { }

    WeakRef* weak_ref() const volatile
    {
        ::weak_ref_debug();

        if (compare_and_swap(&ref_, nil, nil))
        {
            std::auto_ptr<WeakRefImpl> ref(new WeakRefImpl(this));
            Lock<WeakRefImpl> lock(*ref);
            if (compare_and_swap(&ref_, nil, ref.get()))
            {
                ref.release(); // auto_ptr yields ownership
            }
        }
        assert(ref_);
        return ref_;
    }

#else
    SupportsWeakRef() : ref_(new WeakRefImpl(this))
    { }

    WeakRef* weak_ref() const volatile
    {
        return ref_;
    }
#endif

public:
    void release()
    {
        Lock<WeakRefImpl> lock1(*noref);

        //if (!ref_) { ref_ = noref; }
        compare_and_swap(&ref_, nil, noref);

        Lock<WeakRefImpl> lock2(*ref_);

        if (this->dec_ref_and_test())
        {
            delete this;
        }
        //else if (ref_ == noref) { ref_ = 0; }
        else compare_and_swap(&ref_, noref, nil);
    }
};


/**
 * Base for classes which do not support weak referencing
 */
template<typename T>
class NoWeakRef : public T
{
    WeakRef* weak_ref() const volatile
    {
        assert(false);
        return 0;
    }

public:
    enum { supports_weak_ref = false };

protected:
    void release()
    {
        if (this->dec_ref_and_test())
        {
            delete this;
        }
    }
};


#ifdef WEAK_REF_LAZY
 template<typename T>
 typename SupportsWeakRef<T>::WeakRefImpl* SupportsWeakRef<T>::nil = 0;

 template<typename T>
 typename SupportsWeakRef<T>::WeakRefImpl* SupportsWeakRef<T>::noref =
    new typename SupportsWeakRef<T>::NullRefImpl();

#endif
#endif // WEAK_REF_IMPL_H__C4178524_195B_47E8_8102_C31BEFA8ADD2
// vim: tabstop=4:softtabstop=4:expandtab:shiftwidth=4
