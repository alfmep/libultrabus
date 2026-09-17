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
#ifndef ULTRABUS_DBUS_VARIANT_HPP
#define ULTRABUS_DBUS_VARIANT_HPP

#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/utils.hpp>
#include <dbus/dbus.h>
#include <stdexcept>
#include <memory>
#include <string>


namespace ultrabus {


    /**
     * A class representing a DBus Variant type.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#container-types rel="noopener noreferrer" target="_blank">DBus Container Types at dbus.freedesktop.org</a>
     */
    class dbus_variant : public dbus_type {
    private:
        using dbus_type_ptr = std::unique_ptr<dbus_type>;

    public:
        /**
         * Construct a DBus Variant containing a DBus Basic signed
         * 32-bit integer with default value 0.
         * This is the default constructor and the value that the
         * variant holds can later be set to contain any other
         * DBus type and value.
         */
        dbus_variant () : dbus_type(DBUS_TYPE_VARIANT_AS_STRING), value (new dbus_i32) {
        }

        /**
         * Construct a DBus Variant by taking ownership of a pointer
         * to a DBus value.
         * @param pointer_to_value Value to transfer. If it is a variant, its
         *        contained value is transferred instead.
         * @post This variant owns the transferred pointer.
         * @note If <code>pointer_to_value</code> is a <code>nullptr</code>,
         *       a variant containing a default instance of a 32-bit
         *       signed integer will be created.
         */
        dbus_variant (std::unique_ptr<dbus_type>&& pointer_to_value)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING)
        {
            if (pointer_to_value == nullptr)
                value.reset (new dbus_i32);
            else if (pointer_to_value->is_variant())
                value = std::move (pointer_to_value->cast<dbus_variant>().value);
            else
                value = std::forward<std::unique_ptr<dbus_type>> (pointer_to_value);
        }

        /**
         * Construct a DBus Variant and let it contain a default value
         * of the type described by a signature.
         * @param value_signature Valid single DBus type signature.
         * @throws std::invalid_argument If <code>value_signature</code>
         *                               is not a valid signature of
         *                               a single DBus type.
         */
        dbus_variant (const char* value_signature)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING),
              value (create_dbus_type(value_signature))
        {
            if (value == nullptr)
                throw std::invalid_argument ("Can't create dbus_variant, invalid value signature");
        }
        /**
         * Construct a DBus Variant and let it contain a default value
         * of the type described by a signature.
         * @param value_signature Valid single DBus type signature.
         * @throws std::invalid_argument If <code>value_signature</code>
         *                               is not a valid signature of
         *                               a single DBus type.
         */
        dbus_variant (const std::string value_signature)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING),
              value (create_dbus_type(value_signature))
        {
            if (value == nullptr)
                throw std::invalid_argument ("Can't create dbus_variant, invalid value signature");
        }
        /**
         * Construct a DBus Variant and let it contain a default value
         * of the type described by a DBus type code.
         * @param type_code A DBus type code of a signle DBus type.
         * @throws std::invalid_argument If <code>value_signature</code>
         *                               is not a valid DBus type code.
         */
        dbus_variant (int type_code)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING),
              value (create_dbus_type(type_code))
        {
            if (value == nullptr)
                throw std::invalid_argument ("Can't create dbus_variant, invalid DBus type code");
        }

        /**
         * Copy contructor.
         * Make a deep copy of another DBus Variant.
         * @param other The DBus Variant to copy.
         */
        dbus_variant (const dbus_variant& other)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING)
        {
            value = other.get().clone ();
        }
        /**
         * Move constructor.
         * @param other The DBus Variant to move.
         */
        dbus_variant (dbus_variant&& other)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING)
        {
            value = std::move (other.value);
        }

        dbus_variant (const dbus_type& other)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING)
        {
            if (other.is_variant()) {
                value = dynamic_cast<const dbus_variant&>(other).get().clone ();
            }else{
                value = other.clone ();
            }
        }
        dbus_variant (dbus_type&& other)
            : dbus_type(DBUS_TYPE_VARIANT_AS_STRING)
        {
            if (other.is_variant()) {
                value = std::move (dynamic_cast<dbus_variant&>(other).value);
            }else{
                value = other.clone_move ();
            }
        }


        /**
         * Destroys the variant and its contained value.
         */
        virtual ~dbus_variant () = default;


        /**
         * Assign this variant from another dbus_type instance.
         * If <code>rhs</code> is itself a variant, its contained
         * value is copied; otherwise <code>rhs</code> is cloned
         * and stored as the contained value.
         * @param rhs Value to copy.
         * @return A reference to his variant as a generic DBus value.
         */
        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (this != &rhs) {
                if (rhs.is_variant()) {
                    value = dynamic_cast<const dbus_variant&>(rhs).get().clone ();
                }else{
                    value = rhs.clone ();
                }
            }
            return *this;
        }

        /**
         * Assign this variant by moving another dbus_type instance.
         * If <code>rhs</code> is itself a variant, its contained
         * value is moved; otherwise <code>rhs</code> is moved
         * and stored as the contained value.
         * @param rhs Value to move.
         * @return A reference to his variant as a generic DBus value.
         */
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (this != &rhs) {
                if (rhs.is_variant()) {
                    value = std::move (dynamic_cast<dbus_variant&>(rhs).value);
                }else{
                    value = rhs.clone_move ();
                }
            }
            return *this;
        }

        /**
         * Assignment operator. Make a deep copy of another dbus_variant.
         * @param rhs The DBus Variant to copy.
         *            If <code>rhs</code> is itself a variant, its contained
         *            value is copied; otherwise <code>rhs</code> is cloned
         *            and stored as the contained value.
         * @return A reference to this variant.
         */
        dbus_variant& operator= (const dbus_variant& rhs) {
            if (this != &rhs)
                value = rhs.get().clone ();
            return *this;
        }

        /**
         * Move operator.
         * @param rhs The DBus Variant to move.
         *            If <code>rhs</code> is itself a variant, its contained
         *            value is moved; otherwise <code>rhs</code> is moved
         *            and stored as the contained value.
         * @return A reference to this variant.
         */
        dbus_variant& operator= (dbus_variant&& rhs) {
            if (this != &rhs)
                value = std::move (rhs.value);
            return *this;
        }

        /**
         * Assign a DBus Basic value in form of a C++ type to this Variant.
         * @tparam T DBus basic C++ type.
         * @param rhs Value to copy.
         * @return A reference to this variant.
         */
        template<dbus_basic_cpp_types T>
        dbus_variant& operator= (const T& rhs) {
            value.reset (new dbus_basic<T>(rhs));
            return *this;
        }

        /**
         * Assign a string to this DBus Variant.
         * The contained value of this variant will be a <code>dbus_string</code>.
         * @param rhs Null-terminated string to copy.
         * @return A reference to this variant.
         */
        dbus_variant& operator= (const char* rhs) {
            if (value && value->is_string())
                value->cast<dbus_string>() = rhs;
            else
                value.reset (new dbus_string(rhs));
            return *this;
        }
        /**
         * Assign a string to this DBus Variant.
         * The contained value of this variant will be a <code>dbus_string</code>.
         * @param rhs String to copy.
         * @return A reference to this variant.
         */
        dbus_variant& operator= (const std::string& rhs) {
            if (value && value->is_string())
                value->cast<dbus_string>() = rhs;
            else
                value.reset (new dbus_string(rhs));
            return *this;
        }
        /**
         * Assign a string to this DBus Variant by moving it.
         * The contained value of this variant will be a <code>dbus_string</code>.
         * @param rhs String to move.
         * @return A reference to this variant.
         */
        dbus_variant& operator= (std::string&& rhs) {
            if (value && value->is_string())
                value->cast<dbus_string>() = std::forward<std::string>(rhs);
            else
                value.reset (new dbus_string(std::forward<std::string>(rhs)));
            return *this;
        }



        /**
         * Compare this variant with another dbus_variant instance.
         * @param rhs Variant to compare.
         * @return <code>true</code> If the contained value of both
         *                           variants are equal.
         */
        bool operator== (const dbus_variant& rhs) const {
            return get() == rhs.get();
        }

        /**
         * Compare the contained value of this variant with another dbus_type
         * instance. If <code>rhs</code> is itself a variant, its contained
         * value is compared; otherwise <code>rhs</code> is compared directly.
         * @param rhs Value to compare.
         * @return Whether the compared values are equal.
         */
        virtual bool operator== (const dbus_type& rhs) const {
            if (rhs.is_variant())
                return get() == rhs.cast<dbus_variant>().get ();
            else
                return get() == rhs;
        }

        /**
         * Compare the contained value of this variant with another dbus_type
         * instance.
         * @param rhs Value to compare.
         * @return Whether the compared values are equal.
         */
        template <any_dbus_type T>
            requires (!std::same_as<T, dbus_variant> && !std::same_as<T, dbus_type>)
        bool operator== (const T& rhs) const {
            return operator== (static_cast<const dbus_type&>(rhs));
        }

        /**
         * @brief Three-way compares the contained value with @p rhs.
         * @param rhs The variant to compare.
         * @return The comparison result of the contained values.
         */
        std::partial_ordering operator<=> (const dbus_variant& rhs) const {
            return get() <=> rhs.get();
        }

        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const {
            if (rhs.is_variant())
                return get() <=> rhs.cast<dbus_variant>().get ();
            else
                return get() <=> rhs;
        }

        /**
         * @brief Three-way compares the contained value with @p rhs.
         * @param rhs The variant to compare.
         * @return The comparison result of the contained values.
         */
        template <any_dbus_type T>
            requires (!std::same_as<T, dbus_variant> && !std::same_as<T, dbus_type>)
        std::partial_ordering operator<=> (const T& rhs) const {
            return operator<=> (static_cast<const dbus_type&>(rhs));
        }

        /**
         * Return <code>true</code> since this is a DBus Variant.
         * @return <code>true</code>.
         */
        virtual bool is_variant () const {return true;}

        /**
         * Return the signature of the contained value.
         * @return A non-owning reference to the contained value's DBus type signature.
         */
        const std::string& value_signature () const {
            return get().signature ();
        }


        /**
         * Get a reference to the contained value.
         * @tparam DBusType Expected contained DBus type.
         * @return A reference to the contained value owned by this variant.
         * @throws std::bad_cast If the contained value is not @p DBusType.
         */
        template<any_dbus_type DBusType>
        DBusType& get () {
            if constexpr (std::same_as<DBusType, dbus_variant>)
                return *this;
            else
                return get().cast<DBusType> ();
        }
        /**
         * Get a const reference to the contained value.
         * @tparam DBusType Expected contained DBus type.
         * @return A const reference to the contained value owned by this variant.
         * @throws std::bad_cast If the contained value is not @p DBusType.
         */
        template<any_dbus_type DBusType>
        const DBusType& get () const {
            if constexpr (std::same_as<DBusType, dbus_variant>)
                return *this;
            else
                return get().cast<DBusType> ();
        }


        /**
         * Get a reference to the contained value.
         * @return A reference to the contained value owned by this variant.
         */
        inline dbus_type& get () {
            if (value == nullptr)
                value.reset (new dbus_i32);
            return *value;
        }
        /**
         * Get a const reference to the contained value.
         * @return A const reference to the contained value owned by this variant.
         */
        const dbus_type& get () const {
            if (value == nullptr)
                value.reset (new dbus_i32);
            return *value;
        }


        /**
         * Get a reference to the contained underlying DBus basic C++ value.
         * @tparam BasicCppType Expected underlying DBus basic C++ type.
         * @return A reference to the contained basic value.
         * @throws std::bad_cast If the contained value is not the matching DBus basic type.
         */
        template<dbus_basic_cpp_types BasicCppType>
        BasicCppType& get () {
            return dynamic_cast<dbus_basic<BasicCppType>&>(get()).get();
        }
        /**
         * Get a const reference to the contained underlying DBus basic C++ value.
         * @tparam BasicCppType Expected underlying DBus basic C++ type.
         * @return A const reference to the contained basic value.
         * @throws std::bad_cast If the contained value is not the matching DBus basic type.
         */
        template<dbus_basic_cpp_types BasicCppType>
        const BasicCppType& get () const {
            return dynamic_cast<const dbus_basic<BasicCppType>&>(get()).get();
        }



        virtual std::string to_string () const {
            return get().to_string ();
        }

        virtual std::string to_json () const {
            std::string json (R"({"type":"variant","sign":"v","value":)");
            json.append (get().to_json());
            json.push_back ('}');
            return json;
        }

        virtual std::unique_ptr<dbus_type> clone () const {
            return dbus_type_ptr (new dbus_variant(*this));
        }

        virtual std::unique_ptr<dbus_type> clone_move () {
            return dbus_type_ptr (new dbus_variant(std::move(*this)));
        }


    private:
        mutable dbus_type_ptr value;
    };


}
#endif
