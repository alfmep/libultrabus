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
#include <ultrabus/dbus_array.hpp>


namespace ultrabus {


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    bool dbus_array::operator== (const dbus_array& rhs) const
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
        return true; //lhs_i==end() && rhs_i==rhs.end();
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::partial_ordering dbus_array::operator<=> (const dbus_array& rhs) const
    {
        if (sig != rhs.sig)
            return std::partial_ordering::unordered;

        auto lhs_i = begin ();
        auto rhs_i = rhs.begin ();
        while (lhs_i!=end()  &&  rhs_i!=rhs.end()) {
            auto res = *lhs_i <=> *rhs_i;
            if (res != std::partial_ordering::equivalent)
                return res;
            ++lhs_i; ++rhs_i;
        }
        if (lhs_i != end())
            return std::partial_ordering::greater;
        else if (rhs_i != rhs.end())
            return std::partial_ordering::less;
        else
            return std::partial_ordering::equivalent;
    }


    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string dbus_array::to_string () const
    {
        std::string retval {"["};
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
        retval.push_back (']');
        return retval;
    }



    //--------------------------------------------------------------------------
    //--------------------------------------------------------------------------
    std::string dbus_array::to_json () const
    {
        std::string json (R"({"type":"array","sign":")");
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
