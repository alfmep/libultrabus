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
#ifndef ULTRABUS_ORG_FREEDESKTOP_DBUS_OBJECTMANAGER_HPP
#define ULTRABUS_ORG_FREEDESKTOP_DBUS_OBJECTMANAGER_HPP

#include <ultrabus/connection.hpp>
#include <ultrabus/message_filter.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/retvalue.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <string>
#include <mutex>
#include <map>


namespace ultrabus {


    /**
     * Object-manager state indexed by object path and interface name.
     *
     * The outer key is an object path; each value maps interface names to the
     * corresponding property-name/value dictionary. The alias owns all of its
     * keys and values.
     * <code>map&lt;object_path, map&lt;interface, properties&gt;&gt;</code>.<br/>
     * - Object/path/1
     *   - Interface.1
     *     - Property_1
     *       - property_1_name
     *       - property_1_value
     *     - Property_2
     *       - property_2_name
     *       - property_2_value
     *   - Interface.2
     *     - Property_1
     *       - property_1_name
     *       - property_1_value
     * - Object/path/2
     *   - Interface.1
     *     - Property_1
     *       - property_1_name
     *       - property_1_value
     * - ...
     */
    using managed_objects_t = std::map<std::string, std::map<std::string, dbus_string_dict>>;
    //                                 opath                 iface        properties


    /**
     * Proxy for the standard <code>org.freedesktop.DBus.ObjectManager</code>
     * interface.
     *
     * The proxy retains a non-owning reference to its connection, which must
     * outlive the proxy.
     *
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#standard-interfaces-objectmanager
     *       rel="noopener noreferrer" target="_blank">D-Bus Specification - org.freedesktop.DBus.ObjectManager</a>
     */
    class org_freedesktop_DBus_ObjectManager : public message_filter {
    public:

        /**
         * Callback for an added object or newly added interfaces.
         *
         * @param opath The object path of the added/changed object.
         * @param ifaces Map of added interface names to their properties. The
         *               reference is valid only for the callback invocation.
         */
        using iface_added_cb = std::function<void (const std::string& opath,
                                                   std::map<std::string, dbus_string_dict>& ifaces)>;

        /**
         * Callback for a removed object or removed interfaces.
         *
         * @param opath The object that is removed or has lost interfaces.
         * @param ifaces Removed interface names. The reference is valid only
         *               for the callback invocation.
         */
        using iface_removed_cb = std::function<void (const std::string& opath,
                                                     std::set<std::string>& ifaces)>;


        /**
         * Construct an object-manager proxy.
         *
         * @param conn Connection used for requests and signals. It is not
         *             owned and must outlive this proxy.
         */
        org_freedesktop_DBus_ObjectManager (connection& conn) : message_filter(conn) {}

        /**
         * Retrieve all objects, interfaces, and properties managed by a
         * remote object implementing <code>org.freedesktop.DBus.ObjectManager</code>.
         * @param bus_name The bus name of the service managing the objects.
         * @param opath The object path of the object manager.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return Managed objects on success; otherwise a retvalue whose err()
         *         and what() describe the failure.
         * @see retvalue
         */
        retvalue<managed_objects_t> get_managed_objects (
                const std::string& bus_name,
                const std::string& opath,
                int timeout=DBUS_TIMEOUT_USE_DEFAULT);

        /**
         * Asynchronous call to retrieve all objects, interfaces, and
         * properties managed by a remote object implementing
         * <code>org.freedesktop.DBus.ObjectManager</code>.
         * This method queues a message on the message bus and returns immediately,
         * the result is handled in a callback function.
         * @param bus_name The bus name of the service managing the objects.
         * @param opath The object path of the object manager.
         * @param callback Callback invoked when a result is received. The
         *                 connection retains it until delivery; its result
         *                 reference is valid only during the callback.
         *                 <br/>The parameter to the callback is the
         *                 same as the return value from the
         *                 corresponding synchronous method.
         * @param timeout Timeout in milliseconds, or
         *                DBUS_TIMEOUT_USE_DEFAULT to use the bus default.
         * @return <code>true</code> if the message was queued on the message bus,
         *         <code>false</code> if failing to queue the message.
         */
        bool get_managed_objects (const std::string& bus_name,
                                  const std::string& opath,
                                  std::function<void (retvalue<managed_objects_t>& result)> callback,
                                  int timeout=DBUS_TIMEOUT_USE_DEFAULT);


        /**
         * Set or remove the callback for added objects and interfaces.
         * If a callback is already set for this service and object path,
         * it will be replaced with this callback.
         *
         * @param bus_name The bus name of the service managing the objects.
         * @param opath The path of the parent object we want
         *              to receive information about.
         * @param callback A callback function. If this is a
         *                 <code>nullptr</code>, the callback
         *                 will be removed if previously set. A non-null
         *                 callback is retained until it is replaced, removed,
         *                 or this proxy is destroyed.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue when the bus name or object path is invalid
         *         or the callback cannot be configured.
         *
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-object-path
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Valid Object Paths</a>
         */
        retvalue<bool> set_interfaces_added_callback (
                const std::string& bus_name,
                const std::string& opath,
                iface_added_cb callback);

        /**
         * Set or remove the callback for removed objects and interfaces.
         * If a callback is already set for this service and object path,
         * it will be replaced with this callback.
         *
         * @param bus_name The bus name of the service managing the objects.
         * @param opath The path of the parent object we want
         *              to receive information about.
         * @param callback A callback function. If this is a
         *                 <code>nullptr</code>, the callback
         *                 will be removed if previously set. A non-null
         *                 callback is retained until it is replaced, removed,
         *                 or this proxy is destroyed.
         * @return A successful retvalue containing <code>true</code>, or an
         *         error retvalue when the bus name or object path is invalid
         *         or the callback cannot be configured.
         *
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-bus-names
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Message Bus Names</a>
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-object-path
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Valid Object Paths</a>
         */
        retvalue<bool> set_interfaces_removed_callback (
                const std::string& bus_name,
                const std::string& opath,
                iface_removed_cb callback);


    protected:
        virtual bool on_signal (message& msg);


    private:
        std::mutex iface_mutex;
        std::map<std::pair<std::string, std::string>, iface_added_cb>   iface_added_callbacks;
        std::map<std::pair<std::string, std::string>, iface_removed_cb> iface_removed_callbacks;
        //                <bus_name     opath>        callback

        retvalue<bool> set_interfaces_added_callback_impl (
                const std::string& bus_name,
                const std::string& opath,
                iface_added_cb callback);
        retvalue<bool> set_interfaces_removed_callback_impl (
                const std::string& bus_name,
                const std::string& opath,
                iface_removed_cb callback);
        void handle_added_ifaces (message& msg, iface_added_cb cb);
        void handle_removed_ifaces (message& msg, iface_removed_cb cb);
    };


}
#endif
