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
//
// Test application exercising all aspects of ultrabus::dbus_variant.
//
#include <ultrabus/dbus_variant.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <iostream>
#include <string>
#include <cstdint>
#include <cstdlib>


using namespace ultrabus;


//------------------------------------------------------------------------------
// Minimal self-contained test harness (no external test framework is used
// by this project).
//------------------------------------------------------------------------------
static unsigned g_num_checks = 0;
static unsigned g_num_failed = 0;
static std::string g_current_section;

static void section (const std::string& name)
{
    g_current_section = name;
    std::cout << "== " << name << " ==" << std::endl;
}

#define CHECK(cond)                                                    \
    do {                                                               \
        ++g_num_checks;                                                \
        if (!(cond)) {                                                 \
            ++g_num_failed;                                            \
            std::cerr << "FAIL [" << g_current_section << "] "         \
                      << __FILE__ << ":" << __LINE__                   \
                      << ": CHECK(" #cond ") failed" << std::endl;      \
        }                                                               \
    } while (0)

#define CHECK_THROWS(expr)                                             \
    do {                                                               \
        ++g_num_checks;                                                \
        bool threw = false;                                            \
        try { expr; }                                                  \
        catch (const std::exception&) { threw = true; }                \
        if (!threw) {                                                  \
            ++g_num_failed;                                            \
            std::cerr << "FAIL [" << g_current_section << "] "         \
                      << __FILE__ << ":" << __LINE__                   \
                      << ": CHECK_THROWS(" #expr ") did not throw"      \
                      << std::endl;                                    \
        }                                                                \
    } while (0)

#define CHECK_NOTHROW(expr)                                             \
    do {                                                                \
        ++g_num_checks;                                                 \
        try { expr; }                                                   \
        catch (const std::exception& e) {                               \
            ++g_num_failed;                                             \
            std::cerr << "FAIL [" << g_current_section << "] "          \
                      << __FILE__ << ":" << __LINE__                    \
                      << ": CHECK_NOTHROW(" #expr ") threw: "            \
                      << e.what() << std::endl;                         \
        }                                                                \
    } while (0)


//------------------------------------------------------------------------------
// Construction: default, signature/type-code, and from dbus_type instances.
//------------------------------------------------------------------------------
static void test_construction ()
{
    section ("Construction");

    dbus_variant v1;                              // default: signed 32 bit int, value 0
    CHECK (v1.signature() == "v");
    CHECK (v1.value_signature() == "i");
    CHECK (v1.get<dbus_i32>().get() == 0);

    dbus_variant v2 ("s");                        // const char* value signature
    CHECK (v2.value_signature() == "s");

    dbus_variant v3 (std::string("d"));           // std::string value signature
    CHECK (v3.value_signature() == "d");

    dbus_variant v4 (DBUS_TYPE_BOOLEAN);          // basic type code
    CHECK (v4.value_signature() == "b");

    dbus_variant v5 (dbus_i32(42));               // from a dbus_type (by const ref)
    CHECK (v5.value_signature() == "i");
    CHECK (v5.get<dbus_i32>().get() == 42);

    dbus_i32 tmp (99);
    dbus_variant v6 (std::move(tmp));             // from a dbus_type&& (rvalue)
    CHECK (v6.get<dbus_i32>().get() == 99);

    dbus_variant v7 (std::unique_ptr<dbus_type>(new dbus_string("hello"))); // from unique_ptr
    CHECK (v7.value_signature() == "s");
    CHECK (v7.get<dbus_string>().get() == "hello");

    // Constructing from another variant (by const ref/rvalue) unwraps
    // the nested variant instead of nesting variant-in-variant.
    dbus_variant inner (dbus_i32(7));
    dbus_variant v8 (inner);
    CHECK (v8.value_signature() == "i");
    CHECK (v8.get<dbus_i32>().get() == 7);

    dbus_variant v9 (dbus_variant(dbus_string("moved-in")));
    CHECK (v9.value_signature() == "s");
    CHECK (v9.get<dbus_string>().get() == "moved-in");

    // Constructing from a unique_ptr to a variant unwraps it as well.
    dbus_variant v10 (std::unique_ptr<dbus_type>(new dbus_variant(dbus_i32(5))));
    CHECK (v10.value_signature() == "i");
    CHECK (v10.get<dbus_i32>().get() == 5);

    CHECK_THROWS (dbus_variant bad ("("));                 // invalid signature
    CHECK_THROWS (dbus_variant bad2 (DBUS_TYPE_ARRAY));    // not a basic type code
}


//------------------------------------------------------------------------------
// dbus_type introspection: is_variant(), signature(), casting.
//------------------------------------------------------------------------------
static void test_type_introspection ()
{
    section ("Type introspection");

    dbus_variant v (dbus_i32(1));
    CHECK (v.is_variant());
    CHECK (v.signature() == "v");
    CHECK (v.type_code() == DBUS_TYPE_VARIANT);

    dbus_type& base_ref = v;
    CHECK (base_ref.is_variant());
    CHECK_NOTHROW (base_ref.cast<dbus_variant>());
}


//------------------------------------------------------------------------------
// Copy/move constructors.
//------------------------------------------------------------------------------
static void test_copy_move_construction ()
{
    section ("Copy/move construction");

    dbus_variant v1 (dbus_i32(123));

    dbus_variant v2 (v1);                          // copy constructor
    CHECK (v2.get<dbus_i32>().get() == 123);
    v2.get<dbus_i32>().get() = 456;
    CHECK (v1.get<dbus_i32>().get() == 123);       // deep copy, v1 unaffected

    dbus_variant v3 (std::move(v2));               // move constructor
    CHECK (v3.get<dbus_i32>().get() == 456);
    CHECK (v2.get<dbus_i32>().get() == 0);         // v2 reset to default value
}


//------------------------------------------------------------------------------
// Assignment operators: dbus_variant/dbus_variant, dbus_type/dbus_type
// (polymorphic), and from raw C++ basic types/strings.
//------------------------------------------------------------------------------
static void test_assignment ()
{
    section ("Assignment operators");

    // dbus_variant& operator= (const dbus_variant&)
    dbus_variant v1 (dbus_i32(1));
    dbus_variant v2 (dbus_string("x"));
    v2 = v1;
    CHECK (v2.value_signature() == "i");
    CHECK (v2.get<dbus_i32>().get() == 1);
    v2.get<dbus_i32>().get() = 99;
    CHECK (v1.get<dbus_i32>().get() == 1); // deep copy

    // dbus_variant& operator= (dbus_variant&&)
    dbus_variant v3 (dbus_string("moveme"));
    dbus_variant v4;
    v4 = std::move (v3);
    CHECK (v4.get<dbus_string>().get() == "moveme");
    CHECK (v3.get<dbus_i32>().get() == 0); // v3 reset to default value

    // Self-assignment must be a no-op (not crash / corrupt state).
    dbus_variant v_self (dbus_i32(42));
    v_self = v_self;
    CHECK (v_self.get<dbus_i32>().get() == 42);

    // virtual dbus_type& operator= (const dbus_type&) - polymorphic, from
    // a plain (non-variant) dbus_type: rhs is cloned and wrapped.
    dbus_variant v5;
    dbus_i32 plain (77);
    dbus_type& v5_base = v5;
    v5_base = static_cast<dbus_type&>(plain);
    CHECK (v5.value_signature() == "i");
    CHECK (v5.get<dbus_i32>().get() == 77);

    // ... and from another dbus_variant via the dbus_type& overload:
    // the contained value is copied, avoiding variant-in-variant nesting.
    dbus_variant v6;
    dbus_variant v7 (dbus_string("via-base"));
    dbus_type& v6_base = v6;
    v6_base = static_cast<dbus_type&>(v7);
    CHECK (v6.value_signature() == "s");
    CHECK (v6.get<dbus_string>().get() == "via-base");

    // virtual dbus_type& operator= (dbus_type&&) - from a plain dbus_type.
    dbus_variant v8;
    dbus_i32 plain2 (88);
    dbus_type& v8_base = v8;
    v8_base = static_cast<dbus_type&&>(std::move(plain2));
    CHECK (v8.get<dbus_i32>().get() == 88);

    // ... and from a moved-from dbus_variant via the dbus_type&& overload.
    dbus_variant v9;
    dbus_variant v10 (dbus_string("via-base-move"));
    dbus_type& v9_base = v9;
    v9_base = static_cast<dbus_type&&>(std::move(v10));
    CHECK (v9.get<dbus_string>().get() == "via-base-move");

    // Assignment from raw DBus basic C++ types (template operator=).
    dbus_variant vb; vb = true;              CHECK (vb.get<bool>() == true);
    dbus_variant vy; vy = uint8_t(200);       CHECK (vy.get<uint8_t>() == 200);
    dbus_variant vn; vn = int16_t(-100);      CHECK (vn.get<int16_t>() == -100);
    dbus_variant vq; vq = uint16_t(100);      CHECK (vq.get<uint16_t>() == 100);
    dbus_variant vi; vi = int32_t(-1000);     CHECK (vi.get<int32_t>() == -1000);
    dbus_variant vu; vu = uint32_t(1000);     CHECK (vu.get<uint32_t>() == 1000);
    dbus_variant vx; vx = int64_t(-100000);   CHECK (vx.get<int64_t>() == -100000);
    dbus_variant vt; vt = uint64_t(100000);   CHECK (vt.get<uint64_t>() == 100000);
    dbus_variant vd; vd = 3.5;                CHECK (vd.get<double>() == 3.5);

    // Assignment from strings (dedicated overloads).
    dbus_variant vs1; vs1 = "c-string";        CHECK (vs1.get<std::string>() == "c-string");
    dbus_variant vs2; vs2 = std::string("s1"); CHECK (vs2.get<std::string>() == "s1");
    std::string moved_str = "s2";
    dbus_variant vs3; vs3 = std::move (moved_str);
    CHECK (vs3.get<std::string>() == "s2");
}


//------------------------------------------------------------------------------
// Equality and three-way comparison, both dbus_variant/dbus_variant and
// the polymorphic dbus_type/dbus_type overloads.
//------------------------------------------------------------------------------
static void test_equality_and_ordering ()
{
    section ("Equality and ordering");

    dbus_variant v1 (dbus_i32(5));
    dbus_variant v2 (dbus_i32(5));
    dbus_variant v3 (dbus_i32(9));
    dbus_variant v4 (dbus_string("5"));

    CHECK (v1 == v1);                       // self
    CHECK (v1 == v2);                       // dbus_variant/dbus_variant overload
    CHECK (!(v1 == v3));
    CHECK (!(v1 == v4));                    // different contained type

    dbus_type& b1 = v1;
    dbus_type& b2 = v2;
    dbus_type& b3 = v3;
    CHECK (b1 == b2);                       // dbus_type/dbus_type overload, both variants
    CHECK (!(b1 == b3));

    // Comparing a variant against a plain (non-variant) dbus_type with the
    // same contained value must be considered equal.
    dbus_i32 plain (5);
    CHECK (b1 == static_cast<dbus_type&>(plain));
    CHECK (static_cast<dbus_type&>(plain) == b1);

    dbus_variant v5 (dbus_opath("/foo/bar"));
    dbus_string plain_str ("/foo/bar");
    CHECK (v5 == plain_str);
    CHECK (plain_str == v5);

    // Comparing a variant against a dbus_array.
    dbus_variant v6 (dbus_array({0,1,2,3}));
    dbus_array a1 ({0,1,2,3});
    CHECK (v6 == a1);
    CHECK (a1 == v6);

    // Three-way comparison.
    CHECK ((v1 <=> v2) == std::partial_ordering::equivalent);
    CHECK ((v1 <=> v3) == std::partial_ordering::less);
    CHECK ((v3 <=> v1) == std::partial_ordering::greater);

    CHECK ((b1 <=> b2) == std::partial_ordering::equivalent);
    CHECK ((b1 <=> static_cast<dbus_type&>(plain)) == std::partial_ordering::equivalent);

    // Three-way comparison a variant against a dbus_array.
    CHECK ((v6 <=> a1) == std::partial_ordering::equivalent);
    CHECK ((a1 <=> v6) == std::partial_ordering::equivalent);
    dbus_array a2 ({1,2,3,4});
    CHECK ((v6 <=> a2) == std::partial_ordering::less);
    CHECK ((a2 <=> v6) == std::partial_ordering::greater);
}


//------------------------------------------------------------------------------
// is_variant(), value_signature().
//------------------------------------------------------------------------------
static void test_value_signature ()
{
    section ("is_variant() and value_signature()");

    dbus_variant v_int (dbus_i32(1));
    CHECK (v_int.is_variant());
    CHECK (v_int.value_signature() == "i");

    dbus_variant v_str (dbus_string("x"));
    CHECK (v_str.value_signature() == "s");

    dbus_variant v_arr (dbus_array{1, 2, 3});
    CHECK (v_arr.value_signature() == "ai");

    dbus_struct s_tmp;
    s_tmp.add (dbus_i32(1));
    s_tmp.add (dbus_string("a"));
    dbus_variant v_struct (s_tmp);
    CHECK (v_struct.value_signature().front() == '(');

    dbus_string_dict dict ("i");
    dict.set ("k", 1);
    dbus_variant v_dict (dict);
    CHECK (v_dict.value_signature() == "a{si}");
}


//------------------------------------------------------------------------------
// get(): all overloads - by dbus_type, by DBus basic C++ type,
// const and non-const, mutation through references.
//------------------------------------------------------------------------------
static void test_get ()
{
    section ("get()");

    dbus_variant v (dbus_i32(42));

    // get<DBusType>() - non-const.
    dbus_i32& i32_ref = v.get<dbus_i32>();
    CHECK (i32_ref.get() == 42);
    i32_ref.get() = 43;
    CHECK (v.get<dbus_i32>().get() == 43);

    // get<BasicCppType>() - non-const.
    int32_t& raw_ref = v.get<int32_t>();
    CHECK (raw_ref == 43);
    raw_ref = 44;
    CHECK (v.get<dbus_i32>().get() == 44);

    // get() with no template argument - non-const dbus_type&.
    dbus_type& generic_ref = v.get ();
    CHECK (generic_ref.is_basic());
    CHECK (generic_ref.cast<dbus_i32>().get() == 44);

    // Wrong requested dbus_type must throw (dynamic_cast<T&> on mismatch).
    CHECK_THROWS (v.get<dbus_string>());

    // const overloads.
    const dbus_variant& cv = v;
    const dbus_i32& c_i32_ref = cv.get<dbus_i32>();
    CHECK (c_i32_ref.get() == 44);

    const int32_t& c_raw_ref = cv.get<int32_t>();
    CHECK (c_raw_ref == 44);

    const dbus_type& c_generic_ref = cv.get ();
    CHECK (c_generic_ref.cast<dbus_i32>().get() == 44);

    CHECK_THROWS (cv.get<dbus_string>());
}


//------------------------------------------------------------------------------
// to_string() and to_json().
//------------------------------------------------------------------------------
static void test_to_string_and_json ()
{
    section ("to_string() and to_json()");

    dbus_variant v_int (dbus_i32(42));
    CHECK (v_int.to_string() == "42");

    dbus_variant v_str (dbus_string("hi"));
    CHECK (v_str.to_string() == "hi");

    std::string json = v_int.to_json ();
    CHECK (json.find (R"("type":"variant")") != std::string::npos);
    CHECK (json.find (R"("signature":"v")") != std::string::npos);
    CHECK (json.find (R"("value":)") != std::string::npos);
    CHECK (json.find (R"({"type":"int32","signature":"i","value":42})") != std::string::npos);
}


//------------------------------------------------------------------------------
// clone() / clone_move(): polymorphic deep copy through the dbus_type base.
//------------------------------------------------------------------------------
static void test_clone ()
{
    section ("clone() and clone_move()");

    dbus_variant v (dbus_i32(10));
    std::unique_ptr<dbus_type> cloned = v.clone ();
    CHECK (cloned->is_variant());
    dbus_variant& cloned_variant = cloned->cast<dbus_variant>();
    CHECK (cloned_variant.get<dbus_i32>().get() == 10);
    CHECK (cloned_variant == v);

    cloned_variant.get<dbus_i32>().get() = 20; // deep copy, must not affect 'v'
    CHECK (v.get<dbus_i32>().get() == 10);

    dbus_variant v2 (dbus_string("move-me"));
    std::unique_ptr<dbus_type> moved = v2.clone_move ();
    CHECK (moved->cast<dbus_variant>().get<dbus_string>().get() == "move-me");
}


//------------------------------------------------------------------------------
// Storing container types (array, struct, dict) inside a variant, and
// round-tripping values through the generic dbus_type interface.
//------------------------------------------------------------------------------
static void test_container_values ()
{
    section ("Container values inside a variant");

    dbus_variant v_arr (dbus_array{1, 2, 3});
    CHECK (v_arr.value_signature() == "ai");
    dbus_array& arr_ref = v_arr.get<dbus_array>();
    CHECK (arr_ref.size() == 3);
    CHECK (arr_ref.at<dbus_i32>(0).get() == 1);

    dbus_struct s;
    s.add (dbus_i32(1));
    s.add (dbus_string("two"));
    dbus_variant v_struct (s);
    dbus_struct& struct_ref = v_struct.get<dbus_struct>();
    CHECK (struct_ref.size() == 2);

    dbus_string_dict dict ("i");
    dict.set ("k1", 11);
    dbus_variant v_dict (dict);
    dbus_dict<dbus_string>& dict_ref = v_dict.get<dbus_dict<dbus_string>>();
    int32_t k1_val = 0;
    CHECK (dict_ref.get ("k1", k1_val)  &&  k1_val == 11);
}


int main ()
{
    test_construction ();
    test_type_introspection ();
    test_copy_move_construction ();
    test_assignment ();
    test_equality_and_ordering ();
    test_value_signature ();
    test_get ();
    test_to_string_and_json ();
    test_clone ();
    test_container_values ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
