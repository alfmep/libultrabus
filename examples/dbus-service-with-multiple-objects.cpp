#include <ultrabus.hpp>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <cstdint>
#include "interface_foo_bar_tick_manager.hpp"
#include "interface_foo_bar_tick.hpp"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using namespace ultrabus_examples;

namespace ubus = ultrabus;


namespace {

    constexpr const char* service_name = "foo.bar";


    //--------------------------------------------------------------------------
    class dbus_service_foo_bar {
    public:
        dbus_service_foo_bar (ubus::connection& bus_arg, const ubus::dbus_opath& root_path);
        bool start () {return root.enable();}
        void stop ()  {root.disable();}

    private:
        ubus::connection& bus;
        ubus::object root; // Implements org.freedesktop.DBus.ObjectManager
        std::shared_ptr<ubus::object> base; // Our base object at <root>/foo/bar
    };


    //--------------------------------------------------------------------------
    dbus_service_foo_bar::dbus_service_foo_bar (ubus::connection& bus_arg,
                                                const ubus::dbus_opath& root_path)
        : bus (bus_arg),
          root (bus_arg, root_path, true)
    {
        base = root.create_node ("foo/bar");
        base->add_interface (interface_foo_bar_tick_manager::create());
    }




    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string acquire_bus_name (ubus::connection& conn, const std::string& name)
    {
        std::string bus_name = name;
        if (!conn.request_name(bus_name)) {
            bus_name = conn.unique_name ();
            cerr << "Error requesting bus name '" << name << "'" << endl;
            cout << "Unique bus name of this connection: \"" << bus_name << '"' << endl;
        }else{
            cerr << "Service name: " << bus_name << endl;
        }
        return bus_name;
    }


} // Anonymous namespace



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main ()
{
    // Our DBus connection
    ubus::connection bus;

    // Create our DBus service
    dbus_service_foo_bar foo_bar (bus, "/");

    // Connect to the session bus
    bus.connect ();
    if (!bus.is_connected()) {
        cerr << "Error connecting to session DBus" << endl;
        return 1;
    }
    // Try to acquire our service name (bus name)
    std::string bus_name = acquire_bus_name (bus, service_name);

    // Start our DBus service
    if ( ! foo_bar.start()) {
        cerr << "Error starting our DBus service" << endl;
        return 1;
    }
    cout << "Service started" << endl;

    // Wait for user to stop us
    cout << endl;
    cout << "Enter something/whatever and press enter to end" << endl;
    char ch {};
    cin >> ch;
    cout << endl;

    // Stop the DBus service
    foo_bar.stop ();
    cout << "Service stopped" << endl;

    return 0;
}
