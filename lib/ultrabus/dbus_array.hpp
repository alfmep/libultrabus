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
#ifndef ULTRABUS_DBUS_ARRAY_HPP
#define ULTRABUS_DBUS_ARRAY_HPP

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
     * A class representing a DBus Array type.
     * @see <a href=https://dbus.freedesktop.org/doc/dbus-specification.html#container-types rel="noopener noreferrer" target="_blank">DBus Container Types at dbus.freedesktop.org</a>
     */
    class dbus_array : public dbus_type {
    private:
        using dbus_type_ptr = std::unique_ptr<dbus_type>;

    public:
        /**
         * Random-access iterator providing read-only access to array elements.
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
            friend class dbus_array;
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
            friend class dbus_array;
            constexpr iterator (const std::vector<dbus_type_ptr>::iterator& i) : iter(i){}
            std::vector<dbus_type_ptr>::iterator iter;
        };
        using const_reverse_iterator = std::reverse_iterator<const_iterator>; /**< A const reverse iterator. */
        using reverse_iterator = std::reverse_iterator<iterator>; /**< A reverse iterator. */


        dbus_array () = delete;

        /**
         * Create an array of DBus types.
         * @param element_signature The signature of the individual elements in the array.
         * @throw std::invalid_argument If the signature is invalid.
         */
        dbus_array (const char* element_signature) : dbus_type(DBUS_TYPE_ARRAY_AS_STRING) {
            sig.append (element_signature);
            if (!dbus_signature_validate_single(sig.c_str(), nullptr))
                throw std::invalid_argument ("Invalid DBus Array signature");
        }
        /**
         * Create an array of DBus types.
         * @param element_signature The signature of the individual elements in the array.
         * @throw std::invalid_argument If the signature is an invalid signature.
         */
        dbus_array (const std::string& element_signature) : dbus_type(DBUS_TYPE_ARRAY_AS_STRING) {
            sig.append (element_signature);
            if (!dbus_signature_validate_single(sig.c_str(), nullptr))
                throw std::invalid_argument ("Invalid DBus Array signature");
        }
        /**
         * Create an array with elements of a specific DBus Basic type.
         * @param basic_type_code The type code of the individual elements in the array.
         * @throw std::invalid_argument If the type code is not a DBus Basic type.
         */
        dbus_array (int basic_type_code) : dbus_type(DBUS_TYPE_ARRAY_AS_STRING) {
            if (!dbus_type_is_basic(basic_type_code))
                throw std::invalid_argument ("Invalid DBus Array signature");
            sig.push_back ((char)basic_type_code);
        }


        /**
         * Construct an array from DBus basic C++ values.
         * @tparam T C++ representation of the array's DBus basic element type.
         * @param ilist Values to copy into the new array.
         */
        template<dbus_basic_cpp_types T>
        dbus_array (std::initializer_list<T> ilist)
            : dbus_type(DBUS_TYPE_ARRAY_AS_STRING)
        {
            sig.push_back ((char)dbus_basic<T>::make_type_code());
            for (auto i : ilist)
                items.emplace_back (dbus_type_ptr(new dbus_basic<T>(i)));
        }
        /**
         * Construct a DBus string array from null-terminated strings.
         * @param ilist Strings to copy; their pointers are not retained.
         */
        dbus_array (std::initializer_list<const char*> ilist)
            : dbus_type(DBUS_TYPE_ARRAY_AS_STRING)
        {
            sig.push_back ((char)dbus_string::make_type_code());
            for (auto i : ilist) {
                if (i != nullptr)
                    items.emplace_back (dbus_type_ptr(new dbus_string(i)));
            }
        }
        /**
         * Construct a DBus string array from C++ strings.
         * @param ilist Strings to copy into the new array.
         */
        dbus_array (std::initializer_list<std::string> ilist)
            : dbus_type(DBUS_TYPE_ARRAY_AS_STRING)
        {
            sig.push_back ((char)dbus_string::make_type_code());
            for (auto i : ilist)
                items.emplace_back (dbus_type_ptr(new dbus_string(std::forward<std::string>(i))));
        }

        /**
         * Construct a dbus_array from an initializer list of strings.
         * object paths, or signature values.
         * @param type_of_string The type of string.
         *                       One of:
         *                        - <code>DBUS_TYPE_STRING</code>
         *                        - <code>DBUS_TYPE_OBJECT_PATH</code>
         *                        - <code>DBUS_TYPE_SIGNATURE</code>
         * @param ilist The list of items to add to the array.
         * @throws std::invalid_argument If the string type is invalid.
         *                               Or if setting an invalid object path
         *                               or signature value.
         */
        dbus_array (int type_of_string, std::initializer_list<const char*> ilist)
            : dbus_type(DBUS_TYPE_ARRAY_AS_STRING)
        {
            sig.push_back ((char)type_of_string);
            switch (type_of_string) {
            case DBUS_TYPE_STRING:
                for (auto i : ilist) {
                    if (i != nullptr)
                        items.emplace_back (dbus_type_ptr(new dbus_string(i)));
                }
                break;
            case DBUS_TYPE_OBJECT_PATH:
                for (auto i : ilist) {
                    if (i != nullptr)
                        items.emplace_back (dbus_type_ptr(new dbus_opath(i)));
                }
                break;
            case DBUS_TYPE_SIGNATURE:
                for (auto i : ilist) {
                    if (i != nullptr)
                        items.emplace_back (dbus_type_ptr(new dbus_signature(i)));
                }
                break;
            default:
                throw std::invalid_argument ("Not a valid DBus string type code");
            }
        }

        /**
         * Construct a dbus_array from an initializer list of strings.
         * object paths, or signature values.
         * @param type_of_string The type of string.
         *                       One of:
         *                        - <code>DBUS_TYPE_STRING</code>
         *                        - <code>DBUS_TYPE_OBJECT_PATH</code>
         *                        - <code>DBUS_TYPE_SIGNATURE</code>
         * @param ilist The list of items to add to the array.
         * @throws std::invalid_argument If the string type is invalid.
         *                               Or if setting an invalid object path
         *                               or signature value.
         */
        dbus_array (int type_of_string, std::initializer_list<const std::string> ilist)
            : dbus_type(DBUS_TYPE_ARRAY_AS_STRING)
        {
            switch (type_of_string) {
            case DBUS_TYPE_STRING:
                for (auto i : ilist)
                    items.emplace_back (dbus_type_ptr(new dbus_string(std::forward<std::string>(i))));
                break;
            case DBUS_TYPE_OBJECT_PATH:
                for (auto i : ilist)
                    items.emplace_back (dbus_type_ptr(new dbus_opath(std::forward<std::string>(i))));
                break;
            case DBUS_TYPE_SIGNATURE:
                for (auto i : ilist)
                    items.emplace_back (dbus_type_ptr(new dbus_signature(std::forward<std::string>(i))));
                break;
            default:
                throw std::invalid_argument ("Not a valid DBus string type code");
            }
            sig.push_back ((char)type_of_string);
        }


        /**
         * Copy contructor.
         * Make a deep copy of another DBus Array.
         * @param other The DBus array to copy.
         */
        dbus_array (const dbus_array& other) : dbus_type(other.sig) {
            for (auto& item : other.items)
                items.emplace_back (item->clone());
        }
        /**
         * Move constructor.
         * @param other The DBus array to move. It remains valid with an
         *        unspecified moved-from element sequence.
         */
        dbus_array (dbus_array&& other) : dbus_type(other.sig) {
            items = std::move (other.items);
        }

        /**
         * Destructor.
         */
        virtual ~dbus_array () = default;

        /** Return true since this is a DBus Array instance .*/
        virtual bool is_array () const {return true;}


        virtual dbus_type& operator= (const dbus_type& rhs) {
            if (rhs.is_array())
                return operator= (dynamic_cast<const dbus_array&>(rhs));
            else
                throw std::invalid_argument ("Can't assign a dbus_type with different signature to a dbus_array");
        }
        virtual dbus_type& operator= (dbus_type&& rhs) {
            if (rhs.is_array())
                return operator= (std::move(dynamic_cast<dbus_array&>(rhs)));
            else
                throw std::invalid_argument ("Can't move a dbus_type with different signature to a dbus_array");
        }

        /**
         * Assignment operator. Make a deep copy of another dbus_array.
         * @param rhs The DBus Array to copy.
         * @return A reference to this array.
         * @note This may change the type of values this array holds.
         */
        dbus_array& operator= (const dbus_array& rhs) {
            if (this != &rhs) {
                sig = rhs.sig;
                items.clear ();
                for (auto& item : rhs.items)
                    items.emplace_back (item->clone());
            }
            return *this;
        }
        /**
         * Move operator.
         * @param rhs The DBus Array to move.
         * @return A reference to this array. <code>rhs</code> remains valid
         *         with an unspecified moved-from element sequence.
         * @note This may change the type of values this array holds.
         */
        dbus_array& operator= (dbus_array&& rhs) {
            if (this != &rhs) {
                sig = rhs.sig;
                items = std::move (rhs.items);
            }
            return *this;
        }


        /**
         * Assignment operator.
         * @tparam T C++ representation of the DBus basic element type.
         * @param rhs An initializer list with DBus basic values.
         * @return A reference to this array.
         * @note This may change the type of values this array holds.
         */
        template<dbus_basic_cpp_types T>
        dbus_array& operator= (std::initializer_list<T> rhs) {
            items.clear ();
            if (rhs.begin() == rhs.end())
                return *this;

            sig = DBUS_TYPE_ARRAY_AS_STRING;
            sig.push_back ((char)dbus_basic<T>::make_type_code());

            for (auto i : rhs)
                items.emplace_back (dbus_type_ptr(new dbus_basic<T>(i)));

            return *this;
        }

        /**
         * Assignment operator.
         * Assign a list of strings to this array. If this array already
         * holds values of dbus_string, dbus_opath, or dbus_signature, then
         * the values assigned are treated as such. But if this array holds
         * values of another type, it will be converted to hold values
         * of type dbus_string and assigned the values in the initializer list.
         * @param rhs An initializer list with string values to copy.
         * @return A reference to this array.
         * @throws std::invalid_argument If the array holds values of type
         *         dbus_opath or dbus_signature, and the values in the
         *         initializer holds any invalid object path or DBus signature.
         * @note This may change the type of values this array holds.
         */
        dbus_array& operator= (std::initializer_list<const char*> rhs) {
            items.clear ();
            if (rhs.begin() == rhs.end())
                return *this;
            switch (sig[1]) {
            case DBUS_TYPE_OBJECT_PATH:
                for (auto i : rhs)
                    items.emplace_back (dbus_type_ptr(new dbus_opath(i)));
                break;
            case DBUS_TYPE_SIGNATURE:
                for (auto i : rhs)
                    items.emplace_back (dbus_type_ptr(new dbus_signature(i)));
                break;
            case DBUS_TYPE_STRING:
            default:
                if (sig[1] != DBUS_TYPE_STRING)
                    sig = "as";
                for (auto i : rhs)
                    items.emplace_back (dbus_type_ptr(new dbus_string(i)));
                break;
            }
            return *this;
        }

        /**
         * Assignment operator.
         * Assign a list of strings to this array. If this array already
         * holds values of dbus_string, dbus_opath, or dbus_signature, then
         * the values assigned are treated as such. But if this array holds
         * values of another type, it will be converted to hold values
         * of type dbus_string and assigned the values in the initializer list.
         * @param rhs An initializer list with string values to copy.
         * @return A reference to this array.
         * @throws std::invalid_argument If the array holds values of type
         *         dbus_opath or dbus_signature, and the values in the
         *         initializer holds any invalid object path or DBus signature.
         * @note This may change the type of values this array holds.
         */
        dbus_array& operator= (std::initializer_list<const std::string> rhs) {
            items.clear ();
            if (rhs.begin() == rhs.end())
                return *this;
            switch (sig[1]) {
            case DBUS_TYPE_OBJECT_PATH:
                for (auto i : rhs)
                    items.emplace_back (dbus_type_ptr(new dbus_opath(std::forward<std::string>(i))));
                break;
            case DBUS_TYPE_SIGNATURE:
                for (auto i : rhs)
                    items.emplace_back (dbus_type_ptr(new dbus_signature(std::forward<std::string>(i))));
                break;
            case DBUS_TYPE_STRING:
            default:
                if (sig[1] != DBUS_TYPE_STRING)
                    sig = "as";
                for (auto i : rhs)
                    items.emplace_back (dbus_type_ptr(new dbus_string(std::forward<std::string>(i))));
                break;
            }
            return *this;
        }


        /**
         * Access specified element with bounds checking.
         * @param pos The position of the item to access.
         * @return A reference to the item at position <code>pos</code>.
         * @throws std::out_of_range If <code>pos >= size()</code>.
         */
        inline dbus_type& at (size_t pos) {return *items.at(pos);}
        /**
         * Access specified element with bounds checking, and perform
         * a dynamic cast to a specific subclass of dbus_type.
         * @tparam T A subclass of <code>dbus_type</code>.
         * @param pos The position of the item to access.
         * @return A reference to the DBus value at <code>pos</code>.
         * @throws std::out_of_range If <code>pos >= size()</code>.
         * @throws std::bad_cast If the object at position is not of type T.
         */
        template<any_dbus_type T>
        inline T& at (size_t pos) {return dynamic_cast<T&>(*items.at(pos));}
        /**
         * Access specified element with bounds checking.
         * @param pos The position of the item to access.
         * @return A const reference to the item at position <code>pos</code>.
         * @throws std::out_of_range If <code>pos >= size()</code>.
         */
        inline const dbus_type& at (size_t pos) const {return *items.at(pos);}
        /**
         * Access specified element with bounds checking, and perform
         * a dynamic cast to a specific subclass of dbus_type.
         * @tparam T A subclass of <code>dbus_type</code>.
         * @param pos The position of the item to access.
         * @return A const reference to the DBus value at <code>pos</code>.
         * @throws std::out_of_range If <code>pos >= size()</code>.
         * @throws std::bad_cast If the object at position is not of type T.
         */
        template<any_dbus_type T>
        inline const T& at (size_t pos) const {return dynamic_cast<const T&>(*items.at(pos));}

        /**
         * Access a specified element without bounds checking.
         * @param pos The position of the item to access.
         * @return A reference to the DBus value at <code>pos</code>.
         * @warning Passing <code>pos >= size()</code> has undefined behavior.
         */
        inline dbus_type& operator[] (size_t pos) {return *items[pos];}
        /**
         * Access a specified element without bounds checking.
         * @param pos The position of the item to access.
         * @return A const reference to the DBus value at <code>pos</code>.
         * @warning Passing <code>pos >= size()</code> has undefined behavior.
         */
        inline const dbus_type& operator[] (size_t pos) const {return *items[pos];}

        /**
         * Access the first element.
         * @return A reference to the first item in the array.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        inline dbus_type& front () {return *items.front();}
        /**
         * Access the first element.
         * @return A const reference to the first item in the array.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        inline const dbus_type& front () const {return *items.front();}
        /**
         * Access the first element and perform a dynamic
         * cast to a specific subclass of dbus_type.
         * @tparam T A subclass of <code>dbus_type</code>.
         * @return A reference to the first item in the array.
         * @throws std::bad_cast If the object at position is not of type T.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        template<any_dbus_type T>
        inline T& front () {return dynamic_cast<T&>(*items.front());}
        /**
         * Access the first element and perform a dynamic
         * cast to a specific subclass of dbus_type.
         * @tparam T A subclass of <code>dbus_type</code>.
         * @return A const reference to the first item in the array.
         * @throws std::bad_cast If the object at position is not of type T.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        template<any_dbus_type T>
        inline const T& front () const {return dynamic_cast<const T&>(*items.front());}

        /**
         * Access the last element.
         * @return A reference to the last item in the array.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        inline dbus_type& back () {return *items.back();}
        /**
         * Access the last element.
         * @return A const reference to the last item in the array.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        inline const dbus_type& back () const {return *items.back();}

        /**
         * Access the last element and perform a dynamic
         * cast to a specific subclass of dbus_type.
         * @tparam T Expected concrete DBus element type.
         * @return A reference to the last item in the array.
         * @throws std::bad_cast If the object at position is not of type T.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        template<any_dbus_type T>
        inline T& back () {return dynamic_cast<T&>(*items.back());}
        /**
         * Access the last element and perform a dynamic
         * cast to a specific subclass of dbus_type.
         * @tparam T Expected concrete DBus element type.
         * @return A const reference to the last item in the array.
         * @throws std::bad_cast If the object at position is not of type T.
         * @warning Calling this method on an empty array has undefined behavior.
         */
        template<any_dbus_type T>
        inline const T& back () const {return dynamic_cast<const T&>(*items.back());}

        /** Check if the array is empty. */
        constexpr bool empty () const {return items.empty();}

        /** Return the number of elements in the array. */
        constexpr size_t size () const {return items.size();}

        /** Clear the contents of the array. */
        constexpr void clear () {items.clear();}

        /**
         * Return the DBus signature of the type of elements in the array.
         * @return A string containing the DBus signature of the type of elements.
         */
        constexpr const std::string element_signature () const {
            return std::string (sig.c_str()+1, sig.size()-1);
        }

        /**
         * Compare this array with another dbus_array instance.
         * @param rhs Array to compare.
         * @return <code>true</code> when signatures and all element values are equal.
         */
        bool operator== (const dbus_array& rhs) const;

        /**
         * Compare this array with a polymorphic DBus value.
         * @param rhs Value to compare.
         * @return <code>true</code> only for an equal DBus array.
         */
        virtual bool operator== (const dbus_type& rhs) const {
            if (this == &rhs)
                return true;
            else if (rhs.is_array())
                return operator== (rhs.cast<dbus_array>());
            else if (rhs.is_variant())
                return operator== (rhs.cast<dbus_variant>().get());
            else
                return false;
        }

        /**
         * Make a three-way comparison with another dbus_array instance.
         * @param rhs Array to compare.
         */
        std::partial_ordering operator<=> (const dbus_array& rhs) const;

        /**
         * Make a three-way comparison with a polymorphic DBus value.
         * @param rhs Value to compare.
         */
        virtual std::partial_ordering operator<=> (const dbus_type& rhs) const {
            if (this == &rhs)
                return std::partial_ordering::equivalent;
            else if (rhs.is_array())
                return operator<=> (rhs.cast<dbus_array>());
            else if (rhs.is_variant())
                return operator<=> (rhs.cast<dbus_variant>().get());
            else
                return std::partial_ordering::unordered;
        }


    private:
        constexpr void verify_element_signature (const char* esig) {
            if (element_signature() != esig) {
                std::string msg {"Adding dbus_array element with wrong signature '"};
                msg.append (esig);
                msg.append ("', expected '");
                msg.append (element_signature());
                msg.push_back ('\'');
                throw std::invalid_argument (msg);
            }
        }
        constexpr void verify_element_signature (int type_code) {
            if (sig[1] != (char)type_code) {
                std::string msg {"Adding dbus_array element with wrong signature '"};
                msg.push_back ((char)type_code);
                msg.append ("', expected '");
                msg.append (element_signature());
                msg.push_back ('\'');
                throw std::invalid_argument (msg);
            }
        }
        constexpr void verify_element_type_is_type_of_string () {
            if (sig[1] != (char)DBUS_TYPE_STRING  &&
                sig[1] != (char)DBUS_TYPE_OBJECT_PATH &&
                sig[1] != (char)DBUS_TYPE_SIGNATURE)
            {
                std::string msg {"Adding string type to dbus_array with signature '"};
                msg.append (element_signature());
                msg.push_back ('\'');
                throw std::invalid_argument (msg);
            }
        }
    public:
        /**
         * Append a copy of a DBus value.
         * @param item Value to copy into this array.
         * @throws std::invalid_argument If the value signature
         *         differs from the array element signature.
         */
        void push_back (const dbus_type& item) {
            verify_element_signature (item.signature().c_str());
            items.emplace_back (item.clone());
        }
        /**
         * Append a DBus value by moving it into a new element.
         * @param item Value to move from.
         * @throws std::invalid_argument If the value signature
         *         differs from the array element signature.
         */
        void push_back (dbus_type&& item) {
            verify_element_signature (item.signature().c_str());
            items.emplace_back (item.clone_move());
        }

        /**
         * Append a DBus value from a pointer to a <code>dbus_type</code>.
         * @param item Unique ownership to transfer. A null pointer is ignored.
         * @throws std::invalid_argument If a non-null value has a mismatched
         *         signature. On success, ownership transfers to this array.
         */
        void push_back (std::unique_ptr<dbus_type>&& item) {
            if (item) {
                verify_element_signature (item->signature().c_str());
                items.emplace_back (std::forward<dbus_type_ptr>(item));
            }
        }
        /**
         * Append a copied DBus basic C++ value.
         * @tparam T C++ representation of the basic value.
         * @param value Value to copy.
         * @throws std::invalid_argument If its DBus type differs from the
         *         array element signature.
         */
        template<dbus_basic_cpp_types T>
        void push_back (const T& value) {
            verify_element_signature (dbus_basic<T>::make_type_code());
            items.emplace_back (dbus_type_ptr(new dbus_basic<T>(value)));
        }
        /**
         * Append a DBus basic C++ value.
         * @tparam T C++ representation of the basic value.
         * @param value Value to forward to the new element.
         * @throws std::invalid_argument If its DBus type differs from the
         *         array element signature.
         */
        template<dbus_basic_cpp_types T>
        void push_back (T&& value) {
            verify_element_signature (dbus_basic<T>::make_type_code());
            items.emplace_back (dbus_type_ptr(new dbus_basic<T>(std::forward<T>(value))));
        }
        /**
         * Append null-terminated text to an array of strings, object paths, or signatures.
         * @param value The string to append to the array. The string is copied.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        void push_back (const char* value) {
            verify_element_type_is_type_of_string ();
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                items.emplace_back (dbus_type_ptr(new dbus_string(value)));
                break;
            case DBUS_TYPE_OBJECT_PATH:
                items.emplace_back (dbus_type_ptr(new dbus_opath(value)));
                break;
            case DBUS_TYPE_SIGNATURE:
                items.emplace_back (dbus_type_ptr(new dbus_signature(value)));
                break;
            }
        }
        /**
         * Append a string to an array of strings, object paths, or signatures.
         * @param value The string to append to the array. The string is copied.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        void push_back (const std::string& value) {
            verify_element_type_is_type_of_string ();
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                items.emplace_back (dbus_type_ptr(new dbus_string(value)));
                break;
            case DBUS_TYPE_OBJECT_PATH:
                items.emplace_back (dbus_type_ptr(new dbus_opath(value)));
                break;
            case DBUS_TYPE_SIGNATURE:
                items.emplace_back (dbus_type_ptr(new dbus_signature(value)));
                break;
            }
        }
        /**
         * Append a string to an array of strings, object paths, or signatures.
         * @param value The string to append to the array.
         *              The string is moved, not copied.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        void push_back (std::string&& value) {
            verify_element_type_is_type_of_string ();
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                items.emplace_back (dbus_type_ptr(new dbus_string(std::forward<std::string>(value))));
                break;
            case DBUS_TYPE_OBJECT_PATH:
                items.emplace_back (dbus_type_ptr(new dbus_opath(std::forward<std::string>(value))));
                break;
            case DBUS_TYPE_SIGNATURE:
                items.emplace_back (dbus_type_ptr(new dbus_signature(std::forward<std::string>(value))));
                break;
            }
        }

        /**
         * Insert a cloned DBus value before an iterator position.
         * @param pos Position in this array.
         * @param item Value to clone.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If the value has a mismatched signature.
         */
        iterator insert (const_iterator pos, const dbus_type& item) {
            verify_element_signature (item.signature().c_str());
            return iterator (items.insert(pos.iter, item.clone()));
        }
        /**
         * Insert a moved DBus value before an iterator position.
         * @param pos Position in this array.
         * @param item Value to move from.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If the value has a mismatched signature.
         */
        iterator insert (const_iterator pos, dbus_type&& item) {
            verify_element_signature (item.signature().c_str());
            return iterator (items.insert(pos.iter, item.clone_move()));
        }
        /**
         * Insert a copied DBus basic C++ value before an iterator position.
         * @tparam T C++ representation of the basic value.
         * @param pos Position in this array.
         * @param value Value to copy.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If the value has a mismatched type.
         */
        template<dbus_basic_cpp_types T>
        iterator insert (const_iterator pos, const T& value) {
            verify_element_signature (dbus_basic<T>::make_type_code());
            return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_basic<T>(value))));
        }
        /**
         * Insert a forwarded DBus basic C++ value before an iterator position.
         * @tparam T C++ representation of the basic value.
         * @param pos Position in this array.
         * @param value Value to forward.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If the value has a mismatched type.
         */
        template<dbus_basic_cpp_types T>
        iterator insert (const_iterator pos, T&& value) {
            verify_element_signature (dbus_basic<T>::make_type_code());
            return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_basic<T>(std::forward<T>(value)))));
        }
        /**
         * Insert a string to an array of strings, object paths, or signatures.
         * @param pos Position in this array.
         * @param value The string to insert. The string is copied.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        iterator insert (const_iterator pos, const char* value) {
            verify_element_type_is_type_of_string ();
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_string(value))));
            case DBUS_TYPE_OBJECT_PATH:
                return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_opath(value))));
            case DBUS_TYPE_SIGNATURE:
                return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_signature(value))));
            }
            return end ();
        }
        /**
         * Insert a string to an array of strings, object paths, or signatures.
         * @param pos Position in this array.
         * @param value Text to insert by copy.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        iterator insert (const_iterator pos, const std::string& value) {
            verify_element_type_is_type_of_string ();
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_string(value))));
            case DBUS_TYPE_OBJECT_PATH:
                return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_opath(value))));
            case DBUS_TYPE_SIGNATURE:
                return iterator (items.insert(pos.iter, dbus_type_ptr(new dbus_signature(value))));
            }
            return end ();
        }
        /**
         * Insert a string to an array of strings, object paths, or signatures.
         * @param pos Position in this array.
         * @param value Text to insert by moving it.
         * @return An iterator to the inserted element.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        iterator insert (const_iterator pos, std::string&& value) {
            verify_element_type_is_type_of_string ();
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                return iterator (items.insert(pos.iter,
                                              dbus_type_ptr(new dbus_string(std::forward<std::string>(value)))));
            case DBUS_TYPE_OBJECT_PATH:
                return iterator (items.insert(pos.iter,
                                              dbus_type_ptr(new dbus_opath(std::forward<std::string>(value)))));
            case DBUS_TYPE_SIGNATURE:
                return iterator (items.insert(pos.iter,
                                              dbus_type_ptr(new dbus_signature(std::forward<std::string>(value)))));
            }
            return end ();
        }
        /**
         * Insert multiple DBus values at a specific position.
         * @param pos Position in this array.
         * @param count Number of copies to insert.
         * @param item Value to copy.
         * @return An iterator to the first inserted element.
         * @throws std::invalid_argument If the value has a mismatched signature.
         */
        iterator insert (const_iterator pos, size_t count, const dbus_type& item) {
            verify_element_signature (item.signature().c_str());
            auto idx = std::distance (cbegin().iter, pos.iter);
            for (size_t i=0; i<count; ++i)
                items.insert (items.begin()+idx, item.clone());
            return iterator (items.begin()+idx);
        }
        /**
         * Insert values from an iterator range.
         * @tparam InputIt Input iterator whose values are a subclass of dbus_type.
         * @param pos Position in this array.
         * @param first Beginning of the source range.
         * @param last End of the source range.
         * @return An iterator to the first inserted element.
         * @throws std::invalid_argument If the first value has a mismatched signature.
         */
        template<class InputIt>
        iterator insert (const_iterator pos, InputIt first, InputIt last) {
            const_iterator::difference_type idx, ret_idx;
            ret_idx = idx = std::distance (cbegin().iter, pos.iter);
            if (first != last)
                verify_element_signature (first->signature().c_str());
            while (first != last)
                items.insert (items.begin()+(idx++), (first++)->clone());
            return iterator (items.begin()+ret_idx);
        }
        /**
         * Insert DBus basic C++ values from an initializer list.
         * @tparam T C++ representation of the basic values.
         * @param pos Position in this array.
         * @param ilist Values to copy.
         * @return An iterator to the first inserted element.
         * @throws std::invalid_argument If the basic type is mismatched.
         */
        template<dbus_basic_cpp_types T>
        iterator insert (const_iterator pos, std::initializer_list<T> ilist) {
            verify_element_signature (dbus_basic<T>::make_type_code());
            const_iterator::difference_type idx, ret_idx;
            ret_idx = idx = std::distance (cbegin().iter, pos.iter);
            for (auto i : ilist)
                items.insert (items.begin()+(idx++), dbus_type_ptr(new dbus_basic<T>(i)));
            return iterator (items.begin()+ret_idx);
        }
        /**
         * Insert null-terminated strings into an array of strings, object paths, or signatures.
         * @param pos Position in this array.
         * @param ilist Text values to copy; their pointers are not retained.
         * @return An iterator to the first inserted element.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        iterator insert (const_iterator pos, std::initializer_list<const char*> ilist) {
            verify_element_type_is_type_of_string ();
            const_iterator::difference_type idx, ret_idx;
            ret_idx = idx = std::distance (cbegin().iter, pos.iter);
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                for (auto i : ilist)
                    items.insert (items.begin()+(idx++), dbus_type_ptr(new dbus_string(i)));
                break;
            case DBUS_TYPE_OBJECT_PATH:
                for (auto i : ilist)
                    items.insert (items.begin()+(idx++), dbus_type_ptr(new dbus_opath(i)));
                break;
            case DBUS_TYPE_SIGNATURE:
                for (auto i : ilist)
                    items.insert (items.begin()+(idx++), dbus_type_ptr(new dbus_signature(i)));
                break;
            }
            return iterator (items.begin()+ret_idx);
        }
        /**
         * Insert strings into an array of strings, object paths, or signatures.
         * @param pos Position in this array.
         * @param ilist Text values to copy.
         * @return An iterator to the first inserted element.
         * @throws std::invalid_argument If this is not an array of strings,
         *         object paths, or signatures, or if a path/signature is invalid.
         */
        iterator insert (const_iterator pos, std::initializer_list<const std::string> ilist) {
            verify_element_type_is_type_of_string ();
            const_iterator::difference_type idx, ret_idx;
            ret_idx = idx = std::distance (cbegin().iter, pos.iter);
            switch (sig[1]) {
            case DBUS_TYPE_STRING:
                for (auto i : ilist) {
                    items.insert (items.begin() + (idx++),
                                  dbus_type_ptr(new dbus_string(std::forward<std::string>(i))));
                }
                break;
            case DBUS_TYPE_OBJECT_PATH:
                for (auto i : ilist) {
                    items.insert (items.begin() + (idx++),
                                  dbus_type_ptr(new dbus_opath(std::forward<std::string>(i))));
                }
                break;
            case DBUS_TYPE_SIGNATURE:
                for (auto i : ilist) {
                    items.insert (items.begin() + (idx++),
                                  dbus_type_ptr(new dbus_signature(std::forward<std::string>(i))));
                }
                break;
            }
            return iterator (items.begin()+ret_idx);
        }


        /**
         * Remove and destroy the final element, if any.
         */
        constexpr void pop_back () {if(!items.empty()) items.pop_back();};


        /**
         * Remove an element.
         * @param pos Iterator identifying an element in this array.
         * @return An iterator following the erased element.
         */
        constexpr iterator erase (const_iterator pos) {
            return iterator (items.erase(pos.iter));
        }

        /**
         * Remove a range of elements.
         * @param first Beginning of the range to erase.
         * @param last End of the range to erase.
         * @return An iterator following the erased range.
         */
        constexpr iterator erase (const_iterator first, const_iterator last) {
            return iterator (items.erase(first.iter, last.iter));
        }


        /** Return an iterator to the beginning. */
        constexpr iterator begin () {return iterator(items.begin());}
        /** Return an iterator to the end. */
        constexpr iterator end () {return iterator(items.end());}
        /** Return a const iterator to the beginning. */
        constexpr const_iterator begin () const {return const_iterator(items.cbegin());}
        /** Return a const iterator to the end. */
        constexpr const_iterator end () const {return const_iterator(items.cend());}
        /** Return a const iterator to the beginning. */
        constexpr const_iterator cbegin () const {return const_iterator(items.cbegin());}
        /** Return a const iterator to the end. */
        constexpr const_iterator cend () const {return const_iterator(items.cend());}

        /** Return a reverse iterator to the beginning. */
        constexpr reverse_iterator rbegin () {return std::make_reverse_iterator(end());}
        /** Return a reverse iterator to the end. */
        constexpr reverse_iterator rend () {return std::make_reverse_iterator(begin());}
        /** Return a const reverse iterator to the beginning. */
        constexpr const_reverse_iterator rbegin () const {return std::make_reverse_iterator(cend());}
        /** Return a const reverse iterator to the end. */
        constexpr const_reverse_iterator rend () const {return std::make_reverse_iterator(cbegin());}
        /** Return a const reverse iterator to the beginning. */
        constexpr const_reverse_iterator crbegin () const {return std::make_reverse_iterator(cend());}
        /** Return a const reverse iterator to the end. */
        constexpr const_reverse_iterator crend () const {return std::make_reverse_iterator(cbegin());}

        virtual std::string to_string () const;
        virtual std::string to_json () const;

        virtual std::unique_ptr<dbus_type> clone () const {
            return  dbus_type_ptr (new dbus_array(*this));
        }
        virtual std::unique_ptr<dbus_type> clone_move () {
            return  dbus_type_ptr (new dbus_array(std::move(*this)));
        }

    private:
        std::vector<dbus_type_ptr> items;
    };


    inline dbus_array::const_iterator operator+ (dbus_array::const_iterator::difference_type n,
                                                 dbus_array::const_iterator i)
    {
        return dbus_array::const_iterator (n + i.iter);
    }

    inline dbus_array::iterator operator+ (dbus_array::iterator::difference_type n,
                                           dbus_array::iterator i)
    {
        return dbus_array::iterator (n + i.iter);
    }


}
#endif
