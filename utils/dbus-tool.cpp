/*
 * Copyright (C) 2021,2022 Dan Arrhenius <dan@ultramarin.se>
 *
 * This file is part of libultrabus.
 *
 * libultrabus is free software; you can redistribute it and/or modify it
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
#include <ultrabus.hpp>
#include <iostream>
#include <iomanip>
#include <string>
#include <set>
#include <signal.h>

#include "dbus-tool-args.hpp"
#include "dbus-tool-print-introspect.hpp"

namespace ubus = ultrabus;
using namespace std;


static void list_services (ubus::Connection& conn, appargs_t& opt);
static void call_method (ubus::Connection& conn, const appargs_t& opt);
static void introspect (ubus::Connection& conn, const appargs_t& opt);
static void get_property (ubus::Connection& conn, const appargs_t& opt);
static void set_property (ubus::Connection& conn, const appargs_t& opt);
static void objects (ubus::Connection& conn, const appargs_t& opt);
static void listen_for_signals (ubus::Connection& conn, const appargs_t& opt);
static void start_service (ubus::Connection& conn, appargs_t& opt);
static void print_owner (ubus::Connection& conn, appargs_t& opt);
static void print_names (ubus::Connection& conn, appargs_t& opt);
static void ping (ubus::Connection& conn, appargs_t& opt);
static void monitor (ubus::Connection& conn, appargs_t& opt);


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static volatile bool continue_sleep_loop = true;
static void stop_signal_handler (int sig)
{
    continue_sleep_loop = false;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main (int argc, char* argv[])
{
    appargs_t opt (argc, argv);
    try {
        ubus::Connection conn;

        if (opt.bus_address.empty()) {
            conn.connect (opt.bus);
            if (!conn.is_connected()) {
                cerr << "Error: Failed to connect to " <<
                    (opt.bus==DBUS_BUS_SESSION?"session":"system") << " bus" << endl;
                exit (1);
            }
        }else{
            conn.connect (opt.bus_address, opt.timeout, true);
            if (!conn.is_connected()) {
                cerr << "Error: Failed to connect to bus " << opt.bus_address << endl;
                exit (1);
            }
        }

        if (opt.cmd == "list") {
            list_services (conn, opt);
        }
        else if (opt.cmd == "call") {
            call_method (conn, opt);
        }
        else if (opt.cmd == "introspect") {
            introspect (conn, opt);
        }
        else if (opt.cmd == "get") {
            get_property (conn, opt);
        }
        else if (opt.cmd == "set") {
            set_property (conn, opt);
        }
        else if (opt.cmd == "objects") {
            objects (conn, opt);
        }
        else if (opt.cmd == "listen") {
            listen_for_signals (conn, opt);
        }
        else if (opt.cmd == "start") {
            start_service (conn, opt);
        }
        else if (opt.cmd == "owner") {
            print_owner (conn, opt);
        }
        else if (opt.cmd == "names") {
            print_names (conn, opt);
        }
        else if (opt.cmd == "ping") {
            ping (conn, opt);
        }
        else if (opt.cmd == "monitor") {
            monitor (conn, opt);
        }
    }
    catch (std::exception& e) {
        if (!opt.quiet)
            cerr << "Error: " << e.what() << endl;
        exit (1);
    }

    return 0;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void list_services (ubus::Connection& conn, appargs_t& opt)
{
    ubus::org_freedesktop_DBus dbus (conn, opt.timeout);
    auto names = opt.activatable ? dbus.list_activatable_names() : dbus.list_names();
    if (names.err()) {
        cerr << names.what() << endl;
        exit (1);
    }
    for (auto& name : names.get()) {
        if (opt.all || name[0]!=':')
            cout << name << endl;
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void call_method (ubus::Connection& conn, const appargs_t& opt)
{
    ubus::ObjectProxy op (conn, opt.service, opt.opath, opt.iface, opt.timeout);
    ubus::Message msg (opt.service, opt.opath, opt.iface, opt.name);
    for (auto& arg : opt.args)
        msg << arg;

    auto reply = op.send_msg (msg);
    if (reply.is_error()) {
        cerr << "Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
        exit (1);
    }
    auto args = reply.arguments ();
    for (auto& arg : args) {
        cout << arg->str() << endl;
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void introspect (ubus::Connection& conn, const appargs_t& opt)
{
    ubus::ObjectProxy op (conn, opt.service, opt.opath, DBUS_INTERFACE_INTROSPECTABLE, opt.timeout);
    auto reply = op.call ("Introspect");
    if (reply.is_error()) {
        cerr << "Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
        exit (1);
    }

    ubus::dbus_basic xml_doc;
    if (reply.get_args(&xml_doc, nullptr)) {
        if (opt.raw) {
            cout << xml_doc.str() << endl;
        }else{
            cout << "Service: " << opt.service << endl;
            cout << "Object path: " << opt.opath << endl;
            print_introspect (opt.opath, xml_doc.str());
        }
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void get_property (ubus::Connection& conn, const appargs_t& opt)
{
    ubus::org_freedesktop_DBus_Properties properties (conn, opt.timeout);

    if (!opt.name.empty()) {
        //
        // Get a specific property
        //
        auto result = properties.get (opt.service, opt.opath, opt.iface, opt.name);
        if (result.err()) {
            cerr << "Error: " << result.what() << endl;
            exit (1);
        }
        if (opt.print_signature)
            cout << result.get().value().signature() << ' ' << result.get().str() << endl;
        else
            cout << result.get().str() << endl;
    }else{
        //
        // Get all properties
        //
        auto props = properties.get_all (opt.service, opt.opath, opt.iface);
        if (props.err()) {
            cerr << "Error: " << props.what() << endl;
            exit (1);
        }

        // Make a nice output format
        size_t max_width = 1;
        size_t max_sig_width = 1;
        for (auto& prop : props.get().data()) {
            auto& de = dynamic_cast<ubus::dbus_dict_entry&> (prop);
            size_t len = de.key().str().size ();
            if (len > max_width)
                max_width = len;
            if (opt.print_signature) {
                auto& v = dynamic_cast<ubus::dbus_variant&>(de.value());
                len = v.value().signature().size ();
                if (len > max_sig_width)
                    max_sig_width = len;
            }
        }
        for (auto& prop : props.get().data()) {
            auto& de = dynamic_cast<ubus::dbus_dict_entry&> (prop);
            if (opt.print_signature) {
                auto& v = dynamic_cast<ubus::dbus_variant&>(de.value());
                cout << setw(max_width) << de.key().str() << ' '
                     << setw(max_sig_width) << v.value().signature() << ": "
                     << v.str() << endl;
            }else{
                cout << setw(max_width) << de.key().str() << ": " << de.value().str() << endl;
            }
        }
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void set_property (ubus::Connection& conn, const appargs_t& opt)
{
    ubus::org_freedesktop_DBus_Properties prop (conn, opt.timeout);
    ubus::dbus_basic property_value;

    if (opt.args[0] == "true") {
        property_value = true;
    }
    else if (opt.args[0] == "false") {
        property_value = false;
    }
    else{
        try {
            // u32 parameter
            property_value.u32 (std::stoi(opt.args[0]));
        }
        catch (...) {
            // string parameter
            property_value.str (opt.args[0]);
        }
    }

    auto result = prop.set (opt.service, opt.opath, opt.iface, opt.name, property_value);
    if (result.err()) {
        cerr << "Error: " << result.what() << endl;
        exit (1);
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void objects (ubus::Connection& conn, const appargs_t& opt)
{
    ubus::ObjectProxy op (conn, opt.service, opt.opath, "org.freedesktop.DBus.ObjectManager", opt.timeout);

    auto reply = op.call ("GetManagedObjects");
    if (reply.is_error()) {
        cerr << "Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
        exit (1);
    }
    ubus::dbus_array dict;
    if (reply.get_args(&dict, nullptr)) {
        for (auto& entry : dict) {
            auto& de = dynamic_cast<ubus::dbus_dict_entry&> (entry);
            cout << de.key().str() << endl;
        }
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void listen_for_signals (ubus::Connection& conn, const appargs_t& opt)
{
    ubus::ObjectProxy op (conn, opt.service, opt.opath, "", opt.timeout);

    // Install signal handler to exit gracefully on Ctrl-C
    continue_sleep_loop = true;
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sigemptyset (&sa.sa_mask);
    sa.sa_handler = stop_signal_handler;
    sigaction (SIGINT, &sa, nullptr);

    int result = op.add_signal_callback (opt.iface, opt.name, [](ubus::Message &sig)
        {
            // Called from the connection worker thread
            cout << "Got signal: " << sig.name() << endl;
            auto args = sig.arguments ();
            if (!args.empty()) {
                cout << "Arguments: " << endl;
                for (auto& arg : args) {
                    cout << "    " << arg->str() << endl;
                }
                cout << endl;
            }
        });
    if (result) {
        cerr << "Error adding signal listener" << endl;
        exit (1);
    }else{
        while (continue_sleep_loop)
            sleep (1);
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void start_service (ubus::Connection& conn, appargs_t& opt)
{
    ubus::org_freedesktop_DBus dbus (conn, opt.timeout);
    auto result = dbus.start_service_by_name (opt.service);
    if (result.err()) {
        if (!opt.quiet)
            cerr << result.what() << endl;
        exit (1);
    }
    switch (result.get()) {
    case DBUS_START_REPLY_SUCCESS:
        if (!opt.quiet)
            cout << opt.service << " started" << endl;
        break;
    case DBUS_START_REPLY_ALREADY_RUNNING:
        if (!opt.quiet)
            cout << opt.service << " already running" << endl;
        break;
    default:
        if (!opt.quiet)
            cerr << "Error: Unknown return value: " << result.get() << endl;
        exit (1);
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void print_owner (ubus::Connection& conn, appargs_t& opt)
{
    ubus::org_freedesktop_DBus dbus (conn, opt.timeout);
    auto owner = dbus.get_name_owner (opt.service);
    if (owner.err()) {
        cerr << owner.what() << endl;
        exit (1);
    }
    cout << owner.get() << endl;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void print_names (ubus::Connection& conn, appargs_t& opt)
{
    ubus::org_freedesktop_DBus dbus (conn, opt.timeout);
    string bus_name = opt.service;
    if (!opt.service.empty() && opt.service[0]!=':') {
        auto owner = dbus.get_name_owner (opt.service);
        if (owner.err()) {
            cerr << owner.what() << endl;
            exit (1);
        }
        bus_name = owner;
    }
    cout << bus_name << endl;

    auto names = dbus.list_names();
    for (auto& name : names.get()) {
        if (name.empty() || name[0]==':')
            continue;
        auto owner = dbus.get_name_owner (name);
        if (bus_name == owner.get())
            cout << '\t' << name << endl;
    }
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void ping (ubus::Connection& conn, appargs_t& opt)
{
    ubus::org_freedesktop_DBus_Peer peer (conn, opt.timeout);

    auto result = peer.ping (opt.service);
    if (result.err()) {
        if (!opt.quiet)
            cerr << "Fel: " << result.what() << endl;
        exit (1);
    }
    if (!opt.quiet)
        cout << ((float)result.get()/1000) << " ms" << endl;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void monitor (ubus::Connection& conn, appargs_t& opt)
{
    ubus::org_freedesktop_DBus dbus (conn, opt.timeout);
    ubus::CallbackMessageHandler cmh (conn);

    auto result = dbus.become_monitor ();
    if (result.err()) {
        cerr << "Warning: " << result.what() << endl;
        cerr << "Install eavesdrop match rule to monitor messages instead." << endl;
        cmh.add_match_rule ("eavesdrop='true'");
    }

    // Install signal handler to exit gracefully on Ctrl-C
    continue_sleep_loop = true;
    struct sigaction sa;
    memset (&sa, 0, sizeof(sa));
    sigemptyset (&sa.sa_mask);
    sa.sa_handler = stop_signal_handler;
    sigaction (SIGINT, &sa, nullptr);

    // Install message callback function
    cmh.set_message_cb ([](ubus::Message& msg)->bool
        {
            cout << msg.describe() << endl;
            cout << endl;
            return true;
        });

    // Sleep until Ctrl-C (SIGINT)
    while (continue_sleep_loop)
        sleep (1);

    cout << "Done." << endl;
}
