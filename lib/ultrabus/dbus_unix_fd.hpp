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
#ifndef ULTRABUS_DBUS_UNIX_FD_HPP
#define ULTRABUS_DBUS_UNIX_FD_HPP

#include <ultrabus/dbus_basic.hpp>

namespace ultrabus {


    /**
     * Class representing a DBus Basic Unix File Descriptor value.
     * This wrapper stores a descriptor number only; it neither duplicates nor
     * closes the associated operating-system file descriptor.
     *
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types rel="noopener noreferrer" target="_blank">DBus Basic Types at dbus.freedesktop.org</a>
     */
    class dbus_unix_fd : public dbus_basic<unix_fd_t> {
    public:
        /**
         * Default constructor. Creates an invalid descriptor value (<code>-1</code>).
         */
        dbus_unix_fd () {
            sig = make_signature ();
            basic_value = -1;
        }

        /**
         * Copy constructor.
         * @param other The value to copy.
         */
        dbus_unix_fd (const dbus_unix_fd& other) {
            sig = make_signature ();
            basic_value = other.basic_value;
        }

        /**
         * Constructor.
         * @param file_descriptor The file descriptor to copy.
         */
        dbus_unix_fd (const unix_fd_t& file_descriptor) {
            sig = make_signature ();
            basic_value = file_descriptor;
        }
        /**
         * Construct from a DBus C API basic-value union.
         * @param value Union containing the descriptor number; ownership is not transferred.
         */
        dbus_unix_fd (const DBusBasicValue& value) {
            sig = make_signature ();
            basic_value = value.fd;
        }

        virtual ~dbus_unix_fd () = default;

        /**
         * Assignment operator.
         * @param rhs The right-hand side of the assignment.
         * @return This instance.
         */
        dbus_unix_fd& operator= (const dbus_unix_fd& rhs) {
            if (&rhs != this)
                basic_value = rhs.basic_value;
            return *this;
        }

        /**
         * Assignment operator.
         * @param rhs The file descriptor value to copy.
         * @return This instance.
         */
        dbus_unix_fd& operator= (const unix_fd_t& rhs) {
            basic_value = rhs;
            return *this;
        }

        /**
         * Assign a descriptor number from a DBus C API union.
         * @param dbval Union containing the descriptor number; ownership is not transferred.
         * @return A reference to this object.
         */
        virtual dbus_basic& operator= (const DBusBasicValue& dbval) {
            basic_value = dbval.fd;
            return *this;
        }

        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (dynamic_cast<const dbus_unix_fd&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_unix_fd");
        }
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (std::move(dynamic_cast<dbus_unix_fd&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_unix_fd");
        }

        /**
         * Returns <code>false</code> since this is <em>not</em> a DBus Basic 16-bit integer type.
         * @return <code>false</code>.
         */
        virtual bool is_i16 () const {return false;}

        /**
         * Returns <code>false</code> since this is <em>not</em> a DBus Basic 32-bit integer type.
         * @return <code>false</code>.
         */
        virtual bool is_i32 () const {return false;}

        /**
         * Returns <code>false</code> since this is <em>not</em> a DBus Basic 64-bit integer type.
         * @return <code>false</code>.
         */
        virtual bool is_i64 () const {return false;}

        /**
         * Returns <code>true</code> since this is a DBus Basic Unix File Descriptor value.
         * @return <code>true</code>.
         */
        virtual bool is_unix_fd () const {return true;}

        /**
         * Write the descriptor number to a DBus C API union.
         * @param db_value Destination union. No descriptor ownership is transferred.
         */
        virtual void to_DBusBasicValue (DBusBasicValue& db_value) const {
            db_value.fd = basic_value;
        }

        virtual std::string to_json () const {
            std::string json;
            json = R"({"type":"unix_fd","signature":"h","value":)";
            json.append (std::to_string(basic_value));
            json.push_back ('}');
            return json;
        }

        virtual std::unique_ptr<dbus_type> clone () const {
            return std::unique_ptr<dbus_type> (new dbus_unix_fd(*this));
        }

        virtual std::unique_ptr<dbus_type> clone_move () {
            return std::unique_ptr<dbus_type> (new dbus_unix_fd(std::move(*this)));
        }

        /**
         * Static method to return the signature of a DBus Unix fd type.
         * @return The signature of a DBus DBus Unix fd type.
         */
        static constexpr std::string make_signature () {
            return DBUS_TYPE_UNIX_FD_AS_STRING;
        }

        /**
         * Static method to return the type code of a DBus DBus Unix fd type.
         * @return The type code for a DBus DBus Unix fd type.
         */
        static constexpr int make_type_code () {
            return DBUS_TYPE_UNIX_FD;
        }
    };


}
#endif
