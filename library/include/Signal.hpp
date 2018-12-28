/**
Copyright © 2018 Pauli <suokkos@gmail.com>

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
   not claim that you wrote the original software. If you use this
   software in a product, an acknowledgment in the product
   documentation would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
   must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
   distribution.
 */

#pragma once

#include <climits>

#include <atomic>
#include <functional>
#include <list>
#include <memory>
#include <mutex>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace DFHack {

/*!
 * Select inline implementation for Signal members
 * This requires careful destruction order where all connection has been
 * disconnected before Signal::~Signal()
 */
class signal_inline_tag;
/*!
 * Select share_ptr managed implementation for Signal members.
 *
 * If Connection holding object may be deleted without full serialization
 * between disconnect and signal emit the holding object must be managed by
 * shared_ptr and derive from ConnectedBase. It will also have to pass the
 * std::shared_ptr<ConnectedBase> to connect.

 * It uses two way std::weak_ptr reference to guarantee destruction of either
 * object doesn't happen when call is made to them.
 *
 * It is still possible to get a callback call after manual disconnect from
 * outside destructor. But without destruction risk the disconnect race can be
 * handled by slot implementation side.
 */
class signal_shared_tag;

/**
 * Used for signal_shared_tag holders that may race with destructor triggered
 * disconnect and emit from Signal.
 */
class ConnectedBase {
};

template<typename Signature, typename tag = signal_inline_tag>
class Signal;

namespace details {

template<typename Signature, typename tag>
struct SignalImpl;

template<typename Signature, typename tag>
struct selectImpl;

//! Manage callback states in thread safe manner
template<typename Signature, typename tag>
class CallbackHolderImpl;

template<typename RT, typename... Args>
struct CallbackHolderBase {
    using Callback = std::function<RT(Args...)>;

    CallbackHolderBase(const Callback& cb) :
        cb_{cb},
        state_{}
    {}

    //! Block the connection
    void block() noexcept
    {
        state_ += blocked;
    }

    //! Unblock the connection
    void unblock() noexcept
    {
        state_ -= blocked;
    }

    //! Check if connection is deleted
    bool erased() const noexcept
    {
        return state_ & deleted;
    }

    //! Check if connection is still active (not blocked or erased)
    operator bool() const noexcept
    {
        return !(state_ & ~inCall);
    }

protected:
    //! Immutable callback object
    const Callback cb_;
    using state_t = unsigned;
    //! Single shared state as a bitfield to simplify synchronization
    //! between state changes.
    std::atomic<state_t> state_;
    static constexpr state_t deleted = 0x1 << (sizeof(state_t)*CHAR_BIT - 1);
    static constexpr state_t inCall = deleted >> (sizeof(state_t)*CHAR_BIT/2);
    static constexpr state_t blocked = 0x1;
    static constexpr state_t blockedMask = inCall - 1;
    static constexpr state_t inCallMask = (deleted - 1) ^ blockedMask;
};

template<typename RT, typename... Args>
class CallbackHolderImpl<RT(Args...), signal_inline_tag> :
        public CallbackHolderBase<RT, Args...> {
    using parent_t = CallbackHolderBase<RT, Args...>;
public:
    using Callback = typename parent_t::Callback;
private:
    using state_t = typename parent_t::state_t;
    //! Make sure callback pointed object doesn't disappear under us
    //! while we call it.
    struct CallGuard {

        //! Prevent copies but allow copy elision
        CallGuard(const CallGuard&);

        //! Allow implicit conversion to callback for simply syntax
        const Callback& operator*() const noexcept
        {
            return holder_->cb_;
        }

        operator bool() const noexcept
        {
            return *holder_;
        }

        //! Mark call not to be called any more
        ~CallGuard() {
            holder_->state_ -= parent_t::inCall;
        }
    private:
        //! Reference to the connection
        CallbackHolderImpl* holder_;

        //! Mark call to be in process
        CallGuard(CallbackHolderImpl* holder) :
            holder_{holder}
        {
            holder_->state_ += parent_t::inCall;
        }
        //! Only allow construction from the CallbackHolderImpl::prepareCall
        friend class CallbackHolderImpl;
    };
public:
    //! Construct the callback state for a callback
    CallbackHolderImpl(const Callback& cb) :
        parent_t{cb}
    {}

    /*!
     * Data race free disconnection for the connection. It spins until
     * no more callers to wait. Spinning should be problem as callbacks
     * are expected to be simple and fast to execute.
     *
     * Must not be called from withing callback!
     *
     * \todo Maybe use monitor instruction to avoid busy wait and call
     *       std::thread::yield() if wait is longer than expected.
     */
    void erase() noexcept
    {
        state_t oldstate;
        state_t newstate;
        /** Spin until no callers to this callback */
spin:
        while ((oldstate = parent_t::state_) & parent_t::inCallMask) {
            // pause would be portable to all old processors but there
            // isn't portable way to generate it without SSE header.
#ifdef __SSE__
            _mm_pause();
#endif
        }
        do {
            if (oldstate & parent_t::inCallMask)
                goto spin;
            newstate = oldstate | parent_t::deleted;
        } while(!parent_t::state_.compare_exchange_weak(oldstate, newstate));
    }

    //! Return RAII CallGuard to protect race between callback and
    //! disconnect.
    CallGuard prepareCall()
    {
        return {this};
    }
};

template<typename RT, typename... Args>
class CallbackHolderImpl<RT(Args...), signal_shared_tag> :
        public CallbackHolderBase<RT, Args...> {
    using parent_t = CallbackHolderBase<RT, Args...>;
public:
    using Callback = typename parent_t::Callback;
private:
    using state_t = typename parent_t::state_t;
    //! Make sure callback pointed object doesn't disappear under us
    //! while we call it.
    struct CallGuard {

        //! Prevent copies but allow copy elision
        CallGuard(const CallGuard&);

        //! Allow implicit conversion to callback for simply syntax
        const Callback& operator*() const noexcept
        {
            return holder_->cb_;
        }

        operator bool() const noexcept
        {
            // If this is not marked erased then weak_ref->lock succeeded or
            // the slot isn't managed by shared_ptr<ConnectedBase>
            return *holder_;
        }

    private:
        //! Reference to the connection
        CallbackHolderImpl* holder_;
        std::shared_ptr<ConnectedBase> strong_ref_;

        //! Mark call to be in process
        CallGuard(CallbackHolderImpl* holder) :
            holder_{holder},
            strong_ref_{holder->weak_ref_.lock()}
        {
        }
        //! Only allow construction from the CallbackHolderImpl::prepareCall
        friend class CallbackHolderImpl;
    };

    std::weak_ptr<ConnectedBase> weak_ref_;
    friend CallGuard;
public:
    //! Construct the callback state for an automatically synchronized object
    CallbackHolderImpl(const Callback& cb,
            std::shared_ptr<ConnectedBase>& ref) :
        parent_t{cb},
        weak_ref_{ref}
    {}

    //! Construct the callback state for an externally synchronized object
    CallbackHolderImpl(const Callback& cb) :
        parent_t{cb},
        weak_ref_{}
    {}

    /*!
     * erase from destructor can't happen while we are in call because
     */
    void erase() noexcept
    {
        parent_t::state_ |= parent_t::deleted;
    }

    //! Return RAII CallGuard to protect race between callback and
    //! disconnect.
    CallGuard prepareCall()
    {
        return {this};
    }
};

template<typename RT, typename... Args, typename tag>
struct SignalImpl<RT(Args...), tag> : public selectImpl<RT(Args...), tag>::parent_t {
protected:
    using select_t = selectImpl<RT(Args...), tag>;
    using parent_t = typename select_t::parent_t;
public:
    using CallbackHolder = CallbackHolderImpl<RT(Args...), tag>;
    using Callback = typename CallbackHolder::Callback;

    //! The container type used to store callbacks
    using CallbackContainer = std::list<CallbackHolder>;
    struct BlockGuard;

    //! Simple connection class that is required to disconnect from the
    //! signal.
    struct Connection {
        //! Construct a default Connection object but using it will result
        //! to undefined behavior unless proper connection is assigned to it
        Connection() = default;

        Connection(Connection&& o) :
            iter_{o.iter_},
            signal_{}
        {
            std::swap(signal_, o.signal_);
        }

        Connection& operator=(Connection&& o)
        {
            disconnect();
            iter_ = o.iter_;
            std::swap(signal_, o.signal_);
            return *this;
        }

        Connection(const Connection&) = delete;
        Connection& operator=(const Connection&) = delete;

        //! Disconnect from signal
        void disconnect()
        {
            auto s = select_t::lock(signal_);
            if (!s)
                return;

            s->disconnect(*this);
        }

        ~Connection()
        {
            disconnect();
        }
    private:
        //! Block the connection temporary
        void block()
        {
            auto s = select_t::lock(signal_);
            if (!s)
                return;
            iter_->block();
        }

        //! Restore blocked connection
        void unblock()
        {
            auto s = select_t::lock(signal_);
            if (!s)
                return;
            iter_->unblock();
        }

        //! Construct connection object
        Connection(const typename CallbackContainer::iterator &iter,
                typename select_t::weak_ptr ptr) :
            iter_{iter},
            signal_{ptr}
        {}

        //! std::list iterator that is used to access the callback and allow
        //! removal from the list.
        typename CallbackContainer::iterator iter_;
        //! Reference to signal object
        typename select_t::weak_ptr signal_;
        friend SignalImpl;
        friend BlockGuard;
    };

    /*!
     * BlockGuard allows temporary RAII guard managed blocking of a
     * connection object.
     */
    struct BlockGuard {
        /*!
         * Block a connection that belongs to signal
         * \param connection The connection that will be temporary blocked
         */
        BlockGuard(Connection& connection) :
            blocked_{&connection}
        {
            connection.block();
        }

        /*!
         * Unblock the temporary blocked connection
         */
        ~BlockGuard()
        {
            blocked_->unblock();
        }

        //! Prevent copies but allow copy elision
        BlockGuard(const BlockGuard&);
    private:
        Connection* blocked_;
    };

    Connection connect(const Callback& f)
    {
        std::lock_guard<std::mutex> lock(access_);
        auto iter = callbacks_.emplace(callbacks_.begin(), f);
        return {iter, parent_t::shared_from_this()};
    }

    Connection connect(std::shared_ptr<ConnectedBase> c, const Callback& f)
    {
        std::lock_guard<std::mutex> lock(access_);
        auto iter = callbacks_.emplace(callbacks_.begin(), f, c);
        return {iter, parent_t::shared_from_this()};
    }

    void disconnect(Connection& connection) {
        std::lock_guard<std::mutex> lock(access_);
        if (recursion_) {
            deleted_ = true;
            connection.iter_->erase();
        } else {
            callbacks_.erase(connection.iter_);
        }
        select_t::reset(connection.signal_);
    }

    template<typename Combiner>
    void operator()(Combiner &combiner, Args&&... arg)
    {
        std::unique_lock<std::mutex> lock(access_);
        struct RecursionGuard {
            SignalImpl* signal_;
            std::unique_lock<std::mutex>* lock_;
            //! Increment access count to make sure disconnect doesn't erase
            RecursionGuard(SignalImpl *signal, std::unique_lock<std::mutex>* lock) :
                signal_{signal},
                lock_{lock}
            {
                ++signal_->recursion_;
            }

            /*!
             * Clean up deleted functions in data race free and exception
             * safe manner.
             */
            ~RecursionGuard()
            {
                lock_->lock();
                if (--signal_->recursion_ == 0 && signal_->deleted_) {
                    for (auto iter = signal_->callbacks_.begin(); iter != signal_->callbacks_.end();) {
                        if (iter->erased())
                            iter = signal_->callbacks_.erase(iter);
                        else
                            ++iter;
                    }
                    signal_->deleted_ = false;
                }
            }

        } guard{this, &lock};
        // Call begin in locked context to allow data race free iteration
        // even if there is parallel inserts to the begin after unlocking.
        auto iter = callbacks_.begin();
        lock.unlock();
        for (; iter != callbacks_.end(); ++iter) {
            // Quickly skip blocked calls without memory writes
            if (!*iter)
                continue;
            // Protect connection from deletion while we are about to call
            // it.
            auto cb = iter->prepareCall();
            if (cb)
                combiner(*cb, std::forward<Args>(arg)...);
        }
    }

    void operator()(Args&&... arg)
    {
        auto combiner = [](const Callback& cb, Args&&... arg2)
        {
            cb(std::forward<Args>(arg2)...);
        };
        (*this)(combiner,std::forward<Args>(arg)...);
    }

    ~SignalImpl() {
        // Check that callbacks are empty. If this triggers then signal may
        // have to be extended to allow automatic disconnection of active
        // connections in the destructor.
        if (std::is_same<tag, signal_inline_tag>::value)
            assert(callbacks_.empty() && "It is very likely that this signal should use signal_shared_tag");
    }

    //! Simplify access to pimpl when it is inline
    SignalImpl* operator->() {
        return this;
    }
    SignalImpl& operator*() {
        return *this;
    }

    SignalImpl() = default;
private:
    SignalImpl(const SignalImpl&) :
        SignalImpl{}
    {}
    std::mutex access_;
    CallbackContainer callbacks_;
    int recursion_;
    bool deleted_;
    friend Signal<RT(Args...), tag>;
};

template<typename RT, typename... Args>
struct selectImpl<RT(Args...), signal_inline_tag> {
    using impl_t = SignalImpl<RT(Args...), signal_inline_tag>;
    using interface_t = Signal<RT(Args...), signal_inline_tag>;
    using type = impl_t;
    using weak_ptr = impl_t*;
    struct ptr_from_this {
        weak_ptr shared_from_this()
        {
            return static_cast<weak_ptr>(this);
        }
    };
    using parent_t = ptr_from_this;

    selectImpl() = default;

    // Disallow copies for inline version.
    selectImpl(const selectImpl&) = delete;
    selectImpl(selectImpl&&) = delete;
    selectImpl& operator=(const selectImpl&) = delete;
    selectImpl& operator=(selectImpl&&) = delete;


    static type make() {
        return {};
    }

    static void reset(weak_ptr& ptr) {
        ptr = nullptr;
    }

    static weak_ptr lock(weak_ptr& ptr) {
        return ptr;
    }

    static weak_ptr get(interface_t& signal) {
        return &signal.pimpl;
    }
};

template<typename RT, typename... Args>
struct selectImpl<RT(Args...), signal_shared_tag> {
    using impl_t = SignalImpl<RT(Args...), signal_shared_tag>;
    using interface_t = Signal<RT(Args...), signal_shared_tag>;
    using type = std::shared_ptr<impl_t>;
    using weak_ptr = std::weak_ptr<impl_t>;
    using parent_t = std::enable_shared_from_this<impl_t>;

    // Allow copies for shared version

    static type make() {
        return std::make_shared<SignalImpl<RT(Args...), signal_shared_tag>>();
    }

    static void reset(weak_ptr& ptr) {
        ptr.reset();
    }

    static type lock(weak_ptr& ptr) {
        return ptr.lock();
    }

    static weak_ptr get(interface_t& signal) {
        return signal.pimpl;
    }
};
}

/*!
 * As I couldn't figure out which signal library would be a good. Too bad all
 * signal libraries seem to be either heavy with unnecessary features or written
 * before C++11/14 have become useable targets. That seems to indicate everyone
 * is now building signal system with standard components.
 *
 * Implementation and interface is build around std::function holding delegates
 * to a function pointer or a functor. One can put there example lambda function
 * that captures this pointer from connect side. The lambda function then calls
 * the slot method of object correctly.
 *
 * It is fairly simple to change the signal signature to directly call methods
 * but internally that std::function becomes more complex. The pointer to
 * member function is problematic because multiple inheritance requires
 * adjustments to this. The lambda capture approach should be easy to use while
 * letting compiler optimize method call in the callee side.
 *
 * DFHack::Signal::Connection is an connection handle. The handle can be used to
 * disconnect and block a callback. Connection destructor will automatically
 * disconnect from the signal.
 *
 * DFHack::Signal::BlockGuard is an automatic blocked callback guard object. It
 * prevents any signals from calling the slot as long the BlockGuard object is
 * alive. Internally it replaces the callback with an empty callback and stores
 * the real callback in a member variable. Destructor then puts back the real
 * callback. This allows easily recursive BlockGuard work correctly because only
 * the first BlockGuard has the real callback.
 *
 * signal_inline_tag requires careful destruction order where all connection are
 * disconnected before signal destruction. The implementation is specifically
 * targeting places like static and singleton variables and widget hierarchies.
 * It provides data race free connect, disconnect and emit operations.
 *
 * signal_shared_tag allows a bit more freedom when destroying the Signal. It
 * adds data race safety between Connection, BlockGuard and destructor. If
 * multiple callers need access to Signal with potential of destruction of
 * original owner then callers can use Signal copy constructor to take a strong
 * reference managed by shared_ptr or weak_ptr with Signal::weak_from_this().
 * weak_from_this returns an object that forwards call directly to
 * implementation when the shared_ptr is created using Signal::lock
 *
 * \param RT return type is derived from a single signature template argument
 * \param Args Variable argument type list that is derived from a signature
 *             template argument.
 * \param tag The tag type which selects between shared_ptr managed pimpl and
 *            inline member variables.
 */
template<typename RT, typename... Args, typename tag>
class Signal<RT(Args...), tag> : protected details::selectImpl<RT(Args...), tag> {
public:
    //! Type of callable that can be connected to the signal.
    using Callback = std::function<RT(Args...)>;

protected:
    using select_t = details::selectImpl<RT(Args...), tag>;
    using CallbackContainer = typename select_t::impl_t::CallbackContainer;
public:
    using weak_ptr = typename select_t::weak_ptr;

    /*!
     * Simple connection class that is required to disconnect from the
     * signal.
     * \sa SignalImpl::Connection
     */
    using Connection = typename select_t::impl_t::Connection;
    /*!
     * BlockGuard allows temporary RAII guard managed blocking of a
     * connection object.
     * \sa SignalImpl::BlockGuard
     */
    using BlockGuard = typename select_t::impl_t::BlockGuard;

    /*!
     * Connect a callback function to the signal
     *
     * Safe to call from any context as long as SignalImpl destructor can't be
     * called simultaneously from other thread.
     *
     * \param f callable that will connected to the signal
     * \return connection handle that can be used to disconnect it
     */
    Connection connect(const Callback& f)
    {
        return pimpl->connect(f);
    }

    /*!
     * Thread safe connect variant connection and Connected object destruction
     * can't race with emit from different threads.
     *
     * Safe to call from any context as long as SignalImpl destructor can't be
     * called simultaneously from other thread.
     */
    Connection connect(std::shared_ptr<ConnectedBase> c, const Callback& f)
    {
        static_assert(std::is_same<tag, signal_shared_tag>::value,
                "Race free destruction is only possible with signal_shared_tag");
        return pimpl->connect(c, f);
    }

    /*!
     * Disconnection a callback from slots
     *
     * signal_inline_tag:
     * This may not be called if the callback has been called in same
     * thread. If callback should trigger destruction an object then
     * deletion must use deferred. This rule prevents issues if other thread
     * are trying to call the callback when disconnecting.
     *
     * signal_shared_tag:
     * disconnect can be freely called from anywhere as long as caller holds a
     * strong reference to the Signal. Strong reference can be obtained by using
     * Connection::disconnect, Signal copy constructor to have a copy of signal
     * or weak_ptr from weak_from_this() passed to Signal::lock().
     *
     * \param connection the object returned from DFHack::Signal::connect
     */
    void disconnect(Connection& connection)
    {
        pimpl->disconnect(connection);
    }

    /*!
     * Call all connected callbacks using passed arguments.
     *
     * signal_inline_tag:
     * Must not call operator() from callbacks.
     * Must not disconnect called callback from inside callback. Solution often
     * is to set just atomic state variables in callback and do actual
     * processing including deletion in update handler or logic vmethod.
     *
     * signal_shared_tag:
     * Safe to call from any context as long as SignalImpl destructor can't be
     * called simultaneously from other thread.
     * Safe to disconnect any connection from callbacks.
     *
     * \param combiner that calls callbacks and processes return values
     * \param arg arguments list defined by template parameter signature.
     */
    template<typename Combiner>
    void operator()(Combiner &combiner, Args&&... arg)
    {
        (*pimpl)(combiner, std::forward<Args>(arg)...);
    }

    /*!
     * Call all connected callbacks using passed arguments.
     *
     * signal_inline_tag:
     * Must not call operator() from callbacks.
     * Must not disconnect called callback from inside callback. Solution often
     * is to set just atomic state variables in callback and do actual
     * processing including deletion in update handler or logic vmethod.
     *
     * signal_shared_tag:
     * Safe to call from any context as long as SignalImpl destructor can't be
     * called simultaneously from other thread.
     * Safe to disconnect any connection from callbacks.
     *
     * \param arg arguments list defined by template parameter signature.
     */
    void operator()(Args&&... arg)
    {
        (*pimpl)(std::forward<Args>(arg)...);
    }

    /*!
     * Helper to lock the weak_ptr
     */
    static typename select_t::type lock(weak_ptr& ptr)
    {
        return select_t::lock(ptr);
    }

    /*!
     * Helper to create a weak reference to pimpl which can be used to access
     * pimpl directly. If the tag is signal_shared_tag then it provides race
     * free access to Signal when using Signal::lock and checking returned
     * shared_ptr.
     */
    weak_ptr weak_from_this() noexcept
    {
        return select_t::get(*this);
    }

    Signal() :
        pimpl{select_t::make()}
    {}
private:
    typename select_t::type pimpl;
    friend select_t;
};
}
