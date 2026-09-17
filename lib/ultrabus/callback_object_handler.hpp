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
#ifndef ULTRABUS_CALLBACK_OBJECT_HANDLER_HPP
#define ULTRABUS_CALLBACK_OBJECT_HANDLER_HPP

#include <ultrabus/object_handler.hpp>
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
    class callback_object_handler : public object_handler {
    public:
        /**
         * Constructor.
         * @param conn The connection that owns this filter
         *             registration and must outlive it.
         * @param method_call_callback Optional callback for incoming
         *                             D-Bus method calls.
         */
        callback_object_handler (connection& conn, method_call_cb_t method_call_callback=nullptr);

        /**
         * Destructor.
         */
        virtual ~callback_object_handler () = default;

        /**
         * Set the callback for incoming method calls.
         * @param callback The callback to be called for incoming
         *                 method call messages, or <code>nullptr</code>
         *                 to remove the callback.
         */
        void set_method_call_cb (method_call_cb_t callback);


    protected:
        virtual bool on_method_call (message& msg);


    private:
        std::mutex cb_mutex;
        method_call_cb_t on_method_call_cb;
    };



}
#endif
