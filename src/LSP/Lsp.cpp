//
// Cforall Version 1.0.0 Copyright (C) 2015 University of Waterloo
//
// The contents of this file are covered under the licence agreement in the
// file "LICENCE" distributed with Cforall.
//
// Lsp.cpp -- LSP dump mode (cfa-lsp): diagnostics capture and the JSON dump.
//
// The dump format is described in docs/dump-format.md of the cfa-lsp repository.
// In short: every position is (file, line, 0-based byte column in the
// preprocessed line), ends are exclusive, and declarations, references,
// expressions and scopes are taken from the AST right after Resolve.
//
// Generated code reuses the locations of the source it came from, so a node is
// accepted as a reference or declaration only if the preprocessed text at its
// range spells its name.
//

#include "LSP/Lsp.hpp"

#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>

#include "AST/Copy.hpp"
#include "AST/Decl.hpp"
#include "AST/Expr.hpp"
#include "AST/Init.hpp"
#include "AST/Pass.hpp"
#include "AST/Stmt.hpp"
#include "AST/TranslationUnit.hpp"
#include "AST/Type.hpp"
#include "CodeGen/GenType.hpp"
#include "Common/ErrorObjects.hpp"

using json = nlohmann::json;

namespace LSP {

bool enabled = false;
Options options;

namespace {

// ---------------------------------------------------------------------------
// Diagnostics

struct Diagnostic {
	CodeLocation location;
	std::string severity;
	std::string text;
};

std::vector<Diagnostic> diagnostics;
bool errorSeen = false;

// ---------------------------------------------------------------------------
// Uses recorded before Resolve erases them

std::vector<ast::ptr<ast::TypedefDecl>> typedefs;		// keeps the originals alive
std::unordered_set<const ast::TypedefDecl *> typedefSet;
std::unordered_set<const ast::TypedefDecl *> globalTypedefs;

// Only the name and location of the target are kept: holding a tree node would make passes copy it.
struct SideRef {
	CodeLocation location;							// the use
	int keyClass;									// see keyClass()
	std::string name;
	CodeLocation declLocation;
};
std::vector<SideRef> sideRefs;

std::map<std::string, CodeLocation> exceptions;		// by name
std::vector<std::pair<CodeLocation, std::string>> exceptionUses;

std::unordered_map<std::string, std::string> renames;	// qualified name of a nested aggregate -> source name
std::map<std::tuple<std::string, int, int>, int> typeParamClasses;	// (file, line, col) -> class as written
std::vector<ast::ptr<ast::Expr>> resolvedExprs;			// see recordResolvedExpr

// Array and function parameters as written, before Enum and Pointer Decay: (file, line, col) -> (type, declaration).
std::map<std::tuple<std::string, int, int>, std::pair<std::string, std::string>> writtenParams;

// ---------------------------------------------------------------------------
// The preprocessed input, by (file, line) as named by the line markers.

class SourceText {
	// A line has several pieces when cpp split it (see load()); columns restart in each piece.
	std::unordered_map<std::string, std::unordered_map<int, std::vector<std::string>>> lines;
	std::string firstFile;
  public:
	void load( const std::string & path ) {
		std::ifstream in( path, std::ios::binary );
		if ( ! in ) return;
		std::ostringstream buf;
		buf << in.rdbuf();
		const std::string text = buf.str();

		// cpp splits a line around some macro expansions (e.g., bool from stdbool.h) and continues it after a marker
		// repeating the line number. The continuation is another piece of the same line.
		std::string file;
		int line = 1;
		std::vector<std::string> * last = nullptr;		// the last line stored
		std::string lastFile;
		int lastLine = -1;
		bool continues = false;
		int depth = 0;
		size_t pos = 0;
		while ( pos < text.size() ) {
			size_t end = text.find( '\n', pos );
			if ( end == std::string::npos ) end = text.size();
			std::string_view cur( text.data() + pos, end - pos );
			std::string markerFile;
			int markerLine, flags;
			if ( parseMarker( cur, markerFile, markerLine, flags ) ) {
				file = markerFile;
				line = markerLine;
				continues = file == lastFile && line == lastLine;
				if ( flags & 1 ) depth += 1;			// entering an include
				if ( ( flags & 2 ) && depth > 0 ) depth -= 1;	// returning from one
			} else {
				// The main file: the first code outside any include and outside cpp's <built-in> and <command-line>.
				if ( firstFile.empty() && depth == 0 && ! file.empty() && file[0] != '<' ) firstFile = file;
				if ( continues ) {
					if ( last ) last->emplace_back( cur );
				} else {
					auto [it, added] = lines[file].emplace( line, std::vector<std::string>{ std::string( cur ) } );
					last = added ? &it->second : nullptr;	// keeps the first copy of a twice-included line
				} // if
				continues = false;
				lastFile = file;
				lastLine = line;
				line += 1;
			} // if
			pos = end + 1;
		} // while
	}

	// Same shape as the lexer's line-directive rule: # N "file" flags... `flags` has bit 0 set for flag 1 (entering
	// an include) and bit 1 for flag 2 (returning from one).
	static bool parseMarker( std::string_view s, std::string & file, int & line, int & flags ) {
		size_t i = 0;
		auto skipWhite = [&]() { while ( i < s.size() && ( s[i] == ' ' || s[i] == '\t' ) ) i += 1; };
		skipWhite();
		if ( i >= s.size() || s[i] != '#' ) return false;
		i += 1;
		skipWhite();
		if ( i >= s.size() || ! isdigit( (unsigned char)s[i] ) ) return false;
		long n = 0;
		while ( i < s.size() && isdigit( (unsigned char)s[i] ) ) { n = n * 10 + ( s[i] - '0' ); i += 1; }
		skipWhite();
		if ( i >= s.size() || s[i] != '"' ) return false;
		size_t close = s.find( '"', i + 1 );
		if ( close == std::string_view::npos || close == i + 1 ) return false;
		file = std::string( s.substr( i + 1, close - i - 1 ) );
		line = (int)n;
		flags = 0;
		for ( char c : s.substr( close + 1 ) ) {
			if ( c == '1' ) flags |= 1;
			else if ( c == '2' ) flags |= 2;
		} // for
		return true;
	}

	const std::vector<std::string> * line( const std::string & file, int line ) const {
		auto f = lines.find( file );
		if ( f == lines.end() ) return nullptr;
		auto l = f->second.find( line );
		return l == f->second.end() ? nullptr : &l->second;
	}

	bool hasFile( const std::string & file ) const { return lines.count( file ); }
	const std::string & first() const { return firstFile; }
};

SourceText source;

enum class Spelling { Yes, No, Unknown };

// The piece of the line at `loc` whose text at the range spells `name`, ignoring whitespace inside the range (so
// "?{ }" spells "?{}"). A split line has several pieces and the location does not say which one it is in.
const std::string * spellingPiece( const CodeLocation & loc, const std::string & name ) {
	if ( loc.isUnset() || loc.first_column < 0 || name.empty() ) return nullptr;
	if ( loc.last_line != loc.first_line || loc.last_column <= loc.first_column ) return nullptr;
	const std::vector<std::string> * pieces = source.line( loc.filename.str(), loc.first_line );
	if ( ! pieces ) return nullptr;
	for ( const std::string & text : *pieces ) {
		if ( (size_t)loc.last_column > text.size() ) continue;
		std::string word;
		for ( char c : text.substr( loc.first_column, loc.last_column - loc.first_column ) ) {
			if ( c != ' ' && c != '\t' ) word += c;
		} // for
		if ( word == name ) return &text;
	} // for
	return nullptr;
}

Spelling spells( const CodeLocation & loc, const std::string & name ) {
	if ( spellingPiece( loc, name ) ) return Spelling::Yes;
	if ( loc.isSet() && ! source.hasFile( loc.filename.str() ) ) return Spelling::Unknown;
	return Spelling::No;
}

// Is the name at `loc` preceded by "." or "->", ignoring blanks? The operator can end an earlier piece or line.
bool followsSelection( const CodeLocation & loc, const std::string & name ) {
	const std::string * text = spellingPiece( loc, name );
	if ( ! text ) return false;
	auto before = []( const std::string & s, int end ) -> int {	// index of the last non-blank before end, or -1
		int i = end - 1;
		while ( i >= 0 && ( s[i] == ' ' || s[i] == '\t' || s[i] == '\r' ) ) i -= 1;
		return i;
	};
	auto selects = []( const std::string & s, int i ) {
		return s[i] == '.' || ( s[i] == '>' && i > 0 && s[i - 1] == '-' );
	};
	int i = before( *text, loc.first_column );
	if ( i >= 0 ) return selects( *text, i );
	const std::string file = loc.filename.str();
	// Earlier pieces of the same line, then earlier lines; a few blank lines at most.
	const std::vector<std::string> * pieces = source.line( file, loc.first_line );
	int line = loc.first_line;
	size_t piece = pieces ? size_t( text - pieces->data() ) : 0;
	for ( int blank = 0; blank < 8; ) {
		if ( piece == 0 ) {
			line -= 1;
			pieces = line >= 1 ? source.line( file, line ) : nullptr;
			if ( line < 1 ) return false;
			if ( ! pieces || pieces->empty() ) {		// blank lines that cpp dropped
				blank += 1;
				continue;
			} // if
			piece = pieces->size();
		} // if
		piece -= 1;
		const std::string & s = (*pieces)[piece];
		int j = before( s, (int)s.size() );
		if ( j >= 0 ) return selects( s, j );
		blank += 1;
	} // for
	return false;
}

// The first non-blank character at or after (line, col), looking a few lines ahead. On a split line the first piece
// long enough is used.
bool nextNonBlank( const std::string & file, int line, int col, int & atLine, int & atCol, char & c ) {
	for ( int n = 0; n < 8; n += 1, line += 1, col = 0 ) {
		const std::vector<std::string> * pieces = source.line( file, line );
		if ( ! pieces ) continue;
		for ( const std::string & text : *pieces ) {
			if ( col < 0 || (size_t)col > text.size() ) continue;
			size_t i = text.find_first_not_of( " \t\r", col );
			if ( i == std::string::npos ) break;		// the rest of the line is blank
			atLine = line;
			atCol = (int)i;
			c = text[i];
			return true;
		} // for
	} // for
	return false;
}

const std::string opChars = "+-*/%<>=!&|^~\\";

// The one occurrence of the operator `symbol` between (fromLine, fromCol) and (toLine, toCol) that is not part of a
// longer operator. Within one line, the first piece of a split line with exactly one occurrence wins; across lines
// there must be exactly one in all.
bool findOperator( const std::string & file, int fromLine, int fromCol, int toLine, int toCol,
		const std::string & symbol, int & atLine, int & atCol ) {
	if ( fromLine < 1 || fromCol < 0 || toCol < 0 || toLine < fromLine || toLine - fromLine > 8 ) return false;
	if ( fromLine == toLine && toCol <= fromCol ) return false;
	int count = 0;
	for ( int line = fromLine; line <= toLine; line += 1 ) {
		const std::vector<std::string> * pieces = source.line( file, line );
		if ( ! pieces ) continue;
		for ( const std::string & text : *pieces ) {
			size_t from = line == fromLine ? fromCol : 0, to = line == toLine ? toCol : text.size();
			if ( to > text.size() || from > to ) continue;
			int found = -1, here = 0;
			for ( size_t at = text.find( symbol, from ); at != std::string::npos && at + symbol.size() <= to;
					at = text.find( symbol, at + 1 ) ) {
				bool alone = ( at == 0 || opChars.find( text[at - 1] ) == std::string::npos )
					&& ( at + symbol.size() >= text.size() || opChars.find( text[at + symbol.size()] ) == std::string::npos );
				if ( alone ) found = (int)at, here += 1;
			} // for
			if ( fromLine == toLine ) {
				if ( here != 1 ) continue;
				atLine = line;
				atCol = found;
				return true;
			} // if
			if ( here > 0 ) {
				count += here;
				atLine = line;
				atCol = found;
			} // if
		} // for
	} // for
	return fromLine != toLine && count == 1;
}

// ---------------------------------------------------------------------------
// JSON helpers

json rangeJson( const CodeLocation & loc, bool withFile ) {
	int line = loc.first_line, col = std::max( loc.first_column, 0 );
	int endLine = loc.last_line, endCol = loc.last_column;
	if ( loc.first_column < 0 ) {						// only the line is known: cover it
		const std::vector<std::string> * pieces = source.line( loc.filename.str(), line );
		endLine = line;
		endCol = 0;
		if ( pieces ) {
			for ( const std::string & text : *pieces ) endCol = std::max( endCol, (int)text.size() );
		} // if
	} else if ( endLine < line || ( endLine == line && endCol < col ) ) {
		endLine = line;
		endCol = col;
	} // if
	json j;
	if ( withFile ) j["file"] = loc.filename.str();
	j["line"] = line;
	j["col"] = col;
	j["endLine"] = endLine;
	j["endCol"] = endCol;
	return j;
}

void putRange( json & j, const CodeLocation & loc ) {
	json r = rangeJson( loc, true );
	for ( auto & item : r.items() ) j[item.key()] = item.value();
}

// CodeGen cannot print the forall clause of a function type outside C generation, so those are printed
// from a copy without it.
struct FindForall final : public ast::WithShortCircuiting {
	bool found = false;
	void previsit( const ast::FunctionType * type ) {
		if ( ! type->forall.empty() || ! type->assertions.empty() ) found = true;
	}
	void previsit( const ast::Expr * ) { visit_children = false; }
	void previsit( const ast::TypeExpr * ) {}
};

struct StripForall final {
	const ast::FunctionType * postvisit( const ast::FunctionType * type ) {
		if ( type->forall.empty() && type->assertions.empty() ) return type;
		ast::FunctionType * mut = ast::mutate( type );
		mut->forall.clear();
		mut->assertions.clear();
		return mut;
	}
};

bool startsWith( const std::string & s, const char * prefix ) {
	return s.compare( 0, strlen( prefix ), prefix ) == 0;
}

// The name the user wrote for a type name the translator made up:
// - `__anonymous3`, an anonymous aggregate, is "(anonymous)";
// - `__anonymous_Foo`, the anonymous aggregate in `typedef struct { ... } Foo`, is "Foo";
// - `__Outer__Inner`, a nested aggregate after Hoist Struct, is "Inner";
// - `__T_generic_`, a generic aggregate's parameter after Link Instance Types, is "T".
std::string sourceTypeName( const std::string & id ) {
	if ( ! startsWith( id, "__" ) ) return id;
	if ( startsWith( id, "__postfix_func_" ) ) return "?`" + id.substr( strlen( "__postfix_func_" ) );
	auto renamed = renames.find( id );
	if ( renamed != renames.end() ) return sourceTypeName( renamed->second );
	if ( startsWith( id, "__anonymous" ) ) {
		std::string rest = id.substr( strlen( "__anonymous" ) );
		if ( rest.find_first_not_of( "0123456789" ) == std::string::npos ) return "(anonymous)";
		// CodeGen's names for unnamed parameters are __anonymous_object<N>.
		if ( rest.size() > 1 && rest[0] == '_' && ! startsWith( rest, "_object" ) ) return rest.substr( 1 );
		return id;
	} // if
	const size_t suffix = strlen( "_generic_" );
	if ( id.size() > 2 + suffix && id.compare( id.size() - suffix, suffix, "_generic_" ) == 0 ) {
		return id.substr( 2, id.size() - 2 - suffix );
	} // if
	return id;
}

// The source name of a declaration whose name the translator changed (see sourceTypeName).
std::string sourceName( const ast::Decl * decl ) {
	if ( dynamic_cast<const ast::AggregateDecl *>( decl ) ) {
		auto renamed = renames.find( decl->name );
		if ( renamed != renames.end() ) return renamed->second;
	} else if ( dynamic_cast<const ast::TypeDecl *>( decl ) ) {
		return sourceTypeName( decl->name );
	} else if ( startsWith( decl->name, "__postfix_func_" ) ) {	// ?`name
		return "?`" + decl->name.substr( strlen( "__postfix_func_" ) );
	} // if
	return decl->name;
}

// CodeGen spells basic types the long way ("signed long int") and prints generated type names; show them as
// people write them.
std::string readableNames( const std::string & text ) {
	static const std::pair<const char *, const char *> names[] = {
		{ "unsigned long long int", "unsigned long long" }, { "signed long long int", "long long" },
		{ "unsigned short int", "unsigned short" }, { "signed short int", "short" },
		{ "unsigned long int", "unsigned long" }, { "signed long int", "long" },
		{ "signed int", "int" }, { "_Bool", "bool" },
	};
	auto isId = []( char c ) { return isalnum( (unsigned char)c ) || c == '_'; };
	std::string out = text;
	for ( auto [from, to] : names ) {
		size_t len = strlen( from );
		for ( size_t at = out.find( from ); at != std::string::npos; at = out.find( from, at ) ) {
			bool word = ( at == 0 || ! isId( out[at - 1] ) ) && ( at + len == out.size() || ! isId( out[at + len] ) );
			if ( word ) {
				out.replace( at, len, to );
				at += strlen( to );
			} else {
				at += len;
			} // if
		} // for
	} // for
	// Generated type names, one identifier at a time.
	std::string named;
	for ( size_t i = 0; i < out.size(); ) {
		if ( ( isalpha( (unsigned char)out[i] ) || out[i] == '_' ) && ( i == 0 || ! isId( out[i - 1] ) ) ) {
			size_t end = i;
			while ( end < out.size() && isId( out[end] ) ) end += 1;
			named += sourceTypeName( out.substr( i, end - i ) );
			i = end;
		} else {
			named += out[i];
			i += 1;
		} // if
	} // for
	return named;
}

std::string tidy( const std::string & text ) {
	std::string out;
	for ( char c : text ) {
		if ( c == '\n' ) c = ' ';
		if ( c == ' ' && ( out.empty() || out.back() == ' ' ) ) continue;
		// "(T , T )" from function types
		if ( ( c == ',' || c == ')' ) && ! out.empty() && out.back() == ' ' ) out.pop_back();
		out += c;
	}
	while ( ! out.empty() && out.back() == ' ' ) out.pop_back();
	return readableNames( out );
}

std::string typeText( const ast::Type * type, const std::string & name = "" ) {
	if ( ! type ) return name;
	static const ::Options pretty( true, false, false, false );
	ast::Pass<FindForall> find;
	type->accept( find );
	if ( find.core.found ) {
		ast::ptr<ast::Type> copy = ast::deepCopy( type );
		ast::Pass<StripForall> strip;
		copy = copy->accept( strip );
		return tidy( CodeGen::genType( copy, name, pretty ) );
	} // if
	return tidy( CodeGen::genType( type, name, pretty ) );
}

// The parser turns both `T` (otype) and `T *` into sized Dtypes, and Validate moves the otype's assertions
// (?{}, ^?{}, ?=?) onto the function. A sized parameter with a default-constructor assertion was an otype.
std::set<const ast::TypeDecl *> otypeParams;

void noteOtypes( const std::vector<ast::ptr<ast::TypeDecl>> & params,
		const std::vector<ast::ptr<ast::DeclWithType>> & assertions ) {
	for ( const ast::TypeDecl * param : params ) {
		for ( const ast::DeclWithType * assertion : param->assertions ) {
			if ( assertion->name == "?{}" ) otypeParams.insert( param );
		} // for
	} // for
	for ( const ast::DeclWithType * assertion : assertions ) {
		if ( assertion->name != "?{}" ) continue;
		const ast::Type * type = assertion->get_type();
		if ( auto ptr = dynamic_cast<const ast::PointerType *>( type ) ) type = ptr->base;	// after ForallPointerDecay
		auto ftype = dynamic_cast<const ast::FunctionType *>( type );
		if ( ! ftype || ftype->params.size() != 1 ) continue;
		auto ref = dynamic_cast<const ast::ReferenceType *>( ftype->params.front().get() );
		auto inst = ref ? dynamic_cast<const ast::TypeInstType *>( ref->base.get() ) : nullptr;
		if ( ! inst ) continue;
		for ( const ast::TypeDecl * param : params ) {
			if ( inst->base.get() == param || inst->name == param->name ) otypeParams.insert( param );
		} // for
	} // for
}

const char * typeParamSigil( const ast::TypeDecl * decl ) {
	auto written = typeParamClasses.find( { decl->location.filename.str(), decl->location.first_line,
		decl->location.first_column } );
	if ( written != typeParamClasses.end() ) {
		switch ( written->second ) {
		  case ast::TypeDecl::Dtype: return " &";
		  case ast::TypeDecl::DStype: return " *";
		  case ast::TypeDecl::Otype: return "";
		  case ast::TypeDecl::Ttype: return " ...";
		  default: return "";
		} // switch
	} // if
	switch ( decl->kind ) {
	  case ast::TypeDecl::Dtype:
		if ( ! decl->sized ) return " &";
		return otypeParams.count( decl ) ? "" : " *";
	  case ast::TypeDecl::DStype: return " *";
	  case ast::TypeDecl::Ttype: return " ...";
	  default: return "";
	} // switch
}

std::string forallText( const std::vector<ast::ptr<ast::TypeDecl>> & params ) {
	if ( params.empty() ) return "";
	std::string text = "forall( ";
	bool first = true;
	for ( const ast::TypeDecl * param : params ) {
		if ( param->name.empty() ) continue;			// placeholder for bare assertions
		if ( ! first ) text += ", ";
		first = false;
		std::string name = sourceName( param );
		text += param->kind == ast::TypeDecl::Dimension ? "[" + name + "]" : name + typeParamSigil( param );
	} // for
	return first ? "" : text + " ) ";
}

// Strips references, pointers, arrays and qualifiers down to a named aggregate.
const ast::Decl * typeDeclOf( const ast::Type * type ) {
	while ( type ) {
		if ( auto ref = dynamic_cast<const ast::ReferenceType *>( type ) ) {
			type = ref->base;
		} else if ( auto ptr = dynamic_cast<const ast::PointerType *>( type ) ) {
			type = ptr->base;
		} else if ( auto arr = dynamic_cast<const ast::ArrayType *>( type ) ) {
			type = arr->base;
		} else if ( auto inst = dynamic_cast<const ast::StructInstType *>( type ) ) {
			return inst->base.get();
		} else if ( auto inst = dynamic_cast<const ast::UnionInstType *>( type ) ) {
			return inst->base.get();
		} else if ( auto inst = dynamic_cast<const ast::EnumInstType *>( type ) ) {
			return inst->base.get();
		} else {
			return nullptr;
		} // if
	} // while
	return nullptr;
}

// Does a block start at `loc`: a brace, or a statement whose header declaration the translator hoisted into a
// block of its own? Desugaring makes other blocks at user locations.
bool blockStart( const CodeLocation & loc ) {
	const std::vector<std::string> * pieces = source.line( loc.filename.str(), loc.first_line );
	if ( ! pieces ) return true;
	static const std::set<std::string> keywords = { "for", "if", "while", "switch", "choose", "with", "mutex", "catch",
		"catchResume", "fixup", "waitfor", "waituntil", "do", "else", "try", "finally" };
	for ( const std::string & text : *pieces ) {
		if ( (size_t)loc.first_column >= text.size() ) continue;
		if ( text[loc.first_column] == '{' ) return true;
		size_t e = loc.first_column;
		while ( e < text.size() && ( isalnum( (unsigned char)text[e] ) || text[e] == '_' ) ) e += 1;
		if ( keywords.count( text.substr( loc.first_column, e - loc.first_column ) ) ) return true;
	} // for
	return false;
}

// Does the source at `loc` spell a use of `decl`? A postfix function ?`name is used as x`name.
bool spelledUse( const CodeLocation & loc, const ast::Decl * decl ) {
	std::string name = sourceName( decl );
	if ( spells( loc, name ) == Spelling::Yes ) return true;
	return startsWith( name, "?`" ) && spells( loc, name.substr( 2 ) ) == Spelling::Yes;
}

// " = value" for a parameter with a default argument (only constants can be defaults).
std::string defaultArgument( const ast::DeclWithType * param ) {
	auto obj = dynamic_cast<const ast::ObjectDecl *>( param );
	auto init = obj ? obj->init.as<ast::SingleInit>() : nullptr;
	if ( ! init ) return "";
	const ast::Expr * value = init->value;
	if ( auto cast = dynamic_cast<const ast::CastExpr *>( value ) ) value = cast->arg;
	auto constant = dynamic_cast<const ast::ConstantExpr *>( value );
	return constant && ! constant->rep.empty() ? " = " + constant->rep : "";
}

// The block the user wrote as a function's body. A generator main with suspends gets a new outer block located at
// the function's name, whose last statement is the original body.
const ast::CompoundStmt * userBody( const ast::FunctionDecl * func ) {
	const ast::CompoundStmt * body = func->stmts;
	while ( body && ! body->kids.empty() && body->location.first_line == func->location.first_line
			&& body->location.first_column == func->location.first_column ) {
		auto inner = body->kids.back().as<ast::CompoundStmt>();
		if ( ! inner ) break;
		body = inner;
	} // while
	return body;
}

const ast::Type * returnType( const ast::FunctionDecl * func ) {
	return func->returns.size() == 1 ? func->returns.front()->get_type() : nullptr;
}

// Decls are matched by name and location rather than by pointer: the pointer in a reference
// (VariableExpr::var, ...) can be an older version of the node in the tree.
using DeclKey = std::tuple<int, std::string, std::string, int, int>;

int keyClass( const ast::Decl * decl ) {
	if ( dynamic_cast<const ast::DeclWithType *>( decl ) ) return 0;
	if ( dynamic_cast<const ast::AggregateDecl *>( decl ) ) return 1;
	if ( dynamic_cast<const ast::TypedefDecl *>( decl ) ) return 2;
	return 3;
}

DeclKey keyOf( const ast::Decl * decl ) {
	return DeclKey( keyClass( decl ), decl->name, decl->location.filename.str(),
		decl->location.first_line, decl->location.first_column );
}

// Labels are not declarations in the tree; they get a key class of their own.
const int labelClass = 4;

DeclKey labelKey( const CodeLocation & loc, const std::string & name ) {
	return DeclKey( labelClass, name, loc.filename.str(), loc.first_line, loc.first_column );
}

using PlaceKey = std::tuple<std::string, int, int>;
using RangeKey = std::tuple<std::string, int, int, int, int>;

PlaceKey placeKey( const CodeLocation & loc ) {
	return PlaceKey( loc.filename.str(), loc.first_line, loc.first_column );
}

RangeKey rangeKey( const CodeLocation & loc ) {
	return RangeKey( loc.filename.str(), loc.first_line, loc.first_column, loc.last_line, loc.last_column );
}

// Records the named types in a type that a pass is about to erase (the arguments of a trait instance) as SideRefs.
struct SideTypeRefCore final : public ast::WithShortCircuiting {
	void use( const CodeLocation & loc, const ast::Decl * decl ) {
		if ( ! decl || loc.isUnset() || decl->location.isUnset() ) return;
		sideRefs.push_back( { loc, keyClass( decl ), decl->name, decl->location } );
	}

	void previsit( const ast::Expr * ) { visit_children = false; }
	void previsit( const ast::TypeExpr * ) {}
	void previsit( const ast::StructInstType * type ) { use( type->location, type->base.get() ); }
	void previsit( const ast::UnionInstType * type ) { use( type->location, type->base.get() ); }
	void previsit( const ast::EnumInstType * type ) { use( type->location, type->base.get() ); }
	void previsit( const ast::TypeInstType * type ) { use( type->location, type->base.get() ); }
};

// CodeGen asserts on expressions that only the resolver uses, so a type holding one is not printed.
struct Printable final : public ast::WithShortCircuiting {
	bool ok = true;
	void previsit( const ast::QualifiedNameExpr * ) { ok = false; }
	void previsit( const ast::ImplicitCopyCtorExpr * ) { ok = false; }
	void previsit( const ast::UntypedInitExpr * ) { ok = false; }
	void previsit( const ast::InitExpr * ) { ok = false; }
};

// ---------------------------------------------------------------------------
// The dump

struct Ref {
	CodeLocation location;
	const ast::Decl * decl;							// or null, and then key names the target
	DeclKey key;
	const char * role;
};

class Dumper {
  public:
	json decls = json::array();
	json refs = json::array();
	json exprs = json::array();
	json scopes = json::array();

	std::set<std::string> focus;
	std::map<DeclKey, int> ids;
	std::vector<Ref> pendingRefs;
	std::set<std::tuple<std::string, int, int, int, int>> exprSeen;
	std::set<PlaceKey> traitMembers;					// where the members of every trait are

	bool inFocus( const CodeLocation & loc ) const {
		return loc.isSet() && focus.count( loc.filename.str() );
	}

	bool isGenerated( const ast::Decl * decl ) const {
		if ( decl->linkage == ast::Linkage::AutoGen ) return true;
		return spells( decl->location, sourceName( decl ) ) == Spelling::No;
	}

	// Adds a declaration and returns its id, or -1 if it is not dumped.
	int addDecl( const ast::Decl * decl, const char * kind, int parent, bool local ) {
		if ( decl->name.empty() || decl->location.isUnset() ) return -1;
		DeclKey key = keyOf( decl );
		auto found = ids.find( key );
		if ( found != ids.end() ) {
			// A forward declaration made at the same place (exceptions get one) came first; the definition
			// has the real range and body.
			auto aggr = dynamic_cast<const ast::AggregateDecl *>( decl );
			json & j = decls[found->second];
			if ( aggr && aggr->body && aggr->bodyLocation.isSet() && j["body"].is_null() ) {
				if ( decl->extent.isSet() ) putRange( j, decl->extent );
				j["body"] = rangeJson( aggr->bodyLocation, false );
			} // if
			return found->second;
		} // if
		bool generated = isGenerated( decl );
		if ( generated && ! inFocus( decl->location ) ) return -1;

		int id = (int)decls.size();
		json j;
		j["id"] = id;
		j["name"] = sourceName( decl );
		j["kind"] = kind;
		putRange( j, decl->extent.isSet() ? decl->extent : decl->location );
		j["nameRange"] = rangeJson( decl->location, false );
		const std::pair<std::string, std::string> * written = strcmp( kind, "parameter" ) == 0 ? writtenParam( decl ) : nullptr;
		j["type"] = written ? written->first : declType( decl );
		j["signature"] = written ? written->second : signature( decl );
		j["parent"] = parent >= 0 ? json( parent ) : json( nullptr );
		j["typeDecl"] = nullptr;
		j["body"] = nullptr;
		if ( auto func = dynamic_cast<const ast::FunctionDecl *>( decl ) ) {
			json params = json::array();
			for ( const ast::DeclWithType * param : func->params ) params.push_back( param->name );
			j["params"] = params;
			const ast::CompoundStmt * body = userBody( func );
			if ( body && body->location.isSet() ) j["body"] = rangeJson( body->location, false );
		} else if ( auto aggr = dynamic_cast<const ast::AggregateDecl *>( decl ) ) {
			if ( aggr->bodyLocation.isSet() ) j["body"] = rangeJson( aggr->bodyLocation, false );
		} // if
		j["generated"] = generated;
		j["local"] = local;
		decls.push_back( std::move( j ) );
		ids.emplace( key, id );

		// typeDecl can refer to a later declaration, so it is resolved in finish().
		const ast::Type * type = nullptr;
		if ( auto func = dynamic_cast<const ast::FunctionDecl *>( decl ) ) {
			type = returnType( func );
		} else if ( auto dwt = dynamic_cast<const ast::DeclWithType *>( decl ) ) {
			type = dwt->get_type();
		} else if ( auto td = dynamic_cast<const ast::TypedefDecl *>( decl ) ) {
			type = td->base;
		} // if
		if ( const ast::Decl * target = typeDeclOf( type ) ) {
			typeDecls.emplace_back( id, target );
		} else if ( dynamic_cast<const ast::StructInstType *>( type ) || dynamic_cast<const ast::UnionInstType *>( type )
				|| dynamic_cast<const ast::EnumInstType *>( type ) ) {
			// Typedefs are recorded before Link Instance Types sets the base.
			typeDeclNames.emplace_back( id, static_cast<const ast::BaseInstType *>( type )->name );
		} // if
		return id;
	}

	void addRef( const CodeLocation & loc, const ast::Decl * decl, const char * role ) {
		if ( ! decl || ! inFocus( loc ) ) return;
		if ( ! spelledUse( loc, decl ) ) return;
		pendingRefs.push_back( { loc, decl, DeclKey(), role } );
	}

	// `loc` is the operator token, already checked.
	void addOperatorRef( const CodeLocation & loc, const ast::Decl * decl ) {
		if ( inFocus( loc ) ) pendingRefs.push_back( { loc, decl, DeclKey(), "call" } );
	}

	void addRef( const CodeLocation & loc, const DeclKey & key, const char * role ) {
		if ( ! inFocus( loc ) ) return;
		if ( spells( loc, sourceTypeName( std::get<1>( key ) ) ) != Spelling::Yes ) return;
		pendingRefs.push_back( { loc, nullptr, key, role } );
	}

	// A label, at its name in `L: stmt`. It is local to the function `parent`.
	int addLabel( const CodeLocation & loc, const std::string & name, int parent ) {
		DeclKey key = labelKey( loc, name );
		auto found = ids.find( key );
		if ( found != ids.end() ) return found->second;
		int id = (int)decls.size();
		json j;
		j["id"] = id;
		j["name"] = name;
		j["kind"] = "label";
		putRange( j, loc );
		j["nameRange"] = rangeJson( loc, false );
		j["type"] = "";
		j["signature"] = name + ":";
		j["parent"] = parent >= 0 ? json( parent ) : json( nullptr );
		j["typeDecl"] = nullptr;
		j["body"] = nullptr;
		j["generated"] = false;
		j["local"] = true;
		decls.push_back( std::move( j ) );
		ids.emplace( key, id );
		return id;
	}

	bool isTraitMember( const CodeLocation & loc ) const {
		return traitMembers.count( placeKey( loc ) );
	}

	// The type and declaration text of an array or function parameter as written.
	static const std::pair<std::string, std::string> * writtenParam( const ast::Decl * decl ) {
		if ( decl->location.isUnset() ) return nullptr;
		auto found = writtenParams.find( placeKey( decl->location ) );
		return found == writtenParams.end() ? nullptr : &found->second;
	}

	void addExpr( const CodeLocation & loc, const ast::Type * type ) {
		if ( ! type || ! inFocus( loc ) || loc.first_column < 0 ) return;
		std::string text = typeText( type );
		auto key = std::make_tuple( loc.filename.str(), loc.first_line, loc.first_column, loc.last_line, loc.last_column );
		if ( ! exprSeen.insert( key ).second ) return;
		json j;
		putRange( j, loc );
		j["type"] = text;
		const ast::Decl * target = typeDeclOf( type );
		exprTypeDecls.emplace_back( exprs.size(), target );
		j["typeDecl"] = nullptr;
		exprs.push_back( std::move( j ) );
	}

	// Resolves the references and typeDecl links once every declaration has an id.
	void finish() {
		std::set<std::tuple<std::string, int, int, int, int, int>> seen;
		for ( const Ref & ref : pendingRefs ) {
			int id = -1;
			if ( ref.decl ) {
				id = lookup( ref.decl );
			} else {
				auto found = ids.find( ref.key );
				if ( found != ids.end() ) id = found->second;
			} // if
			if ( id < 0 ) continue;
			const json & target = decls[id];
			if ( target["generated"].get<bool>() ) continue;
			// A declaration's own name is not a reference to it.
			if ( target["file"] == ref.location.filename.str() && target["nameRange"]["line"] == ref.location.first_line
					&& target["nameRange"]["col"] == ref.location.first_column ) continue;
			auto key = std::make_tuple( ref.location.filename.str(), ref.location.first_line, ref.location.first_column,
				ref.location.last_line, ref.location.last_column, id );
			if ( ! seen.insert( key ).second ) continue;
			json j;
			putRange( j, ref.location );
			j["decl"] = id;
			j["role"] = ref.role;
			refs.push_back( std::move( j ) );
		} // for
		for ( auto & [index, target] : exprTypeDecls ) {
			if ( ! target ) continue;
			int t = lookup( target );
			if ( t >= 0 ) exprs[index]["typeDecl"] = t;
		} // for
		// By index: lookup() can add declarations, and with them more typeDecl links.
		for ( size_t i = 0; i < typeDecls.size(); i += 1 ) {
			auto [id, target] = typeDecls[i];
			int t = lookup( target );
			if ( t >= 0 ) decls[id]["typeDecl"] = t;
		} // for
		if ( ! typeDeclNames.empty() ) {
			std::map<std::string, int> aggregates;		// the definition if there is one
			static const std::set<std::string> kinds = { "struct", "union", "enum", "coroutine", "monitor", "thread",
				"generator", "exception" };
			for ( const json & j : decls ) {
				if ( ! kinds.count( j["kind"].get<std::string>() ) ) continue;
				auto [it, fresh] = aggregates.emplace( j["name"].get<std::string>(), j["id"].get<int>() );
				if ( ! fresh && decls[it->second]["body"].is_null() && ! j["body"].is_null() ) it->second = j["id"];
			} // for
			for ( auto & [id, name] : typeDeclNames ) {
				auto it = aggregates.find( name );
				if ( it != aggregates.end() ) decls[id]["typeDecl"] = it->second;
			} // for
		} // if
	}

	// The id of a declaration, adding it if the traversal did not reach it.
	int lookup( const ast::Decl * decl ) {
		if ( ! decl || decl->location.isUnset() ) return -1;
		auto found = ids.find( keyOf( decl ) );
		if ( found != ids.end() ) return found->second;
		if ( isGenerated( decl ) ) return -1;
		return addDecl( decl, kindOf( decl, false ), -1, false );
	}

	static const char * kindOf( const ast::Decl * decl, bool param ) {
		if ( dynamic_cast<const ast::FunctionDecl *>( decl ) ) return "function";
		if ( param ) return "parameter";
		if ( auto obj = dynamic_cast<const ast::ObjectDecl *>( decl ) ) return obj->isMember ? "enumerator" : "variable";
		if ( auto aggr = dynamic_cast<const ast::StructDecl *>( decl ) ) {
			auto exc = exceptions.find( aggr->name );
			if ( exc != exceptions.end() && exc->second.first_line == aggr->location.first_line
					&& exc->second.first_column == aggr->location.first_column ) return "exception";
			switch ( aggr->kind ) {
			  case ast::AggregateDecl::Coroutine: return "coroutine";
			  case ast::AggregateDecl::Monitor: return "monitor";
			  case ast::AggregateDecl::Thread: return "thread";
			  case ast::AggregateDecl::Generator: return "generator";
			  case ast::AggregateDecl::Exception: return "exception";
			  default: return "struct";
			} // switch
		} // if
		if ( dynamic_cast<const ast::UnionDecl *>( decl ) ) return "union";
		if ( dynamic_cast<const ast::EnumDecl *>( decl ) ) return "enum";
		if ( dynamic_cast<const ast::TraitDecl *>( decl ) ) return "trait";
		if ( dynamic_cast<const ast::TypedefDecl *>( decl ) ) return "typedef";
		if ( dynamic_cast<const ast::TypeDecl *>( decl ) ) return "typeParam";
		return "variable";
	}

	static std::string declType( const ast::Decl * decl ) {
		if ( auto dwt = dynamic_cast<const ast::DeclWithType *>( decl ) ) return typeText( dwt->get_type() );
		if ( auto td = dynamic_cast<const ast::TypedefDecl *>( decl ) ) return typeText( td->base );
		return "";
	}

	static std::string signature( const ast::Decl * decl ) {
		if ( auto func = dynamic_cast<const ast::FunctionDecl *>( decl ) ) {
			noteOtypes( func->type_params, func->assertions );
			std::string text = forallText( func->type_params );
			if ( func->returns.empty() ) {
				text += "void";
			} else if ( func->returns.size() == 1 ) {
				text += typeText( func->returns.front()->get_type() );
			} else {
				text += "[ ";
				for ( size_t i = 0; i < func->returns.size(); i += 1 ) {
					if ( i ) text += ", ";
					text += typeText( func->returns[i]->get_type(), func->returns[i]->name );
				} // for
				text += " ]";
			} // if
			text += " " + sourceName( func ) + "(";
			for ( size_t i = 0; i < func->params.size(); i += 1 ) {
				text += i ? ", " : " ";
				if ( auto written = writtenParam( func->params[i] ) ) {
					text += written->second;
				} else {
					text += typeText( func->params[i]->get_type(), func->params[i]->name );
				} // if
				text += defaultArgument( func->params[i] );
			} // for
			if ( func->type && func->type->isVarArgs ) text += func->params.empty() ? " ..." : ", ...";
			text += func->params.empty() && ! ( func->type && func->type->isVarArgs ) ? ")" : " )";
			return text;
		} // if
		if ( auto dwt = dynamic_cast<const ast::DeclWithType *>( decl ) ) return typeText( dwt->get_type(), dwt->name );
		if ( auto aggr = dynamic_cast<const ast::AggregateDecl *>( decl ) ) {
			// The keyword as written: exceptions are structs by now, but kindOf remembers them.
			const char * kind = kindOf( decl, false );
			if ( auto trait = dynamic_cast<const ast::TraitDecl *>( decl ) ) {
				// A trait keeps no trace of the otype assertions; `forall( S )` is far more common than `forall( S * )`.
				for ( const ast::TypeDecl * param : trait->params ) otypeParams.insert( param );
			} // if
			return forallText( aggr->params ) + kind + " " + sourceTypeName( sourceName( aggr ) );
		} // if
		if ( auto td = dynamic_cast<const ast::TypedefDecl *>( decl ) ) {
			// typedef struct { ... } Foo: the struct is named after the typedef.
			if ( auto inst = dynamic_cast<const ast::BaseInstType *>( td->base.get() ) ) {
				if ( inst->name == "__anonymous_" + td->name ) {
					const char * kind = dynamic_cast<const ast::UnionInstType *>( inst ) ? "union"
						: dynamic_cast<const ast::EnumInstType *>( inst ) ? "enum" : "struct";
					return std::string( "typedef " ) + kind + " { ... } " + td->name;
				} // if
			} // if
			return "typedef " + typeText( td->base, td->name );
		} // if
		if ( auto td = dynamic_cast<const ast::TypeDecl *>( decl ) ) return sourceName( td ) + typeParamSigil( td );
		return decl->name;
	}

	std::vector<std::pair<int, const ast::Decl *>> typeDecls;
	std::vector<std::pair<int, std::string>> typeDeclNames;		// by aggregate name, for typedefs
	std::vector<std::pair<size_t, const ast::Decl *>> exprTypeDecls;
};

Dumper * dumper = nullptr;

// Records the named types in a type spelled in the source.
struct TypeRefCore final : public ast::WithShortCircuiting {
	Dumper & d;
	TypeRefCore( Dumper & d ) : d( d ) {}

	void previsit( const ast::Expr * ) { visit_children = false; }	// dimensions, assertions
	void previsit( const ast::TypeExpr * ) {}						// generic arguments
	void previsit( const ast::StructInstType * type ) { d.addRef( type->location, type->base.get(), "type" ); }
	void previsit( const ast::UnionInstType * type ) { d.addRef( type->location, type->base.get(), "type" ); }
	void previsit( const ast::EnumInstType * type ) { d.addRef( type->location, type->base.get(), "type" ); }
	void previsit( const ast::TraitInstType * type ) { d.addRef( type->location, type->base.get(), "type" ); }
	void previsit( const ast::TypeInstType * type ) { d.addRef( type->location, type->base.get(), "type" ); }
};

void typeRefs( Dumper & d, const ast::Type * type ) {
	if ( ! type ) return;
	ast::Pass<TypeRefCore> pass( d );
	type->accept( pass );
}

struct DumpCore final : public ast::WithShortCircuiting, public ast::WithGuards, public ast::WithVisitorRef<DumpCore> {
	Dumper & d;
	DumpCore( Dumper & d ) : d( d ) {}

	enum class Context { Global, Function, Aggregate, Enum, Trait };
	Context context = Context::Global;
	int parent = -1;
	int functionDepth = 0;

	// Parameters wait here for the function body's scope.
	const ast::CompoundStmt * paramBody = nullptr;
	// A block the translator wrapped around the function body (see userBody).
	const ast::CompoundStmt * wrapperBody = nullptr;
	std::vector<int> paramIds;
	std::unordered_set<const ast::Decl *> params;

	// A catch clause's declaration waits here for the clause body's scope.
	const ast::Decl * catchDecl = nullptr;
	const ast::Stmt * catchBody = nullptr;
	std::vector<int> catchIds;

	// Copies of assertions (from trait expansion), skipped.
	std::unordered_set<const ast::Decl *> skip;

	std::vector<int> scopeStack;						// indexes into d.scopes

	// Callees of applications, so their names are "call" references.
	std::unordered_set<const ast::Expr *> callees;

	// Inside the initializer of a generated local, whose blocks are not scopes the user wrote.
	bool inGenerated = false;

	// The labels of the function being walked, and the uses waiting for them (a goto can come before its label).
	const ast::FunctionDecl * labelOwner = nullptr;
	std::map<std::string, DeclKey> labels;
	std::vector<std::pair<CodeLocation, std::string>> labelUses;

	// The expressions of with clauses. A name found through with is a member of one of them.
	std::set<RangeKey> withExprs;

	bool local() const { return functionDepth > 0; }

	// Emits a declaration; false means do not visit its children.
	bool declare( const ast::Decl * decl, const char * kind, int & id ) {
		id = -1;
		if ( skip.count( decl ) ) return false;
		if ( decl->location.isUnset() ) return decl->linkage != ast::Linkage::AutoGen;
		// The parameters of a generic aggregate or a trait are only in scope inside it.
		bool isLocal = local() || ( dynamic_cast<const ast::TypeDecl *>( decl )
			&& ( context == Context::Aggregate || context == Context::Trait ) );
		if ( isLocal && ! d.inFocus( decl->location ) ) return false;
		if ( d.isGenerated( decl ) ) {
			if ( ! isLocal ) id = d.addDecl( decl, kind, parent, false );
			// Anonymous aggregates get generated names, but their members are real.
			return dynamic_cast<const ast::AggregateDecl *>( decl ) && decl->linkage != ast::Linkage::AutoGen;
		} // if
		id = d.addDecl( decl, kind, parent, isLocal );
		if ( id >= 0 && isLocal && context == Context::Function ) {
			if ( params.count( decl ) ) {
				paramIds.push_back( id );
			} else if ( decl == catchDecl ) {
				catchIds.push_back( id );
			} else if ( ! scopeStack.empty() ) {
				d.scopes[scopeStack.back()]["decls"].push_back( id );
			} // if
		} // if
		return true;
	}

	void enter( Context ctx, int id ) {
		GuardValue( context ) = ctx;
		GuardValue( parent ) = id >= 0 ? id : parent;
	}

	void previsit( const ast::FunctionDecl * decl ) {
		int id;
		bool walk = declare( decl, "function", id );
		// Desugaring moves user code into nested functions it makes (the body of corun). Their names are not
		// spelled in the source, but the code inside is the user's.
		bool generated = ! walk && local() && decl->stmts && decl->linkage != ast::Linkage::AutoGen
			&& ! skip.count( decl ) && d.inFocus( decl->location ) && d.isGenerated( decl );
		if ( ! ( walk || generated ) || ( ! d.inFocus( decl->location ) && ! local() ) ) {
			visit_children = false;
			return;
		} // if
		for ( const ast::DeclWithType * assertion : decl->assertions ) {
			skip.insert( assertion );
			assertionRefs( assertion );
		} // for
		for ( const ast::Expr * expr : decl->withExprs ) noteWith( expr );
		enter( Context::Function, id );
		GuardValue( functionDepth ) += 1;
		const ast::CompoundStmt * body = userBody( decl );
		if ( generated && body && ! blockStart( body->location ) ) body = nullptr;	// corun f( x ); has no braces
		GuardValue( paramBody ) = body;
		GuardValue( wrapperBody ) = body != decl->stmts.get() ? decl->stmts.get() : nullptr;
		GuardValue( paramIds ).clear();
		GuardValue( labelOwner ) = decl;
		GuardValue( labels ).clear();
		GuardValue( labelUses ).clear();
		for ( const ast::DeclWithType * param : decl->params ) params.insert( param );
		for ( const ast::DeclWithType * ret : decl->returns ) params.insert( ret );
	}

	// A goto, break or continue can name a label defined later in the function.
	void postvisit( const ast::FunctionDecl * decl ) {
		if ( decl != labelOwner ) return;
		for ( const auto & [loc, name] : labelUses ) {
			auto found = labels.find( name );
			if ( found != labels.end() ) d.addRef( loc, found->second, "read" );
		} // for
	}

	// The types in an assertion written in the source, as in forall( T | { int ?<?( T, T ); } ). Assertions copied
	// from a trait (forall( T | ord( T ) )) are located at the trait's members and are skipped.
	void assertionRefs( const ast::DeclWithType * assertion ) {
		if ( ! d.inFocus( assertion->location ) || d.isTraitMember( assertion->location ) ) return;
		typeRefs( d, assertion->get_type() );
	}

	// The dimensions of the arrays in a declared type, as in int a[N]. (A dimension the translator hoisted into a
	// generated local is walked there.) This visits other nodes with this pass, which leaves GuardValue pointing at
	// the finished guard of the last of them, so it is only called from a postvisit.
	void dimensionRefs( const ast::Type * type ) {
		while ( type ) {
			if ( auto array = dynamic_cast<const ast::ArrayType *>( type ) ) {
				if ( array->dimension ) array->dimension->accept( *visitor );
				type = array->base;
			} else if ( auto ptr = dynamic_cast<const ast::PointerType *>( type ) ) {	// a decayed array parameter
				if ( ptr->dimension ) ptr->dimension->accept( *visitor );
				type = ptr->base;
			} else if ( auto ref = dynamic_cast<const ast::ReferenceType *>( type ) ) {
				type = ref->base;
			} else {
				break;
			} // if
		} // while
	}

	void noteWith( const ast::Expr * expr ) {
		if ( expr && expr->location.isSet() ) withExprs.insert( rangeKey( expr->location ) );
	}

	void previsit( const ast::WithStmt * stmt ) {
		for ( const ast::Expr * expr : stmt->exprs ) noteWith( expr );
	}

	// Labels defined on a statement: L: stmt. The label's location starts at its name.
	void labelDecls( const ast::Stmt * stmt ) {
		if ( context != Context::Function ) return;
		for ( const ast::Label & label : stmt->labels ) {
			CodeLocation loc = label.location;
			if ( ! d.inFocus( loc ) || loc.first_column < 0 || label.name.empty() ) continue;
			loc.last_line = loc.first_line;
			loc.last_column = loc.first_column + (int)label.name.size();
			if ( spells( loc, label.name ) != Spelling::Yes ) continue;	// generated labels
			if ( d.addLabel( loc, label.name, parent ) >= 0 ) labels.emplace( label.name, labelKey( loc, label.name ) );
		} // for
	}

	void previsit( const ast::Stmt * stmt ) { labelDecls( stmt ); }

	// goto L, break L, continue L and fallthrough L. The statement starts at the keyword and the label follows it;
	// the branch's own target can be a label the translator made (multi-level exits become gotos).
	void previsit( const ast::BranchStmt * stmt ) {
		labelDecls( stmt );
		const CodeLocation & loc = stmt->location;
		if ( context != Context::Function || ! d.inFocus( loc ) || loc.first_column < 0 ) return;
		const std::vector<std::string> * pieces = source.line( loc.filename.str(), loc.first_line );
		if ( ! pieces ) return;
		static const std::set<std::string> keywords = { "goto", "break", "continue", "fallthrough", "fallthru" };
		auto isId = []( char c ) { return isalnum( (unsigned char)c ) || c == '_'; };
		for ( const std::string & text : *pieces ) {
			size_t at = loc.first_column, end = at;
			if ( at >= text.size() ) continue;
			while ( end < text.size() && isId( text[end] ) ) end += 1;
			if ( ! keywords.count( text.substr( at, end - at ) ) ) continue;
			at = text.find_first_not_of( " \t", end );
			if ( at == std::string::npos || ! ( isalpha( (unsigned char)text[at] ) || text[at] == '_' ) ) return;
			end = at;
			while ( end < text.size() && isId( text[end] ) ) end += 1;
			std::string name = text.substr( at, end - at );
			if ( name == "default" ) return;			// fallthrough default
			CodeLocation use = loc;
			use.last_line = use.first_line;
			use.first_column = (int)at;
			use.last_column = (int)end;
			labelUses.emplace_back( use, name );
			return;
		} // for
	}

	void previsit( const ast::ObjectDecl * decl ) {
		const char * kind = params.count( decl ) ? "parameter"
			: context == Context::Aggregate ? "field"
			: context == Context::Enum ? "enumerator"
			: "variable";
		if ( skip.count( decl ) ) {
			visit_children = false;
			return;
		} // if
		// Inside a function, return values are declared by the translator (_retval_f), but their types are the
		// ones written in the source.
		if ( local() || d.inFocus( decl->location ) ) typeRefs( d, decl->type );
		int id;
		if ( ! declare( decl, kind, id ) ) {
			// Desugaring moves user expressions into the initializers of generated locals (tuple assignment,
			// with on an rvalue, compound literals). Refs and exprs are only kept where the source spells the
			// name, so walking them lets the user's names through and nothing generated.
			visit_children = local() && d.inFocus( decl->location ) && d.isGenerated( decl );
			if ( visit_children ) GuardValue( inGenerated ) = true;
		} // if
	}

	void postvisit( const ast::ObjectDecl * decl ) {
		if ( ! skip.count( decl ) && ( local() || d.inFocus( decl->location ) ) ) dimensionRefs( decl->type );
	}

	void previsit( const ast::InlineMemberDecl * decl ) {
		int id;
		declare( decl, "field", id );
		visit_children = false;
	}

	template<typename AggrDecl>
	void aggregate( const AggrDecl * decl, const char * kind, Context ctx ) {
		int id;
		if ( ! declare( decl, kind, id ) ) {
			visit_children = false;
			return;
		} // if
		enter( ctx, id );
	}

	void previsit( const ast::StructDecl * decl ) { aggregate( decl, Dumper::kindOf( decl, false ), Context::Aggregate ); }
	void previsit( const ast::UnionDecl * decl ) { aggregate( decl, "union", Context::Aggregate ); }
	void previsit( const ast::EnumDecl * decl ) {
		aggregate( decl, "enum", Context::Enum );
		if ( d.inFocus( decl->location ) ) typeRefs( d, decl->base );
	}
	void previsit( const ast::TraitDecl * decl ) { aggregate( decl, "trait", Context::Trait ); }

	void previsit( const ast::TypedefDecl * decl ) {
		visit_children = false;
		if ( ! typedefSet.count( decl ) && ! isRecordedKey( decl ) ) return;	// implicit typedef of an aggregate
		int id;
		if ( declare( decl, "typedef", id ) && d.inFocus( decl->location ) ) typeRefs( d, decl->base );
	}

	void previsit( const ast::TypeDecl * decl ) {
		for ( const ast::DeclWithType * assertion : decl->assertions ) skip.insert( assertion );
		int id;
		if ( ! declare( decl, "typeParam", id ) ) {
			visit_children = false;
			return;
		} // if
		if ( d.inFocus( decl->location ) ) {
			typeRefs( d, decl->base );
			typeRefs( d, decl->init );
			for ( const ast::DeclWithType * assertion : decl->assertions ) assertionRefs( assertion );
		} // if
		visit_children = false;
	}

	void previsit( const ast::CompoundStmt * stmt ) {
		labelDecls( stmt );
		bool isBody = stmt == paramBody;
		if ( ! d.inFocus( stmt->location ) || stmt->location.first_column < 0 || inGenerated || stmt == wrapperBody
				|| ( ! isBody && ! blockStart( stmt->location ) ) ) {
			if ( isBody ) GuardValue( paramBody ) = nullptr;
			return;
		} // if
		json j;
		putRange( j, stmt->location );
		j["parent"] = scopeStack.empty() ? json( nullptr ) : json( scopeStack.back() );
		j["decls"] = json::array();
		if ( isBody ) {
			for ( int id : paramIds ) j["decls"].push_back( id );
			GuardValue( paramBody ) = nullptr;
		} // if
		if ( stmt == catchBody ) {
			for ( int id : catchIds ) j["decls"].push_back( id );
		} // if
		d.scopes.push_back( std::move( j ) );
		scopeStack.push_back( (int)d.scopes.size() - 1 );
		GuardAction( [this]() { scopeStack.pop_back(); } );
	}

	void previsit( const ast::CatchClause * clause ) {
		GuardValue( catchDecl ) = clause->decl.get();
		GuardValue( catchBody ) = clause->body.get();
		GuardValue( catchIds ).clear();
	}

	void previsit( const ast::ApplicationExpr * expr ) {
		callees.insert( expr->func.get() );
	}

	void postvisit( const ast::ApplicationExpr * expr ) {
		auto var = expr->func.as<ast::VariableExpr>();
		if ( ! var || ! var->var ) return;
		if ( spelledUse( var->location, var->var ) ) {
			d.addExpr( expr->location, expr->result );
		} else {
			operatorRef( expr, var->var );
		} // if
	}

	// An operator written as one (v + w, -x, i++) is a call of its function at the operator token. The
	// application covers the whole expression, so the token is looked for between the operands, which can be on
	// different lines.
	void operatorRef( const ast::ApplicationExpr * expr, const ast::Decl * decl ) {
		const std::string & name = decl->name;
		if ( name == "?[?]" ) return bracketRef( expr, decl, '[', ']' );
		if ( name == "?()" ) return bracketRef( expr, decl, '(', ')' );
		if ( name == "?{}" ) return bracketRef( expr, decl, '{', '}' );
		if ( name.size() < 2 || name.find( '?' ) == std::string::npos ) return;
		bool binary = name.size() >= 3 && name.front() == '?' && name.back() == '?';
		bool prefix = ! binary && name.back() == '?';
		std::string symbol = binary ? name.substr( 1, name.size() - 2 ) : prefix ? name.substr( 0, name.size() - 1 ) : name.substr( 1 );
		if ( symbol.empty() || symbol.find_first_not_of( opChars ) != std::string::npos ) return;	// ^?{}
		if ( expr->args.size() != ( binary ? 2u : 1u ) ) return;
		const CodeLocation & whole = expr->location, & first = expr->args.front()->location;
		const CodeLocation & last = expr->args.back()->location;
		if ( ! d.inFocus( whole ) || first.isUnset() || last.isUnset() ) return;
		int fromLine, fromCol, toLine, toCol;
		if ( binary ) {
			fromLine = first.last_line, fromCol = first.last_column, toLine = last.first_line, toCol = last.first_column;
		} else if ( prefix ) {
			fromLine = whole.first_line, fromCol = whole.first_column, toLine = first.first_line, toCol = first.first_column;
		} else {
			fromLine = first.last_line, fromCol = first.last_column, toLine = whole.last_line, toCol = whole.last_column;
		} // if
		int line, col;
		if ( ! findOperator( whole.filename.str(), fromLine, fromCol, toLine, toCol, symbol, line, col ) ) return;
		CodeLocation loc = whole;
		loc.first_line = loc.last_line = line;
		loc.first_column = col;
		loc.last_column = col + (int)symbol.size();
		d.addOperatorRef( loc, decl );
	}

	// ?[?], ?() and ?{} are written as a bracket after their first argument: a[i], f( x ), p{ 1 }, and in a
	// declaration Point p = { 1 }, where the first argument is the declared name. The ref is the opening bracket.
	// The next argument has to end after it, which rules out the translator's own calls at the declaration (the
	// loop that constructs each element of an array).
	void bracketRef( const ast::ApplicationExpr * expr, const ast::Decl * decl, char open, char close ) {
		if ( expr->args.empty() ) return;
		const CodeLocation & first = expr->args.front()->location;
		if ( ! d.inFocus( first ) || first.last_column < 0 ) return;
		const std::string file = first.filename.str();
		int line, col;
		char c;
		if ( ! nextNonBlank( file, first.last_line, first.last_column, line, col, c ) ) return;
		if ( c == '=' && open == '{' && ! nextNonBlank( file, line, col + 1, line, col, c ) ) return;
		if ( c != open ) return;
		if ( expr->args.size() > 1 ) {
			const CodeLocation & next = expr->args[1]->location;
			if ( next.isUnset() || next.filename.str() != file ) return;
			if ( std::make_pair( next.last_line, next.last_column ) <= std::make_pair( line, col ) ) return;
		} else {
			int closeLine, closeCol;
			char after;
			if ( ! nextNonBlank( file, line, col + 1, closeLine, closeCol, after ) || after != close ) return;
		} // if
		CodeLocation loc = first;
		loc.first_line = loc.last_line = line;
		loc.first_column = col;
		loc.last_column = col + 1;
		d.addOperatorRef( loc, decl );
	}

	void postvisit( const ast::VariableExpr * expr ) {
		if ( ! expr->var ) return;
		bool call = callees.count( expr );
		d.addRef( expr->location, expr->var.get(), call ? "call" : "read" );
		if ( ! call && spelledUse( expr->location, expr->var ) ) d.addExpr( expr->location, expr->result );
	}

	// A name found through a with clause becomes a member of the with clause's expression; in x.f and x->f the
	// aggregate is x. Conversions and anonymous members in between are skipped.
	bool foundThroughWith( const ast::MemberExpr * expr ) const {
		const ast::Expr * aggregate = expr->aggregate.get();
		while ( aggregate ) {
			if ( auto cast = dynamic_cast<const ast::CastExpr *>( aggregate ) ) {
				aggregate = cast->arg.get();
			} else if ( auto member = dynamic_cast<const ast::MemberExpr *>( aggregate ) ;
					member && member->member && spells( member->location, member->member->name ) != Spelling::Yes ) {
				aggregate = member->aggregate.get();
			} else {
				break;
			} // if
		} // while
		if ( ! aggregate || aggregate->location.isUnset() ) return ! followsSelection( expr->location, expr->member->name );
		return withExprs.count( rangeKey( aggregate->location ) );
	}

	void postvisit( const ast::MemberExpr * expr ) {
		if ( ! expr->member ) return;
		d.addRef( expr->location, expr->member.get(), foundThroughWith( expr ) ? "with" : "member" );
		if ( spells( expr->location, expr->member->name ) == Spelling::Yes ) d.addExpr( expr->location, expr->result );
	}

	void postvisit( const ast::CastExpr * expr ) {
		if ( expr->isGenerated != ast::ExplicitCast ) return;
		if ( d.inFocus( expr->location ) ) typeRefs( d, expr->result );
		d.addExpr( expr->location, expr->result );
	}

	void previsit( const ast::TypeExpr * expr ) {
		if ( d.inFocus( expr->location ) ) typeRefs( d, expr->type );
	}
	void previsit( const ast::SizeofExpr * expr ) {
		if ( d.inFocus( expr->location ) ) typeRefs( d, expr->type );
	}
	void previsit( const ast::AlignofExpr * expr ) {
		if ( d.inFocus( expr->location ) ) typeRefs( d, expr->type );
	}
	void previsit( const ast::CompoundLiteralExpr * expr ) {
		if ( d.inFocus( expr->location ) ) typeRefs( d, expr->result );
	}

	// Types are walked by TypeRefCore where they are spelled in the source; elsewhere (expression results,
	// function types) their locations are copies.
	void previsit( const ast::Type * ) { visit_children = false; }

	std::set<DeclKey> typedefKeys;
	bool typedefKeysReady = false;
	bool isRecordedKey( const ast::TypedefDecl * decl ) {
		if ( ! typedefKeysReady ) {
			for ( const ast::TypedefDecl * td : typedefs ) typedefKeys.insert( keyOf( td ) );
			typedefKeysReady = true;
		} // if
		return typedefKeys.count( keyOf( decl ) );
	}
};

} // namespace

// ---------------------------------------------------------------------------

void addDiagnostic( const CodeLocation & location, const char * severity, const std::string & text ) {
	if ( std::string( severity ) == "error" ) errorSeen = true;
	diagnostics.push_back( { location, severity, text } );
}

void addErrors( const SemanticErrorException & errors ) {
	for ( const error & err : errors.getErrors() ) {
		addDiagnostic( err.location, "error", err.description );
	} // for
}

void addInternalError( const std::string & text ) {
	addDiagnostic( CodeLocation(), "error", "internal translator error: " + text );
}

bool hasErrors() {
	return errorSeen;
}

void recordTypedef( const ast::TypedefDecl * decl, bool global ) {
	if ( typedefSet.insert( decl ).second ) typedefs.emplace_back( decl );
	if ( global ) globalTypedefs.insert( decl );
}

bool isRecordedTypedef( const ast::TypedefDecl * decl ) {
	return typedefSet.count( decl );
}

void recordTypedefUse( const CodeLocation & use, const ast::TypedefDecl * decl ) {
	sideRefs.push_back( { use, 2, decl->name, decl->location } );
}

void recordTraitUse( const ast::TraitInstType * inst ) {
	if ( ! inst || inst->location.isUnset() ) return;
	if ( inst->base ) sideRefs.push_back( { inst->location, 1, inst->base->name, inst->base->location } );
	ast::Pass<SideTypeRefCore> pass;
	for ( const ast::Expr * arg : inst->params ) arg->accept( pass );
}

void recordException( const CodeLocation & location, const std::string & name ) {
	exceptions[name] = location;
}

void recordExceptionUse( const CodeLocation & use, const std::string & name ) {
	exceptionUses.emplace_back( use, name );
}

void recordRename( const std::string & newName, const std::string & oldName ) {
	renames[newName] = oldName;
}

void recordTypeParam( const CodeLocation & location, int tyClass ) {
	if ( location.isSet() ) typeParamClasses[{ location.filename.str(), location.first_line, location.first_column }] = tyClass;
}

void recordResolvedExpr( const ast::Expr * expr ) {
	if ( expr ) resolvedExprs.emplace_back( expr );
}

void recordParam( const ast::DeclWithType * param ) {
	if ( ! param || param->location.isUnset() ) return;
	const ast::Type * type = param->get_type();
	if ( ! dynamic_cast<const ast::ArrayType *>( type ) && ! dynamic_cast<const ast::FunctionType *>( type ) ) return;
	ast::Pass<Printable> printable;
	type->accept( printable );
	if ( ! printable.core.ok ) return;
	writtenParams[placeKey( param->location )] = { typeText( type ), typeText( type, param->name ) };
}

static void loadSource() {
	static bool loaded = false;
	if ( loaded ) return;
	loaded = true;
	source.load( options.input );
}

// The index of the innermost scope containing `loc`, or -1. Scopes are in pre-order, so it is the last one.
static int innermostScope( const json & scopes, const CodeLocation & loc ) {
	int best = -1;
	std::pair<int, int> at( loc.first_line, loc.first_column );
	for ( size_t i = 0; i < scopes.size(); i += 1 ) {
		const json & s = scopes[i];
		if ( s["file"] != loc.filename.str() ) continue;
		std::pair<int, int> start( s["line"], s["col"] ), end( s["endLine"], s["endCol"] );
		if ( start <= at && at < end ) best = (int)i;
	} // for
	return best;
}

void snapshot( const ast::TranslationUnit & unit ) {
	if ( dumper ) return;
	loadSource();
	dumper = new Dumper;
	for ( const std::string & file : options.focus ) dumper->focus.insert( file );
	if ( dumper->focus.empty() && ! source.first().empty() ) dumper->focus.insert( source.first() );

	// Assertions copied from a trait keep the locations of the trait's members.
	for ( const ast::Decl * decl : unit.decls ) {
		if ( auto trait = dynamic_cast<const ast::TraitDecl *>( decl ) ) {
			for ( const ast::Decl * member : trait->members ) dumper->traitMembers.insert( placeKey( member->location ) );
		} // if
	} // for

	ast::Pass<DumpCore> pass( *dumper );
	for ( const ast::Decl * decl : unit.decls ) {
		decl->accept( pass );
	} // for
	for ( const ast::Expr * expr : resolvedExprs ) {
		expr->accept( pass );
	} // for

	// Typedefs are gone from the tree by now: global ones for every file (their uses are refs), local ones
	// for focus files.
	for ( const ast::TypedefDecl * decl : typedefs ) {
		if ( dumper->ids.count( keyOf( decl ) ) ) continue;
		bool global = globalTypedefs.count( decl );
		if ( global || dumper->inFocus( decl->location ) ) {
			int id = dumper->addDecl( decl, "typedef", -1, ! global );
			if ( id < 0 || ! dumper->inFocus( decl->location ) ) continue;
			if ( ! global ) {
				int scope = innermostScope( dumper->scopes, decl->location );
				if ( scope >= 0 ) dumper->scopes[scope]["decls"].push_back( id );
			} // if
			typeRefs( *dumper, decl->base );
		} // if
	} // for
	for ( const SideRef & ref : sideRefs ) {
		dumper->addRef( ref.location, DeclKey( ref.keyClass, ref.name, ref.declLocation.filename.str(),
			ref.declLocation.first_line, ref.declLocation.first_column ), "type" );
	} // for
	for ( auto & [use, name] : exceptionUses ) {
		auto exc = exceptions.find( name );
		if ( exc == exceptions.end() ) continue;
		dumper->addRef( use, DeclKey( 1, name, exc->second.filename.str(), exc->second.first_line,
			exc->second.first_column ), "type" );
	} // for
	dumper->finish();
}

// JSON strings must be valid UTF-8, but diagnostics can quote single bytes of a multi-byte character (the lexer
// reports unknown characters a byte at a time) or Latin-1 string literals. Bytes that are not part of a valid
// UTF-8 sequence are written as \xNN.
static std::string validUtf8( const std::string & text ) {
	std::string out;
	size_t i = 0;
	while ( i < text.size() ) {
		unsigned char c = text[i];
		size_t len = c < 0x80 ? 1 : ( c >> 5 ) == 0x6 ? 2 : ( c >> 4 ) == 0xe ? 3 : ( c >> 3 ) == 0x1e ? 4 : 0;
		bool ok = len > 0 && i + len <= text.size() && ! ( c == 0xc0 || c == 0xc1 || c > 0xf4 );
		for ( size_t k = 1; ok && k < len; k += 1 ) ok = ( (unsigned char)text[i + k] >> 6 ) == 0x2;
		if ( ok && len == 3 ) {
			unsigned char c1 = text[i + 1];
			ok = ! ( c == 0xe0 && c1 < 0xa0 ) && ! ( c == 0xed && c1 >= 0xa0 );	// overlong, surrogate
		} else if ( ok && len == 4 ) {
			unsigned char c1 = text[i + 1];
			ok = ! ( c == 0xf0 && c1 < 0x90 ) && ! ( c == 0xf4 && c1 >= 0x90 );	// overlong, above U+10FFFF
		} // if
		if ( ok ) {
			out.append( text, i, len );
			i += len;
		} else {
			static const char hex[] = "0123456789abcdef";
			out += "\\x";
			out += hex[c >> 4];
			out += hex[c & 0xf];
			i += 1;
		} // if
	} // while
	return out;
}

bool write( bool complete ) {
	loadSource();
	json doc;
	doc["format"] = 1;
	doc["complete"] = complete;

	std::string fallbackFile = options.focus.empty() ? source.first() : options.focus.front();
	json diags = json::array();
	// A statement that the translator copies (the range of a for loop, for one) reports its error once per copy.
	std::set<std::tuple<std::string, int, int, int, int, std::string, std::string>> seen;
	for ( const Diagnostic & diag : diagnostics ) {
		CodeLocation loc = diag.location;
		if ( loc.isUnset() || loc.filename.str().empty() ) {
			loc = CodeLocation( fallbackFile.c_str(), 1 );
			loc.first_column = loc.last_column = 0;
			loc.last_line = 1;
		} // if
		json j;
		putRange( j, loc );
		std::string text = validUtf8( diag.text );
		while ( ! text.empty() && ( text.back() == '\n' || text.back() == ' ' ) ) text.pop_back();
		auto key = std::make_tuple( j["file"].get<std::string>(), j["line"].get<int>(), j["col"].get<int>(),
			j["endLine"].get<int>(), j["endCol"].get<int>(), diag.severity, text );
		if ( ! seen.insert( std::move( key ) ).second ) continue;
		j["file"] = validUtf8( j["file"].get<std::string>() );
		j["severity"] = diag.severity;
		size_t newline = text.find( '\n' );
		j["message"] = text.substr( 0, newline );
		if ( newline != std::string::npos ) j["detail"] = text.substr( newline + 1 );
		diags.push_back( std::move( j ) );
	} // for
	doc["diagnostics"] = std::move( diags );

	if ( dumper ) {
		doc["decls"] = std::move( dumper->decls );
		doc["refs"] = std::move( dumper->refs );
		doc["exprs"] = std::move( dumper->exprs );
		doc["scopes"] = std::move( dumper->scopes );
	} else {
		doc["decls"] = json::array();
		doc["refs"] = json::array();
		doc["exprs"] = json::array();
		doc["scopes"] = json::array();
	} // if

	std::ofstream out( options.jsonOut, std::ios::binary | std::ios::trunc );
	if ( ! out ) return false;
	// Names and type text can still hold odd bytes (a Latin-1 file name or string literal); replace them
	// rather than throw.
	out << doc.dump( -1, ' ', false, json::error_handler_t::replace );
	out.close();
	return ! out.fail();
}

} // namespace LSP

// Local Variables: //
// tab-width: 4 //
// mode: c++ //
// End: //
