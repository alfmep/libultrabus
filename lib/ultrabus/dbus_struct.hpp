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
#ifndef ULTRABUS_DBUS_STRUCT_HPP
#define ULTRABUS_DBUS_STRUCT_HPP

#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/utils.hpp>
#include <dbus/dbus.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <cstdint>
#include <cctype>


namespace ultrabus {

    /**
     * A class representing a DBus Struct type.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#container-types rel="noopener noreferrer" target="_blank">DBus Container Types at dbus.freedesktop.org</a>
     */
    class dbus_struct : public dbus_type {
    private:
        using dbus_type_ptr = std::unique_ptr<dbus_type>;

    public:
        /**
         * Random-access iterator providing read-only access to struct attributes.
         */
        class const_iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = const dbus_type;
            using difference_type = ptrdiff_t;
            using pointer = const dbus_type*;
            using reference = const dbus_type&;
            constexpr const_iterator () = default;
            constexpr const_iterator& operator= (const const_iterator& rhs) {
                if (this != &rhs)
                    iter = rhs.iter;
                return *this;
            }
            constexpr const_iterator& operator++ () {++iter; return *this;}
            constexpr const_iterator operator++ (int) {const_iterator i(iter); ++iter;return i;}
            constexpr const_iterator operator+ (difference_type n) {return const_iterator(iter + n);}
            friend const_iterator operator+ (difference_type n, const_iterator i);
            constexpr const_iterator& operator+= (int n) {iter += n; return *this;}
            constexpr const_iterator& operator-- () {--iter; return *this;}
            constexpr const_iterator operator-- (int) {const_iterator i(iter); --iter;return i;}
            constexpr const_iterator operator- (difference_type n) {return const_iterator(iter - n);}
            constexpr difference_type operator- (const_iterator i) {return iter - i.iter;}
            constexpr const_iterator& operator-= (int n) {iter -= n; return *this;}
            inline const dbus_type& operator*() {return *(*iter);}
            inline dbus_type* const operator->() {return iter->get();}
            inline const dbus_type& operator[] (size_t pos) {return *iter[pos];}
            constexpr bool operator== (const const_iterator& rhs) const {return iter==rhs.iter;}
            constexpr auto operator<=> (const const_iterator& rhs) const {return iter <=> rhs.iter;}
        private:
            friend class dbus_struct;
            constexpr const_iterator (const std::vector<dbus_type_ptr>::const_iterator& i) : iter(i){}
            std::vector<dbus_type_ptr>::const_iterator iter;
        };

        /**
         * Random-access iterator providing mutable access to array elements.
         */
        class iterator {
        public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = dbus_type;
            using difference_type = ptrdiff_t;
            using pointer = dbus_type*;
            using reference = dbus_type&;
            constexpr iterator () = default;
            constexpr operator const_iterator() const {return const_iterator(iter);}
            constexpr iterator& operator= (const iterator& rhs) {
                if (this != &rhs)
                    iter = rhs.iter;
                return *this;
            }
            constexpr iterator& operator++ () {++iter; return *this;}
            constexpr iterator operator++ (int) {iterator i(iter); ++iter;return i;}
            constexpr iterator operator+ (int n) {return iterator(iter + n);}
            friend iterator operator+ (difference_type n, iterator i);
            constexpr iterator& operator+= (int n) {iter += n; return *this;}
            constexpr iterator& operator-- () {--iter; return *this;}
            constexpr iterator operator-- (int) {iterator i(iter); --iter;return i;}
            constexpr iterator operator- (int n) {return iterator(iter - n);}
            constexpr difference_type operator- (iterator i) {return iter - i.iter;}
            constexpr iterator& operator-= (int n) {iter -= n; return *this;}
            inline dbus_type& operator*() {return *(*iter);}
            inline dbus_type* operator->() {return iter->get();}
            inline dbus_type& operator[] (size_t pos) {return *iter[pos];}
            constexpr bool operator== (const iterator& rhs) const {return iter==rhs.iter;}
            constexpr auto operator<=> (const iterator& rhs) const {return iter<=>rhs.iter;}
        private:
            friend class dbus_struct;
            constexpr iterator (const std::vector<dbus_type_ptr>::iterator& i) : iter(i){}
            std::vector<dbus_type_ptr>::iterator iter;
        };
        using const_reverse_iterator = std::reverse_iterator<const_iterator>; /**< A const reverse iterator. */
        using reverse_iterator = std::reverse_iterator<iterator>; /**< A reverse iterator. */


        /**
         * Default constructor. Constructs an empty DBus struct with no attributes.
         * @note An empty struct without attributes is not a valid DBus struct.
         *       At least one attribute must be added for it to be valid.
         */
        constexpr dbus_struct () : dbus_type(DBUS_STRUCT_BEGIN_CHAR_AS_STRING DBUS_STRUCT_END_CHAR_AS_STRING) {
        }

        /**
         * Construct a DBus struct with attributes defined in a DBus signature.
         * @param attribute_signatures The signatures of the attributes in the struct.
         * @throws std::invalid_argument If <code>attribute_signatures</code> is not a
         *         valid sequence of DBus type signatures.
         */
        dbus_struct (const std::string& attribute_signatures);

        /**
         * Copy constructor. Make a deep copy of another DBsu struct.
         * @param other The DBus struct to copy.
         */
        dbus_struct (const dbus_struct& other) : dbus_type(other.sig) {
            for (auto& item : other.items)
                items.emplace_back (item->clone());
        }
        /**
         * Move constructor.
         * @param other The DBus struct to move.
         */
        dbus_struct (dbus_struct&& other) : dbus_type(other.sig) {
            items = std::move (other.items);
            other.sig = DBUS_STRUCT_BEGIN_CHAR_AS_STRING DBUS_STRUCT_END_CHAR_AS_STRING;
        }

        virtual ~dbus_struct () = default;

        virtual int type_code () const {return DBUS_TYPE_STRUCT;}

        /** @brief Returns <code>true</code> because this is a DBus struct. */
        virtual bool is_struct () const {return true;}

        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (rhs.is_struct())
                return operator= (dynamic_cast<const dbus_struct&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_struct");
        }
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (rhs.is_struct())
                return operator= (std::move(dynamic_cast<dbus_struct&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_struct");
        }

        /**
         * Assignment operator. Copy another DBus struct.
         * @param rhs The DBus Struct to copy.
         * @return This struct.
         * @note After the assignment, this struct may have
         *       a attribute different signature.
         */
        dbus_struct& operator= (const dbus_struct& rhs) {
            if (this != &rhs) {
                sig = rhs.sig;
                items.clear ();
                for (auto& item : rhs.items)
                    items.emplace_back (item->clone());
            }
            return *this;
        }
        /**
         * Move operator. Moves another struct into this one.
         * @param rhs The DBus Struct to move.
         * @return This struct.
         * @note After the assignment, this struct may have
         *       a attribute different signature.
         */
        dbus_struct& operator= (dbus_struct&& rhs) {
            if (this != &rhs) {
                sig = rhs.sig;
                items = std::move (rhs.items);
                rhs.sig = DBUS_STRUCT_BEGIN_CHAR_AS_STRING DBUS_STRUCT_END_CHAR_AS_STRING;
            }
            return *this;
        }

        /**
         * Check if this DBus struct equals another DBus struct.
         * @param rhs Right-hand side of comparison.
         * @return <code>true</code> if both signatures and attributes are equal.
         */
        bool operator== (const dbus_struct& rhs) const;

        virtual bool operator== (const dbus_type& rhs) const {
            if (this == &rhs)
                return true;
            else if (rhs.is_struct())
                return operator== (rhs.cast<dbus_struct>());
            else if (rhs.is_variant())
                return operator== (rhs.cast<dbus_variant>().get());
            else
                return false;
        }

        /**
         * Perform a three-way comparison with another DBus struct.
         * @param rhs Right-hand side of comparison.
         * @return The lexicographical attribute comparison, or unordered when
         *         the struct signatures differ.
         */
        std::partial_ordering operator<=> (const dbus_struct& rhs) const;

        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const {
            if (this == &rhs)
                return std::partial_ordering::equivalent;
            else if (rhs.is_struct())
                return operator<=> (dynamic_cast<const dbus_struct&>(rhs));
            else if (rhs.is_variant())
                return operator<=> (rhs.cast<dbus_variant>().get());
            else
                return std::partial_ordering::unordered;
        }

        /**
         * Checks whether this is a valid DBus struct.
         * A DBus struct must contain at least one attribute to be a valid struct.
         * @return <code>true</code> if at least one attribute is present.
         */
        bool is_valid () const {return !items.empty();}

        /**
         * Get the number of attributes in the struct.
         * @return The number of attributes in the struct.
         */
        size_t size () const {return items.size();}

        /**
         * Access an attribute in the struct.
         * @param pos The position of the attribute to acccess.
         * @return A reference owned by this struct to the attribute at @p pos.
         * @throws std::out_of_range If pos >= size().
         */
        dbus_type& get (size_t pos) {return *items.at(pos);}

        /**
         * Access an attribute in the struct.
         * @param pos The position of the attribute to acccess.
         * @tparam T Expected DBus attribute type.
         * @return A reference owned by this struct to the attribute at @p pos.
         * @throws std::bad_cast If the attribute at pos is not of type T.
         * @throws std::out_of_range If pos >= size().
         */
        template<any_dbus_type T>
        T& get (size_t pos) {return dynamic_cast<T&>(*items.at(pos));}

        /**
         * Access an attribute in the struct.
         * @param pos The position of the attribute to acccess.
         * @return A const reference owned by this struct to the attribute at @p pos.
         * @throws std::out_of_range If pos >= size().
         */
        const dbus_type& get (size_t pos) const {return *items.at(pos);}

        /**
         * Access an attribute in the struct.
         * @param pos The position of the attribute to acccess.
         * @tparam T Expected DBus attribute type.
         * @return A const reference owned by this struct to the attribute at @p pos.
         * @throws std::bad_cast If the attribute at pos is not of type T.
         * @throws std::out_of_range If pos >= size().
         */
        template<any_dbus_type T>
        const T& get (size_t pos) const {return dynamic_cast<const T&>(*items.at(pos));}

    private:
        inline void add_attrib_signature (const dbus_type& new_attrib) {
            sig.pop_back ();
            sig.append (new_attrib.signature());
            sig.push_back (DBUS_STRUCT_END_CHAR);
        }

    public:
        /**
         * @brief Clones and appends an attribute.
         * @param item Attribute to clone.
         */
        void add (const dbus_type& item) {
            add_attrib_signature (item);
            items.emplace_back (item.clone());
        }

        /**
         * @brief Moves and appends an attribute.
         * @param item Attribute to move.
         */
        void add (dbus_type&& item) {
            add_attrib_signature (item);
            items.emplace_back (item.clone_move());
            update_signature ();
        }

        /**
         * @brief Appends a DBus basic C++ value.
         * @tparam T DBus basic C++ type.
         * @param item Value to copy.
         */
        template<dbus_basic_cpp_types T>
        void add (const T& item) {
            add (std::unique_ptr<dbus_type>(new dbus_basic<T>(item)));
        }

        /**
         * @brief Appends a moved DBus basic C++ value.
         * @tparam T DBus basic C++ type.
         * @param item Value to move.
         */
        template<dbus_basic_cpp_types T>
        void add (T&& item) {
            add (std::unique_ptr<dbus_type>(new dbus_basic<T>(std::forward<T>(item))));
        }

        /**
         * @brief Appends a C string as a DBus string.
         * @param item Null-terminated string; a null pointer is ignored.
         */
        void add (const char* item) {
            if (item)
                add (std::unique_ptr<dbus_type>(new dbus_string(item)));
        }

        /**
         * @brief Transfers an attribute into the struct.
         * @param item Owned attribute to transfer; a null pointer is ignored.
         * @post On success, this struct owns the object formerly owned by @p item.
         */
        void add (std::unique_ptr<dbus_type>&& item) {
            if (item) {
                add_attrib_signature (*item);
                items.emplace_back (std::forward<dbus_type_ptr>(item));
            }
        }

        /**
         * Remove an attribute from the struct.
         * @param pos The position of the item.
         * @note An out-of-range position is ignored.
         */
        void remove (size_t pos) {
            if (pos >= items.size())
                return;
            items.erase (items.begin() + pos);
            update_signature ();
        }

        /**
         * Rebuilds the DBus signature from the current attributes.
         * This method recreates the signature of the DBus struct.
         * This is normally not needed since the signature is automatically
         * updated when attributes are added or removed.<br/>
         * But one (or more) attribute in a struct may be another struct.
         * And if the child struct is modified by adding or removing an
         * attribute (after at call to get() from the parent struct), the
         * parent struct has no way of knowing that. So then a call to
         * update_signature() in the parent struct is needed.
         */
        void update_signature () {
            sig = DBUS_STRUCT_BEGIN_CHAR_AS_STRING;
            for (auto& i : items)
                sig.append (i->signature());
            sig.push_back (DBUS_STRUCT_END_CHAR);
        }

        /** Return an iterator to the first attribute in the struct. */
        iterator begin () {return iterator(items.begin());}
        /** Return an iterator past the last attribute in the struct. */
        iterator end () {return iterator(items.end());}
        /** Return a const iterator to the first attribute in the struct. */
        const_iterator begin () const {return const_iterator(items.cbegin());}
        /** Return a const iterator past the last attribute in the struct. */
        const_iterator end () const {return const_iterator(items.cend());}
        /** Return a const iterator to the first attribute in the struct. */
        const_iterator cbegin () const {return const_iterator(items.cbegin());}
        /** Return a const iterator past the last attribute in the struct. */
        const_iterator cend () const {return const_iterator(items.cend());}

        /** Return a reverse iterator to the first attribute in the struct. */
        reverse_iterator rbegin () {return std::make_reverse_iterator(end());}
        /** Return a reverse iterator past the last attribute in the struct. */
        reverse_iterator rend () {return std::make_reverse_iterator(begin());}
        /** Return a const reverse iterator to the first attribute in the struct. */
        const_reverse_iterator rbegin () const {return std::make_reverse_iterator(cend());}
        /** Return a const reverse iterator past the last attribute in the struct. */
        const_reverse_iterator rend () const {return std::make_reverse_iterator(cbegin());}
        /** Return a const reverse iterator to the first attribute in the struct. */
        const_reverse_iterator crbegin () const {return std::make_reverse_iterator(cend());}
        /** Return a const reverse iterator past the last attribute in the struct. */
        const_reverse_iterator crend () const {return std::make_reverse_iterator(cbegin());}

        virtual std::string to_string () const;
        virtual std::string to_json () const;

        virtual std::unique_ptr<dbus_type> clone () const {
            return dbus_type_ptr (new dbus_struct(*this));
        }
        virtual std::unique_ptr<dbus_type> clone_move () {
            return dbus_type_ptr (new dbus_struct(std::move(*this)));
        }

    private:
        std::vector<dbus_type_ptr> items;
    };


    inline dbus_struct::const_iterator operator+ (dbus_struct::const_iterator::difference_type n,
                                                 dbus_struct::const_iterator i)
    {
        return dbus_struct::const_iterator (n + i.iter);
    }


    inline dbus_struct::iterator operator+ (dbus_struct::iterator::difference_type n,
                                           dbus_struct::iterator i)
    {
        return dbus_struct::iterator (n + i.iter);
    }


}
#endif
