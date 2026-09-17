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
#ifndef ULTRABUS_DBUS_SIGNATURE_HPP
#define ULTRABUS_DBUS_SIGNATURE_HPP

#include <ultrabus/dbus_string.hpp>

namespace ultrabus {


    /**
     * Class representing a DBus Basic Signature value.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types rel="noopener noreferrer" target="_blank">DBus Basic Types at dbus.freedesktop.org</a>
     */
    class dbus_signature : public dbus_basic<std::string> {
    public:
        /**
         * Default constructor, creates an empty DBus signature value.
         * @note The default constructor creates an invalid DBus signature.
         */
        constexpr dbus_signature () {
            sig = make_signature ();
        }
        /**
         * Copy contructor.
         * @param other Value to copy.
         */
        constexpr dbus_signature (const dbus_signature& other) {
            sig = make_signature ();
            basic_value = other.basic_value;
        }
        /**
         * Move contructor.
         * @param other Value to move.
         */
        constexpr dbus_signature (dbus_signature&& other) {
            sig = make_signature ();
            basic_value = std::move (other.basic_value);
        }
        /**
         * Construct from null-terminated signature text.
         * @param signature The signature value to copy.
         * @throws std::invalid_argument If the signature is null or invalid.
         */
        dbus_signature (const char* signature) {
            if (!signature || dbus_signature_validate(signature, nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            sig = make_signature ();
            basic_value = signature;
        }
        /**
         * Construct from signature text.
         * @param signature Signature to copy.
         * @throws std::invalid_argument If the signature is invalid.
         */
        dbus_signature (const std::string& signature) {
            if (dbus_signature_validate(signature.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            sig = make_signature ();
            basic_value = signature;
        }
        /**
         * Construct by moving signature text.
         * @param signature Signature to validate and move from.
         * @throws std::invalid_argument If the signature is invalid.
         */
        dbus_signature (std::string&& signature) {
            if (dbus_signature_validate(signature.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            sig = make_signature ();
            basic_value = std::forward<std::string> (signature);
        }
        /**
         * Construct from a DBus C API basic-value union.
         * @param dbval Union containing a signature string to copy.
         * @throws std::invalid_argument If its string is null or invalid.
         */
        dbus_signature (const DBusBasicValue& dbval) {
            if (!dbval.str || dbus_signature_validate(dbval.str, nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            sig = make_signature ();
            basic_value = dbval.str;
        }

        virtual ~dbus_signature () = default;

        /**
         * Assignment operator.
         * @param rhs The right-hand side of the assignment.
         * @return This instance.
         */
        constexpr dbus_signature& operator= (const dbus_signature& rhs) {
            if (&rhs != this)
                basic_value = rhs.basic_value;
            return *this;
        }
        /**
         * Move operator.
         * @param rhs The right-hand side of the move assignment.
         * @return This instance.
         */
        constexpr dbus_signature& operator= (dbus_signature&& rhs) {
            if (&rhs != this)
                basic_value = std::move (rhs.basic_value);
            return *this;
        }
        /**
         * Assign null-terminated signature text.
         * @param rhs Signature to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If the signature is null or invalid.
         */
        dbus_signature& operator= (const char* rhs) {
            if (!rhs || dbus_signature_validate(rhs, nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            basic_value = rhs;
            return *this;
        }
        /**
         * Assign signature text.
         * @param rhs Signature to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If the signature is invalid.
         */
        dbus_signature& operator= (const std::string& rhs) {
            if (dbus_signature_validate(rhs.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            basic_value = rhs;
            return *this;
        }
        /**
         * Move-assign signature text.
         * @param rhs Signature to validate and move from.
         * @return A reference to this object.
         * @throws std::invalid_argument If the signature is invalid.
         */
        dbus_signature& operator= (std::string&& rhs) {
            if (dbus_signature_validate(rhs.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            basic_value = std::forward<std::string>(rhs);
            return *this;
        }
        /**
         * Assign from a DBus C API basic-value union.
         * @param dbval Union containing a signature string to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If the signature is null or invalid.
         */
        virtual dbus_basic& operator= (const DBusBasicValue& dbval) {
            if (!dbval.str || dbus_signature_validate(dbval.str, nullptr) == false)
                throw std::invalid_argument ("Invalid DBus signature");
            basic_value = dbval.str;
            return *this;
        }

        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (dynamic_cast<const dbus_signature&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_signature");
        }

        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (std::move(dynamic_cast<dbus_signature&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_signature");
        }

        /** Returns <code>false</code> since this is not a DBus Basic String. */
        virtual bool is_string () const {return false;}

        /** Returns <code>true</code> since this is a DBus Basic Signature value. */
        virtual bool is_signature () const {return true;}

        virtual std::unique_ptr<dbus_type> clone () const {
            return std::unique_ptr<dbus_type> (new dbus_signature(*this));
        }
        virtual std::unique_ptr<dbus_type> clone_move () {
            return std::unique_ptr<dbus_type> (new dbus_signature(std::move(*this)));
        }

        /**
         * Static method to return the signature of a DBus Signature type.
         * @return The signature of a DBus Signature type.
         */
        static constexpr std::string make_signature () {
            return DBUS_TYPE_SIGNATURE_AS_STRING;
        }

        /**
         * Static method to return the type code of a DBus Signature type.
         * @return The type code for a DBus Signature type.
         */
        static constexpr int make_type_code () {
            return DBUS_TYPE_SIGNATURE;
        }
    };


}
#endif
