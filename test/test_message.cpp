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
// Test application exercising all aspects of ultrabus::message.
//
// These tests operate solely on locally constructed DBusMessage/message
// objects; no DBus daemon connection is required or used.
//
#include <ultrabus/message.hpp>
#include <ultrabus/dbus_array.hpp>
#include <ultrabus/dbus_struct.hpp>
#include <ultrabus/dbus_dict.hpp>
#include <ultrabus/dbus_variant.hpp>
#include <dbus/dbus.h>
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
// Construction: default constructor, method call constructor, and
// destination/path/interface/method validation.
//------------------------------------------------------------------------------
static void test_construction_method_call ()
{
    section ("Construction: method call");

    message m0;                                // default constructor
    CHECK (m0.is_valid());
    CHECK (m0.is_method_call());

    message mc ("org.example.Dest", "/org/example/Path", "org.example.Iface", "Method");
    CHECK (mc.is_valid());
    CHECK (mc.is_method_call());
    CHECK (!mc.is_method_return());
    CHECK (!mc.is_error());
    CHECK (!mc.is_signal());
    CHECK (mc.destination() == "org.example.Dest");
    CHECK (mc.path() == "/org/example/Path");
    CHECK (mc.interface() == "org.example.Iface");
    CHECK (mc.name() == "Method");

    // Interface is optional for a method call.
    CHECK_NOTHROW (message mc2 ("org.example.Dest", "/p", "", "Method"));
    message mc2 ("org.example.Dest", "/p", "", "Method");
    CHECK (mc2.interface() == "");

    CHECK_THROWS (message bad1 ("not a valid bus name", "/p", "org.example.I", "M"));
    CHECK_THROWS (message bad2 ("org.example.Dest", "not-an-object-path", "org.example.I", "M"));
    CHECK_THROWS (message bad3 ("org.example.Dest", "/p", "not an interface", "M"));
    CHECK_THROWS (message bad4 ("org.example.Dest", "/p", "org.example.I", "not a member"));
}


//------------------------------------------------------------------------------
// Construction: signal messages.
//------------------------------------------------------------------------------
static void test_construction_signal ()
{
    section ("Construction: signal");

    message sig ("/org/example/Path", "org.example.Iface", "SignalName");
    CHECK (sig.is_valid());
    CHECK (sig.is_signal());
    CHECK (!sig.is_method_call());
    CHECK (sig.path() == "/org/example/Path");
    CHECK (sig.interface() == "org.example.Iface");
    CHECK (sig.name() == "SignalName");

    CHECK_THROWS (message bad1 ("not-a-path", "org.example.I", "Name"));
    CHECK_THROWS (message bad2 ("/p", "not an interface", "Name"));
    CHECK_THROWS (message bad3 ("/p", "org.example.I", "not a member"));
}


//------------------------------------------------------------------------------
// Construction: from a raw DBusMessage pointer (nullptr yields an invalid
// message; a valid pointer increases its reference count for the lifetime
// of the ultrabus::message object).
//------------------------------------------------------------------------------
static void test_construction_from_dbus_message_pointer ()
{
    section ("Construction: from DBusMessage*");

    message minvalid (nullptr);
    CHECK (!minvalid.is_valid());
    CHECK (minvalid.type() == DBUS_MESSAGE_TYPE_INVALID);
    CHECK (minvalid.handle() == nullptr);

    message src ("org.example.Dest", "/p", "org.example.I", "M");
    DBusMessage* raw = src.handle();
    dbus_uint32_t refs_before = dbus_message_get_serial(raw); // just to touch raw handle

    message wrapped (raw);                     // increases refcount; doesn't take ownership
    CHECK (wrapped.is_valid());
    CHECK (wrapped.handle() == raw);
    CHECK (wrapped.destination() == "org.example.Dest");
    (void) refs_before;
}


//------------------------------------------------------------------------------
// Construction: reply/error-reply messages, both from a raw DBusMessage*
// and from another ultrabus::message. A serial number of 0 (not yet sent)
// must be rejected.
//------------------------------------------------------------------------------
static void test_construction_reply ()
{
    section ("Construction: reply");

    message mc ("org.example.Dest", "/p", "org.example.I", "M");
    CHECK (mc.serial() == 0);

    CHECK_THROWS (message reply (mc, false));           // serial 0: must throw
    CHECK_THROWS (message reply (mc.handle(), false));
    CHECK_THROWS (message no_msg (static_cast<const DBusMessage*>(nullptr), false));

    // Fake a serial number as if the message had been sent, so we can
    // exercise the reply-construction path.
    dbus_message_set_serial (mc.handle(), 123);
    CHECK (mc.serial() == 123);

    message reply (mc, false);                          // via ultrabus::message&
    CHECK (reply.is_method_return());
    CHECK (reply.reply_serial() == 123);

    message reply2 (mc.handle(), false);                // via DBusMessage*
    CHECK (reply2.is_method_return());
    CHECK (reply2.reply_serial() == 123);

    message err (mc, true, "org.example.Error", "Something failed");
    CHECK (err.is_error());
    CHECK (err.error_name() == "org.example.Error");
    CHECK (err.error_msg() == "Something failed");
    CHECK (err.reply_serial() == 123);

    // error_name()/error_msg() are irrelevant (empty) for non-error messages.
    CHECK (reply.error_name() == "");
    CHECK (reply.error_msg() == "");
}


//------------------------------------------------------------------------------
// make_reply(): equivalent convenience method for constructing a reply.
//------------------------------------------------------------------------------
static void test_make_reply ()
{
    section ("make_reply()");

    message mc ("org.example.Dest", "/p", "org.example.I", "M");
    dbus_message_set_serial (mc.handle(), 55);

    message reply = mc.make_reply (false);
    CHECK (reply.is_method_return());
    CHECK (reply.reply_serial() == 55);

    message err = mc.make_reply (true, "org.example.Error", "bad things happened");
    CHECK (err.is_error());
    CHECK (err.error_name() == "org.example.Error");
    CHECK (err.error_msg() == "bad things happened");
}


//------------------------------------------------------------------------------
// Copy and move construction/assignment.
//------------------------------------------------------------------------------
static void test_copy_move ()
{
    section ("Copy and move");

    message m1 ("org.example.Dest", "/p", "org.example.I", "M");
    m1.append_args (42);

    message m2 (m1);                            // copy constructor
    CHECK (m2.handle() != m1.handle());         // deep copy: distinct DBusMessage
    CHECK (m2.destination() == "org.example.Dest");
    CHECK (m2.signature() == "i");

    message m3 (std::move(m2));                 // move constructor
    CHECK (m3.destination() == "org.example.Dest");
    CHECK (m2.handle() == nullptr);              // moved-from message is emptied
    CHECK (!m2.is_valid());

    message m4;
    m4 = m1;                                    // copy assignment
    CHECK (m4.handle() != m1.handle());
    CHECK (m4.destination() == "org.example.Dest");

    m4 = m4;                                    // self-assignment: no-op
    CHECK (m4.destination() == "org.example.Dest");

    message m5;
    m5 = std::move (m4);                        // move assignment
    CHECK (m5.destination() == "org.example.Dest");
    CHECK (m4.handle() == nullptr);
}


//------------------------------------------------------------------------------
// Type predicates (is_valid/is_method_call/is_method_return/is_error/
// is_signal) and type().
//------------------------------------------------------------------------------
static void test_type_predicates ()
{
    section ("Type predicates");

    message mc ("org.example.Dest", "/p", "org.example.I", "M");
    CHECK (mc.type() == DBUS_MESSAGE_TYPE_METHOD_CALL);

    message sig ("/p", "org.example.I", "N");
    CHECK (sig.type() == DBUS_MESSAGE_TYPE_SIGNAL);

    dbus_message_set_serial (mc.handle(), 1);
    message ret = mc.make_reply (false);
    CHECK (ret.type() == DBUS_MESSAGE_TYPE_METHOD_RETURN);

    message err = mc.make_reply (true, "org.example.Err", "desc");
    CHECK (err.type() == DBUS_MESSAGE_TYPE_ERROR);

    message inv (nullptr);
    CHECK (inv.type() == DBUS_MESSAGE_TYPE_INVALID);
}


//------------------------------------------------------------------------------
// Getters/setters: destination, path, interface, name.
//------------------------------------------------------------------------------
static void test_accessors ()
{
    section ("Accessors: destination/path/interface/name");

    message m ("org.example.Dest", "/p", "org.example.I", "M");

    m.destination ("org.example.Other");
    CHECK (m.destination() == "org.example.Other");
    m.destination ("");                          // clears destination
    CHECK (m.destination() == "");

    m.path ("/org/example/Other");
    CHECK (m.path() == "/org/example/Other");
    m.path ("");
    CHECK (m.path() == "");

    m.interface ("org.example.OtherIface");
    CHECK (m.interface() == "org.example.OtherIface");
    m.interface ("");
    CHECK (m.interface() == "");

    m.name ("OtherMethod");
    CHECK (m.name() == "OtherMethod");
    m.name ("");
    CHECK (m.name() == "");
}


//------------------------------------------------------------------------------
// error_name()/error_msg() setters, and their being no-ops on a
// non-error message.
//------------------------------------------------------------------------------
static void test_error_accessors ()
{
    section ("Accessors: error_name/error_msg");

    message mc ("org.example.Dest", "/p", "org.example.I", "M");
    dbus_message_set_serial (mc.handle(), 1);
    message err = mc.make_reply (true, "org.example.Err", "original");
    CHECK (err.error_name() == "org.example.Err");

    err.error_name ("org.example.Changed");
    CHECK (err.error_name() == "org.example.Changed");

    // error_name() setter is a no-op on a non-error message.
    message ret = mc.make_reply (false);
    ret.error_name ("org.example.ShouldBeIgnored");
    CHECK (ret.error_name() == "");
}


//------------------------------------------------------------------------------
// sender(), serial(), reply_serial() and want_reply().
//------------------------------------------------------------------------------
static void test_sender_serial_want_reply ()
{
    section ("sender()/serial()/reply_serial()/want_reply()");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    CHECK (m.sender() == "");                    // no sender set on a locally built message
    CHECK (m.serial() == 0);
    CHECK (m.reply_serial() == 0);

    dbus_message_set_serial (m.handle(), 77);
    CHECK (m.serial() == 77);

    message reply = m.make_reply (false);
    CHECK (reply.reply_serial() == 77);
    CHECK (reply.serial() == 0);                 // reply itself not yet "sent"

    CHECK (m.want_reply());                      // default: a reply is expected
    m.want_reply (true);
    CHECK (m.want_reply());
    m.want_reply (false);
    CHECK (!m.want_reply());

    // want_reply() on an invalid message is a harmless no-op / false.
    message inv (nullptr);
    CHECK (!inv.want_reply());
    CHECK_NOTHROW (inv.want_reply(true));
}


//------------------------------------------------------------------------------
// signature(): empty for a message without arguments; reflects appended
// argument types.
//------------------------------------------------------------------------------
static void test_signature ()
{
    section ("signature()");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    CHECK (m.signature() == "");

    m.append_args (int32_t(1), std::string("s"), true, 3.5);
    CHECK (m.signature() == "isbd");
}


//------------------------------------------------------------------------------
// append_args(): basic C++ types, const char*, and multiple arguments in one
// call.
//------------------------------------------------------------------------------
static void test_append_args_basic_types ()
{
    section ("append_args(): basic types");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    m.append_args (true,
                  uint8_t(200),
                  int16_t(-1000),
                  uint16_t(2000),
                  int32_t(-100000),
                  uint32_t(100000),
                  int64_t(-1000000000LL),
                  uint64_t(1000000000ULL),
                  3.25,
                  std::string("std-string"),
                  "literal");
    CHECK (m.signature() == "bynqiuxtdss");

    bool b{}; uint8_t y{}; int16_t n{}; uint16_t q{}; int32_t i{}; uint32_t u{};
    int64_t x{}; uint64_t t{}; double d{}; std::string s1; std::string s2;
    bool ok = m.get_args (b, y, n, q, i, u, x, t, d, s1, s2);
    CHECK (ok);
    CHECK (b == true);
    CHECK (y == 200);
    CHECK (n == -1000);
    CHECK (q == 2000);
    CHECK (i == -100000);
    CHECK (u == 100000);
    CHECK (x == -1000000000LL);
    CHECK (t == 1000000000ULL);
    CHECK (d == 3.25);
    CHECK (s1 == "std-string");
    CHECK (s2 == "literal");
}


//------------------------------------------------------------------------------
// append_args(): dbus_type arguments (basic, struct, array, dict, variant).
//------------------------------------------------------------------------------
static void test_append_args_dbus_types ()
{
    section ("append_args(): dbus_type arguments");

    // Basic dbus_type.
    {
        message m ("org.example.Dest", "/p", "org.example.I", "M");
        m.append_args (dbus_i32(7));
        CHECK (m.signature() == "i");
        auto args = m.arguments ();
        CHECK (args.size() == 1);
        CHECK (args[0]->cast<dbus_i32>().get() == 7);
    }

    // Struct.
    {
        dbus_struct s ("is");
        s.get<dbus_i32>(0) = 1;
        s.get<dbus_string>(1) = "x";

        message m ("org.example.Dest", "/p", "org.example.I", "M");
        m.append_args (s);
        CHECK (m.signature() == "(is)");
        auto args = m.arguments ();
        CHECK (args.size() == 1);
        CHECK (args[0]->is_struct());
        CHECK (args[0]->cast<dbus_struct>().get<dbus_i32>(0).get() == 1);
        CHECK (args[0]->cast<dbus_struct>().get<dbus_string>(1).get() == "x");
    }

    // Array.
    {
        dbus_array a ("i");
        a.push_back (1); a.push_back (2); a.push_back (3);

        message m ("org.example.Dest", "/p", "org.example.I", "M");
        m.append_args (a);
        CHECK (m.signature() == "ai");
        auto args = m.arguments ();
        CHECK (args.size() == 1);
        CHECK (args[0]->is_array());
        auto& arr = args[0]->cast<dbus_array>();
        CHECK (arr.size() == 3);
        CHECK (arr.at<dbus_i32>(0).get() == 1);
        CHECK (arr.at<dbus_i32>(2).get() == 3);
    }

    // Dict.
    {
        dbus_string_dict d;
        d.set (dbus_string("key1"), dbus_i32(11));
        d.set (dbus_string("key2"), dbus_i32(22));

        message m ("org.example.Dest", "/p", "org.example.I", "M");
        m.append_args (d);
        CHECK (m.signature() == "a{sv}");
        auto args = m.arguments ();
        CHECK (args.size() == 1);
        CHECK (args[0]->is_dict());
        auto& dict = args[0]->cast<dbus_string_dict>();
        CHECK (dict.size() == 2);
    }

    // Variant.
    {
        dbus_variant v (dbus_string("wrapped"));

        message m ("org.example.Dest", "/p", "org.example.I", "M");
        m.append_args (v);
        CHECK (m.signature() == "v");
        auto args = m.arguments ();
        CHECK (args.size() == 1);
        CHECK (args[0]->is_variant());
        CHECK (args[0]->cast<dbus_variant>().get().cast<dbus_string>().get() == "wrapped");
    }

    // Multiple mixed arguments in one call.
    {
        dbus_struct s ("i");
        s.get<dbus_i32>(0) = 9;
        message m ("org.example.Dest", "/p", "org.example.I", "M");
        m.append_args (dbus_i32(1), std::string("two"), s);
        CHECK (m.signature() == "is(i)");
        CHECK (m.arguments().size() == 3);
    }
}


//------------------------------------------------------------------------------
// get_args()/get_args_strict(): success, type mismatch, and missing
// argument cases, for both basic C++ types and dbus_type arguments.
//------------------------------------------------------------------------------
static void test_get_args ()
{
    section ("get_args()/get_args_strict()");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    m.append_args (int32_t(5), std::string("hi"));

    int i; std::string s;
    CHECK (m.get_args (i, s));
    CHECK (i == 5);
    CHECK (s == "hi");

    // Wrong type for the first argument.
    std::string wrong_first;
    int wrong_second;
    CHECK (!m.get_args (wrong_first, wrong_second));

    // Requesting more arguments than are present.
    int i2; std::string s2; bool extra;
    CHECK (!m.get_args (i2, s2, extra));

    // get_args with a dbus_type target.
    dbus_i32 di;
    dbus_string ds;
    CHECK (m.get_args (di, ds));
    CHECK (di.get() == 5);
    CHECK (ds.get() == "hi");

    // get_args_strict with matching dbus_type signatures.
    dbus_i32 di2;
    dbus_string ds2;
    CHECK (m.get_args_strict (di2, ds2));

    // get_args_strict with a dbus_array target against a non-array arg fails.
    message ma ("org.example.Dest", "/p", "org.example.I", "M");
    ma.append_args (int32_t(1));
    dbus_array arr_target ("i");
    CHECK (!ma.get_args_strict (arr_target));

    // A relaxed get_args() with a dbus_array target succeeds if the
    // message argument is also an array (regardless of element signature).
    dbus_array arr_src ("i");
    arr_src.push_back (1);
    message marr ("org.example.Dest", "/p", "org.example.I", "M");
    marr.append_args (arr_src);
    dbus_array arr_target2 ("i");
    CHECK (marr.get_args (arr_target2));
    CHECK (arr_target2.size() == 1);
}


//------------------------------------------------------------------------------
// arguments(): retrieving all arguments, or a limited number via max_args.
//------------------------------------------------------------------------------
static void test_arguments ()
{
    section ("arguments()");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    CHECK (m.arguments().empty());               // no arguments appended yet

    m.append_args (1, 2, 3, 4);
    auto all = m.arguments ();
    CHECK (all.size() == 4);
    for (int idx = 0; idx < 4; ++idx)
        CHECK (all[static_cast<size_t>(idx)]->cast<dbus_i32>().get() == idx + 1);

    auto limited = m.arguments (2);
    CHECK (limited.size() == 2);
    CHECK (limited[0]->cast<dbus_i32>().get() == 1);
    CHECK (limited[1]->cast<dbus_i32>().get() == 2);

    auto unlimited = m.arguments (0);            // 0 means "all"
    CHECK (unlimited.size() == 4);
}


//------------------------------------------------------------------------------
// handle(): mutable and const access to the underlying DBusMessage*.
//------------------------------------------------------------------------------
static void test_handle ()
{
    section ("handle()");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    CHECK (m.handle() != nullptr);

    const message& cm = m;
    CHECK (cm.handle() == m.handle());

    message inv (nullptr);
    CHECK (inv.handle() == nullptr);
}


//------------------------------------------------------------------------------
// to_json(): structure and content for the different message types.
//------------------------------------------------------------------------------
static void test_to_json ()
{
    section ("to_json()");

    message m ("org.example.Dest", "/p", "org.example.I", "M");
    m.append_args (int32_t(42));
    std::string json = m.to_json ();
    CHECK (json.find (R"json("type":"method_call")json") != std::string::npos);
    CHECK (json.find (R"json("destination":"org.example.Dest")json") != std::string::npos);
    CHECK (json.find (R"json("object_path":"/p")json") != std::string::npos);
    CHECK (json.find (R"json("interface":"org.example.I")json") != std::string::npos);
    CHECK (json.find (R"json("name":"M")json") != std::string::npos);
    CHECK (json.find (R"json("signature":"i")json") != std::string::npos);
    CHECK (json.find ("42") != std::string::npos);
    CHECK (json.find (R"json("reply_serial")json") == std::string::npos); // not a reply

    message sig ("/p", "org.example.I", "N");
    std::string sig_json = sig.to_json ();
    CHECK (sig_json.find (R"json("type":"signal")json") != std::string::npos);

    dbus_message_set_serial (m.handle(), 9);
    message reply = m.make_reply (false);
    std::string reply_json = reply.to_json ();
    CHECK (reply_json.find (R"json("type":"method_return")json") != std::string::npos);
    CHECK (reply_json.find (R"json("reply_serial":9)json") != std::string::npos);

    message err = m.make_reply (true, "org.example.Err", "desc");
    std::string err_json = err.to_json ();
    CHECK (err_json.find (R"json("type":"error")json") != std::string::npos);

    message inv (nullptr);
    CHECK (inv.to_json() == R"json({"type":"invalid"})json");
}


int main ()
{
    test_construction_method_call ();
    test_construction_signal ();
    test_construction_from_dbus_message_pointer ();
    test_construction_reply ();
    test_make_reply ();
    test_copy_move ();
    test_type_predicates ();
    test_accessors ();
    test_error_accessors ();
    test_sender_serial_want_reply ();
    test_signature ();
    test_append_args_basic_types ();
    test_append_args_dbus_types ();
    test_get_args ();
    test_arguments ();
    test_handle ();
    test_to_json ();

    std::cout << std::endl
               << g_num_checks << " checks run, "
               << g_num_failed << " failed." << std::endl;

    return g_num_failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
