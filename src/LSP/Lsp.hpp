//
// Cforall Version 1.0.0 Copyright (C) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in the
// file "LICENCE" distributed with Cforall.
//
// Lsp.hpp -- LSP dump mode (cfa-lsp): diagnostics capture and the JSON dump.
//
// The dump format is described in docs/dump-format.md of the cfa-lsp repository.
//

#pragma once

#include <string>
#include <vector>

#include "Common/CodeLocation.hpp"

class SemanticErrorException;

namespace ast {
	class TranslationUnit;
	class TypedefDecl;
	class TraitDecl;
	class TraitInstType;
	class DeclWithType;
	class Expr;
}

namespace LSP {

struct Options {
	std::string jsonOut;								// --lsp
	std::string cOut;									// --lsp-c-out
	std::vector<std::string> focus;						// --lsp-focus
	std::string input;									// the preprocessed input file
	bool stopAfterResolve = false;						// --lsp-stop-after-resolve
	std::vector<std::string> skipBodies;				// --lsp-skip-bodies
};

// True when --lsp was given. Diagnostics then go into the dump instead of stderr.
extern bool enabled;
extern Options options;
// True once the snapshot holds a cleanly resolved unit. Errors in later passes then leave the dump complete.
extern bool resolved;

void addDiagnostic( const CodeLocation & location, const char * severity, const std::string & text );
void addErrors( const SemanticErrorException & errors );
// A translator failure that is not a user error (unexpected exception).
void addInternalError( const std::string & text );
bool hasErrors();
// Called after a parse that had syntax errors. Translation goes on with what was parsed, so the dump has the
// declarations and references of the rest of the file, but later diagnostics are dropped: most would be about the
// code the parser skipped.
void syntaxErrorsFound();
bool syntaxErrors();

// Typedef and trait uses disappear before Resolve (typedefs are replaced by their
// base types, trait instances are expanded into assertions), so the passes
// that erase them record them here.
void recordTypedef( const ast::TypedefDecl * decl, bool global );
bool isRecordedTypedef( const ast::TypedefDecl * decl );
void recordTypedefUse( const CodeLocation & use, const ast::TypedefDecl * decl );
// A trait instance in an assertion, and the types named in its arguments.
void recordTraitUse( const ast::TraitInstType * inst );
// Exception declarations become plain structs, and vtable( E ) a vtable struct.
void recordException( const CodeLocation & location, const std::string & name );
void recordExceptionUse( const CodeLocation & use, const std::string & name );
// Hoist Struct renames nested aggregates to qualified names (__Outer__Inner); the dump shows the source name.
void recordRename( const std::string & newName, const std::string & oldName );
// The class of a type parameter as written (ast::TypeDecl::Kind, with Otype for a plain `T`), by location.
void recordTypeParam( const CodeLocation & location, int tyClass );
// An expression that resolution replaces with its type (sizeof( e ), typeof( e )); its names are still refs.
void recordResolvedExpr( const ast::Expr * expr );
// Enum and Pointer Decay turns array and function parameters into pointers; the dump shows them as written.
void recordParam( const ast::DeclWithType * param );

// True for a location in a file under one of the --lsp-skip-bodies directories that is not a focus file.
bool skipsBody( const CodeLocation & location );
// Empties the body of each function declared where skipsBody is true, keeping the body's location. Call before
// Resolve: the passes before it still see the bodies, and calls need only the declarations. The passes after it
// must not check an emptied body against what the function should do (Fix Init's constructor checks).
void skipBodies( ast::TranslationUnit & unit );

// Builds decls, refs, exprs and scopes from the resolved (or partially
// resolved) translation unit. Call at most once.
void snapshot( const ast::TranslationUnit & unit );

// Writes the JSON dump. Returns false if the file could not be written.
bool write( bool complete );

// Called when the translator is about to die: a failed assertion, abort or a fatal signal. Adds an internal error
// saying what happened, writes the dump with what was collected so far and exits with status 0. Returns without doing
// anything when LSP mode is off or a crash is already being handled, and returns after trying when the dump cannot be
// written; the caller then aborts as usual.
void crash( const char * what );

} // namespace LSP

// Local Variables: //
// tab-width: 4 //
// mode: c++ //
// End: //
