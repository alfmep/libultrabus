#include <ultrabus.hpp>
#include <iostream>
#include <cstdint>

namespace ubus = ultrabus;


namespace {

    /**
     * A DBus interface with only methods.
     *
     * The DBus method GetCounter increments a
     * counter value and returns the new value.
     *
     * The DBus method SetCounter sets the counter to a
     * specific value, or returns an error if the value
     * is negative.
     *
     * The DBus method ModifyCounter modifies the counter
     * either by setting it to a specific value if argument
     * 'absolute' is true, or by adding a value to the
     * current counter value if argument 'absolute' is false.
     * The new counter value is returned.
     */
    class iface_only_methods {
    public:
        /**
         * The name of our interface
         */
        static constexpr const char* interface_name = "only.methods";

        /**
         * Constructor.
         */
        iface_only_methods ();

        /**
         * Return a shared pointer to the ultrabus::interface instance.
         */
        std::shared_ptr<ubus::interface> get_iface () {return iface;}

    private:
        std::shared_ptr<ubus::interface> iface;
        int32_t counter;
        void get_counter (ubus::connection& conn, ubus::message& msg);
        void set_counter (ubus::connection& conn, ubus::message& msg);
        void modify_counter (ubus::connection& conn, ubus::message& msg);
    };

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    iface_only_methods::iface_only_methods ()
        : counter {0}
    {
        // Create the interface instance
        iface = ubus::interface::create (interface_name);

        // Add method GetCounter to the interface
        iface->add_method ("GetCounter", // Name of the DBus method
                           "",           // Empty string: No input arguments to the method.
                           "counter:i",  // Return value in the reply. "i" = 32-bit interger.
                           [this](ubus::connection& conn, ubus::message& msg){
                               get_counter (conn, msg);
                           });

        // Add method SetCounter to the interface
        iface->add_method ("SetCounter",  // Name of the DBus method
                           "new_value:i", // Argument to the method. "i" = 32-bit interger.
                           "",            // Empty string: No return value in the reply.
                           [this](ubus::connection& conn, ubus::message& msg){
                               set_counter (conn, msg);
                           });

        // Add method ModifyCounter to the interface
        iface->add_method ("ModifyCounter",  // Name of the DBus method
                           {   // Arguments to the method.
                               {"modifier:i"}, // "i" = 32-bit interger.
                               {"absolute:b"}, // "b" = boolean
                           },
                           "counter:i", // Return value in the reply. "i" = 32-bit interger.
                           [this](ubus::connection& conn, ubus::message& msg){
                               modify_counter (conn, msg);
                           });

    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void iface_only_methods::get_counter (ubus::connection& conn, ubus::message& msg)
    {
        auto reply = msg.make_reply (); // Create a reply message
        ++counter;                      // Increment the counter
        reply.append_args (counter);    // Add the counter reply argument
        conn.send (reply);              // Send the reply
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void iface_only_methods::set_counter (ubus::connection& conn, ubus::message& msg)
    {
        int32_t new_value {};
        msg.get_args (new_value); // Get the 'new_value' argument

        if (new_value >= 0) {
            counter = new_value;          // Set the counter value to the new value
            conn.send (msg.make_reply()); // Send a reply
        }else{
            // 'new_value' is negative, send an error reply
            conn.send (msg.make_reply("only.methods.error", "Counter value can't be negative"));
        }
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void iface_only_methods::modify_counter (ubus::connection& conn, ubus::message& msg)
    {
        int32_t tmp_counter {};
        int32_t modifier {};
        bool absolute {};
        msg.get_args (modifier, absolute); // Get the arguments

        tmp_counter = absolute ? modifier : counter + modifier;

        if (tmp_counter >= 0) {
            counter = tmp_counter;          // Set the counter value to the new value
            auto reply = msg.make_reply (); // Create a reply message
            reply.append_args (counter);    // Add the counter reply argument
            conn.send (reply);              // Send the reply
        }else{
            // The new counter value would be negative, send an error reply
            conn.send (msg.make_reply("only.methods.error", "Counter value can't be negative"));
        }
    }


} // Anonymous namespace



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int main ()
{
    ubus::connection conn;
    iface_only_methods only_methods;

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
    std::cout << "To view our interface, run: 'dbus-tool introspect " << bus_name << " /foo/bar'" << std::endl;
    std::cout << "To call method GetCounter, run: 'dbus-tool call " << bus_name
              << " /foo/bar only.methods GetCounter'" << std::endl;
    std::cout << "To call method SetCounter, run: 'dbus-tool call " << bus_name
              << " /foo/bar only.methods SetCounter <some_number>'" << std::endl;
    std::cout << "To call method ModifyCounter, run: 'dbus-tool call " << bus_name
              << " /foo/bar only.methods ModifyCounter i <some_number> b <true|false>'" << std::endl;


    ubus::object obj (conn, "/foo/bar");          // Create a DBus object on object path "/foo/bar"
    obj.add_interface (only_methods.get_iface()); // Add our interface to the DBus object
    obj.enable ();                                // Enable the object

    std::cout << std::endl;
    std::cout << "Enter something/whatever and press enter to end" << std::endl;
    char ch {};
    std::cin >> ch;

    return 0;
}
