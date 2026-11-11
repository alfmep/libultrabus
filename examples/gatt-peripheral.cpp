#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include <cerrno>
#include <unistd.h>
#include <ultrabus.hpp>

using std::cin;
using std::cout;
using std::cerr;
using std::endl;

namespace ubus = ultrabus;


namespace {

    static constexpr const char* service_uuid        = "0000dead-0000-1000-8001-00805f9b34fb";
    static constexpr const char* characteristic_uuid = "0000beef-0000-1000-8001-00805f9b34fb";


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::shared_ptr<ubus::interface> create_interface_LEAdvertisement ()
    {
        auto iface = ubus::interface::create ("org.bluez.LEAdvertisement1");

        // Initialize manufacturer data
        ubus::dbus_u16_dict manufacturer_data;
        manufacturer_data.set ((uint16_t)0xffff,
                               ubus::dbus_array({(uint8_t)0, (uint8_t)1, (uint8_t)2, (uint8_t)3}));

        iface->add_method ("Release", [](ubus::connection& conn, ubus::message& msg){
            conn.send (msg.make_reply());
        });

        iface->add_property ("Type", ubus::dbus_string("peripheral"));
        iface->add_property ("ServiceUUIDs", ubus::dbus_array({{service_uuid}}));
        iface->add_property ("ManufacturerData", manufacturer_data);
        iface->add_property ("ServiceData", ubus::dbus_string_dict("v"));
        return iface;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::shared_ptr<ubus::interface> create_interface_GattService ()
    {
        auto iface = ubus::interface::create ("org.bluez.GattService1");
        iface->add_property ("UUID", ubus::dbus_string(service_uuid));
        iface->add_property ("Primary", ubus::dbus_bool(true));
        return iface;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::shared_ptr<ubus::interface> create_interface_GattCharacteristic (
            const ubus::dbus_opath& service_object_path)
    {
        auto iface = ubus::interface::create ("org.bluez.GattCharacteristic1");
        ubus::interface* iface_ptr = iface.get ();

        iface->add_method ("ReadValue",
                           "options:a{sv}", // Parameter "name:signature"
                           "value:ay",      // Return value "name:signature"
                           [iface_ptr](ubus::connection& conn, ubus::message& msg){
                               auto reply = msg.make_reply ();
                               ubus::dbus_variant* value = iface_ptr->property("Value");
                               cout << "ReadValue: " << value->to_string() << endl;
                               reply.append_args (value->get());
                               conn.send (reply);
                           });
        iface->add_method ("WriteValue",
                           {{"value:ay"},{"options:a{sv}"}}, // Parameters
                           "",                               // Return value (none)
                           [iface_ptr](ubus::connection& conn, ubus::message& msg){
                               ubus::dbus_array new_value ("y");
                               msg.get_args (new_value);
                               cout << "WriteValue: " << new_value.to_string() << endl;
                               iface_ptr->set_property ("Value", std::move(new_value));
                               conn.send (msg.make_reply());
                           });

        iface->add_property ("UUID", ubus::dbus_string(characteristic_uuid));
        iface->add_property ("Service", ubus::dbus_opath(service_object_path));
        iface->add_property ("Flags", ubus::dbus_array({{"read"},{"write"}}));
        iface->add_property ("Value", ubus::dbus_array({(uint8_t)0, (uint8_t)1, (uint8_t)2, (uint8_t)3}),
                             true, true); // read-write
        return iface;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool start_gatt (ubus::object_proxy& adapter,
                     const ubus::dbus_opath& application_path,
                     const ubus::dbus_opath& le_advertisement_path)
    {
        cout << "Register GATT application..." << std::flush;
        auto reply = adapter.call_iface ("org.bluez.GattManager1",
                                         "RegisterApplication",
                                         application_path,
                                         ubus::dbus_string_dict("v"));
        if (reply.is_error()) {
            cerr << " Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
            return false;
        }
        cout << " registered" << endl;

        cout << "Register LE advertisement..." << std::flush;
        reply = adapter.call_iface ("org.bluez.LEAdvertisingManager1",
                                    "RegisterAdvertisement",
                                    le_advertisement_path,
                                    ubus::dbus_string_dict("v"));
        if (reply.is_error()) {
            cerr << " Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
            return false;
        }
        cout << " registered" << endl;

        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void stop_gatt (ubus::object_proxy& adapter,
                    const ubus::dbus_opath& application_path,
                    const ubus::dbus_opath& le_advertisement_path)
    {
        cout << "Unregister LE advertisement..." << std::flush;
        auto reply = adapter.call_iface ("org.bluez.LEAdvertisingManager1",
                                         "UnregisterAdvertisement",
                                         le_advertisement_path);
        if (reply.is_error()) {
            cerr << " Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
        }else{
            cout << " unregistered" << endl;
        }

        cout << "Unregister GATT application..." << std::flush;
        reply = adapter.call_iface ("org.bluez.GattManager1",
                                    "UnregisterApplication",
                                    application_path);
        if (reply.is_error()) {
            cerr << " Error: " << reply.error_name() << " - " << reply.error_msg() << endl;
        }else{
            cout << " unregistered" << endl;
        }
    }


} // Anonymous namespace


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main (int argc, char* argv[])
{
    ubus::connection conn;
    if ( ! conn.connect (DBUS_BUS_SYSTEM)) {
        cerr << "Error connecting to system DBus" << endl;
        return 1;
    }
    cout << "Connected to system bus, bus name: " << conn.unique_name() << endl;

    // Root object, implements org.freedesktop.DBus.ObjectManager
    //     /
    ubus::object gatt_root (conn, "/", true);

    // Create a DBus object implementing org.bluez.LEAdvertisement1
    //     /
    //     |- foo/bar/gatt
    auto leadv_node = gatt_root.create_node ("foo/bar/gatt");
    leadv_node->add_interface (create_interface_LEAdvertisement());

    // Create a DBus object implementing org.bluez.GattService1
    //     /
    //     +- foo/bar/gatt/
    //                    |
    //                    +- service0
    auto service_node = leadv_node->create_node ("service0");
    service_node->add_interface (create_interface_GattService());

    // Create a DBus object implementing org.bluez.GattCharacteristic1
    //     /
    //     +- foo/bar/gatt/
    //                    |
    //                    +- service0/
    //                               |
    //                               +- characteristic0
    auto chr_node = service_node->create_node ("characteristic0");
    chr_node->add_interface (create_interface_GattCharacteristic(service_node->path()));


    gatt_root.enable (); // Enable our object tree


    ubus::object_proxy adapter (conn, "org.bluez", "/org/bluez/hci0");
    if ( ! start_gatt(adapter, gatt_root.path(), leadv_node->path()))
        return 1;

    // Wait for user to stop us
    cout << endl;
    cout << "Enter something/whatever and press enter to end" << endl;
    char ch {};
    cin >> ch;
    cout << endl;

    stop_gatt (adapter, gatt_root.path(), leadv_node->path());

    cerr << "Done" << endl;
    return 0;
}
