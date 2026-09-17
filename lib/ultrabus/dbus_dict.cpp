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
#include <ultrabus/dbus_dict.hpp>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::unique_ptr<dbus_type> create_dbus_dict (int dbus_type_code, const char* value_signature)
    {
        if (!dbus_signature_validate_single(value_signature, nullptr))
            return nullptr;
        switch (dbus_type_code) {
        case DBUS_TYPE_BOOLEAN:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_bool>(value_signature));
        case DBUS_TYPE_BYTE:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_byte>(value_signature));
        case DBUS_TYPE_INT16:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_i16>(value_signature));
        case DBUS_TYPE_UINT16:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_u16>(value_signature));
        case DBUS_TYPE_INT32:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_i32>(value_signature));
        case DBUS_TYPE_UINT32:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_u32>(value_signature));
        case DBUS_TYPE_INT64:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_i64>(value_signature));
        case DBUS_TYPE_UINT64:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_u64>(value_signature));
        case DBUS_TYPE_DOUBLE:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_double>(value_signature));
        case DBUS_TYPE_STRING:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_string>(value_signature));
        case DBUS_TYPE_OBJECT_PATH:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_opath>(value_signature));
        case DBUS_TYPE_SIGNATURE:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_signature>(value_signature));
        case DBUS_TYPE_UNIX_FD:
            return std::unique_ptr<dbus_type> (new dbus_dict<dbus_unix_fd>(value_signature));
        default:
            return nullptr;
        }
    }


}
