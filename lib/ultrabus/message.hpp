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
#ifndef ULTRABUS_MESSAGE
#define ULTRABUS_MESSAGE


#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <dbus/dbus.h>


namespace ultrabus {


    /**
     * A representation of a DBus message.
     */
    class message {
    public:
        /**
         * Construct an empty method-call message.
         * @throws std::bad_alloc if libdbus cannot allocate the message.
         */
        message ();

        /**
         * Create a method call message.
         * @param destination The bus name of the service that will
         *                    receive the message.
         * @param path The destination object path of the message.
         * @param iface The interface of the method to invoke.
         * @param method The method to invoke.
         * @throws std::invalid_argument if an argument is not a valid D-Bus
         *         destination, object path, interface, or member name.
         * @throws std::bad_alloc if libdbus cannot allocate the message.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names rel="noopener noreferrer" target="_blank">Valid names at dbus.freedesktop.org</a>
         */
        message (const std::string& destination,
                 const std::string& path,
                 const std::string& iface,
                 const std::string& method);

        /**
         * Create a signal message.
         * @param path The path of the object emitting the signal.
         * @param iface The interface the signal is emitted from.
         * @param name The name of the signal.
         * @throws std::invalid_argument if <code>path</code>,
         *         <code>iface</code>, or <code>name</code> is invalid.
         * @throws std::bad_alloc if libdbus cannot allocate the message.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names rel="noopener noreferrer" target="_blank">Valid names at dbus.freedesktop.org</a>
         */
        message (const std::string& path,
                 const std::string& iface,
                 const std::string& name);

        /**
         * Create a method reply message.
         * @param msg The DBus message object being replied to.
         * @param set_error Set to <code>true</code> if this is an error response.
         * @param error_name The error name when <code>set_error</code> is true.
         * @param error_message The error description.
         * @throws std::invalid_argument if <code>msg</code> is null or has no
         *                               serial number.
         * @throws std::bad_alloc if libdbus cannot allocate the reply.
         */
        message (const DBusMessage* msg,
                 const bool set_error,
                 const std::string& error_name="",
                 const std::string& error_message="");

        /**
         * Create a method reply message.
         * @param msg The DBus message object being replied to.
         * @param set_error Set to <code>true</code> if this is an error response.
         * @param error_name The error name when <code>set_error</code> is true.
         * @param error_message The error description.
         * @throws std::invalid_argument if <code>msg</code> is not a method
         *                               call, or has no serial number.
         * @throws std::bad_alloc if libdbus cannot allocate the reply.
         */
        message (const message& msg,
                 const bool set_error,
                 const std::string& error_name="",
                 const std::string& error_message="")
            : message(msg.handle(),
                      set_error,
                      error_name,
                      error_message)
            {
            }

        /**
         * Create a message object using a DBusMessage pointer.
         * @param msg A pointer to a DBus message.
         *            The original DBusMessage is not copied but its
         *            reference counter is increased for this object's
         *            lifetime; ownership of the caller's reference remains
         *            with the caller.<br/>
         *            If <code>msg</code> is a <code>nullptr</code>,
         *            an invalid message will be constructed.
         */
        message (DBusMessage* msg);

        /**
         * Copy constructor.
         * Create an independent copy of another message.
         * @param msg The message to copy.
         * @throws std::bad_alloc if libdbus cannot allocate the copy.
         */
        message (const message& msg);

        /**
         * Move constructor.
         * @param msg The message to move.
         */
        message (message&& msg) {
            ctx = msg.ctx;
            msg.ctx = nullptr;
        }

        /**
         * Destroy the message and release its DBus message reference, if any.
         */
        ~message () {if (ctx) dbus_message_unref(ctx);}

        /**
         * Assignment operator.
         * @param msg The message to copy.
         * @return A reference to this message.
         * @throws std::bad_alloc if libdbus cannot allocate the copy. On
         *         failure, this message is unchanged.
         */
        message& operator= (const message& msg);

        /**
         * Move-assignment operator.
         * @param msg The message to move.
         * @return A reference to this message.
         */
        message& operator= (message&& msg) noexcept;

        /**
         * Return the DBus message type.
         * @return The DBus message type.
         *         One of:
         *          - DBUS_MESSAGE_TYPE_METHOD_CALL
         *          - DBUS_MESSAGE_TYPE_METHOD_RETURN
         *          - DBUS_MESSAGE_TYPE_ERROR
         *          - DBUS_MESSAGE_TYPE_SIGNAL
         *          - DBUS_MESSAGE_TYPE_INVALID
         */
        int type () const {return !ctx ? DBUS_MESSAGE_TYPE_INVALID : dbus_message_get_type(ctx);}

        /**
         * Check whether this message has a valid D-Bus message type.
         * @return <code>true</code> unless the message has no handle or its
         *         type is <code>DBUS_MESSAGE_TYPE_INVALID</code>.
         */
        bool is_valid () const {return type() != DBUS_MESSAGE_TYPE_INVALID;}

        /**
         * Check if this is a method call.
         * @return <code>true</code> if this is a method-call message.
         */
        bool is_method_call () const {return type() == DBUS_MESSAGE_TYPE_METHOD_CALL;}

        /**
         * Check if this is a method return.
         * @return <code>true</code> if this is a method-return message.
         */
        bool is_method_return () const {return type() == DBUS_MESSAGE_TYPE_METHOD_RETURN;}

        /**
         * Check if this is an error message.
         * @return <code>true</code> if this is an error message.
         */
        bool is_error () const {return type() == DBUS_MESSAGE_TYPE_ERROR;}

        /**
         * Check if this is a signal.
         * @return <code>true</code> if this is a signal message.
         */
        bool is_signal () const {return type() == DBUS_MESSAGE_TYPE_SIGNAL;}

        /**
         * Get the destination of the message.
         * @return The destination bus name, or an empty string when none is set.
         */
        std::string destination () const;

        /**
         * Set the destination of the message.
         * @param bus_name The destination of the message.
         */
        void destination (const std::string& bus_name);

        /**
         * Get the object path of the message.
         * @return The object path, or an empty string when none is set.
         */
        std::string path () const;

        /**
         * Set the object path of the message.
         * @param path The object path the message is sent to.
         */
        void path (const std::string& path);

        /**
         * Get the interface the method belongs to.
         * @return The interface name, or an empty string when none is set.
         */
        std::string interface () const;

        /**
         * Set the interface the method belongs to.
         * @param iface The interface the method belongs to.
         */
        void interface (const std::string& iface);

        /**
         * Return the name of the message.
         * @return The member name, or an empty string when none is set.
         */
        std::string name () const;

        /**
         * Set the name of the message.
         * @param msg_name The name of the message.
         */
        void name (const std::string& msg_name);

        /**
         * Get the error name if this message is an error message.
         * This method is irrelevant if the message isn't an error message.
         * @return The error name, or an empty string when none is set.
         */
        std::string error_name () const;

        /**
         * Set the error name if this message is an error message.
         * This method is irrelevant if the message isn't an error message.
         * @param err_name The name of the error.
         */
        void error_name (const std::string& err_name);

        /**
         * Return the error description, if any.
         * The error message is the first argument of the message if that argument is a string.
         * This method is irrelevant if the message isn't an error message.
         * @return The error description from the first string argument, or an
         *         empty string when it is unavailable.
         */
        std::string error_msg () const;

        /**
         * Get the unique name of the connection which originated the message.
         * @return The sender's unique bus name, or an empty string when none is
         *         set.
         */
        std::string sender () const;

        /**
         * Get the serial number of the message.
         * @return A message serial number, or 0 if there isn't a serial number.
         */
        uint32_t serial () const {return ctx ? dbus_message_get_serial(ctx) : 0;}

        /**
         * If this is a message reply, return the serial number of the message
         * to which it replies.
         * @return A message serial number, or 0 if there isn't a serial number.
         */
        uint32_t reply_serial () const {return ctx ? dbus_message_get_reply_serial(ctx) : 0;}

        /**
         * Return <code>true</code> if the message expects a reply.
         * @return <code>true</code> if this message expects a reply.
         */
        bool want_reply () const {
            return ctx && dbus_message_get_no_reply(ctx)==FALSE;
        }

        /**
         * Indicate if this message expects a reply or not.
         * @param a_reply_please If <code>false</code>, indicate that
         *                       this message(typically a method call)
         *                       doesn't expect a reply.
         */
        void want_reply (bool a_reply_please) {
            if (ctx)
                dbus_message_set_no_reply (ctx, a_reply_please?FALSE:TRUE);
        }


        /**
         * Return the DBus signature of the message.
         * @return The DBus signature of the message.
         */
        std::string signature () const;

        /**
         * Append one or more arguments to the message.
         * @param arg The argument to add.
         * @param args Optionally more arguments to add.
         * @note This has no effect is the message is not a valid message.
         */
        template<typename T, typename... Targs>
        void append_args (const T& arg, const Targs&... args) {
            if (!is_valid())
                return;
            DBusMessageIter iter;
            dbus_message_iter_init_append (ctx, &iter);
            append_args_impl (&iter, arg, args...);
        }

        /**
         * Get one or more argument from the message.
         * All arguments that are retrieved are copied from the message.
         * @param arg The first argument we wish to retrieve.
         * @param args Optionally more arguments to retrieve.
         * @return <code>true</code> If all requested
         *         arguments were retrieved.<br/>
         *         <code>false</code> if not all requested
         *         arguments were available, or if any
         *         argument is of the wrong type.
         *         Output arguments before the
         *         failing argument may have been modified.
         * @note If we, for example, wish to retrieve a DBus array argument,
         *       then the parameter can be a reference to a <code>dbus_array</code>
         *       with an arbitrary element signature. Upon return the
         *       element signature will be updated to reflect the actual DBus
         *       array type that was retrieved.<br/>
         *       If we wish that the element signature of the array to retrieve
         *       must match exactly the one we present, use method
         *       <code>get_args_strict()</code>.<br/>
         *       The same is true for the container types dbus_struct and dbus_dict.<br/>
         * Example:
         * @code
         *     ultrabus::dbus_array a ("i"); // We create an empty array of 32-bit integers
         *
         *     // This will print "i"
         *     std::cout << a.element_signature() << std::endl;
         *
         *     // We retrieve an array argument from a DBus message,
         *     // but we don't care if the type of elements doesn't match
         *     // the array we supply.
         *     if (msg.get_args(a) == false) {
         *         // Fails if the first argument in the message isn't a dbus_array
         *         std::cerr << "Failed to get array argument from message" << std::endl;
         *     }else{
         *         // Now 'a' is a copy of the array argument in the DBus message,
         *         // but it may contain another type of elements than 32-bit integers.
         *         // The following line may, or may not, print "i"
         *         std::cout << a.element_signature() << std::endl;
         *     }
         * @endcode
         * @see get_args_strict()
         *
         */
        template<any_dbus_or_basic_cpp_type T, any_dbus_or_basic_cpp_type... Targs>
        bool get_args (T& arg, Targs&... args) {
            DBusMessageIter iter;
            if (!ctx || !dbus_message_iter_init(ctx, &iter))
                return false;
            return get_args_impl(&iter, false, arg, args...);
        }

        /**
         * Get one or more arguments from the message
         * using exactly matching D-Bus signatures.
         * All arguments that are retrieved are copied from the message.
         * @param arg The first argument we wish to retrieve.
         * @param args Optionally more arguments to retrieve.
         * @return <code>true</code> If all requested
         *         arguments were retrieved.<br/>
         *         <code>false</code> if not all requested
         *         arguments were available, or if any
         *         argument is of the wrong type and/or signature.
         *         Output arguments before the
         *         failing argument may have been modified.
         * @note If we wish to retrieve any of the container types
         *       dbus_array, dbus_struct, or dbus_dict, then when
         *       calling this method, the signature of the arguments
         *       we supply must match exactly the ones we wish to retrieve.
         *       If not, this method will return <code>false</code>.<br/>
         *       If we wish to retrieve any of those container types but
         *       don't care if the exact signature of the contained items match
         *       (such as array element type), then use method get_args().<br/>
         * Example:
         * @code
         *     ultrabus::dbus_array a (DBUS_TYPE_STRING); // We create an empty array of DBus strings.
         *
         *     // This will print "s"
         *     std::cout << a.element_signature() << std::endl;
         *
         *     // We retrieve an array argument from a DBus message,
         *     // and the array we retrieve _must_ be an array of strings.
         *     if (msg.get_args_strict(a) == false) {
         *         // Fails if the first argument in the message isn't a dbus_array,
         *         // or if it is an array with elements of another type than dbus_string.
         *         std::cerr << "Failed to get string array argument from message" << std::endl;
         *     }else{
         *         // Now 'a' is a copy of the array argument in the DBus message,
         *         // and we can be sure it is an array of strings.
         *         // This will print "s"
         *         std::cout << a.element_signature() << std::endl;
         *     }
         * @endcode
         * @see get_args()
         */
        template<any_dbus_or_basic_cpp_type T, any_dbus_or_basic_cpp_type... Targs>
        bool get_args_strict (T& arg, Targs&... args) {
            DBusMessageIter iter;
            if (!ctx || !dbus_message_iter_init(ctx, &iter))
                return false;
            return get_args_impl(&iter, true, arg, args...);
        }

        /**
         * Return message arguments in a vector.
         * @param max_args Maximum number of arguments to return.
         *                 If 0, return all available arguments.
         * @return Owning pointers to copies of the message arguments.
         * @note The returned arguments are copies, so modifying
         *       them doesn't alter the message object.
         */
        std::vector<std::unique_ptr<dbus_type>> arguments (size_t max_args=0) const;

        /**
         * Create and return a reply message.
         * @param set_error Set to <code>true</code> if this is an error response.
         * @param error_name The error name when <code>set_error</code> is true.
         * @param error_message The error description.
         * @return A reply message.
         * @throws std::invalid_argument if this message has no serial number.
         * @throws std::bad_alloc if libdbus cannot allocate the reply.
         */
        message make_reply (const bool set_error,
                            const std::string& error_name="",
                            const std::string& error_message="") const;

        /**
         * Return the underlying DBus message handle.
         * Use this only when calling the low-level D-Bus API. The returned
         * pointer is borrowed and remains valid only while this message exists
         * and is not move-assigned or destroyed.
         * @return A borrowed pointer to a <code>DBusMessage</code>, or
         *         <code>nullptr</code> when no handle is held.
         * @see https://dbus.freedesktop.org/doc/api/html/group__DBusMessage.html
         */
        DBusMessage* handle () {return ctx;}

        /**
         * Return the underlying DBus message handle.
         * Use this only when calling the low-level D-Bus API. The returned
         * pointer is borrowed and remains valid only while this message exists
         * and is not move-assigned or destroyed.
         * @return A borrowed const pointer to a <code>DBusMessage</code>, or
         *         <code>nullptr</code> when no handle is held.
         * @see https://dbus.freedesktop.org/doc/api/html/group__DBusMessage.html
         */
        const DBusMessage* handle () const {return ctx;}

        /**
         * Serialize this message and its arguments as JSON.
         * @return A JSON representation of the message.
         */
        std::string to_json () const;


    private:
        DBusMessage* ctx;

        void append_dbus_type_arg (DBusMessageIter* iter, const dbus_type& arg);
        inline void append_args_impl (DBusMessageIter* iter) {}

        template<typename... Targs>
        void append_args_impl (DBusMessageIter* iter, const dbus_type& arg, const Targs&... args) {
            append_dbus_type_arg (iter, arg);
            append_args_impl (iter, args...);
        }
        template<typename... Targs>
        void append_args_impl (DBusMessageIter* iter, const char* arg, const Targs&... args) {
            DBusBasicValue value;
            value.str = const_cast<char*>(arg);
            dbus_message_iter_append_basic (iter, DBUS_TYPE_STRING, &value);
            append_args_impl (iter, args...);
        }
        template<dbus_basic_cpp_types T, typename... Targs>
        void append_args_impl (DBusMessageIter* iter, const T& arg, const Targs&... args) {
            DBusBasicValue value;
            if constexpr (std::same_as<T, bool>)
                value.bool_val = arg;
            else if constexpr (std::same_as<T, uint8_t>)
                value.byt = arg;
            else if constexpr (std::same_as<T, int16_t>)
                value.i16 = arg;
            else if constexpr (std::same_as<T, uint16_t>)
                value.u16 = arg;
            else if constexpr (std::same_as<T, int32_t>)
                value.i32 = arg;
            else if constexpr (std::same_as<T, uint32_t>)
                value.u32 = arg;
            else if constexpr (std::same_as<T, int64_t>)
                value.i64 = arg;
            else if constexpr (std::same_as<T, uint64_t>)
                value.u64 = arg;
            else if constexpr (std::same_as<T, double>)
                value.dbl = arg;
            else if constexpr (std::same_as<T, std::string>)
                value.str = const_cast<char*>(arg.c_str());
            else
                static_assert (false, "Invalid DBus Basic type");
            dbus_message_iter_append_basic (iter, dbus_basic<T>::make_type_code(), &value);
            append_args_impl (iter, args...);
        }

        template<any_dbus_basic_type KeyType>
        void append_dbus_dict_arg (DBusMessageIter* iter, const dbus_dict<KeyType>& dict) {
            std::string dict_entry_sig (DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING);
            dict_entry_sig.push_back (dict.key_type_code());
            dict_entry_sig.append (dict.value_signature());
            dict_entry_sig.push_back (DBUS_DICT_ENTRY_END_CHAR);

            DBusMessageIter dict_iter;
            dbus_message_iter_open_container (iter, DBUS_TYPE_ARRAY, dict_entry_sig.c_str(), &dict_iter);
            for (auto& entry : dict) {
                DBusMessageIter key_value_iter;
                dbus_message_iter_open_container (&dict_iter, DBUS_TYPE_DICT_ENTRY, nullptr, &key_value_iter);
                append_dbus_type_arg (&key_value_iter, entry.first);
                append_dbus_type_arg (&key_value_iter, entry.second);
                dbus_message_iter_close_container (&dict_iter, &key_value_iter);
            }
            dbus_message_iter_close_container (iter, &dict_iter);
        }



        inline bool get_args_impl (DBusMessageIter* iter, bool strict_signature) {return true;}

        template<any_dbus_type T, any_dbus_or_basic_cpp_type... Targs>
        bool get_args_impl (DBusMessageIter* iter, bool strict_signature, T& arg, Targs&... more_args) {
            auto next_arg = get_next_message_argument (iter);
            if (!next_arg)
                return false;
            dbus_message_iter_next (iter);
            if (strict_signature) {
                if (arg.signature() != next_arg->signature())
                    return false;
            }else{
                if (arg.is_array()) {
                    if (! next_arg->is_array())
                        return false;
                }else if (arg.is_dict()) {
                    if (! next_arg->is_dict())
                        return false;
                }else if (arg.signature() != next_arg->signature()) {
                    return false;
                }
            }
            arg = std::move (*next_arg);
            return get_args_impl (iter, strict_signature, more_args...);
        }

        template<dbus_basic_cpp_types T, any_dbus_or_basic_cpp_type... Targs>
        bool get_args_impl (DBusMessageIter* iter, bool strict_signature, T& arg, Targs&... more_args) {
            if (!get_next_message_argument(iter, arg))
                return false;
            dbus_message_iter_next (iter);
            return get_args_impl (iter, strict_signature, more_args...);
        }

        std::unique_ptr<dbus_type> get_next_message_argument (DBusMessageIter* iter) const;

        template<dbus_basic_cpp_types T>
        bool get_next_message_argument (DBusMessageIter* iter, T& arg) const {
            if constexpr (std::same_as<T, std::string>) {
                int type_code = dbus_message_iter_get_arg_type (iter);
                if (type_code != DBUS_TYPE_STRING  &&
                    type_code != DBUS_TYPE_OBJECT_PATH  &&
                    type_code != DBUS_TYPE_SIGNATURE)
                {
                    return false;
                }
            }else{
                if (dbus_message_iter_get_arg_type(iter) != dbus_basic<T>::make_type_code())
                    return false;
            }
            DBusBasicValue value;
            dbus_message_iter_get_basic (iter, &value);
            if constexpr (std::same_as<T, bool>)
                arg = value.bool_val;
            else if constexpr (std::same_as<T, uint8_t>)
                arg = value.byt;
            else if constexpr (std::same_as<T, int16_t>)
                arg = value.i16;
            else if constexpr (std::same_as<T, uint16_t>)
                arg = value.u16;
            else if constexpr (std::same_as<T, int32_t>)
                arg = value.i32;
            else if constexpr (std::same_as<T, uint32_t>)
                arg = value.u32;
            else if constexpr (std::same_as<T, int64_t>)
                arg = value.i64;
            else if constexpr (std::same_as<T, uint64_t>)
                arg = value.u64;
            else if constexpr (std::same_as<T, double>)
                arg = value.dbl;
            else if constexpr (std::same_as<T, std::string>)
                arg = value.str ? value.str : "";
            else
                static_assert (false, "Invalid DBus Basic type");
            return true;
        }
    };


}
#endif
