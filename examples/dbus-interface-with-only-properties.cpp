#include <ultrabus.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdint>

namespace ubus = ultrabus;


namespace {

    /**
     * A DBus interface with only properties.
     *
     * Property: Name
     * A read-only property. The name of our interface
     *
     * Property: SomeData
     * A read-only property. A static read-only array of 32-bit signed integers
     *
     * Property: Number
     * A read-write property. A 32-bit signed number that can be read and written
     *
     */
    class iface_only_props {
    public:
        /**
         * The name of our interface
         */
        static constexpr const char* interface_name = "only.props";

        /** Constructor */
        iface_only_props ();
        /** Destructor */
        ~iface_only_props ();

        /**
         * Return a shared pointer to the ultrabus::interface instance
         */
        std::shared_ptr<ubus::interface> get_iface () {return iface;}

    private:
        std::shared_ptr<ubus::interface> iface;
        std::string comment;
        std::thread tick_thread;
        bool stop_tick_thread;
        bool get_prop_value (const std::string& name, ubus::dbus_variant& value);
        bool set_comment (const ubus::dbus_variant& value);
        void run_ticks ();
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    iface_only_props::iface_only_props ()
        : comment {"No comment!"},
          stop_tick_thread {true}
    {
        // Create the interface instance
        iface = ubus::interface::create (interface_name);

        // Add read-only property 'Name' to the interface
        // The property value is handled internally in the interface object
        iface->add_property ("Name",                             // The name of the property
                             ubus::dbus_string(interface_name)); // The initial value of the property

        // Add read-only property 'SomeData' to the interface
        // The property value is handled internally in the interface object
        iface->add_property ("SomeData",                   // The name of the property
                             ubus::dbus_array({1,2,3,4})); // The initial value of the property

        // Add read-write property 'Number' to the interface
        // The property value is handled internally in the interface object
        iface->add_property ("Number",          // The name of the property
                             ubus::dbus_i32(0), // The initial value of the property
                             true,              // The property is readable
                             true);             // The property is writable

        // Add write-only property 'Hidden' to the interface
        // The property value is handled internally in the interface object
        iface->add_property ("Hidden",          // The name of the property
                             ubus::dbus_i32(0), // The initial value of the property
                             false,             // The property is not readable
                             true,              // The property is writable
                             true);             // Don't emit a signal when property changed

        // Add read-only property 'HiddenStatus' to the interface
        // The property value is handled by us in callback functions for get and set
        iface->add_property ("HiddenStatus",  // The name of the property
                             "s",             // The signature(type) of the property (s == string)
                             [this](const std::string& name, ubus::dbus_variant& value){
                                 // Callback for getting the property value
                                 return get_prop_value (name, value);
                             }); // No callback for setting the property value

        // Add read-write property 'Comment' to the interface
        // The property value is handled by us in callback functions for get and set
        iface->add_property ("Comment",       // The name of the property
                             "s",             // The signature(type) of the property (s == string)
                             [this](const std::string& name, ubus::dbus_variant& value){
                                 // Callback for getting the property value
                                 return get_prop_value (name, value);
                             },
                             [this](const std::string& /*name*/, const ubus::dbus_variant& value){
                                 // Callback for setting the property value
                                 return set_comment (value);
                             });

        // Add read-only property 'Tick' to the interface
        // The property value is normally handled internally in the interface object
        iface->add_property ("Tick",            // The name of the property
                             ubus::dbus_u32(0), // The initial value of the property
                             true);             // The property is read-only

        // Start a thread that increases the value of property Tick once every 5:th second
        stop_tick_thread = false;
        tick_thread = std::thread ([this](){
            run_ticks ();
        });
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    iface_only_props::~iface_only_props ()
    {
        // Stop tick thread
        stop_tick_thread = true;
        tick_thread.join ();
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool iface_only_props::get_prop_value (const std::string& name, ubus::dbus_variant& value)
    {
        if (name == "Comment") {
            value = comment;
        }
        else if (name == "HiddenStatus") {
            auto hidden_variant = iface->property ("Hidden");
            if (hidden_variant == nullptr) {
                value = "Hidden value no longer with us";
            }else{
                int32_t hidden_value = hidden_variant->get<ubus::dbus_i32>();
                if (hidden_value < 0)
                    value = "A negative number.";
                else if (hidden_value == 0)
                    value = "The number is zero.";
                else if (hidden_value < 10)
                    value = "A small number.";
                else if (hidden_value < 100)
                    value = "A somewhat small number.";
                else if (hidden_value < 1000)
                    value = "A medium number.";
                else
                    value = "A large number.";
            }
        }
        return true;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool iface_only_props::set_comment (const ubus::dbus_variant& value)
    {
        std::string tmp_comment = value.get<ubus::dbus_string> ();

        if (tmp_comment.empty()) {
            // Return error, we don't want an empty comment
            return false;
        }

        if (tmp_comment == "remove hidden value") {
            tmp_comment = "Hidden value removed";
            auto hidden_variant = iface->property ("Hidden");
            if (hidden_variant != nullptr)
                iface->remove_property ("Hidden", true);
        }
        else if (tmp_comment == "add hidden value") {
            auto hidden_variant = iface->property ("Hidden");
            if (hidden_variant == nullptr) {
                iface->add_property ("Hidden",          // The name of the property
                                     ubus::dbus_i32(0), // The initial value of the property
                                     false,             // The property is not readable
                                     true,              // The property is writable
                                     true);             // Don't emit a signal when property Hdden changed
                tmp_comment = "Hidden value added";
            }else{
                tmp_comment = "Hidden already value added";
            }
        }

        if (comment != tmp_comment) {
            comment = tmp_comment;                             // Set the new property value
            iface->emit_property_changed ("Comment", comment); // Emit a property changed signal
        }
        return true;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void iface_only_props::run_ticks ()
    {
        unsigned counter {};
        while (true) {
            std::this_thread::sleep_for (std::chrono::milliseconds(100));
            if (stop_tick_thread)
                break;
            if ((++counter % 50) != 0)
                continue;
            //
            // Increase value of property Tick once every 5:th second
            //
            auto tick = iface->property ("Tick");
            if (!tick)
                continue;

            ++(uint32_t&)tick->get<ubus::dbus_u32>(); // Increment value
            iface->emit_property_changed ("Tick");    // Emit property changed signal
        }
    }


} // Anonymous namespace



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main ()
{
    ubus::connection conn;
    iface_only_props only_props;

    // Connect to the session bus
    if (!conn.connect()) {
        std::cerr << "Error connecting to session bus" << std::endl;
        return 1;
    }

    // Try to acquire the bus name "foo.bar"
    std::string bus_name = "foo.bar";
    std::cout << std::endl;
    if (!conn.request_name(bus_name)) {
        bus_name = conn.unique_name ();
        std::cerr << "Error requesting bus name '" << bus_name << "'" << std::endl;
        std::cout << "Unique bus name of this connection: \"" << bus_name << '"' << std::endl;
    }else{
        std::cerr << "Service name: " << bus_name << std::endl;
    }

    // Print some help
    std::cout << "To introspect our object, run: 'dbus-tool introspect "
              << bus_name << " /foo/bar'" << std::endl;
    std::cout << "To view all properties, run: 'dbus-tool get -s " << bus_name
              << " /foo/bar only.props'" << std::endl;
    std::cout << "To view a specific property, run: 'dbus-tool get " << bus_name
              << " /foo/bar only.props <property_name>'" << std::endl;
    std::cout << "To set a property value, run: 'dbus-tool set " << bus_name
              << " /foo/bar only.props <property_name> <value>'" << std::endl;
    std::cout << "To monitor property changed signals, run: 'dbus-tool listen -s "
              << bus_name << " /foo/bar'" << std::endl;

    // Create a DBus object at object path /foo/bar
    ubus::object obj (conn, "/foo/bar");

    obj.add_interface (only_props.get_iface()); // Add our interface to the DBus object
    obj.enable ();                              // Enable the object

    std::cout << std::endl;
    std::cout << "Enter something/whatever and press enter to end" << std::endl;
    char ch {};
    std::cin >> ch;
    std::cout << std::endl;

    // Access a property value in our interface
    //
    uint32_t number;
    only_props.get_iface()->get_property ("Tick", number);
    std::cout << "Final value of 'Tick': " << number << std::endl;

    return 0;
}
