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
#ifndef ULTRABUS_OBJECT_HPP
#define ULTRABUS_OBJECT_HPP

#include <ultrabus/object_handler.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_opath.hpp>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <map>


namespace ultrabus {


    // Forward declarations
    class interface;


    /**
     * A class used for implementing DBus objects.
     */
    class object : public object_handler {
    public:
        /**
         * Creates a DBus object.
         * @param conn A DBus connection.
         * @param object_path The Object Path of the object.
         * @param enable_interface_objectmanager If <code>true</code>,
         *                                 this object will implement
         *                                 standard interface
         *                                 org.freedesktop.DBus.ObjectManager.
         * @param enable_interface_introspectable If <code>true</code>,
         *                                 this object will implement
         *                                 standard interface
         *                                 org.freedesktop.DBus.Introspectable.
         */
        object (connection& conn,
                const dbus_opath& object_path,
                const bool enable_interface_objectmanager=false,
                const bool enable_interface_introspectable=true);

        virtual ~object (); /*< Disable this object and all child nodes. */

        /**
         * Return the path of this DBus object.
         * @return A reference to an object path.
         * @note The returned reference is only valid as long
         *       as this object instance is valid.
         */
        const dbus_opath& path () const {return opath;}

        /**
         * Enable this object.
         * If this object wasn't already enabled, its parent(if
         * it has one), or the first parents parent that implements
         * standard interface org.freedesktop.DBus.ObjectManager
         * and is enabled, will emit signal
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesAdded</code></em>.
         *
         * For each child node that isn't already enabled, an
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesAdded</code></em>
         * signal is emitted. Either from this object if it implements
         * standard interface org.freedesktop.DBus.ObjectManager, or from
         * some enabled parent object implementing it.
         *
         * @param enable_sub_nodes If <code>true</code>, also
         *                         enable all child nodes.
         * @return <code>true</code> on success.<br/>
         *         <code>false</code> if not connected to a bus,
         *         if the object path can't be registered on the bus,
         *         or if one or more sub-nodes fails to be enabled.
         */
        bool enable (bool enable_sub_nodes=true);

        /**
         * Disable this object and all child nodes.
         * If this object wasn't already disabled, its parent(if
         * it has one), or the first parents parent that implements
         * standard interface org.freedesktop.DBus.ObjectManager
         * and is enabled, will emit signal
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesRemoved</code></em>.
         *
         * For each child node that isn't already disabled, an
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesRemoved</code></em>
         * signal is emitted. Either from this object if it implements
         * standard interface org.freedesktop.DBus.ObjectManager, or from
         * some enabled parent object implementing it.
         */
        void disable ();

        /**
         * Check if this object is enabled.
         * @return <code>true</code> if this object is enabled,
         * @return <code>false</code> if not.
         */
        bool is_enabled () const {return enabled;}

        /**
         * Create a child object.
         * This will create a child node that is initially disabled.
         * @param node_path The path to the child object to be
         *                  created. If the path is an absolute
         *                  path, starting with '/', it must start
         *                  with the path to this object.<br>
         *                  If the path is a relative path, not
         *                  starting with '/', its absolute path
         *                  will be prefixed with the path to this
         *                  object.
         * @param enable_interface_objectmanager If <code>true</code>,
         *                                 the child object will implement
         *                                 standard interface
         *                                 org.freedesktop.DBus.ObjectManager.
         * @param enable_interface_introspectable If <code>true</code>,
         *                                 the child object will implement
         *                                 standard interface
         *                                 org.freedesktop.DBus.Introspectable.
         * @return A pointer to a DBus object instance, or
         *         <code>nullptr</code> if the object path is invalid.
         */
        std::shared_ptr<object> create_node (const std::string& node_path,
                                             const bool enable_interface_objectmanager=false,
                                             const bool enable_interface_introspectable=true);

        /**
         * Disable and remove a child object.
         * The child object, and all of its children in turn, will
         * be disabled and removed from the object hierarchy.
         *
         * If the child node(or some of its children)
         * wasn't already disabled, an
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesRemoved</code></em>
         * signal is emitted. Either from this object if it implements
         * standard interface org.freedesktop.DBus.ObjectManager, or from
         * some enabled parent object implementing it.
         * @note This method can only be used to remove child nodes created
         *       by this instance. It will not remove a child node of a child
         *       node even if an object exists at the specified object
         *       path.
         * @param node_path A relative or absolute path to the
         *                  child object to remove.
         * @return <code>true</code> if a child node was removed,
         *         <code>false</code> if no such child node was created by
         *         this object instance.
         */
        bool remove_node (const std::string& node_path);

        /**
         * Return the bus connection used by this object.
         * @return A reference to a connection object.
         */
        connection& get_connection () {return conn;}

        /**
         * Return the bus connection used by this object.
         * @return A const reference to a connection object.
         */
        const connection& get_connection () const {return conn;}

        /**
         * Add a DBus interface to this object.
         * This will bind an interface instance to this object,
         * making this object its owner.
         *
         * If this object is enabled, its parent(if it has one),
         * or the first parents parent that implements standard
         * interface org.freedesktop.DBus.ObjectManager and is
         * enabled, will emit signal
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesAdded</code></em>.
         * @param iface The DBus interface instance to bind to this object.
         * @return <code>true</code> if the interface was added.<br/>
         *         <code>false</code> if the interface has already been
         *         added to this or some other object. Or iface
         *         is a <code>nullptr</code>.
         */
        bool add_interface (std::shared_ptr<interface> iface);

        /**
         * Remove a DBus interface from this object.
         * This will remove an interface from this object.
         * The interface that is removed will have its owner
         * set to a <code>nullptr</code>, indicating that it
         * is not bound to an object.
         *
         * If this object is enabled, its parent(if it has one),
         * or the first parents parent that implements standard
         * interface org.freedesktop.DBus.ObjectManager and is
         * enabled, will emit signal
         * <em><code>org.freedesktop.DBus.ObjectManager.InterfacesRemoved</code></em>.
         * @param interface_name The name of the interface to be removed.
         * @return <code>true</code> if the interface was removed.<br/>
         *         <code>false</code> if not such interface was found.
         */
        bool remove_interface (const std::string& interface_name);

        /**
         * Check if this object implements standard interface
         * org.freedesktop.DBus.Introspectable.
         */
        bool has_interface_introspectable () const {return implements_introspectable;}

        /**
         * Check if this object implements standard interface
         * org.freedesktop.DBus.Properties.
         */
        bool has_interface_properties () const {return implements_properties;}

        /**
         * Check if this object implements standard interface
         * org.freedesktop.DBus.ObjectManager.
         */
        bool has_interface_objectmanager () const {return implements_objectmanager;}

        /**
         * Enable standard interface org.freedesktop.DBus.Properties
         * for this object.
         */
        void enable_interface_properties ();

        /**
         * Return a pointer to the parent of this object.
         * @return A pointer to the parent of this object,
         *         a <code>nullptr</code> if this object
         *         doesn't have a parent.
         */
        object* parent () {return parent_obj;}

        /**
         * Return a pointer to the parent of this object.
         * @return A pointer to the parent of this object,
         *         a <code>nullptr</code> if this object
         *         doesn't have a parent.
         */
        const object* parent () const {return parent_obj;}

        /**
         * Try to find an object that may be somewhere down in the object tree.
         * @param path An absolute or relative path to the
         *             DBus object we are searching for.
         * @return A pointer to the object we found, or
         *         a <code>nullptr</code> if none was found.
         */
        std::shared_ptr<object> find (const std::string& path) const;


    protected:
        virtual bool on_method_call (message& msg);


    private:
        bool on_method_call_without_interface_in_msg (message& msg);
        void emit_all_interfaces_added ();
        void emit_all_interfaces_removed ();
        void emit_interface_added (interface* iface);
        void emit_interface_added (const std::string& interface_name);
        void emit_interface_removed (const std::string& iface_name);
        void enable_interface_properties_impl ();

        bool on_introspect_method (message& msg);
        bool on_properties_get_set_method (message& msg, bool get_value);
        bool on_properties_get_all_method (message& msg);
        bool on_properties_method (message& msg);
        bool on_objectmanager_method (message& msg);
        void get_all_objects_ifaces_properties (dbus_opath_dict& dict);

        dbus_opath opath;
        object* parent_obj;
        std::map<std::string, std::shared_ptr<object>> nodes;
        std::map<std::string, std::shared_ptr<interface>> interfaces;
        mutable std::mutex node_mutex;
        std::mutex interfaces_mutex;
        bool implements_properties;
        bool implements_objectmanager;
        bool implements_introspectable;
        bool enabled;
    };


} // namespace ultrabus
#endif
