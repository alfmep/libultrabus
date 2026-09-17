# ultrabus

libultrabus is a C++20 library that wraps the low-level
`libdbus-1` C API (`dbus/dbus.h`) in type-safe, RAII-friendly C++ classes for
building DBus applications. It provides a DBus type system, a connection/event
loop abstraction, message filtering, and client/server object helpers, along
with thin wrappers for the standard `org.freedesktop.DBus.*` interfaces.


## Features

- Type-safe C++ representations of DBus wire types (`dbus_basic<T>`,
  `dbus_array`, `dbus_struct`, `dbus_dict<T>`, `dbus_variant`), all deriving
  from a common polymorphic `dbus_type` base.
- A `connection` class that owns the DBus bus connection and runs an internal
  epoll-based event loop thread, dispatching signals/replies to registered
  message filters and callbacks.
- `object_proxy` for calling methods and reading properties on remote objects,
  and `object_handler` for exporting an object path and handling incoming
  method calls.
- Wrappers for the standard `org.freedesktop.DBus.{Peer,Introspectable,
  Properties,ObjectManager}` interfaces.
- `retvalue<T>`, an error-carrying return type used throughout the library
  instead of exceptions for recoverable errors.

## Requirements

- CMake >= 3.25
- A C++20 compiler
- The `dbus-1` development package (resolved via pkg-config through
  `find_package (DBus1 REQUIRED)`)
- [Doxygen](https://www.doxygen.nl/) (optional, for building documentation)

## Building

```sh
cmake -B build -S .
cmake --build build -j$(nproc)
```

### CMake options

| Option | Default | Description |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `ON` | Build a shared library instead of a static one. |
| `BUILD_DOC` | `ON` | Build Doxygen documentation in `doc/` (if `doxygen` is found). |
| `BUILD_TESTING` | `OFF` | Build the test executables under `test/` and register them with CTest. |

## Installing

```sh
cmake --install build
```

This installs the library, the public headers (under `include/ultrabus/`,
plus the umbrella `include/ultrabus.hpp`), a CMake package config
(`lib/cmake/ultrabus/`), a pkg-config file (`lib/pkgconfig/ultrabus.pc`), and,
if `BUILD_DOC` generated it, the Doxygen HTML documentation under
`share/doc/ultrabus/html/`. Use `-DCMAKE_INSTALL_PREFIX=<dir>` at configure
time to change the install location.

Consume the installed library from another CMake project with:

```cmake
find_package (ultrabus REQUIRED)
target_link_libraries (myapp PRIVATE ultrabus::ultrabus)
```

or via pkg-config with `pkg-config --cflags --libs ultrabus`.

To remove installed files:

```sh
cmake --build build --target uninstall
```

## Documentation

API documentation is generated with Doxygen (from `doc/doxygen.cfg.in`) when
`BUILD_DOC` is enabled and `doxygen` is available on the system; the generated
docs are written to `doc/`.

## Usage

Include the umbrella header to pull in the full public API:

```cpp
#include <ultrabus.hpp>

using namespace ultrabus;
```

All library symbols live in the `ultrabus` namespace.

## License

This project is licensed under the GNU General Public License, version 2 or
(at your option) any later version. See the license headers in individual
source files for details.
