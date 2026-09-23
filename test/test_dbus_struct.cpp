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
// Test application exercising all aspects of ultrabus::dbus_struct.
//
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <iostream>
#include <memory>
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
// Construction: default (invalid/empty) struct, and construction from an
// attribute-signature string.
//------------------------------------------------------------------------------
static void test_construction ()
{
    section ("Construction");

    dbus_struct s0;                            // default: no attributes
    CHECK (s0.signature() == "()");
    CHECK (s0.size() == 0);
    CHECK (!s0.is_valid());                    // empty struct isn't a valid DBus struct

    dbus_struct s1 ("isb");                    // attribute signatures
    CHECK (s1.signature() == "(isb)");
    CHECK (s1.size() == 3);
    CHECK (s1.is_valid());
    CHECK (s1.get(0).signature() == "i");
    CHECK (s1.get(1).signature() == "s");
    CHECK (s1.get(2).signature() == "b");

    dbus_struct s2 (std::string("(ii)a{sv}"));  // nested struct/dict attribute
    CHECK (s2.signature() == "((ii)a{sv})");
    CHECK (s2.size() == 2);
    CHECK (s2.get(0).is_struct());
    CHECK (s2.get(0).signature() == "(ii)");
    CHECK (s2.get(1).signature() == "a{sv}");

    CHECK_THROWS (dbus_struct s3 ("("));                 // unbalanced signature
}


//------------------------------------------------------------------------------
// Copy and move construction.
//------------------------------------------------------------------------------
static void test_copy_move ()
{
    section ("Copy and move construction");

    dbus_struct s1 ("is");
    s1.get<dbus_i32>(0) = 42;
    s1.get<dbus_string>(1) = "hello";

    dbus_struct s2 (s1);                       // copy: deep copy
    CHECK (s2.size() == 2);
    CHECK (s2.get<dbus_i32>(0).get() == 42);
    CHECK (s2.get<dbus_string>(1).get() == "hello");
    s2.get<dbus_i32>(0) = 99;                  // must not affect s1
    CHECK (s1.get<dbus_i32>(0).get() == 42);

    dbus_struct s3 (std::move(s2));            // move
    CHECK (s3.size() == 2);
    CHECK (s3.get<dbus_i32>(0).get() == 99);
    CHECK (s2.size() == 0);                    // moved-from struct is emptied
    CHECK (s2.signature() == "()");
}


//------------------------------------------------------------------------------
// Assignment: typed copy/move assignment, and polymorphic (dbus_type&)
// assignment.
//------------------------------------------------------------------------------
static void test_assignment ()
{
    section ("Assignment");

    dbus_struct s1 ("is");
    s1.get<dbus_i32>(0) = 1;
    s1.get<dbus_string>(1) = "a";

    dbus_struct s2 ("is");
    s2 = s1;                                   // typed copy assignment
    CHECK (s2.get<dbus_i32>(0).get() == 1);
    CHECK (s2.get<dbus_string>(1).get() == "a");
    s2.get<dbus_i32>(0) = 7;
    CHECK (s1.get<dbus_i32>(0).get() == 1);    // deep copy: s1 unaffected

    dbus_struct s3 ("is");
    s3 = std::move (s2);                       // typed move assignment
    CHECK (s3.get<dbus_i32>(0).get() == 7);
    CHECK (s2.size() == 0);

    // Self-assignment must be a no-op (not corrupt the object).
    s3 = s3;
    CHECK (s3.get<dbus_i32>(0).get() == 7);

    dbus_type& base1 = s1;
    dbus_struct s4 ("is");
    dbus_type& base4 = s4;
    base4 = base1;                             // polymorphic copy assignment
    CHECK (s4.get<dbus_i32>(0).get() == 1);
    CHECK (s4.get<dbus_string>(1).get() == "a");

    dbus_struct s5 ("is");
    s5.get<dbus_i32>(0) = 55;
    dbus_struct s6 ("is");
    dbus_type& base6 = s6;
    base6 = std::move (static_cast<dbus_type&>(s5));  // polymorphic move assignment
    CHECK (s6.get<dbus_i32>(0).get() == 55);

    dbus_i32 not_struct (1);
    CHECK_THROWS (base4 = static_cast<dbus_type&>(not_struct));
    CHECK_THROWS (base4 = std::move (static_cast<dbus_type&>(not_struct)));
}


//------------------------------------------------------------------------------
// Element access: get() by index, both mutable/const and typed/untyped.
//------------------------------------------------------------------------------
static void test_element_access ()
{
    section ("Element access");

    dbus_struct s ("isb");
    s.get<dbus_i32>(0) = 10;
    s.get<dbus_string>(1) = "text";
    s.get<dbus_bool>(2) = true;

    CHECK (s.get(0).signature() == "i");
    CHECK (s.get<dbus_i32>(0).get() == 10);
    CHECK (s.get<dbus_string>(1).get() == "text");
    CHECK (s.get<dbus_bool>(2).get() == true);

    const dbus_struct& cs = s;
    CHECK (cs.get(0).signature() == "i");
    CHECK (cs.get<dbus_i32>(0).get() == 10);
    CHECK (cs.get<dbus_string>(1).get() == "text");

    CHECK_THROWS (s.get(5));                    // out of range
    CHECK_THROWS (cs.get(5));
    CHECK_THROWS (s.get<dbus_string>(0));        // wrong type -> std::bad_cast
    CHECK_THROWS (cs.get<dbus_string>(0));
}


//------------------------------------------------------------------------------
// add() overloads: dbus_type lvalue/rvalue, basic C++ types, const char*,
// and unique_ptr<dbus_type>. Verify signature is kept in sync.
//------------------------------------------------------------------------------
static void test_add ()
{
    section ("add()");

    dbus_struct s;
    CHECK (!s.is_valid());

    s.add (dbus_i32(1));                       // const dbus_type&
    CHECK (s.signature() == "(i)");
    CHECK (s.is_valid());

    dbus_string tmp ("abc");
    s.add (std::move(tmp));                    // dbus_type&&
    CHECK (s.signature() == "(is)");
    CHECK (s.get<dbus_string>(1).get() == "abc");

    s.add (true);                              // basic C++ type (bool, by value/copy)
    CHECK (s.signature() == "(isb)");
    CHECK (s.get<dbus_bool>(2).get() == true);

    int32_t v = 77;
    s.add (std::move(v));                      // basic C++ type (rvalue)
    CHECK (s.signature() == "(isbi)");
    CHECK (s.get<dbus_i32>(3).get() == 77);

    s.add ("literal");                         // const char*
    CHECK (s.signature() == "(isbis)");
    CHECK (s.get<dbus_string>(4).get() == "literal");

    const char* null_cstr = nullptr;
    s.add (null_cstr);                         // null const char* is ignored
    CHECK (s.size() == 5);

    s.add (std::unique_ptr<dbus_type>(new dbus_double(3.5))); // unique_ptr<dbus_type>
    CHECK (s.signature() == "(isbisd)");
    CHECK (s.get<dbus_double>(5).get() == 3.5);

    std::unique_ptr<dbus_type> null_ptr;
    s.add (std::move(null_ptr));               // null unique_ptr is ignored
    CHECK (s.size() == 6);

    CHECK (s.size() == 6);
}


//------------------------------------------------------------------------------
// remove() and update_signature().
//------------------------------------------------------------------------------
static void test_remove_and_update_signature ()
{
    section ("remove() and update_signature()");

    dbus_struct s ("isb");
    s.get<dbus_i32>(0) = 1;
    s.get<dbus_string>(1) = "x";
    s.get<dbus_bool>(2) = true;

    s.remove (1);                              // remove the string attribute
    CHECK (s.size() == 2);
    CHECK (s.signature() == "(ib)");
    CHECK (s.get<dbus_i32>(0).get() == 1);
    CHECK (s.get<dbus_bool>(1).get() == true);

    s.remove (100);                            // out-of-range remove is a no-op
    CHECK (s.size() == 2);

    // A nested struct attribute modified through get() invalidates the
    // parent's cached signature until update_signature() is called.
    dbus_struct outer ("(i)");
    outer.get<dbus_struct>(0).add (dbus_string("nested"));
    CHECK (outer.signature() == "((i))");      // stale, not yet refreshed
    outer.update_signature ();
    CHECK (outer.signature() == "((is))");

    while (s.size() > 0)
        s.remove (0);
    CHECK (s.size() == 0);
    CHECK (!s.is_valid());
}


//------------------------------------------------------------------------------
// Equality and three-way comparison, including polymorphic comparisons
// against dbus_variant.
//------------------------------------------------------------------------------
static void test_equality_and_ordering ()
{
    section ("Equality and ordering");

    dbus_struct a ("is");
    a.get<dbus_i32>(0) = 1;
    a.get<dbus_string>(1) = "x";

    dbus_struct b ("is");
    b.get<dbus_i32>(0) = 1;
    b.get<dbus_string>(1) = "x";

    dbus_struct c ("is");
    c.get<dbus_i32>(0) = 2;
    c.get<dbus_string>(1) = "x";

    dbus_struct d ("i");                       // different signature
    d.get<dbus_i32>(0) = 1;

    CHECK (a == b);
    CHECK (!(a == c));
    CHECK (!(a == d));

    const dbus_type& a_base = a;
    const dbus_type& b_base = b;
    const dbus_type& d_base = d;
    CHECK (a_base == b_base);
    CHECK (!(a_base == d_base));

    dbus_i32 not_struct (1);
    CHECK (!(a_base == static_cast<const dbus_type&>(not_struct)));

    dbus_variant var (a);                      // variant wrapping an equal struct
    CHECK (a == var.get());
    CHECK (a_base == static_cast<const dbus_type&>(var));

    CHECK ((a <=> b) == std::partial_ordering::equivalent);
    CHECK ((a <=> c) < 0);
    CHECK ((c <=> a) > 0);
    CHECK ((a <=> d) == std::partial_ordering::unordered);

    const dbus_type& c_base = c;
    CHECK ((a_base <=> c_base) < 0);
    CHECK ((a_base <=> d_base) == std::partial_ordering::unordered);
    CHECK ((a_base <=> static_cast<const dbus_type&>(var)) == std::partial_ordering::equivalent);
}


//------------------------------------------------------------------------------
// Iterators: forward, const, and reverse, over the struct's attributes.
//------------------------------------------------------------------------------
static void test_iterators ()
{
    section ("Iterators");

    dbus_struct s ("iii");
    s.get<dbus_i32>(0) = 1;
    s.get<dbus_i32>(1) = 2;
    s.get<dbus_i32>(2) = 3;

    int32_t expected = 1;
    for (auto& item : s) {
        CHECK (item.cast<dbus_i32>().get() == expected);
        ++expected;
    }

    const dbus_struct& cs = s;
    expected = 1;
    for (auto& item : cs) {
        CHECK (item.cast<dbus_i32>().get() == expected);
        ++expected;
    }

    expected = 3;
    for (auto it = s.rbegin(); it != s.rend(); ++it) {
        CHECK ((*it).cast<dbus_i32>().get() == expected);
        --expected;
    }

    auto it = s.begin ();
    CHECK (it->cast<dbus_i32>().get() == 1);
    ++it;
    CHECK (it->cast<dbus_i32>().get() == 2);
    it += 1;
    CHECK (it == s.end() - 1);
    CHECK (it->cast<dbus_i32>().get() == 3);
    --it;
    CHECK (it->cast<dbus_i32>().get() == 2);

    CHECK (s.end() - s.begin() == 3);
    CHECK (s.cend() - s.cbegin() == 3);
}


//------------------------------------------------------------------------------
// String and JSON serialization.
//------------------------------------------------------------------------------
static void test_to_string_and_json ()
{
    section ("to_string() and to_json()");

    dbus_struct s ("isb");
    s.get<dbus_i32>(0) = 42;
    s.get<dbus_string>(1) = "hi";
    s.get<dbus_bool>(2) = true;

    CHECK (s.to_string() == R"((42,"hi",true))");

    std::string json = s.to_json ();
    CHECK (json.find (R"json("type":"struct")json") != std::string::npos);
    CHECK (json.find (R"json("signature":"(isb)")json") != std::string::npos);
    CHECK (json.find ("42") != std::string::npos);
    CHECK (json.find (R"json("hi")json") != std::string::npos);

    dbus_struct empty;
    CHECK (empty.to_string() == "()");
}


//------------------------------------------------------------------------------
// clone() and clone_move(): deep copy vs. move via the polymorphic API.
//------------------------------------------------------------------------------
static void test_clone ()
{
    section ("clone() and clone_move()");

    dbus_struct a ("is");
    a.get<dbus_i32>(0) = 1;
    a.get<dbus_string>(1) = "x";

    std::unique_ptr<dbus_type> cloned = a.clone ();
    CHECK (cloned->is_struct());
    dbus_struct& cloned_struct = cloned->cast<dbus_struct>();
    CHECK (cloned_struct == a);

    cloned_struct.get<dbus_i32>(0) = 999;      // deep copy: must not affect 'a'
    CHECK (a.get<dbus_i32>(0).get() == 1);

    dbus_struct b ("is");
    b.get<dbus_i32>(0) = 5;
    b.get<dbus_string>(1) = "y";
    std::unique_ptr<dbus_type> moved = b.clone_move ();
    CHECK (moved->cast<dbus_struct>().get<dbus_i32>(0).get() == 5);
    CHECK (b.size() == 0);                     // moved-from struct is emptied
}


//------------------------------------------------------------------------------
// type_code() and is_struct().
//------------------------------------------------------------------------------
static void test_type_identity ()
{
    section ("type_code() and is_struct()");

    dbus_struct s ("i");
    CHECK (s.type_code() == DBUS_TYPE_STRUCT);
    CHECK (s.is_struct());

    const dbus_type& base = s;
    CHECK (base.is_struct());
    CHECK (!base.is_array());
    CHECK (!base.is_variant());
}


//------------------------------------------------------------------------------
// Structs nested inside a struct, and a struct nested inside a dbus_array,
// built via add()/push_back() (which clone the given dbus_type directly and
// therefore don't rely on parsing a signature string for the nested type).
//------------------------------------------------------------------------------
static void test_nested_struct ()
{
    section ("Nested struct");

    dbus_struct inner ("ii");
    inner.get<dbus_i32>(0) = 1;
    inner.get<dbus_i32>(1) = 2;

    dbus_struct outer;
    outer.add (inner);
    outer.add (dbus_string("tag"));
    CHECK (outer.signature() == "((ii)s)");
    CHECK (outer.get(0).is_struct());
    CHECK (outer.get<dbus_struct>(0).get<dbus_i32>(0).get() == 1);
    CHECK (outer.get<dbus_struct>(0).get<dbus_i32>(1).get() == 2);
    CHECK (outer.get<dbus_string>(1).get() == "tag");

    dbus_struct outer2 (outer);                 // copy: deep copy of nested struct too
    outer2.get<dbus_struct>(0).get<dbus_i32>(0) = 99;
    CHECK (outer.get<dbus_struct>(0).get<dbus_i32>(0).get() == 1); // unaffected

    CHECK (outer == outer);
    CHECK (!(outer == outer2));

    dbus_array structs (outer.get(0).signature()); // array of "(ii)" structs
    structs.push_back (inner);
    dbus_struct inner2 ("ii");
    inner2.get<dbus_i32>(0) = 3;
    inner2.get<dbus_i32>(1) = 4;
    structs.push_back (inner2);
    CHECK (structs.size() == 2);
    CHECK (structs.at<dbus_struct>(0).get<dbus_i32>(0).get() == 1);
    CHECK (structs.at<dbus_struct>(1).get<dbus_i32>(1).get() == 4);
}


int main ()
{
    test_construction ();
    test_copy_move ();
    test_assignment ();
    test_element_access ();
    test_add ();
    test_remove_and_update_signature ();
    test_equality_and_ordering ();
    test_iterators ();
    test_to_string_and_json ();
    test_clone ();
    test_type_identity ();
    test_nested_struct ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
