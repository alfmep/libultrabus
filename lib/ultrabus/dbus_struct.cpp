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
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/utils.hpp>

#include <iostream>

namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    dbus_struct::dbus_struct (const std::string& attribute_signatures)
        : dbus_type(DBUS_STRUCT_BEGIN_CHAR_AS_STRING)
    {
        sig.append (attribute_signatures);
        sig.push_back (DBUS_STRUCT_END_CHAR);
        if (attribute_signatures.empty() || dbus_signature_validate(sig.c_str(), nullptr) == FALSE)
            throw std::invalid_argument ("Invalid signature for DBus struct attributes");

        DBusSignatureIter top;
        DBusSignatureIter attrib_iter;
        dbus_signature_iter_init (&top, sig.c_str());
        dbus_signature_iter_recurse (&top, &attrib_iter);
        do {
            char* tmp_sig = dbus_signature_iter_get_signature (&attrib_iter);
            auto tmp_ptr = create_dbus_type (tmp_sig);
            dbus_free (tmp_sig);
            if (tmp_ptr != nullptr)
                items.emplace_back (std::move(tmp_ptr));
            else
                throw std::invalid_argument ("Invalid signature for DBus struct attribute");
        }while (dbus_signature_iter_next(&attrib_iter) == TRUE);
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool dbus_struct::operator== (const dbus_struct& rhs) const
    {
        if (sig != rhs.sig  ||  size() != rhs.size())
            return false;

        auto lhs_i = begin ();
        auto rhs_i = rhs.begin ();
        while (lhs_i!=end()  &&  rhs_i!=rhs.end()) {
            if ( ! (*lhs_i == *rhs_i))
                return false;
            ++lhs_i; ++rhs_i;
        }
        return true;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::partial_ordering dbus_struct::operator<=> (const dbus_struct& rhs) const
    {
        if (sig != rhs.sig  ||  size() != rhs.size())
            return std::partial_ordering::unordered;

        auto lhs_i = begin ();
        auto rhs_i = rhs.begin ();
        while (lhs_i!=end()  &&  rhs_i!=rhs.end()) {
            auto res = *lhs_i <=> *rhs_i;
            if (res != std::partial_ordering::equivalent)
                return res;
            ++lhs_i; ++rhs_i;
        }
        return std::partial_ordering::equivalent;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string dbus_struct::to_string () const
    {
        std::string retval {"("};
        for (auto i=items.begin(); i != items.end(); ++i) {
            if (i != items.begin())
                retval.push_back (',');
            const dbus_type& attrib = *(*i);
            if (attrib.is_string()) {
                retval.push_back ('"');
                retval.append (attrib.to_string());
                retval.push_back ('"');
            }else{
                retval.append (attrib.to_string());
            }
        }
        retval.push_back (')');
        return retval;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string dbus_struct::to_json () const
    {
        std::string json (R"({"type":"struct","sign":")");
        json.append (signature());
        json.append (R"(","value":[)");

        for (auto i=items.begin(); i != items.end(); ++i) {
            if (i != items.begin())
                json.push_back (',');
            json.append ((*i)->to_json());
        }
        json.append ("]}");
        return json;
    }


}
