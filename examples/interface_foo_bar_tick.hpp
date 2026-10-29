#ifndef EXAMPLES_INTERFACE_FOO_BAR_TICK_HPP
#define EXAMPLES_INTERFACE_FOO_BAR_TICK_HPP

#include <ultrabus.hpp>
#include <condition_variable>
#include <memory>
#include <thread>
#include <mutex>


namespace ultrabus_examples {


    //--------------------------------------------------------------------------
    //
    class interface_foo_bar_tick : public ultrabus::interface {
    public:
        static constexpr const char* interface_name = "foo.bar.Tick";

        static std::shared_ptr<ultrabus::interface> create () {
            return std::shared_ptr<ultrabus::interface> (new interface_foo_bar_tick);
        }

        virtual ~interface_foo_bar_tick ();


    protected:
        interface_foo_bar_tick ();


    private:
        std::thread tick_thread;
        ultrabus::dbus_u32 counter;
        std::condition_variable stop_cond;
        std::mutex stop_cond_mutex;
        bool stop_tick_thread;

        void start (ultrabus::connection& bus, ultrabus::message& msg);
        void read_counter (ultrabus::connection& bus, ultrabus::message& msg);
        void stop ();
        void tick_loop ();
    };


}
#endif
