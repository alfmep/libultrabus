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
#ifndef ULTRABUS_UTILS_HPP
#define ULTRABUS_UTILS_HPP

#include <ultrabus/dbus_type.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <memory>
#include <dbus/dbus.h>


namespace ultrabus {


    /**
     * Create a new dbus_type instance matching a given DBus signature.
     * @param signature A DBus type signature of a single type,
     *                  for example <code>"s"</code>
     *                  or <code>"a{sv}"</code>.
     * @return A unique pointer to a newly created dbus_type instance
     *         matching the signature. Or <code>nullptr</code> if the
     *         DBus signature is invalid.
     */
    std::unique_ptr<dbus_type> create_dbus_type (const std::string& signature);


    /**
     * Create a new dbus_type instance matching a given DBus signature.
     * @param signature A DBus type signature of a single type,
     *                  for example <code>"s"</code>
     *                  or <code>"a{sv}"</code>.
     * @return A unique pointer to a newly created dbus_type instance
     *         matching the signature. Or <code>nullptr</code> if the
     *         DBus signature is invalid.
     */
    static inline std::unique_ptr<dbus_type> create_dbus_type (const std::string_view& signature) {
        return create_dbus_type (std::string(signature.begin(), signature.end()));
    }


    /**
     * Create a DBus type from a null-terminated signature.
     * @param signature A null-terminated DBus type signature of a single type.
     * @return A newly allocated type matching the first complete signature.
     *         Or <code>nullptr</code> if the DBus signature is invalid.
     */
    static inline std::unique_ptr<dbus_type> create_dbus_type (const char* signature) {
        return create_dbus_type (std::string(signature));
    }

    /**
     * Create a new dbus_type instance matching a given DBus type code.
     * @param type_code A DBus type code of a single complete type,
     *                  for example <code>DBUS_TYPE_STRING</code>.
     * @return A unique pointer to a newly created dbus_type instance
     *         matching the type code.
     *         Or <code>nullptr</code> if the DBus signature is invalid.
     */
    static inline std::unique_ptr<dbus_type> create_dbus_type (int type_code) {
        if (dbus_type_is_valid(type_code) != TRUE)
            return nullptr;
        else
            return create_dbus_type (std::string(1, (char)type_code));
    }


}
#endif
