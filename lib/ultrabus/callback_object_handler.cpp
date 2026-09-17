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
#include <ultrabus/callback_object_handler.hpp>
#include <ultrabus/connection.hpp>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    callback_object_handler::callback_object_handler (
            connection& conn_arg,
            method_call_cb_t method_call_callback)
        : object_handler (conn_arg),
          on_method_call_cb {method_call_callback}
    {
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void callback_object_handler::set_method_call_cb (method_call_cb_t callback)
    {
        std::lock_guard lock (cb_mutex);
        on_method_call_cb = callback;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool callback_object_handler::on_method_call (message& msg)
    {
        cb_mutex.lock ();
        auto cb = on_method_call_cb;
        cb_mutex.unlock ();
        return cb ? cb(msg) : false;
    }


}
