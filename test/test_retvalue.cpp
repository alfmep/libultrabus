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
// Test application exercising all aspects of ultrabus::retvalue.
//
#include <ultrabus/retvalue.hpp>
#include <iostream>
#include <string>
#include <utility>
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


//------------------------------------------------------------------------------
// Default constructor: error code 0, empty error description.
// Note: the return value itself is left undefined by the default
// constructor, so it is not checked here.
//------------------------------------------------------------------------------
static void test_default_construction ()
{
    section ("Default construction");

    retvalue<int> r;
    CHECK (r.err() == 0);
    CHECK (r.what() == "");

    retvalue<std::string> rs;
    CHECK (rs.err() == 0);
    CHECK (rs.what() == "");
}


//------------------------------------------------------------------------------
// Construction from a value (by const-ref and by rvalue): error code is set
// to 0(success) and the error description is empty.
//------------------------------------------------------------------------------
static void test_value_construction ()
{
    section ("Value construction");

    retvalue<int> r1 (42);                     // const T&
    CHECK (r1.get() == 42);
    CHECK (r1.err() == 0);
    CHECK (r1.what() == "");

    int v = 7;
    retvalue<int> r2 (std::move(v));           // T&&
    CHECK (r2.get() == 7);
    CHECK (r2.err() == 0);

    std::string s ("hello");
    retvalue<std::string> r3 (s);              // const T& (string, lvalue)
    CHECK (r3.get() == "hello");
    CHECK (s == "hello");                      // unaffected (copied)

    retvalue<std::string> r4 (std::move(s));   // T&& (string, moved)
    CHECK (r4.get() == "hello");
}


//------------------------------------------------------------------------------
// Construction from an error code and description (by const-ref and by
// rvalue string).
//------------------------------------------------------------------------------
static void test_error_construction ()
{
    section ("Error construction");

    retvalue<int> r1 (-1, "failed");           // const std::string&
    CHECK (r1.err() == -1);
    CHECK (r1.what() == "failed");

    std::string desc ("moved-desc");
    retvalue<int> r2 (-2, std::move(desc));    // std::string&&
    CHECK (r2.err() == -2);
    CHECK (r2.what() == "moved-desc");
}


//------------------------------------------------------------------------------
// Copy and move construction: value, error code, and error description are
// all copied/moved.
//------------------------------------------------------------------------------
static void test_copy_move_construction ()
{
    section ("Copy and move construction");

    retvalue<std::string> r1 (-3, "oops");
    r1.set ("payload");

    retvalue<std::string> r2 (r1);             // copy constructor
    CHECK (r2.get() == "payload");
    CHECK (r2.err() == -3);
    CHECK (r2.what() == "oops");
    CHECK (r1.get() == "payload");             // r1 unaffected by copy

    retvalue<std::string> r3 (std::move(r2));  // move constructor
    CHECK (r3.get() == "payload");
    CHECK (r3.err() == -3);
    CHECK (r3.what() == "oops");
}


//------------------------------------------------------------------------------
// Copy and move assignment of a retvalue<T> (value + error code + error
// description).
//------------------------------------------------------------------------------
static void test_retvalue_assignment ()
{
    section ("retvalue<T> assignment");

    retvalue<std::string> r1 (-4, "bad");
    r1.set ("data");

    retvalue<std::string> r2;
    r2 = r1;                                   // copy assignment
    CHECK (r2.get() == "data");
    CHECK (r2.err() == -4);
    CHECK (r2.what() == "bad");
    CHECK (r1.get() == "data");                // r1 unaffected

    // Self-assignment must be a no-op (not corrupt the object).
    r1 = r1;
    CHECK (r1.get() == "data");
    CHECK (r1.err() == -4);
    CHECK (r1.what() == "bad");

    retvalue<std::string> r3;
    r3 = std::move (r2);                       // move assignment
    CHECK (r3.get() == "data");
    CHECK (r3.err() == -4);
    CHECK (r3.what() == "bad");

    r3 = r3;                                   // self move-assignment: no-op
    CHECK (r3.get() == "data");
}


//------------------------------------------------------------------------------
// Assignment of a bare T value (by const-ref and rvalue): only the value is
// updated; the error code/description are left untouched.
//------------------------------------------------------------------------------
static void test_value_assignment ()
{
    section ("Value assignment");

    retvalue<int> r (-5, "some error");
    r = 100;                                   // const T&
    CHECK (r.get() == 100);
    CHECK (r.err() == -5);                     // error state untouched
    CHECK (r.what() == "some error");

    int v = 200;
    r = std::move (v);                         // T&&
    CHECK (r.get() == 200);
    CHECK (r.err() == -5);

    // operator= (const T&) / (T&&) return a reference to the stored value.
    int& ref = (r = 300);
    CHECK (&ref == &r.get());
    CHECK (ref == 300);
}


//------------------------------------------------------------------------------
// Implicit conversion to T& and const T&.
//------------------------------------------------------------------------------
static void test_implicit_conversion ()
{
    section ("Implicit conversion");

    retvalue<int> r (5);
    int x = r;                                 // operator T&()
    CHECK (x == 5);

    r = 9;                                      // via implicit conversion path in expr below
    int y = r + 1;                              // uses operator T&() in an expression
    CHECK (y == 10);

    const retvalue<int> cr (12);
    int z = cr;                                 // operator const T&() const
    CHECK (z == 12);

    // Mutating through the non-const conversion operator affects the object.
    retvalue<int> r2 (1);
    static_cast<int&>(r2) = 42;
    CHECK (r2.get() == 42);
}


//------------------------------------------------------------------------------
// get(): mutable and const access to the stored value.
//------------------------------------------------------------------------------
static void test_get ()
{
    section ("get()");

    retvalue<int> r (1);
    r.get() = 55;                               // mutable get() allows modification
    CHECK (r.get() == 55);

    const retvalue<int> cr (77);
    CHECK (cr.get() == 77);
}


//------------------------------------------------------------------------------
// set(): copy and move overloads, and chaining via the returned reference.
//------------------------------------------------------------------------------
static void test_set ()
{
    section ("set()");

    retvalue<std::string> r;
    std::string v ("copied");
    retvalue<std::string>& ref1 = r.set (v);    // const T&
    CHECK (&ref1 == &r);
    CHECK (r.get() == "copied");
    CHECK (v == "copied");                      // unaffected (copied)

    std::string v2 ("moved");
    retvalue<std::string>& ref2 = r.set (std::move(v2)); // T&&
    CHECK (&ref2 == &r);
    CHECK (r.get() == "moved");

    // Chaining set() and err() through their returned references.
    retvalue<int> r2;
    r2.set (1).err (-1, "chained");
    CHECK (r2.get() == 1);
    CHECK (r2.err() == -1);
    CHECK (r2.what() == "chained");
}


//------------------------------------------------------------------------------
// err(): getter, and all setter overloads (code only, code+desc by
// const-ref/rvalue, desc only by const-ref/rvalue).
//------------------------------------------------------------------------------
static void test_err ()
{
    section ("err()");

    retvalue<int> r;
    CHECK (r.err() == 0);

    retvalue<int>& ref1 = r.err (-1);           // set error code only
    CHECK (&ref1 == &r);
    CHECK (r.err() == -1);
    CHECK (r.what() == "");                     // description untouched

    r.err (0);                                  // reset to success
    r.err (-2, "desc-by-copy");                 // const std::string&
    CHECK (r.err() == -2);
    CHECK (r.what() == "desc-by-copy");

    std::string desc ("desc-by-move");
    r.err (-3, std::move(desc));                // std::string&&
    CHECK (r.err() == -3);
    CHECK (r.what() == "desc-by-move");

    r.err ("desc-only-by-copy");                // const std::string&, code untouched
    CHECK (r.err() == -3);
    CHECK (r.what() == "desc-only-by-copy");

    r.err (0);                                  // reset to success
    CHECK (r.err() == 0);
    r.err ("desc-only-by-copy_again");          // const std::string&, code set to -1
    CHECK (r.err() == -1);
    CHECK (r.what() == "desc-only-by-copy_again");

    r.err (-3);
    CHECK (r.err() == -3);
    std::string desc2 ("desc-only-by-move");
    r.err (std::move(desc2));                   // std::string&&, code untouched
    CHECK (r.err() == -3);
    CHECK (r.what() == "desc-only-by-move");

    r.err (0);                                  // reset to success
    CHECK (r.err() == 0);
    std::string desc3 ("desc-only-by-move_again");
    r.err (std::move(desc3));                   // std::string&&, code set to -1
    CHECK (r.err() == -1);
    CHECK (r.what() == "desc-only-by-move_again");
}


//------------------------------------------------------------------------------
// what(): returns the (possibly empty) error description.
//------------------------------------------------------------------------------
static void test_what ()
{
    section ("what()");

    retvalue<int> r;
    CHECK (r.what() == "");

    r.err (-1, "an error occurred");
    CHECK (r.what() == "an error occurred");

    const retvalue<int>& cr = r;
    CHECK (cr.what() == "an error occurred");
}


//------------------------------------------------------------------------------
// A more realistic usage pattern: a function returning retvalue<T> used as
// a fallible operation, checked via err()/what() and implicit conversion.
//------------------------------------------------------------------------------
static retvalue<int> divide (int a, int b)
{
    if (b == 0)
        return retvalue<int> (-1, "division by zero");
    return retvalue<int> (a / b);
}

static void test_usage_pattern ()
{
    section ("Typical usage pattern");

    retvalue<int> ok = divide (10, 2);
    CHECK (ok.err() == 0);
    CHECK (ok.get() == 5);
    int implicit = ok;                          // implicit conversion
    CHECK (implicit == 5);

    retvalue<int> fail = divide (10, 0);
    CHECK (fail.err() != 0);
    CHECK (fail.what() == "division by zero");
}


int main ()
{
    test_default_construction ();
    test_value_construction ();
    test_error_construction ();
    test_copy_move_construction ();
    test_retvalue_assignment ();
    test_value_assignment ();
    test_implicit_conversion ();
    test_get ();
    test_set ();
    test_err ();
    test_what ();
    test_usage_pattern ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
