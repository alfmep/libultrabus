#include <string>
#include <cstdint>
#include "interface_foo_bar_tick.hpp"


namespace ubus = ultrabus;


namespace ultrabus_examples {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    interface_foo_bar_tick::interface_foo_bar_tick ()
        : ubus::interface (interface_name),
          counter {0},
          stop_tick_thread {true}
    {
        // Add two methods to the interface
        //
        add_method ("Start", "", "", // No input/output arguments
                    [this](ubus::connection& bus, ubus::message& msg){start(bus, msg);});
        add_method ("Stop", "", "",  // No input/output arguments
                    [this](ubus::connection& bus, ubus::message& msg){
                        stop ();
                        bus.send (msg.make_reply());
                    });
        add_method ("ReadCounter", "", "counter:u", // Returns the current counter value
                    [this](ubus::connection& bus, ubus::message& msg){read_counter(bus,msg);});

        // Add two properties to the interface
        //
        add_property ("Delay",              // Delay in ms before sending next tick
                      ubus::dbus_u32(2000), // 2000ms (unsigned 32-bit integer)
                      true, true);          // read-write

        add_property ("Running",            // true: tick running, false: tick stopped
                      ubus::dbus_bool(false),
                      true, false);         // read-only

        // Add a signal named 'Tick' to the interface
        // This signal has one argument named 'counter' with
        // the same signature as the 'counter' attribute.
        std::string arg_description = std::string("counter:") + counter.signature();
        add_signal ("Tick", arg_description);
        //
        // Note that instead of the above, we could add the signal with the call:
        //     add_signal ("Tick", counter.signature());
        // But then the argument name 'counter' would not be included in the
        // XML introspect data returned by this interface, only the signature
        // of the argument would be included.
        //
        // Another note. Calling 'add_signal(...)' is strictly not neccessary to
        // be able to send signals from an interface instance. But it makes life
        // a bit easier by including information about signals in the XML data
        // returned when method org.freedesktop.DBus.Introspectable.Introspect
        // is called on an object implementing this interface.
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    interface_foo_bar_tick::~interface_foo_bar_tick ()
    {
        stop ();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick::start (ubus::connection& bus, ubus::message& msg)
    {
        if (stop_tick_thread) {
            if (tick_thread.joinable())
                tick_thread.join ();
            stop_tick_thread = false;

            // Set property 'Running' to true and send signal
            // org.freedesktop.DBus.Properties.PropertiesChanged
            *property("Running") = true;
            emit_property_changed ("Running");

            // Start the tick thread
            tick_thread = std::thread ([this](){
                tick_loop ();
            });
        }

        // Send a reply to the entity calling method 'Start'
        bus.send (msg.make_reply());
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick::read_counter (ultrabus::connection& bus, ultrabus::message& msg)
    {
        auto reply = msg.make_reply ();
        reply.append_args (counter);
        bus.send (reply);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick::stop ()
    {
        if (!stop_tick_thread) {
            // Tell the tick thread to end
            stop_tick_thread = true;
            stop_cond.notify_one ();
        }
        if (tick_thread.joinable()) {
            // Wait for the tick thread to end
            tick_thread.join ();
        }
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick::tick_loop ()
    {
        while (!stop_tick_thread) {

            // Get a reference to the dbus_u32 instance that represents property 'Delay'
            ubus::dbus_u32& delay_prop  = property("Delay")->get<ubus::dbus_u32>();

            // Get a reference to the actual uin32_t value of the property
            uint32_t& delay_value = delay_prop;

            // Sanity check
            if (delay_value < 50) {
                // We won't allow shorter delay than 50 ms
                // Set the delay value to 50ms and send signal
                // org.freedesktop.DBus.Properties.PropertiesChanged
                delay_value = 50;
                emit_property_changed ("Delay", delay_prop);
            }

            {
                // Sleep for 'delay_value' milliseconds,
                // or until instructed to stop
                std::unique_lock lock (stop_cond_mutex);
                stop_cond.wait_for (lock,
                                    std::chrono::milliseconds(delay_value),
                                    [this](){return stop_tick_thread;});
            }
            if (stop_tick_thread)
                break;

            // Increase our counter and emit signal 'foo.bar.Tick'
            // with the counter value as argument
            ++(uint32_t&)counter;
            if ( ! emit_signal ("Tick", counter))
                stop_tick_thread = true; // Stop if unable to send signals (if the bus connection is broken)
        }

        // Set property 'Running' to false and emit
        // signal 'org.freedesktop.DBus.Properties.PropertiesChanged'
        *property("Running") = false;
        emit_property_changed ("Running");
    }


} // namespace ultrabus_examples
