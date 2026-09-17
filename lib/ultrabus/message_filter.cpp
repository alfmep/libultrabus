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
#include <ultrabus/message_filter.hpp>
#include <ultrabus/connection.hpp>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    message_filter::message_filter (connection& conn_arg)
        : conn (conn_arg)
    {
        if (!install_filter_handler())
            throw std::system_error (ENOMEM, std::generic_category());
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    message_filter::message_filter (connection& conn_arg, bool delay_filter_install)
        : conn (conn_arg)
    {
        if ( ! delay_filter_install) {
            if (!install_filter_handler())
                throw std::system_error (ENOMEM, std::generic_category());
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    message_filter::~message_filter ()
    {
        const std::lock_guard lock (match_rule_mutex);
        if (conn.is_connected()) {
            remove_filter_handler ();
            for (const auto& rule : match_rules)
                dbus_bus_remove_match (conn.handle(), rule.c_str(), nullptr);
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::install_filter_handler () {
        return dbus_connection_add_filter (conn.handle(), static_dbus_handler, this, nullptr);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void message_filter::remove_filter_handler () {
        dbus_connection_remove_filter (conn.handle(), static_dbus_handler, this);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> message_filter::add_match (const std::string& rule, int timeout)
    {
        if ( ! conn.is_connected()) {
            retvalue<bool> ret (false);
            ret.err (-1, "Not connected");
            return ret;
        }

        retvalue<bool> retval (true);
        const std::lock_guard lock (match_rule_mutex);

        if (match_rules.find(rule) != match_rules.end())
            return retval;

        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "AddMatch");
        msg.append_args (rule);
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            retval = false;
            retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }else{
            match_rules.emplace (rule);
        }
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::add_match (const std::string& rule,
                                    std::function<void (retvalue<bool>& retval)> callback,
                                    int timeout)
    {
        if ( ! conn.is_connected())
            return false;

        const std::lock_guard lock (match_rule_mutex);

        if (match_rules.find(rule) != match_rules.end())
            return true;

        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "AddMatch");
        msg.append_args (rule);

        if (!callback) {
            match_rules.emplace (rule);
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (msg,
                          [this, rule, callback](message& reply){
                              match_rule_mutex.lock ();
                              retvalue<bool> retval (true);
                              if (reply.is_error()) {
                                  retval = false;
                                  retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                              }else{
                                  match_rules.emplace (rule);
                              }
                              match_rule_mutex.unlock ();
                              callback (retval);
                          },
                          timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    retvalue<bool> message_filter::remove_match (const std::string& rule, int timeout)
    {
        if ( ! conn.is_connected()) {
            retvalue<bool> ret (false);
            ret.err (-1, "Not connected");
            return ret;
        }
        retvalue<bool> retval (true);
        const std::lock_guard lock (match_rule_mutex);

        auto i = match_rules.find (rule);
        if (i == match_rules.end())
            return retval;

        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "RemoveMatch");
        msg.append_args (rule);
        auto reply = conn.send_and_wait (msg, timeout);
        if (reply.is_error()) {
            retval = false;
            retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
        }else{
            match_rules.erase (i);
        }
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::remove_match (const std::string& rule,
                                       std::function<void (retvalue<bool>& retval)> callback,
                                       int timeout)
    {
        if ( ! conn.is_connected())
            return false;

        const std::lock_guard lock (match_rule_mutex);

        auto i = match_rules.find (rule);
        if (i == match_rules.end()) {
            return true;
        }

        message msg (DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS, "RemoveMatch");
        msg.append_args (rule);

        if (!callback) {
            match_rules.erase (i);
            msg.want_reply (false);
            return conn.send (msg);
        }
        return conn.send (msg,
                          [this, rule, callback](message& reply){
                              match_rule_mutex.lock ();
                              retvalue<bool> retval (true);
                              if (reply.is_error()) {
                                  retval = false;
                                  retval.err (-1, reply.error_name() + std::string(": ") + reply.error_msg());
                              }else{
                                  match_rules.erase (rule);
                              }
                              match_rule_mutex.unlock ();
                              callback (retval);
                          },
                          timeout);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::on_method_call (message& /*msg*/)
    {
        return false;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::on_signal (message& /*msg*/)
    {
        return false;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::on_message (message& msg)
    {
        return dispatch_msg (msg);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool message_filter::dispatch_msg (message& msg)
    {
        if (msg.is_method_call())
            return on_method_call (msg);
        else if (msg.is_signal())
            return on_signal (msg);
        return false;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    DBusHandlerResult message_filter::static_dbus_handler (
            DBusConnection* /*dbconn*/,
            DBusMessage* dbmsg,
            void* user_data)
    {
        message_filter* handler {static_cast<message_filter*>(user_data)};
        message msg (dbmsg);
        return handler->on_message(msg) ?
            DBUS_HANDLER_RESULT_HANDLED :
            DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }


}
