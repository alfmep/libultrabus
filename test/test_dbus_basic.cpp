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
// Test application exercising all aspects of ultrabus::dbus_basic<T>
// (and its specializations dbus_opath, dbus_signature, dbus_unix_fd).
//
#include <ultrabus/dbus_basic_types.hpp>
#include <iostream>
#include <sstream>
#include <memory>
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
// Default construction: numeric types default to 0/false, strings to "".
//------------------------------------------------------------------------------
static void test_default_construction ()
{
    section ("Default construction");

    dbus_bool   b;
    dbus_byte   y;
    dbus_i16    n;
    dbus_u16    q;
    dbus_i32    i;
    dbus_u32    u;
    dbus_i64    x;
    dbus_u64    t;
    dbus_double d;
    dbus_string s;

    CHECK (b.get() == false);
    CHECK (y.get() == 0);
    CHECK (n.get() == 0);
    CHECK (q.get() == 0);
    CHECK (i.get() == 0);
    CHECK (u.get() == 0);
    CHECK (x.get() == 0);
    CHECK (t.get() == 0);
    CHECK (d.get() == 0.0);
    CHECK (s.get() == "");
    CHECK (s.empty());

    dbus_opath     op;
    dbus_signature sg;
    dbus_unix_fd   fd;
    CHECK (op.get() == "/");
    CHECK (sg.get() == "");
    CHECK (fd.get() == -1);
}


//------------------------------------------------------------------------------
// Construction with an initial value.
//------------------------------------------------------------------------------
static void test_value_construction ()
{
    section ("Value construction");

    dbus_bool   b (true);
    dbus_byte   y (uint8_t{42});
    dbus_i16    n (int16_t{-1234});
    dbus_u16    q (uint16_t{1234});
    dbus_i32    i (int32_t{-123456});
    dbus_u32    u (uint32_t{123456});
    dbus_i64    x (int64_t{-123456789});
    dbus_u64    t (uint64_t{123456789});
    dbus_double d (3.14159);
    dbus_string s (std::string{"hello"});
    dbus_string s2 ("world");   // const char* constructor

    CHECK (b.get() == true);
    CHECK (y.get() == 42);
    CHECK (n.get() == -1234);
    CHECK (q.get() == 1234);
    CHECK (i.get() == -123456);
    CHECK (u.get() == 123456u);
    CHECK (x.get() == -123456789);
    CHECK (t.get() == 123456789u);
    CHECK (d.get() == 3.14159);
    CHECK (s.get() == "hello");
    CHECK (s2.get() == "world");
}


//------------------------------------------------------------------------------
// Copy- and move-construction/assignment.
//------------------------------------------------------------------------------
static void test_copy_move ()
{
    section ("Copy and move semantics");

    dbus_i32 a (42);
    dbus_i32 b (a);            // copy construction
    CHECK (b.get() == 42);
    CHECK (a.get() == 42);     // 'a' unaffected

    dbus_i32 c (std::move(b)); // move construction (value_type is trivially movable)
    CHECK (c.get() == 42);

    dbus_i32 d;
    d = a;                     // copy assignment
    CHECK (d.get() == 42);

    dbus_i32 e;
    e = std::move(c);          // move assignment (dbus_type&& overload)
    CHECK (e.get() == 42);

    dbus_string s1 ("original");
    dbus_string s2 (s1);       // copy construction
    CHECK (s2.get() == "original");

    dbus_string s3 (std::move(s2)); // move construction
    CHECK (s3.get() == "original");

    dbus_string s4;
    s4 = s1;                   // copy assignment
    CHECK (s4.get() == "original");

    dbus_string s5;
    s5 = std::move(s3);        // move assignment
    CHECK (s5.get() == "original");
}


//------------------------------------------------------------------------------
// Construction from a generic dbus_type& (cross-type conversion).
//------------------------------------------------------------------------------
static void test_construction_from_dbus_type ()
{
    section ("Construction from dbus_type&");

    dbus_i32 src (99);
    dbus_type& base = src;

    dbus_u64 dst (base);       // numeric cross-type conversion via base class
    CHECK (dst.get() == 99u);

    dbus_string strval ("a string");
    dbus_type& str_base = strval;
    CHECK_THROWS (dbus_i32 bad (str_base)); // strings can't convert to numeric basics

    dbus_opath opath_val ("/org/example/Object");
    dbus_type& opath_base = opath_val;
    CHECK_THROWS (dbus_i32 bad2 (opath_base));

    dbus_signature sig_val ("i");
    dbus_type& sig_base = sig_val;
    CHECK_THROWS (dbus_i32 bad3 (sig_base));
}


//------------------------------------------------------------------------------
// Construction/assignment from a raw DBusBasicValue union.
//------------------------------------------------------------------------------
static void test_DBusBasicValue ()
{
    section ("DBusBasicValue conversion");

    DBusBasicValue v;

    v.bool_val = TRUE;
    dbus_bool b (v);
    CHECK (b.get() == true);

    v.byt = 7;
    dbus_byte y (v);
    CHECK (y.get() == 7);

    v.i32 = -777;
    dbus_i32 i (v);
    CHECK (i.get() == -777);

    v.u64 = 999999ull;
    dbus_u64 t (v);
    CHECK (t.get() == 999999ull);

    v.dbl = 2.71828;
    dbus_double d (v);
    CHECK (d.get() == 2.71828);

    v.str = const_cast<char*>("dbus-value-string");
    dbus_string s (v);
    CHECK (s.get() == "dbus-value-string");

    // Assignment from DBusBasicValue
    dbus_i32 i2;
    v.i32 = 555;
    i2 = v;
    CHECK (i2.get() == 555);

    dbus_string s2;
    v.str = const_cast<char*>("assigned");
    s2 = v;
    CHECK (s2.get() == "assigned");
}


//------------------------------------------------------------------------------
// Assignment operators: from same dbus_basic<T>, from value_type, and via the
// polymorphic dbus_type& base-class overloads.
//------------------------------------------------------------------------------
static void test_assignment ()
{
    section ("Assignment operators");

    dbus_i32 a;
    a = 123;                   // assignment from value_type
    CHECK (a.get() == 123);

    dbus_i32 b;
    b = a;                     // assignment from dbus_basic<int32_t>
    CHECK (b.get() == 123);

    dbus_i32 c (0);
    dbus_type& c_base = c;
    dbus_i32 d (55);
    c_base = d;                 // dbus_type& operator= (const dbus_type&)
    CHECK (c.get() == 55);

    dbus_i32 e (0);
    dbus_type& e_base = e;
    CHECK_THROWS (e_base = dbus_string("nope")); // mismatched type -> throws

    dbus_string s;
    s = "plain c-string";
    CHECK (s.get() == "plain c-string");
    s = std::string ("std-string");
    CHECK (s.get() == "std-string");

    dbus_type& s_base = s;
    dbus_string s2 ("via-base");
    CHECK_NOTHROW (s_base = s2);
    CHECK (s.get() == "via-base");
    CHECK_THROWS (s_base = dbus_i32(1)); // mismatched type -> throws
}


//------------------------------------------------------------------------------
// Equality: dbus_basic<N> vs dbus_basic<M>, vs raw value_type, vs
// DBusBasicValue, and via the polymorphic dbus_type::operator==.
//------------------------------------------------------------------------------
static void test_equality ()
{
    section ("Equality operators");

    dbus_i32 a (10);
    dbus_i32 b (10);
    dbus_i32 c (20);
    CHECK (a == b);
    CHECK (!(a == c));

    dbus_i64 big (10);
    CHECK (a == big);          // cross dbus_basic<N> comparison (same signedness)

    CHECK (a == 10);           // compare against raw value_type
    CHECK (!(a == 11));

    DBusBasicValue v;
    v.i32 = 10;
    CHECK (a == v);

    const dbus_type& a_base = a;
    const dbus_type& b_base = b;
    const dbus_type& c_base = c;
    CHECK (a_base == b_base);  // polymorphic comparison
    CHECK (!(a_base == c_base));

    dbus_string s1 ("same");
    dbus_string s2 ("same");
    dbus_string s3 ("different");
    CHECK (s1 == s2);
    CHECK (!(s1 == s3));
    CHECK (s1 == "same");
    CHECK (s1 == std::string("same"));

    const dbus_type& s1_base = s1;
    const dbus_type& s2_base = s2;
    CHECK (s1_base == s2_base);
}


//------------------------------------------------------------------------------
// Three-way comparison (<=>) and the relational operators it provides.
//------------------------------------------------------------------------------
static void test_ordering ()
{
    section ("Three-way comparison");

    dbus_i32 a (5);
    dbus_i32 b (10);

    CHECK ((a <=> b) < 0);
    CHECK ((b <=> a) > 0);
    CHECK ((a <=> a) == 0);
    CHECK (a < b);
    CHECK (b > a);
    CHECK (a <= a);
    CHECK (a >= a);

    CHECK ((a <=> 5) == 0);
    CHECK ((a <=> 100) < 0);

    dbus_double da (1.5);
    dbus_double db (2.5);
    CHECK ((da <=> db) < 0);

    const dbus_type& a_base = a;
    const dbus_type& b_base = b;
    CHECK ((a_base <=> b_base) == std::partial_ordering::less);
    CHECK ((b_base <=> a_base) == std::partial_ordering::greater);

    dbus_bool flag (true);
    const dbus_type& flag_base = flag;
    CHECK ((flag_base <=> a_base) == std::partial_ordering::unordered);

    dbus_string s1 ("abc");
    dbus_string s2 ("abd");
    CHECK ((s1 <=> s2) < 0);
    CHECK (s1 < s2);
}


//------------------------------------------------------------------------------
// Implicit conversion operator and get() accessors.
//------------------------------------------------------------------------------
static void test_conversion_and_accessors ()
{
    section ("Implicit conversion and accessors");

    dbus_i32 a (77);
    int32_t plain = a;          // implicit conversion operator
    CHECK (plain == 77);

    a.get() = 88;                // mutable get()
    CHECK (a.get() == 88);

    const dbus_i32 b (99);
    CHECK (b.get() == 99);       // const get()

    dbus_string s ("value");
    std::string plain_s = s;     // implicit conversion to std::string
    CHECK (plain_s == "value");
}


//------------------------------------------------------------------------------
// Type predicates: is_bool(), is_byte(), ..., is_basic().
//------------------------------------------------------------------------------
static void test_type_queries ()
{
    section ("Type predicates");

    dbus_bool   b;
    dbus_byte   y;
    dbus_i16    n;
    dbus_u16    q;
    dbus_i32    i;
    dbus_u32    u;
    dbus_i64    x;
    dbus_u64    t;
    dbus_double d;
    dbus_string s;
    dbus_opath  op ("/a/b");
    dbus_signature sg ("i");
    dbus_unix_fd fd;

    CHECK (b.is_basic());
    CHECK (b.is_bool());
    CHECK (!b.is_byte());

    CHECK (y.is_byte());
    CHECK (n.is_i16());
    CHECK (q.is_u16());
    CHECK (i.is_i32());
    CHECK (u.is_u32());
    CHECK (x.is_i64());
    CHECK (t.is_u64());
    CHECK (d.is_double());
    CHECK (s.is_string());

    // Object path / signature are not plain strings, but are basics.
    CHECK (op.is_basic());
    CHECK (op.is_opath());
    CHECK (!op.is_string());

    CHECK (sg.is_basic());
    CHECK (sg.is_signature());
    CHECK (!sg.is_string());

    CHECK (fd.is_basic());
    CHECK (fd.is_unix_fd());
    CHECK (!fd.is_i16());
    CHECK (!fd.is_i32());
    CHECK (!fd.is_i64());

    // Sanity: none of the numeric types claim to be some other numeric type.
    CHECK (!i.is_u32());
    CHECK (!u.is_i32());
}


//------------------------------------------------------------------------------
// to_DBusBasicValue(): serialize into the underlying DBusBasicValue union.
//------------------------------------------------------------------------------
static void test_to_DBusBasicValue ()
{
    section ("to_DBusBasicValue()");

    DBusBasicValue v {};

    dbus_i32 i (4242);
    i.to_DBusBasicValue (v);
    CHECK (v.i32 == 4242);

    dbus_bool bo (true);
    bo.to_DBusBasicValue (v);
    CHECK (v.bool_val == TRUE);

    dbus_double d (1.25);
    d.to_DBusBasicValue (v);
    CHECK (v.dbl == 1.25);

    dbus_string s ("round-trip");
    s.to_DBusBasicValue (v);
    CHECK (std::string(v.str) == "round-trip");

    dbus_unix_fd fd (3);
    fd.to_DBusBasicValue (v);
    CHECK (v.fd == 3);
}


//------------------------------------------------------------------------------
// clone() / clone_move(): polymorphic deep copy through the dbus_type base.
//------------------------------------------------------------------------------
static void test_clone ()
{
    section ("clone() and clone_move()");

    dbus_i32 a (321);
    std::unique_ptr<dbus_type> cloned = a.clone();
    CHECK (cloned->is_i32());
    CHECK (cloned->cast<dbus_i32>().get() == 321);
    CHECK (a.get() == 321); // original untouched by clone()

    dbus_i32 b (654);
    std::unique_ptr<dbus_type> moved = b.clone_move();
    CHECK (moved->cast<dbus_i32>().get() == 654);

    dbus_string s ("clone-me");
    std::unique_ptr<dbus_type> cloned_s = s.clone();
    CHECK (cloned_s->is_string());
    CHECK (cloned_s->cast<dbus_string>().get() == "clone-me");
}


//------------------------------------------------------------------------------
// Static make_signature()/make_type_code() and instance signature()/type_code().
//------------------------------------------------------------------------------
static void test_signature_and_type_code ()
{
    section ("Signature and type code");

    CHECK (dbus_bool::make_signature() == "b");
    CHECK (dbus_byte::make_signature() == "y");
    CHECK (dbus_i16::make_signature() == "n");
    CHECK (dbus_u16::make_signature() == "q");
    CHECK (dbus_i32::make_signature() == "i");
    CHECK (dbus_u32::make_signature() == "u");
    CHECK (dbus_i64::make_signature() == "x");
    CHECK (dbus_u64::make_signature() == "t");
    CHECK (dbus_double::make_signature() == "d");
    CHECK (dbus_string::make_signature() == "s");
    CHECK (dbus_opath::make_signature() == "o");
    CHECK (dbus_signature::make_signature() == "g");

    dbus_i32 i;
    CHECK (i.signature() == "i");
    CHECK (i.type_code() == DBUS_TYPE_INT32);
    CHECK (dbus_i32::make_type_code() == DBUS_TYPE_INT32);

    dbus_opath op ("/x");
    CHECK (op.signature() == "o");
    CHECK (op.type_code() == DBUS_TYPE_OBJECT_PATH);
}


//------------------------------------------------------------------------------
// to_string(): human readable representation.
//------------------------------------------------------------------------------
static void test_to_string ()
{
    section ("to_string()");

    CHECK (dbus_bool(true).to_string() == "true");
    CHECK (dbus_bool(false).to_string() == "false");
    CHECK (dbus_i32(-5).to_string() == "-5");
    CHECK (dbus_u32(5).to_string() == "5");
    CHECK (dbus_string("txt").to_string() == "txt");
    CHECK (dbus_opath("/a/b").to_string() == "/a/b");
}


//------------------------------------------------------------------------------
// to_json(): JSON representation including type/sign/value fields.
//------------------------------------------------------------------------------
static void test_to_json ()
{
    section ("to_json()");

    CHECK (dbus_i32(42).to_json() == R"({"type":"int32","signature":"i","value":42})");
    CHECK (dbus_bool(true).to_json() == R"({"type":"boolean","signature":"b","value":true})");
    CHECK (dbus_double(1.5).to_json() == R"({"type":"double","signature":"d","value":1.500000})");

    CHECK (dbus_string("hi").to_json() == R"({"type":"string","signature":"s","value":"hi"})");
    CHECK (dbus_opath("/a").to_json() == R"({"type":"object_path","signature":"o","value":"/a"})");
    CHECK (dbus_signature("i").to_json() == R"({"type":"signature","signature":"g","value":"i"})");

    // Special characters must be escaped.
    dbus_string special ("a\"b\\c\nd");
    std::string json = special.to_json();
    CHECK (json.find (R"(\")") != std::string::npos);
    CHECK (json.find (R"(\\)") != std::string::npos);
    CHECK (json.find (R"(\n)") != std::string::npos);
}


//------------------------------------------------------------------------------
// dbus_string-specific helpers: clear(), empty(), size(), c_str().
//------------------------------------------------------------------------------
static void test_string_helpers ()
{
    section ("dbus_string helpers");

    dbus_string s ("hello");
    CHECK (!s.empty());
    CHECK (s.size() == 5);
    CHECK (std::string(s.c_str()) == "hello");

    s.clear();
    CHECK (s.empty());
    CHECK (s.size() == 0);
}


//------------------------------------------------------------------------------
// dbus_opath: validates DBus object-path syntax.
//------------------------------------------------------------------------------
static void test_opath ()
{
    section ("dbus_opath");

    CHECK_NOTHROW (dbus_opath ("/org/example/Object"));
    CHECK_NOTHROW (dbus_opath (std::string("/org/example/Object")));
    CHECK_THROWS (dbus_opath ("not-a-valid-path"));
    CHECK_THROWS (dbus_opath (""));
    CHECK_THROWS (dbus_opath (static_cast<const char*>(nullptr)));

    dbus_opath op ("/a/b");
    CHECK_NOTHROW (op = "/c/d");
    CHECK (op.get() == "/c/d");
    CHECK_THROWS (op = "invalid");

    CHECK (op.is_opath());
    CHECK (!op.is_string());
}


//------------------------------------------------------------------------------
// dbus_signature: validates DBus type-signature syntax.
//------------------------------------------------------------------------------
static void test_signature_type ()
{
    section ("dbus_signature");

    CHECK_NOTHROW (dbus_signature ("i"));
    CHECK_NOTHROW (dbus_signature ("a{sv}"));
    CHECK_NOTHROW (dbus_signature (""));   // empty signature is valid
    CHECK_THROWS (dbus_signature ("("));   // unbalanced struct

    dbus_signature sg ("i");
    CHECK_NOTHROW (sg = "as");
    CHECK (sg.get() == "as");
    CHECK_THROWS (sg = "(");

    CHECK (sg.is_signature());
    CHECK (!sg.is_string());
}


//------------------------------------------------------------------------------
// dbus_unix_fd: simple wrapper around a file-descriptor integer.
//------------------------------------------------------------------------------
static void test_unix_fd ()
{
    section ("dbus_unix_fd");

    dbus_unix_fd fd;
    CHECK (fd.get() == -1);

    dbus_unix_fd fd2 (5);
    CHECK (fd2.get() == 5);

    fd = fd2;
    CHECK (fd.get() == 5);

    fd = static_cast<unix_fd_t>(9);
    CHECK (fd.get() == 9);

    CHECK (fd.is_unix_fd());
    CHECK (fd.signature() == DBUS_TYPE_UNIX_FD_AS_STRING);
}


//------------------------------------------------------------------------------
// create_dbus_basic(): factory function declared in dbus_basic.cpp.
//------------------------------------------------------------------------------
namespace ultrabus {
    std::unique_ptr<dbus_type> create_dbus_basic (int type_code_arg);
}

static void test_create_dbus_basic_factory ()
{
    section ("create_dbus_basic() factory");

    auto b = create_dbus_basic (DBUS_TYPE_BOOLEAN);
    CHECK (b->is_bool());

    auto i = create_dbus_basic (DBUS_TYPE_INT32);
    CHECK (i->is_i32());

    auto s = create_dbus_basic (DBUS_TYPE_STRING);
    CHECK (s->is_string());

    auto op = create_dbus_basic (DBUS_TYPE_OBJECT_PATH);
    CHECK (op->is_opath());

    auto sg = create_dbus_basic (DBUS_TYPE_SIGNATURE);
    CHECK (sg->is_signature());

    auto fd = create_dbus_basic (DBUS_TYPE_UNIX_FD);
    CHECK (fd->is_unix_fd());

    auto ar = create_dbus_basic (DBUS_TYPE_ARRAY);
    CHECK (ar == nullptr);
}


int main ()
{
    test_default_construction ();
    test_value_construction ();
    test_copy_move ();
    test_construction_from_dbus_type ();
    test_DBusBasicValue ();
    test_assignment ();
    test_equality ();
    test_ordering ();
    test_conversion_and_accessors ();
    test_type_queries ();
    test_to_DBusBasicValue ();
    test_clone ();
    test_signature_and_type_code ();
    test_to_string ();
    test_to_json ();
    test_string_helpers ();
    test_opath ();
    test_signature_type ();
    test_unix_fd ();
    test_create_dbus_basic_factory ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
