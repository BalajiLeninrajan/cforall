//
// Cforall Version 1.0.0 Copyright (C) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in the
// file "LICENCE" distributed with Cforall.
//
// Assert.cpp --
//
// Author           : Peter A. Buhr
// Created On       : Thu Aug 18 13:26:59 2016
// Last Modified By : Peter A. Buhr
// Last Modified On : Mon Nov 20 22:57:18 2023
// Update Count     : 11
//

#include <cstdarg>  // for va_end, va_list, va_start
#include <cstdio>   // for fprintf, stderr, vfprintf
#include <cstdlib>  // for abort

#include "LSP/Lsp.hpp"  // for LSP::crash

extern const char * __progname;							// global name of running executable (argv[0])

#define CFA_ASSERT_FMT "*CFA assertion error* \"%s\" from program \"%s\" in \"%s\" at line %d in file \"%s\""

// called by macro assert in cassert
void __assert_fail( const char *assertion, const char *file, unsigned int line, const char *function ) {
	fprintf( stderr, CFA_ASSERT_FMT ".\n", assertion, __progname, function, line, file );
	char what[2048];
	snprintf( what, sizeof( what ), "assertion \"%s\" failed in %s (%s:%u)", assertion, function, file, line );
	LSP::crash( what );									// in LSP mode, writes the dump and exits
	abort();
}

// called by macro assertf
void __assert_fail_f( const char *assertion, const char *file, unsigned int line, const char *function, const char *fmt, ... ) {
	fprintf( stderr, CFA_ASSERT_FMT ": ", assertion, __progname, function, line, file );
	va_list args, copy;
	va_start( args, fmt );
	va_copy( copy, args );
	vfprintf( stderr, fmt, args );
	va_end( args );
	fprintf( stderr, "\n" );
	char what[2048];
	int len = snprintf( what, sizeof( what ), "assertion \"%s\" failed in %s (%s:%u): ", assertion, function, file, line );
	if ( len >= 0 && (size_t)len < sizeof( what ) ) vsnprintf( what + len, sizeof( what ) - len, fmt, copy );
	va_end( copy );
	LSP::crash( what );
	abort();
}

void abort(const char *fmt, ... ) noexcept __attribute__((noreturn, format(printf, 1, 2)));
void abort(const char *fmt, ... ) noexcept {
	va_list args, copy;
	va_start( args, fmt );
	va_copy( copy, args );
	vfprintf( stderr, fmt, args );
	va_end( args );
	fprintf( stderr, "\n" );
	char what[2048];
	vsnprintf( what, sizeof( what ), fmt, copy );
	va_end( copy );
	LSP::crash( what );
	abort();
}

// Local Variables: //
// tab-width: 4 //
// mode: c++ //
// compile-command: "make install" //
// End:  //
