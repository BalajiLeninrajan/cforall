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
	class Expr;
}

namespace LSP {

struct Options {
	std::string jsonOut;								// --lsp
	std::string cOut;									// --lsp-c-out
	std::vector<std::string> focus;						// --lsp-focus
	std::string input;									// the preprocessed input file
};

// True when --lsp was given. Diagnostics then go into the dump instead of stderr.
extern bool enabled;
extern Options options;

void addDiagnostic( const CodeLocation & location, const char * severity, const std::string & text );
void addErrors( const SemanticErrorException & errors );
// A translator failure that is not a user error (unexpected exception).
void addInternalError( const std::string & text );
bool hasErrors();

// Typedef and trait uses disappear before Resolve (typedefs are replaced by their
// base types, trait instances are expanded into assertions), so the passes
// that erase them record them here.
void recordTypedef( const ast::TypedefDecl * decl, bool global );
bool isRecordedTypedef( const ast::TypedefDecl * decl );
void recordTypedefUse( const CodeLocation & use, const ast::TypedefDecl * decl );
void recordTraitUse( const CodeLocation & use, const ast::TraitDecl * decl );
// Exception declarations become plain structs, and vtable( E ) a vtable struct.
void recordException( const CodeLocation & location, const std::string & name );
void recordExceptionUse( const CodeLocation & use, const std::string & name );
// Hoist Struct renames nested aggregates to qualified names (__Outer__Inner); the dump shows the source name.
void recordRename( const std::string & newName, const std::string & oldName );
// The class of a type parameter as written (ast::TypeDecl::Kind, with Otype for a plain `T`), by location.
void recordTypeParam( const CodeLocation & location, int tyClass );
// An expression that resolution replaces with its type (sizeof( e ), typeof( e )); its names are still refs.
void recordResolvedExpr( const ast::Expr * expr );

// Builds decls, refs, exprs and scopes from the resolved (or partially
// resolved) translation unit. Call at most once.
void snapshot( const ast::TranslationUnit & unit );

// Writes the JSON dump. Returns false if the file could not be written.
bool write( bool complete );

} // namespace LSP

// Local Variables: //
// tab-width: 4 //
// mode: c++ //
// End: //
