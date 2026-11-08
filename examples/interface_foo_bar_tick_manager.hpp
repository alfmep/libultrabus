#ifndef EXAMPLES_INTERFACE_FOO_BAR_TICK_MANAGER_HPP
#define EXAMPLES_INTERFACE_FOO_BAR_TICK_MANAGER_HPP

#include <ultrabus.hpp>
#include <memory>
#include <set>


namespace ultrabus_examples {


    //--------------------------------------------------------------------------
    //
    class interface_foo_bar_tick_manager : public ultrabus::interface {
    public:
        static constexpr const char* interface_name = "foo.bar.TickManager";
        static std::shared_ptr<ultrabus::interface> create () {
            return std::shared_ptr<ultrabus::interface> (new interface_foo_bar_tick_manager());
        }

        virtual ~interface_foo_bar_tick_manager () = default;


    protected:
        interface_foo_bar_tick_manager ();


    private:
        unsigned id_counter;
        std::set<ultrabus::dbus_opath> tick_objects;
        void add_tick (ultrabus::connection& bus, ultrabus::message& msg);
        void remove_tick (ultrabus::connection& bus, ultrabus::message& msg);
        void get_tick_objects (ultrabus::dbus_variant& value) const;
    };


}
#endif
