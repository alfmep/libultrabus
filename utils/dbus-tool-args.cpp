/*
 * Copyright (C) 2021-2023 Dan Arrhenius <dan@ultramarin.se>
 *
 * This file is part of libultrabus.
 *
 * libultrabus is free software; you can redistribute it and/or modify it
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
#include <unistd.h>
#include <getopt.h>

#include "dbus-tool-args.hpp"

#ifdef __GLIBC__
#define PROGRAM_NAME program_invocation_short_name
#else
#include <stdlib.h>
#define PROGRAM_NAME getprogname()
#endif

using namespace std;

static constexpr const char* prog_name = "dbus-tool";


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void appargs_t::print_usage_and_exit (ostream& out, int exit_code)
{
    out << endl;
    out << "Usage: " << PROGRAM_NAME << " [OPTIONS] <command> [command argument ...]" << endl;
    out << endl;
    out << "Common options:" << endl;
    out << "  -y, --system                  Connect to the system bus instead of the session bus." << endl;
    out << "  -b, --bus=ADDRESS             Connect to a specific bus address. Ignoring parameter --system." << endl;
    out << "  -t, --timeout=MILLISECONDS    Set a specific timeout when waiting for message replies." << endl;
    out << "  -v, --version                 Print version and exit." << endl;
    out << "  -h, --help                    Print this help message and exit." << endl;
    out << endl;
    out << "Commands:" << endl;
    out << "  list" << endl;
    out << "      List the bus names(services) on this connection." << endl;
    out << "      Options:" << endl;
    out << "          -a, --all            Also include unique bus names." << endl;
    out << "          -x, --activatable    Instead of already connected names," << endl;
    out << "                               list all names that can be activated on the bus." << endl;
    out << endl;
    out << "  call <service> <object_path> <interface> <method> [signature argument ...]" << endl;
    out << "      Call a specific method on an object in a DBus service." << endl;
    out << "      Any returned argument is printed to standard output." << endl;
    out << "      Arguments to the method begins with a DBus signature," << endl;
    out << "      then the argument value." << endl;
    out << "      If there is only a single argument, the signature can be" << endl;
    out << "      omitted if the argument is a boolean(true|false), string, or" << endl;
    out << "      a signed integer." << endl;
    out << endl;
    out << "  introspect <service> [object_path]" << endl;
    out << "      Print introspect data for a specific object in a DBus service." << endl;
    out << "      If the object_path arguments is omitted, the root object \"/\" is used." << endl;
    out << "      Options:" << endl;
    out << "          -r, --raw    Don't parse the introspect data, print it \"as is\"." << endl;
    out << endl;
    out << "  get <service> <object_path> <interface> [property]" << endl;
    out << "      Get(and print) the property of an object in a DBus service." << endl;
    out << "      If argument 'property' is omitted, the names and values of all properties are printed to standard output." << endl;
    out << "      Options:" << endl;
    out << "          -s, --signature    Print the DBus signature of the properties." << endl;
    out << endl;
    out << "  set <service> <object_path> <interface> <property> [value_signature] <value>" << endl;
    out << "      Set the property of an object in a DBus service." << endl;
    out << "      The signature of the value can omitted if the value is a boolean(true|false)," << endl;
    out << "      string, or a signed integer." << endl;
    out << endl;
    out << "  objects <service> [object_path]" << endl;
    out << "      List all objects beloning to a specific service and object." << endl;
    out << "      If the object_path arguments is omitted, the root object \"/\" is used." << endl;
    out << endl;
    out << "  listen <service> <object_path> <interface> <signal>" << endl;
    out << "      Listen for a specific DBus signal." << endl;
    out << "      When a signal is received, is is printed to standard output." << endl;
    out << "      Stop listening and exit the program by pressing Ctrl-C." << endl;
    out << endl;
    out << "  start <service>" << endl;
    out << "      Try to launch the executable associated with a service name." << endl;
    out << "      Options:" << endl;
    out << "          -q, --quiet    Suppress output, exit with 0 on success and 1 on falure." << endl;
    out << endl;
    out << "  owner <service>" << endl;
    out << "      Print the unique bus name of the primary owner of the service name." << endl;
    out << endl;
    out << "  names <bus-name>" << endl;
    out << "      Print the names acquired by the bus connection." << endl;
    out << endl;
    out << "  ping <service>" << endl;
    out << "      Ping a service on the bus and print the response time in milliseconds." << endl;
    out << "      Options:" << endl;
    out << "          -q, --quiet    Suppress output, exit with 0 on success and 1 on falure." << endl;
    out << endl;
    out << "  monitor" << endl;
    out << "      Monitor messages on the message bus and display them on standard output." << endl;
    exit (exit_code);
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
appargs_t::appargs_t (int argc, char* argv[])
    : bus (DBUS_BUS_SESSION),
      timeout (DBUS_TIMEOUT_USE_DEFAULT),
      all (false),
      activatable (false),
      print_signature (false),
      quiet (false),
      raw (false)
{
    static struct option long_options[] = {
        { "system",      no_argument,       0, 'y'},
        { "bus",         required_argument, 0, 'b'},
        { "timeout",     required_argument, 0, 't'},
        { "all",         no_argument,       0, 'a'},
        { "activatable", no_argument,       0, 'x'},
        { "signature",   no_argument,       0, 's'},
        { "quiet",       no_argument,       0, 'q'},
        { "raw",         no_argument,       0, 'r'},
        { "version",     no_argument,       0, 'v'},
        { "help",        no_argument,       0, 'h'},
        { 0, 0, 0, 0}
    };
    static const char* arg_format = "yb:t:axsqrvh";
    bool be_quiet = false;

    while (true) {
        int c = getopt_long (argc, argv, arg_format, long_options, nullptr);
        if (c == -1)
            break;
        switch (c) {
        case 'y':
            bus = DBUS_BUS_SYSTEM;
            break;
        case 'a':
            all = true;
            break;
        case 'b':
            bus_address = std::string (optarg);
            break;
        case 't':
            timeout = atoi (optarg);
            if (timeout <= 0) {
                cerr << "Error: Invalid timeout argument" << endl;
                exit (1);
            }
            break;
        case 'x':
            activatable = true;
            break;
        case 's':
            print_signature = true;
            break;
        case 'q':
            be_quiet = true;
            break;
        case 'r':
            raw = true;
            break;
        case 'v': // --version
            std::cout << prog_name << ' ' << PACKAGE_VERSION << std::endl;
            exit (0);
            break;
        case 'h': // --help
            print_usage_and_exit (cout, 0);
            break;
        default:
            cerr << "Invalid option" << endl;
            print_usage_and_exit (cerr, 1);
            break;
        }
    }
    if (optind >= argc) {
        cerr << "Error: missing command (--help for help)" << endl;
        exit (1);
    }

    cmd = argv[optind++];

    if (cmd == "list") {
        ;
    }
    else if (cmd == "call") {
        if (optind > argc-4) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
        opath   = argv[optind++];
        iface   = argv[optind++];
        name    = argv[optind++]; // method name
        while (optind < argc)
            args.emplace_back (argv[optind++]);
    }
    else if (cmd == "introspect") {
        if (optind > argc-1) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
        if (optind < argc)
            opath = argv[optind++];
        else
            opath = "/";
    }
    else if (cmd == "get") {
        if (optind > argc-3) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
        opath   = argv[optind++];
        iface   = argv[optind++]; // property interface
        if (optind < argc)
            name = argv[optind++]; // property name
    }
    else if (cmd == "set") {
        if (optind > argc-5) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
        opath   = argv[optind++];
        iface   = argv[optind++]; // property interface
        name    = argv[optind++]; // property name
        args.emplace_back (argv[optind++]); // signature or value
        if (optind < argc)
            args.emplace_back (argv[optind++]); // value
    }
    else if (cmd == "objects") {
        if (optind > argc-1) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
        if (optind < argc)
            opath = argv[optind++];
        else
            opath = "/";
    }
    else if (cmd == "listen") {
        if (optind > argc-4) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
        opath   = argv[optind++];
        iface   = argv[optind++];
        name    = argv[optind++]; // signal name
    }
    else if (cmd == "start") {
        quiet = be_quiet;
        if (optind > argc-1) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
    }
    else if (cmd == "owner") {
        if (optind > argc-1) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
    }
    else if (cmd == "names") {
        if (optind > argc-1) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
    }
    else if (cmd == "ping") {
        quiet = be_quiet;
        if (optind > argc-1) {
            cerr << "Error: too few arguments (--help for help)" << endl;
            exit (1);
        }
        service = argv[optind++];
    }
    else if (cmd == "monitor") {
        ;
    }
    if (optind < argc) {
        cerr << "Error: too many arguments (--help for help)" << endl;
        exit (1);
    }
}
