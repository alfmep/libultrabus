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
#include <ultrabus/interface.hpp>
#include <ultrabus/dbus_array.hpp>

namespace {

    //--------------------------------------------------------------------------
    // Split a string in format "name:signature" into
    // two strings: "name" and "signature"
    //--------------------------------------------------------------------------
    std::pair<std::string, std::string> split_name_signature (const std::string& name_sign)
    {
        std::string name;
        std::string sign;
        auto colon_pos = name_sign.find_first_of (':');
        if (colon_pos == std::string::npos) {
            sign = name_sign;
        }else{
            name = name_sign.substr (0, colon_pos);
            sign = name_sign.substr (colon_pos + 1);
        }
        return std::make_pair (std::move(name), std::move(sign));
    }

} // Anonymous namespace




namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::shared_ptr<interface> interface::create (const std::string& interface_name)
    {
        std::shared_ptr<interface> iface (new interface(interface_name));
        return iface;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    interface::interface (const std::string& interface_name)
        : if_name (interface_name),
          obj (nullptr)
    {
        if (dbus_validate_interface(if_name.c_str(), nullptr) != TRUE)
            throw std::invalid_argument ("Invalid interface name");
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    interface::~interface ()
    {
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    const std::string interface::to_xml () const
    {
        std::string xml;
        xml.append (R"(<interface name=")");
        xml.append (if_name);
        xml.append (R"(">)");

        // Append methods
        for (const auto& entry : xml_methods)
            xml.append (entry.second);

        // Append signals
        for (auto& entry : signals) {
            auto& name = entry.first;
            auto& arguments = entry.second.second;
            xml.append (R"(<signal name=")");
            xml.append (name);
            xml.append (arguments.empty() ? R"("/>)" : R"(">)");
            for (auto& arg : arguments) {
                auto& name = arg.first;
                auto& signature = arg.second;
                xml.append (R"(<arg)");
                if (!name.empty()) {
                    xml.append (R"( name=")");
                    xml.append (name);
                    xml.push_back ('"');
                }
                xml.append (R"( type=")");
                xml.append (signature);
                xml.append (R"("/>)");
            }
            if ( ! arguments.empty())
                xml.append ("</signal>");
        }

        // Append properties
        for (auto& entry : properties) {
            xml.append (R"(<property name=")");
            xml.append (entry.first);
            xml.append (R"(" type=")");
            xml.append (entry.second.sign);
            xml.append (R"(" access=")");
            if (entry.second.readable)
                xml.append ("read");
            if (entry.second.writable)
                xml.append ("write");
            xml.append (R"("/>)");
        }

        xml.append ("</interface>");
        return xml;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_method (const std::string& name,
                                iface_method_cb_t callback)
    {
        std::vector<std::string> in_params;
        return add_method (name, in_params, "", callback);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_method (const std::string& name,
                                const std::string& in_param,
                                const std::string& out_param,
                                iface_method_cb_t callback)
    {
        std::vector<std::string> in_params;
        if ( ! in_param.empty())
            in_params.emplace_back (in_param);
        return add_method (name, in_params, out_param, callback);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_method (const std::string& name,
                                const std::vector<std::string>& in_params,
                                const std::string& out_param,
                                iface_method_cb_t callback)
    {
        if (!callback) {
            return false;
        }
        if (dbus_validate_member(name.c_str(), nullptr) != TRUE) {
            return false;
        }

        std::string out_name;
        std::string out_signature;
        if ( ! out_param.empty()) {
            auto name_sign = split_name_signature (out_param);
            out_name = std::move (name_sign.first);
            out_signature = std::move (name_sign.second);
            if (out_name.empty()==false && dbus_validate_member(out_name.c_str(), nullptr) != TRUE) {
                return false;
            }
            if (out_signature.empty()==false &&
                dbus_signature_validate_single(out_signature.c_str(), nullptr) != TRUE)
            {
                return false;
            }
        }

        std::string xml (R"(<method name=")");
        xml.append (name);
        if (out_signature.empty() && in_params.empty())
            xml.append (R"("/>)");
        else
            xml.append (R"(">)");

        std::string in_signature;
        for (const auto& param : in_params) {
            auto name_sign = split_name_signature (param);
            auto& param_name = name_sign.first;
            auto& param_signature = name_sign.second;
            if (param_name.empty()==false && dbus_validate_member(param_name.c_str(), nullptr) != TRUE) {
                return false;
            }
            if (param_signature.empty() || dbus_signature_validate_single(param_signature.c_str(), nullptr) != TRUE) {
                return false;
            }
            in_signature.append (param_signature);

            xml.append ("<arg");
            if ( ! param_name.empty()) {
                xml.append (R"( name=")");
                xml.append (param_name);
                xml.push_back ('"');
            }
            xml.append (R"( type=")");
            xml.append (param_signature);
            xml.push_back ('"');
            xml.append (R"( direction="in"/>)");
        }

        if ( ! out_signature.empty()) {
            xml.append ("<arg");
            if ( ! out_name.empty()) {
                xml.append (R"( name=")");
                xml.append (out_name);
                xml.push_back ('"');
            }
            xml.append (R"( type=")");
            xml.append (out_signature);
            xml.push_back ('"');
            xml.append (R"( direction="out"/>)");
        }

        if (!out_signature.empty() || !in_params.empty())
            xml.append ("</method>");

        methods.emplace (name, std::make_tuple(in_signature, out_signature, callback));
        xml_methods.emplace (name, xml);
        return true;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface::remove_method (const std::string& name)
    {
        methods.erase (name);
        xml_methods.erase (name);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::on_method (message& msg)
    {
        auto entry = methods.find (msg.name());
        if (entry == methods.end() || !obj)
            return false;
        const auto& in_signature = std::get<0> (entry->second);
        //const auto& out_signature = std::get<1> (entry->second);
        //const auto& method = std::get<2> (entry->second);

        if (in_signature != msg.signature())
            return false;

        std::get<2>(entry->second) (obj->get_connection(), msg);
        // method (msg);
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_property (const std::string& name,
                                  const std::string& signature,
                                  const bool readable,
                                  const bool writable,
                                  const bool disable_emit_on_set_property)
    {
        return add_property_impl (name, signature, nullptr, false,
                                  readable, writable, nullptr, nullptr,
                                  disable_emit_on_set_property);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_property (const std::string& name,
                                  const dbus_type& initial_value,
                                  const bool readable,
                                  const bool writable,
                                  const bool disable_emit_on_set_property)
    {
        return add_property_impl (name, "", &initial_value, false,
                                  readable, writable, nullptr, nullptr,
                                  disable_emit_on_set_property);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_property (const std::string& name,
                                  dbus_type&& initial_value,
                                  const bool readable,
                                  const bool writable,
                                  const bool disable_emit_on_set_property)
    {
        return add_property_impl (name, "", &initial_value, true,
                                  readable, writable, nullptr, nullptr,
                                  disable_emit_on_set_property);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_property (const std::string& name,
                                  const std::string& signature,
                                  get_property_cb_t get_callback,
                                  set_property_cb_t set_callback)
    {
        return add_property_impl (name, signature, nullptr, false,
                                  get_callback ? true : false,
                                  set_callback ? true : false,
                                  get_callback, set_callback,
                                  false);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_property_impl (const std::string& name,
                                       const std::string& signature,
                                       const dbus_type* initial_value,
                                       const bool move_initial_value,
                                       const bool readable,
                                       const bool writable,
                                       get_property_cb_t get_callback,
                                       set_property_cb_t set_callback,
                                       const bool disable_emit_on_set_property)
    {
        if (name.empty() || dbus_validate_member(name.c_str(), nullptr) != TRUE) {
            return false;
        }
        if (initial_value==nullptr &&
            (signature.empty() || dbus_signature_validate_single(signature.c_str(), nullptr) != TRUE))
        {
            return false;
        }
        if (!readable && !writable)
            return false;
        if (properties.contains(name))
            return false;

        property_t p;
        if (initial_value) {
            p.sign = initial_value->signature ();
            if (move_initial_value)
                p.value = std::move (*initial_value);
            else
                p.value = *initial_value;
        }else{
            p.sign = signature;
        }
        p.readable = readable;
        p.writable = writable;
        p.get_cb = get_callback;
        p.set_cb = set_callback;

        if (!get_callback && !set_callback)
            p.disable_emit_on_set_property = disable_emit_on_set_property;

        properties.emplace (name, p);
        return true;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface::remove_property (const std::string& name, bool emit_signal)
    {
        auto i = properties.find (name);
        if (i != properties.end()) {
            properties.erase (i);
            if (emit_signal)
                emit_property_invalid (name);
        }
    }

    //--------------------------------------------------------------------------
    //  0: Success
    // -1: Property not found
    // -2: Property is write-only
    // -3: Wrong value type
    // -4: Error getting property value
    //--------------------------------------------------------------------------
    int interface::get_property (const std::string& name, dbus_type& value) const
    {
        auto i = properties.find (name);
        if (i == properties.end())
            return -1;

        const auto& p = i->second;
        if (!p.readable)
            return -2; // Property is write-only

        bool ok {true};
        if (value.is_variant()) {
            if (p.get_cb) {
                ok = p.get_cb (name, value.cast<dbus_variant>());
            }else{
                value = p.value;
            }
            return ok ? 0 : -4;
        }

        if (p.sign=="v" && p.value.value_signature() != value.signature())
            return -3;
        else if (p.sign != value.signature())
            return -3;

        if (p.get_cb) {
            dbus_variant v;
            ok = p.get_cb (name, v);
            value = v.get();
        }else{
            value = p.value.get ();
        }
        return ok ? 0 : -4;
    }

    //--------------------------------------------------------------------------
    //  0: Success
    // -1: Property not found
    // -2: Property is read-only
    // -3: Wrong value type
    // -4: Error setting property value
    //--------------------------------------------------------------------------
    int interface::set_property (const std::string& name, const dbus_type& value, bool emit_if_changed)
    {
        auto i = properties.find (name);
        if (i == properties.end())
            return -1;

        auto& p = i->second;
        if (!p.writable)
            return -2; // Property is read-only

        if (p.sign != "v") {
            if (value.is_variant()) {
                if (value.cast<dbus_variant>().value_signature() != p.sign) {
                    return -3; // Setting a value of wrong type
                }
            }else if (value.signature() != p.sign){
                return -3; // Setting a value of wrong type
            }
        }

        bool ok {true};
        if (p.set_cb) {
            ok = p.set_cb (name, value);
        }else if (emit_if_changed && !p.disable_emit_on_set_property) {
            if (p.value != value) {
                p.value = value;
                emit_property_changed (name, value);
            }
        }else{
            p.value = value;
        }
        return ok ? 0 : -4;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    dbus_string_dict interface::get_all_properties () const
    {
        dbus_string_dict props (DBUS_TYPE_VARIANT);
        for (auto& entry : properties) {
            const auto& name = entry.first;
            const auto& p = entry.second;

            if (!p.readable)
                continue;

            if (p.get_cb) {
                dbus_variant value;
                if (p.get_cb(name, value))
                    props.set (name, std::move(value));
            }else{
                props.set (name, p.value);
            }
        }
        return props;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    dbus_variant* interface::property (const std::string& name)
    {
        auto i = properties.find (name);
        if (i == properties.end())
            return nullptr;
        else
            return &i->second.value;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::emit_property_changed (const std::string& property_name)
    {
        if (!obj || !obj->is_enabled())
            return false;

        auto i = properties.find (property_name);
        if (i == properties.end())
            return false;
        return emit_property_changed (property_name, i->second.value);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::emit_property_changed (const std::string& property_name,
                                           const dbus_type& value)
    {
        if (!obj || !obj->is_enabled())
            return false;
        if (property_name.empty()) // Sanity check
            return false;

        dbus_string_dict props (DBUS_TYPE_VARIANT);
        dbus_array invalid_props (DBUS_TYPE_STRING);
        props.set (property_name, value);

        message sig (obj->path(), "org.freedesktop.DBus.Properties", "PropertiesChanged");
        sig.append_args (if_name, std::move(props), std::move(invalid_props));
        return obj->get_connection().send (sig);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::emit_property_invalid (const std::string& property_name)
    {
        std::vector<std::string> changed_properties;
        std::vector<std::string> invalid_names {property_name};
        return emit_properties_changed (changed_properties, invalid_names);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::emit_properties_changed (const std::vector<std::string>& property_names,
                                             const std::vector<std::string>& invalid_names)
    {
        if (!obj || !obj->is_enabled())
            return false;

        dbus_string_dict props (DBUS_TYPE_VARIANT);
        dbus_array invalid_props (DBUS_TYPE_STRING);
        dbus_variant value;

        for (auto& property_name : property_names) {
            dbus_variant value;
            if (get_property(property_name, value))
                props.set (property_name, std::move(value));
        }
        for (auto& invalid_name : invalid_names)
            invalid_props.push_back (invalid_name);

        if (props.empty() && invalid_props.empty())
            return false;

        message sig (obj->path(), "org.freedesktop.DBus.Properties", "PropertiesChanged");
        sig.append_args (if_name, std::move(props), std::move(invalid_props));
        return obj->get_connection().send (sig);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_signal (const std::string& name)
    {
        std::vector<std::string> arguments;
        return add_signal (name, arguments);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_signal (const std::string& name, const std::string& argument)
    {
        std::vector<std::string> arguments {argument};
        return add_signal (name, arguments);
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::add_signal (const std::string& name, const std::vector<std::string>& arguments)
    {
        if (dbus_validate_member(name.c_str(), nullptr) != TRUE) {
            return false;
        }
        std::string signal_signature;
        std::vector<signal_arg_desc_t> signal_args;
        for (auto arg : arguments) {
            auto arg_desc = split_name_signature (arg);
            auto& arg_name = arg_desc.first;
            auto& arg_signature = arg_desc.second;
            if (arg_name.empty()==false && dbus_validate_member(arg_name.c_str(), nullptr) != TRUE) {
                return false;
            }
            if (arg_signature.empty() || dbus_signature_validate_single(arg_signature.c_str(), nullptr) != TRUE) {
                return false;
            }
            signal_signature.append (arg_signature);
            signal_args.emplace_back (arg_desc);
        }
        signals.emplace (name, std::make_pair(signal_signature, signal_args));
        return true;
    }

    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    void interface::remove_signal (const std::string& name)
    {
        signals.erase (name);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool interface::emit_signal_impl (message signal)
    {
        if (!obj || !obj->is_enabled())
            return false;

        auto i = signals.find (signal.name());
        if (i == signals.end())
            return false;

        auto& signal_signature = i->second.first;
        if (signal.signature() != signal_signature)
            return false; // Argument(s) not same as registered signal

        return obj->get_connection().send (signal);
    }


}
