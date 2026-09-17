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
 * @return <code>true</code> for a DBus basic value.
 */
#ifndef ULTRABUS_DBUS_TYPE_HPP
#define ULTRABUS_DBUS_TYPE_HPP

#include <string>
#include <memory>
#include <compare>
#include <concepts>
#include <cstdint>


namespace ultrabus {


    /**
     * Valid C++ types for <code>T</code> in template <code>dbus_basic&lt;T&gt;</code>.
     * @tparam T A C++ representation of a DBus basic value.
     */
    template<typename T>
    concept dbus_basic_cpp_types = (
            std::same_as<T, bool> ||
            std::same_as<T, uint8_t> ||
            std::same_as<T, int16_t> ||
            std::same_as<T, uint16_t> ||
            std::same_as<T, int32_t> ||
            std::same_as<T, uint32_t> ||
            std::same_as<T, int64_t> ||
            std::same_as<T, uint64_t> ||
            std::same_as<T, double> ||
            std::same_as<T, std::string>);


    /**
     * Integer type used to store a DBus Unix file descriptor.
     * This alias has the same bit size as <code>int</code>, as required by the
     * DBus C API.
     */
    using unix_fd_t =
        std::conditional<sizeof(int) == sizeof(int64_t), int64_t,
                         std::conditional<sizeof(int) == sizeof(int32_t), int32_t,
                                          int16_t>::type>::type;

    // forward declarations and aliases
    class dbus_type;

    class dbus_basic_base;
    template<dbus_basic_cpp_types T> class dbus_basic;
    using dbus_bool = dbus_basic<bool>;     /**< Class representing a DBus Basic Boolean. */
    using dbus_byte = dbus_basic<uint8_t>;  /**< Class representing a DBus Basic Byte. */
    using dbus_i16 = dbus_basic<int16_t>;   /**< Class representing a DBus Basic 16 bit integer. */
    using dbus_u16 = dbus_basic<uint16_t>;  /**< Class representing a DBus Basic 16 bit unsigned integer. */
    using dbus_i32 = dbus_basic<int32_t>;   /**< Class representing a DBus Basic 32 bit integer. */
    using dbus_u32 = dbus_basic<uint32_t>;  /**< Class representing a DBus Basic 32 bit unsigned integer. */
    using dbus_i64 = dbus_basic<int64_t>;   /**< Class representing a DBus Basic 64 bit integer. */
    using dbus_u64 = dbus_basic<uint64_t>;  /**< Class representing a DBus Basic 64 bit unsigned integer. */
    using dbus_double = dbus_basic<double>; /**< Class representing a DBus Basic Double. */
    using dbus_string = dbus_basic<std::string>; /**< Class representing a DBus Basic String. */


    class dbus_opath;
    class dbus_signature;
    class dbus_unix_fd;
    class dbus_array;
    class dbus_struct;
    class dbus_variant;

    /**
     * Any DBus Basic type.
     * @tparam T Type to verify as a DBus Basic type.
     */
    template<typename T>
    concept any_dbus_basic_type = (
            std::same_as<T, dbus_bool>      ||
            std::same_as<T, dbus_byte>      ||
            std::same_as<T, dbus_i16>       ||
            std::same_as<T, dbus_u16>       ||
            std::same_as<T, dbus_i32>       ||
            std::same_as<T, dbus_u32>       ||
            std::same_as<T, dbus_i64>       ||
            std::same_as<T, dbus_u64>       ||
            std::same_as<T, dbus_double>    ||
            std::same_as<T, dbus_string>    ||
            std::same_as<T, dbus_opath>     ||
            std::same_as<T, dbus_signature> ||
            std::same_as<T, dbus_unix_fd>);

    class dbus_dict_base;
    template<any_dbus_basic_type T> class dbus_dict;
    using dbus_bool_dict = dbus_dict<dbus_bool>;           /**< DBus dictionary with key type dbus_bool. */
    using dbus_byte_dict = dbus_dict<dbus_byte>;           /**< DBus dictionary with key type dbus_byte. */
    using dbus_i16_dict = dbus_dict<dbus_i16>;             /**< DBus dictionary with key type dbus_i16. */
    using dbus_u16_dict = dbus_dict<dbus_u16>;             /**< DBus dictionary with key type dbus_u16. */
    using dbus_i32_dict = dbus_dict<dbus_i32>;             /**< DBus dictionary with key type dbus_i32. */
    using dbus_u32_dict = dbus_dict<dbus_u32>;             /**< DBus dictionary with key type dbus_u32. */
    using dbus_i64_dict = dbus_dict<dbus_i64>;             /**< DBus dictionary with key type dbus_i64. */
    using dbus_u64_dict = dbus_dict<dbus_u64>;             /**< DBus dictionary with key type dbus_u64. */
    using dbus_double_dict = dbus_dict<dbus_double>;       /**< DBus dictionary with key type dbus_double. */
    using dbus_string_dict = dbus_dict<dbus_string>;       /**< DBus dictionary with key type dbus_string. */
    using dbus_opath_dict = dbus_dict<dbus_opath>;         /**< DBus dictionary with key type dbus_opath. */
    using dbus_signature_dict = dbus_dict<dbus_signature>; /**< DBus dictionary with key type dbus_signature. */
    using dbus_unix_fd_dict = dbus_dict<dbus_unix_fd>;     /**< DBus dictionary with key type dbus_unix_fd. */

    /**
     * All available DBus data types in libultrabus.
     * @tparam T Type to verify as a DBus data type.
     */
    template<typename T>
    concept any_dbus_type = (
            std::same_as<T, dbus_type> ||
            std::same_as<T, dbus_basic_base> ||
            std::same_as<T, dbus_bool> ||
            std::same_as<T, dbus_byte> ||
            std::same_as<T, dbus_i16> ||
            std::same_as<T, dbus_u16> ||
            std::same_as<T, dbus_i32> ||
            std::same_as<T, dbus_u32> ||
            std::same_as<T, dbus_i64> ||
            std::same_as<T, dbus_u64> ||
            std::same_as<T, dbus_double> ||
            std::same_as<T, dbus_string> ||
            std::same_as<T, dbus_opath> ||
            std::same_as<T, dbus_signature> ||
            std::same_as<T, dbus_unix_fd> ||
            std::same_as<T, dbus_array> ||
            std::same_as<T, dbus_struct> ||
            std::same_as<T, dbus_dict_base> ||
            std::same_as<T, dbus_bool_dict> ||
            std::same_as<T, dbus_byte_dict> ||
            std::same_as<T, dbus_i16_dict> ||
            std::same_as<T, dbus_u16_dict> ||
            std::same_as<T, dbus_i32_dict> ||
            std::same_as<T, dbus_u32_dict> ||
            std::same_as<T, dbus_i64_dict> ||
            std::same_as<T, dbus_u64_dict> ||
            std::same_as<T, dbus_double_dict> ||
            std::same_as<T, dbus_string_dict> ||
            std::same_as<T, dbus_opath_dict> ||
            std::same_as<T, dbus_signature_dict> ||
            std::same_as<T, dbus_unix_fd_dict> ||
            std::same_as<T, dbus_variant>);


    /**
     * All available DBus data types and DBus Basic C++ types in libultrabus.
     * @tparam T Type to verify as a DBus data type, or a valid C++ type representing a DBus Basic.
     */
    template<typename T>
    concept any_dbus_or_basic_cpp_type = (
            std::same_as<T, bool> ||
            std::same_as<T, uint8_t> ||
            std::same_as<T, int16_t> ||
            std::same_as<T, uint16_t> ||
            std::same_as<T, int32_t> ||
            std::same_as<T, uint32_t> ||
            std::same_as<T, int64_t> ||
            std::same_as<T, uint64_t> ||
            std::same_as<T, double> ||
            std::same_as<T, std::string> ||
            std::same_as<T, dbus_type> ||
            std::same_as<T, dbus_basic_base> ||
            std::same_as<T, dbus_bool> ||
            std::same_as<T, dbus_byte> ||
            std::same_as<T, dbus_i16> ||
            std::same_as<T, dbus_u16> ||
            std::same_as<T, dbus_i32> ||
            std::same_as<T, dbus_u32> ||
            std::same_as<T, dbus_i64> ||
            std::same_as<T, dbus_u64> ||
            std::same_as<T, dbus_double> ||
            std::same_as<T, dbus_string> ||
            std::same_as<T, dbus_opath> ||
            std::same_as<T, dbus_signature> ||
            std::same_as<T, dbus_unix_fd> ||
            std::same_as<T, dbus_array> ||
            std::same_as<T, dbus_struct> ||
            std::same_as<T, dbus_dict_base> ||
            std::same_as<T, dbus_bool_dict> ||
            std::same_as<T, dbus_byte_dict> ||
            std::same_as<T, dbus_i16_dict> ||
            std::same_as<T, dbus_u16_dict> ||
            std::same_as<T, dbus_i32_dict> ||
            std::same_as<T, dbus_u32_dict> ||
            std::same_as<T, dbus_i64_dict> ||
            std::same_as<T, dbus_u64_dict> ||
            std::same_as<T, dbus_double_dict> ||
            std::same_as<T, dbus_string_dict> ||
            std::same_as<T, dbus_opath_dict> ||
            std::same_as<T, dbus_signature_dict> ||
            std::same_as<T, dbus_unix_fd_dict> ||
            std::same_as<T, dbus_variant>);


    /**
     * Base class for all DBus data types in libultrabus.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#type-system rel="noopener noreferrer" target="_blank">DBus Type System at dbus.freedesktop.org</a>
     */
    class dbus_type {
    public:
        virtual ~dbus_type () = default;

        /**
         * Perform a dynamic cast to a specific sub type of <code>dbus_type</code>.
         * @tparam DBusType The type we wish to perform a dynamic cast to.
         * @return A reference to this object as <code>DBusType</code>.
         * @throws std::bad_cast If this object is not a <code>DBusType</code>.
         */
        template<any_dbus_type DBusType>
        constexpr DBusType& cast () {return dynamic_cast<DBusType&>(*this);}

        /**
         * Perform a dynamic cast to a specific const type of instance.
         * @tparam DBusType The type we wish to perform a dynamic cast to.
         * @return A const reference to this object as <code>DBusType</code>.
         * @throws std::bad_cast If this object is not a <code>DBusType</code>.
         */
        template<any_dbus_type DBusType>
        constexpr const DBusType& cast () const {return dynamic_cast<const DBusType&>(*this);}

        /**
         * Copy-assign a value of the same concrete DBus type.
         * @param rhs Value to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If <code>rhs</code> is not the same DBus type.
         */
        virtual dbus_type& operator= (const dbus_type& rhs) = 0;

        /**
         * Move-assign a value of the same concrete DBus type.
         * @param rhs Value to move from.
         * @return A reference to this object.
         * @throws std::invalid_argument If <code>rhs</code> is not the same DBus type.
         */
        virtual dbus_type& operator= (dbus_type&& rhs) = 0;

        /**
         * Compare this value for equality with another DBus value.
         * @param rhs Value to compare.
         * @return <code>true</code> when the values are equal.
         */
        virtual bool operator== (const dbus_type& rhs) const = 0;

        /**
         * Perform a three-way comparison with another DBus value.
         * @param rhs Value to compare.
         * @return The comparison result, or <code>unordered</code> when the
         *         values cannot be ordered.
         */
        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const = 0;


        /**
         * Check whether this instance is one of the DBus Basic types.
         * The DBus Basic types includes:
         *  - dbus_bool
         *  - dbus_byte
         *  - dbus_i16
         *  - dbus_u16
         *  - dbus_i32
         *  - dbus_u32
         *  - dbus_i64
         *  - dbus_u64
         *  - dbus_double
         *  - dbus_string
         *  - dbus_opath
         *  - dbus_signature
         *  - dbus_unix_fd
         * @return <code>true</code> if this instance is a DBus Basic value.
         */
        virtual bool is_basic () const {return false;}

        virtual bool is_bool () const {return false;}     /**< Check if this is a dbus_bool. */
        virtual bool is_byte () const {return false;}     /**< Check if this is a dbus_byte. */
        virtual bool is_i16 () const {return false;}      /**< Check if this is a dbus_i16. */
        virtual bool is_u16 () const {return false;}      /**< Check if this is a dbus_u16. */
        virtual bool is_i32 () const {return false;}      /**< Check if this is a dbus_i32. */
        virtual bool is_u32 () const {return false;}      /**< Check if this is a dbus_u32. */
        virtual bool is_i64 () const {return false;}      /**< Check if this is a dbus_i64. */
        virtual bool is_u64 () const {return false;}      /**< Check if this is a dbus_u64. */
        virtual bool is_double () const {return false;}   /**< Check if this is a dbus_double. */
        virtual bool is_string () const {return false;}   /**< Check if this is a dbus_string. */
        virtual bool is_opath () const {return false;}    /**< Check if this is a dbus_opath. */
        virtual bool is_signature () const {return false;}/**< Check if this is a dbus_signature. */
        virtual bool is_unix_fd () const {return false;}  /**< Check if this is a dbus_unix_fd. */
        virtual bool is_array () const {return false;}    /**< Check if this is a dbus_array. */
        virtual bool is_struct () const {return false;}   /**< Check if this is a dbus_struct. */
        virtual bool is_dict () const {return false;}     /**< Check if this is a dbus_dict. */
        virtual bool is_variant () const {return false;}  /**< Check if this is a dbus_variant. */

        /**
         * Return the DBus signature of this type.
         * @return The DBus signature. The returned reference remains valid for
         *         the lifetime of this object and/or until its signature changes.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#type-system rel="noopener noreferrer" target="_blank">DBus Type System at dbus.freedesktop.org</a>
         */
        constexpr const std::string& signature () const {return sig;}

        /**
         * Return the DBus type code of this type.
         * @return The DBus type code.
         * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#type-system rel="noopener noreferrer" target="_blank">DBus Type System at dbus.freedesktop.org</a>
         */
        virtual int type_code () const {return (int)sig[0];}

        /**
         * Return a string representation of the DBus value.
         * @return A string representation of the DBus value.
         */
        virtual std::string to_string () const = 0;

        /**
         * Return a string representation of the object in JSON format.
         * @return A string containing a JSON representation of the object.
         */
        virtual std::string to_json () const = 0;

        /**
         * Create a polymorphic copy of this instance.
         * @return A pointer to a copy of this object.
         */
        virtual std::unique_ptr<dbus_type> clone () const = 0;

       /**
         * Move this instance into a newly allocated instance.
         * @return A pointer to the new instance; this object remains
         *         valid but has an unspecified moved-from value.
         */
        virtual std::unique_ptr<dbus_type> clone_move () = 0;


    protected:
        /** Default constructor, creates a type with an empty DBus signature. */
        constexpr dbus_type () = default;

        /**
         * Construct a type with a DBus signature.
         * @param sig_arg Null-terminated DBus signature copied by this object.
         * @note The signature isn't verified to be a valid DBus signature.
         */
        constexpr dbus_type (const char* sig_arg) : sig{sig_arg} {}

        /**
         * Construct a type with a DBus signature.
         * @param sig_arg DBus signature copied by this object.
         * @note The signature isn't verified to be a valid DBus signature.
         */
        constexpr dbus_type (const std::string& sig_arg) : sig{sig_arg} {}

        std::string sig; /**< The DBus signature of this type. */
    };


}


#endif
