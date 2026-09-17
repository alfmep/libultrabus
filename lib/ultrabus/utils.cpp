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
#include <ultrabus/utils.hpp>
#include <ultrabus/dbus_basic_types.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <dbus/dbus.h>


namespace ultrabus {


    std::unique_ptr<dbus_type> create_dbus_type (const std::string& sig)
    {
        if (sig.empty() || !dbus_signature_validate_single(sig.c_str(), nullptr))
            return nullptr;

        const int type_code = (uint8_t) sig[0];

        switch (type_code) {
        case DBUS_TYPE_ARRAY:
            if (sig[1] == DBUS_DICT_ENTRY_BEGIN_CHAR) {
                return create_dbus_dict (sig[2], sig.substr(3, sig.size()-4));
            }else{
                return std::unique_ptr<dbus_type> (new dbus_array(sig.c_str()+1));
            }

        case DBUS_STRUCT_BEGIN_CHAR:
            return std::unique_ptr<dbus_type> (new dbus_struct(sig.substr(1, sig.size()-2)));

        case DBUS_TYPE_VARIANT:
            return std::unique_ptr<dbus_type> (new dbus_variant);

        default:
            if (dbus_type_is_valid(type_code) && dbus_type_is_basic(type_code)) {
                return create_dbus_basic (type_code);
            }else{
                return nullptr;
            }
        }
    }


}
