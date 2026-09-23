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
// Test application exercising all aspects of ultrabus::dbus_array.
//
#include <ultrabus/dbus_array.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
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
// Construction: element-signature/type-code constructors, and validation.
//------------------------------------------------------------------------------
static void test_construction ()
{
    section ("Construction");

    dbus_array a1 ("i");                       // const char* element signature
    CHECK (a1.element_signature() == "i");
    CHECK (a1.signature() == "ai");
    CHECK (a1.empty());

    dbus_array a2 (std::string("s"));          // std::string element signature
    CHECK (a2.element_signature() == "s");
    CHECK (a2.signature() == "as");

    dbus_array a3 (DBUS_TYPE_INT32);           // basic type code
    CHECK (a3.element_signature() == "i");

    CHECK_THROWS (dbus_array a4 ("("));                    // invalid/unbalanced signature
    CHECK_THROWS (dbus_array a5 (DBUS_TYPE_ARRAY));         // not a basic type code
    CHECK_THROWS (dbus_array a6 (DBUS_TYPE_INVALID));

    // Nested container element signature is allowed as long as it's valid.
    CHECK_NOTHROW (dbus_array a7 ("ai"));
    dbus_array a7 ("ai");
    CHECK (a7.element_signature() == "ai");
}


//------------------------------------------------------------------------------
// Construction from initializer lists of DBus Basic C++ types and strings.
//------------------------------------------------------------------------------
static void test_initializer_list_construction ()
{
    section ("Initializer-list construction");

    dbus_array ints {1, 2, 3};                 // initializer_list<int32_t>
    CHECK (ints.size() == 3);
    CHECK (ints.element_signature() == "i");
    CHECK (ints.at<dbus_i32>(0).get() == 1);
    CHECK (ints.at<dbus_i32>(2).get() == 3);

    dbus_array dbls {1.5, 2.5};                // initializer_list<double>
    CHECK (dbls.size() == 2);
    CHECK (dbls.element_signature() == "d");
    CHECK (dbls.at<dbus_double>(1).get() == 2.5);

    dbus_array cstrs {"one", "two", "three"};  // initializer_list<const char*>
    CHECK (cstrs.size() == 3);
    CHECK (cstrs.element_signature() == "s");
    CHECK (cstrs.at<dbus_string>(0).get() == "one");

    dbus_array strs {std::string("alpha"), std::string("beta")}; // initializer_list<std::string>
    CHECK (strs.size() == 2);
    CHECK (strs.element_signature() == "s");
    CHECK (strs.at<dbus_string>(1).get() == "beta");

    dbus_array opaths (DBUS_TYPE_OBJECT_PATH, {"/a/b", "/c/d"});
    CHECK (opaths.size() == 2);
    CHECK (opaths.element_signature() == "o");
    CHECK (opaths.at<dbus_opath>(0).get() == "/a/b");

    dbus_array sigs (DBUS_TYPE_SIGNATURE, {"i", "as"});
    CHECK (sigs.size() == 2);
    CHECK (sigs.element_signature() == "g");
    CHECK (sigs.at<dbus_signature>(1).get() == "as");

    CHECK_THROWS (dbus_array bad (DBUS_TYPE_INT32, {"x", "y"})); // not a string type code
}


//------------------------------------------------------------------------------
// Copy- and move-construction, and that copies are deep (independent).
//------------------------------------------------------------------------------
static void test_copy_move ()
{
    section ("Copy and move semantics");

    dbus_array a {1, 2, 3};
    dbus_array b (a);                  // copy construction
    CHECK (b.size() == 3);
    CHECK (a == b);

    b.at<dbus_i32>(0).get() = 99;      // mutate the copy...
    CHECK (a.at<dbus_i32>(0).get() == 1);  // ...original must be unaffected (deep copy)
    CHECK (!(a == b));

    dbus_array c (std::move(b));       // move construction
    CHECK (c.size() == 3);
    CHECK (c.at<dbus_i32>(0).get() == 99);

    dbus_array d ("i");
    d = a;                              // copy assignment
    CHECK (d == a);
    d.at<dbus_i32>(1).get() = 42;
    CHECK (a.at<dbus_i32>(1).get() == 2); // still unaffected

    dbus_array e ("i");
    e = std::move (c);                  // move assignment
    CHECK (e.size() == 3);
    CHECK (e.at<dbus_i32>(0).get() == 99);
}


//------------------------------------------------------------------------------
// Polymorphic dbus_type& assignment.
//------------------------------------------------------------------------------
static void test_polymorphic_assignment ()
{
    section ("Polymorphic assignment via dbus_type&");

    dbus_array a {1, 2};
    dbus_array b {10, 20, 30};
    dbus_type& a_base = a;
    dbus_type& b_base = b;

    CHECK_NOTHROW (a_base = b_base);
    CHECK (a.size() == 3);
    CHECK (a.at<dbus_i32>(2).get() == 30);

    dbus_i32 not_an_array (5);
    dbus_type& not_array_base = not_an_array;
    CHECK_THROWS (a_base = not_array_base);

    dbus_array c {100, 200};
    dbus_type& c_base = c;
    CHECK_NOTHROW (a_base = std::move(c_base));
    CHECK (a.size() == 2);
    CHECK (a.at<dbus_i32>(0).get() == 100);
}


//------------------------------------------------------------------------------
// Assignment from initializer lists.
//------------------------------------------------------------------------------
static void test_initializer_list_assignment ()
{
    section ("Initializer-list assignment");

    dbus_array a ("i");
    a = {7, 8, 9};
    CHECK (a.size() == 3);
    CHECK (a.at<dbus_i32>(1).get() == 8);

    dbus_array b ("s");
    b = {"x", "y"};
    CHECK (b.size() == 2);
    CHECK (b.at<dbus_string>(0).get() == "x");

    dbus_array c (DBUS_TYPE_OBJECT_PATH, {"/a"});
    c = {"/b", "/c"};
    CHECK (c.size() == 2);
    CHECK (c.at<dbus_opath>(1).get() == "/c");

    dbus_array d (DBUS_TYPE_SIGNATURE, {"i"});
    d = {"as", "i"};
    CHECK (d.size() == 2);
    CHECK (d.at<dbus_signature>(0).get() == "as");

    dbus_array e ("s");
    std::initializer_list<const char*> empty_list {};
    e = empty_list;             // empty initializer list clears the array
    CHECK (e.empty());
}


//------------------------------------------------------------------------------
// Element access: at()/at<T>(), operator[], front()/back() (+ templated
// forms), and their exceptions.
//------------------------------------------------------------------------------
static void test_element_access ()
{
    section ("Element access");

    dbus_array a {1, 2, 3};

    CHECK (a.at(0).to_string() == "1");
    CHECK (a.at<dbus_i32>(1).get() == 2);
    CHECK_THROWS (a.at(99));                       // out_of_range
    CHECK_THROWS (a.at<dbus_string>(0));            // bad_cast (wrong type)

    const dbus_array& ca = a;
    CHECK (ca.at(2).to_string() == "3");
    CHECK (ca.at<dbus_i32>(2).get() == 3);

    CHECK (a[0].to_string() == "1");
    a[0].cast<dbus_i32>().get() = 111;
    CHECK (a.at<dbus_i32>(0).get() == 111);

    CHECK (a.front().to_string() == "111");
    CHECK (a.front<dbus_i32>().get() == 111);
    CHECK (a.back().to_string() == "3");
    CHECK (a.back<dbus_i32>().get() == 3);

    const dbus_array& ca2 = a;
    CHECK (ca2.front<dbus_i32>().get() == 111);
    CHECK (ca2.back<dbus_i32>().get() == 3);
}


//------------------------------------------------------------------------------
// empty(), size(), clear(), element_signature().
//------------------------------------------------------------------------------
static void test_capacity ()
{
    section ("Capacity and signature accessors");

    dbus_array a ("i");
    CHECK (a.empty());
    CHECK (a.size() == 0);

    a.push_back (1);
    a.push_back (2);
    CHECK (!a.empty());
    CHECK (a.size() == 2);
    CHECK (a.element_signature() == "i");

    a.clear ();
    CHECK (a.empty());
    CHECK (a.size() == 0);
}


//------------------------------------------------------------------------------
// Equality and three-way comparison.
//------------------------------------------------------------------------------
static void test_equality_and_ordering ()
{
    section ("Equality and ordering");

    dbus_array a {1, 2, 3};
    dbus_array b {1, 2, 3};
    dbus_array c {1, 2, 4};
    dbus_array d {1, 2};
    dbus_array e ("s");
    e = {"1", "2", "3"};

    CHECK (a == b);
    CHECK (!(a == c));
    CHECK (!(a == d));         // different size
    CHECK (!(a == e));         // different element signature

    const dbus_type& a_base = a;
    const dbus_type& b_base = b;
    const dbus_type& e_base = e;
    CHECK (a_base == b_base);
    CHECK (!(a_base == e_base));

    dbus_i32 not_array (1);
    CHECK (!(a_base == static_cast<const dbus_type&>(not_array)));

    CHECK ((a <=> b) == std::partial_ordering::equivalent);
    CHECK ((a <=> c) < 0);
    CHECK ((c <=> a) > 0);
    CHECK ((a <=> d) > 0);      // 'a' has more elements once common prefix matches
    CHECK ((d <=> a) < 0);
    CHECK ((a <=> e) == std::partial_ordering::unordered); // different signature

    const dbus_type& c_base = c;
    CHECK ((a_base <=> c_base) < 0);
    CHECK ((a_base <=> e_base) == std::partial_ordering::unordered);
}


//------------------------------------------------------------------------------
// push_back() overloads and signature verification.
//------------------------------------------------------------------------------
static void test_push_back ()
{
    section ("push_back()");

    dbus_array a ("i");
    a.push_back (1);                     // by value_type
    int32_t v = 2;
    a.push_back (std::move(v));          // by rvalue value_type
    a.push_back (dbus_i32(3));           // by const dbus_type&
    a.push_back (dbus_i32(4));           // constructs temporary -> dbus_type&&
    a.push_back (std::unique_ptr<dbus_type>(new dbus_i32(5))); // by unique_ptr
    CHECK (a.size() == 5);
    for (int i = 0; i < 5; ++i)
        CHECK (a.at<dbus_i32>(static_cast<size_t>(i)).get() == i + 1);

    CHECK_THROWS (a.push_back (dbus_string("wrong-type")));
    CHECK_THROWS (a.push_back ("wrong-type"));

    dbus_array strs ("s");
    strs.push_back ("literal");
    strs.push_back (std::string("std-string"));
    std::string moved_str ("moved");
    strs.push_back (std::move(moved_str));
    CHECK (strs.size() == 3);
    CHECK (strs.at<dbus_string>(0).get() == "literal");
    CHECK (strs.at<dbus_string>(2).get() == "moved");

    dbus_array opaths (DBUS_TYPE_OBJECT_PATH, std::initializer_list<const char*>{});
    opaths.push_back ("/a/b");
    CHECK (opaths.at<dbus_opath>(0).get() == "/a/b");

    dbus_array sigs (DBUS_TYPE_SIGNATURE, std::initializer_list<const char*>{});
    sigs.push_back (std::string("a{sv}"));
    CHECK (sigs.at<dbus_signature>(0).get() == "a{sv}");

    CHECK_THROWS (a.push_back ("nope")); // 'a' holds ints, not string types
}


//------------------------------------------------------------------------------
// insert() overloads: single value, count, range, and initializer lists.
//------------------------------------------------------------------------------
static void test_insert ()
{
    section ("insert()");

    dbus_array a {1, 2, 3};
    auto it = a.insert (a.begin() + 1, dbus_i32(99)); // insert dbus_type&
    CHECK (a.size() == 4);
    CHECK ((*it).to_string() == "99");
    CHECK (a.at<dbus_i32>(1).get() == 99);

    a.insert (a.begin(), 7);                   // insert value_type
    CHECK (a.at<dbus_i32>(0).get() == 7);

    a.insert (a.end(), 3, dbus_i32(5));         // insert 'count' copies
    CHECK (a.size() == 4 + 1 + 3);
    CHECK (a.at<dbus_i32>(a.size()-1).get() == 5);

    dbus_array src {1000, 2000};
    a.insert (a.begin(), src.begin(), src.end()); // range insert
    CHECK (a.at<dbus_i32>(0).get() == 1000);
    CHECK (a.at<dbus_i32>(1).get() == 2000);

    dbus_array b ("i");
    b.insert (b.begin(), {1, 2, 3});             // initializer_list<T>
    CHECK (b.size() == 3);
    CHECK (b.at<dbus_i32>(2).get() == 3);

    dbus_array strs ("s");
    strs.insert (strs.begin(), {"a", "b"});      // initializer_list<const char*>
    CHECK (strs.size() == 2);
    CHECK (strs.at<dbus_string>(1).get() == "b");

    dbus_array strs2 ("s");
    strs2.insert (strs2.begin(),
                  std::initializer_list<const std::string>{std::string("x"), std::string("y")});
    CHECK (strs2.size() == 2);
    CHECK (strs2.at<dbus_string>(0).get() == "x");

    CHECK_THROWS (b.insert (b.begin(), dbus_string("wrong")));
}


//------------------------------------------------------------------------------
// pop_back() and erase().
//------------------------------------------------------------------------------
static void test_erase_and_pop_back ()
{
    section ("erase() and pop_back()");

    dbus_array a {1, 2, 3, 4, 5};

    a.pop_back ();
    CHECK (a.size() == 4);
    CHECK (a.back<dbus_i32>().get() == 4);

    auto it = a.erase (a.begin());
    CHECK (a.size() == 3);
    CHECK ((*it).to_string() == "2");
    CHECK (a.at<dbus_i32>(0).get() == 2);

    it = a.erase (a.begin(), a.begin() + 2);
    CHECK (a.size() == 1);
    CHECK (a.at<dbus_i32>(0).get() == 4);

    dbus_array empty_arr ("i");
    CHECK_NOTHROW (empty_arr.pop_back ()); // no-op on empty array
    CHECK (empty_arr.empty());
}


//------------------------------------------------------------------------------
// Iterators: forward/reverse, arithmetic, dereference, and mutability.
//------------------------------------------------------------------------------
static void test_iterators ()
{
    section ("Iterators");

    dbus_array a {10, 20, 30};

    // Forward iteration.
    int32_t expected = 10;
    for (auto it = a.begin(); it != a.end(); ++it) {
        CHECK (it->cast<dbus_i32>().get() == expected);
        expected += 10;
    }

    // Const iteration.
    const dbus_array& ca = a;
    int count = 0;
    for (auto it = ca.cbegin(); it != ca.cend(); ++it)
        ++count;
    CHECK (count == 3);

    // Reverse iteration.
    auto rit = a.rbegin();
    CHECK ((*rit).cast<dbus_i32>().get() == 30);
    ++rit;
    CHECK ((*rit).cast<dbus_i32>().get() == 20);
    CHECK (a.rend() - a.rbegin() == 3);

    auto crit = ca.crbegin();
    CHECK ((*crit).cast<dbus_i32>().get() == 30);

    // Random-access arithmetic.
    auto it0 = a.begin();
    auto it2 = it0 + 2;
    CHECK (it2->cast<dbus_i32>().get() == 30);
    CHECK ((it2 - it0) == 2);
    it2 -= 1;
    CHECK (it2->cast<dbus_i32>().get() == 20);
    it2 += 1;
    CHECK (it2->cast<dbus_i32>().get() == 30);
    auto it3 = 1 + it0; // friend operator+ (difference_type, iterator)
    CHECK (it3->cast<dbus_i32>().get() == 20);

    CHECK (a.begin()[1].cast<dbus_i32>().get() == 20);

    // Mutate through iterator.
    (*a.begin()).cast<dbus_i32>().get() = 999;
    CHECK (a.at<dbus_i32>(0).get() == 999);

    // const_iterator obtained implicitly from iterator.
    dbus_array::const_iterator cit = a.begin();
    CHECK (cit->cast<dbus_i32>().get() == 999);

    // const_iterator arithmetic.
    auto cbeg = ca.cbegin();
    auto cend2 = ca.cend();
    CHECK ((cend2 - cbeg) == 3);
    auto cit2 = cbeg + 1;
    CHECK (cit2->cast<dbus_i32>().get() == 20);
    auto cit3 = 1 + cbeg;
    CHECK (cit3->cast<dbus_i32>().get() == 20);
}


//------------------------------------------------------------------------------
// to_string() and to_json().
//------------------------------------------------------------------------------
static void test_to_string_and_json ()
{
    section ("to_string() and to_json()");

    dbus_array ints {1, 2, 3};
    CHECK (ints.to_string() == "[1,2,3]");

    dbus_array strs {"a", "b"};
    CHECK (strs.to_string() == R"(["a","b"])");

    dbus_array empty_arr ("i");
    CHECK (empty_arr.to_string() == "[]");

    std::string json = ints.to_json();
    CHECK (json.find (R"("type":"array")") != std::string::npos);
    CHECK (json.find (R"("signature":"ai")") != std::string::npos);
    CHECK (json.find (R"("value":[)") != std::string::npos);
    CHECK (json.find (R"({"type":"int32","signature":"i","value":1})") != std::string::npos);
}


//------------------------------------------------------------------------------
// clone() / clone_move(): polymorphic deep copy through the dbus_type base.
//------------------------------------------------------------------------------
static void test_clone ()
{
    section ("clone() and clone_move()");

    dbus_array a {1, 2, 3};
    std::unique_ptr<dbus_type> cloned = a.clone();
    CHECK (cloned->is_array());
    dbus_array& cloned_arr = cloned->cast<dbus_array>();
    CHECK (cloned_arr.size() == 3);
    CHECK (cloned_arr == a);

    cloned_arr.at<dbus_i32>(0).get() = 555; // deep copy: must not affect 'a'
    CHECK (a.at<dbus_i32>(0).get() == 1);

    dbus_array b {9, 8, 7};
    std::unique_ptr<dbus_type> moved = b.clone_move();
    CHECK (moved->cast<dbus_array>().at<dbus_i32>(0).get() == 9);
    CHECK (b.size() == 0);
}


int main ()
{
    test_construction ();
    test_initializer_list_construction ();
    test_copy_move ();
    test_polymorphic_assignment ();
    test_initializer_list_assignment ();
    test_element_access ();
    test_capacity ();
    test_equality_and_ordering ();
    test_push_back ();
    test_insert ();
    test_erase_and_pop_back ();
    test_iterators ();
    test_to_string_and_json ();
    test_clone ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
