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
#ifndef ULTRABUS_INTERFACE_HPP
#define ULTRABUS_INTERFACE_HPP

#include <ultrabus/object.hpp>
#include <ultrabus/connection.hpp>
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <tuple>
#include <map>


namespace ultrabus {


    /**
     * A class used for implementing DBus interfaces.
     */
    class interface {
    public:
        /**
         * A callback used for implementing DBus method calls.
         * @param conn A DBus connection object.
         * @param msg The DBus method call message.
         * @note It is the responsibility of this callback
         *       to send a reply message when it has finished
         *       doing what it is suppose to do. Either a normal
         *       reply with or without arguments, or an
         *       error reply.<br/>
         *       If no reply is sent, the entity making the
         *       DBus method call will have to wait until it
         *       gets a timeout (no response) error.<br/>
         *       <br/>
         *       An example of the simplest way of sending a reply
         *       from within the callback:
         *       <pre>
         *       conn.send (msg.make_reply());
         *       </pre>
         *       A simple example of sending an error reply from within
         *       the callback:
         *       <pre>
         *       conn.send (msg.make_reply("an.error.name", "An error description"));
         *       </pre>
         */
        using iface_method_cb_t = std::function<void (connection& conn, message& msg)>;

        /**
         * A callback used for getting an interface property value.
         * When implementing this callback, <code>true</code> must
         * be returned if the callback could successfully assign
         * the <code>value</code> parameter with the property value.<br/>
         * If this callback returns <code>false</code>, the entity trying
         * to get the property value will instead get an error response.
         * @param name The name of the property.
         * @param value A <code>dbus_variant</code> that will be
         *              assigned the property value.
         * @return <code>true</code> if the value could be retrieved
         *         without error. <code>false</code> if the property
         *         couldn't be retrieved for some reason.
         */
        using get_property_cb_t = std::function<bool (const std::string& name,
                                                      dbus_variant& value)>;

        /**
         * A callback used for setting an interface property value.
         * When implementing this callback, <code>true</code> must
         * be returned if the callback could successfully assign
         * the new value to the property.<br/>
         * If this callback returns <code>false</code>, the entity trying
         * to set the property value will instead get an error response.
         * @param name The name of the property.
         * @param value A <code>dbus_variant</code> that contains
         *              the value to be assigned to the property.
         * @return <code>true</code> if the value could be assigned
         *         without error. <code>false</code> if the property
         *         couldn't be assigned the new value for some reason.
         */
        using set_property_cb_t = std::function<bool (const std::string& name,
                                                      const dbus_variant& value)>;

        /**
         * Create an instance of this class.
         * @param interface_name The name of the newly created interface.
         *                       This must be a valid DBus interface name.
         * @return A shared pointer to an <code>interface</code> instance.
         *         Or a <code>nullptr</code> if the interface name isn't
         *         a valid DBus interface name.
         */
        static std::shared_ptr<interface> create (const std::string& interface_name);

        virtual ~interface (); /**< Destructor. Free resources. */

        /**
         * Get the name of this DBus interface.
         * @return A const reference to the DBus interface name.
         * @note The returned reference to the interface name is
         *       only valid for the lifetime of this instance.
         */
        const std::string& name () const {return if_name;}

        /**
         * Add a DBus method to this interface that takes no
         * arguments and doesn't return a value.
         * Note that the supplied callback must still send back
         * a reply, even if that reply doesn't include a return value
         * as argument.
         * @param name The name of the DBus method we wish to add.
         * @param callback A callback that will be called whenever
         *                 some other enitiy on the Dbus calls this
         *                 method.
         * @return <code>true</code> if the method was successfully added.<br/>
         *         <code>false</code> if the method name isn't a valid DBus
         *         method call name, if a method with the same name has
         *         already been added to this interface, or if
         *         <code>callback</code> is a <code>nullptr</code>.
         * @see iface_method_cb_t A callback implementing the DBus method call.
         */
        bool add_method (const std::string& name,
                         iface_method_cb_t callback);


        /**
         * Add a DBus method to this interface that may take
         * one argument and may include a return value in the reply.
         * @param name The name of the DBus method we wish to add.
         * @param in_param A string describing the input parameter to the
         *                 method. This description can be in one of
         *                 three forms:
         *                 <ul>
         *                   <li>"" (an empty string)</li>
         *                   <li>"signature"</li>
         *                   <li>"name:signature"</li>
         *                 </ul>
         *                 The first form, an empty string, means that
         *                 the DBus method doesn't have an input parameter.<br/>
         *                 The second form is a DBus signature of a single
         *                 DBus data type.<br/>
         *                 The third form is a descriptive name of the
         *                 parameter, followed by a colon(:), followed by
         *                 a DBus signature of a single DBus data type.<br/>
         *                 When using the third form, the XML output from
         *                 a call to
         *                 <code>org.freedesktop.DBus.Introspectable.Introspect</code>
         *                 will include the name given to the parameter,
         *                 making the introspect data a bit more descriptive.
         *                 Note that the parameter name must be a valid DBus
         *                 member name.
         * @param out_param A string describing the return value of the
         *                  method. See <code>in_param</code> for the
         *                  format of this argument.<br>
         *                  An empty string means that the method doesn't
         *                  include a return value in the reply.
         * @param callback A callback that will be called whenever
         *                 some other enitiy on the Dbus calls this
         *                 method.
         * @return <code>true</code> if the method is successfully added
         *         to the interface.<br/>
         *         <code>false</code> if a method with the same name is already
         *         added, if the name isn't a valid DBus method name, if any
         *         in/out parameter description is invalid, or if
         *         <code>callback</code> is a <code>nullptr</code>
         * @see iface_method_cb_t A callback implementing the DBus method call.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Type System</a>
         */
        bool add_method (const std::string& name,
                         const std::string& in_param,
                         const std::string& out_param,
                         iface_method_cb_t callback);

        /**
         * Add a DBus method to this interface that may take
         * one argument and may include a return value in the reply.
         * @param name The name of the DBus method we wish to add.
         * @param in_params A vector of strings that describes all input
         *                 parameters to this method. If the vector is
         *                 empty, this method doesn't have an input
         *                 parameter.<br/>
         *                 Each string in the vector is a description of
         *                 an input parameter. This description can be
         *                 in one of three forms:
         *                 <ul>
         *                   <li>"" (an empty string)</li>
         *                   <li>"signature"</li>
         *                   <li>"name:signature"</li>
         *                 </ul>
         *                 The first form, an empty string, is simply
         *                 ignored, not counted as an input parameter.<br/>
         *                 The second form is a DBus signature of a single
         *                 DBus data type.<br/>
         *                 The third form is a descriptive name of the
         *                 parameter, followed by a colon(:), followed by
         *                 a DBus signature of a single DBus data type.<br/>
         *                 When using the third form, the XML output from
         *                 a call to
         *                 <code>org.freedesktop.DBus.Introspectable.Introspect</code>
         *                 will include the name given to the parameter,
         *                 making the introspect data a bit more descriptive.
         *                 Note that the parameter name must be a valid DBus
         *                 member name.
         * @param out_param A string describing the return value of the
         *                  method. See <code>in_param</code> for the
         *                  format of this argument.<br>
         *                  An empty string means that the method doesn't
         *                  include a return value in the reply.
         * @param callback A callback that will be called whenever
         *                 some other enitiy on the Dbus calls this
         *                 method.
         * @return <code>true</code> if the method is successfully added
         *         to the interface.<br/>
         *         <code>false</code> if a method with the same name is already
         *         added, if the name isn't a valid DBus method name, if any
         *         in/out parameter description is invalid, or if
         *         <code>callback</code> is a <code>nullptr</code>
         * @see iface_method_cb_t A callback implementing the DBus method call.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#type-system
         *       rel="noopener noreferrer" target="_blank">D-Bus Specification - Type System</a>
         */
        bool add_method (const std::string& name,
                         const std::vector<std::string>& in_params,
                         const std::string& out_param,
                         iface_method_cb_t callback);

        /**
         * Remove a DBus method from this interface.
         * @param name The name of the DBus method
         *             to remove from the interface.
         * @return <code>true</code> if the method was removed.
         *         <code>false</code> if no such method was found.
         */
        bool remove_method (const std::string& name);

        /**
         * Check if the interface has a specific DBus method.
         * @param name The name of the DBus method.
         * @return <code>true</code> if this interface has
         *         a DBus method with the given name.<br/>
         *         <code>false</code> if not.
         */
        bool has_method (const std::string& name) const;


        /**
         * Add a DBus property to this interface.
         * This adds a named property of a specific DBus data type to the
         * interface. The property value can be read-write, read only,
         * or write-only.<br/>
         * The initial value of the property will be the default value
         * of the precified DBus data type.<br/>
         * The property value can be set using method <code>set_property()</code>,
         * and be read using method <code>get_property()</code>. Or it
         * can be directly modified by getting a pointer to the property
         * value using method <code>property()</code>.<br/>
         * When using method <code>set_property()</code> to update the
         * value of the property, a
         * <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal will automatically be emitted from the DBus object owning
         * this interface.
         * Note that if the property is read-only, using method
         * <code>set_property()</code> to change the value will fail. To
         * change the value of a read-only property, use method
         * <code>property()</code> to get a pointer to the property value.
         *
         * The name, type, and access permission of the added property will
         * be included in the introspect data returned by the DBus object
         * owning this interface.
         *
         * @param name The name of the property to add.
         * @param signature The DBus signature of the property data type.
         * @param readable <code>true</code> if the property can be read,
         *                 <code>false</code> if not.
         * @param writable <code>true</code> if the property can be written,
         *                 <code>false</code> if not.
         * @param disable_emit_on_set_property Set this parameter to
         *                 <code>true</code> if we, for some reason,
         *                 don't want to automatically emit a
         *                 <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *                 signal when the value of the property is changed.
         * @return <code>true</code> if successfully added.<br/>
         *         <code>false</code> if a property with the same name is
         *         already added, if the property name is an invalid DBus name,
         *         if the signature isn't a valid DBus signature of a single
         *         data type, if both <code>readable</code> and
         *         <code>writable</code> are set to <code>false</code>.
         * @see ultrabus::object::add_interface
         */
        bool add_property (const std::string& name,
                           const std::string& signature,
                           const bool readable=true,
                           const bool writable=false,
                           const bool disable_emit_on_set_property=false);

        /**
         * Add a DBus property to this interface.
         * This adds a named property of a specific DBus data type to the
         * interface. The property value can be read-write, read only,
         * or write-only.<br/>
         * The DBus data type and the initial value of the property is
         * the same as the parameter <code>initial_value</code>.
         * The property value can be set using method <code>set_property()</code>,
         * and be read using method <code>get_property()</code>. Or it
         * can be directly modified by getting a pointer to the property
         * value using method <code>property()</code>.<br/>
         * When using method <code>set_property()</code> to update the
         * value of the property, a
         * <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal will automatically be emitted from the DBus object owning
         * this interface.
         * Note that if the property is read-only, using method
         * <code>set_property()</code> to change the value will fail. To
         * change the value of a read-only property, use method
         * <code>property()</code> to get a pointer to the property value.
         *
         * The name, type, and access permission of the added property will
         * be included in the introspect data returned by the DBus object
         * owning this interface.
         *
         * @param name The name of the property to add.
         * @param initial_value The initial value of the new property.
         * @param readable <code>true</code> if the property can be read,
         *                 <code>false</code> if not.
         * @param writable <code>true</code> if the property can be written,
         *                 <code>false</code> if not.
         * @param disable_emit_on_set_property Set this parameter to
         *                 <code>true</code> if we, for some reason,
         *                 don't want to automatically emit a
         *                 <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *                 signal when the value of the property is changed.
         * @return <code>true</code> if successfully added.<br/>
         *         <code>false</code> if a property with the same name is
         *         already added, if the property name is an invalid DBus name,
         *         if the signature isn't a valid DBus signature of a single
         *         data type, if both <code>readable</code> and
         *         <code>writable</code> are set to <code>false</code>.
         * @see ultrabus::object::add_interface
         */
        bool add_property (const std::string& name,
                           const dbus_type& initial_value,
                           const bool readable=true,
                           const bool writable=false,
                           const bool disable_emit_on_set_property=false);

        /**
         * Add a DBus property to this interface.
         * This adds a named property of a specific DBus data type to the
         * interface. The property value can be read-write, read only,
         * or write-only.<br/>
         * The DBus data type and the initial value of the property is
         * the same as the parameter <code>initial_value</code>.
         * The property value can be set using method <code>set_property()</code>,
         * and be read using method <code>get_property()</code>. Or it
         * can be directly modified by getting a pointer to the property
         * value using method <code>property()</code>.<br/>
         * When using method <code>set_property()</code> to update the
         * value of the property, a
         * <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal will automatically be emitted from the DBus object owning
         * this interface.
         * Note that if the property is read-only, using method
         * <code>set_property()</code> to change the value will fail. To
         * change the value of a read-only property, use method
         * <code>property()</code> to get a pointer to the property value.
         *
         * The name, type, and access permission of the added property will
         * be included in the introspect data returned by the DBus object
         * owning this interface.
         *
         * @param name The name of the property to add.
         * @param initial_value The initial value of the new property.
         * @param readable <code>true</code> if the property can be read,
         *                 <code>false</code> if not.
         * @param writable <code>true</code> if the property can be written,
         *                 <code>false</code> if not.
         * @param disable_emit_on_set_property Set this parameter to
         *                 <code>true</code> if we, for some reason,
         *                 don't want to automatically emit a
         *                 <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *                 signal when the value of the property is changed.
         * @return <code>true</code> if successfully added.<br/>
         *         <code>false</code> if a property with the same name is
         *         already added, if the property name is an invalid DBus name,
         *         if the signature isn't a valid DBus signature of a single
         *         data type, if both <code>readable</code> and
         *         <code>writable</code> are set to <code>false</code>.
         * @see ultrabus::object::add_interface
         */
        bool add_property (const std::string& name,
                           dbus_type&& initial_value,
                           const bool readable=true,
                           const bool writable=false,
                           const bool disable_emit_on_set_property=false);

        /**
         * Add a DBus property to this interface.
         * This adds a named property of a specific DBus data type to the
         * interface. The property value can be read-write, read only,
         * or write-only.
         *
         * Setting and getting the value of this property is handled
         * in the supplied callback funtions.
         *
         * Use this method to add a property when we want to have more
         * control over what happens when the property value is read or
         * written, for example when we want to have a dynamic property
         * value that is determined at the time it is read.
         *
         * The property value can be set using method <code>set_property()</code>,
         * and be read using method <code>get_property()</code>.
         * Using <code>set_property()</code> and <code>get_property()</code>
         * will trigger calls to the supplied callback functions for
         * implementation specific handling of the property value.<br/>
         *
         * The name, type, and access permission of the added property will
         * be included in the introspect data returned by the DBus object
         * owning this interface.
         * @note When using this method to add a property, the method
         *       <code>property()</code> may return a pointer to a property
         *       value that isn't used or updated at all, depending on the
         *       implementation in the supplied callback functions.
         * @note The property added this way will not trigger a
         *       <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *       signal when the value is set using method <code>set_property()</code>.
         *       It is the responsibility of the <code>set_callback</code>
         *       function to emit this signal using
         *       <code>emit_property_changed()</code> if needed.
         *
         * @param name The name of the property to add.
         * @param signature The DBus signature of the property data type.
         * @param get_callback A callback function that is called to read
         *                     the value of the property. If this parameter
         *                     is a <code>nullptr</code>, the value is not
         *                     readable.
         * @param set_callback A callback function that is called to write
         *                     the value of the property. If this parameter
         *                     is a <code>nullptr</code>, the value is not
         *                     writeable.
         * @return <code>true</code> if successfully added.<br/>
         *         <code>false</code> if a property with the same name is
         *         already added, if the property name is an invalid DBus name,
         *         if the signature isn't a valid DBus signature of a single
         *         data type, if both <code>get_callback</code> and
         *         <code>get_callback</code> are <code>nullptr</code>.
         * @see ultrabus::object::add_interface
         */
        bool add_property (const std::string& name,
                           const std::string& signature,
                           get_property_cb_t get_callback,
                           set_property_cb_t set_callback=nullptr);

        /**
         * Remove a property from this interface.
         * @param name The name of the property to remove.
         * @param emit_signal Set this parameter to <code>false</code>
         *                    if we, for some reason, don't want to emit a
         *       <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *                    signal when the property is removed.
         * @return <code>true</code> if the property was removed.<br/>
         *         <code>false</code> if this interface doesn't have
         *         a property with this name.
         */
        bool remove_property (const std::string& name, bool emit_signal=true);

        /**
         * Get the value of a property.
         * @param name The name of the property.
         * @param value If the property is successfully read,
         *              this parameter will contain the current
         *              value of the property.
         * @return One of:
         *         -  0 The property value was successfully read.
         *         - -1 Property not found.
         *         - -2 The property is write-only.
         *         - -3 The property isn't of the same DBus
         *              data type as parameter <code>value</code>.
         *         - -4 Error getting the property value (the
         *              <code>get_callback</code> callback
         *              function returned <code>false</code>).
         */
        int get_property (const std::string& name, dbus_type& value) const;

        /**
         * Get the value of a property.
         * @param name The name of the property.
         * @param value If the property is successfully read,
         *              this parameter will contain the current
         *              value of the property.
         * @return One of:
         *         -  0 The property value was successfully read.
         *         - -1 Property not found.
         *         - -2 The property is write-only.
         *         - -3 The property isn't of the same DBus
         *              data type as parameter <code>value</code>.
         *         - -4 Error getting the property value (the
         *              <code>get_callback</code> callback
         *              function returned <code>false</code>).
         */
        template<dbus_basic_cpp_types T>
        int get_property (const std::string& name, T& value) const {
            dbus_basic<T> v;
            int retval = get_property (name, v);
            if (retval == 0)
                value = v.get ();
            return retval;
        }

        /**
         * Set the value of a property.
         * @param name The name of the property.
         * @param value The new value of the property.
         * @param emit_if_changed Set this parameter to <code>false</code>
         *                        if we, for some reason, don't want to emit a
         *         <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *                        signal if the value is changed.
         * @return One of:
         *         -  0 The property value was successfully set.
         *         - -1 Property not found.
         *         - -2 The property is read-only.
         *         - -3 The property isn't of the same DBus
         *              data type as parameter <code>value</code>.
         *         - -4 Error setting the property value (the
         *              <code>set_callback</code> callback
         *              function returned <code>false</code>).
         */
        int set_property (const std::string& name, const dbus_type& value, bool emit_if_changed=true);

        /**
         * Set the value of a property.
         * @param name The name of the property.
         * @param value The new value of the property.
         * @param emit_if_changed Set this parameter to <code>false</code>
         *                        if we, for some reason, don't want to emit a
         *         <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         *                        signal if the value is changed.
         * @return One of:
         *         -  0 The property value was successfully set.
         *         - -1 Property not found.
         *         - -2 The property is read-only.
         *         - -3 The property isn't of the same DBus
         *              data type as parameter <code>value</code>.
         *         - -4 Error setting the property value (the
         *              <code>set_callback</code> callback
         *              function returned <code>false</code>).
         */
        template<dbus_basic_cpp_types T>
        int set_property (const std::string& name, const T& value, bool emit_if_changed=true) {
            return set_property (name, dbus_basic<T>(value), emit_if_changed);
        }

        /**
         * Get all properties of this interface as a DBus dict.
         * @return A DBus dict containing all properties of this
         *         interface with the current values.
         * @note Properties that are write-only will not be included
         *       in the returned dict.
         */
        dbus_string_dict get_all_properties () const;

        /**
         * Check if this interface instance has any properties.
         * @return <code>true</code> if this interface
         *         instance has one or more properties.
         */
        bool has_properties () const;

        /**
         * Check if this interface instance has a property
         * with a given name.
         * @param name The name of the property we are interested in.
         * @return <code>true</code> if this interface
         *         instance has a property with the given name.
         */
        bool has_property (const std::string name) const;

        /**
         * Return a pointer to a <code>dbus_variant</code>
         * conatining the value of the named interface property.
         * @param name The name of the property we are interested in.
         * @return A pointer to the <code>dbus_variant</code> containing
         *         the property value. Or a <code>nullptr</code> if
         *         no such property is found.
         * @note If the property was created with callbacks getting
         *       and setting the value, the actual <code>dbus_variant</code>
         *       instance pointed to by the return value may not
         *       be used at all by this interface instance.
         */
        dbus_variant* property (const std::string& name);
        /**
         * Return a pointer to a const <code>dbus_variant</code>
         * conatining the value of the named interface property.
         * @param name The name of the property we are interested in.
         * @return A pointer to a const <code>dbus_variant</code>
         *         containing the property value. Or a
         *         <code>nullptr</code> if no such property is found.
         * @note If the property was created with callbacks getting
         *       and setting the value, the actual <code>dbus_variant</code>
         *       instance pointed to by the return value may not
         *       be used at all by this interface instance.
         */
        const dbus_variant* property (const std::string& name) const;

        /**
         * Return a pointer to a <code>dbus_variant</code>
         * conatining the value of the named interface property.
         * @note This method doesn't lock the mutex protecting
         *       the set of properties in this class.<br/>
         *       This method is only intended to be called from
         *       withing a callback that gets of sets a property value.
         * @param name The name of the property we are interested in.
         * @return A pointer to the <code>dbus_variant</code> containing
         *         the property value. Or a <code>nullptr</code> if
         *         no such property is found.
         * @note If the property was created with callbacks getting
         *       and setting the value, the actual <code>dbus_variant</code>
         *       instance pointed to by the return value may not
         *       be used at all by this interface instance.
         */
        dbus_variant* property_nolock (const std::string& name);

        /**
         * Return a pointer to a const <code>dbus_variant</code>
         * conatining the value of the named interface property.
         * @note This method doesn't lock the mutex protecting
         *       the set of properties in this class.<br/>
         *       This method is only intended to be called from
         *       withing a callback that gets of sets a property value.
         * @param name The name of the property we are interested in.
         * @return A pointer to a const <code>dbus_variant</code>
         *         containing the property value. Or a
         *         <code>nullptr</code> if no such property is found.
         * @note If the property was created with callbacks getting
         *       and setting the value, the actual <code>dbus_variant</code>
         *       instance pointed to by the return value may not
         *       be used at all by this interface instance.
         */
        const dbus_variant* property_nolock (const std::string& name) const;

        /**
         * Emit a <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal containing the named property with its current value.
         * This will read the current value of the named property and
         * send a <code>PropertiesChanged</code> signal including that
         * property.
         * @param property_name The name of the property that will be
         *                      included in the DBus signal.
         * @return <code>true</code> if a signal was successfully sent.<br>
         *         <code>false</code> if the property couldn't be read or
         *         a signal couldn't be sent.
         */
        bool emit_property_changed (const std::string& property_name);

        /**
         * Emit a <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal containing a property name and value.
         * @note This method will sent a signal containing the property name and
         *       value regardless if this interface has a property with that
         *       name or not, and regardless of the actual type and value of
         *       the property if this interface has such a property.
         * @param property_name The name of the property that will be
         *                      included in the DBus signal.
         * @param value The value of the property in the signal we are
         *              sending.
         * @return <code>true</code> if a signal was successfully sent.<br>
         *         <code>false</code> if a signal couldn't be sent.
         */
        bool emit_property_changed (const std::string& property_name, const dbus_type& value);

        /**
         * Emit a <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal containing a property name and value.
         * @note This method will sent a signal containing the property name and
         *       value regardless if this interface has a property with that
         *       name or not, and regardless of the actual type and value of
         *       the property if this interface has such a property.
         * @param property_name The name of the property that will be
         *                      included in the DBus signal.
         * @param value The value of the property in the signal we are
         *              sending.
         * @return <code>true</code> if a signal was successfully sent.<br>
         *         <code>false</code> if a signal couldn't be sent.
         */
        template<dbus_basic_cpp_types T>
        bool emit_property_changed (const std::string& property_name, const T& value) {
            return emit_property_changed (property_name, dbus_basic<T>(value));
        }

        /**
         * Emit a <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal containing a property name that is included in the
         * <code>invalidated_properties</code> argument of the signal.
         * @note This method will sent a signal containing the property name
         *       regardless if this interface has a property with that
         *       name or not.
         * @param property_name The name of the property that will
         *                      be marked as invalid in the signal.
         * @return <code>true</code> if a signal was successfully sent.<br>
         *         <code>false</code> if a signal couldn't be sent.
         */
        bool emit_property_invalid (const std::string& property_name);

        /**
         * Emit a <em><code>org.freedesktop.DBus.Properties.PropertiesChanged</code></em>
         * signal containing changed and/or invalid properties.
         * This will read the current values of the properties in
         * the <code>property_names</code> parameter and
         * send a <code>PropertiesChanged</code> signal including
         * those properties. And optionally include a list of
         * invalid property names in the signal
         * @param property_names A vector containing the names of the properties
         *                       we want to include in the
         *                       <code>changed_properties</code>
         *                       argument in the signal.
         * @param invalid_names A vector containing the names of the properties
         *                       we want to include in the
         *                       <code>invalidated_properties</code>
         *                       argument in the signal.
         */
        bool emit_properties_changed (const std::vector<std::string>& property_names,
                                      const std::vector<std::string>& invalid_names={});


        /**
         * Add a signal that can be emitted from this interface.
         * This adds a signal without arguments that can be sent
         * from the interface.
         * @param name The name of the signal.
         * @return <code>true</code> if the signal was successfully added.<br>
         *         <code>false</code> if a signal with the same name was
         *         already added or if the name is an invalid DBus signal name.
         */
        bool add_signal (const std::string& name);

        /**
         * Add a signal that can be emitted from this interface.
         * This adds a signal with one argument that can be sent
         * from the interface.
         * @param name The name of the signal.
         * @param argument A string describing the argument in the signal.
         *                 This description can be in one of
         *                 three forms:
         *                 <ul>
         *                   <li>"" (an empty string)</li>
         *                   <li>"signature"</li>
         *                   <li>"name:signature"</li>
         *                 </ul>
         *                 The first form, an empty string, means that
         *                 the signal doesn't have an argument.<br/>
         *                 The second form is a DBus signature of a single
         *                 DBus data type.<br/>
         *                 The third form is a descriptive name of the
         *                 argument, followed by a colon(:), followed by
         *                 a DBus signature of a single DBus data type.<br/>
         *                 When using the third form, the XML output from
         *                 a call to
         *                 <code>org.freedesktop.DBus.Introspectable.Introspect</code>
         *                 will include the name given to the argument,
         *                 making the introspect data a bit more descriptive.
         *                 Note that the argument name must be a valid DBus
         *                 member name.
         * @return <code>true</code> if the signal was successfully added.<br>
         *         <code>false</code> if a signal with the same name was
         *         already added, if the name is an invalid DBus signal name,
         *         or if the signature isn't a valid signature of a single
         *         DBus data type.
         */
        bool add_signal (const std::string& name, const std::string& argument);

        /**
         * Add a signal that can be emitted from this interface.
         * This adds a signal with multiple arguments that can
         * be sent from the interface.
         * @param name The name of the signal.
         * @param arguments A vector of strings describing all arguments
         *                 in the signal.<br/>
         *                 Each string in the vector is a description of
         *                 a aignal argument. This description can be
         *                 in one of three forms:
         *                 <ul>
         *                   <li>"" (an empty string)</li>
         *                   <li>"signature"</li>
         *                   <li>"name:signature"</li>
         *                 </ul>
         *                 The first form, an empty string, is simply
         *                 ignored, not counted as a signal argument.<br/>
         *                 The second form is a DBus signature of a single
         *                 DBus data type.<br/>
         *                 The third form is a descriptive name of the
         *                 argument, followed by a colon(:), followed by
         *                 a DBus signature of a single DBus data type.<br/>
         *                 When using the third form, the XML output from
         *                 a call to
         *                 <code>org.freedesktop.DBus.Introspectable.Introspect</code>
         *                 will include the name given to the argument,
         *                 making the introspect data a bit more descriptive.
         *                 Note that the argument name must be a valid DBus
         *                 member name.
         * @return <code>true</code> if the signal was successfully added.<br>
         *         <code>false</code> if a signal with the same name was
         *         already added, if the name is an invalid DBus signal name,
         *         or if an argument signature isn't a valid signature of a
         *         single DBus data type.
         */
        bool add_signal (const std::string& name, const std::vector<std::string>& arguments);

        /**
         * Remove a signal from the set of signals that can
         * be sent from this interface.
         * @param name The name of the signal to remove.
         * @return <code>true</code> if the signal was removed.
         *         <code>false</code> if no such signal was found.
         */
        bool remove_signal (const std::string& name);

        /**
         * Check if the interface can emit a specific signal.
         * @param name The name of the DBus signal.
         * @return <code>true</code> if this interface can emit
         *         a DBus signal with the given name.<br/>
         *         <code>false</code> if not.
         */
        bool has_signal (const std::string& name) const;

        /**
         * Send a DBus signal from this interface.
         * This will send a DBus signal from the object owning this
         * interface.
         * @param name The name of the signal to send.
         * @return <code>true</code> if the signal was sent.<br/>
         *         <code>false</code> if a signal with this name
         *         hasn't been added to this interface, of if
         *         the registered signal has different arguments,
         *         or if this interface instance isn't added to
         *         an object that is currently enabled.
         */
        bool emit_signal (const std::string& name) {
            std::scoped_lock lock (obj_mutex, signals_mutex);
            if (!obj || dbus_validate_member(name.c_str(), nullptr) != TRUE)
                return false;
            message signal (obj->path(), if_name, name);
            return emit_signal_impl (signal);
        }

        /**
         * Send a DBus signal from this interface.
         * This will send a DBus signal from the object owning this
         * interface.
         * @param name The name of the signal to send.
         * @param arg The first argument in the signal.
         * @param args Optionally more arguments in the signal.
         * @return <code>true</code> if the signal was sent.<br/>
         *         <code>false</code> if a signal with this name
         *         hasn't been added to this interface, of if
         *         the registered signal has different arguments,
         *         or if no enabled object owns this interface
         *         instance.
         */
        template<typename T, typename... Targs>
        bool emit_signal (const std::string& name, const T& arg, const Targs&... args) {
            std::scoped_lock lock (obj_mutex, signals_mutex);
            if (!obj || dbus_validate_member(name.c_str(), nullptr) != TRUE)
                return false;
            message signal (obj->path(), if_name, name);
            signal.append_args (arg, args...);
            return emit_signal_impl (signal);
        }

        /**
         * Return a pointer to the DBus object owning this interface instance.
         * @return A pointer to a DBus object, or a <code>nullptr</code>
         *         if this interface instance isn't added to a DBus object
         *         instance.
         */
        object* owner () const;

        /**
         * Return XML introspect data describing this interface.
         * @return A string containing XML introspect data.
         */
        const std::string to_xml () const;


    protected:
        /**
         * Protected constructor.
         * Instances of this class is created using method create().
         */
        interface (const std::string& interface_name);


    private:
        friend class object;

        //                               in_sign      out_sign     callback           introspect-XML
        using method_desc_t = std::tuple<std::string, std::string, iface_method_cb_t, std::string>;

        //                                  name         signature
        using signal_arg_desc_t = std::pair<std::string, std::string>;

        struct property_t {
            dbus_variant value;
            std::string signature;
            get_property_cb_t get_cb;
            set_property_cb_t set_cb;
            bool readable;
            bool writable;
            bool disable_emit_on_set_property;
            property_t () : value("v"), readable{false}, writable{false}, disable_emit_on_set_property{false} {}
        };

        std::string if_name; // The name of this interface
        object* obj;         // Object implementing this interface
        std::map<std::string, method_desc_t> methods;
        std::map<std::string, property_t> properties;

        //       signal name            sig sign     arguments
        std::map<std::string, std::pair<std::string, std::vector<signal_arg_desc_t>>> signals;

        mutable std::mutex obj_mutex;
        mutable std::mutex methods_mutex;
        mutable std::mutex properties_mutex;
        mutable std::mutex signals_mutex;

        int get_property_impl (const std::string& name, dbus_type& value) const;
        bool on_method (message& msg);
        void set_owner (object* obj_arg);
        bool add_property_impl (const std::string& name,
                                const std::string& signature,
                                const dbus_type* initial_value,
                                const bool move_initial_value,
                                const bool readable,
                                const bool writable,
                                get_property_cb_t get_callback,
                                set_property_cb_t set_callback,
                                const bool disable_emit_on_set_property);
        bool emit_signal_impl (message signal);
    };


} // namespace ultrabus
#endif
