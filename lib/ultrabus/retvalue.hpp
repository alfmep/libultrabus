/*
 * Copyright (C) 2017,2021,2026 Dan Arrhenius <dan@ultramarin.se>
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
#ifndef ULTRABUS_RETVALUE_HPP
#define ULTRABUS_RETVALUE_HPP


#include <string>


namespace ultrabus {


    /**
     * A value together with an error code and optional error description.
     *
     * @tparam T Type of the contained value. A value is meaningful only when
     *           err() returns zero, error-only constructors leave it
     *           unspecified.
     */
    template <typename T>
    class retvalue {
    public:

        /**
         * Construct a successful result using its default value.
         */
        retvalue () : err_num{0} {}

        /**
         * Construct a successful result by copying a value.
         * @param val Value to copy into this result.
         */
        retvalue (const T& val) : value{val}, err_num{0} {}

        /**
         * Construct a successful result by moving a value.
         * @param val Value to move into this result.
         */
        retvalue (T&& val) : value{std::forward<T>(val)}, err_num{0} {}

        /**
         * Construct an error result by copying an error description.
         * @param err Non-zero error code describing the error.
         * @param err_desc Error description to copy. It may be empty.
         */
        retvalue (int err, const std::string& err_desc) : err_num{err}, err_str{err_desc} {}

        /**
         * Construct an error result by moving its error description.
         * @param err Non-zero error code describing the error.
         * @param err_desc Error description to move. It may be empty.
         */
        retvalue (int err, std::string&& err_desc)
            : err_num{err}, err_str{std::forward<std::string>(err_desc)}
        {
        }

        /**
         * Copy-constructor.
         * @param other Result to copy.
         */
        retvalue (const retvalue<T>& other) {
            value   = other.value;
            err_num = other.err_num;
            err_str = other.err_str;
        }

        /**
         * Move-constructor.
         * @param other Result whose value and description are moved.
         */
        retvalue (retvalue<T>&& other) {
            value   = std::move (other.value);
            err_num = other.err_num;
            err_str = std::move (other.err_str);
        }

        /**
         * Assignment operator.
         *
         * @param rhs Result to copy.
         * @return A reference to this result.
         */
        retvalue<T>& operator= (const retvalue<T>& rhs) {
            if (&rhs != this) {
                value   = rhs.value;
                err_num = rhs.err_num;
                err_str = rhs.err_str;
            }
            return *this;
        }

        /**
         * Move operator.
         *
         * @param rhs Result whose value and description are moved.
         * @return A reference to this result.
         */
        retvalue<T>& operator= (retvalue<T>&& rhs) {
            if (&rhs != this) {
                value   = std::move (rhs.value);
                err_num = rhs.err_num;
                err_str = std::move (rhs.err_str);
            }
            return *this;
        }

        /**
         * Copy-assign the contained value without changing the error state.
         *
         * @param rhs Value to copy.
         * @return A reference to the new contained value.
         */
        T& operator= (const T& rhs) {
            value = rhs;
            return value;
        }

        /**
         * Move-assign the contained value without changing the error state.
         *
         * @param rhs Value to move.
         * @return A reference to the new contained value.
         */
        T& operator= (T&& rhs) {
            value = std::forward<T>(rhs);
            return value;
        }

        /**
         * Implicitly convert to the contained value.
         *
         * @return A mutable reference to the value owned by this result.
         */
        operator T& () {
            return value;
        }

        /**
         * Implicitly convert to the contained value.
         *
         * @return A read-only reference to the value owned by this result.
         */
        operator const T& () const {
            return value;
        }

        /**
         * Access the contained value.
         *
         * @return A mutable reference to the value owned by this result.
         *         The reference is valid while this result remains alive.
         */
        T& get () {
            return value;
        }

        /**
         * Access the contained value.
         *
         * @return A read-only reference to the value owned by this result.
         *         The reference is valid while this result remains alive.
         */
        const T& get () const {
            return value;
        }

        /**
         * Copy a value into this result without changing the error state.
         *
         * @param return_value Value to copy.
         * @return A reference to this result.
         */
        retvalue<T>& set (const T& return_value) {
            value = return_value;
            return *this;
        }

        /**
         * Move a value into this result without changing the error state.
         *
         * @param return_value Value to move.
         * @return A reference to this result.
         */
        retvalue<T>& set (T&& return_value) {
            value = std::forward<T> (return_value);
            return *this;
        }

        /**
         * Return the error code.
         *
         * @return Zero for success, or a non-zero error code.
         */
        int err () const {
            return err_num;
        }

        /**
         * Set the error code without changing the description.
         *
         * @param e New error code, zero conventionally denotes success.
         * @return A reference to this result.
         */
        retvalue<T>& err (int e) {
            err_num = e;
            return *this;
        }

        /**
         * Set the error code and copy an error description.
         *
         * @param e New error code, zero conventionally denotes success.
         * @param description Description to copy. It may be empty.
         * @return A reference to this result.
         */
        retvalue<T>& err (int e, const std::string& description) {
            err_num  = e;
            err_str = description;
            return *this;
        }

        /**
         * Set the error code and move an error description.
         *
         * @param e New error code, zero conventionally denotes success.
         * @param description Description to move. It may be empty.
         * @return A reference to this result.
         */
        retvalue<T>& err (int e, std::string&& description) {
            err_num  = e;
            err_str = std::forward<std::string> (description);
            return *this;
        }

        /**
         * Set an error description, and the error code to
         * -1 if it is currently zero.
         * If the error code is anything other than zero,
         * it is left untouched.
         *
         * @param description Description to copy. It may be empty.
         * @return A reference to this result.
         */
        retvalue<T>& err (const std::string& description) {
            if (err_num == 0)
                err_num = -1;
            err_str = description;
            return *this;
        }

        /**
         * Set an error description, and the error code to
         * -1 if it is currently zero.
         * If the error code is anything other than zero,
         * it is left untouched.
         *
         * @param description Description to move. It may be empty.
         * @return A reference to this result.
         */
        retvalue<T>& err (std::string&& description) {
            if (err_num == 0)
                err_num = -1;
            err_str = std::forward<std::string> (description);
            return *this;
        }

        /**
         * Return the error description.
         *
         * @return A read-only reference to the description owned by this
         *         result. It may be empty and is valid while this result
         *         remains alive.
         */
        const std::string& what () const {
            return err_str;
        }

    private:
        T value;
        int err_num;
        std::string err_str;
    };


}

#endif
