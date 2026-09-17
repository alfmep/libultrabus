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
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <cstring>


namespace ultrabus {


    namespace {
        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        bool msg_validate_dest (const std::string& path,
                                const std::string& iface,
                                const std::string& method,
                                DBusError& err)
        {
            dbus_error_init (&err);
            if (!dbus_validate_path(path.c_str(), &err))
                return false;
            if (!dbus_validate_interface(iface.c_str(), &err))
                return false;
            if (!dbus_validate_member(method.c_str(), &err))
                return false;
            return true;
        }


        //-------------------------------------------------------------------
        //-------------------------------------------------------------------
        bool msg_validate_msg_dest (const std::string& destination,
                                    const std::string& path,
                                    const std::string& iface,
                                    const std::string& method,
                                    DBusError& err)
        {
            dbus_error_init (&err);
            if (!dbus_validate_bus_name(destination.c_str(), &err))
                return false;
            if (!dbus_validate_path(path.c_str(), &err))
                return false;
            if (!iface.empty() && !dbus_validate_interface(iface.c_str(), &err))
                return false;
            if (!dbus_validate_member(method.c_str(), &err))
                return false;
            return true;
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    message::message ()
        : ctx {dbus_message_new(DBUS_MESSAGE_TYPE_METHOD_CALL)}
    {
        if (ctx == nullptr)
            throw std::bad_alloc ();
    }


    //-----------------------------------------------------------------------
    // Create a method call message
    //-----------------------------------------------------------------------
    message::message (const std::string& destination,
                      const std::string& path,
                      const std::string& iface,
                      const std::string& method)
    {
        DBusError err;
        if (!msg_validate_msg_dest(destination, path, iface, method, err)) {
            const std::string msg =
                std::string(err.name) +
                std::string(": ") +
                std::string(err.message);
            dbus_error_free (&err);
            throw std::invalid_argument (msg);
        }
        ctx = dbus_message_new_method_call (destination.c_str(),
                                            path.c_str(),
                                            iface.empty() ? nullptr : iface.c_str(),
                                            method.c_str());
        if (ctx == nullptr)
            throw std::bad_alloc ();
    }


    //-----------------------------------------------------------------------
    // Create a message object from a DBusMessage pointer.
    //-----------------------------------------------------------------------
    message::message (DBusMessage* msg)
        : ctx {msg}
    {
        if (ctx != nullptr)
            dbus_message_ref (ctx); // Increase the reference counter
    }


    //-----------------------------------------------------------------------
    // Create a signal message
    //-----------------------------------------------------------------------
    message::message (const std::string& path,
                      const std::string& iface,
                      const std::string& name)
    {
        DBusError err;
        if (!msg_validate_dest(path, iface, name, err)) {
            const std::string msg =
                std::string(err.name) +
                std::string(": ") +
                std::string(err.message);
            dbus_error_free (&err);
            throw std::invalid_argument (msg);
        }

        ctx = dbus_message_new_signal (path.c_str(), iface.c_str(), name.c_str());
        if (ctx == nullptr)
            throw std::bad_alloc ();
    }


    //-----------------------------------------------------------------------
    // Create a reply message (or an error message)
    //-----------------------------------------------------------------------
    message::message (const DBusMessage* msg,
                      const bool set_error,
                      const std::string& error_name,
                      const std::string& error_message)
    {
        if (!msg)
            throw std::invalid_argument ("Got nullptr argument");

        if (dbus_message_get_serial(const_cast<DBusMessage*>(msg)) == 0)
            throw std::invalid_argument ("Can't create reply from message with serial 0");

        if (set_error) {
            ctx = dbus_message_new_error (const_cast<DBusMessage*>(msg),
                                          error_name.c_str(),
                                          error_message.c_str());
        }else{
            ctx = dbus_message_new_method_return (const_cast<DBusMessage*>(msg));
        }
        if (ctx == nullptr)
            throw std::bad_alloc ();
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    message::message (const message& msg)
    {
        if (msg.ctx) {
            ctx = dbus_message_copy (msg.ctx);
            if (ctx == nullptr)
                throw std::bad_alloc ();
        }else{
            ctx = nullptr;
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    message& message::operator= (const message& msg)
    {
        if (&msg != this) {
            DBusMessage* old_ctx = ctx;
            if (msg.ctx) {
                ctx = dbus_message_copy (msg.ctx);
                if (ctx == nullptr) {
                    ctx = old_ctx;
                    throw std::bad_alloc ();
                }
            }else{
                ctx = nullptr;
            }
            if (old_ctx)
                dbus_message_unref (old_ctx);
        }
        return *this;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    message& message::operator= (message&& msg) noexcept
    {
        if (&msg != this) {
            if (ctx)
                dbus_message_unref (ctx);
            ctx = msg.ctx;
            msg.ctx = nullptr;
        }
        return *this;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::destination () const
    {
        const char* str = nullptr;
        if (ctx)
            str = dbus_message_get_destination (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void message::destination (const std::string& bus_name)
    {
        if (ctx) {
            bool ok = true;
            if (bus_name.empty())
                ok = dbus_message_set_destination (ctx, nullptr);
            else
                ok = dbus_message_set_destination (ctx, bus_name.c_str());
            if (!ok)
                throw std::bad_alloc ();
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::path () const
    {
        const char* str = nullptr;
        if (ctx)
            str = dbus_message_get_path (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void message::path (const std::string& opath)
    {
        if (ctx) {
            bool ok = true;
            if (opath.empty())
                ok = dbus_message_set_path (ctx, nullptr);
            else
                ok = dbus_message_set_path (ctx, opath.c_str());
            if (!ok)
                throw std::bad_alloc ();
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::interface () const
    {
        const char* str = nullptr;
        if (ctx)
            str = dbus_message_get_interface (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void message::interface (const std::string& iface)
    {
        if (ctx) {
            bool ok = true;
            if (iface.empty())
                ok = dbus_message_set_interface (ctx, nullptr);
            else
                ok = dbus_message_set_interface (ctx, iface.c_str());
            if (!ok)
                throw std::bad_alloc ();
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::name () const
    {
        const char* str = nullptr;
        if (ctx)
            str = dbus_message_get_member (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void message::name (const std::string& msg_name)
    {
        if (ctx) {
            bool ok = true;
            if (msg_name.empty())
                ok = dbus_message_set_member (ctx, nullptr);
            else
                ok = dbus_message_set_member (ctx, msg_name.c_str());
            if (!ok)
                throw std::bad_alloc ();
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::error_name () const
    {
        const char* str = nullptr;
        if (ctx && is_error())
            str = dbus_message_get_error_name (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void message::error_name (const std::string& err_name)
    {
        if (ctx && is_error()) {
            bool ok = true;
            if (err_name.empty())
                ok = dbus_message_set_error_name (ctx, nullptr);
            else
                ok = dbus_message_set_error_name (ctx, err_name.c_str());
            if (!ok)
                throw std::bad_alloc ();
        }
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::error_msg () const
    {
        std::string error_message;
        if (ctx && is_error()) {
            // The first argument is the error message.
            DBusMessageIter iter;
            if (dbus_message_iter_init(ctx, &iter) &&
                dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_STRING)
            {
                DBusBasicValue basic_value {};
                dbus_message_iter_get_basic (&iter, &basic_value);
                if (basic_value.str)
                    error_message = basic_value.str;
            }
        }
        return error_message;
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::sender () const
    {
        const char* str = nullptr;
        if (ctx)
            str = dbus_message_get_sender (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    std::string message::signature () const
    {
        const char* str = nullptr;
        if (ctx)
            str = dbus_message_get_signature (ctx);
        return {str?str:""};
    }


    //-----------------------------------------------------------------------
    //-----------------------------------------------------------------------
    void message::append_dbus_type_arg (DBusMessageIter* iter, const dbus_type& arg)
    {
        if (arg.is_basic()) {
            DBusBasicValue value {};
            arg.cast<dbus_basic_base>().to_DBusBasicValue (value);
            dbus_message_iter_append_basic (iter, arg.type_code(), &value);
        }else if (arg.is_struct()) {
            const auto& s = arg.cast<dbus_struct> ();
            if (s.size()) {
                DBusMessageIter sub_iter;
                dbus_message_iter_open_container (iter, DBUS_TYPE_STRUCT, nullptr, &sub_iter);
                for (size_t i=0; i<s.size(); ++i)
                    append_dbus_type_arg (&sub_iter, s.get(i));
                dbus_message_iter_close_container (iter, &sub_iter);
            }
        }else if (arg.is_array()) {
            const auto& a = arg.cast<dbus_array> ();
            DBusMessageIter sub_iter;
            dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, a.element_signature().c_str(), &sub_iter);
            for (const auto& element : a)
                append_dbus_type_arg (&sub_iter, element);
            dbus_message_iter_close_container (iter, &sub_iter);
        }else if (arg.is_dict()) {
            switch (arg.cast<dbus_dict_base>().key_type_code()) {
            case DBUS_TYPE_BOOLEAN: append_dbus_dict_arg(iter, arg.cast<dbus_bool_dict>()); break;
            case DBUS_TYPE_BYTE: append_dbus_dict_arg(iter, arg.cast<dbus_byte_dict>()); break;
            case DBUS_TYPE_INT16: append_dbus_dict_arg(iter, arg.cast<dbus_i16_dict>()); break;
            case DBUS_TYPE_UINT16: append_dbus_dict_arg(iter, arg.cast<dbus_u16_dict>()); break;
            case DBUS_TYPE_INT32: append_dbus_dict_arg(iter, arg.cast<dbus_i32_dict>()); break;
            case DBUS_TYPE_UINT32: append_dbus_dict_arg(iter, arg.cast<dbus_u32_dict>()); break;
            case DBUS_TYPE_INT64: append_dbus_dict_arg(iter, arg.cast<dbus_i64_dict>()); break;
            case DBUS_TYPE_UINT64: append_dbus_dict_arg(iter, arg.cast<dbus_u64_dict>()); break;
            case DBUS_TYPE_DOUBLE: append_dbus_dict_arg(iter, arg.cast<dbus_double_dict>()); break;
            case DBUS_TYPE_STRING: append_dbus_dict_arg(iter, arg.cast<dbus_string_dict>()); break;
            case DBUS_TYPE_OBJECT_PATH: append_dbus_dict_arg(iter, arg.cast<dbus_opath_dict>()); break;
            case DBUS_TYPE_SIGNATURE: append_dbus_dict_arg(iter, arg.cast<dbus_signature_dict>()); break;
            case DBUS_TYPE_UNIX_FD: append_dbus_dict_arg(iter, arg.cast<dbus_unix_fd_dict>()); break;
            default: break;
            }
        }else if (arg.is_variant()) {
            const auto& v = arg.cast<dbus_variant> ();
            DBusMessageIter sub_iter;
            dbus_message_iter_open_container (iter, DBUS_TYPE_VARIANT, v.get().signature().c_str(), &sub_iter);
            append_dbus_type_arg (&sub_iter, v.get());
            dbus_message_iter_close_container (iter, &sub_iter);
        }
    }



    namespace {
        std::unique_ptr<dbus_type> create_value_from_iter (DBusMessageIter* iter);


        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::unique_ptr<dbus_type> create_struct_from_iter (DBusMessageIter* iter)
        {
            std::unique_ptr<dbus_type> struct_instance (new dbus_struct);

            DBusMessageIter sub_iter;
            dbus_message_iter_recurse (iter, &sub_iter);

            auto& s = struct_instance->cast<dbus_struct> ();
            std::unique_ptr<dbus_type> attrib;
            while ((attrib=create_value_from_iter(&sub_iter)) != nullptr) {
                s.add (std::move(attrib));
                dbus_message_iter_next (&sub_iter);
            }

            return struct_instance;
        }


        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        template<any_dbus_basic_type KeyType>
        std::unique_ptr<dbus_type> create_dict_from_iter_impl (DBusMessageIter* iter,
                                                               const std::string& value_signature)
        {
            std::unique_ptr<dbus_type> dict_instance (new dbus_dict<KeyType>(value_signature));
            auto& dict = dict_instance->cast<dbus_dict<KeyType>> ();

            DBusMessageIter dict_iter;
            dbus_message_iter_recurse (iter, &dict_iter);
            while (dbus_message_iter_get_arg_type(&dict_iter) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter entry_iter;
                std::unique_ptr<dbus_type> key;
                std::unique_ptr<dbus_type> value;

                dbus_message_iter_recurse (&dict_iter, &entry_iter);
                if (dbus_message_iter_get_arg_type(&entry_iter) != KeyType::make_type_code()) {
                    continue;
                }
                key = create_value_from_iter (&entry_iter);
                if (!key) {
                    continue;
                }

                dbus_message_iter_next (&entry_iter);
                if (dbus_message_iter_get_arg_type(&entry_iter) == DBUS_TYPE_INVALID) {
                    continue;
                }
                value = create_value_from_iter (&entry_iter);
                if (!value) {
                    continue;
                }

                dict.set (key->cast<KeyType>(), std::move(value));

                dbus_message_iter_next (&dict_iter);
            }
            return dict_instance;
        }


        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::unique_ptr<dbus_type> create_dict_from_iter (DBusMessageIter* iter)
        {
            auto* dict_signature = dbus_message_iter_get_signature (iter);
            if (dict_signature == nullptr)
                throw std::bad_alloc ();
            const int key_type_code = (uint8_t) dict_signature[2];
            const std::string value_signature (dict_signature+3, strlen(dict_signature)-4);
            dbus_free (dict_signature);

            switch (key_type_code) {
            case DBUS_TYPE_BOOLEAN: return create_dict_from_iter_impl<dbus_bool>(iter, value_signature);
            case DBUS_TYPE_BYTE: return create_dict_from_iter_impl<dbus_byte>(iter, value_signature);
            case DBUS_TYPE_INT16: return create_dict_from_iter_impl<dbus_i16>(iter, value_signature);
            case DBUS_TYPE_UINT16: return create_dict_from_iter_impl<dbus_u16>(iter, value_signature);
            case DBUS_TYPE_INT32: return create_dict_from_iter_impl<dbus_i32>(iter, value_signature);
            case DBUS_TYPE_UINT32: return create_dict_from_iter_impl<dbus_u32>(iter, value_signature);
            case DBUS_TYPE_INT64: return create_dict_from_iter_impl<dbus_i64>(iter, value_signature);
            case DBUS_TYPE_UINT64: return create_dict_from_iter_impl<dbus_u64>(iter, value_signature);
            case DBUS_TYPE_DOUBLE: return create_dict_from_iter_impl<dbus_double>(iter, value_signature);
            case DBUS_TYPE_STRING: return create_dict_from_iter_impl<dbus_string>(iter, value_signature);
            case DBUS_TYPE_OBJECT_PATH: return create_dict_from_iter_impl<dbus_opath>(iter, value_signature);
            case DBUS_TYPE_SIGNATURE: return create_dict_from_iter_impl<dbus_signature>(iter, value_signature);
            case DBUS_TYPE_UNIX_FD: return create_dict_from_iter_impl<dbus_unix_fd>(iter, value_signature);
            default:
                return nullptr;
            }
        }


        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::unique_ptr<dbus_type> create_array_from_iter (DBusMessageIter* iter)
        {
            if (dbus_message_iter_get_element_type(iter) == DBUS_TYPE_DICT_ENTRY) {
                return create_dict_from_iter (iter);
            }

            char* array_signature = dbus_message_iter_get_signature (iter);
            if (!array_signature)
                throw std::bad_alloc ();
            std::unique_ptr<dbus_type> array_instance (new dbus_array(array_signature+1));
            dbus_free (array_signature);

            DBusMessageIter sub_iter;
            dbus_message_iter_recurse (iter, &sub_iter);

            auto& array = array_instance->cast<dbus_array> ();
            std::unique_ptr<dbus_type> element;
            while ((element=create_value_from_iter(&sub_iter)) != nullptr) {
                array.push_back (std::move(element));
                dbus_message_iter_next (&sub_iter);
            }

            return array_instance;
        }


        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::unique_ptr<dbus_type> create_variant_from_iter (DBusMessageIter* iter)
        {
            DBusMessageIter sub_iter;
            dbus_message_iter_recurse (iter, &sub_iter);

            if (dbus_message_iter_get_arg_type(&sub_iter) == DBUS_TYPE_INVALID) {
                return nullptr;
            }

            auto value = create_value_from_iter(&sub_iter);
            if (!value) {
                return nullptr;
            }

            return std::unique_ptr<dbus_type> (new dbus_variant(std::move(value)));
        }

        //----------------------------------------------------------------------
        //----------------------------------------------------------------------
        std::unique_ptr<dbus_type> create_value_from_iter (DBusMessageIter* iter)
        {
            DBusBasicValue basic_value;
            const int type_code = dbus_message_iter_get_arg_type (iter);

            switch (type_code) {
            case DBUS_TYPE_BYTE:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_byte(basic_value));
            case DBUS_TYPE_BOOLEAN:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_bool(basic_value));
            case DBUS_TYPE_INT16:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_i16(basic_value));
            case DBUS_TYPE_UINT16:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_u16(basic_value));
            case DBUS_TYPE_INT32:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_i32(basic_value));
            case DBUS_TYPE_UINT32:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_u32(basic_value));
            case DBUS_TYPE_INT64:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_i64(basic_value));
            case DBUS_TYPE_UINT64:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_u64(basic_value));
            case DBUS_TYPE_DOUBLE:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_double(basic_value));
            case DBUS_TYPE_STRING:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_string(basic_value));
            case DBUS_TYPE_OBJECT_PATH:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_opath(basic_value));
            case DBUS_TYPE_SIGNATURE:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_signature(basic_value));
            case DBUS_TYPE_UNIX_FD:
                dbus_message_iter_get_basic (iter, &basic_value);
                return std::unique_ptr<dbus_type> (new dbus_unix_fd(basic_value));
            case DBUS_TYPE_STRUCT:
                return create_struct_from_iter (iter);
            case DBUS_TYPE_ARRAY:
                return create_array_from_iter (iter);
            case DBUS_TYPE_DICT_ENTRY:
                return nullptr;
            case DBUS_TYPE_VARIANT:
                return create_variant_from_iter (iter);
            default:
                return nullptr;
            }
        }


    }



    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::vector<std::unique_ptr<dbus_type>> message::arguments (size_t max_args) const
    {
        std::vector<std::unique_ptr<dbus_type>> args;
        DBusMessageIter iter;
        if (ctx && dbus_message_iter_init(ctx, &iter)) {
            std::unique_ptr<dbus_type> arg;
            while ((arg = create_value_from_iter(&iter)) != nullptr) {
                args.emplace_back (std::move(arg));
                if (max_args && --max_args==0)
                    break;
                dbus_message_iter_next (&iter);
            }
        }
        return args;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::unique_ptr<dbus_type> message::get_next_message_argument (DBusMessageIter* iter) const
    {
        return create_value_from_iter (iter);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    message message::make_reply (const bool set_error,
                                 const std::string& error_name,
                                 const std::string& error_message) const
    {
        //return message (*this, set_error, error_name, error_message);
        return {*this, set_error, error_name, error_message};
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string message::to_json () const
    {
        std::string json (R"({"type":)");
        switch (type()) {
        case DBUS_MESSAGE_TYPE_METHOD_CALL:
            json.append (R"("method_call")");
            break;
        case DBUS_MESSAGE_TYPE_METHOD_RETURN:
            json.append (R"("method_return")");
            break;
        case DBUS_MESSAGE_TYPE_SIGNAL:
            json.append (R"("signal")");
            break;
        case DBUS_MESSAGE_TYPE_ERROR:
            json.append (R"("error")");
            break;
        case DBUS_MESSAGE_TYPE_INVALID:
        default:
            json.append (R"("invalid"})");
            return json;
        }
        json.append (R"(,"destination":")");
        json.append (destination());
        json.append (R"(","object_path":")");
        json.append (path());
        json.append (R"(","interface":")");
        json.append (interface());
        json.append (R"(","name":")");
        json.append (name());
        json.append (R"(","sender":")");
        json.append (sender());
        json.append (R"(","serial":)");
        json.append (std::to_string(serial()));
        if (is_method_return() || is_error()) {
            json.append (R"(,"reply_serial":)");
            json.append (std::to_string(reply_serial()));
        }
        json.append (R"(,"signature":")");
        json.append (signature());
        json.append (R"(","arguments":[)");
        auto args = arguments ();
        for (auto i=args.begin(); i != args.end(); ++i) {
            if (i != args.begin())
                json.push_back (',');
            json.append ((*i)->to_json());
        }
        json.append ("]}");
        return json;
    }



}
