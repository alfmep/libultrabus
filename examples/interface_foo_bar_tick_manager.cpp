#include <string>
#include <cstdint>
#include "interface_foo_bar_tick_manager.hpp"
#include "interface_foo_bar_tick.hpp"

#include <iostream>


namespace ubus = ultrabus;


namespace ultrabus_examples {


    //--------------------------------------------------------------------------
    interface_foo_bar_tick_manager::interface_foo_bar_tick_manager ()
        : ubus::interface (interface_name),
          id_counter {0}
    {
        // DBus Method: AddTick
        // In: <none>
        // Out: opath - Object path of the new object implementing DBus interface 'foo.bar.Tick'
        add_method ("AddTick", "", "path:o",
                    [this](ubus::connection& bus, ubus::message& msg){add_tick(bus, msg);});

        // DBus Method: RemoveTick
        // In: opath - Object path of the object implementing DBus interface 'foo.bar.Tick'
        // Out: ok - Boolean: true: object removed, false: No such object or we don't control the object
        add_method ("RemoveTick", "path:o", "ok:b",
                    [this](ubus::connection& bus, ubus::message& msg){remove_tick(bus, msg);});

        // DBus property: TickObjects
        // A read-only property with a callback function to get the property value
        add_property ("TickObjects", // All Tick objects
                      "ao",          // Array of object paths
                      [this](const std::string& name, ubus::dbus_variant& value)->bool{
                          get_tick_objects (value);
                          return true;
                      });
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick_manager::add_tick (ubus::connection& bus, ubus::message& msg)
    {
        std::string path = "tick/";
        path.append (std::to_string(id_counter++));
        auto tick_obj = owner()->create_node (path);
        tick_obj->add_interface (interface_foo_bar_tick::create());
        tick_obj->enable ();

        tick_objects.emplace (tick_obj->path());

        auto reply = msg.make_reply ();
        reply.append_args (tick_obj->path());
        bus.send (reply);

        emit_property_changed ("TickObjects");
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick_manager::remove_tick (ubus::connection& bus, ubus::message& msg)
    {
        ubus::dbus_opath path;
        msg.get_args (path);
        ubus::dbus_bool ok = owner()->remove_node (path);
        if (ok)
            tick_objects.erase (path);

        auto reply = msg.make_reply ();
        reply.append_args (ok);
        bus.send (reply);

        if (ok)
            emit_property_changed ("TickObjects");
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface_foo_bar_tick_manager::get_tick_objects (ubus::dbus_variant& value) const
    {
        ubus::dbus_array objs ("o");
        for (const auto& path : tick_objects)
            objs.push_back (path);
        value = objs;
    }


} // namespace ultrabus_examples
