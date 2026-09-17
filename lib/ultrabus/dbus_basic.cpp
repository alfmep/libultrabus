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
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_variant.hpp>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool dbus_basic_base::compare_to_variant (const dbus_type& rhs) const
    {
        return operator== (rhs.cast<dbus_variant>().get());
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::partial_ordering dbus_basic_base::compare_three_way_to_variant (const dbus_type& rhs) const
    {
        return operator<=> (rhs.cast<dbus_variant>().get());
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::unique_ptr<dbus_type> create_dbus_basic (int type_code_arg)
    {
        switch (type_code_arg) {
        case DBUS_TYPE_BOOLEAN:
            return std::unique_ptr<dbus_type> (new dbus_bool);
        case DBUS_TYPE_BYTE:
            return std::unique_ptr<dbus_type> (new dbus_byte);
        case DBUS_TYPE_INT16:
            return std::unique_ptr<dbus_type> (new dbus_i16);
        case DBUS_TYPE_UINT16:
            return std::unique_ptr<dbus_type> (new dbus_u16);
        case DBUS_TYPE_INT32:
            return std::unique_ptr<dbus_type> (new dbus_i32);
        case DBUS_TYPE_UINT32:
            return std::unique_ptr<dbus_type> (new dbus_u32);
        case DBUS_TYPE_INT64:
            return std::unique_ptr<dbus_type> (new dbus_i64);
        case DBUS_TYPE_UINT64:
            return std::unique_ptr<dbus_type> (new dbus_u64);
        case DBUS_TYPE_DOUBLE:
            return std::unique_ptr<dbus_type> (new dbus_double);
        case DBUS_TYPE_STRING:
            return std::unique_ptr<dbus_type> (new dbus_string);
        case DBUS_TYPE_OBJECT_PATH:
            return std::unique_ptr<dbus_type> (new dbus_opath);
        case DBUS_TYPE_SIGNATURE:
            return std::unique_ptr<dbus_type> (new dbus_signature);
        case DBUS_TYPE_UNIX_FD:
            return std::unique_ptr<dbus_type> (new dbus_unix_fd);
        default:
            return nullptr;
        }
    }


} // namespace ultrabus
