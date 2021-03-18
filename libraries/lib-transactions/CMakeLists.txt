#[[
A simple RAII object TransactionScope.  Its dtor rolls back the project database
unless it is explicitly committed.

Also a facade that hides details of the implementation of the project database.

The main program can inject an implementation.  If this isn't done, then the
object has no effect.

Also part of the facade is an abstraction auto-saving of the state of the
project.
]]

set( SOURCES
   TransactionScope.cpp
   TransactionScope.h
)
set( LIBRARIES
   lib-exceptions-interface
   PRIVATE
   wxBase
)
audacity_library( lib-transactions "${SOURCES}" "${LIBRARIES}"
   ""
   ""
)
