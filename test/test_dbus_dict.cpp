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
// Test application exercising all aspects of ultrabus::dbus_dict.
//
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/dbus_type.hpp>
#include <ultrabus/dbus_variant.hpp>
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
// Construction: value-signature/type-code constructors, and validation.
//------------------------------------------------------------------------------
static void test_construction ()
{
    section ("Construction");

    dbus_string_dict d1;                        // default: variant values
    CHECK (d1.key_type_code() == DBUS_TYPE_STRING);
    CHECK (d1.value_signature() == "v");
    CHECK (d1.signature() == "a{sv}");
    CHECK (d1.empty());
    CHECK (d1.size() == 0);

    dbus_string_dict d2 ("i");                  // const char* value signature
    CHECK (d2.value_signature() == "i");
    CHECK (d2.signature() == "a{si}");

    dbus_string_dict d3 (std::string("as"));    // std::string value signature
    CHECK (d3.value_signature() == "as");
    CHECK (d3.signature() == "a{sas}");

    dbus_string_dict d4 (DBUS_TYPE_INT32);      // basic type code
    CHECK (d4.value_signature() == "i");

    dbus_string_dict d5 (DBUS_TYPE_VARIANT);    // variant type code
    CHECK (d5.value_signature() == "v");

    CHECK_THROWS (dbus_string_dict d6 ("("));               // invalid signature
    CHECK_THROWS (dbus_string_dict d7 (DBUS_TYPE_ARRAY));    // not variant/basic

    // Nested container value signature is allowed as long as it's valid.
    CHECK_NOTHROW (dbus_string_dict d8 ("ai"));
    dbus_string_dict d8 ("ai");
    CHECK (d8.value_signature() == "ai");

    // Different key types.
    dbus_i32_dict di;
    CHECK (di.key_type_code() == DBUS_TYPE_INT32);
    CHECK (di.signature() == "a{iv}");

    dbus_byte_dict db ("y");
    CHECK (db.key_type_code() == DBUS_TYPE_BYTE);
    CHECK (db.signature() == "a{yy}");
}


//------------------------------------------------------------------------------
// dbus_type introspection: is_dict(), signature(), casting.
//------------------------------------------------------------------------------
static void test_type_introspection ()
{
    section ("Type introspection");

    dbus_string_dict d ("i");
    CHECK (d.is_dict());
    CHECK_NOTHROW (d.cast<dbus_string_dict>());
    CHECK_THROWS (d.cast<dbus_i32_dict>());

    dbus_type& base_ref = d;
    CHECK (base_ref.is_dict());
    CHECK (base_ref.signature() == "a{si}");
}


//------------------------------------------------------------------------------
// Copy/move constructors and copy/move assignment (both dedicated
// dbus_dict overloads and the polymorphic dbus_type overloads).
//------------------------------------------------------------------------------
static void test_copy_move ()
{
    section ("Copy/move construction and assignment");

    dbus_string_dict d1 ("i");
    d1.set ("a", 1);
    d1.set ("b", 2);

    // Copy constructor - deep copy.
    dbus_string_dict d2 (d1);
    CHECK (d2.size() == 2);
    d2.set ("a", 99);
    int32_t a_val = 0;
    CHECK (d1.get ("a", a_val)  &&  a_val == 1);   // d1 unaffected
    CHECK (d2.get ("a", a_val)  &&  a_val == 99);

    // Move constructor.
    dbus_string_dict d3 (std::move(d2));
    CHECK (d3.size() == 2);
    CHECK (d2.size() == 0);  // NOLINT - moved-from, checking the postcondition

    // Copy assignment (dbus_dict&).
    dbus_string_dict d4 ("i");
    d4 = d1;
    CHECK (d4.size() == 2);
    d4.set ("a", 123);
    CHECK (d1.get ("a", a_val)  &&  a_val == 1); // d1 unaffected

    // Move assignment (dbus_dict&&).
    dbus_string_dict d5 ("i");
    d5 = std::move (d4);
    CHECK (d5.size() == 2);

    // Polymorphic assignment through dbus_type&.
    dbus_string_dict d6 ("i");
    d6.set ("x", 7);
    dbus_type& base_ref = d6;
    dbus_string_dict d7 ("i");
    d7.set ("y", 8);
    d7 = base_ref;
    CHECK (d7.size() == 1);
    int32_t x_val = 0;
    CHECK (d7.get ("x", x_val)  &&  x_val == 7);

    // Polymorphic assignment from an incompatible dbus_type must throw.
    dbus_i32 not_a_dict (42);
    CHECK_THROWS (d7 = static_cast<dbus_type&>(not_a_dict));
}


//------------------------------------------------------------------------------
// Element access and mutation: operator[], set(), get(), find(), contains().
//------------------------------------------------------------------------------
static void test_element_access ()
{
    section ("Element access");

    dbus_string_dict d ("i");

    // operator[] creates a default value if it doesn't exist.
    d["one"].cast<dbus_i32>().get() = 1;
    CHECK (d.size() == 1);
    CHECK (d["one"].cast<dbus_i32>().get() == 1);

    // set() with a raw C++ basic type.
    CHECK (d.set ("two", int32_t(2)));
    CHECK (d.size() == 2);

    // set() with a dbus_type (matching signature).
    CHECK (d.set ("three", dbus_i32(3)));

    // set() with a dbus_type&& (matching signature).
    CHECK (d.set ("four", dbus_i32(4)));

    // set() with a unique_ptr<dbus_type>.
    CHECK (d.set ("five", std::unique_ptr<dbus_type>(new dbus_i32(5))));
    CHECK_NOTHROW (d.set ("bad-null", std::unique_ptr<dbus_type>()));
    CHECK (!d.set ("bad-null", std::unique_ptr<dbus_type>()));

    // set() with mismatched signature must fail (not throw).
    CHECK (!d.set ("six", dbus_string("wrong-type")));

    // get() into a dbus_type destination.
    dbus_i32 dest;
    CHECK (d.get ("one", dest));
    CHECK (dest.get() == 1);
    CHECK (!d.get ("does-not-exist", dest));

    // get() into a raw C++ basic type destination.
    int32_t raw_dest = 0;
    CHECK (d.get ("two", raw_dest));
    CHECK (raw_dest == 2);
    CHECK (!d.get ("does-not-exist", raw_dest));

    // find()
    auto it = d.find ("three");
    CHECK (it != d.end());
    CHECK (it->second.cast<dbus_i32>().get() == 3);
    CHECK (d.find ("does-not-exist") == d.end());

    // contains()
    CHECK (d.contains ("four"));
    CHECK (!d.contains ("does-not-exist"));

    // Overwriting an existing key with set().
    CHECK (d.set ("one", int32_t(111)));
    raw_dest = 0;
    CHECK (d.get ("one", raw_dest)  &&  raw_dest == 111);
}


//------------------------------------------------------------------------------
// set()/get() with a dbus_variant-valued dict, and all supported
// DBus basic C++ types (bool, integers, double, string types).
//------------------------------------------------------------------------------
static void test_variant_values ()
{
    section ("Variant-valued dict: set()/get() for all basic C++ types");

    dbus_string_dict d;  // default: values are dbus_variant

    CHECK (d.set ("b",  true));
    CHECK (d.set ("y",  uint8_t(200)));
    CHECK (d.set ("n",  int16_t(-100)));
    CHECK (d.set ("q",  uint16_t(100)));
    CHECK (d.set ("i",  int32_t(-1000)));
    CHECK (d.set ("u",  uint32_t(1000)));
    CHECK (d.set ("x",  int64_t(-100000)));
    CHECK (d.set ("t",  uint64_t(100000)));
    CHECK (d.set ("d",  3.5));
    CHECK (d.set ("s",  std::string("hello")));
    CHECK (d.set ("cs", "hello-c-str"));

    bool b_val = false;
    CHECK (d.get ("b", b_val)  &&  b_val == true);

    uint8_t y_val = 0;
    CHECK (d.get ("y", y_val)  &&  y_val == 200);

    int16_t n_val = 0;
    CHECK (d.get ("n", n_val)  &&  n_val == -100);

    uint16_t q_val = 0;
    CHECK (d.get ("q", q_val)  &&  q_val == 100);

    int32_t i_val = 0;
    CHECK (d.get ("i", i_val)  &&  i_val == -1000);

    uint32_t u_val = 0;
    CHECK (d.get ("u", u_val)  &&  u_val == 1000);

    int64_t x_val = 0;
    CHECK (d.get ("x", x_val)  &&  x_val == -100000);

    uint64_t t_val = 0;
    CHECK (d.get ("t", t_val)  &&  t_val == 100000);

    double d_val = 0.0;
    CHECK (d.get ("d", d_val)  &&  d_val == 3.5);

    std::string s_val;
    CHECK (d.get ("s", s_val)  &&  s_val == "hello");
    CHECK (d.get ("cs", s_val)  &&  s_val == "hello-c-str");

    // Values are stored as dbus_variant since no explicit value signature was given.
    CHECK (d.find("i")->second.is_variant());

    // Getting a value with a mismatched destination type must fail.
    int32_t wrong_type = 0;
    CHECK (!d.get ("s", wrong_type));

    // Setting a null C-string must fail.
    CHECK (!d.set ("null-cstr", static_cast<const char*>(nullptr)));
}


//------------------------------------------------------------------------------
// set()/get() with a dict whose values are a fixed (non-variant) basic type.
//------------------------------------------------------------------------------
static void test_fixed_value_type ()
{
    section ("Fixed-type-valued dict");

    dbus_string_dict d ("u"); // values must be uint32_t

    CHECK (d.set ("a", uint32_t(42)));
    CHECK (!d.find("a")->second.is_variant()); // stored directly, not wrapped in a variant

    // Setting a value of the wrong basic C++ type must fail.
    CHECK (!d.set ("b", int32_t(1)));
    CHECK (!d.set ("b", std::string("nope")));

    uint32_t val = 0;
    CHECK (d.get ("a", val)  &&  val == 42);

    // String subtypes (string/object-path/signature) route through sig[3].
    dbus_string_dict opath_dict ("o");
    CHECK (opath_dict.set ("k", std::string("/some/object/path")));
    std::string opath_val;
    CHECK (opath_dict.get ("k", opath_val)  &&  opath_val == "/some/object/path");

    dbus_string_dict sig_dict ("g");
    CHECK (sig_dict.set ("k", "a{sv}"));
    std::string sig_val;
    CHECK (sig_dict.get ("k", sig_val)  &&  sig_val == "a{sv}");
}


//------------------------------------------------------------------------------
// reset(): clears content and changes the value signature/type.
//------------------------------------------------------------------------------
static void test_reset ()
{
    section ("reset()");

    dbus_string_dict d ("i");
    d.set ("a", 1);
    CHECK (d.size() == 1);

    CHECK (d.reset ("s"));
    CHECK (d.empty());
    CHECK (d.value_signature() == "s");
    CHECK (d.set ("a", std::string("now-a-string")));

    CHECK (d.reset (std::string("d")));
    CHECK (d.empty());
    CHECK (d.value_signature() == "d");

    CHECK (d.reset (DBUS_TYPE_BOOLEAN));
    CHECK (d.empty());
    CHECK (d.value_signature() == "b");

    CHECK (d.reset (DBUS_TYPE_VARIANT));
    CHECK (d.value_signature() == "v");

    // Invalid reset arguments leave the dict unchanged and return false.
    d.set ("k", true);
    CHECK (!d.reset ("("));
    CHECK (!d.reset (static_cast<const char*>(nullptr)));
    CHECK (!d.reset (DBUS_TYPE_ARRAY));
    CHECK (d.value_signature() == "v"); // unchanged
    CHECK (d.size() == 1);              // unchanged
}


//------------------------------------------------------------------------------
// Equality and three-way comparison.
//------------------------------------------------------------------------------
static void test_equality_and_ordering ()
{
    section ("Equality and ordering");

    dbus_string_dict d1 ("i");
    d1.set ("a", 1);
    d1.set ("b", 2);

    dbus_string_dict d2 ("i");
    d2.set ("a", 1);
    d2.set ("b", 2);

    dbus_string_dict d3 ("i");
    d3.set ("a", 1);
    d3.set ("b", 999);

    CHECK (d1 == d1);                 // self
    CHECK (d1 == d2);                 // dbus_dict/dbus_dict overload
    CHECK (!(d1 == d3));

    dbus_type& base1 = d1;
    dbus_type& base2 = d2;
    CHECK (base1 == base2);           // dbus_type/dbus_type overload
    CHECK (!(base1 == static_cast<dbus_type&>(d3)));

    // Different value signature -> never equal, even with same keys/values.
    dbus_string_dict d4 ("u");
    d4.set ("a", uint32_t(1));
    CHECK (!(base1 == static_cast<dbus_type&>(d4)));

    // Comparison with an unrelated dbus_type is always false.
    dbus_i32 not_a_dict (1);
    CHECK (!(base1 == static_cast<dbus_type&>(not_a_dict)));

    // Three-way comparison.
    CHECK ((d1 <=> d2) == std::partial_ordering::equivalent);
    CHECK ((d1 <=> d3) != std::partial_ordering::equivalent);

    dbus_string_dict shorter ("i");
    shorter.set ("a", 1);
    CHECK ((shorter <=> d1) == std::partial_ordering::less);
    CHECK ((d1 <=> shorter) == std::partial_ordering::greater);

    // Comparing dicts with different signatures via the polymorphic overload.
    CHECK ((base1 <=> static_cast<dbus_type&>(d4)) == std::partial_ordering::unordered);
}


//------------------------------------------------------------------------------
// Iterators: forward, const, reverse, and const-reverse.
//------------------------------------------------------------------------------
static void test_iterators ()
{
    section ("Iterators");

    dbus_string_dict d ("i");
    d.set ("a", 1);
    d.set ("b", 2);
    d.set ("c", 3);

    // Forward iteration (std::map key order: a, b, c).
    int32_t expected = 1;
    for (auto it = d.begin(); it != d.end(); ++it) {
        CHECK (it->second.cast<dbus_i32>().get() == expected);
        ++expected;
    }

    // Const iteration.
    const dbus_string_dict& cd = d;
    int count = 0;
    for (auto it = cd.cbegin(); it != cd.cend(); ++it)
        ++count;
    CHECK (count == 3);

    // Reverse iteration.
    auto rit = d.rbegin();
    CHECK (rit->second.cast<dbus_i32>().get() == 3);
    ++rit;
    CHECK (rit->second.cast<dbus_i32>().get() == 2);

    auto crit = cd.crbegin();
    CHECK (crit->second.cast<dbus_i32>().get() == 3);

    // Mutate through iterator.
    auto it = d.begin();
    it->second.cast<dbus_i32>().get() = 42;
    int32_t a_val = 0;
    CHECK (d.get ("a", a_val)  &&  a_val == 42);

    // Iterator equality/inequality.
    CHECK (d.begin() == d.begin());
    CHECK (d.begin() != d.end());

    // const_iterator obtained implicitly from iterator.
    dbus_string_dict::const_iterator cit = d.begin();
    CHECK (cit == d.cbegin());
}


//------------------------------------------------------------------------------
// clear(), empty(), size().
//------------------------------------------------------------------------------
static void test_capacity ()
{
    section ("clear()/empty()/size()");

    dbus_string_dict d ("i");
    CHECK (d.empty());
    CHECK (d.size() == 0);

    d.set ("a", 1);
    d.set ("b", 2);
    CHECK (!d.empty());
    CHECK (d.size() == 2);

    d.clear ();
    CHECK (d.empty());
    CHECK (d.size() == 0);
}


//------------------------------------------------------------------------------
// to_string() and to_json().
//------------------------------------------------------------------------------
static void test_to_string_and_json ()
{
    section ("to_string() and to_json()");

    dbus_string_dict d ("i");
    CHECK (d.to_string() == "[]");

    d.set ("a", 1);
    CHECK (d.to_string() == R"([{"a":1}])");

    d.set ("b", 2);
    CHECK (d.to_string() == R"([{"a":1},{"b":2}])");

    std::string json = d.to_json();
    CHECK (json.find (R"("type":"dict")") != std::string::npos);
    CHECK (json.find (R"("signature":"a{si}")") != std::string::npos);
    CHECK (json.find (R"("value":[)") != std::string::npos);
    CHECK (json.find (R"({"key":{"type":"string","signature":"s","value":"a"},)") != std::string::npos);
}


//------------------------------------------------------------------------------
// clone() / clone_move(): polymorphic deep copy through the dbus_type base.
//------------------------------------------------------------------------------
static void test_clone ()
{
    section ("clone() and clone_move()");

    dbus_string_dict d ("i");
    d.set ("a", 1);
    d.set ("b", 2);

    std::unique_ptr<dbus_type> cloned = d.clone ();
    CHECK (cloned->is_dict());
    dbus_string_dict& cloned_dict = cloned->cast<dbus_string_dict>();
    CHECK (cloned_dict.size() == 2);
    CHECK (cloned_dict == d);

    cloned_dict.set ("a", 999); // deep copy: must not affect 'd'
    int32_t a_val = 0;
    CHECK (d.get ("a", a_val)  &&  a_val == 1);

    dbus_string_dict d2 ("i");
    d2.set ("x", 7);
    std::unique_ptr<dbus_type> moved = d2.clone_move ();
    int32_t x_val = 0;
    CHECK (moved->cast<dbus_string_dict>().get ("x", x_val)  &&  x_val == 7);
    CHECK (d2.size() == 0);
}


//------------------------------------------------------------------------------
// create_dbus_dict(): factory function returning a generic dbus_type.
//------------------------------------------------------------------------------
static void test_create_dbus_dict ()
{
    section ("create_dbus_dict()");

    auto d1 = create_dbus_dict (DBUS_TYPE_STRING, "i");
    CHECK (d1->is_dict());
    CHECK (d1->signature() == "a{si}");
    CHECK_NOTHROW (d1->cast<dbus_string_dict>());

    auto d2 = create_dbus_dict (DBUS_TYPE_INT32, std::string("s"));
    CHECK (d2->signature() == "a{is}");
    CHECK_NOTHROW (d2->cast<dbus_i32_dict>());

    auto d3 = create_dbus_dict (DBUS_TYPE_BYTE, DBUS_TYPE_VARIANT);
    CHECK (d3->signature() == "a{yv}");
    CHECK_NOTHROW (d3->cast<dbus_byte_dict>());

    // Exercise creation for the remaining supported key types.
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_BOOLEAN, "i")->cast<dbus_bool_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_INT16, "i")->cast<dbus_i16_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_UINT16, "i")->cast<dbus_u16_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_UINT32, "i")->cast<dbus_u32_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_INT64, "i")->cast<dbus_i64_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_UINT64, "i")->cast<dbus_u64_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_DOUBLE, "i")->cast<dbus_double_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_OBJECT_PATH, "i")->cast<dbus_opath_dict>());
    CHECK_NOTHROW (create_dbus_dict (DBUS_TYPE_SIGNATURE, "i")->cast<dbus_signature_dict>());

    auto d4 = create_dbus_dict (DBUS_TYPE_STRING, "_");  // Invalid signature
    CHECK (d4 == nullptr);

    auto d5 = create_dbus_dict (DBUS_TYPE_STRING, "");   // Empty (invalid) signature
    CHECK (d5 == nullptr);

    auto d6 = create_dbus_dict (DBUS_TYPE_STRING, "ii"); // Signature containing multiple types
    CHECK (d6 == nullptr);
}


int main ()
{
    test_construction ();
    test_type_introspection ();
    test_copy_move ();
    test_element_access ();
    test_variant_values ();
    test_fixed_value_type ();
    test_reset ();
    test_equality_and_ordering ();
    test_iterators ();
    test_capacity ();
    test_to_string_and_json ();
    test_clone ();
    test_create_dbus_dict ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
