#
# Copyright (C) 2021 Dan Arrhenius <dan@ultramarin.se>
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
AC_INIT([libultrabus], [0.0.0], [ultrabus@ultramarin.se])
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-bzip2 subdir-objects])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_MKDIR_P
AC_PROG_INSTALL
#AC_PROG_YACC
AM_PROG_AR
#AM_PROG_LEX

LT_INIT()
LT_LANG([C++])

AC_CONFIG_MACRO_DIR([m4])
#AC_CONFIG_HEADERS(src/include/ultrabus/config.h src/include/ultrabus/ultrabus_config.hpp)
AM_SILENT_RULES([yes])

#
# Release 0.0.0
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
LIBRARY_VERSION=0:0:0
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
# Give the user an option to not build utilities
#
AC_ARG_ENABLE([utils],
	[AS_HELP_STRING([--disable-utils], [don't build utilities])])
AM_CONDITIONAL([ENABLE_UTILS_SET], [test "x$enable_utils" != "xno"])

AM_COND_IF([ENABLE_UTILS_SET],
	[
	 # Check for libxml2, used to parse dbus introspect data
	 PKG_CHECK_MODULES([libxml2],
	     [libxml-2.0],
	     [AM_CONDITIONAL([HAVE_LIBXML2], true)],
	     [AC_MSG_WARN(Could not find libxml2 - dbus-tool will not format introspect output);
	      AM_CONDITIONAL([HAVE_LIBXML2], false)])
	 AC_SUBST([libxml2_CFLAGS])
	 AC_SUBST([libxml2_LIBS])
	],
	[AM_CONDITIONAL([HAVE_LIBXML2], false)]
)



#
# Give the user an option to not build example applications
#
AC_ARG_ENABLE([examples],
	[AS_HELP_STRING([--disable-examples], [don't build example applications])])
AM_CONDITIONAL([ENABLE_EXAMPLES_SET], [test "x$enable_examples" != "xno"])



#
# All libraries are added
#
AC_SUBST([LIBS_IN_PC_FILE])
AC_SUBST([REQUIRE_IN_PC_FILE])
AC_SUBST([PREDEFINED_IN_DOXYGEN])


#
# Give the user an option to build doxygen documentation
#
AC_ARG_ENABLE([doxygen],
	[AS_HELP_STRING([--enable-doxygen], [build doxygen documentation, if possible])],,
	enable_doxygen=no)
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


AC_OUTPUT([
	Makefile
	src/Makefile
	src/ultrabus.pc
	utils/Makefile
	examples/Makefile
	doc/Makefile
])


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
AM_COND_IF([ENABLE_UTILS_SET],
	[
	 AM_COND_IF([HAVE_LIBXML2],
	 	[AC_MSG_NOTICE([ Build utilities...................... yes])],
	 	[AC_MSG_NOTICE([ Build utilities...................... yes (libxml2 not found - dbus-tool will not format introspect output)])]
		)
	],
	[AC_MSG_NOTICE([ Build utilities...................... no])]
)
AM_COND_IF([ENABLE_EXAMPLES_SET],
	[AC_MSG_NOTICE([ Build example applications........... yes (test applications are not installed)])],
	[AC_MSG_NOTICE([ Build example applications........... no])]
)
AC_MSG_NOTICE([])
AC_MSG_NOTICE([])