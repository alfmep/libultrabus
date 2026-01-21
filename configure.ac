#
# Copyright (C) 2021-2023,2026 Dan Arrhenius <dan@ultramarin.se>
#
# This file is part of libultrabus.
#
# libultrabus is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published
# by the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
AC_INIT([libultrabus], [0.0.4], [ultrabus@ultramarin.se])
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2 subdir-objects])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_MKDIR_P
AC_PROG_INSTALL
AM_PROG_AR

LT_INIT()
LT_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])

#
# Release 0.0.4
#
# Library interface version x:y:z
#                           | | |
#                           | | +- AGE - how many previous versions of CURRENT this interface is compatible with.
#                           | |
#                           | +--- REVISION - Update this number when no interface change has been made.
#                           |
#                           +----- CURRENT - Update this number when an interface change has been made.
#
# Library version (CURRENT:REVISION:AGE)
#
LIBRARY_VERSION=0:4:0
AC_SUBST([LIBRARY_VERSION])

#
# Libraries to be listed in ultrabus.pc
#
LIBS_IN_PC_FILE=

#
# Required packages to be listed in ultrabus.pc
#
REQUIRE_IN_PC_FILE=

#
# Defines in the doxygen config file
#
PREDEFINED_IN_DOXYGEN=


#
# Check for dbus
#
PKG_CHECK_MODULES([dbus],
    [dbus-1],
    [export REQUIRE_IN_PC_FILE="$REQUIRE_IN_PC_FILE dbus-1"],
    [AC_MSG_ERROR(Could not find dbus)])
AC_SUBST([dbus_CFLAGS])
AC_SUBST([dbus_LIBS])


#
# Check for libiomultiplex
#
PKG_CHECK_MODULES([iomultiplex],
    [iomultiplex],
    [export REQUIRE_IN_PC_FILE="$REQUIRE_IN_PC_FILE iomultiplex"],
    [AC_MSG_ERROR(Could not find iomultiplex)])
AC_SUBST([iomultiplex_CFLAGS])
AC_SUBST([iomultiplex_LIBS])


#
# Give the user an option to build example applications
#
AC_ARG_ENABLE([examples],
	[AS_HELP_STRING([--enable-examples],
	                [build example applications. Example applications are not installed.])],,
	enable_examples=no)
AM_CONDITIONAL([ENABLE_EXAMPLES_SET], [test "x$enable_examples" != "xno"])



#
# All libraries are added
#
AC_SUBST([LIBS_IN_PC_FILE])
AC_SUBST([REQUIRE_IN_PC_FILE])
AC_SUBST([PREDEFINED_IN_DOXYGEN])


#
# Give the user an option to disable doxygen documentation
#
AC_ARG_ENABLE([doxygen],
	[AS_HELP_STRING([--disable-doxygen],
	[disable doxygen documentation [default=auto]])],,
	enable_doxygen=yes)
AM_CONDITIONAL([ENABLE_DOXYGEN_SET], [test "x$enable_doxygen" != "xno"])


AM_COND_IF([ENABLE_DOXYGEN_SET],
	[
	# Check for doxygen support
	#
	AC_CHECK_PROGS([DOXYGEN], [doxygen])
	if test -z "$DOXYGEN";
	   then AC_MSG_WARN([doxygen not found - continuing without doxygen support])
	fi
	AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
	AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([doc/doxygen.cfg])])
	],
	[
	# doxygen disabled
	#
	AM_CONDITIONAL([HAVE_DOXYGEN], false)
	]
)


AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/ultrabus.pc
	examples/Makefile
	doc/Makefile
])

AC_OUTPUT


#
# Print summary of the confiuration
#
AC_MSG_NOTICE([])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ Configuration summary:])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([ Installation prefix.................. ${prefix}])
AM_COND_IF([HAVE_DOXYGEN],
	[AC_MSG_NOTICE([ Generate doxygen documentation....... yes])],
	[AC_MSG_NOTICE([ Generate doxygen documentation....... no])]
)
AM_COND_IF([ENABLE_EXAMPLES_SET],
	[AC_MSG_NOTICE([ Build example applications........... yes (example applications are not installed)])],
	[AC_MSG_NOTICE([ Build example applications........... no])]
)
AC_MSG_NOTICE([])
AC_MSG_NOTICE([])
