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
#ifndef ULTRABUS_DBUS_DICT_HPP
#define ULTRABUS_DBUS_DICT_HPP

#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <dbus/dbus.h>
#include <string>
#include <string_view>
#include <map>


namespace ultrabus {


    /**
     * Abstract base for DBus dictionaries with different key types.
     */
    class dbus_dict_base : public dbus_type {
    public:
        virtual ~dbus_dict_base () = default;

        virtual dbus_type& operator= (const dbus_type& rhs) = 0;
        virtual dbus_type& operator= (dbus_type&& rhs) = 0;

        /** Returns <code>true</code> since this is a DBus dictionary. */
        virtual bool is_dict () const {return true;}

        /** Returns the DBus type code for dictionary keys. */
        virtual const int key_type_code () const = 0;

        /** @brief Return string view of the dictionary value signature. */
        virtual const std::string_view value_signature () const = 0;
        // virtual bool empty () const = 0;
        // virtual size_t size () const = 0;
        // virtual void clear () = 0;

    protected:
        constexpr dbus_dict_base (const char* sig_arg) : dbus_type(sig_arg) {}
        constexpr dbus_dict_base (const std::string& sig_arg) : dbus_type{sig_arg} {}
    };



    /**
     * A DBus dictionary with a fixed basic key type.
     * @tparam KeyDBusBasicType DBus basic type used as a key in the dict.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#container-types rel="noopener noreferrer" target="_blank">DBus Container Types at dbus.freedesktop.org</a>
     */
    template<any_dbus_basic_type KeyDBusBasicType>
    class dbus_dict : public dbus_dict_base {
    private:
        using dbus_type_ptr = std::unique_ptr<dbus_type>;

    public:
        /** DBus wrapper type used for dictionary keys. */
        using key_type_t = KeyDBusBasicType;

        /**
         * Const iterator over dictionary entries.
         */
        class const_iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = const std::pair<const key_type_t&, dbus_type&>;
            //using difference_type = ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;
            constexpr const_iterator () = default;
            constexpr const_iterator (const const_iterator& rhs) : iter(rhs.iter) {}
            constexpr const_iterator& operator= (const const_iterator& rhs) {
                if (this != &rhs)
                    iter = rhs.iter;
                return *this;
            }
            constexpr const_iterator& operator++ () {++iter; return *this;}
            constexpr const_iterator operator++ (int) {const_iterator i(iter); ++iter; return i;}
            constexpr const_iterator& operator-- () {--iter; return *this;}
            constexpr const_iterator operator-- (int) {const_iterator i(iter); --iter; return i;}
            inline value_type& operator*() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return *kv_ptr;
            }
            inline value_type* operator->() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return kv_ptr.get ();
            }
            constexpr bool operator== (const const_iterator& rhs) const {return iter==rhs.iter;}
            constexpr auto operator<=> (const const_iterator& rhs) const {return iter<=>rhs.iter;}
        private:
            friend class dbus_dict;
            constexpr const_iterator (const std::map<key_type_t, dbus_type_ptr>::const_iterator& i) : iter(i){}
            std::map<key_type_t, dbus_type_ptr>::const_iterator iter;
            std::unique_ptr<std::pair<const key_type_t&, dbus_type&>> kv_ptr;
        };

        /**
         * Iterator over dictionary entries.
         */
        class iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::pair<const key_type_t&, dbus_type&>;
            using difference_type = ptrdiff_t;
            using pointer = dbus_type*;
            using reference = dbus_type&;
            constexpr iterator () = default;
            constexpr iterator (const iterator& rhs) : iter(rhs.iter) {}
            constexpr operator const_iterator() const {return const_iterator(iter);}
            constexpr iterator& operator= (const iterator& rhs) {
                if (this != &rhs)
                    iter = rhs.iter;
                return *this;
            }
            constexpr iterator& operator++ () {++iter; return *this;}
            constexpr iterator operator++ (int) {iterator i(iter); ++iter; return i;}
            constexpr iterator& operator-- () {--iter; return *this;}
            constexpr iterator operator-- (int) {iterator i(iter); --iter; return i;}
            inline value_type& operator*() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return *kv_ptr;
            }
            inline value_type* operator->() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return kv_ptr.get ();
            }
            constexpr bool operator== (const iterator& rhs) const {return iter==rhs.iter;}
            constexpr auto operator<=> (const iterator& rhs) const {return iter<=>rhs.iter;}
        private:
            friend class dbus_dict;
            constexpr iterator (const std::map<key_type_t, dbus_type_ptr>::iterator& i) : iter(i){}
            std::map<key_type_t, dbus_type_ptr>::iterator iter;
            std::unique_ptr<std::pair<const key_type_t&, dbus_type&>> kv_ptr;
        };

        /**
         * Const reverse iterator over dictionary entries.
         */
        class const_reverse_iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = const std::pair<const key_type_t&, dbus_type&>;
            using difference_type = ptrdiff_t;
            using pointer = dbus_type*;
            using reference = dbus_type&;
            constexpr const_reverse_iterator () = default;
            constexpr const_reverse_iterator (const const_reverse_iterator& rhs) : iter(rhs.iter) {}
            constexpr const_reverse_iterator& operator= (const const_reverse_iterator& rhs) {
                if (this != &rhs)
                    iter = rhs.iter;
                return *this;
            }
            constexpr const_reverse_iterator& operator++ () {++iter; return *this;}
            constexpr const_reverse_iterator operator++ (int) {const_reverse_iterator i(iter); ++iter; return i;}
            constexpr const_reverse_iterator& operator-- () {--iter; return *this;}
            constexpr const_reverse_iterator operator-- (int) {const_reverse_iterator i(iter); --iter; return i;}
            inline value_type& operator*() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return *kv_ptr;
            }
            inline value_type* operator->() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return kv_ptr.get ();
            }
            constexpr bool operator== (const const_reverse_iterator& rhs) const {return iter==rhs.iter;}
            constexpr auto operator<=> (const const_reverse_iterator& rhs) const {return iter<=>rhs.iter;}
        private:
            friend class dbus_dict;
            constexpr const_reverse_iterator (const std::map<key_type_t, dbus_type_ptr>::const_reverse_iterator& i)
                : iter(i){}
            std::map<key_type_t, dbus_type_ptr>::const_reverse_iterator iter;
            std::unique_ptr<std::pair<const key_type_t&, dbus_type&>> kv_ptr;
        };

        /**
         * Reverse iterator over dictionary entries.
         */
        class reverse_iterator {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = std::pair<const key_type_t&, dbus_type&>;
            using difference_type = ptrdiff_t;
            using pointer = dbus_type*;
            using reference = dbus_type&;
            constexpr reverse_iterator () = default;
            constexpr reverse_iterator (const reverse_iterator& rhs) : iter(rhs.iter) {}
            constexpr operator const_reverse_iterator() const {return const_reverse_iterator(iter);}
            constexpr reverse_iterator& operator= (const reverse_iterator& rhs) {
                if (this != &rhs)
                    iter = rhs.iter;
                return *this;
            }
            constexpr reverse_iterator& operator++ () {++iter; return *this;}
            constexpr reverse_iterator operator++ (int) {reverse_iterator i(iter); ++iter; return i;}
            constexpr reverse_iterator& operator-- () {--iter; return *this;}
            constexpr reverse_iterator operator-- (int) {reverse_iterator i(iter); --iter; return i;}
            inline value_type& operator*() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return *kv_ptr;
            }
            inline value_type* operator->() {
                kv_ptr.reset (new std::pair<const key_type_t&, dbus_type&>(iter->first, *iter->second));
                return kv_ptr.get ();
            }
            constexpr bool operator== (const reverse_iterator& rhs) const {return iter==rhs.iter;}
            constexpr auto operator<=> (const reverse_iterator& rhs) const {return iter<=>rhs.iter;}
        private:
            friend class dbus_dict;
            constexpr reverse_iterator (const std::map<key_type_t, dbus_type_ptr>::reverse_iterator& i) : iter(i){}
            std::map<key_type_t, dbus_type_ptr>::reverse_iterator iter;
            std::unique_ptr<std::pair<const key_type_t&, dbus_type&>> kv_ptr;
        };


        /**
         * Default constructor. Creates a dictionary whose values are DBus variants.
         * The resulting value signature is <code>v</code>.
         */
        dbus_dict ()
            : dbus_dict_base(DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING)
        {
            sig.push_back (key_type_t::make_type_code());
            sig.push_back (DBUS_TYPE_VARIANT);
            sig.push_back (DBUS_DICT_ENTRY_END_CHAR);
        }

        /**
         * Constructs a dictionary with a fixed value signature.
         * @param value_signature The signature of the values in the dictionary.
         * @throws std::invalid_argument If the signature is invalid.
         */
        dbus_dict (const char* value_signature)
            : dbus_dict_base(DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING)
        {
            if (!value_signature || !dbus_signature_validate_single(value_signature, nullptr))
                throw std::invalid_argument ("Invalid DBus Dict value signature");
            sig.push_back (key_type_t::make_type_code());
            sig.append (value_signature);
            sig.push_back (DBUS_DICT_ENTRY_END_CHAR);
        }

        /**
         * Constructs a dictionary with a fixed value signature.
         * @param value_signature The signature of the values in the dictionary.
         * @throws std::invalid_argument If the signature is invalid.
         */
        dbus_dict (const std::string& value_signature)
            : dbus_dict_base(DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING)
        {
            if (!dbus_signature_validate_single(value_signature.c_str(), nullptr))
                throw std::invalid_argument ("Invalid DBus Dict value signature");
            sig.push_back (key_type_t::make_type_code());
            sig.append (value_signature);
            sig.push_back (DBUS_DICT_ENTRY_END_CHAR);
        }

        /**
         * Constructs a dictionary with a fixed basic or variant value type.
         * @param variant_or_basic_type_code A DBus Basic or Variant type code of the values in the dictionary.
         * @throws std::invalid_argument If <code>variant_or_basic_type_code</code> is
         *         neither a basic DBus type nor <code>DBUS_TYPE_VARIANT</code>.
         */
        dbus_dict (int variant_or_basic_type_code)
            : dbus_dict_base(DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING)
        {
            if (variant_or_basic_type_code!=DBUS_TYPE_VARIANT  &&  !dbus_type_is_basic(variant_or_basic_type_code))
                throw std::invalid_argument ("Invalid DBus Dict value signature");
            sig.push_back (key_type_t::make_type_code());
            sig.push_back (variant_or_basic_type_code);
            sig.push_back (DBUS_DICT_ENTRY_END_CHAR);
        }

        /**
         * Copy constructor.
         * @param other The <code>dbus_dict</code> to copy.
         */
        dbus_dict (const dbus_dict& other)
            : dbus_dict_base(other.sig)
        {
            for (auto& i : other.items)
                items.emplace (i.first, i.second->clone());
        }

        /**
         * Move constructor.
         * @param other The <code>dbus_dict</code> to move.
         */
        dbus_dict (dbus_dict&& other)
            : dbus_dict_base(other.sig),
              items (std::move(other.items))
        {
        }

        virtual ~dbus_dict () = default;

        /**
         * Returns the DBus type code of the key type.
         */
        virtual const int key_type_code () const {
            return key_type_t::make_type_code ();
        }

        /**
         * Returns a string view of the value type signature.
         */
        virtual const std::string_view value_signature () const {
            return std::string_view (sig.data()+3, sig.size()-4);
        }


        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (rhs.is_dict())
                return operator= (dynamic_cast<const dbus_dict<key_type_t>&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_dict");
        }
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (rhs.is_dict())
                return operator= (std::move(dynamic_cast<dbus_dict<key_type_t>&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_dict");
        }


        /**
         * Assignment operator. Copy another <code>dbus_dict</code> object.
         * @param rhs The right-hand side of the assignment.
         * @return This dictionary.
         */
        dbus_dict& operator= (const dbus_dict& rhs) {
            if (this != &rhs) {
                sig = rhs.sig;
                items.clear ();
                for (auto& i : rhs.items)
                    items.emplace (i.first, i.second->clone());
            }
            return *this;
        }

        /**
         * Move operator.
         * @param rhs The right-hand side of the assignment.
         * @return This dictionary.
         */
        dbus_dict& operator= (dbus_dict&& rhs) {
            if (this != &rhs) {
                sig = rhs.sig;
                items = std::move (rhs.items);
            }
            return *this;
        }

        /**
         * Check if another <code>dbus_dict</code> equals this instance.
         * @param rhs The right-hand side of the comparison.
         * @return <code>true</code> if signatures and all key/value pairs are equal.
         */
        bool operator== (const dbus_dict<key_type_t>& rhs) const {
            if (this == &rhs)
                return true;
            if (sig != rhs.sig)
                return false;
            auto lhs_i = items.cbegin ();
            auto rhs_i = rhs.items.cbegin ();
            while (lhs_i!=items.cend()  &&  rhs_i!=rhs.items.cend()) {
                if (!(lhs_i->first == rhs_i->first))
                    return false;
                if (*lhs_i->second != *rhs_i->second)
                    return false;
                ++lhs_i; ++rhs_i;
            }
            return lhs_i==items.cend() && rhs_i==rhs.items.cend();
        }

        virtual bool operator== (const dbus_type& rhs) const {
            if (this == &rhs)
                return true;
            else if (rhs.is_dict())
                return operator== (rhs.cast<dbus_dict<key_type_t>>());
            else if (rhs.is_variant())
                return operator== (rhs.cast<dbus_variant>().get());
            else
                return false;
        }

        /**
         * Perform a lexicographical three-way comparison with another <code>dbus_dict</code>.
         * @param rhs The right-hand side of the comparison.
         * @return The comparison result, or unordered if the signatures differ.
         */
        std::partial_ordering operator<=> (const dbus_dict<key_type_t>& rhs) const {
            if (this == &rhs)
                return std::partial_ordering::equivalent;
            if (sig != rhs.signature())
                return std::partial_ordering::unordered;
            auto lhs_i = items.begin ();
            auto rhs_i = rhs.items.begin ();
            while (lhs_i!=items.end()  &&  rhs_i!=rhs.items.end()) {
                auto res_k = lhs_i->first <=> rhs_i->first;
                if (res_k != std::partial_ordering::equivalent)
                    return res_k;
                auto res_v = *lhs_i->second <=> *rhs_i->second;
                if (res_v != std::partial_ordering::equivalent)
                    return res_v;
                ++lhs_i; ++rhs_i;
            }
            if (lhs_i != items.end())
                return std::partial_ordering::greater;
            else if (rhs_i != rhs.items.end())
                return std::partial_ordering::less;
            else
                return std::partial_ordering::equivalent;
        }

        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const {
            if (this == &rhs)
                return std::partial_ordering::equivalent;
            else if (rhs.is_dict())
                return operator<=> (rhs.cast<dbus_dict<key_type_t>>());
            else if (rhs.is_variant())
                return operator<=> (rhs.cast<dbus_variant>().get());
            else
                return std::partial_ordering::unordered;
        }


        /** Check if the dictionary is empty */
        constexpr bool empty () const {return items.empty();}
        //virtual bool empty () const {return items.empty();}

        /** Return the number of entries in the dictionary. */
        constexpr size_t size () const {return items.size();}
        //virtual size_t size () const {return items.size();}

        /** Clear the contents of the dictionary. */
        constexpr void clear () {items.clear();}
        //virtual void clear () {items.clear();}

        /**
         * Check if the dictionary contains a specific key.
         * @param key The key to search for.
         * @return <code>true</code> if an entry exists for <code>key</code>.
         */
        bool contains (const key_type_t& key) const {
            return items.contains (key);
        }

        /**
         * Access a value for a specific key, creating it with a default
         * value if needed.
         * @param key Key of the value to retrieve or create.
         * @return A reference to the value.
         * @note The returned reference is invalidated if the entry is replaced,
         *       erased, or the dictionary is cleared, reset, or assigned.
         */
        dbus_type& operator[] (const key_type_t& key) {
            auto& ptr = items[key];
            if (ptr.get() == nullptr)
                ptr = create_dbus_type (value_signature());
            return *ptr;
        }

        /**
         * Set a value for a specific key.
         * @param key Key to set.
         * @param value The value for the given key.
         *              The value is copied.
         * @return <code>true</code> if the value signature is compatible; otherwise
         *         <code>false</code> and the dictionary is unchanged.
         */
        bool set (const key_type_t& key, const dbus_type& value) {
            if (value.signature() == value_signature())
                items[key] = value.clone ();
            else if (value_signature()[0] == DBUS_TYPE_VARIANT)
                items[key] = dbus_type_ptr (new dbus_variant(value));
            else
                return false;
            return true;
        }

        /**
         * Set a value for a specific key by moving it.
         * @param key Key to set.
         * @param value The value for the given key.
         *              The value is moved.
         * @return <code>true</code> if the value signature is compatible; otherwise
         *         <code>false</code> and the dictionary is unchanged.
         */
        bool set (const key_type_t& key, dbus_type&& value) {
            if (value.signature() == value_signature())
                items[key] = value.clone_move ();
            else if (value_signature()[0] == DBUS_TYPE_VARIANT)
                items[key] = dbus_type_ptr (new dbus_variant(std::forward<dbus_type>(value)));
            else
                return false;
            return true;
        }

        /**
         * Set a value for a specific key by moving a pointer to the value.
         * @param key Key to set.
         * @param value A unique pointer to the value to set.
         *              The pointer is transferred to the dictionary and
         *              is invalid if this method returns <code>true</code>.
         * @return <code>true</code> if the value signature is compatible; otherwise
         *         <code>false</code> and the dictionary and pointer is unchanged.
         */
        bool set (const key_type_t& key, std::unique_ptr<dbus_type>&& value) {
            if (value == nullptr)
                return false;
            if (value->signature() == value_signature())
                items[key] = std::forward<std::unique_ptr<dbus_type>> (value);
            else if (value_signature()[0] == DBUS_TYPE_VARIANT)
                items[key] = dbus_type_ptr (new dbus_variant(std::forward<std::unique_ptr<dbus_type>>(value)));
            else
                return false;
            return true;
        }

        /**
         * Set a basic value for a specific key.
         * @tparam CppType DBus basic C++ type.
         * @param key Key to set.
         * @param value The value for the given key.
         * @return <code>true</code> if the signature and content of
         *         the value is compatible; otherwise <code>false</code>
         *         and the dictionary is unchanged.
         */
        template<dbus_basic_cpp_types CppType>
        bool set (const key_type_t& key, const CppType& value) {
            try {
                if constexpr (std::same_as<CppType, bool>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_bool(value))));
                    }else{
                        if (sig[3] != dbus_bool::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_bool(value));
                    }
                }else if constexpr (std::same_as<CppType, uint8_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_byte(value))));
                    }else{
                        if (sig[3] != dbus_byte::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_byte(value));
                    }
                }else if constexpr (std::same_as<CppType, int16_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_i16(value))));
                    }else{
                        if (sig[3] != dbus_i16::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_i16(value));
                    }
                }else if constexpr (std::same_as<CppType, uint16_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_u16(value))));
                    }else{
                        if (sig[3] != dbus_u16::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_u16(value));
                    }
                }else if constexpr (std::same_as<CppType, int32_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_i32(value))));
                    }else{
                        if (sig[3] != dbus_i32::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_i32(value));
                    }
                }else if constexpr (std::same_as<CppType, uint32_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_u32(value))));
                    }else{
                        if (sig[3] != dbus_u32::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_u32(value));
                    }
                }else if constexpr (std::same_as<CppType, int64_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_i64(value))));
                    }else{
                        if (sig[3] != dbus_i64::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_i64(value));
                    }
                }else if constexpr (std::same_as<CppType, uint64_t>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_u64(value))));
                    }else{
                        if (sig[3] != dbus_u64::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_u64(value));
                    }
                }else if constexpr (std::same_as<CppType, double>) {
                    if (sig[3] == DBUS_TYPE_VARIANT) {
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_double(value))));
                    }else{
                        if (sig[3] != dbus_double::make_type_code())
                            return false;
                        items[key] = dbus_type_ptr (new dbus_double(value));
                    }
                }else if constexpr (std::same_as<CppType, std::string>) {
                    switch (sig[3]) {
                    case DBUS_TYPE_VARIANT:
                        items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_string(value))));
                        break;
                    case DBUS_TYPE_STRING:
                        items[key] = dbus_type_ptr (new dbus_string(value));
                        break;
                    case DBUS_TYPE_OBJECT_PATH:
                        items[key] = dbus_type_ptr (new dbus_opath(value));
                        break;
                    case DBUS_TYPE_SIGNATURE:
                        items[key] = dbus_type_ptr (new dbus_signature(value));
                        break;
                    default:
                        return false;
                    }
                }else{
                    static_assert (false, "Invalid CppType");
                }
            }catch (...) {
                return false;
            }
            return true;
        }

        /**
         * Set a string value for a specific key.
         * @param key Key to set.
         * @param value The value for the given key.
         * @return <code>true</code> if the signature and content of
         *         the value is compatible; otherwise <code>false</code>
         *         and the dictionary is unchanged.
         */
        bool set (const key_type_t& key, const char* value) {
            if (value == nullptr)
                return false;
            try {
                switch (sig[3]) {
                case DBUS_TYPE_VARIANT:
                    items[key] = dbus_type_ptr (new dbus_variant(dbus_type_ptr(new dbus_string(value))));
                    break;
                case DBUS_TYPE_STRING:
                    items[key] = dbus_type_ptr (new dbus_string(value));
                    break;
                case DBUS_TYPE_OBJECT_PATH:
                    items[key] = dbus_type_ptr (new dbus_opath(value));
                    break;
                case DBUS_TYPE_SIGNATURE:
                    items[key] = dbus_type_ptr (new dbus_signature(value));
                break;
                default:
                    return false;
                }
            }catch (...) {
                return false;
            }
            return true;
        }


        /**
         * Find the entry of a specific key.
         * @param key Key value of the element to search for.
         * @return An iterator to the requested value.
         *         If not found, the end() iterator is returned.
         */
        iterator find (const key_type_t& key) {
            return items.find (key);
        }

        /**
         * Find the entry of a specific key.
         * @param key Key value of the element to search for.
         * @return A const iterator to the requested value.
         *         If not found, the end() iterator is returned.
         */
        const_iterator find (const key_type_t& key) const {
            return items.find (key);
        }


        /**
         * Get a copy of the value for a specific key.
         * @param key The key for the value to get.
         * @param dest Where to store a copy of the value.<br/>
         *             The DBus signature of <code>dest</code>
         *             must match exactly with the value referred
         *             to by <code>key</code>. Or if <code>key</code>
         *             points to a <code>dbus_variant</code>, then the
         *             value contained in the <code>dbus_variant</code>
         *             must have the same signature as <code>dest</code>.
         * @return <code>true</code> if the key was
         *         found and the value is of the wanted type.<br/>
         *         If the key wasn't found, or the data
         *         type of <code>dest</code> doesn't match
         *         the value, <code>false</code> is returned
         *         and <code>dest</code> remains unchanged.
         * @note The value for the given key is
         *       <em>copied</em> to <code>dest</code>. Use
         *       an iterator returned by <code>find()</code>
         *       if a reference to the value is preferred.
         */
        bool get (const key_type_t& key, dbus_type& dest) const {
            const auto entry = items.find (key);
            if (entry == items.end())
                return false;

            const dbus_type& value = *entry->second;
            bool got_value = false;

            if (dest.is_variant()  ||  dest.signature() == value_signature()) {
                dest = value;
                got_value = true;
            }
            else if (value.is_variant()) {
                const dbus_type& real_value = value.cast<dbus_variant>().get ();
                if (dest.signature() == real_value.signature()) {
                    dest = real_value;
                    got_value = true;
                }
            }
            return got_value;
        }

        /**
         * Get a copy of the value for a specific key.
         * This method is used to get a copy of a dbus_basic value
         * for a given key, or a copy of a dbus_basic value contained
         * in a dbus_variant for the given key.
         * @tparam T Expected DBus basic C++ type.
         * @param key The key for the value to get.
         * @param dest Where to store a copy of the value.<br/>
         *             <code>dest</code> is a reference to one of
         *             the C++ types that represents a DBus Basic type.
         *             The type <code>T</code> must match the type
         *             of DBus basic value referred to by
         *             <code>key</code>. Or if <code>key</code>
         *             points to a <code>dbus_variant</code>, then the
         *             value contained in the <code>dbus_variant</code>
         *             must be a <code>dbus_basic</code> of the same
         *             type as <code>T</code>.
         * @return <code>true</code> if the key was
         *         found and the value is of the wanted type.<br/>
         *         If the key wasn't found, or the data
         *         type of <code>dest</code> doesn't match
         *         the value, <code>false</code> is returned
         *         and <code>dest</code> remains unchanged.
         * @note The value for the given key is
         *       <em>copied</em> to <code>dest</code>. Use
         *       an iterator returned by <code>find()</code>
         *       if a reference to the value is preferred.
         */
        template<dbus_basic_cpp_types T>
        bool get (const key_type_t& key, T& dest) {
            const auto entry = items.find (key);
            if (entry == items.end())
                return false;

            const dbus_type& value = *entry->second;
            const dbus_type* real_value {};
            if (value.is_variant())
                real_value = &(value.cast<dbus_variant>().get());
            else
                real_value = &value;

            if constexpr (std::same_as<T, bool>) {
                if (real_value->type_code() == DBUS_TYPE_BOOLEAN) {
                    dest = real_value->cast<dbus_bool>().get ();
                    return true;
                }
            }else if constexpr (std::same_as<T, uint8_t>) {
                if (real_value->type_code() == DBUS_TYPE_BYTE) {
                    dest = real_value->cast<dbus_byte>().get ();
                    return true;
                }
            } else if constexpr (std::same_as<T, int16_t>) {
                if (real_value->type_code() == DBUS_TYPE_INT16) {
                    dest = real_value->cast<dbus_i16>().get ();
                    return true;
                }
                if constexpr (std::same_as<unix_fd_t, int16_t>) {
                    if (real_value->type_code() == DBUS_TYPE_UNIX_FD) {
                        dest = real_value->cast<dbus_unix_fd>().get ();
                        return true;
                    }
                }
            } else if constexpr (std::same_as<T, uint16_t>) {
                if (real_value->type_code() == DBUS_TYPE_UINT16) {
                    dest = real_value->cast<dbus_u16>().get ();
                    return true;
                }
            } else if constexpr (std::same_as<T, int32_t>) {
                if (real_value->type_code() == DBUS_TYPE_INT32) {
                    dest = real_value->cast<dbus_i32>().get ();
                    return true;
                }
                if constexpr (std::same_as<unix_fd_t, int32_t>) {
                    if (real_value->type_code() == DBUS_TYPE_UNIX_FD) {
                        dest = real_value->cast<dbus_unix_fd>().get ();
                        return true;
                    }
                }
            } else if constexpr (std::same_as<T, uint32_t>) {
                if (real_value->type_code() == DBUS_TYPE_UINT32) {
                    dest = real_value->cast<dbus_u32>().get ();
                    return true;
                }
            } else if constexpr (std::same_as<T, int64_t>) {
                if (real_value->type_code() == DBUS_TYPE_INT64) {
                    dest = real_value->cast<dbus_i64>().get ();
                    return true;
                }
                if constexpr (std::same_as<unix_fd_t, int64_t>) {
                    if (real_value->type_code() == DBUS_TYPE_UNIX_FD) {
                        dest = real_value->cast<dbus_unix_fd>().get ();
                        return true;
                    }
                }
            } else if constexpr (std::same_as<T, uint64_t>) {
                if (real_value->type_code() == DBUS_TYPE_UINT64) {
                    dest = real_value->cast<dbus_u64>().get ();
                    return true;
                }
            } else if constexpr (std::same_as<T, double>) {
                if (real_value->type_code() == DBUS_TYPE_DOUBLE) {
                    dest = real_value->cast<dbus_double>().get ();
                    return true;
                }
            }else if constexpr (std::same_as<T, std::string>) {
                switch (real_value->type_code()) {
                case DBUS_TYPE_STRING:
                case DBUS_TYPE_OBJECT_PATH:
                case DBUS_TYPE_SIGNATURE:
                    dest = real_value->cast<dbus_string>().get ();
                    return true;
                default:
                    return false;
                }
            }
            return false;
        }


        /**
         * Clear the content of the dictionary and set a new value signature.
         * @param value_signature A DBus value signature.
         * @return <code>true</code> if the new signature is valid and the
         *         dictionary was reset; otherwise <code>false</code> and the
         *         dictionary remains unchanged.
         */
        bool reset (const char* value_signature) {
            if (value_signature==nullptr || !dbus_signature_validate_single(value_signature, nullptr))
                return false;
            items.clear ();
            sig = DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
            sig.push_back (key_type_t::make_type_code());
            sig.append (value_signature);
            sig.push_back (DBUS_DICT_ENTRY_END_CHAR);
            return true;
        }

        /**
         * Clear the content of the dictionary and set a new value signature.
         * @param value_signature A DBus value signature.
         * @return <code>true</code> if the new signature is valid and the
         *         dictionary was reset; otherwise <code>false</code> and the
         *         dictionary remains unchanged.
         */
        bool reset (const std::string& value_signature) {
            return reset (value_signature.c_str());
        }

        /**
         * Clear the content of the dictionary and set a new value signature.
         * @param variant_or_basic_type_code A basic or variant DBus value type.
         * @return <code>true</code> if the new signature is valid and the
         *         dictionary was reset; otherwise <code>false</code> and the
         *         dictionary remains unchanged.
         */
        bool reset (int variant_or_basic_type_code) {
            if (variant_or_basic_type_code!=DBUS_TYPE_VARIANT  &&  !dbus_type_is_basic(variant_or_basic_type_code))
                return false;
            sig = DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING;
            sig.push_back (key_type_t::make_type_code());
            sig.push_back (variant_or_basic_type_code);
            sig.push_back (DBUS_DICT_ENTRY_END_CHAR);
            return true;
        }


        virtual std::string to_string () const {
            std::string s {"["};
            for (auto i=items.begin(); i != items.end(); ++i) {
                if (i != items.begin())
                    s.push_back (',');
                auto& k = i->first;
                auto& v = *i->second;
                s.push_back ('{');
                if (k.is_string()) {
                    s.push_back ('"');
                    s.append (k.to_string());
                    s.push_back ('"');
                }else{
                    s.append (k.to_string());
                }
                s.push_back (':');
                if (v.is_string()) {
                    s.push_back ('"');
                    s.append (v.to_string());
                    s.push_back ('"');
                }else{
                    s.append (v.to_string());
                }
                s.push_back ('}');
            }
            s.push_back (']');
            return s;
        }

        virtual std::string to_json () const {
            std::string json (R"({"type":"dict","signature":")");
            json.append (signature());
            json.append (R"(","value":[)");
            for (auto i=items.begin(); i != items.end(); ++i) {
                if (i != items.begin())
                    json.push_back (',');
                auto& k = i->first;
                auto& v = *i->second;
                json.append (R"({"key":)");
                json.append (k.to_json());
                json.append (R"(,"value":)");
                json.append (v.to_json());
                json.push_back ('}');
            }
            json.append ("]}");
            return json;
        }

        virtual std::unique_ptr<dbus_type> clone () const {
            return  dbus_type_ptr (new dbus_dict<key_type_t>(*this));
        }

        virtual std::unique_ptr<dbus_type> clone_move () {
            return  dbus_type_ptr (new dbus_dict<key_type_t>(std::move(*this)));
        }

        /** @brief Returns an iterator to the first entry. */
        constexpr iterator begin () {return iterator(items.begin());}
        /** @brief Returns an iterator one past the last entry. */
        constexpr iterator end () {return iterator(items.end());}
        /** @brief Returns a const iterator to the first entry. */
        constexpr const_iterator begin () const {return const_iterator(items.cbegin());}
        /** @brief Returns a const iterator one past the last entry. */
        constexpr const_iterator end () const {return const_iterator(items.cend());}
        /** @brief Returns a const iterator to the first entry. */
        constexpr const_iterator cbegin () const {return const_iterator(items.cbegin());}
        /** @brief Returns a const iterator one past the last entry. */
        constexpr const_iterator cend () const {return const_iterator(items.cend());}

        /** @brief Returns a reverse iterator to the last entry. */
        constexpr reverse_iterator rbegin () {return reverse_iterator(items.rbegin());}
        /** @brief Returns a reverse iterator before the first entry. */
        constexpr reverse_iterator rend () {return reverse_iterator(items.rend());}
        /** @brief Returns a const reverse iterator to the last entry. */
        constexpr const_reverse_iterator rbegin () const {return const_reverse_iterator(items.crbegin());}
        /** @brief Returns a const reverse iterator before the first entry. */
        constexpr const_reverse_iterator rend () const {return const_reverse_iterator(items.crend());}
        /** @brief Returns a const reverse iterator to the last entry. */
        constexpr const_reverse_iterator crbegin () const {return const_reverse_iterator(items.crbegin());}
        /** @brief Returns a const reverse iterator before the first entry. */
        constexpr const_reverse_iterator crend () const {return const_reverse_iterator(items.crend());}


    private:
        std::map<key_type_t, dbus_type_ptr> items;
    };



    /**
     * Create a DBus dictionary with a specific key and value type.
     * @param dbus_type_code Basic DBus type code for the keys in the dictionary.
     * @param value_signature DBus signature of the values in the dictionary.
     * @return A newly allocated dictionary, or <code>nullptr</code> on error.
     */
    std::unique_ptr<dbus_type> create_dbus_dict (int dbus_type_code, const char* value_signature);

    /**
     * Create a DBus dictionary with a specific key and value type.
     * @param dbus_type_code Basic DBus type code for the keys in the dictionary.
     * @param value_signature DBus signature of the values in the dictionary.
     * @return A newly allocated dictionary, or <code>nullptr</code> on error.
     */
    static inline std::unique_ptr<dbus_type> create_dbus_dict (int dbus_type_code, const std::string& value_signature) {
        return create_dbus_dict (dbus_type_code, value_signature.c_str());
    }

    /**
     * Create a DBus dictionary with a specific key and value type.
     * @param dbus_type_code Basic DBus type code for the keys in the dictionary.
     * @param variant_or_basic_type_code A DBus type code of a variant of a basic value.
     * @return A newly allocated dictionary, or <code>nullptr</code> on error.
     */
    static inline std::unique_ptr<dbus_type> create_dbus_dict (int dbus_type_code, int variant_or_basic_type_code) {
        char value_signature[2] {};
        value_signature[0] = (char)variant_or_basic_type_code;
        return create_dbus_dict (dbus_type_code, (const char*)value_signature);
    }


}
#endif
