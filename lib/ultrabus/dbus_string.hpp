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
#ifndef ULTRABUS_DBUS_STRING_HPP
#define ULTRABUS_DBUS_STRING_HPP

#include <ultrabus/dbus_basic.hpp>

namespace ultrabus {


    /**
     * This class represents a DBus Basic string type.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#basic-types rel="noopener noreferrer" target="_blank">DBus Basic Types at dbus.freedesktop.org</a>
     */
    template<>
    class dbus_basic<std::string> : public dbus_basic_base {
    public:
        using value_type = std::string; /**< C++ type used to store the string. */

        /**
         * Default constructor.
         * Create an instance with an empty string value.
         */
        constexpr dbus_basic () : dbus_basic_base (basic_signature) {
        }

        /**
         * Copy constructor.
         * @param other The other instance to copy.
         */
        constexpr dbus_basic (const dbus_basic& other)
            : dbus_basic_base (basic_signature),
              basic_value (other.basic_value)
        {
        }

        /**
         * Move constructor.
         * @param other The other instance to move. It remains valid with an
         *        unspecified moved-from value.
         */
        constexpr dbus_basic (dbus_basic&& other)
            : dbus_basic_base (basic_signature),
              basic_value (std::move(other.basic_value))
        {
        }

        /**
         * Create an instance with an initial value.
         * @param value Null-terminated text to copy.
         */
        constexpr dbus_basic (const char* value)
            : dbus_basic_base (basic_signature),
              basic_value (value?value:"")
        {
        }

        /**
         * Create an instance with an initial value.
         * @param value Text to copy.
         */
        constexpr dbus_basic (const std::string& value)
            : dbus_basic_base (basic_signature),
              basic_value (value)
        {
        }

        /**
         * Create an instance with an initial value.
         * @param value Text to move from.
         */
        constexpr dbus_basic (std::string&& value)
            : dbus_basic_base (basic_signature),
              basic_value (std::forward<std::string>(value))
        {
        }

        /**
         * Create an instance with an initial value from a DBusBasicValue.
         * @param value C API union whose string pointer is copied, if non-null.
         */
        constexpr dbus_basic (const DBusBasicValue& value) : dbus_basic_base(basic_signature) {
            basic_value = value.str ? value.str : "";
        }

        virtual ~dbus_basic () = default;

        /**
         * Copy-assign another DBus string.
         * @param rhs Value to copy.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (const dbus_basic& rhs) {
            if (&rhs != this)
                basic_value = rhs.basic_value;
            return *this;
        }

        /**
         * Move-assign another DBus string.
         * @param rhs Value to move from.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (dbus_basic&& rhs) {
            if (&rhs != this)
                basic_value = std::move (rhs.basic_value);
            return *this;
        }

        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (dynamic_cast<const dbus_basic<value_type>&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_string");
        }
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (type_code() == rhs.type_code())
                return operator= (std::move(dynamic_cast<dbus_basic<value_type>&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_string");
        }

        /**
         * Assign null-terminated text.
         * @param rhs Text to copy.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (const char* rhs) {
            basic_value = rhs;
            return *this;
        }

        /**
         * Copy-assign text.
         * @param rhs Text to copy.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (const std::string& rhs) {
            basic_value = rhs;
            return *this;
        }

        /**
         * Move-assign text.
         * @param rhs Text to move from.
         * @return A reference to this object.
         */
        constexpr dbus_basic& operator= (std::string&& rhs) {
            basic_value = std::forward<std::string>(rhs);
            return *this;
        }

        /**
         * Assign text from a DBus C API union.
         * @param dbval Union whose string pointer is copied.
         * @return A reference to this object.
         */
        virtual dbus_basic& operator= (const DBusBasicValue& dbval) {
            basic_value = dbval.str ? dbval.str : "";
            return *this;
        }

        /**
         * Compare with another DBus string.
         * @param rhs Value to compare.
         * @return <code>true</code> if the strings are equal.
         */
        constexpr bool operator== (const dbus_basic<std::string>& rhs) const {
            return get() == rhs.get();
        }

        /**
         * Compare with null-terminated text.
         * @param rhs Text to compare.
         * @return <code>true</code> if the strings are equal.
         */
        constexpr bool operator== (const char* rhs) const {
            return get() == rhs;
        }

        /**
         * Compare with a C++ string.
         * @param rhs Text to compare.
         * @return <code>true</code> if the strings are equal.
         */
        constexpr bool operator== (const std::string& rhs) const {
            return get() == rhs;
        }

        /**
         * Compare with text in a DBus C API union.
         * @param rhs Union whose string pointer is read.
         * @return <code>true</code> if the strings are equal.
         */
        constexpr bool operator== (const DBusBasicValue& rhs) const {
            return get() == (rhs.str ? rhs.str : "");
        }

        virtual bool operator== (const dbus_type& rhs) const {
            if (rhs.is_variant())
                return compare_to_variant (rhs);
            switch (rhs.type_code()) {
            case DBUS_TYPE_STRING:
            case DBUS_TYPE_OBJECT_PATH:
            case DBUS_TYPE_SIGNATURE:
                return get() == dynamic_cast<const dbus_basic<std::string>&>(rhs).get();
            default:
                return false;
            }
        }

        /**
         * Three-way compare another DBus basic value.
         * @tparam N C++ storage type of the other value.
         * @param rhs Value to compare.
         * @return The comparison result, or <code>unordered</code> for a
         *         non-string value.
         */
        template<typename N>
        constexpr auto operator<=> (const dbus_basic<N>& rhs) const {
            if constexpr (std::same_as<N, std::string>)
                return get() <=> rhs.get();
            else
                return std::partial_ordering::unordered;
        }

        /**
         * Three-way compare null-terminated text.
         * @param rhs Text to compare.
         * @return The lexical comparison result.
         */
        constexpr auto operator<=> (const char* rhs) const {
            return get() <=> rhs;
        }

        /**
         * Three-way compare C++ string text.
         * @param rhs Text to compare.
         * @return The lexical comparison result.
         */
        constexpr auto operator<=> (const std::string rhs) const {
            return get() <=> rhs;
        }

        /**
         * Three-way compare text in a DBus C API union.
         * @param rhs Union whose string pointer is read.
         * @return The lexical comparison result.
         */
        constexpr auto operator<=> (const DBusBasicValue& rhs) const {
            return get() <=> (rhs.str ? rhs.str : "");
        }

        /**
         * Three-way compare a polymorphic DBus basic value.
         * @param rhs Value to compare.
         * @return The lexical comparison result for string-like values, or
         *         <code>unordered</code> otherwise.
         */
        constexpr std::partial_ordering operator<=> (const dbus_basic_base& rhs) const {
            switch (rhs.type_code()) {
            case DBUS_TYPE_STRING:
            case DBUS_TYPE_OBJECT_PATH:
            case DBUS_TYPE_SIGNATURE:
                return get() <=> dynamic_cast<const dbus_basic<std::string>&>(rhs).get();
            default:
                return std::partial_ordering::unordered;
            }
        }

        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const {
            if (rhs.is_variant())
                return compare_three_way_to_variant (rhs);
            switch (rhs.type_code()) {
            case DBUS_TYPE_STRING:
            case DBUS_TYPE_OBJECT_PATH:
            case DBUS_TYPE_SIGNATURE:
                return get() <=> dynamic_cast<const dbus_basic<std::string>&>(rhs).get();
            default:
                return std::partial_ordering::unordered;
            }
        }

        /**
         * Convert to a C++ string.
         * @return A copy of the stored text.
         */
        constexpr operator std::string() const {
            return basic_value;
        }

        /** Clear the stored text. References and pointers into it may be invalidated. */
        constexpr void clear () {basic_value.clear();}

        /**
         * Check if the string is empty.
         * @return Whether the stored text is empty.
         */
        constexpr bool empty () const {return basic_value.empty();}

        /**
         * Get the size of the string.
         * @return Number of characters in the stored text.
         */
        constexpr std::string::size_type size () const {return basic_value.size();}

        /**
         * Return a null-terminated view of the stored text.
         * @return A non-owning pointer valid until this object is modified or destroyed.
         */
        constexpr const char* c_str () {return basic_value.c_str();}

        /**
         * Return mutable access to the stored text.
         * @return A reference owned by this object.
         */
        constexpr std::string& get () {
            return basic_value;
        }

        /**
         * Return read-only access to the stored text.
         * @return A const reference owned by this object.
         */
        constexpr const std::string& get () const {
            return basic_value;
        }

        /** @return <code>true</code> because this is a DBus string value. */
        virtual bool is_string () const {return true;}

        /**
         * Write a borrowed C string pointer to a DBus C API union.
         * @param db_value Destination union. Its <code>str</code> member must
         *        not outlive this object or a modification of its text.
         */
        virtual void to_DBusBasicValue (DBusBasicValue& db_value) const {
            db_value.str = const_cast<char*> (basic_value.c_str());
        }

        virtual std::unique_ptr<dbus_type> clone () const {
            return std::unique_ptr<dbus_type> (new dbus_basic<std::string>(*this));
        }

        virtual std::unique_ptr<dbus_type> clone_move () {
            return std::unique_ptr<dbus_type> (new dbus_basic<std::string>(std::move(*this)));
        }

        /**
         * Static method to return the signature of a DBus String type.
         * @return The signature of a DBus String type.
         */
        static constexpr std::string make_signature () {
            return basic_signature;
        }

        /**
         * Static method to return the type code of a DBus String type.
         * @return The type code for a DBus String type.
         */
        static constexpr int make_type_code () {
            return basic_type_code;
        }

        virtual std::string to_string () const {
            return basic_value;
        }

        virtual std::string to_json () const {
            static const char* hex = "0123456789abcdef";
            std::string json (R"({"type":")");
            switch (type_code()) {
            case 's':
                json.append (R"(string","sign":"s","value":")");
                break;
            case 'o':
                json.append (R"(object_path","sign":"o","value":")");
                break;
            case 'g':
                json.append (R"(signature","sign":"g","value":")");
                break;
            default:
                json.append (R"(invalid","sign":"-","value":")");
            }
            for (unsigned char ch : basic_value) {
                if (ch<0x20 || ch=='"' || ch=='\\') {
                    switch (ch) {
                    case 0x08: // backspace
                        json.append (R"(\b)");
                        break;
                    case 0x09: // tab
                        json.append (R"(\t)");
                        break;
                    case 0x0a: // newline
                        json.append (R"(\n)");
                        break;
                    case 0x0c: // formfeed
                        json.append (R"(\f)");
                        break;
                    case 0x0d: // return
                        json.append (R"(\r)");
                        break;
                    case 0x22: // double quote
                        json.append (R"(\")");
                        break;
                    // case 0x2f: // slash
                    //     json.append (R"(\/)");
                    //     break;
                    case 0x5c: // backslash
                        json.append (R"(\\)");
                        break;
                    default:
                        if (ch < 0x10)
                            json.append (R"(\u000)");
                        else
                            json.append (R"(\u001)");
                        json.push_back (hex[ch & 0x0f]);
                    }
                }else{
                    json.push_back (ch);
                }
            }
            json.append (R"("})");
            return json;
        }


    protected:
        std::string basic_value;

    private:
        static constexpr const char* basic_signature = DBUS_TYPE_STRING_AS_STRING;
        static constexpr const int basic_type_code = DBUS_TYPE_STRING;
    };


}
#endif
