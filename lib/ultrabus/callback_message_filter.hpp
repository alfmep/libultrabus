/*
 * Copyright (C) 2026 Dan Arrhenius <dan@ultramarin.se>
 *
 * This file is part of ultrabus.
 *
 * ultrabus is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef ULTRABUS_CALLBACK_MESSAGE_FILTER_HPP
#define ULTRABUS_CALLBACK_MESSAGE_FILTER_HPP

#include <ultrabus/message_filter.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/connection.hpp>
#include <functional>
#include <mutex>


namespace ultrabus {


    /**
     * A message filter that delegates handling to user-supplied callbacks.
     *
     * Callbacks execute on the associated connection's dispatch thread
     * and the message reference is valid only during the callback.
     * The connection must outlive this filter.
     */
    class callback_message_filter : public message_filter {
    public:
        /**
         * Function type for handling a received message.
         * @param msg The received message.
         * @return <code>true</code> if the message was handled by
         *         the callback. If not, return <code>false</code>.
         */
        using msg_cb_t = std::function<bool (message& msg)>;

        /**
         * Constructor.
         * @param conn The connection that owns this filter registration and
         *        must outlive it.
         * @param method_call_callback Optional callback for incoming D-Bus
         *        method calls.
         * @param signal_callback Optional callback for incoming D-Bus signals.
         * @throws std::system_error if the underlying filter cannot be
         *         registered because libdbus runs out of memory.
         */
        callback_message_filter (connection& conn,
                                 msg_cb_t method_call_callback = nullptr,
                                 msg_cb_t signal_callback = nullptr);

        /**
         * Destructor.
         * Destroy the filter and unregister it from the connection.
         */
        virtual ~callback_message_filter () = default;

        /**
         * Set the callback for incoming method calls.
         *
         * <b>Note!</b><br/>
         * If a callback is installed using method
         * <code>set_message_cb()</code>, all messages will be
         * handled in that callback and this callback will never
         * be called.
         *
         * @param callback The callback to be called for incoming
         *                 method call messages, or <code>nullptr</code>
         *                 to remove the callback.
         */
        void set_method_call_cb (msg_cb_t callback);

        /**
         * Set the callback for incoming signals.
         *
         * <b>Note!</b><br/>
         * If a callback is installed using method
         * <code>set_message_cb()</code>, all messages will be
         * handled in that callback and this callback will never
         * be called.
         *
         * @param callback The callback to be called for incoming
         *                 signals, or <code>nullptr</code>
         *                 to remove the callback.
         */
        void set_signal_cb (msg_cb_t callback);

        /**
         * Set the callback for all incoming messages.
         *
         * <b>Note!</b><br/>
         * If a callback is installed using this method,
         * all messages will be handled in that callback and any
         * callback installed using methods <code>set_method_call_cb()</code>
         * or <code>set_signal_cb()</code> will never be called.
         *
         * @param callback The callback to be called for incoming
         *                 messages, or <code>nullptr</code>
         *                 to remove the callback.
         */
        void set_message_cb (msg_cb_t callback);


    protected:
        /**
         * Invoke the configured method-call callback, when present.
         * @param msg The incoming method-call message.
         * @return The callback result, or <code>false</code> when no callback
         *         is installed.
         */
        virtual bool on_method_call (message& msg);

        /**
         * Invoke the configured signal callback, when present.
         * @param msg The incoming signal message.
         * @return The callback result, or <code>false</code> when no callback
         *         is installed.
         */
        virtual bool on_signal (message& msg);

        /**
         * Invoke the all-message callback or dispatch to the typed callbacks.
         * @param msg The incoming message.
         * @return The invoked callback's result, or <code>false</code> when
         *         no applicable callback is installed.
         */
        virtual bool on_message (message& msg);


    private:
        std::mutex cb_mutex;
        msg_cb_t on_method_call_cb;
        msg_cb_t on_signal_cb;
        msg_cb_t on_message_cb;
    };



}
#endif
