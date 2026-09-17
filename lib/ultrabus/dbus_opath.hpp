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
#ifndef ULTRABUS_DBUS_OPATH_HPP
#define ULTRABUS_DBUS_OPATH_HPP

#include <ultrabus/dbus_string.hpp>

namespace ultrabus {


    /**
     * Class representing a DBus Basic Object Path value.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types rel="noopener noreferrer" target="_blank">DBus Basic Types at dbus.freedesktop.org</a>
     */
    class dbus_opath : public dbus_basic<std::string> {
    public:
        /**
         * Default constructor.
         * Creates an object root path; value "/".
         */
        constexpr dbus_opath () {
            sig = make_signature ();
            basic_value = "/";
        }
        /**
         * Copy contructor.
         * @param other Value to copy.
         */
        constexpr dbus_opath (const dbus_opath& other) {
            sig = make_signature ();
            basic_value = other.basic_value;
        }
        /**
         * Move contructor.
         * @param other Value to move.
         */
        constexpr dbus_opath (dbus_opath&& other) {
            sig = make_signature ();
            basic_value = std::move (other.basic_value);
            other.basic_value = "/";
        }
        /**
         * Construct an dbus_opath from null-terminated object-path text.
         * @param object_path The object path.
         * @throws std::invalid_argument If the path is null or invalid.
         */
        dbus_opath (const char* object_path) {
            if (!object_path || dbus_validate_path(object_path, nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            sig = make_signature ();
            basic_value = object_path;
        }
        /**
         * Construct an dbus_opath from a string containing an object path value.
         * @param object_path The object path value to copy.
         * @throws std::invalid_argument If the path is invalid.
         */
        dbus_opath (const std::string& object_path) {
            if (dbus_validate_path(object_path.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            sig = make_signature ();
            basic_value = object_path;
        }
        /**
         * Construct an dbus_opath from a string containing an object path value.
         * @param object_path The object path value to move.
         * @throws std::invalid_argument If the path is invalid.
         */
        dbus_opath (std::string&& object_path) {
            if (dbus_validate_path(object_path.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            sig = make_signature ();
            basic_value = std::forward<std::string> (object_path);
        }
        /**
         * Construct from a DBus C API basic-value union.
         * @param dbval Union containing an object-path string to copy.
         * @throws std::invalid_argument If its string is null or an invalid object path.
         */
        dbus_opath (const DBusBasicValue& dbval) {
            if (!dbval.str || dbus_validate_path(dbval.str, nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            sig = make_signature ();
            basic_value = dbval.str;
        }

        virtual ~dbus_opath () = default;

        /**
         * Assignment operator.
         * @param rhs The right-hand side of the assignment.
         * @return This instance.
         */
        constexpr dbus_opath& operator= (const dbus_opath& rhs) {
            if (&rhs != this)
                basic_value = rhs.basic_value;
            return *this;
        }
        /**
         * Move operator.
         * @param rhs The right-hand side of the move assignment.
         * @return This instance.
         */
        constexpr dbus_opath& operator= (dbus_opath&& rhs) {
            if (&rhs != this) {
                basic_value = std::move (rhs.basic_value);
                rhs.basic_value = "/";
            }
            return *this;
        }
        /**
         * Assign null-terminated object-path text.
         * @param rhs Object path to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If the path is
         *         null or an invalid object path.
         */
        dbus_opath& operator= (const char* rhs) {
            if (!rhs || dbus_validate_path(rhs, nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            basic_value = rhs;
            return *this;
        }
        /**
         * Assign an object-path text.
         * @param rhs Object path to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If the path is
         *         null or an invalid object path.
         */
        dbus_opath& operator= (const std::string& rhs) {
            if (dbus_validate_path(rhs.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            basic_value = rhs;
            return *this;
        }
        /**
         * Move-assign an object-path text.
         * @param rhs Object path to move.
         * @return A reference to this object.
         * @throws std::invalid_argument If the path is
         *         null or an invalid object path.
         */
        dbus_opath& operator= (std::string&& rhs) {
            if (dbus_validate_path(rhs.c_str(), nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            basic_value = std::forward<std::string>(rhs);
            return *this;
        }
        /**
         * Assign from a DBus C API basic-value union.
         * @param dbval Union containing an object-path string to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If the path is
         *         null or an invalid object path.
         */
        virtual dbus_basic& operator= (const DBusBasicValue& dbval) {
            if (!dbval.str || dbus_validate_path(dbval.str, nullptr) == false)
                throw std::invalid_argument ("Invalid object path");
            basic_value = dbval.str;
            return *this;
        }

        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (dynamic_cast<const dbus_opath&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_opath");
        }
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (std::move(dynamic_cast<dbus_opath&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_opath");
        }

        /** Returns <code>false</code> since this is not a DBus Basic String. */
        virtual bool is_string () const {return false;}

        /** Returns <code>true</code> since this is a DBus Basic Object Path value. */
        virtual bool is_opath () const {return true;}

        virtual std::unique_ptr<dbus_type> clone () const {
            return std::unique_ptr<dbus_type> (new dbus_opath(*this));
        }

        virtual std::unique_ptr<dbus_type> clone_move () {
            return std::unique_ptr<dbus_type> (new dbus_opath(std::move(*this)));
        }

        /**
         * Static method to return the signature of a DBus Object Path type.
         * @return The signature of an Object Path type.
         */
        static constexpr std::string make_signature () {
            return DBUS_TYPE_OBJECT_PATH_AS_STRING;
        }

        /**
         * Static method to return the type code of a DBus Object Path type.
         * @return The type code for an Object Path type.
         */
        static constexpr int make_type_code () {
            return DBUS_TYPE_OBJECT_PATH;
        }
    };


}
#endif
