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

#include <list>
#include <functional>

namespace DFHack {

namespace detail {

//! Helper type to generate matching call signature for a callback
template<typename Signature>
    struct NoOperationCB;

template<typename... Args>
    struct NoOperationCB<void(Args...)> {
        static void cb(Args...)
        {}
    };

template<typename RT, typename... Args>
    struct NoOperationCB<RT(Args...)> {
        static RT cb(Args...)
        { return {}; }
    };
} /* namespace detail */

template<typename Signature>
    class Signal;

/*!
 * As I couldn't figure out which signal library would be a good. Too bad all
 * signal libraries seem to be either heavy with unnecessary features or written
 * before C++11/14 have become useable targets. That seems to indicate everyone
 * is now building signal system with standard components.
 *
 * DFHack::Signal is designed to be very simple implementation under interface
 * that is very close boost::signal. Similarity hopefully makes it easy to
 * replace this with library implementation in future if features are required.
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
 * disconnect and block a callback. The connection requires explicit management
 * because first implementation uses simple list iterators. It could be possible
 * to change Connection to automatic lifetime management.
 *
 * DFHack::Signal::BlockGuard is an automatic blocked callback guard object. It
 * prevents any signals from calling the slot as long the BlockGuard object is
 * alive. Internally it replaces the callback with an empty callback and stores
 * the real callback in a member variable. Destructor then puts back the real
 * callback. This allows easily recursive BlockGuard work correctly because only
 * the first BlockGuard has the real callback.
 *
 * \param RT return type is derived from a single signature template argument
 * \param Args Variable argument type list that is derived from a signature
 *             template argument.
 */
template<typename RT, typename... Args>
    class Signal<RT(Args...)> {
    public:
        //! Type of callable that can be connected to the signal.
        using Callback = std::function<RT(Args...)>;
        //! The container type used to store callbacks
        using CallbackContainer = std::list<Callback>;
        using SignalType = Signal<RT(Args...)>;
        using NoOperationCB = detail::NoOperationCB<RT(Args...)>;

        struct BlockGuard;

        //! Simple connection class that is required to disconnect from the
        //! signal.
        struct Connection {
            //! Construct a default Connection object but using it will result
            //! to undefined behavior unless proper connection is assigned to it
            Connection() = default;
            //! Allow connection object to be used for disconnect
            void disconnect(SignalType& signal)
            {
                signal.disconnect(*this);
            }
        private:
            //! Block the connection temporary
            Callback block(SignalType& signal)
            {
                if (iter_ == signal.callbacks_.end())
                    return Callback{};
                Callback rv = NoOperationCB::cb;
                std::swap(rv, *iter_);
                return rv;
            }

            //! Restore blocked connection
            void unblock(SignalType& signal, Callback&& cb)
            {
                if (iter_ == signal.callbacks_.end())
                    return;
                *iter_ = std::move(cb);
            }
            //! std::list iterator that is used to access the callback and allow
            //! removal from the list.
            typename CallbackContainer::iterator iter_;
            //! Construct connection object
            explicit Connection(typename CallbackContainer::iterator &iter) :
                iter_(iter)
            {}
            friend SignalType;
            friend BlockGuard;
        };

        /*!
         * BlockGuard allows temporary RAII guard managed blocking of a
         * connection object.
         */
        struct BlockGuard {
            /*!
             * Block a connection that belongs to signal
             * \param signal The signal source object where connection is
             *               linked to. It is just to check if connection is
             *               connected.
             * \param connection The connection that will be temporary blocked
             */
            BlockGuard(SignalType& signal, Connection& connection) :
                signal_{signal},
                blocked_{connection},
                stored_{connection.block(signal)}
            {}

            /*!
             * Unblock the temporary blocked connection
             */
            ~BlockGuard()
            {
                blocked_.unblock(signal_, std::move(stored_));
            }

            //! Prevent copies by allow copy elision
            BlockGuard(const BlockGuard&);
        private:
            Signal& signal_;
            Connection& blocked_;
            Callback stored_;
        };

        /*!
         * Connect a callback function to the signal
         * Data races: Must be externally serialized
         * \param f callable that will connected to the signal
         * \return connection handle that can be used to disconnect it
         */
        Connection connect(Callback f)
        {
            auto iter = callbacks_.emplace(callbacks_.end(), f);
            return Connection(iter);
        }

        /*!
         * Disconnection a callback from slots
         * Data races: Must be externally serialized
         * \param connection the object returned from DFHack::Signal::connect
         */
        void disconnect(Connection& connection)
        {
            if (connection.iter_ == callbacks_.end())
                return;
            callbacks_.erase(connection.iter_);
            // Mark the connection disconnected
            connection.iter_ = callbacks_.end();
        }

        /*!
         * Call all connected callbacks using passed arguments.
         * Callback function can disconnect self but any other disconnect
         * operation may results to undefined behavior while inside a callback.
         * Data races: Must be externally serialized
         * \todo support return value combiner if non-void return is required
         * \param arg arguments list defined by template parameter signature.
         */
        void emit(Args&&... arg)
        {
            auto iter = callbacks_.begin();
            // Use next iterator to allow removal of connection from the
            // callback.
            auto next = iter;
            for (;iter != callbacks_.end(); iter = next) {
                ++next;
                (*iter)(std::forward<Args>(arg)...);
            }
        }
    private:
       CallbackContainer callbacks_;
    };

}
