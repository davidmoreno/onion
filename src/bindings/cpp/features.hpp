/*
	Onion HTTP server library
	Copyright (C) 2010-2016 David Moreno Montero and others

	This library is free software; you can redistribute it and/or
	modify it under the terms of, at your choice:
	
	a. the Apache License Version 2.0.
	
	b. the GNU General Public License as published by the 
		Free Software Foundation; either version 2.0 of the License
		or (at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of both licenses, if not see 
	<http://www.gnu.org/licenses/> and 
	<http://www.apache.org/licenses/LICENSE-2.0>.
	*/

#ifndef ONION_FEATURES_HPP
#define ONION_FEATURES_HPP

#if !defined(__clang__) && defined(__GNUC__)
// Check for < GCC 4.7, undefined for clarity
#define ONION_GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100)

#if ONION_GCC_VERSION < 40700
#undef ONION_HAS_LAMBDAS			// No lambdas
#undef ONION_HAS_NOEXCEPT			// No noexcept
#undef ONION_HAS_NULLPTR			// No nullptr
#undef ONION_HAS_RANGED_FOR			// No for(auto p : list)
#define ONION_HAS_BRACED_INIT 1		// We can do int { 0 }
#undef ONION_HAS_DEFAULT_FUNCS		// No Contructor() = default;
#undef ONION_HAS_DELETED_FUNCS		// No Constructor() = deleted;
#undef ONION_HAS_TEMPLATE_ALIAS		// No using type = othertype
#define ONION_HAS_RVALUE_REFS 1		// We can do I&& i
#define ONION_HAS_INIT_LIST 1		// We can do Clazz c { 0, 1, 2, 3, 4}
#else
// Greater than 4.7 has all the modern support we need
#define ONION_HAS_LAMBDAS 1
#define ONION_HAS_NOEXCEPT 1
#define ONION_HAS_NULLPTR 1
#define ONION_HAS_RANGED_FOR 1
#define ONION_HAS_BRACED_INIT 1
#define ONION_HAS_DEFAULT_FUNCS 1
#define ONION_HAS_DELETED_FUNCS 1
#define ONION_HAS_TEMPLATE_ALIAS 1
#define ONION_HAS_RVALUE_REFS 1
#define ONION_HAS_INIT_LIST 1
#endif

// End of GCC defines
#elif defined(__clang__)
// Any recent version of clang has more than enough support (> 2.9)
#define ONION_HAS_LAMBDAS 1
#define ONION_HAS_NOEXCEPT 1
#define ONION_HAS_NULLPTR 1
#define ONION_HAS_RANGED_FOR 1
#define ONION_HAS_BRACED_INIT 1
#define ONION_HAS_DEFAULT_FUNCS 1
#define ONION_HAS_DELETED_FUNCS 1
#define ONION_HAS_TEMPLATE_ALIAS 1
#define ONION_HAS_RVALUE_REFS 1
#define ONION_HAS_INIT_LIST 1
#endif

// Might as well approximate behavior for basic things
#ifndef ONION_HAS_NOEXCEPT
#define noexcept throw()
#endif

#ifndef ONION_HAS_NULLPTR
#define nullptr 0
#endif

#endif