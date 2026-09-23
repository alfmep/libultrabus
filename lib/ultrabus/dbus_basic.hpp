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
#ifndef ULTRABUS_DBUS_BASIC_HPP
#define ULTRABUS_DBUS_BASIC_HPP

#include <ultrabus/dbus_type.hpp>
#include <stdexcept>
#include <concepts>
#include <compare>
#include <string>
#include <memory>
#include <cstdint>
#include <dbus/dbus.h>


namespace ultrabus {


    /**
     * Abstract base for all types of DBus Basic classes.
     */
    class dbus_basic_base : public dbus_type {
    public:
        virtual ~dbus_basic_base () = default;

        /**
         * Copy-assign a compatible DBus basic value.
         * @param rhs Value to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If <code>rhs</code> is incompatible.
         */
        virtual dbus_type& operator= (const dbus_type& rhs) = 0;
        /**
         * Move-assign a compatible DBus basic value.
         * @param rhs Value to move from.
         * @return A reference to this object.
         * @throws std::invalid_argument If <code>rhs</code> is incompatible.
         */
        virtual dbus_type& operator= (dbus_type&& rhs) = 0;

        virtual std::string to_string () const = 0;
        virtual std::unique_ptr<dbus_type> clone () const = 0;
        virtual std::unique_ptr<dbus_type> clone_move () = 0;

        /** Return <code>true</code> since this is a DBus Basic. */
        virtual bool is_basic () const {
            return true;
        }

        /**
         * Write this value to a DBus C API basic-value union.
         * @param db_value Destination union to populate. For string values,
         *        its <code>str</code> member borrows storage from this object
         *        and must not outlive it or a mutation of its value.
         */
        virtual void to_DBusBasicValue (DBusBasicValue& db_value) const = 0;

    protected:
        constexpr dbus_basic_base (const char* sig_arg) : dbus_type(sig_arg) {
        }
        bool compare_to_variant (const dbus_type& rhs) const;
        std::partial_ordering compare_three_way_to_variant (const dbus_type& rhs) const;
    };


    /**
     * This class represents a DBus Basic type.
     * @tparam BasicCppType C++ type used to store the DBus basic value.
     * @see ultrabus::dbus_bool
     * @see ultrabus::dbus_byte
     * @see ultrabus::dbus_i16
     * @see ultrabus::dbus_u16
     * @see ultrabus::dbus_i32
     * @see ultrabus::dbus_u32
     * @see ultrabus::dbus_i64
     * @see ultrabus::dbus_u64
     * @see ultrabus::dbus_double
     * @see ultrabus::dbus_string
     * @see ultrabus::dbus_opath
     * @see ultrabus::dbus_signature
     * @see ultrabus::dbus_unix_fd
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types rel="noopener noreferrer" target="_blank">DBus Basic Types at dbus.freedesktop.org</a>
     */
    template<dbus_basic_cpp_types BasicCppType>
    class dbus_basic : public dbus_basic_base {
    public:
        using value_type = BasicCppType; /**< C++ type used to store the value. */

        /**
         * Create an instance of a basic type with value 0, or
         * <code>false</code>, or an empty string.
         */
        constexpr dbus_basic () : dbus_basic_base(basic_signature) {
            if constexpr (std::same_as<value_type, std::string>)
                basic_value = "";
            else
                basic_value = 0;
        }

        /**
         * Copy constructor.
         * @param other The other instance to copy.
         */
        constexpr dbus_basic (const dbus_basic& other) : dbus_basic_base(basic_signature) {
            basic_value = other.basic_value;
        }

        /**
         * Construct a dbus_basic from a DBus basic value reference.
         * @param other The source basic value.
         * @throws std::invalid_argument If <code>other</code> is not a
         *         compatible DBus basic value.
         */
        dbus_basic (const dbus_type& other) : dbus_basic_base(basic_signature) {
            switch (other.type_code()) {
            case DBUS_TYPE_BOOLEAN:
                basic_value = other.cast<dbus_bool>().get();
                break;
            case DBUS_TYPE_BYTE:
                basic_value = other.cast<dbus_byte>().get();
                break;
            case DBUS_TYPE_INT16:
                basic_value = other.cast<dbus_i16>().get();
                break;
            case DBUS_TYPE_UINT16:
                basic_value = other.cast<dbus_u16>().get();
                break;
            case DBUS_TYPE_INT32:
                basic_value = other.cast<dbus_i32>().get();
                break;
            case DBUS_TYPE_UINT32:
                basic_value = other.cast<dbus_u32>().get();
                break;
            case DBUS_TYPE_INT64:
                basic_value = other.cast<dbus_i64>().get();
                break;
            case DBUS_TYPE_UINT64:
                basic_value = other.cast<dbus_u64>().get();
                break;
            case DBUS_TYPE_DOUBLE:
                basic_value = other.cast<dbus_double>().get();
                break;
            case DBUS_TYPE_UNIX_FD:
                basic_value = other.cast<dbus_basic<unix_fd_t>>().get();
                break;
            case DBUS_TYPE_STRING:
                throw std::invalid_argument ("Can't assign a dbus_string to a dbus_basic");
                break;
            case DBUS_TYPE_OBJECT_PATH:
                throw std::invalid_argument ("Can't assign a dbus_opath to a dbus_basic");
                break;
            case DBUS_TYPE_SIGNATURE:
                throw std::invalid_argument ("Can't assign a dbus_signature to a dbus_basic");
                break;
            default:
                throw std::invalid_argument ("Can't assign a non-dbus_basic to a dbus_basic");
            }
        }

        /**
         * Create an instance with an initial value.
         * @param value The initial value to copy.
         */
        constexpr dbus_basic (const value_type& value) : dbus_basic_base(basic_signature) {
            basic_value = value;
        }

        /**
         * Create an instance with an initial value from a DBusBasicValue.
         * @param value DBus C API union containing the initial value. String
         *        specializations copy the pointed-to text.
         */
        constexpr dbus_basic (const DBusBasicValue& value) : dbus_basic_base(basic_signature) {
            if constexpr (std::same_as<value_type, bool>)
                basic_value = value.bool_val;
            else if constexpr (std::same_as<value_type, uint8_t>)
                basic_value = value.byt;
            else if constexpr (std::same_as<value_type, int16_t>)
                basic_value = value.i16;
            else if constexpr (std::same_as<value_type, uint16_t>)
                basic_value = value.u16;
            else if constexpr (std::same_as<value_type, int32_t>)
                basic_value = value.i32;
            else if constexpr (std::same_as<value_type, uint32_t>)
                basic_value = value.u32;
            else if constexpr (std::same_as<value_type, int64_t>)
                basic_value = value.i64;
            else if constexpr (std::same_as<value_type, uint64_t>)
                basic_value = value.u64;
            else if constexpr (std::same_as<value_type, double>)
                basic_value = value.dbl;
            else if constexpr (std::same_as<value_type, std::string>)
                basic_value = value.str ? value.str : "";
            else
                static_assert (false, "Invalid DBus Basic type");
        }


        virtual ~dbus_basic () = default;

        /**
         * Copy-assign another basic value.
         * @param rhs Value to copy.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (const dbus_basic& rhs) {
            if (&rhs != this)
                basic_value = rhs.basic_value;
            return *this;
        }

        /**
         * Assign from a DBus basic value through its polymorphic base.
         * @param rhs Value to copy.
         * @return A reference to this object.
         * @throws std::invalid_argument If <code>rhs</code> is incompatible.
         */
        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (&rhs != this) {
                switch (rhs.type_code()) {
                case DBUS_TYPE_BOOLEAN:
                    basic_value = rhs.cast<dbus_bool>().get();
                    break;
                case DBUS_TYPE_BYTE:
                    basic_value = rhs.cast<dbus_byte>().get();
                    break;
                case DBUS_TYPE_INT16:
                    basic_value = rhs.cast<dbus_i16>().get();
                    break;
                case DBUS_TYPE_UINT16:
                    basic_value = rhs.cast<dbus_u16>().get();
                    break;
                case DBUS_TYPE_INT32:
                    basic_value = rhs.cast<dbus_i32>().get();
                    break;
                case DBUS_TYPE_UINT32:
                    basic_value = rhs.cast<dbus_u32>().get();
                    break;
                case DBUS_TYPE_INT64:
                    basic_value = rhs.cast<dbus_i64>().get();
                    break;
                case DBUS_TYPE_UINT64:
                    basic_value = rhs.cast<dbus_u64>().get();
                    break;
                case DBUS_TYPE_DOUBLE:
                    basic_value = rhs.cast<dbus_double>().get();
                    break;
                case DBUS_TYPE_UNIX_FD:
                    basic_value = rhs.cast<dbus_basic<unix_fd_t>>().get();
                    break;
                case DBUS_TYPE_STRING:
                    throw std::invalid_argument ("Can't assign a dbus_string to a dbus_basic");
                    break;
                case DBUS_TYPE_OBJECT_PATH:
                    throw std::invalid_argument ("Can't assign a dbus_opath to a dbus_basic");
                    break;
                case DBUS_TYPE_SIGNATURE:
                    throw std::invalid_argument ("Can't assign a dbus_signature to a dbus_basic");
                    break;
                default:
                    throw std::invalid_argument ("Can't assign a non-dbus_basic to a dbus_basic");
                }
            }
            return *this;
        }

        /**
         * Move from a DBus basic value through its polymorphic base.
         * @param rhs Value to move from. Numeric values are copied.
         * @return A reference to this object.
         * @throws std::invalid_argument If <code>rhs</code> is incompatible.
         */
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (&rhs != this) {
                switch (rhs.type_code()) {
                case DBUS_TYPE_BOOLEAN:
                    basic_value = rhs.cast<dbus_bool>().get();
                    break;
                case DBUS_TYPE_BYTE:
                    basic_value = rhs.cast<dbus_byte>().get();
                    break;
                case DBUS_TYPE_INT16:
                    basic_value = rhs.cast<dbus_i16>().get();
                    break;
                case DBUS_TYPE_UINT16:
                    basic_value = rhs.cast<dbus_u16>().get();
                    break;
                case DBUS_TYPE_INT32:
                    basic_value = rhs.cast<dbus_i32>().get();
                    break;
                case DBUS_TYPE_UINT32:
                    basic_value = rhs.cast<dbus_u32>().get();
                    break;
                case DBUS_TYPE_INT64:
                    basic_value = rhs.cast<dbus_i64>().get();
                    break;
                case DBUS_TYPE_UINT64:
                    basic_value = rhs.cast<dbus_u64>().get();
                    break;
                case DBUS_TYPE_DOUBLE:
                    basic_value = rhs.cast<dbus_double>().get();
                    break;
                case DBUS_TYPE_UNIX_FD:
                    basic_value = rhs.cast<dbus_basic<unix_fd_t>>().get();
                    break;
                case DBUS_TYPE_STRING:
                    throw std::invalid_argument ("Can't assign a dbus_string to a dbus_basic");
                    break;
                case DBUS_TYPE_OBJECT_PATH:
                    throw std::invalid_argument ("Can't assign a dbus_opath to a dbus_basic");
                    break;
                case DBUS_TYPE_SIGNATURE:
                    throw std::invalid_argument ("Can't assign a dbus_signature to a dbus_basic");
                    break;
                default:
                    throw std::invalid_argument ("Can't assign a non-dbus_basic to a dbus_basic");
                }
            }
            return *this;
        }

        /**
         * Assign a C++ basic value.
         * @param rhs Value to copy.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (const value_type& rhs) {
            basic_value = rhs;
            return *this;
        }

        /**
         * Assign from a DBus C API basic-value union.
         * @param dbval Source union. String specializations copy the
         *        pointed-to text rather than retaining the pointer.
         * @return A reference to this object.
         */
        virtual dbus_basic& operator= (const DBusBasicValue& dbval) {
            if constexpr (std::same_as<value_type, bool>)
                basic_value = dbval.bool_val;
            else if constexpr (std::same_as<value_type, uint8_t>)
                basic_value = dbval.byt;
            else if constexpr (std::same_as<value_type, int16_t>)
                basic_value = dbval.i16;
            else if constexpr (std::same_as<value_type, uint16_t>)
                basic_value = dbval.u16;
            else if constexpr (std::same_as<value_type, int32_t>)
                basic_value = dbval.i32;
            else if constexpr (std::same_as<value_type, uint32_t>)
                basic_value = dbval.u32;
            else if constexpr (std::same_as<value_type, int64_t>)
                basic_value = dbval.i64;
            else if constexpr (std::same_as<value_type, uint64_t>)
                basic_value = dbval.u64;
            else if constexpr (std::same_as<value_type, double>)
                basic_value = dbval.dbl;
            else if constexpr (std::same_as<value_type, std::string>)
                basic_value = dbval.str ? dbval.str : "";
            else
                static_assert (false, "Invalid DBus type");
            return *this;
        }

        /**
         * Compare with another DBus basic value.
         * @tparam N C++ storage type of the right-hand side value.
         * @param rhs Value to compare.
         * @return <code>true</code> if the stored values compare equal.
         */
        template<typename N>
        constexpr bool operator== (const dbus_basic<N>& rhs) const {
            return get() == rhs.get();
        }

        /**
         * Compare with a C++ basic value.
         * @tparam N Type of the value to compare.
         * @param rhs Value to compare.
         * @return <code>true</code> if the stored values compare equal.
         */
        template<dbus_basic_cpp_types N>
        constexpr bool operator== (const N& rhs) {
            return get() == rhs;
        }

        /**
         * Compare with a DBus C API basic-value union.
         * @param rhs Union containing the value to compare.
         * @return <code>true</code> if the active DBus basic values compare equal.
         */
        constexpr bool operator== (const DBusBasicValue& rhs) {
            if constexpr (std::same_as<value_type, bool>)
                return basic_value == rhs.bool_val;
            else if constexpr (std::same_as<value_type, uint8_t>)
                return basic_value == rhs.byt;
            else if constexpr (std::same_as<value_type, int16_t>)
                return basic_value == rhs.i16;
            else if constexpr (std::same_as<value_type, uint16_t>)
                return basic_value == rhs.u16;
            else if constexpr (std::same_as<value_type, int32_t>)
                return basic_value == rhs.i32;
            else if constexpr (std::same_as<value_type, uint32_t>)
                return basic_value == rhs.u32;
            else if constexpr (std::same_as<value_type, int64_t>)
                return basic_value == rhs.i64;
            else if constexpr (std::same_as<value_type, uint64_t>)
                return basic_value == rhs.u64;
            else if constexpr (std::same_as<value_type, double>)
                return basic_value == rhs.dbl;
            // else if constexpr (std::same_as<value_type, std::string>)
            //     return basic_value == rhs.str ? rhs.str : "";
            else
                static_assert (false, "Invalid DBus type");
        }

        /**
         * Compare with another polymorphic DBus value.
         * @param rhs Value to compare.
         * @return <code>true</code> when compatible values compare equal;
         *         otherwise <code>false</code>.
         */
        virtual bool operator== (const dbus_type& rhs) const {
            if (rhs.is_variant())
                return compare_to_variant (rhs);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
            switch (rhs.type_code()) {
            case DBUS_TYPE_BOOLEAN:
                return get() == rhs.cast<dbus_bool>().get();
            case DBUS_TYPE_BYTE:
                return get() == rhs.cast<dbus_byte>().get();
            case DBUS_TYPE_INT16:
                return get() == rhs.cast<dbus_i16>().get();
            case DBUS_TYPE_UINT16:
                return get() == rhs.cast<dbus_u16>().get();
            case DBUS_TYPE_INT32:
                return get() == rhs.cast<dbus_i32>().get();
            case DBUS_TYPE_UINT32:
                return get() == rhs.cast<dbus_u32>().get();
            case DBUS_TYPE_INT64:
                return get() == rhs.cast<dbus_i64>().get();
            case DBUS_TYPE_UINT64:
                return get() == rhs.cast<dbus_u64>().get();
            case DBUS_TYPE_DOUBLE:
                return get() == rhs.cast<dbus_double>().get();
            case DBUS_TYPE_UNIX_FD:
                return get() == rhs.cast<dbus_basic<unix_fd_t>>().get();
            // case DBUS_TYPE_STRING:
            // case DBUS_TYPE_OBJECT_PATH:
            // case DBUS_TYPE_SIGNATURE:
            default:
                return false;
            }
#pragma GCC diagnostic pop
        }

        /**
         * Three-way compare another DBus basic value.
         * @tparam N C++ storage type of the right-hand side value.
         * @param rhs Value to compare.
         * @return The comparison result.
         */
        template<typename N>
        constexpr auto operator<=> (const dbus_basic<N>& rhs) const {
            return get() <=> rhs.get();
        }

        /**
         * Three-way compare a C++ basic value.
         * @tparam N Type of the value to compare.
         * @param rhs Value to compare.
         * @return The comparison result.
         */
        template<dbus_basic_cpp_types N>
        constexpr auto operator<=> (const N& rhs) {
            return get() <=> rhs;
        }

        /**
         * Three-way compare a DBus C API basic-value union.
         * @param rhs Union containing the value to compare.
         * @return The comparison result.
         */
        constexpr auto operator<=> (const DBusBasicValue& rhs) {
            if constexpr (std::same_as<value_type, bool>)
                return basic_value <=> (bool)rhs.bool_val;
            else if constexpr (std::same_as<value_type, uint8_t>)
                return basic_value <=> rhs.byt;
            else if constexpr (std::same_as<value_type, int16_t>)
                return basic_value <=> rhs.i16;
            else if constexpr (std::same_as<value_type, uint16_t>)
                return basic_value <=> rhs.u16;
            else if constexpr (std::same_as<value_type, int32_t>)
                return basic_value <=> rhs.i32;
            else if constexpr (std::same_as<value_type, uint32_t>)
                return basic_value <=> rhs.u32;
            else if constexpr (std::same_as<value_type, int64_t>)
                return basic_value <=> rhs.i64;
            else if constexpr (std::same_as<value_type, uint64_t>)
                return basic_value <=> rhs.u64;
            else if constexpr (std::same_as<value_type, double>)
                return basic_value <=> rhs.dbl;
            // else if constexpr (std::same_as<value_type, std::string>)
            //     return std::partial_ordering::unordered;
            else
                static_assert (false, "Invalid DBus type");
        }

        /**
         * Three-way compare another polymorphic DBus value.
         * @param rhs Value to compare.
         * @return The comparison result, or <code>unordered</code> for
         *         incompatible values.
         */
        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const {
            if (rhs.is_variant())
                return compare_three_way_to_variant (rhs);
            if constexpr (std::same_as<value_type, bool>) {
                if (rhs.is_bool())
                    return get() <=> rhs.cast<dbus_bool>().get();
                else
                    return std::partial_ordering::unordered;
            }else{
                switch (rhs.type_code()) {
                case DBUS_TYPE_BOOLEAN:
                    return std::partial_ordering::unordered;
                case DBUS_TYPE_BYTE:
                    return get() <=> rhs.cast<dbus_byte>().get();
                case DBUS_TYPE_INT16:
                    if constexpr (std::same_as<value_type, uint64_t>)
                        return std::partial_ordering::unordered;
                    else if constexpr (std::same_as<value_type, uint32_t>)
                        return std::partial_ordering::unordered;
                    else
                        return get() <=> rhs.cast<dbus_i16>().get();
                case DBUS_TYPE_UINT16:
                    return get() <=> rhs.cast<dbus_u16>().get();
                case DBUS_TYPE_INT32:
                    if constexpr (std::same_as<value_type, uint64_t>)
                        return std::partial_ordering::unordered;
                    else if constexpr (std::same_as<value_type, uint32_t>)
                        return std::partial_ordering::unordered;
                    else
                        return get() <=> rhs.cast<dbus_i32>().get();
                case DBUS_TYPE_UINT32:
                    if constexpr (std::same_as<value_type, int16_t>)
                        return std::partial_ordering::unordered;
                    else if constexpr (std::same_as<value_type, int32_t>)
                        return std::partial_ordering::unordered;
                    else
                        return get() <=> rhs.cast<dbus_u32>().get();
                case DBUS_TYPE_INT64:
                    if constexpr (std::same_as<value_type, uint64_t>)
                        return std::partial_ordering::unordered;
                    else
                        return get() <=> rhs.cast<dbus_i64>().get();
                case DBUS_TYPE_UINT64:
                    if constexpr (std::same_as<value_type, uint64_t>)
                        return get() <=> rhs.cast<dbus_u64>().get();
                    else if constexpr (std::same_as<value_type, double>)
                        return get() <=> rhs.cast<dbus_u64>().get();
                    else
                        return std::partial_ordering::unordered;
                case DBUS_TYPE_DOUBLE:
                    return get() <=> rhs.cast<dbus_double>().get();
                case DBUS_TYPE_UNIX_FD:
                    if constexpr (std::same_as<value_type, unix_fd_t>)
                        return get() <=> rhs.cast<dbus_basic<unix_fd_t>>().get();
                    else
                        return std::partial_ordering::unordered;
                    // case DBUS_TYPE_STRING:
                    // case DBUS_TYPE_OBJECT_PATH:
                    // case DBUS_TYPE_SIGNATURE:
                default:
                    return std::partial_ordering::unordered;
                }
            }
        }

        /**
         * Convert to the stored C++ value.
         * @return A copy of the stored value.
         */
        constexpr operator value_type() const {
            return basic_value;
        }

        /**
         * Return mutable access to the stored value.
         * @return A reference owned by this object. It remains valid until
         *         the object is destroyed or its value is replaced.
         */
        constexpr value_type& get () {
            return basic_value;
        }

        /**
         * Return read-only access to the stored value.
         * @return A const reference owned by this object. It remains valid
         *         until the object is destroyed or its value is replaced.
         */
        constexpr const value_type& get () const {
            return basic_value;
        }

        /** @return Whether this value represents a DBus boolean type. */
        virtual bool is_bool () const {if constexpr(std::same_as<value_type, bool>) return true; else return false;}
        /** @return Whether this value represents a DBus byte type. */
        virtual bool is_byte () const {if constexpr (std::same_as<value_type, uint8_t>) return true; else return false;}
        /** @return Whether this value represents a DBus signed 16-bit integer type. */
        virtual bool is_i16 () const {if constexpr (std::same_as<value_type, int16_t>) return true; else return false;}
        /** @return Whether this value represents a DBus unsigned 16-bit integer type. */
        virtual bool is_u16 () const {if constexpr (std::same_as<value_type, uint16_t>) return true; else return false;}
        /** @return Whether this value represents a DBus signed 32-bit integer type. */
        virtual bool is_i32 () const {if constexpr (std::same_as<value_type, int32_t>) return true; else return false;}
        /** @return Whether this value represents a DBus unsigned 32-bit integer type. */
        virtual bool is_u32 () const {if constexpr (std::same_as<value_type, uint32_t>) return true; else return false;}
        /** @return Whether this value represents a DBus signed 64-bit integer type. */
        virtual bool is_i64 () const {if constexpr (std::same_as<value_type, int64_t>) return true; else return false;}
        /** @return Whether this value represents a DBus unsigned 64-bit integer type. */
        virtual bool is_u64 () const {if constexpr (std::same_as<value_type, uint64_t>) return true; else return false;}
        /** @return Whether this value represents a DBus double type. */
        virtual bool is_double () const {
            if constexpr (std::same_as<value_type, double>) return true; else return false;
        }

        /**
         * Write the stored numeric value to a DBus C API basic-value union, or
         * in case of a <code>dbus_string</code>, write a borrowed C string
         * pointer to a DBus C API union.
         * @param db_value Destination union to populate.
         * @note If this is a <code>dbus_string</code> instance, the <code>str</code>
         *       member of <code>db_value</code> must not outlive this object or
         *       a modification of its text
         */
        virtual void to_DBusBasicValue (DBusBasicValue& db_value) const {
            if constexpr (std::same_as<value_type, bool>)
                db_value.bool_val = basic_value;
            else if constexpr (std::same_as<value_type, uint8_t>)
                db_value.byt = basic_value;
            else if constexpr (std::same_as<value_type, int16_t>)
                db_value.i16 = basic_value;
            else if constexpr (std::same_as<value_type, uint16_t>)
                db_value.u16 = basic_value;
            else if constexpr (std::same_as<value_type, int32_t>)
                db_value.i32 = basic_value;
            else if constexpr (std::same_as<value_type, uint32_t>)
                db_value.u32 = basic_value;
            else if constexpr (std::same_as<value_type, int64_t>)
                db_value.i64 = basic_value;
            else if constexpr (std::same_as<value_type, uint64_t>)
                db_value.u64 = basic_value;
            else if constexpr (std::same_as<value_type, double>)
                db_value.dbl = basic_value;
            // else if constexpr (std::same_as<value_type, std::string>)
            //     return std::partial_ordering::unordered;
            else
                static_assert (false, "Invalid DBus type");
        }

        /**
         * Create a polymorphic copy of this value.
         * @return Unique ownership of an independent copy.
         */
        virtual std::unique_ptr<dbus_type> clone () const {
            return std::unique_ptr<dbus_type> (new dbus_basic<value_type>(*this));
        }

        /**
         * Move this value into a new polymorphic instance.
         * @return Unique ownership of the new value; this object remains valid
         *         with an unspecified moved-from value.
         */
        virtual std::unique_ptr<dbus_type> clone_move () {
            return std::unique_ptr<dbus_type> (new dbus_basic<value_type>(std::move(*this)));
        }

        /**
         * Return the DBus signature for this dbus_basic type.
         * @return A newly allocated single-type DBus signature.
         */
        static constexpr std::string make_signature () {
            return basic_signature;
        }

        /**
         * Return the DBus type code for this C++ storage type.
         * @return The DBus C API type code.
         */
        static constexpr int make_type_code () {
            return basic_type_code;
        }

        /**
         * Return a textual representation of the DBus Basic value.
         * @return A newly allocated string representation.
         */
        virtual std::string to_string () const {
            if constexpr (std::same_as<value_type, bool>)
                return basic_value ? "true" : "false";
            else
                return std::to_string (basic_value);
        }

        virtual std::string to_json () const {
            std::string json;
            if constexpr (std::same_as<value_type, bool>) {
                json = R"({"type":"boolean","signature":"b","value":)";
                json.append (basic_value ? "true" : "false");
            }else if constexpr (std::same_as<value_type, uint8_t>) {
                json = R"({"type":"byte","signature":"y","value":)";
                json.append (std::to_string((unsigned)basic_value));
            }else if constexpr (std::same_as<value_type, int16_t>) {
                json = R"({"type":"int16","signature":"n","value":)";
                json.append (std::to_string(basic_value));
            }else if constexpr (std::same_as<value_type, uint16_t>) {
                json = R"({"type":"uint16","signature":"q","value":)";
                json.append (std::to_string(basic_value));
            }else if constexpr (std::same_as<value_type, int32_t>) {
                json = R"({"type":"int32","signature":"i","value":)";
                json.append (std::to_string(basic_value));
            }else if constexpr (std::same_as<value_type, uint32_t>) {
                json = R"({"type":"uint32","signature":"u","value":)";
                json.append (std::to_string(basic_value));
            }else if constexpr (std::same_as<value_type, int64_t>) {
                json = R"({"type":"int64","signature":"x","value":)";
                json.append (std::to_string(basic_value));
            }else if constexpr (std::same_as<value_type, uint64_t>) {
                json = R"({"type":"uint64","signature":"t","value":)";
                json.append (std::to_string(basic_value));
            }else if constexpr (std::same_as<value_type, double>) {
                json = R"({"type":"double","signature":"d","value":)";
                json.append (std::to_string(basic_value));
            }
            json.push_back ('}');
            return json;
        }

    protected:
        /** Stored C++ value representing the DBus Basic type. */
        value_type basic_value;

    private:
        static constexpr const char* basic_signature =
            std::same_as<value_type, bool> ? DBUS_TYPE_BOOLEAN_AS_STRING :
            std::same_as<value_type, uint8_t> ? DBUS_TYPE_BYTE_AS_STRING :
            std::same_as<value_type, int16_t> ? DBUS_TYPE_INT16_AS_STRING :
            std::same_as<value_type, uint16_t> ? DBUS_TYPE_UINT16_AS_STRING :
            std::same_as<value_type, int32_t> ? DBUS_TYPE_INT32_AS_STRING :
            std::same_as<value_type, uint32_t> ? DBUS_TYPE_UINT32_AS_STRING :
            std::same_as<value_type, int64_t> ? DBUS_TYPE_INT64_AS_STRING :
            std::same_as<value_type, uint64_t> ? DBUS_TYPE_UINT64_AS_STRING :
            std::same_as<value_type, double> ? DBUS_TYPE_DOUBLE_AS_STRING :
            std::same_as<value_type, std::string> ? DBUS_TYPE_STRING_AS_STRING :
            DBUS_TYPE_INVALID_AS_STRING;
        static constexpr const int basic_type_code =
            std::same_as<value_type, bool> ? DBUS_TYPE_BOOLEAN :
            std::same_as<value_type, uint8_t> ? DBUS_TYPE_BYTE :
            std::same_as<value_type, int16_t> ? DBUS_TYPE_INT16 :
            std::same_as<value_type, uint16_t> ? DBUS_TYPE_UINT16 :
            std::same_as<value_type, int32_t> ? DBUS_TYPE_INT32 :
            std::same_as<value_type, uint32_t> ? DBUS_TYPE_UINT32 :
            std::same_as<value_type, int64_t> ? DBUS_TYPE_INT64 :
            std::same_as<value_type, uint64_t> ? DBUS_TYPE_UINT64 :
            std::same_as<value_type, double> ? DBUS_TYPE_DOUBLE :
            std::same_as<value_type, std::string> ? DBUS_TYPE_STRING :
            DBUS_TYPE_INVALID;
    };


    /**
     * Create a DBus Basic instance from a DBus type code.
     * @param type_code_arg DBus C API type code for a supported basic type.
     * @return A unique pointer to an allocated DBus Basic value, or
     *         <code>nullptr</code> if the type code is not supported
     */
    std::unique_ptr<dbus_type> create_dbus_basic (int type_code_arg);


}
#endif
