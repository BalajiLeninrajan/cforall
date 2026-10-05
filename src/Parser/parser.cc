/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 34 "Parser/parser.yy"

#define YYDEBUG_LEXER_TEXT( yylval )					// lexer loads this up each time
#define YYDEBUG 1										// get the pretty debugging code to compile
#define YYERROR_VERBOSE									// more information in syntax errors

#undef __GNUC_MINOR__

#include <cstdio>
#include <sstream>
#include <stack>
#include <vector>
using namespace std;

#include "DeclarationNode.hpp"                          // for DeclarationNode, ...
#include "ExpressionNode.hpp"                           // for ExpressionNode, ...
#include "InitializerNode.hpp"                          // for InitializerNode, ...
#include "ParserTypes.hpp"
#include "StatementNode.hpp"                            // for build_...
#include "TypedefTable.hpp"
#include "TypeData.hpp"
#include "AST/Type.hpp"                                 // for BasicType, BasicKind
#include "Common/SemanticError.hpp"                     // error_str
#include "Common/Utility.hpp"                           // for maybeMoveBuild, maybeBuild, CodeLo...
#include "Common/Iterate.hpp"							// for reverseIterate
#include "AST/Attribute.hpp"							// for Attribute
#include "AST/Print.hpp"								// for print
#include "LSP/Lsp.hpp"									// for LSP::enabled, LSP::addDiagnostic

// lex uses __null in a boolean context, it's fine.
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wparentheses-equality"
#endif

extern DeclarationNode * parseTree;
extern ast::Linkage::Spec linkage;
extern TypedefTable typedefTable;

stack<ast::Linkage::Spec> linkageStack;

bool appendStr( string & to, string & from ) {
	// 1. Multiple strings are concatenated into a single string but not combined internally. The reason is that
	//    "\x12" "3" is treated as 2 characters versus 1 because "escape sequences are converted into single members of
	//    the execution character set just prior to adjacent string literal concatenation" (C11, Section 6.4.5-8). It is
	//    easier to let the C compiler handle this case.
	//
	// 2. String encodings are transformed into canonical form (one encoding at start) so the encoding can be found
	//    without searching the string, e.g.: "abc" L"def" L"ghi" => L"abc" "def" "ghi". Multiple encodings must match,
	//    e.g., u"a" U"b" L"c" is disallowed.

	if ( from[0] != '"' ) {								// encoding ?
		if ( to[0] != '"' ) {							// encoding ?
			if ( to[0] != from[0] || to[1] != from[1] ) { // different encodings ?
				yyerror( "non-matching string encodings for string-literal concatenation" );
				return false;							// parse error, must call YYERROR in action
			} else if ( from[1] == '8' ) {
				from.erase( 0, 1 );						// remove 2nd encoding
			} // if
		} else {
			if ( from[1] == '8' ) {						// move encoding to start
				to = "u8" + to;
				from.erase( 0, 1 );						// remove 2nd encoding
			} else {
				to = from[0] + to;
			} // if
		} // if
		from.erase( 0, 1 );								// remove 2nd encoding
	} // if
	to += " " + from;									// concatenated into single string
	return true;
} // appendStr

// Source ranges for the LSP dump: nameLoc is the token naming a declaration or type, extent covers a whole
// declaration and bodyLoc the braces of an aggregate.

static DeclarationNode * setNameLoc( DeclarationNode * decl, const CodeLocation & loc ) {
	if ( decl ) decl->nameLoc = loc;
	return decl;
} // setNameLoc

static DeclarationNode * setExtent( DeclarationNode * decls, const CodeLocation & loc ) {
	for ( DeclarationNode * cur = decls ; cur ; cur = cur->next ) {
		if ( cur->extent.isUnset() ) cur->extent = loc;
	} // for
	return decls;
} // setExtent

static TypeData * setTypeNameLoc( TypeData * type, const CodeLocation & loc ) {
	if ( type ) type->nameLoc = loc;
	return type;
} // setTypeNameLoc

static DeclarationNode * setAggrLocs( DeclarationNode * decl, const CodeLocation & name, const CodeLocation & extent, const CodeLocation & body ) {
	if ( decl && decl->type ) {
		decl->type->nameLoc = name;
		decl->type->extent = extent;
		decl->type->bodyLoc = body;
	} // if
	return decl;
} // setAggrLocs

static CodeLocation span( const CodeLocation & first, const CodeLocation & last ) {
	CodeLocation loc = first;
	loc.last_line = last.last_line;
	loc.last_column = last.last_column;
	loc.last_pline = last.last_pline;
	return loc;
} // span

DeclarationNode * distTypeSpec( DeclarationNode * typeSpec, DeclarationNode * declList ) {
	// Distribute type specifier across all declared variables, e.g., static, const, __attribute__.
	assert( declList );

	// Do not distribute attributes for aggregates because the attributes surrounding the aggregate belong it not the
	// variables in the declaration list, e.g.,
	//
	//   struct __attribute__(( aligned(128) )) S { ...
	//   } v1 __attribute__(( aligned(64) )), v2 __attribute__(( aligned(32) )), v3;
	//   struct S v4;
	//
	// v1 => 64, v2 =>32, v3 => 128, v2 => 128
	//
	// Anonymous aggregates are a special case because there is no aggregate to bind the attribute to; hence it floats
	// to the declaration list.
	//
	//   struct __attribute__(( aligned(128) )) /*anonymous */ { ... } v1;
	//
	// v1 => 128

	bool copyattr = ! (typeSpec->type && typeSpec->type->kind == TypeData::Aggregate && ! typeSpec->type->aggregate.anon );

	// addType copies the type information for the aggregate instances from typeSpec into cl's aggInst.aggregate.
	DeclarationNode * cl = (new DeclarationNode)->addType( typeSpec ); // typeSpec IS DELETED!!!

	// Start at second variable in declaration list and clone the type specifiers for each variable.
	for ( DeclarationNode * cur = declList->next ; cur != nullptr; cur = cur->next ) {
		cl->cloneBaseType( cur, copyattr );				// cur is modified
	} // for

	// Add first variable in declaration list with hidden type information in aggInst.aggregate, which is used by
	// extractType to recover the type for the aggregate instances.
	declList->addType( cl, copyattr );					// cl IS DELETED!!!
	return declList;
} // distTypeSpec

void distAttr( DeclarationNode * attributes, DeclarationNode * declaration ) {
	// distribute attributes across all declaring list
	for ( DeclarationNode * attr = attributes; attr != nullptr ; attr = attr->next ) {
		for ( DeclarationNode * decl = declaration ; decl != nullptr ; decl = decl->next ) {
			decl->attributes.insert( decl->attributes.begin(), attr->attributes.begin(), attr->attributes.end() );
		} // for
	} // for
} // distAttr

void distExt( DeclarationNode * declaration ) {
	// distribute EXTENSION across all declarations
	for ( DeclarationNode * decl = declaration ; decl != nullptr ; decl = decl->next ) {
		decl->set_extension( true );
	} // for
} // distExt

void distInl( DeclarationNode * declaration ) {
	// distribute INLINE across all declarations
	for ( DeclarationNode * decl = declaration ; decl != nullptr ; decl = decl->next ) {
		decl->set_inLine( true );
	} // for
} // distInl

void distQual( DeclarationNode * declaration, DeclarationNode * qualifiers ) {
	// distribute qualifiers across all non-variable declarations in a distribution statemement
	for ( DeclarationNode * decl = declaration ; decl != nullptr ; decl = decl->next ) {
		// SKULLDUGGERY: Distributions are parsed inside out, so qualifiers are added to declarations inside out. Since
		// addQualifiers appends to the back of the list, the forall clauses are in the wrong order (right to left). To
		// get the qualifiers in the correct order and still use addQualifiers (otherwise, 90% of addQualifiers has to
		// be copied to add to front), the appropriate forall pointers are interchanged before calling addQualifiers.
		DeclarationNode * clone = qualifiers->clone();
		if ( qualifiers->type ) {						// forall clause ? (handles SC)
			if ( decl->type->kind == TypeData::Aggregate ) { // struct/union ?
				swap( clone->type->forall, decl->type->aggregate.params );
				decl->addQualifiers( clone );
			} else if ( decl->type->kind == TypeData::AggregateInst && decl->type->aggInst.aggregate->aggregate.body ) { // struct/union ?
				// Create temporary node to hold aggregate, call addQualifiers as above, then put nodes back together.
				DeclarationNode newnode;
				swap( newnode.type, decl->type->aggInst.aggregate );
				swap( clone->type->forall, newnode.type->aggregate.params );
				newnode.addQualifiers( clone );
				swap( newnode.type, decl->type->aggInst.aggregate );
			} else if ( decl->type->kind == TypeData::Function ) { // routines ?
				swap( clone->type->forall, decl->type->forall );
				decl->addQualifiers( clone );
			} // if
		} else {										// just SC qualifiers
			decl->addQualifiers( clone );
		} // if
	} // for
	delete qualifiers;
} // distQual

// There is an ambiguity for inline generic-routine return-types and generic routines.
//   forall( otype T ) struct S { int i; } bar( T ) {}
// Does the forall bind to the struct or the routine, and how would it be possible to explicitly specify the binding.
//   forall( otype T ) struct S { int T; } forall( otype W ) bar( W ) {}
// Currently, the forall is associated with the routine, and the generic type has to be separately defined:
//   forall( otype T ) struct S { int T; };
//   forall( otype W ) bar( W ) {}

void rebindForall( DeclarationNode * declSpec, DeclarationNode * funcDecl ) {
	if ( declSpec->type->kind == TypeData::Aggregate ) { // ignore aggregate definition
		funcDecl->type->forall = declSpec->type->aggregate.params; // move forall from aggregate to function type
		declSpec->type->aggregate.params = nullptr;
	} // if
} // rebindForall

string * build_postfix_name( string * name ) {
	*name = string("__postfix_func_") + *name;
	return name;
} // build_postfix_name

DeclarationNode * fieldDecl( DeclarationNode * typeSpec, DeclarationNode * fieldList ) {
	if ( fieldList == nullptr ) {
		if ( !( typeSpec->type && typeSpec->type->kind == TypeData::Aggregate ) ) { // int; no fieldList
			// printf( "fieldDecl1 typeSpec %p\n", typeSpec ); typeSpec->type->print( std::cout );
			SemanticWarning( yylloc, Warning::SuperfluousDecl );
			return nullptr;
		} // if
		// printf( "fieldDecl2 typeSpec %p\n", typeSpec ); typeSpec->type->print( std::cout );
		fieldList = DeclarationNode::newName( nullptr ); // struct S { ... } no fieldList
	} // if

	// printf( "fieldDecl3 typeSpec %p\n", typeSpec ); typeSpec->print( std::cout, 0 );
	DeclarationNode * temp = distTypeSpec( typeSpec, fieldList ); // mark all fields in list
	// printf( "fieldDecl4 temp %p\n", temp ); temp->print( std::cout, 0 );
	return temp;
} // fieldDecl

#define NEW_ZERO new ExpressionNode( build_constantInteger( yylloc, *new string( "0" ) ) )
#define NEW_ONE  new ExpressionNode( build_constantInteger( yylloc, *new string( "1" ) ) )
#define UPDOWN( compop, left, right ) (compop == OperKinds::LThan || compop == OperKinds::LEThan || compop == OperKinds::Neq ? left : right)
#define MISSING_ANON_FIELD "illegal syntax, missing loop fields with an anonymous loop index is meaningless as loop index is unavailable in loop body."
#define MISSING_LOW "illegal syntax, missing low value for ascanding range so index is uninitialized."
#define MISSING_HIGH "illegal syntax, missing high value for descending range so index is uninitialized."

static ForCtrl * makeForCtrl( const CodeLocation & location, DeclarationNode * init, OperKinds compop, ExpressionNode * comp, ExpressionNode * inc ) {
	// Wrap both comp/inc if they are non-null.
	if ( comp ) comp = new ExpressionNode( build_binary_val( location,
		compop,
		new ExpressionNode( build_varref( location, new string( *init->name ) ) ),
		comp ) );
	if ( inc ) inc = new ExpressionNode( build_binary_val( location,
		// choose += or -= for upto/downto
		UPDOWN( compop, OperKinds::PlusAssn, OperKinds::MinusAssn ),
		new ExpressionNode( build_varref( location, new string( *init->name ) ) ),
		inc ) );
	// The StatementNode call frees init->name, it must happen later.
	return new ForCtrl( new StatementNode( init ), comp, inc );
}

ForCtrl * forCtrl( const CodeLocation & location, DeclarationNode * index, ExpressionNode * start, OperKinds compop, ExpressionNode * comp, ExpressionNode * inc ) {
	if ( index->initializer ) {
		SemanticError( location, "illegal syntax, direct initialization disallowed. Use instead: type var; initialization ~ comparison ~ increment." );
	} // if
	if ( index->next ) {
		SemanticError( location, "illegal syntax, multiple loop indexes disallowed in for-loop declaration." );
	} // if
	DeclarationNode * initDecl = index->addInitializer( new InitializerNode( start ) );
	return makeForCtrl( location, initDecl, compop, comp, inc );
} // forCtrl

ForCtrl * forCtrl( const CodeLocation & location, ExpressionNode * type, string * index, ExpressionNode * start, OperKinds compop, ExpressionNode * comp, ExpressionNode * inc, const CodeLocation & indexLoc = CodeLocation() ) {
	ast::ConstantExpr * constant = dynamic_cast<ast::ConstantExpr *>(type->expr.get());
	if ( constant && (constant->rep == "0" || constant->rep == "1") ) {
		type = new ExpressionNode( new ast::CastExpr( location, maybeMoveBuild(type), new ast::BasicType( ast::BasicKind::SignedInt ) ) );
	} // if
	DeclarationNode * initDecl = distTypeSpec(
		DeclarationNode::newTypeof( type, true ),
		setNameLoc( DeclarationNode::newName( index ), indexLoc )->addInitializer( new InitializerNode( start ) )
	);
	return makeForCtrl( location, initDecl, compop, comp, inc );
} // forCtrl

#define MISSING_LOOP_INDEX "illegal syntax, only a single identifier or declaration allowed in initialization, e.g., for ( i; ... ) or for ( int i; ... ). Expression disallowed."

ForCtrl * forCtrl( const CodeLocation & location, ExpressionNode * type, ExpressionNode * index, ExpressionNode * start, OperKinds compop, ExpressionNode * comp, ExpressionNode * inc ) {
	if ( auto identifier = dynamic_cast<ast::NameExpr *>(index->expr.get()) ) {
		return forCtrl( location, type, new string( identifier->name ), start, compop, comp, inc, identifier->location );
	} else {
		SemanticError( location, MISSING_LOOP_INDEX ); return nullptr;
	} // if
} // forCtrl

ForCtrl * enumRangeCtrl( ExpressionNode * index_expr, OperKinds compop, ExpressionNode * range_over_expr, DeclarationNode * type ) {
	assert( compop == OperKinds::LEThan || compop == OperKinds::GEThan );
	if ( auto identifier = dynamic_cast<ast::NameExpr *>(index_expr->expr.get()) ) {
		DeclarationNode * indexDecl =
			setNameLoc( DeclarationNode::newName( new std::string(identifier->name) ), identifier->location )->addType( type );
		return new ForCtrl( new StatementNode( indexDecl ), range_over_expr, compop );
	} else {
		SemanticError( yylloc, MISSING_LOOP_INDEX ); return nullptr;
	} // if
} // enumRangeCtrl

// An error reported by the action of an invalid-syntax rule. In LSP mode it is recorded, and the action then uses
// YYERROR (or, in an error production, carries on) so the parser recovers as from any syntax error. Otherwise it
// ends translation.
static void syntaxError( const CodeLocation & location, const string & msg ) {
	if ( LSP::enabled ) {
		SemanticErrorThrow = true;
		LSP::addDiagnostic( location, "error", msg );
		return;
	} // if
	SemanticError( location, msg );
} // syntaxError

static void IdentifierBeforeIdentifier( string & identifier1, string & identifier2, const char * kind ) {
	syntaxError( yylloc, "illegal syntax, adjacent identifiers \"" + identifier1 + "\" and \"" + identifier2
				 + "\" are not meaningful in an " + kind + ".\n"
				 "Possible cause is misspelled type name or missing generic parameter." );
} // IdentifierBeforeIdentifier

static void IdentifierBeforeType( string & identifier, const char * kind ) {
	syntaxError( yylloc, "illegal syntax, identifier \"" + identifier + "\" cannot appear before a " + kind + ".\n"
				 "Possible cause is misspelled storage/CV qualifier, misspelled typename, or missing generic parameter." );
} // IdentifierBeforeType

bool forall = false;									// aggregate have one or more forall qualifiers ?

// Syntax error recovery, in LSP mode only: the error productions in statement_decl, statement_list_nodecl and
// external_definition let the parser carry on after a syntax error, so the rest of the file is still translated.
// Otherwise they end the parse at the first error, as before they existed. The code the parser skips may have
// opened typedef scopes without closing them. Blocks, switch bodies and external
// definitions record the scope depth at their start, and recovery goes back to the innermost one.
static vector<size_t> recoveryDepths;

static void enterRecoveryScope() {
	typedefTable.enterScope();
	recoveryDepths.push_back( typedefTable.depth() );
} // enterRecoveryScope

static void leaveRecoveryScope() {
	if ( ! recoveryDepths.empty() ) recoveryDepths.pop_back();
	typedefTable.leaveScope();
} // leaveRecoveryScope

static void recoverFromSyntaxError() {
	if ( ! recoveryDepths.empty() ) typedefTable.restoreDepth( recoveryDepths.back() );
	forall = false;
} // recoverFromSyntaxError

// Top-level definitions go into parseTree as soon as they are parsed, so the ones before a syntax error the parser
// cannot recover from are kept.
static DeclarationNode * parseTreeLast = nullptr;

static void addToParseTree( DeclarationNode * decls ) {
	if ( ! decls ) return;
	if ( parseTree ) {
		( parseTreeLast ? parseTreeLast : parseTree )->set_last( decls );
	} else {
		parseTree = decls;
	} // if
	parseTreeLast = decls->get_last();
} // addToParseTree

// https://www.gnu.org/software/bison/manual/bison.html#Location-Type
// Empty symbols (e.g., push, attribute_list_opt) sit at the end of the previous token, so they are skipped at both
// ends of a rule; otherwise a rule starting with one would begin at the previous token, possibly on an earlier line.
static inline bool emptyLoc( const CodeLocation & loc ) {
	return loc.first_line == loc.last_line && loc.first_column == loc.last_column;
}

#define YYLLOC_DEFAULT(Cur, Rhs, N)												\
if ( N ) {																		\
	int first_ = 1, last_ = N;													\
	while ( first_ < last_ && emptyLoc( YYRHSLOC( Rhs, first_ ) ) ) first_ += 1; \
	while ( last_ > first_ && emptyLoc( YYRHSLOC( Rhs, last_ ) ) ) last_ -= 1;	\
	(Cur).first_line   = YYRHSLOC( Rhs, first_ ).first_line;					\
	(Cur).first_column = YYRHSLOC( Rhs, first_ ).first_column;					\
	(Cur).last_line    = YYRHSLOC( Rhs, last_ ).last_line;						\
	(Cur).last_column  = YYRHSLOC( Rhs, last_ ).last_column;					\
	(Cur).filename     = YYRHSLOC( Rhs, first_ ).filename;						\
	(Cur).first_pline  = YYRHSLOC( Rhs, first_ ).first_pline;					\
	(Cur).last_pline   = YYRHSLOC( Rhs, last_ ).last_pline;						\
} else {																		\
	(Cur).first_line   = (Cur).last_line = YYRHSLOC( Rhs, 0 ).last_line;		\
	(Cur).first_column = (Cur).last_column = YYRHSLOC( Rhs, 0 ).last_column;	\
	(Cur).filename     = YYRHSLOC( Rhs, 0 ).filename;							\
	(Cur).first_pline  = (Cur).last_pline = YYRHSLOC( Rhs, 0 ).last_pline;		\
}

#line 459 "Parser/parser.cc"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

/* Use api.header.include to #include this header
   instead of duplicating it here.  */
#ifndef YY_YY_PARSER_PARSER_HH_INCLUDED
# define YY_YY_PARSER_PARSER_HH_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    TYPEDEF = 258,                 /* TYPEDEF  */
    EXTERN = 259,                  /* EXTERN  */
    STATIC = 260,                  /* STATIC  */
    AUTO = 261,                    /* AUTO  */
    REGISTER = 262,                /* REGISTER  */
    THREADLOCALGCC = 263,          /* THREADLOCALGCC  */
    THREADLOCALC11 = 264,          /* THREADLOCALC11  */
    INLINE = 265,                  /* INLINE  */
    FORTRAN = 266,                 /* FORTRAN  */
    NORETURN = 267,                /* NORETURN  */
    CONST = 268,                   /* CONST  */
    VOLATILE = 269,                /* VOLATILE  */
    RESTRICT = 270,                /* RESTRICT  */
    ATOMIC = 271,                  /* ATOMIC  */
    FORALL = 272,                  /* FORALL  */
    MUTEX = 273,                   /* MUTEX  */
    VIRTUAL = 274,                 /* VIRTUAL  */
    VTABLE = 275,                  /* VTABLE  */
    COERCE = 276,                  /* COERCE  */
    VOID = 277,                    /* VOID  */
    CHAR = 278,                    /* CHAR  */
    SHORT = 279,                   /* SHORT  */
    INT = 280,                     /* INT  */
    LONG = 281,                    /* LONG  */
    FLOAT = 282,                   /* FLOAT  */
    DOUBLE = 283,                  /* DOUBLE  */
    SIGNED = 284,                  /* SIGNED  */
    UNSIGNED = 285,                /* UNSIGNED  */
    BOOL = 286,                    /* BOOL  */
    COMPLEX = 287,                 /* COMPLEX  */
    IMAGINARY = 288,               /* IMAGINARY  */
    INT128 = 289,                  /* INT128  */
    UINT128 = 290,                 /* UINT128  */
    FLOAT80 = 291,                 /* FLOAT80  */
    uuFLOAT128 = 292,              /* uuFLOAT128  */
    FLOAT16 = 293,                 /* FLOAT16  */
    FLOAT32 = 294,                 /* FLOAT32  */
    FLOAT32X = 295,                /* FLOAT32X  */
    FLOAT64 = 296,                 /* FLOAT64  */
    FLOAT64X = 297,                /* FLOAT64X  */
    FLOAT128 = 298,                /* FLOAT128  */
    FLOAT128X = 299,               /* FLOAT128X  */
    FLOAT32X4 = 300,               /* FLOAT32X4  */
    FLOAT64X2 = 301,               /* FLOAT64X2  */
    SVFLOAT32 = 302,               /* SVFLOAT32  */
    SVFLOAT64 = 303,               /* SVFLOAT64  */
    SVBOOL = 304,                  /* SVBOOL  */
    DECIMAL32 = 305,               /* DECIMAL32  */
    DECIMAL64 = 306,               /* DECIMAL64  */
    DECIMAL128 = 307,              /* DECIMAL128  */
    ZERO_T = 308,                  /* ZERO_T  */
    ONE_T = 309,                   /* ONE_T  */
    SIZEOF = 310,                  /* SIZEOF  */
    TYPEOF = 311,                  /* TYPEOF  */
    VA_LIST = 312,                 /* VA_LIST  */
    VA_ARG = 313,                  /* VA_ARG  */
    AUTO_TYPE = 314,               /* AUTO_TYPE  */
    COUNTOF = 315,                 /* COUNTOF  */
    OFFSETOF = 316,                /* OFFSETOF  */
    BASETYPEOF = 317,              /* BASETYPEOF  */
    TYPEID = 318,                  /* TYPEID  */
    ENUM = 319,                    /* ENUM  */
    STRUCT = 320,                  /* STRUCT  */
    UNION = 321,                   /* UNION  */
    EXCEPTION = 322,               /* EXCEPTION  */
    GENERATOR = 323,               /* GENERATOR  */
    COROUTINE = 324,               /* COROUTINE  */
    MONITOR = 325,                 /* MONITOR  */
    THREAD = 326,                  /* THREAD  */
    OTYPE = 327,                   /* OTYPE  */
    FTYPE = 328,                   /* FTYPE  */
    DTYPE = 329,                   /* DTYPE  */
    TTYPE = 330,                   /* TTYPE  */
    TRAIT = 331,                   /* TRAIT  */
    LABEL = 332,                   /* LABEL  */
    SUSPEND = 333,                 /* SUSPEND  */
    ATTRIBUTE = 334,               /* ATTRIBUTE  */
    EXTENSION = 335,               /* EXTENSION  */
    IF = 336,                      /* IF  */
    ELSE = 337,                    /* ELSE  */
    SWITCH = 338,                  /* SWITCH  */
    CASE = 339,                    /* CASE  */
    DEFAULT = 340,                 /* DEFAULT  */
    DO = 341,                      /* DO  */
    WHILE = 342,                   /* WHILE  */
    FOR = 343,                     /* FOR  */
    BREAK = 344,                   /* BREAK  */
    CONTINUE = 345,                /* CONTINUE  */
    GOTO = 346,                    /* GOTO  */
    RETURN = 347,                  /* RETURN  */
    CHOOSE = 348,                  /* CHOOSE  */
    FALLTHROUGH = 349,             /* FALLTHROUGH  */
    WITH = 350,                    /* WITH  */
    WHEN = 351,                    /* WHEN  */
    WAITFOR = 352,                 /* WAITFOR  */
    WAITUNTIL = 353,               /* WAITUNTIL  */
    CORUN = 354,                   /* CORUN  */
    COFOR = 355,                   /* COFOR  */
    DISABLE = 356,                 /* DISABLE  */
    ENABLE = 357,                  /* ENABLE  */
    TRY = 358,                     /* TRY  */
    THROW = 359,                   /* THROW  */
    THROWRESUME = 360,             /* THROWRESUME  */
    AT = 361,                      /* AT  */
    ASM = 362,                     /* ASM  */
    ALIGNAS = 363,                 /* ALIGNAS  */
    ALIGNOF = 364,                 /* ALIGNOF  */
    __ALIGNOF = 365,               /* __ALIGNOF  */
    GENERIC = 366,                 /* GENERIC  */
    STATICASSERT = 367,            /* STATICASSERT  */
    IDENTIFIER = 368,              /* IDENTIFIER  */
    TYPEDIMname = 369,             /* TYPEDIMname  */
    TYPEDEFname = 370,             /* TYPEDEFname  */
    TYPEGENname = 371,             /* TYPEGENname  */
    TIMEOUT = 372,                 /* TIMEOUT  */
    WAND = 373,                    /* WAND  */
    WOR = 374,                     /* WOR  */
    CATCH = 375,                   /* CATCH  */
    RECOVER = 376,                 /* RECOVER  */
    CATCHRESUME = 377,             /* CATCHRESUME  */
    FIXUP = 378,                   /* FIXUP  */
    FINALLY = 379,                 /* FINALLY  */
    INTEGERconstant = 380,         /* INTEGERconstant  */
    CHARACTERconstant = 381,       /* CHARACTERconstant  */
    STRINGliteral = 382,           /* STRINGliteral  */
    DIRECTIVE = 383,               /* DIRECTIVE  */
    C23_ATTRIBUTE = 384,           /* C23_ATTRIBUTE  */
    FLOATING_DECIMALconstant = 385, /* FLOATING_DECIMALconstant  */
    FLOATING_FRACTIONconstant = 386, /* FLOATING_FRACTIONconstant  */
    FLOATINGconstant = 387,        /* FLOATINGconstant  */
    ARROW = 388,                   /* ARROW  */
    ICR = 389,                     /* ICR  */
    DECR = 390,                    /* DECR  */
    LS = 391,                      /* LS  */
    RS = 392,                      /* RS  */
    LE = 393,                      /* LE  */
    GE = 394,                      /* GE  */
    EQ = 395,                      /* EQ  */
    NE = 396,                      /* NE  */
    ANDAND = 397,                  /* ANDAND  */
    OROR = 398,                    /* OROR  */
    ATTR = 399,                    /* ATTR  */
    ELLIPSIS = 400,                /* ELLIPSIS  */
    EXPassign = 401,               /* EXPassign  */
    MULTassign = 402,              /* MULTassign  */
    DIVassign = 403,               /* DIVassign  */
    MODassign = 404,               /* MODassign  */
    PLUSassign = 405,              /* PLUSassign  */
    MINUSassign = 406,             /* MINUSassign  */
    LSassign = 407,                /* LSassign  */
    RSassign = 408,                /* RSassign  */
    ANDassign = 409,               /* ANDassign  */
    ERassign = 410,                /* ERassign  */
    ORassign = 411,                /* ORassign  */
    ErangeUpLt = 412,              /* ErangeUpLt  */
    ErangeUpLe = 413,              /* ErangeUpLe  */
    ErangeEq = 414,                /* ErangeEq  */
    ErangeNe = 415,                /* ErangeNe  */
    ErangeDownGt = 416,            /* ErangeDownGt  */
    ErangeDownGe = 417,            /* ErangeDownGe  */
    ErangeDownEq = 418,            /* ErangeDownEq  */
    ErangeDownNe = 419,            /* ErangeDownNe  */
    ATassign = 420,                /* ATassign  */
    THEN = 421                     /* THEN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define TYPEDEF 258
#define EXTERN 259
#define STATIC 260
#define AUTO 261
#define REGISTER 262
#define THREADLOCALGCC 263
#define THREADLOCALC11 264
#define INLINE 265
#define FORTRAN 266
#define NORETURN 267
#define CONST 268
#define VOLATILE 269
#define RESTRICT 270
#define ATOMIC 271
#define FORALL 272
#define MUTEX 273
#define VIRTUAL 274
#define VTABLE 275
#define COERCE 276
#define VOID 277
#define CHAR 278
#define SHORT 279
#define INT 280
#define LONG 281
#define FLOAT 282
#define DOUBLE 283
#define SIGNED 284
#define UNSIGNED 285
#define BOOL 286
#define COMPLEX 287
#define IMAGINARY 288
#define INT128 289
#define UINT128 290
#define FLOAT80 291
#define uuFLOAT128 292
#define FLOAT16 293
#define FLOAT32 294
#define FLOAT32X 295
#define FLOAT64 296
#define FLOAT64X 297
#define FLOAT128 298
#define FLOAT128X 299
#define FLOAT32X4 300
#define FLOAT64X2 301
#define SVFLOAT32 302
#define SVFLOAT64 303
#define SVBOOL 304
#define DECIMAL32 305
#define DECIMAL64 306
#define DECIMAL128 307
#define ZERO_T 308
#define ONE_T 309
#define SIZEOF 310
#define TYPEOF 311
#define VA_LIST 312
#define VA_ARG 313
#define AUTO_TYPE 314
#define COUNTOF 315
#define OFFSETOF 316
#define BASETYPEOF 317
#define TYPEID 318
#define ENUM 319
#define STRUCT 320
#define UNION 321
#define EXCEPTION 322
#define GENERATOR 323
#define COROUTINE 324
#define MONITOR 325
#define THREAD 326
#define OTYPE 327
#define FTYPE 328
#define DTYPE 329
#define TTYPE 330
#define TRAIT 331
#define LABEL 332
#define SUSPEND 333
#define ATTRIBUTE 334
#define EXTENSION 335
#define IF 336
#define ELSE 337
#define SWITCH 338
#define CASE 339
#define DEFAULT 340
#define DO 341
#define WHILE 342
#define FOR 343
#define BREAK 344
#define CONTINUE 345
#define GOTO 346
#define RETURN 347
#define CHOOSE 348
#define FALLTHROUGH 349
#define WITH 350
#define WHEN 351
#define WAITFOR 352
#define WAITUNTIL 353
#define CORUN 354
#define COFOR 355
#define DISABLE 356
#define ENABLE 357
#define TRY 358
#define THROW 359
#define THROWRESUME 360
#define AT 361
#define ASM 362
#define ALIGNAS 363
#define ALIGNOF 364
#define __ALIGNOF 365
#define GENERIC 366
#define STATICASSERT 367
#define IDENTIFIER 368
#define TYPEDIMname 369
#define TYPEDEFname 370
#define TYPEGENname 371
#define TIMEOUT 372
#define WAND 373
#define WOR 374
#define CATCH 375
#define RECOVER 376
#define CATCHRESUME 377
#define FIXUP 378
#define FINALLY 379
#define INTEGERconstant 380
#define CHARACTERconstant 381
#define STRINGliteral 382
#define DIRECTIVE 383
#define C23_ATTRIBUTE 384
#define FLOATING_DECIMALconstant 385
#define FLOATING_FRACTIONconstant 386
#define FLOATINGconstant 387
#define ARROW 388
#define ICR 389
#define DECR 390
#define LS 391
#define RS 392
#define LE 393
#define GE 394
#define EQ 395
#define NE 396
#define ANDAND 397
#define OROR 398
#define ATTR 399
#define ELLIPSIS 400
#define EXPassign 401
#define MULTassign 402
#define DIVassign 403
#define MODassign 404
#define PLUSassign 405
#define MINUSassign 406
#define LSassign 407
#define RSassign 408
#define ANDassign 409
#define ERassign 410
#define ORassign 411
#define ErangeUpLt 412
#define ErangeUpLe 413
#define ErangeEq 414
#define ErangeNe 415
#define ErangeDownGt 416
#define ErangeDownGe 417
#define ErangeDownEq 418
#define ErangeDownNe 419
#define ATassign 420
#define THEN 421

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 426 "Parser/parser.yy"

	// A raw token can be used.
	Token tok;

	// The general node types hold some generic node or list of nodes.
	DeclarationNode * decl;
	InitializerNode * init;
	ExpressionNode * expr;
	StatementNode * stmt;
	ClauseNode * clause;
	TypeData * type;

	// Special "nodes" containing compound information.
	CondCtrl * ifctrl;
	ForCtrl * forctrl;
	LabelNode * labels;

	// Various flags and single values that become fields later.
	ast::AggregateDecl::Aggregate aggKey;
	ast::TypeDecl::Kind tclass;
	OperKinds oper;
	bool is_volatile;
	EnumHiding enum_hiding;
	ast::ExceptionKind except_kind;
	// String passes ownership with it.
	std::string * str;

	// Narrower node types are used to avoid constant unwrapping.
	ast::WaitForStmt * wfs;
	ast::WaitUntilStmt::ClauseNode * wucn;
	ast::GenericExpr * genexpr;

#line 877 "Parser/parser.cc"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif


extern YYSTYPE yylval;
extern YYLTYPE yylloc;

int yyparse (void);


#endif /* !YY_YY_PARSER_PARSER_HH_INCLUDED  */
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_TYPEDEF = 3,                    /* TYPEDEF  */
  YYSYMBOL_EXTERN = 4,                     /* EXTERN  */
  YYSYMBOL_STATIC = 5,                     /* STATIC  */
  YYSYMBOL_AUTO = 6,                       /* AUTO  */
  YYSYMBOL_REGISTER = 7,                   /* REGISTER  */
  YYSYMBOL_THREADLOCALGCC = 8,             /* THREADLOCALGCC  */
  YYSYMBOL_THREADLOCALC11 = 9,             /* THREADLOCALC11  */
  YYSYMBOL_INLINE = 10,                    /* INLINE  */
  YYSYMBOL_FORTRAN = 11,                   /* FORTRAN  */
  YYSYMBOL_NORETURN = 12,                  /* NORETURN  */
  YYSYMBOL_CONST = 13,                     /* CONST  */
  YYSYMBOL_VOLATILE = 14,                  /* VOLATILE  */
  YYSYMBOL_RESTRICT = 15,                  /* RESTRICT  */
  YYSYMBOL_ATOMIC = 16,                    /* ATOMIC  */
  YYSYMBOL_FORALL = 17,                    /* FORALL  */
  YYSYMBOL_MUTEX = 18,                     /* MUTEX  */
  YYSYMBOL_VIRTUAL = 19,                   /* VIRTUAL  */
  YYSYMBOL_VTABLE = 20,                    /* VTABLE  */
  YYSYMBOL_COERCE = 21,                    /* COERCE  */
  YYSYMBOL_VOID = 22,                      /* VOID  */
  YYSYMBOL_CHAR = 23,                      /* CHAR  */
  YYSYMBOL_SHORT = 24,                     /* SHORT  */
  YYSYMBOL_INT = 25,                       /* INT  */
  YYSYMBOL_LONG = 26,                      /* LONG  */
  YYSYMBOL_FLOAT = 27,                     /* FLOAT  */
  YYSYMBOL_DOUBLE = 28,                    /* DOUBLE  */
  YYSYMBOL_SIGNED = 29,                    /* SIGNED  */
  YYSYMBOL_UNSIGNED = 30,                  /* UNSIGNED  */
  YYSYMBOL_BOOL = 31,                      /* BOOL  */
  YYSYMBOL_COMPLEX = 32,                   /* COMPLEX  */
  YYSYMBOL_IMAGINARY = 33,                 /* IMAGINARY  */
  YYSYMBOL_INT128 = 34,                    /* INT128  */
  YYSYMBOL_UINT128 = 35,                   /* UINT128  */
  YYSYMBOL_FLOAT80 = 36,                   /* FLOAT80  */
  YYSYMBOL_uuFLOAT128 = 37,                /* uuFLOAT128  */
  YYSYMBOL_FLOAT16 = 38,                   /* FLOAT16  */
  YYSYMBOL_FLOAT32 = 39,                   /* FLOAT32  */
  YYSYMBOL_FLOAT32X = 40,                  /* FLOAT32X  */
  YYSYMBOL_FLOAT64 = 41,                   /* FLOAT64  */
  YYSYMBOL_FLOAT64X = 42,                  /* FLOAT64X  */
  YYSYMBOL_FLOAT128 = 43,                  /* FLOAT128  */
  YYSYMBOL_FLOAT128X = 44,                 /* FLOAT128X  */
  YYSYMBOL_FLOAT32X4 = 45,                 /* FLOAT32X4  */
  YYSYMBOL_FLOAT64X2 = 46,                 /* FLOAT64X2  */
  YYSYMBOL_SVFLOAT32 = 47,                 /* SVFLOAT32  */
  YYSYMBOL_SVFLOAT64 = 48,                 /* SVFLOAT64  */
  YYSYMBOL_SVBOOL = 49,                    /* SVBOOL  */
  YYSYMBOL_DECIMAL32 = 50,                 /* DECIMAL32  */
  YYSYMBOL_DECIMAL64 = 51,                 /* DECIMAL64  */
  YYSYMBOL_DECIMAL128 = 52,                /* DECIMAL128  */
  YYSYMBOL_ZERO_T = 53,                    /* ZERO_T  */
  YYSYMBOL_ONE_T = 54,                     /* ONE_T  */
  YYSYMBOL_SIZEOF = 55,                    /* SIZEOF  */
  YYSYMBOL_TYPEOF = 56,                    /* TYPEOF  */
  YYSYMBOL_VA_LIST = 57,                   /* VA_LIST  */
  YYSYMBOL_VA_ARG = 58,                    /* VA_ARG  */
  YYSYMBOL_AUTO_TYPE = 59,                 /* AUTO_TYPE  */
  YYSYMBOL_COUNTOF = 60,                   /* COUNTOF  */
  YYSYMBOL_OFFSETOF = 61,                  /* OFFSETOF  */
  YYSYMBOL_BASETYPEOF = 62,                /* BASETYPEOF  */
  YYSYMBOL_TYPEID = 63,                    /* TYPEID  */
  YYSYMBOL_ENUM = 64,                      /* ENUM  */
  YYSYMBOL_STRUCT = 65,                    /* STRUCT  */
  YYSYMBOL_UNION = 66,                     /* UNION  */
  YYSYMBOL_EXCEPTION = 67,                 /* EXCEPTION  */
  YYSYMBOL_GENERATOR = 68,                 /* GENERATOR  */
  YYSYMBOL_COROUTINE = 69,                 /* COROUTINE  */
  YYSYMBOL_MONITOR = 70,                   /* MONITOR  */
  YYSYMBOL_THREAD = 71,                    /* THREAD  */
  YYSYMBOL_OTYPE = 72,                     /* OTYPE  */
  YYSYMBOL_FTYPE = 73,                     /* FTYPE  */
  YYSYMBOL_DTYPE = 74,                     /* DTYPE  */
  YYSYMBOL_TTYPE = 75,                     /* TTYPE  */
  YYSYMBOL_TRAIT = 76,                     /* TRAIT  */
  YYSYMBOL_LABEL = 77,                     /* LABEL  */
  YYSYMBOL_SUSPEND = 78,                   /* SUSPEND  */
  YYSYMBOL_ATTRIBUTE = 79,                 /* ATTRIBUTE  */
  YYSYMBOL_EXTENSION = 80,                 /* EXTENSION  */
  YYSYMBOL_IF = 81,                        /* IF  */
  YYSYMBOL_ELSE = 82,                      /* ELSE  */
  YYSYMBOL_SWITCH = 83,                    /* SWITCH  */
  YYSYMBOL_CASE = 84,                      /* CASE  */
  YYSYMBOL_DEFAULT = 85,                   /* DEFAULT  */
  YYSYMBOL_DO = 86,                        /* DO  */
  YYSYMBOL_WHILE = 87,                     /* WHILE  */
  YYSYMBOL_FOR = 88,                       /* FOR  */
  YYSYMBOL_BREAK = 89,                     /* BREAK  */
  YYSYMBOL_CONTINUE = 90,                  /* CONTINUE  */
  YYSYMBOL_GOTO = 91,                      /* GOTO  */
  YYSYMBOL_RETURN = 92,                    /* RETURN  */
  YYSYMBOL_CHOOSE = 93,                    /* CHOOSE  */
  YYSYMBOL_FALLTHROUGH = 94,               /* FALLTHROUGH  */
  YYSYMBOL_WITH = 95,                      /* WITH  */
  YYSYMBOL_WHEN = 96,                      /* WHEN  */
  YYSYMBOL_WAITFOR = 97,                   /* WAITFOR  */
  YYSYMBOL_WAITUNTIL = 98,                 /* WAITUNTIL  */
  YYSYMBOL_CORUN = 99,                     /* CORUN  */
  YYSYMBOL_COFOR = 100,                    /* COFOR  */
  YYSYMBOL_DISABLE = 101,                  /* DISABLE  */
  YYSYMBOL_ENABLE = 102,                   /* ENABLE  */
  YYSYMBOL_TRY = 103,                      /* TRY  */
  YYSYMBOL_THROW = 104,                    /* THROW  */
  YYSYMBOL_THROWRESUME = 105,              /* THROWRESUME  */
  YYSYMBOL_AT = 106,                       /* AT  */
  YYSYMBOL_ASM = 107,                      /* ASM  */
  YYSYMBOL_ALIGNAS = 108,                  /* ALIGNAS  */
  YYSYMBOL_ALIGNOF = 109,                  /* ALIGNOF  */
  YYSYMBOL___ALIGNOF = 110,                /* __ALIGNOF  */
  YYSYMBOL_GENERIC = 111,                  /* GENERIC  */
  YYSYMBOL_STATICASSERT = 112,             /* STATICASSERT  */
  YYSYMBOL_IDENTIFIER = 113,               /* IDENTIFIER  */
  YYSYMBOL_TYPEDIMname = 114,              /* TYPEDIMname  */
  YYSYMBOL_TYPEDEFname = 115,              /* TYPEDEFname  */
  YYSYMBOL_TYPEGENname = 116,              /* TYPEGENname  */
  YYSYMBOL_TIMEOUT = 117,                  /* TIMEOUT  */
  YYSYMBOL_WAND = 118,                     /* WAND  */
  YYSYMBOL_WOR = 119,                      /* WOR  */
  YYSYMBOL_CATCH = 120,                    /* CATCH  */
  YYSYMBOL_RECOVER = 121,                  /* RECOVER  */
  YYSYMBOL_CATCHRESUME = 122,              /* CATCHRESUME  */
  YYSYMBOL_FIXUP = 123,                    /* FIXUP  */
  YYSYMBOL_FINALLY = 124,                  /* FINALLY  */
  YYSYMBOL_INTEGERconstant = 125,          /* INTEGERconstant  */
  YYSYMBOL_CHARACTERconstant = 126,        /* CHARACTERconstant  */
  YYSYMBOL_STRINGliteral = 127,            /* STRINGliteral  */
  YYSYMBOL_DIRECTIVE = 128,                /* DIRECTIVE  */
  YYSYMBOL_C23_ATTRIBUTE = 129,            /* C23_ATTRIBUTE  */
  YYSYMBOL_FLOATING_DECIMALconstant = 130, /* FLOATING_DECIMALconstant  */
  YYSYMBOL_FLOATING_FRACTIONconstant = 131, /* FLOATING_FRACTIONconstant  */
  YYSYMBOL_FLOATINGconstant = 132,         /* FLOATINGconstant  */
  YYSYMBOL_ARROW = 133,                    /* ARROW  */
  YYSYMBOL_ICR = 134,                      /* ICR  */
  YYSYMBOL_DECR = 135,                     /* DECR  */
  YYSYMBOL_LS = 136,                       /* LS  */
  YYSYMBOL_RS = 137,                       /* RS  */
  YYSYMBOL_LE = 138,                       /* LE  */
  YYSYMBOL_GE = 139,                       /* GE  */
  YYSYMBOL_EQ = 140,                       /* EQ  */
  YYSYMBOL_NE = 141,                       /* NE  */
  YYSYMBOL_ANDAND = 142,                   /* ANDAND  */
  YYSYMBOL_OROR = 143,                     /* OROR  */
  YYSYMBOL_ATTR = 144,                     /* ATTR  */
  YYSYMBOL_ELLIPSIS = 145,                 /* ELLIPSIS  */
  YYSYMBOL_EXPassign = 146,                /* EXPassign  */
  YYSYMBOL_MULTassign = 147,               /* MULTassign  */
  YYSYMBOL_DIVassign = 148,                /* DIVassign  */
  YYSYMBOL_MODassign = 149,                /* MODassign  */
  YYSYMBOL_PLUSassign = 150,               /* PLUSassign  */
  YYSYMBOL_MINUSassign = 151,              /* MINUSassign  */
  YYSYMBOL_LSassign = 152,                 /* LSassign  */
  YYSYMBOL_RSassign = 153,                 /* RSassign  */
  YYSYMBOL_ANDassign = 154,                /* ANDassign  */
  YYSYMBOL_ERassign = 155,                 /* ERassign  */
  YYSYMBOL_ORassign = 156,                 /* ORassign  */
  YYSYMBOL_ErangeUpLt = 157,               /* ErangeUpLt  */
  YYSYMBOL_ErangeUpLe = 158,               /* ErangeUpLe  */
  YYSYMBOL_ErangeEq = 159,                 /* ErangeEq  */
  YYSYMBOL_ErangeNe = 160,                 /* ErangeNe  */
  YYSYMBOL_ErangeDownGt = 161,             /* ErangeDownGt  */
  YYSYMBOL_ErangeDownGe = 162,             /* ErangeDownGe  */
  YYSYMBOL_ErangeDownEq = 163,             /* ErangeDownEq  */
  YYSYMBOL_ErangeDownNe = 164,             /* ErangeDownNe  */
  YYSYMBOL_ATassign = 165,                 /* ATassign  */
  YYSYMBOL_THEN = 166,                     /* THEN  */
  YYSYMBOL_167_ = 167,                     /* '}'  */
  YYSYMBOL_168_ = 168,                     /* '('  */
  YYSYMBOL_169_ = 169,                     /* '@'  */
  YYSYMBOL_170_ = 170,                     /* ')'  */
  YYSYMBOL_171_ = 171,                     /* '.'  */
  YYSYMBOL_172_ = 172,                     /* '['  */
  YYSYMBOL_173_ = 173,                     /* ']'  */
  YYSYMBOL_174_ = 174,                     /* ','  */
  YYSYMBOL_175_ = 175,                     /* ':'  */
  YYSYMBOL_176_ = 176,                     /* '{'  */
  YYSYMBOL_177_ = 177,                     /* '`'  */
  YYSYMBOL_178_ = 178,                     /* '^'  */
  YYSYMBOL_179_ = 179,                     /* '*'  */
  YYSYMBOL_180_ = 180,                     /* '&'  */
  YYSYMBOL_181_ = 181,                     /* '+'  */
  YYSYMBOL_182_ = 182,                     /* '-'  */
  YYSYMBOL_183_ = 183,                     /* '!'  */
  YYSYMBOL_184_ = 184,                     /* '~'  */
  YYSYMBOL_185_ = 185,                     /* '\\'  */
  YYSYMBOL_186_ = 186,                     /* '/'  */
  YYSYMBOL_187_ = 187,                     /* '%'  */
  YYSYMBOL_188_ = 188,                     /* '<'  */
  YYSYMBOL_189_ = 189,                     /* '>'  */
  YYSYMBOL_190_ = 190,                     /* '|'  */
  YYSYMBOL_191_ = 191,                     /* '?'  */
  YYSYMBOL_192_ = 192,                     /* '='  */
  YYSYMBOL_193_ = 193,                     /* ';'  */
  YYSYMBOL_YYACCEPT = 194,                 /* $accept  */
  YYSYMBOL_push = 195,                     /* push  */
  YYSYMBOL_recovery_push = 196,            /* recovery_push  */
  YYSYMBOL_recovery_pop = 197,             /* recovery_pop  */
  YYSYMBOL_pop = 198,                      /* pop  */
  YYSYMBOL_constant = 199,                 /* constant  */
  YYSYMBOL_quasi_keyword = 200,            /* quasi_keyword  */
  YYSYMBOL_identifier = 201,               /* identifier  */
  YYSYMBOL_identifier_at = 202,            /* identifier_at  */
  YYSYMBOL_identifier_or_type_name = 203,  /* identifier_or_type_name  */
  YYSYMBOL_string_literal = 204,           /* string_literal  */
  YYSYMBOL_string_literal_list = 205,      /* string_literal_list  */
  YYSYMBOL_primary_expression = 206,       /* primary_expression  */
  YYSYMBOL_generic_assoc_list = 207,       /* generic_assoc_list  */
  YYSYMBOL_generic_association = 208,      /* generic_association  */
  YYSYMBOL_postfix_expression = 209,       /* postfix_expression  */
  YYSYMBOL_field_name_list = 210,          /* field_name_list  */
  YYSYMBOL_field = 211,                    /* field  */
  YYSYMBOL_field_name = 212,               /* field_name  */
  YYSYMBOL_fraction_constants_opt = 213,   /* fraction_constants_opt  */
  YYSYMBOL_unary_expression = 214,         /* unary_expression  */
  YYSYMBOL_alignof_operator = 215,         /* alignof_operator  */
  YYSYMBOL_ptrref_operator = 216,          /* ptrref_operator  */
  YYSYMBOL_unary_operator = 217,           /* unary_operator  */
  YYSYMBOL_cast_expression = 218,          /* cast_expression  */
  YYSYMBOL_qualifier_cast_list = 219,      /* qualifier_cast_list  */
  YYSYMBOL_cast_modifier = 220,            /* cast_modifier  */
  YYSYMBOL_exponential_expression = 221,   /* exponential_expression  */
  YYSYMBOL_multiplicative_expression = 222, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 223,      /* additive_expression  */
  YYSYMBOL_shift_expression = 224,         /* shift_expression  */
  YYSYMBOL_relational_expression = 225,    /* relational_expression  */
  YYSYMBOL_equality_expression = 226,      /* equality_expression  */
  YYSYMBOL_AND_expression = 227,           /* AND_expression  */
  YYSYMBOL_exclusive_OR_expression = 228,  /* exclusive_OR_expression  */
  YYSYMBOL_inclusive_OR_expression = 229,  /* inclusive_OR_expression  */
  YYSYMBOL_logical_AND_expression = 230,   /* logical_AND_expression  */
  YYSYMBOL_logical_OR_expression = 231,    /* logical_OR_expression  */
  YYSYMBOL_conditional_expression = 232,   /* conditional_expression  */
  YYSYMBOL_constant_expression = 233,      /* constant_expression  */
  YYSYMBOL_argument_expression_list_opt = 234, /* argument_expression_list_opt  */
  YYSYMBOL_argument_expression_list = 235, /* argument_expression_list  */
  YYSYMBOL_argument_expression = 236,      /* argument_expression  */
  YYSYMBOL_assignment_expression = 237,    /* assignment_expression  */
  YYSYMBOL_assignment_expression_opt = 238, /* assignment_expression_opt  */
  YYSYMBOL_assignment_operator = 239,      /* assignment_operator  */
  YYSYMBOL_simple_assignment_operator = 240, /* simple_assignment_operator  */
  YYSYMBOL_compound_assignment_operator = 241, /* compound_assignment_operator  */
  YYSYMBOL_tuple = 242,                    /* tuple  */
  YYSYMBOL_tuple_expression_list = 243,    /* tuple_expression_list  */
  YYSYMBOL_comma_expression = 244,         /* comma_expression  */
  YYSYMBOL_comma_expression_opt = 245,     /* comma_expression_opt  */
  YYSYMBOL_statement = 246,                /* statement  */
  YYSYMBOL_labelled_statement = 247,       /* labelled_statement  */
  YYSYMBOL_compound_statement = 248,       /* compound_statement  */
  YYSYMBOL_statement_decl_list = 249,      /* statement_decl_list  */
  YYSYMBOL_statement_decl = 250,           /* statement_decl  */
  YYSYMBOL_statement_list_nodecl = 251,    /* statement_list_nodecl  */
  YYSYMBOL_expression_statement = 252,     /* expression_statement  */
  YYSYMBOL_selection_statement = 253,      /* selection_statement  */
  YYSYMBOL_conditional_declaration = 254,  /* conditional_declaration  */
  YYSYMBOL_case_value = 255,               /* case_value  */
  YYSYMBOL_case_value_list = 256,          /* case_value_list  */
  YYSYMBOL_case_label = 257,               /* case_label  */
  YYSYMBOL_case_label_list = 258,          /* case_label_list  */
  YYSYMBOL_case_clause = 259,              /* case_clause  */
  YYSYMBOL_switch_clause_list_opt = 260,   /* switch_clause_list_opt  */
  YYSYMBOL_switch_clause_list = 261,       /* switch_clause_list  */
  YYSYMBOL_iteration_statement = 262,      /* iteration_statement  */
  YYSYMBOL_for_control_expression_list = 263, /* for_control_expression_list  */
  YYSYMBOL_for_control_expression = 264,   /* for_control_expression  */
  YYSYMBOL_enum_key = 265,                 /* enum_key  */
  YYSYMBOL_updown = 266,                   /* updown  */
  YYSYMBOL_updownS = 267,                  /* updownS  */
  YYSYMBOL_updownEq = 268,                 /* updownEq  */
  YYSYMBOL_jump_statement = 269,           /* jump_statement  */
  YYSYMBOL_with_statement = 270,           /* with_statement  */
  YYSYMBOL_mutex_statement = 271,          /* mutex_statement  */
  YYSYMBOL_when_clause = 272,              /* when_clause  */
  YYSYMBOL_when_clause_opt = 273,          /* when_clause_opt  */
  YYSYMBOL_cast_expression_list = 274,     /* cast_expression_list  */
  YYSYMBOL_timeout = 275,                  /* timeout  */
  YYSYMBOL_wor = 276,                      /* wor  */
  YYSYMBOL_waitfor = 277,                  /* waitfor  */
  YYSYMBOL_wor_waitfor_clause = 278,       /* wor_waitfor_clause  */
  YYSYMBOL_waitfor_statement = 279,        /* waitfor_statement  */
  YYSYMBOL_wand = 280,                     /* wand  */
  YYSYMBOL_waituntil = 281,                /* waituntil  */
  YYSYMBOL_waituntil_clause = 282,         /* waituntil_clause  */
  YYSYMBOL_wand_waituntil_clause = 283,    /* wand_waituntil_clause  */
  YYSYMBOL_wor_waituntil_clause = 284,     /* wor_waituntil_clause  */
  YYSYMBOL_waituntil_statement = 285,      /* waituntil_statement  */
  YYSYMBOL_corun_statement = 286,          /* corun_statement  */
  YYSYMBOL_cofor_statement = 287,          /* cofor_statement  */
  YYSYMBOL_exception_statement = 288,      /* exception_statement  */
  YYSYMBOL_handler_clause = 289,           /* handler_clause  */
  YYSYMBOL_handler_predicate_opt = 290,    /* handler_predicate_opt  */
  YYSYMBOL_handler_key = 291,              /* handler_key  */
  YYSYMBOL_finally_clause = 292,           /* finally_clause  */
  YYSYMBOL_exception_declaration = 293,    /* exception_declaration  */
  YYSYMBOL_enable_disable_statement = 294, /* enable_disable_statement  */
  YYSYMBOL_enable_disable_key = 295,       /* enable_disable_key  */
  YYSYMBOL_asm_statement = 296,            /* asm_statement  */
  YYSYMBOL_asm_volatile_opt = 297,         /* asm_volatile_opt  */
  YYSYMBOL_asm_operands_opt = 298,         /* asm_operands_opt  */
  YYSYMBOL_asm_operands_list = 299,        /* asm_operands_list  */
  YYSYMBOL_asm_operand = 300,              /* asm_operand  */
  YYSYMBOL_asm_clobbers_list_opt = 301,    /* asm_clobbers_list_opt  */
  YYSYMBOL_asm_label_list = 302,           /* asm_label_list  */
  YYSYMBOL_declaration_list_opt = 303,     /* declaration_list_opt  */
  YYSYMBOL_declaration_list = 304,         /* declaration_list  */
  YYSYMBOL_KR_parameter_list_opt = 305,    /* KR_parameter_list_opt  */
  YYSYMBOL_KR_parameter_list = 306,        /* KR_parameter_list  */
  YYSYMBOL_local_label_declaration_opt = 307, /* local_label_declaration_opt  */
  YYSYMBOL_local_label_declaration_list = 308, /* local_label_declaration_list  */
  YYSYMBOL_local_label_list = 309,         /* local_label_list  */
  YYSYMBOL_declaration = 310,              /* declaration  */
  YYSYMBOL_static_assert = 311,            /* static_assert  */
  YYSYMBOL_cfa_declaration = 312,          /* cfa_declaration  */
  YYSYMBOL_cfa_variable_declaration = 313, /* cfa_variable_declaration  */
  YYSYMBOL_cfa_variable_specifier = 314,   /* cfa_variable_specifier  */
  YYSYMBOL_cfa_function_declaration = 315, /* cfa_function_declaration  */
  YYSYMBOL_cfa_function_specifier = 316,   /* cfa_function_specifier  */
  YYSYMBOL_cfa_function_return = 317,      /* cfa_function_return  */
  YYSYMBOL_cfa_typedef_declaration = 318,  /* cfa_typedef_declaration  */
  YYSYMBOL_typedef_declaration = 319,      /* typedef_declaration  */
  YYSYMBOL_typedef_expression = 320,       /* typedef_expression  */
  YYSYMBOL_c_declaration = 321,            /* c_declaration  */
  YYSYMBOL_declaring_list = 322,           /* declaring_list  */
  YYSYMBOL_general_function_declarator = 323, /* general_function_declarator  */
  YYSYMBOL_declaration_specifier = 324,    /* declaration_specifier  */
  YYSYMBOL_invalid_types = 325,            /* invalid_types  */
  YYSYMBOL_declaration_specifier_nobody = 326, /* declaration_specifier_nobody  */
  YYSYMBOL_type_specifier = 327,           /* type_specifier  */
  YYSYMBOL_type_specifier_nobody = 328,    /* type_specifier_nobody  */
  YYSYMBOL_type_qualifier_list_opt = 329,  /* type_qualifier_list_opt  */
  YYSYMBOL_type_qualifier_list = 330,      /* type_qualifier_list  */
  YYSYMBOL_type_qualifier = 331,           /* type_qualifier  */
  YYSYMBOL_type_qualifier_name = 332,      /* type_qualifier_name  */
  YYSYMBOL_forall = 333,                   /* forall  */
  YYSYMBOL_declaration_qualifier_list = 334, /* declaration_qualifier_list  */
  YYSYMBOL_storage_class_list = 335,       /* storage_class_list  */
  YYSYMBOL_storage_class = 336,            /* storage_class  */
  YYSYMBOL_basic_type_name = 337,          /* basic_type_name  */
  YYSYMBOL_basic_type_name_type = 338,     /* basic_type_name_type  */
  YYSYMBOL_vtable_opt = 339,               /* vtable_opt  */
  YYSYMBOL_vtable = 340,                   /* vtable  */
  YYSYMBOL_default_opt = 341,              /* default_opt  */
  YYSYMBOL_basic_declaration_specifier = 342, /* basic_declaration_specifier  */
  YYSYMBOL_basic_type_specifier = 343,     /* basic_type_specifier  */
  YYSYMBOL_direct_type = 344,              /* direct_type  */
  YYSYMBOL_indirect_type = 345,            /* indirect_type  */
  YYSYMBOL_sue_declaration_specifier = 346, /* sue_declaration_specifier  */
  YYSYMBOL_sue_type_specifier = 347,       /* sue_type_specifier  */
  YYSYMBOL_348_1 = 348,                    /* $@1  */
  YYSYMBOL_sue_declaration_specifier_nobody = 349, /* sue_declaration_specifier_nobody  */
  YYSYMBOL_sue_type_specifier_nobody = 350, /* sue_type_specifier_nobody  */
  YYSYMBOL_type_declaration_specifier = 351, /* type_declaration_specifier  */
  YYSYMBOL_type_type_specifier = 352,      /* type_type_specifier  */
  YYSYMBOL_type_name = 353,                /* type_name  */
  YYSYMBOL_typegen_name = 354,             /* typegen_name  */
  YYSYMBOL_elaborated_type = 355,          /* elaborated_type  */
  YYSYMBOL_elaborated_type_nobody = 356,   /* elaborated_type_nobody  */
  YYSYMBOL_aggregate_type = 357,           /* aggregate_type  */
  YYSYMBOL_358_2 = 358,                    /* $@2  */
  YYSYMBOL_359_3 = 359,                    /* $@3  */
  YYSYMBOL_360_4 = 360,                    /* $@4  */
  YYSYMBOL_361_5 = 361,                    /* $@5  */
  YYSYMBOL_type_parameters_opt = 362,      /* type_parameters_opt  */
  YYSYMBOL_aggregate_type_nobody = 363,    /* aggregate_type_nobody  */
  YYSYMBOL_aggregate_key = 364,            /* aggregate_key  */
  YYSYMBOL_aggregate_data = 365,           /* aggregate_data  */
  YYSYMBOL_aggregate_control = 366,        /* aggregate_control  */
  YYSYMBOL_field_declaration_list_opt = 367, /* field_declaration_list_opt  */
  YYSYMBOL_field_declaration = 368,        /* field_declaration  */
  YYSYMBOL_field_declaring_list_opt = 369, /* field_declaring_list_opt  */
  YYSYMBOL_field_declaring_list = 370,     /* field_declaring_list  */
  YYSYMBOL_field_declarator = 371,         /* field_declarator  */
  YYSYMBOL_field_abstract_list_opt = 372,  /* field_abstract_list_opt  */
  YYSYMBOL_field_abstract = 373,           /* field_abstract  */
  YYSYMBOL_cfa_field_declaring_list = 374, /* cfa_field_declaring_list  */
  YYSYMBOL_cfa_field_abstract_list = 375,  /* cfa_field_abstract_list  */
  YYSYMBOL_bit_subrange_size_opt = 376,    /* bit_subrange_size_opt  */
  YYSYMBOL_bit_subrange_size = 377,        /* bit_subrange_size  */
  YYSYMBOL_enum_type = 378,                /* enum_type  */
  YYSYMBOL_379_6 = 379,                    /* $@6  */
  YYSYMBOL_380_7 = 380,                    /* $@7  */
  YYSYMBOL_enumerator_type = 381,          /* enumerator_type  */
  YYSYMBOL_hide_opt = 382,                 /* hide_opt  */
  YYSYMBOL_enum_type_nobody = 383,         /* enum_type_nobody  */
  YYSYMBOL_enumerator_list = 384,          /* enumerator_list  */
  YYSYMBOL_visible_hide_opt = 385,         /* visible_hide_opt  */
  YYSYMBOL_enumerator_value_opt = 386,     /* enumerator_value_opt  */
  YYSYMBOL_parameter_list_ellipsis_opt = 387, /* parameter_list_ellipsis_opt  */
  YYSYMBOL_parameter_list = 388,           /* parameter_list  */
  YYSYMBOL_cfa_parameter_list_ellipsis_opt = 389, /* cfa_parameter_list_ellipsis_opt  */
  YYSYMBOL_cfa_parameter_list = 390,       /* cfa_parameter_list  */
  YYSYMBOL_cfa_abstract_parameter_list = 391, /* cfa_abstract_parameter_list  */
  YYSYMBOL_parameter_declaration = 392,    /* parameter_declaration  */
  YYSYMBOL_abstract_parameter_declaration = 393, /* abstract_parameter_declaration  */
  YYSYMBOL_cfa_parameter_declaration = 394, /* cfa_parameter_declaration  */
  YYSYMBOL_cfa_abstract_parameter_declaration = 395, /* cfa_abstract_parameter_declaration  */
  YYSYMBOL_identifier_list = 396,          /* identifier_list  */
  YYSYMBOL_type_no_function = 397,         /* type_no_function  */
  YYSYMBOL_type = 398,                     /* type  */
  YYSYMBOL_initializer_opt = 399,          /* initializer_opt  */
  YYSYMBOL_initializer = 400,              /* initializer  */
  YYSYMBOL_initializer_list_opt = 401,     /* initializer_list_opt  */
  YYSYMBOL_designation = 402,              /* designation  */
  YYSYMBOL_designator_list = 403,          /* designator_list  */
  YYSYMBOL_designator = 404,               /* designator  */
  YYSYMBOL_type_parameter_list = 405,      /* type_parameter_list  */
  YYSYMBOL_type_initializer_opt = 406,     /* type_initializer_opt  */
  YYSYMBOL_type_parameter = 407,           /* type_parameter  */
  YYSYMBOL_408_8 = 408,                    /* $@8  */
  YYSYMBOL_409_9 = 409,                    /* $@9  */
  YYSYMBOL_new_type_class = 410,           /* new_type_class  */
  YYSYMBOL_type_class = 411,               /* type_class  */
  YYSYMBOL_assertion_list_opt = 412,       /* assertion_list_opt  */
  YYSYMBOL_assertion_list = 413,           /* assertion_list  */
  YYSYMBOL_assertion = 414,                /* assertion  */
  YYSYMBOL_type_list = 415,                /* type_list  */
  YYSYMBOL_type_declaring_list = 416,      /* type_declaring_list  */
  YYSYMBOL_type_declarator = 417,          /* type_declarator  */
  YYSYMBOL_type_declarator_name = 418,     /* type_declarator_name  */
  YYSYMBOL_trait_specifier = 419,          /* trait_specifier  */
  YYSYMBOL_trait_declaration_list = 420,   /* trait_declaration_list  */
  YYSYMBOL_trait_declaration = 421,        /* trait_declaration  */
  YYSYMBOL_cfa_trait_declaring_list = 422, /* cfa_trait_declaring_list  */
  YYSYMBOL_trait_declaring_list = 423,     /* trait_declaring_list  */
  YYSYMBOL_translation_unit = 424,         /* translation_unit  */
  YYSYMBOL_top_definition_list = 425,      /* top_definition_list  */
  YYSYMBOL_external_definition_list_opt = 426, /* external_definition_list_opt  */
  YYSYMBOL_external_definition_list = 427, /* external_definition_list  */
  YYSYMBOL_up = 428,                       /* up  */
  YYSYMBOL_down = 429,                     /* down  */
  YYSYMBOL_external_definition = 430,      /* external_definition  */
  YYSYMBOL_431_10 = 431,                   /* $@10  */
  YYSYMBOL_432_11 = 432,                   /* $@11  */
  YYSYMBOL_433_12 = 433,                   /* $@12  */
  YYSYMBOL_434_13 = 434,                   /* $@13  */
  YYSYMBOL_435_14 = 435,                   /* $@14  */
  YYSYMBOL_external_function_definition = 436, /* external_function_definition  */
  YYSYMBOL_with_clause_opt = 437,          /* with_clause_opt  */
  YYSYMBOL_function_definition = 438,      /* function_definition  */
  YYSYMBOL_declarator = 439,               /* declarator  */
  YYSYMBOL_subrange = 440,                 /* subrange  */
  YYSYMBOL_asm_name_opt = 441,             /* asm_name_opt  */
  YYSYMBOL_attribute_list_opt = 442,       /* attribute_list_opt  */
  YYSYMBOL_attribute_list = 443,           /* attribute_list  */
  YYSYMBOL_attribute = 444,                /* attribute  */
  YYSYMBOL_attribute_name_list = 445,      /* attribute_name_list  */
  YYSYMBOL_attribute_name = 446,           /* attribute_name  */
  YYSYMBOL_attr_name = 447,                /* attr_name  */
  YYSYMBOL_paren_identifier = 448,         /* paren_identifier  */
  YYSYMBOL_variable_declarator = 449,      /* variable_declarator  */
  YYSYMBOL_variable_ptr = 450,             /* variable_ptr  */
  YYSYMBOL_variable_array = 451,           /* variable_array  */
  YYSYMBOL_variable_function = 452,        /* variable_function  */
  YYSYMBOL_function_declarator = 453,      /* function_declarator  */
  YYSYMBOL_function_no_ptr = 454,          /* function_no_ptr  */
  YYSYMBOL_function_ptr = 455,             /* function_ptr  */
  YYSYMBOL_function_array = 456,           /* function_array  */
  YYSYMBOL_KR_function_declarator = 457,   /* KR_function_declarator  */
  YYSYMBOL_KR_function_no_ptr = 458,       /* KR_function_no_ptr  */
  YYSYMBOL_KR_function_ptr = 459,          /* KR_function_ptr  */
  YYSYMBOL_KR_function_array = 460,        /* KR_function_array  */
  YYSYMBOL_paren_type = 461,               /* paren_type  */
  YYSYMBOL_variable_type_redeclarator = 462, /* variable_type_redeclarator  */
  YYSYMBOL_variable_type_ptr = 463,        /* variable_type_ptr  */
  YYSYMBOL_variable_type_array = 464,      /* variable_type_array  */
  YYSYMBOL_variable_type_function = 465,   /* variable_type_function  */
  YYSYMBOL_function_type_redeclarator = 466, /* function_type_redeclarator  */
  YYSYMBOL_function_type_no_ptr = 467,     /* function_type_no_ptr  */
  YYSYMBOL_function_type_ptr = 468,        /* function_type_ptr  */
  YYSYMBOL_function_type_array = 469,      /* function_type_array  */
  YYSYMBOL_identifier_parameter_declarator = 470, /* identifier_parameter_declarator  */
  YYSYMBOL_identifier_parameter_ptr = 471, /* identifier_parameter_ptr  */
  YYSYMBOL_identifier_parameter_array = 472, /* identifier_parameter_array  */
  YYSYMBOL_identifier_parameter_function = 473, /* identifier_parameter_function  */
  YYSYMBOL_type_parameter_redeclarator = 474, /* type_parameter_redeclarator  */
  YYSYMBOL_typedef_name = 475,             /* typedef_name  */
  YYSYMBOL_type_parameter_ptr = 476,       /* type_parameter_ptr  */
  YYSYMBOL_type_parameter_array = 477,     /* type_parameter_array  */
  YYSYMBOL_type_parameter_function = 478,  /* type_parameter_function  */
  YYSYMBOL_abstract_declarator = 479,      /* abstract_declarator  */
  YYSYMBOL_abstract_ptr = 480,             /* abstract_ptr  */
  YYSYMBOL_abstract_array = 481,           /* abstract_array  */
  YYSYMBOL_abstract_function = 482,        /* abstract_function  */
  YYSYMBOL_array_dimension = 483,          /* array_dimension  */
  YYSYMBOL_array_type_list = 484,          /* array_type_list  */
  YYSYMBOL_upupeq = 485,                   /* upupeq  */
  YYSYMBOL_multi_array_dimension = 486,    /* multi_array_dimension  */
  YYSYMBOL_abstract_parameter_declarator_opt = 487, /* abstract_parameter_declarator_opt  */
  YYSYMBOL_abstract_parameter_declarator = 488, /* abstract_parameter_declarator  */
  YYSYMBOL_abstract_parameter_ptr = 489,   /* abstract_parameter_ptr  */
  YYSYMBOL_abstract_parameter_array = 490, /* abstract_parameter_array  */
  YYSYMBOL_abstract_parameter_function = 491, /* abstract_parameter_function  */
  YYSYMBOL_array_parameter_dimension = 492, /* array_parameter_dimension  */
  YYSYMBOL_array_parameter_1st_dimension = 493, /* array_parameter_1st_dimension  */
  YYSYMBOL_variable_abstract_declarator = 494, /* variable_abstract_declarator  */
  YYSYMBOL_variable_abstract_ptr = 495,    /* variable_abstract_ptr  */
  YYSYMBOL_variable_abstract_array = 496,  /* variable_abstract_array  */
  YYSYMBOL_variable_abstract_function = 497, /* variable_abstract_function  */
  YYSYMBOL_cfa_identifier_parameter_declarator_tuple = 498, /* cfa_identifier_parameter_declarator_tuple  */
  YYSYMBOL_cfa_identifier_parameter_declarator_no_tuple = 499, /* cfa_identifier_parameter_declarator_no_tuple  */
  YYSYMBOL_cfa_identifier_parameter_ptr = 500, /* cfa_identifier_parameter_ptr  */
  YYSYMBOL_cfa_identifier_parameter_array = 501, /* cfa_identifier_parameter_array  */
  YYSYMBOL_cfa_array_parameter_1st_dimension = 502, /* cfa_array_parameter_1st_dimension  */
  YYSYMBOL_cfa_abstract_declarator_tuple = 503, /* cfa_abstract_declarator_tuple  */
  YYSYMBOL_cfa_abstract_declarator_no_tuple = 504, /* cfa_abstract_declarator_no_tuple  */
  YYSYMBOL_cfa_abstract_ptr = 505,         /* cfa_abstract_ptr  */
  YYSYMBOL_cfa_abstract_array = 506,       /* cfa_abstract_array  */
  YYSYMBOL_cfa_abstract_tuple = 507,       /* cfa_abstract_tuple  */
  YYSYMBOL_cfa_abstract_function = 508,    /* cfa_abstract_function  */
  YYSYMBOL_comma_opt = 509,                /* comma_opt  */
  YYSYMBOL_default_initializer_opt = 510   /* default_initializer_opt  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  29
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   32775

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  194
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  317
/* YYNRULES -- Number of rules.  */
#define YYNRULES  1165
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  2279

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   421


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   183,     2,     2,     2,   187,   180,     2,
     168,   170,   179,   181,   174,   182,   171,   186,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   175,   193,
     188,   192,   189,   191,   169,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   172,   185,   173,   178,     2,   177,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   176,   190,   167,   184,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144,
     145,   146,   147,   148,   149,   150,   151,   152,   153,   154,
     155,   156,   157,   158,   159,   160,   161,   162,   163,   164,
     165,   166
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   751,   751,   757,   761,   765,   772,   773,   774,   775,
     776,   780,   781,   782,   783,   784,   785,   786,   787,   791,
     792,   796,   797,   802,   803,   804,   808,   812,   813,   824,
     826,   828,   830,   831,   833,   835,   837,   839,   849,   851,
     853,   855,   857,   859,   864,   865,   876,   881,   886,   887,
     892,   894,   896,   902,   904,   906,   908,   910,   930,   933,
     935,   937,   939,   941,   943,   945,   947,   949,   951,   953,
     955,   964,   965,   969,   970,   972,   974,   976,   978,   980,
     985,   987,   989,   995,   996,  1004,  1007,  1008,  1010,  1015,
    1031,  1033,  1035,  1037,  1039,  1041,  1043,  1046,  1052,  1054,
    1057,  1059,  1064,  1066,  1071,  1072,  1076,  1077,  1079,  1083,
    1084,  1085,  1086,  1090,  1091,  1093,  1095,  1097,  1099,  1101,
    1103,  1105,  1112,  1113,  1114,  1115,  1119,  1120,  1124,  1125,
    1130,  1131,  1133,  1135,  1140,  1141,  1143,  1148,  1149,  1151,
    1156,  1157,  1159,  1161,  1163,  1168,  1169,  1171,  1176,  1177,
    1182,  1183,  1188,  1189,  1194,  1195,  1200,  1201,  1206,  1207,
    1209,  1214,  1219,  1220,  1224,  1226,  1231,  1234,  1237,  1242,
    1243,  1251,  1257,  1258,  1262,  1263,  1267,  1268,  1272,  1273,
    1274,  1275,  1276,  1277,  1278,  1279,  1280,  1281,  1282,  1288,
    1291,  1293,  1295,  1297,  1302,  1303,  1305,  1307,  1312,  1313,
    1319,  1320,  1326,  1327,  1328,  1329,  1330,  1331,  1332,  1333,
    1334,  1335,  1336,  1337,  1338,  1339,  1341,  1342,  1348,  1350,
    1360,  1362,  1370,  1371,  1376,  1378,  1380,  1382,  1384,  1386,
    1391,  1393,  1395,  1405,  1434,  1437,  1439,  1441,  1452,  1454,
    1456,  1462,  1467,  1469,  1471,  1473,  1481,  1482,  1484,  1488,
    1490,  1494,  1496,  1497,  1499,  1501,  1506,  1507,  1511,  1516,
    1517,  1521,  1523,  1528,  1530,  1535,  1537,  1539,  1541,  1546,
    1548,  1550,  1552,  1557,  1559,  1564,  1565,  1587,  1589,  1593,
    1596,  1598,  1601,  1603,  1606,  1608,  1613,  1619,  1621,  1626,
    1631,  1633,  1635,  1637,  1639,  1644,  1646,  1649,  1651,  1656,
    1662,  1665,  1668,  1670,  1675,  1681,  1683,  1688,  1694,  1697,
    1699,  1702,  1704,  1709,  1716,  1719,  1721,  1726,  1732,  1734,
    1739,  1745,  1748,  1752,  1763,  1768,  1773,  1784,  1786,  1788,
    1790,  1795,  1797,  1801,  1803,  1805,  1807,  1812,  1814,  1819,
    1821,  1823,  1825,  1828,  1832,  1835,  1839,  1841,  1843,  1845,
    1847,  1849,  1851,  1853,  1855,  1857,  1859,  1864,  1870,  1878,
    1883,  1884,  1888,  1889,  1894,  1898,  1899,  1902,  1904,  1909,
    1912,  1914,  1916,  1919,  1921,  1926,  1931,  1932,  1936,  1941,
    1943,  1948,  1950,  1955,  1957,  1959,  1964,  1969,  1974,  1979,
    1981,  1983,  1988,  1990,  1996,  1997,  2001,  2002,  2003,  2004,
    2008,  2013,  2014,  2016,  2018,  2020,  2024,  2028,  2029,  2033,
    2035,  2037,  2039,  2041,  2047,  2048,  2054,  2055,  2059,  2060,
    2065,  2067,  2076,  2077,  2079,  2084,  2086,  2094,  2095,  2099,
    2101,  2107,  2108,  2112,  2114,  2118,  2120,  2124,  2125,  2129,
    2130,  2134,  2136,  2138,  2142,  2144,  2159,  2160,  2161,  2162,
    2164,  2168,  2170,  2174,  2181,  2183,  2185,  2187,  2195,  2197,
    2202,  2203,  2205,  2207,  2209,  2219,  2221,  2233,  2236,  2241,
    2243,  2249,  2254,  2259,  2270,  2277,  2282,  2284,  2286,  2292,
    2294,  2299,  2301,  2302,  2303,  2319,  2321,  2324,  2326,  2329,
    2334,  2335,  2339,  2340,  2341,  2342,  2351,  2352,  2353,  2362,
    2363,  2364,  2368,  2369,  2370,  2379,  2380,  2381,  2386,  2387,
    2396,  2398,  2403,  2408,  2410,  2412,  2414,  2421,  2426,  2431,
    2432,  2434,  2444,  2446,  2451,  2453,  2455,  2457,  2459,  2461,
    2464,  2466,  2468,  2473,  2479,  2481,  2483,  2485,  2487,  2489,
    2491,  2493,  2495,  2497,  2499,  2501,  2503,  2505,  2507,  2509,
    2512,  2514,  2516,  2518,  2520,  2522,  2524,  2526,  2528,  2530,
    2532,  2534,  2536,  2538,  2540,  2542,  2544,  2546,  2551,  2552,
    2556,  2562,  2563,  2569,  2570,  2572,  2574,  2576,  2581,  2584,
    2586,  2591,  2592,  2594,  2596,  2601,  2603,  2605,  2607,  2609,
    2611,  2616,  2617,  2619,  2621,  2626,  2628,  2627,  2631,  2639,
    2640,  2642,  2644,  2649,  2650,  2652,  2657,  2659,  2661,  2663,
    2668,  2670,  2672,  2677,  2679,  2681,  2683,  2684,  2686,  2691,
    2693,  2695,  2700,  2701,  2705,  2706,  2713,  2712,  2717,  2716,
    2726,  2725,  2736,  2735,  2745,  2750,  2751,  2756,  2762,  2780,
    2781,  2785,  2787,  2789,  2794,  2796,  2798,  2800,  2805,  2807,
    2812,  2814,  2823,  2824,  2829,  2831,  2836,  2838,  2840,  2849,
    2851,  2852,  2854,  2856,  2858,  2859,  2864,  2865,  2869,  2870,
    2875,  2877,  2880,  2883,  2890,  2891,  2892,  2897,  2902,  2904,
    2910,  2911,  2917,  2918,  2922,  2930,  2937,  2950,  2949,  2953,
    2956,  2955,  2964,  2968,  2972,  2974,  2980,  2981,  2986,  2991,
    3000,  3001,  3003,  3009,  3011,  3016,  3017,  3023,  3024,  3025,
    3034,  3035,  3037,  3038,  3043,  3044,  3046,  3047,  3049,  3051,
    3057,  3058,  3060,  3061,  3062,  3064,  3066,  3073,  3074,  3076,
    3078,  3083,  3084,  3093,  3095,  3100,  3102,  3107,  3108,  3110,
    3113,  3115,  3119,  3120,  3121,  3123,  3125,  3133,  3135,  3140,
    3141,  3143,  3147,  3148,  3150,  3151,  3157,  3158,  3159,  3160,
    3164,  3165,  3170,  3171,  3172,  3173,  3174,  3188,  3189,  3194,
    3195,  3200,  3202,  3204,  3206,  3208,  3231,  3232,  3238,  3239,
    3245,  3244,  3249,  3248,  3252,  3258,  3261,  3271,  3272,  3274,
    3278,  3283,  3285,  3287,  3289,  3295,  3296,  3300,  3301,  3306,
    3308,  3315,  3317,  3318,  3320,  3325,  3327,  3329,  3334,  3336,
    3341,  3346,  3354,  3359,  3361,  3366,  3371,  3372,  3377,  3378,
    3382,  3383,  3384,  3390,  3392,  3394,  3400,  3402,  3406,  3408,
    3419,  3420,  3424,  3426,  3431,  3435,  3439,  3441,  3453,  3455,
    3457,  3459,  3461,  3463,  3465,  3466,  3471,  3474,  3473,  3485,
    3484,  3497,  3496,  3510,  3509,  3523,  3522,  3535,  3537,  3539,
    3544,  3551,  3553,  3559,  3560,  3571,  3578,  3583,  3589,  3592,
    3595,  3599,  3605,  3608,  3611,  3616,  3617,  3618,  3619,  3623,
    3631,  3632,  3644,  3645,  3649,  3650,  3655,  3657,  3659,  3661,
    3666,  3667,  3673,  3674,  3676,  3681,  3682,  3684,  3719,  3721,
    3724,  3729,  3731,  3732,  3734,  3739,  3741,  3743,  3745,  3747,
    3752,  3754,  3756,  3758,  3760,  3762,  3764,  3769,  3771,  3773,
    3775,  3784,  3786,  3787,  3792,  3794,  3796,  3798,  3800,  3805,
    3807,  3809,  3811,  3813,  3818,  3820,  3822,  3824,  3826,  3828,
    3840,  3841,  3842,  3846,  3848,  3850,  3852,  3854,  3859,  3861,
    3863,  3865,  3867,  3872,  3874,  3876,  3878,  3880,  3882,  3894,
    3899,  3904,  3906,  3907,  3909,  3914,  3916,  3918,  3920,  3922,
    3927,  3929,  3931,  3933,  3935,  3937,  3939,  3944,  3946,  3948,
    3950,  3959,  3961,  3962,  3967,  3969,  3971,  3973,  3975,  3980,
    3982,  3984,  3986,  3988,  3993,  3995,  3997,  3999,  4001,  4003,
    4013,  4015,  4018,  4019,  4021,  4026,  4028,  4030,  4032,  4037,
    4039,  4041,  4043,  4048,  4050,  4052,  4066,  4068,  4071,  4072,
    4074,  4079,  4081,  4086,  4088,  4090,  4092,  4097,  4099,  4104,
    4106,  4123,  4124,  4126,  4131,  4133,  4135,  4137,  4139,  4141,
    4146,  4147,  4149,  4151,  4156,  4158,  4160,  4166,  4168,  4171,
    4174,  4181,  4183,  4192,  4194,  4196,  4197,  4199,  4201,  4205,
    4207,  4212,  4214,  4216,  4218,  4253,  4254,  4258,  4259,  4262,
    4264,  4269,  4271,  4273,  4275,  4277,  4282,  4283,  4285,  4287,
    4292,  4294,  4296,  4302,  4303,  4305,  4314,  4317,  4319,  4322,
    4324,  4326,  4340,  4341,  4343,  4348,  4350,  4352,  4354,  4356,
    4361,  4362,  4364,  4366,  4371,  4373,  4381,  4382,  4383,  4388,
    4389,  4390,  4396,  4398,  4400,  4402,  4404,  4406,  4408,  4415,
    4417,  4419,  4421,  4423,  4425,  4427,  4429,  4431,  4433,  4436,
    4438,  4440,  4442,  4444,  4449,  4451,  4453,  4458,  4484,  4485,
    4487,  4491,  4492,  4496,  4498,  4500,  4502,  4504,  4506,  4508,
    4515,  4517,  4519,  4521,  4523,  4525,  4530,  4532,  4534,  4539,
    4541,  4543,  4561,  4563,  4568,  4569
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "TYPEDEF", "EXTERN",
  "STATIC", "AUTO", "REGISTER", "THREADLOCALGCC", "THREADLOCALC11",
  "INLINE", "FORTRAN", "NORETURN", "CONST", "VOLATILE", "RESTRICT",
  "ATOMIC", "FORALL", "MUTEX", "VIRTUAL", "VTABLE", "COERCE", "VOID",
  "CHAR", "SHORT", "INT", "LONG", "FLOAT", "DOUBLE", "SIGNED", "UNSIGNED",
  "BOOL", "COMPLEX", "IMAGINARY", "INT128", "UINT128", "FLOAT80",
  "uuFLOAT128", "FLOAT16", "FLOAT32", "FLOAT32X", "FLOAT64", "FLOAT64X",
  "FLOAT128", "FLOAT128X", "FLOAT32X4", "FLOAT64X2", "SVFLOAT32",
  "SVFLOAT64", "SVBOOL", "DECIMAL32", "DECIMAL64", "DECIMAL128", "ZERO_T",
  "ONE_T", "SIZEOF", "TYPEOF", "VA_LIST", "VA_ARG", "AUTO_TYPE", "COUNTOF",
  "OFFSETOF", "BASETYPEOF", "TYPEID", "ENUM", "STRUCT", "UNION",
  "EXCEPTION", "GENERATOR", "COROUTINE", "MONITOR", "THREAD", "OTYPE",
  "FTYPE", "DTYPE", "TTYPE", "TRAIT", "LABEL", "SUSPEND", "ATTRIBUTE",
  "EXTENSION", "IF", "ELSE", "SWITCH", "CASE", "DEFAULT", "DO", "WHILE",
  "FOR", "BREAK", "CONTINUE", "GOTO", "RETURN", "CHOOSE", "FALLTHROUGH",
  "WITH", "WHEN", "WAITFOR", "WAITUNTIL", "CORUN", "COFOR", "DISABLE",
  "ENABLE", "TRY", "THROW", "THROWRESUME", "AT", "ASM", "ALIGNAS",
  "ALIGNOF", "__ALIGNOF", "GENERIC", "STATICASSERT", "IDENTIFIER",
  "TYPEDIMname", "TYPEDEFname", "TYPEGENname", "TIMEOUT", "WAND", "WOR",
  "CATCH", "RECOVER", "CATCHRESUME", "FIXUP", "FINALLY", "INTEGERconstant",
  "CHARACTERconstant", "STRINGliteral", "DIRECTIVE", "C23_ATTRIBUTE",
  "FLOATING_DECIMALconstant", "FLOATING_FRACTIONconstant",
  "FLOATINGconstant", "ARROW", "ICR", "DECR", "LS", "RS", "LE", "GE", "EQ",
  "NE", "ANDAND", "OROR", "ATTR", "ELLIPSIS", "EXPassign", "MULTassign",
  "DIVassign", "MODassign", "PLUSassign", "MINUSassign", "LSassign",
  "RSassign", "ANDassign", "ERassign", "ORassign", "ErangeUpLt",
  "ErangeUpLe", "ErangeEq", "ErangeNe", "ErangeDownGt", "ErangeDownGe",
  "ErangeDownEq", "ErangeDownNe", "ATassign", "THEN", "'}'", "'('", "'@'",
  "')'", "'.'", "'['", "']'", "','", "':'", "'{'", "'`'", "'^'", "'*'",
  "'&'", "'+'", "'-'", "'!'", "'~'", "'\\\\'", "'/'", "'%'", "'<'", "'>'",
  "'|'", "'?'", "'='", "';'", "$accept", "push", "recovery_push",
  "recovery_pop", "pop", "constant", "quasi_keyword", "identifier",
  "identifier_at", "identifier_or_type_name", "string_literal",
  "string_literal_list", "primary_expression", "generic_assoc_list",
  "generic_association", "postfix_expression", "field_name_list", "field",
  "field_name", "fraction_constants_opt", "unary_expression",
  "alignof_operator", "ptrref_operator", "unary_operator",
  "cast_expression", "qualifier_cast_list", "cast_modifier",
  "exponential_expression", "multiplicative_expression",
  "additive_expression", "shift_expression", "relational_expression",
  "equality_expression", "AND_expression", "exclusive_OR_expression",
  "inclusive_OR_expression", "logical_AND_expression",
  "logical_OR_expression", "conditional_expression", "constant_expression",
  "argument_expression_list_opt", "argument_expression_list",
  "argument_expression", "assignment_expression",
  "assignment_expression_opt", "assignment_operator",
  "simple_assignment_operator", "compound_assignment_operator", "tuple",
  "tuple_expression_list", "comma_expression", "comma_expression_opt",
  "statement", "labelled_statement", "compound_statement",
  "statement_decl_list", "statement_decl", "statement_list_nodecl",
  "expression_statement", "selection_statement", "conditional_declaration",
  "case_value", "case_value_list", "case_label", "case_label_list",
  "case_clause", "switch_clause_list_opt", "switch_clause_list",
  "iteration_statement", "for_control_expression_list",
  "for_control_expression", "enum_key", "updown", "updownS", "updownEq",
  "jump_statement", "with_statement", "mutex_statement", "when_clause",
  "when_clause_opt", "cast_expression_list", "timeout", "wor", "waitfor",
  "wor_waitfor_clause", "waitfor_statement", "wand", "waituntil",
  "waituntil_clause", "wand_waituntil_clause", "wor_waituntil_clause",
  "waituntil_statement", "corun_statement", "cofor_statement",
  "exception_statement", "handler_clause", "handler_predicate_opt",
  "handler_key", "finally_clause", "exception_declaration",
  "enable_disable_statement", "enable_disable_key", "asm_statement",
  "asm_volatile_opt", "asm_operands_opt", "asm_operands_list",
  "asm_operand", "asm_clobbers_list_opt", "asm_label_list",
  "declaration_list_opt", "declaration_list", "KR_parameter_list_opt",
  "KR_parameter_list", "local_label_declaration_opt",
  "local_label_declaration_list", "local_label_list", "declaration",
  "static_assert", "cfa_declaration", "cfa_variable_declaration",
  "cfa_variable_specifier", "cfa_function_declaration",
  "cfa_function_specifier", "cfa_function_return",
  "cfa_typedef_declaration", "typedef_declaration", "typedef_expression",
  "c_declaration", "declaring_list", "general_function_declarator",
  "declaration_specifier", "invalid_types", "declaration_specifier_nobody",
  "type_specifier", "type_specifier_nobody", "type_qualifier_list_opt",
  "type_qualifier_list", "type_qualifier", "type_qualifier_name", "forall",
  "declaration_qualifier_list", "storage_class_list", "storage_class",
  "basic_type_name", "basic_type_name_type", "vtable_opt", "vtable",
  "default_opt", "basic_declaration_specifier", "basic_type_specifier",
  "direct_type", "indirect_type", "sue_declaration_specifier",
  "sue_type_specifier", "$@1", "sue_declaration_specifier_nobody",
  "sue_type_specifier_nobody", "type_declaration_specifier",
  "type_type_specifier", "type_name", "typegen_name", "elaborated_type",
  "elaborated_type_nobody", "aggregate_type", "$@2", "$@3", "$@4", "$@5",
  "type_parameters_opt", "aggregate_type_nobody", "aggregate_key",
  "aggregate_data", "aggregate_control", "field_declaration_list_opt",
  "field_declaration", "field_declaring_list_opt", "field_declaring_list",
  "field_declarator", "field_abstract_list_opt", "field_abstract",
  "cfa_field_declaring_list", "cfa_field_abstract_list",
  "bit_subrange_size_opt", "bit_subrange_size", "enum_type", "$@6", "$@7",
  "enumerator_type", "hide_opt", "enum_type_nobody", "enumerator_list",
  "visible_hide_opt", "enumerator_value_opt",
  "parameter_list_ellipsis_opt", "parameter_list",
  "cfa_parameter_list_ellipsis_opt", "cfa_parameter_list",
  "cfa_abstract_parameter_list", "parameter_declaration",
  "abstract_parameter_declaration", "cfa_parameter_declaration",
  "cfa_abstract_parameter_declaration", "identifier_list",
  "type_no_function", "type", "initializer_opt", "initializer",
  "initializer_list_opt", "designation", "designator_list", "designator",
  "type_parameter_list", "type_initializer_opt", "type_parameter", "$@8",
  "$@9", "new_type_class", "type_class", "assertion_list_opt",
  "assertion_list", "assertion", "type_list", "type_declaring_list",
  "type_declarator", "type_declarator_name", "trait_specifier",
  "trait_declaration_list", "trait_declaration",
  "cfa_trait_declaring_list", "trait_declaring_list", "translation_unit",
  "top_definition_list", "external_definition_list_opt",
  "external_definition_list", "up", "down", "external_definition", "$@10",
  "$@11", "$@12", "$@13", "$@14", "external_function_definition",
  "with_clause_opt", "function_definition", "declarator", "subrange",
  "asm_name_opt", "attribute_list_opt", "attribute_list", "attribute",
  "attribute_name_list", "attribute_name", "attr_name", "paren_identifier",
  "variable_declarator", "variable_ptr", "variable_array",
  "variable_function", "function_declarator", "function_no_ptr",
  "function_ptr", "function_array", "KR_function_declarator",
  "KR_function_no_ptr", "KR_function_ptr", "KR_function_array",
  "paren_type", "variable_type_redeclarator", "variable_type_ptr",
  "variable_type_array", "variable_type_function",
  "function_type_redeclarator", "function_type_no_ptr",
  "function_type_ptr", "function_type_array",
  "identifier_parameter_declarator", "identifier_parameter_ptr",
  "identifier_parameter_array", "identifier_parameter_function",
  "type_parameter_redeclarator", "typedef_name", "type_parameter_ptr",
  "type_parameter_array", "type_parameter_function", "abstract_declarator",
  "abstract_ptr", "abstract_array", "abstract_function", "array_dimension",
  "array_type_list", "upupeq", "multi_array_dimension",
  "abstract_parameter_declarator_opt", "abstract_parameter_declarator",
  "abstract_parameter_ptr", "abstract_parameter_array",
  "abstract_parameter_function", "array_parameter_dimension",
  "array_parameter_1st_dimension", "variable_abstract_declarator",
  "variable_abstract_ptr", "variable_abstract_array",
  "variable_abstract_function",
  "cfa_identifier_parameter_declarator_tuple",
  "cfa_identifier_parameter_declarator_no_tuple",
  "cfa_identifier_parameter_ptr", "cfa_identifier_parameter_array",
  "cfa_array_parameter_1st_dimension", "cfa_abstract_declarator_tuple",
  "cfa_abstract_declarator_no_tuple", "cfa_abstract_ptr",
  "cfa_abstract_array", "cfa_abstract_tuple", "cfa_abstract_function",
  "comma_opt", "default_initializer_opt", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-2012)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-1164)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     166,   -17, -2012,  3177,   194,   200, -2012,   334, -2012,  1288,
   -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012,   301, -2012,   231, -2012,
   -2012,  6813, -2012,  3177,    35, -2012,  3177,  8327,  6813,    64,
    4576,   170, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012,   286,  1194,   341, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
     105,   335, -2012, -2012, -2012, -2012, -2012, -2012,  6543,  6543,
    6813,   420,   438, 32599, -2012,   494, -2012, -2012,  2963, -2012,
     458, 16030, -2012, -2012,  3468, -2012, -2012, -2012, 19199, -2012,
     140,   290,   506,   244,    67, -2012,  6187,   539,   565,   573,
     570,  4834,   773,   979, 13639,   334, -2012,   709, 19539,  2531,
     334, -2012, -2012, -2012,  4728,   819, 12007, 12701,  1563,  4728,
     990,   656, -2012, -2012, -2012, -2012,   334, -2012, -2012, -2012,
   -2012,   682, -2012, -2012, -2012, -2012,   706,    64,   334, -2012,
     334, 23249, -2012, -2012, -2012, 26985,  6543, -2012, -2012,  6543,
     322, -2012, -2012, 31950,   711, 32028,   717,   724, 32106, -2012,
   -2012,   729, 32659, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   32184, 32184, 23585, 29527,  4869, -2012, -2012, -2012, -2012,  3468,
     367, -2012,   375,   798, -2012,  2127,  6182, 32262, 32106, 32106,
   -2012,   751,   473,   440,   769,   854,   513,   761,   778,   779,
     821,   -66, -2012,   830,   857, -2012, -2012, -2012,   818, -2012,
     879, -2012,   870, 27045,   902,  3436, -2012, -2012, -2012, -2012,
     208, 21355,   334,  3689, -2012, -2012,   915, -2012,   898,   922,
   -2012,   973, 32106, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   -2012, 23757,  3707,  2963,   278,   936,   938,   945,   958,   963,
     982, -2012, -2012,   334, 14944, 25698,   981, 24101,   984, -2012,
    6058,  6334,  1015, 22063, 18251,  4728,  4728,  1018,  4728,   937,
    4728,   905,   818, -2012, -2012,   334, -2012,   743,   897, -2012,
   -2012, -2012, -2012, 27213,  6543, -2012, -2012, 27273,  6200, -2012,
   -2012, 16211,  1000, -2012, 19879, -2012,  1315,   990, 19369, -2012,
   -2012, 27441, -2012, -2012,  1010, -2012, -2012, -2012,  1021, -2012,
   29605,  1170, 29839, -2012,  1032,  6543,    64,  1046,  1048, -2012,
     334,   334,  3468, -2012, -2012, -2012,  5081,  5835,  1094,  1166,
     173,  1166, -2012,   334,   334,    26, 23019,   681,  1166, -2012,
     334,   334,    26,   334, -2012,   334, -2012,  6071, -2012, -2012,
    1089,  1140,   990, 18419, 21532, 19199, -2012,  6187,   334,  4728,
   -2012,  2154,   656,  1116,  1208, 23019,  6543, -2012,  6543,   244,
   -2012, 14204, -2012,  1315,   990,  1139,  1208, 23019,  6543,   334,
   -2012, 29412, -2012, -2012, -2012, -2012,  1315, -2012, -2012, -2012,
   -2012,   990, -2012,  1314,  1068,  5754,  6543, -2012, 25203, -2012,
   -2012, -2012,  4576,    64, 23134,  1151,  5458, 25143, 18419, 17116,
   26025, -2012, 28636, -2012,  6543,  1166,    48,  1180, 23929, -2012,
    4869, 24445, -2012, 27501, 26025, -2012, -2012, 32106, -2012, -2012,
   -2012, -2012, -2012, -2012, 24445, -2012, -2012, 26529, 27501, 27501,
   15125,  2113,  2261, 24273,   186,  2367, -2012,   699,  1198,  1206,
   28636,   965,  1200, -2012, -2012,  1199,  1216,  1207, 29917,  1217,
    1224, 32106,  3468, 32106,  3468, -2012, -2012,  2852, -2012, -2012,
    8327,  3532, 29995,  8327,  3468, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012,  1242, 32106, -2012, -2012,
   24617, -2012, -2012, -2012, 32106, 32106, 32106, 32106, 32106, 32106,
   32106, 32106, 32106, 32106, 32106, 32106, 32106, 32106, 32106, 32106,
   32106, 32106, 32106, 30073, -2012,  8327,  5604, -2012, -2012,  1323,
   32106, -2012, -2012,  5458, 28686, -2012,  1245,  1265, -2012, -2012,
   -2012, -2012,  6543,  4512,   167,   763, -2012,  6543,   898, -2012,
    1074, -2012, 16392, 26193,  1127, 22063, -2012,  1315,   990,  1266,
   -2012,  1010,  4052,   475,   726, -2012,   803,   656,  1262,   334,
    3436,  1249,   898,  3436,  1285, -2012,   772, -2012, 15306, -2012,
   -2012, -2012,   806, 26025, -2012,  8080,  2963,  1303,  1305,  1310,
    1326,  1334,  1336, -2012, -2012,  1007,  1321, -2012,   829,  1321,
    5993, 25362,  1124, 17297, 24789,  1316, 32340,  1348, -2012, 28847,
   27441, -2012, -2012, 15487, -2012, 27669, -2012,  1315,  1315, 29062,
   -2012, -2012,  1010, -2012, -2012, -2012, 14016, 30151,  1464, 32106,
    5617,  1033,  1337, -2012,   334,   334,  1337,  1146, -2012,   334,
     334,  1346,  1337, -2012,   334,   334, -2012,  1321, -2012, 30229,
   16573, 27729, -2012, -2012,  6543, 22655,  1315,  1315, -2012, -2012,
    5993, -2012, 22240, -2012, 22240, -2012, 28475, -2012, -2012,  1337,
   17478, -2012, 27213, -2012, -2012, -2012,   198, 26589, 22417, -2012,
   -2012, -2012, -2012, -2012, 29362, -2012, -2012, 32418, -2012, 11048,
    5063, 29527, 29605,  1352,  1362, -2012, -2012,  1365, 29839,    84,
   -2012, -2012, -2012, 24273,  1378, -2012,   973, -2012,  3468,  5458,
    1372,  5081,   756,  1384,  1416,  1425,   802,  1428,  1430,  1432,
    1445,  1449,  1450,  7256,  5081, -2012, -2012, -2012,   334,  1433,
   28890, -2012, -2012,  1346,   244, -2012, -2012,    64,  1208, 25539,
   -2012, -2012,   244, -2012, -2012,    64, -2012, -2012,  5330,  6604,
    6071, -2012,  1212, -2012, -2012, -2012, -2012, 24273, 24273, -2012,
    1315,   334,  5458, 16754,  2801, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012,    64,  1208,  1476,  1456, -2012, -2012,  4728,  1486,
    1208, 23019, -2012, -2012,    64,  1208, -2012, -2012, 13689, -2012,
   -2012,  1315,  1315, -2012, -2012, -2012,   327,   916,   327,   656,
    1493, -2012, -2012, -2012, 22655,  1501,  1498, -2012, -2012,   848,
   25875, 18419, -2012,  1487, -2012, -2012, -2012, 27906,  1506,  1513,
    1150, 29212, 27959,  6543,  1166, -2012, -2012, -2012, -2012,  1520,
   26361,  1521,  1518,  1523, 16935,  1522,  1528,  1525,  1531, 32106,
    1537,  1538,  1539, 28019, 32106, -2012, -2012,  2412, -2012, -2012,
   -2012, 32106, -2012, 20824,  1381, -2012, -2012,   334,   334, -2012,
    1543,  1547, 29683, 29995,  1546, -2012, 29761,  8327, 32106,  1548,
   -2012,  1550, -2012, -2012,  5545, -2012,  1554, -2012,  5545, -2012,
   -2012, -2012, -2012,  1184,  1560, -2012, 29605, -2012,  1561,  1565,
   -2012,   751,   751,   751,   473,   473,   440,   440,   769,   769,
     769,   769,   854,   854,   513,   761,   778,   779,   821, 32106,
    1108, -2012,  5545, -2012,  6543, 13066,  1656, -2012, -2012, 22240,
    1166,  6813, -2012,  6543,  1572, 19031,  1573, -2012, -2012, -2012,
   -2012, -2012,  3436, -2012, -2012,  1661, 26757, 21001,  1731,  2121,
   -2012, -2012,   334,  1575,   334, -2012,   521,  1569,   921, 26025,
     929,  1562, -2012,   973, -2012, 24273, -2012, -2012, -2012,  1300,
    1321, -2012,   978,  1321, 25539, -2012, -2012,  1346, 25539, -2012,
    1346, -2012, 21178, -2012, 27213, -2012, -2012, 15668,  1581, 24961,
    1585,   818,  1589, 17659, -2012, -2012, -2012, -2012, -2012, -2012,
   18251, 21178, 14016,  1593,   815,  1598,  1602,  1606,  1607,  1611,
    1615,  1617, -2012,  2222,  4126, -2012,  5203, -2012,  7896, -2012,
   -2012, -2012, 25539, -2012, -2012, -2012, -2012, -2012, -2012, 25539,
   -2012, -2012, -2012, -2012, -2012, -2012, -2012,  1346, -2012,  1600,
   27273, 17297, -2012, -2012, -2012,  1337,  1315, -2012,  1250, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012, 21178, -2012,  6543,  5545,
   -2012,  1619,   313,  1616,  1365, -2012, 29605,  1631, -2012,  3106,
   32106, -2012, -2012,   995, -2012,  1635, 21178, -2012, -2012, 32106,
    1013,  1639,  1640,  1643,  1025,  1644,  1646,  1647,  1649,  1651,
    1653,  1313,  1321, -2012, -2012,  1483,  1321, -2012, -2012,  1568,
    1321, -2012, -2012, -2012, -2012, -2012, -2012,  5458,  1803,  1321,
     569, -2012,   818,  1279, -2012, -2012,    64,  1662, -2012, -2012,
    5330,   847,  6071,  5330, -2012, 25539,  1029,  1667,  1038,  1670,
   -2012,  1316,   315, -2012,    64, 18191, -2012,    64,  1208,   315,
   -2012,    64, -2012, -2012, -2012, -2012, -2012,   814, -2012,  3468,
   -2012, -2012, -2012, -2012, -2012, 22417,  1166, -2012, 21178,   227,
    1671, -2012, 22655,   227,  3468, -2012, 26817,   227, -2012, 32106,
   32106, 32106, -2012, -2012, -2012, -2012,  1672,  1673,  1677,  1686,
    1109, -2012,  1030, -2012, -2012, -2012, 32106, 32106,  1688, 29605,
   -2012,  1655, -2012, -2012,  1655,  1699, -2012, -2012, -2012, -2012,
    4754, -2012, -2012,  1296, -2012,    80, -2012,  1304, -2012, 30307,
   -2012,  1365,   227, -2012, -2012, 32106,  1335, -2012,   403, -2012,
   12875, -2012, 13257,  6543,  1695, -2012,   315,  1700,   898, -2012,
   -2012, -2012,  5458, 28187, 18359, -2012,   449,   501, 24273,  1682,
   -2012,  1682, -2012, -2012,   334,  1174, -2012,   521,  1569,  1569,
     208, -2012, -2012,  1701,  6543,  1705, -2012, -2012,  1706, -2012,
    1707, -2012, -2012, 25539, -2012, -2012,  1346, 25539, -2012,  1346,
    1713,  1716, -2012,  1717,  1702,  1714, -2012, -2012, -2012, -2012,
   -2012, -2012,  1720, 21178, 21178, -2012,  1726, -2012,  1603,  1321,
   -2012,  1650,  1659,  1321, -2012,  1315,  7045,  2835, -2012,   334,
     334, -2012, -2012, -2012,  3677,  1872,  7241, -2012, -2012,  1729,
    1730, -2012, -2012, -2012, 22240, -2012,   244,  1338, 32106, -2012,
   32106, -2012,  1735, -2012, 29839, -2012,   334, 21178,   334, -2012,
   -2012,  1793,  1321, -2012,  1829,  1321, -2012, -2012,  1837,  1321,
   25539, -2012, -2012,  1346, 25539, -2012, -2012,  1346, 25539, -2012,
   -2012,  1346,  1166, -2012,  1346, -2012, 30385, -2012, 32106, -2012,
   10070, -2012, -2012,  1047, -2012, -2012, -2012, -2012, -2012,   519,
   -2012, -2012, -2012, 18527,   315, -2012,    64, -2012, -2012,  1734,
    1736,  1738,   591, -2012, 22655, -2012, -2012, -2012, -2012,  1260,
    1737,  1746,  1054, -2012,  1751, -2012, -2012, -2012, -2012,  1869,
    1321, -2012, -2012, -2012, -2012, -2012, 29605,  1365, 30307,  1732,
    1733, -2012,  1797,  5545, -2012,  1797,  1797, -2012,  5545,  4797,
    5370, -2012, -2012, -2012,  1762, -2012, -2012,  6543, -2012, -2012,
   -2012,   744,   129, 14763,  1763,  1767, 22828,  1771,  1776,  2622,
    2701,  4376, 30463,  1779,  2382,  1781,  1782, 22828,  1784, -2012,
   -2012,    64, 32106, 32106,  1939,  1787,   616, -2012, 23413, 15849,
    1788,  1780,  1775, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   -2012, -2012, -2012,  1278,   395, -2012,   419, -2012,   395, -2012,
   -2012, -2012, -2012, -2012,  3468, -2012, -2012, 13828, 19709, -2012,
     509, -2012, -2012,  6543, -2012, -2012, -2012, -2012,  6543, -2012,
    5458, -2012,  1057, 26025,   898,   898,  1569,  1701,  1798,  1801,
     656,   165,  1789,  1777,   521, 18695, -2012,  1809,  1812, -2012,
   -2012, -2012, 21709, 21886, -2012, -2012, -2012,  1813,   334, 25539,
   -2012, -2012,  1346, 25539, -2012, -2012, 25539, -2012, -2012,  1346,
   32106, 32106,  1816,  1818, -2012,  1823, -2012, -2012,  3677,  4500,
    7676,  7896, -2012, -2012, -2012,  1824, -2012, -2012,  1826, -2012,
   -2012, -2012, -2012, -2012, -2012,  1830, 25539, -2012, -2012,  1346,
   25539, -2012, -2012,  1346, 25539, -2012, -2012,  1346,  1832,  1833,
    1840,   244, -2012,  1360, -2012,   134, -2012,   818,  1839, -2012,
    6813, -2012, -2012, -2012,  1853, -2012, -2012, -2012,  1828, 20417,
   -2012, -2012,  1851,  1852, -2012, -2012, 32106, -2012, 26817, 32106,
   25539, -2012, -2012,  1346,  1365,  1857, -2012, -2012, -2012,  1370,
   -2012,  5545, -2012,  5545, -2012, -2012, -2012,  8327,   -15,   127,
   -2012, -2012, -2012, -2012, 14763, 32106,  1858,  1943, 14581, 12467,
   -2012,  1838, -2012,  1843, 32106,  1845, 29605,  1846, 32106,  1847,
   -2012,  1850, 24273, 32106, -2012, 13448,  2108, -2012,  1859,    -7,
   -2012,    36,  1934,   345,   334, -2012,  1877,  1883, 22828, 22828,
   -2012, -2012,  1962, -2012, -2012,    21,    21,   836, 14392, -2012,
    1892,   167, -2012, -2012, -2012, -2012, -2012, -2012,  1884,  1894,
     521,   521,   208,  6543,   334, 30541, -2012,  1701, -2012, 18863,
   -2012, -2012, -2012,  1891, -2012,  1896,  1900, -2012,  1901,  1903,
    1905, -2012, -2012,  1908, -2012,  1909, -2012, -2012,  1913,   334,
    1914,  1921,  1922, -2012, -2012, -2012, -2012, -2012, 32106, -2012,
    6813, -2012,  1902, -2012,   721,   885,   976, 24273,   334,   334,
   18419,   334, 27501,  1904,   515,   526,  2529, 20647, -2012,   563,
    6543,   334, -2012, -2012, -2012, -2012,  1924,  1911, -2012, -2012,
    1377,  1394,  1928, -2012, -2012, -2012, -2012,  1780,  1929, 32106,
     290,  1932,   570, 20595, 27045,  1067,  1940, 22828,  1941, -2012,
   -2012, -2012, -2012,  1958, 22828, 32106,  1608,   416, -2012, 32106,
   29444, -2012, -2012,   575, -2012,  1365, -2012,  1072, -2012, -2012,
    1080,  1104,   661, -2012, -2012, -2012, -2012,    64,  2108,  1944,
   -2012, -2012, 32106, -2012,  1946,   973, -2012, 11646, 32106, 32106,
   -2012, -2012,   402,    21, -2012,   459, -2012, -2012, -2012,  1682,
     521,   334,  1701,  1701,   656,  1777, -2012, 29605, -2012,  1935,
   -2012,   334,   334, -2012, -2012, -2012,  1948,  1949, -2012, -2012,
   -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012, -2012,  1828,
    1828,  1828,  1135, -2012,  3452, 27501,  3452,   581, -2012, -2012,
   -2012,  8094, 32106,  7559,   401, -2012, -2012, -2012,   206,  1942,
    1942,  1942,  6543, -2012, -2012, -2012, -2012, -2012, -2012, -2012,
   22828, 22828,  1780, 22594,    72, 30619,  2041, 22828, -2012, 32106,
   -2012, 30697,  2051,  1947, 11832, 30775, 22828, 13448,  1780,   980,
    2676,  1950, 32106, -2012,  1972,    98, 22828, -2012, 22828, -2012,
    1973, -2012, 28247,  1953,   973,   672, -2012, -2012,  1978,  1312,
    1152, 22828,  1981, 22828, 22828, 22828,   898,  1701, -2012,  1985,
    1986, -2012,  1365,   334, -2012, -2012, -2012, -2012, -2012,   334,
     334,   334, -2012,   597,  1356,  1961,   624, -2012,   652, -2012,
    8094,   856, -2012,  7879,  8094, -2012,   334, -2012, -2012, -2012,
   -2012, -2012, -2012, -2012,  2073,  6300,    47, 17843, -2012, 22697,
   -2012,     8,  1175, 22828,  2076,   665,  1975,   452, 22828, 32106,
     980,  2676,  1967, 30858,  1219,  1315,  1980,   453,  2080, -2012,
   30936, -2012, -2012, -2012, -2012, 31014, 32106, 32106,  1780,  1976,
   18023, -2012, -2012, -2012, 28247,  1982,  4400, 28415,  3468, -2012,
    1990,  1983,   187, -2012, 32106,  8327, -2012, -2012, 32106,   395,
   -2012, -2012, -2012,  2010,   334,   334,  2014, -2012, -2012, -2012,
   -2012, -2012,  1356,  2584,   694, -2012, -2012, -2012, -2012,   334,
     334, -2012, -2012, -2012, -2012,  2011,  3452, 22828, -2012,   -25,
   -2012,    90, -2012, -2012, -2012,  2015,   549, -2012, -2012, 22828,
   -2012,    31, -2012, 22828, 32106,  2018, 31092, -2012, -2012, 31170,
   31248, 32106, 32106,  5993,  1780, -2012,   818, 31326, 31404, 22828,
    2004,   483,  2008,   517,  1780, -2012, -2012,  2026,   549,  1982,
   32106,  2025,  4287,  6028, -2012, -2012, -2012,  2027, -2012,  2088,
    2036,   689,  2032, -2012, -2012,  2037,  1181,   164,   334, -2012,
   -2012, -2012,  2038,  2039,  2043,  1836, -2012, -2012,   334, -2012,
   -2012, -2012, -2012, -2012, 32106, -2012, 32106, -2012, -2012,  1488,
   20068, 20246, -2012, 22828, -2012, -2012,  1780, -2012, -2012,  1780,
    2030,   577,  2031,   650,  1780, -2012, -2012,   656, -2012,  1780,
   -2012,  1780, -2012,  2042, 31482, 31560, 31638, -2012,  1488,  2070,
   -2012,    64,  7648,  5330,   187,  2071, 32106,  2050,   187,   187,
   -2012, -2012, 22828,  2165, -2012,  1885,  1321, -2012, -2012,  1356,
   -2012, -2012,   972,  2081,  1488, -2012, -2012, -2012,  2083, 31716,
   31794, 31872, -2012, -2012,  1780, -2012,  1780, -2012,  1780,  2087,
      64, -2012,  2082,   973,  2089, -2012,   697, -2012, -2012, 22828,
   25539, -2012, -2012,  1346, -2012, 12069, 22828, -2012,   972, -2012,
   -2012,  1780, -2012,  1780, -2012,  1780, -2012, -2012,   973,  2098,
   -2012,  2074,   973, -2012,  2100, -2012, 22828, -2012, 12228, -2012,
    1414, 32106, -2012,  1189, -2012, -2012,   973,  6543,  2101,  2075,
   -2012, -2012,  1197, -2012, -2012,  2079,  6543, -2012, -2012
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
     882,     0,   889,   892,     0,   882,     3,   883,   884,   892,
     897,   896,    19,    24,    25,    11,    12,    13,    14,    15,
      16,    17,    18,    20,    23,   895,     0,   890,   893,     1,
       3,     0,   885,   892,     0,   888,   892,   162,     0,   858,
     882,   524,   525,   526,   527,   528,   529,   530,   531,   532,
     513,   515,   514,   516,     0,     0,     0,   534,   536,   563,
     537,   564,   540,   541,   561,   562,   535,   559,   560,   538,
     539,   542,   543,   544,   545,   546,   547,   548,   549,   550,
     551,   552,   553,   554,   555,   556,   557,   558,   565,   566,
     882,   568,   642,   643,   646,   648,   644,   650,     0,     0,
       0,     0,     0,    19,   613,   619,   836,   108,     0,    22,
       0,   508,   106,   107,     0,   857,    21,   898,   508,   837,
       0,     0,   446,   756,   448,   460,   880,   447,   482,   483,
       0,     0,     0,     0,   596,   882,   512,   517,   508,   519,
     882,   581,   533,   567,   492,   573,   882,   494,   591,   493,
     882,   610,   616,   595,   622,   634,   882,   639,   640,   623,
     693,   449,   450,     4,   844,   860,     0,     0,   882,   922,
     882,   508,   940,   941,   942,   508,     0,  1141,  1142,     0,
       0,   887,   891,     0,     0,     0,     0,     0,     0,   104,
     105,     0,    29,    31,     6,    10,    27,     7,     8,     9,
       0,     0,   508,     0,     0,   109,   110,   111,   112,   166,
      86,    30,    87,    26,    48,    85,   113,     0,     0,     0,
     128,   130,   134,   137,   140,   145,   148,   150,   152,   154,
     156,   158,   169,     0,   163,   164,   168,    32,     0,     4,
       3,   859,     0,   508,   847,     0,   645,   647,   649,   651,
       0,   508,   882,   696,   641,   569,   810,   805,   795,     0,
     845,     0,     0,   524,   838,   842,   843,   839,   517,   840,
     841,   508,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   614,   617,   882,   508,   508,   106,   508,     0,   741,
       0,  1164,     0,   509,   508,   519,   499,   573,   500,   599,
     501,   882,   610,   603,   624,   882,   625,     0,     0,   737,
     742,   727,   731,   508,   743,  1109,  1110,   508,   744,   746,
     899,   508,     0,  1143,   596,   502,   503,   882,   508,   929,
     948,   508,  1148,  1140,  1138,  1146,   443,   442,     0,   177,
     762,   176,     0,   451,     0,     0,     0,     0,     0,   458,
     882,   882,     0,   441,  1021,  1022,     0,     0,   481,   880,
     882,   880,   902,   882,   882,   491,   508,   882,   880,   962,
     882,   882,   490,   882,   982,   882,   959,     0,   589,   590,
       0,     0,   508,   508,   508,   508,   461,   880,   882,   520,
     582,     0,   611,     0,   863,   508,     0,   510,     0,   756,
     462,   596,   574,   592,   882,     0,   863,   508,     0,   882,
     522,   882,   583,   584,   578,   495,   593,   497,   498,   496,
     598,   882,   612,   606,     0,   626,     0,   828,   508,   861,
     921,   923,   882,     0,   508,     0,     0,   596,   508,   508,
     508,  1152,   596,  1155,     0,   880,   880,     0,   508,    93,
       0,   508,   102,   508,   508,   113,    88,     0,    38,    42,
      43,    39,    40,    41,   508,    91,    92,   508,   508,   508,
     508,   109,   110,   508,     0,     0,   198,     0,     0,   749,
     596,   640,     0,   751,  1138,  1162,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    28,    60,     0,    66,    67,
     162,     0,     0,   162,     0,   178,   179,   180,   181,   182,
     183,   184,   185,   186,   187,   188,   176,     0,   174,   175,
     508,    96,    89,    90,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   894,     0,     0,   829,   220,   435,
       0,   471,   472,     0,   596,   834,     0,     0,   791,   793,
     792,   794,     0,     0,   787,     0,   776,     0,   785,   797,
       0,   694,   508,   508,  1164,   509,   573,   599,   882,     0,
     743,   744,   696,   613,   619,   697,   698,   699,     0,   882,
       0,   808,   796,     0,     0,   161,     0,   620,   508,   802,
     752,   801,     0,   508,   754,     0,     0,     0,     0,     0,
       0,     0,     0,   900,   927,   882,   938,   946,   951,   957,
       0,   508,     0,   509,   508,   613,     0,     0,  1150,   596,
     508,  1153,  1062,   508,  1112,   509,   505,   506,   507,   508,
    1117,  1106,  1107,  1115,  1061,     2,   508,     2,   107,     0,
     882,   882,  1164,  1002,   882,   882,  1164,   882,  1018,   882,
     882,  1085,  1164,  1067,   882,   882,  1076,  1083,   735,     0,
     508,   508,   604,  1111,   745,   509,   600,   601,   605,   606,
       0,   469,   508,  1156,   508,  1127,   509,  1133,  1128,  1164,
     508,  1121,   508,  1130,  1122,     2,  1164,   508,   508,   931,
     950,  1139,   504,  1144,   596,   930,   949,     0,     2,    29,
       0,     0,   762,    30,     0,   760,   763,  1162,     0,     0,
     769,   758,   757,   508,     0,   865,     0,     2,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   905,   965,   989,   882,   487,
       0,   901,   910,  1052,   756,   903,   904,     0,   863,   508,
     961,   970,   756,   963,   964,     0,   981,   983,     0,     0,
       0,   477,   882,   875,   877,   876,   878,   508,   508,   580,
     509,   579,     0,   508,     0,  1145,  1149,  1147,   459,   511,
     597,   834,     0,   863,     0,     0,   452,   463,   521,     0,
     863,   508,   607,   834,     0,   863,   806,   523,   576,   577,
     575,   594,   609,   608,   615,   618,   613,   619,   637,   638,
       0,   807,   711,   747,   509,     0,   712,   714,   716,     0,
     508,   508,   862,     0,   433,   491,   490,   596,   106,     0,
       0,   508,   508,     0,   880,   454,     2,   455,   886,     0,
     508,     0,     0,     0,   508,     0,     0,     0,     0,     0,
       0,     0,     0,   508,     0,   127,   126,     0,   123,   122,
      33,     0,    34,   508,   882,   750,  1031,   882,   882,  1040,
       0,     0,     0,  1163,     0,   189,     0,   162,     0,     0,
      56,     0,    57,    64,     0,    63,     0,    59,     0,    58,
      62,   195,   194,     0,     0,    55,   762,   170,     0,     0,
     129,   131,   132,   133,   135,   136,   138,   139,   143,   144,
     141,   142,   146,   147,   149,   151,   153,   155,   157,     0,
       0,   165,     0,    35,     0,     0,   436,   479,   474,   508,
     880,     0,   834,     0,     0,     0,     0,   790,   789,   788,
     782,   518,     0,   780,   798,   571,   508,   508,   107,   882,
     745,   695,   882,     0,   882,   687,   696,   696,     0,   508,
       0,     0,   445,     0,   621,   508,   753,   755,   928,   882,
     939,   947,   952,   958,   508,   932,   934,   936,   508,   953,
     955,   698,   508,  1119,   508,  1129,  1120,   508,   106,   508,
       0,   611,     0,   509,     2,     2,  1151,  1154,  1108,  1113,
     509,   508,   508,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1086,     0,   882,  1165,  1072,  1071,   883,  1005,
    1023,  1073,   508,  1000,  1009,   733,  1003,  1004,   734,   508,
    1016,  1027,  1019,  1020,   736,  1069,  1070,  1084,  1157,     0,
     508,   509,  1114,  1118,  1116,  1164,   602,   637,     0,   729,
     728,   732,   738,  1125,  1132,  1126,   508,   739,     0,     0,
     771,   161,     0,     0,  1162,   768,  1163,     0,   764,     0,
       0,   767,   770,     0,     2,     0,   508,   473,   475,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   882,   915,   919,   960,   882,   975,   979,   987,   882,
     998,   907,   967,   991,   906,   966,   990,     0,     0,  1047,
       0,  1053,  1054,     0,   485,   866,     0,     0,   486,   867,
       0,     0,     0,     0,   478,   508,     0,     0,     0,     0,
     476,     0,   882,   868,     0,     0,   834,     0,   863,   882,
     869,     0,   630,   632,   628,   652,   924,   882,   943,     0,
     715,   717,   434,  1064,  1063,   508,   880,   456,   508,    94,
       0,    98,   508,   103,     0,   101,   508,     0,   117,     0,
       0,     0,   121,   125,   124,   199,     0,     0,     0,     0,
    1035,  1034,   883,  1036,  1032,  1033,     0,     0,     0,   762,
     114,  1162,   191,   190,  1162,     0,   167,    50,    51,    83,
       0,    83,    83,     0,    71,    73,    53,     0,    49,     0,
      52,  1162,    97,    99,   160,     0,     0,   439,     0,   229,
       0,   222,   508,     0,     0,   835,   882,     0,   795,   825,
     820,   821,     0,   509,     0,   816,     0,     0,   508,   778,
     777,   778,   572,   570,   882,  1072,   690,   696,   696,   696,
       0,   706,   705,  1162,     0,     0,   811,   809,     0,   846,
       0,   804,   803,   508,   933,   935,   937,   508,   954,   956,
       0,     0,   721,     0,   722,   723,  1123,  1131,  1124,  1134,
    1135,  1136,     0,   508,   508,     5,     0,  1080,   882,  1012,
    1015,   882,   882,  1079,  1082,   508,     5,     0,  1068,   882,
     882,  1007,  1025,  1074,     0,   107,     0,  1006,  1024,     0,
       0,  1158,   740,   470,   508,     5,   756,     0,     0,   772,
       0,   773,     0,   765,     0,   759,   882,   508,   882,     5,
     480,   882,   916,   920,   882,   976,   980,   988,   882,   999,
     508,   908,   911,   913,   508,   968,   971,   973,   508,   992,
     994,   996,   880,   488,  1048,  1060,     0,  1059,     0,  1051,
       0,   871,   984,     0,   586,   585,   588,   587,   835,   882,
       3,   872,   813,     0,   882,   870,     0,   835,   873,     0,
       0,     0,   882,   713,   508,   748,   457,     5,    95,  1065,
       0,     0,     0,    44,     0,   118,   120,   119,  1044,   882,
    1043,  1046,  1038,  1037,   116,   115,   762,  1162,  1163,     0,
       0,    70,    80,     0,    74,    81,    82,    65,     0,     0,
       0,    61,   197,   196,     0,   159,    36,     0,   437,   221,
     223,     0,     0,   508,     0,     0,   360,     0,     0,     0,
       0,     0,   200,     0,     0,     0,     0,   360,     0,   408,
     407,     0,   172,   172,   414,   613,   619,   217,   508,   508,
       0,   201,     0,   228,   202,   203,   204,   205,   206,   207,
     208,   209,   361,     0,   375,   210,   381,   383,   386,   211,
     212,   213,   214,   215,     0,   216,   224,   596,   508,   226,
       0,   848,   835,     0,   784,   823,   800,   817,     0,   818,
       0,   819,     0,   508,   795,   795,   696,  1162,     0,     0,
     702,   696,     0,   707,   696,     0,   444,     0,     0,   925,
     944,  1159,   508,   508,  1137,     5,     5,     0,   882,   508,
    1008,  1010,  1011,   508,  1026,  1028,   508,  1075,  1077,  1078,
       0,     0,   106,     0,     5,     0,  1001,  1017,     0,     0,
       0,     0,  1013,  1029,   730,     0,   453,   775,     0,   879,
     761,   766,   864,     5,   881,     0,   508,   909,   912,   914,
     508,   969,   972,   974,   508,   993,   995,   997,     0,     0,
       0,   756,  1049,     0,  1055,     0,  1056,  1057,     0,     3,
       0,   815,   835,   874,     0,   652,   652,   652,   635,   508,
     718,   719,     0,     0,  1066,   100,     0,    37,   508,     0,
     508,  1039,  1041,  1042,  1162,     0,   193,   192,    84,     0,
      72,     0,    78,     0,    76,   171,   440,   162,     0,     0,
     348,   349,   225,   227,   508,     0,     0,     0,   508,   508,
     344,     0,   342,     0,     0,     0,   762,     0,     0,     0,
     339,     0,   508,     0,   387,   508,     0,   173,     0,     0,
     415,     0,     0,     0,   882,   233,     0,     0,   360,   360,
     366,   365,   360,   377,   376,   360,   360,     0,   596,   438,
       0,   787,   822,   824,   799,   779,   783,   781,     0,     0,
     696,   696,     0,     0,   882,     0,   701,  1162,   812,     0,
     926,   945,   725,   724,   726,     0,     0,  1161,     0,     0,
       0,     5,     5,     0,  1088,     0,  1160,   774,     0,   882,
       0,     0,     0,   917,   977,   985,   489,  1050,     0,   852,
       0,     4,     0,   854,   882,   882,   882,   508,   882,   882,
     508,   882,   508,     0,     0,     0,   666,   596,   653,     0,
       0,   882,    54,    47,    45,    46,     0,     0,    68,    75,
       0,     0,     0,   352,   353,   350,   351,   242,     0,     0,
     244,   448,   243,   596,   508,     0,     0,   360,     0,   327,
     329,   328,   330,     0,   360,   200,   282,     0,   275,     0,
     200,   345,   343,     0,   337,  1162,   346,     0,   341,   340,
       0,     0,     0,   396,   397,   398,   399,     0,   389,     0,
     390,   354,     0,   355,     0,     0,   380,     0,     0,     0,
     369,   379,     0,   360,   382,     0,   384,   406,   850,   778,
     696,   882,  1162,  1162,   704,   707,   685,   762,   708,     0,
     814,   882,   882,  1014,  1030,  1081,     0,     0,  1087,  1089,
     464,   468,   918,   978,   986,  1058,     4,   832,   856,   635,
     635,   635,     0,   627,   666,   508,   666,     0,   665,   664,
     660,     0,     0,     0,     0,   667,   668,   670,   882,   682,
     682,   682,     0,   661,   678,   467,  1045,    69,    79,    77,
     360,   360,   245,   596,     0,     0,   263,   360,   331,     0,
     332,     0,   271,     0,   200,     0,   360,   508,   283,     0,
     309,     0,     0,   338,     0,     0,   360,   359,   360,   400,
       0,   391,   508,     0,     0,     0,   219,   218,   362,     0,
       0,   360,     0,   360,   360,   360,   795,  1162,   686,     0,
       0,   703,  1162,   882,   466,   465,  1090,  1091,   833,   882,
     882,   882,   636,     0,   674,   640,     0,   680,     0,   662,
       0,     0,   684,     0,     0,   655,   882,   654,   671,   683,
     672,   673,   679,   358,   234,     0,     0,     0,   256,   360,
     236,     0,     0,   360,   265,   280,   291,   285,   360,   200,
       0,   295,     0,     0,     0,   322,   286,   284,   273,   276,
       0,   333,   334,   335,   336,     0,     0,   200,   310,     0,
       0,   239,   357,   388,   508,   394,   401,   509,   405,   356,
       0,     0,   416,   367,     0,   162,   378,   371,     0,   372,
     370,   385,   786,     0,   882,   882,     0,   689,   631,   633,
     629,   657,     0,   882,     0,   675,  1100,   677,  1092,   882,
     882,   659,   681,   663,   656,     0,     0,   360,   251,   246,
     249,     0,   248,   255,   254,     0,   882,   258,   257,   360,
     267,     0,   264,   360,     0,     0,     0,   272,   277,     0,
       0,     0,   200,     0,   296,   323,   324,     0,     0,   360,
       0,   312,   313,   311,   314,   279,   347,     0,   882,   394,
       0,     0,     0,   882,   402,   403,   404,     0,   409,     0,
       0,     0,   417,   418,   363,     0,     0,     0,   882,   692,
     688,   709,     0,     0,     0,  1096,  1095,  1097,   882,   658,
    1093,  1094,   669,   235,     0,   253,     0,   252,   238,   259,
     508,   508,   268,   360,   269,   266,   281,   294,   292,   288,
     300,   298,   299,   297,   301,   278,   325,   326,   293,   289,
     290,   287,   274,     0,     0,     0,     0,   241,   259,     0,
     395,     0,  1096,   883,   416,     0,     0,     0,   416,     0,
     368,   364,   360,     0,   691,   882,  1103,  1105,  1098,     0,
     247,   250,   882,     0,   260,   430,   429,   270,     0,     0,
       0,     0,   321,   319,   316,   320,   317,   318,   315,     0,
       0,   392,     0,     0,     0,   410,     0,   419,   373,   360,
     508,  1099,  1101,  1102,   676,     0,   360,   237,   882,   308,
     306,   303,   307,   304,   305,   302,   240,   393,   422,     0,
     420,     0,   422,   374,     0,   232,   360,   230,     0,   423,
       0,     0,   411,     0,  1104,   231,     0,     0,     0,     0,
     424,   425,     0,   421,   412,     0,     0,   413,   426
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
   -2012,  -470,   -28,  -221,   520, -2012,    -3,   868,  2530,   155,
    -204, -2012,  -141, -2012,   664, -2012,  -834, -1102, -2012,   379,
    5366, -2012,  7180, -2012,  -100, -2012,  1811,   171,  1079,  1083,
     931,  1099,  1745,  1747,  1750,  1753,  1749, -2012,  -226,  -243,
    -490, -2012,  1742, 10259,   833, -2012,  2086, -2012, -2012,  -172,
    2568, -1229,  3110, -2012,    29, -2012,  1062,    68, -2012, -2012,
     659,   169, -2012, -1930, -2011,   385,   135, -2012, -2012,   647,
     404, -2012, -1279, -1270,   325, -2012, -2012, -2012,   196, -1386,
   -2012, -2012, -1455,   496, -2012, -2012, -2012, -2012, -2012,   -37,
   -1426, -2012, -2012, -2012, -2012, -2012,   221,   514,   516,   307,
   -2012, -2012, -2012, -2012,  -801, -2012,   146,    85, -2012,   230,
   -2012,  -293, -2012, -2012, -2012,  1110, -1033,   733,  -199, -2012,
    -134, -1505,    16,  6643,   735,   736, -2012,  -127, -2012, -2012,
      24, -2012,   350,  2683,  1886,  -361,  4474,  9934,  -449,    40,
     -26,   924,  2341,  2656, -2012, -2012,  2255, -2012,   190,  4990,
   -2012,  2201, -2012,    95, -2012, -2012,  3681,   845,  5959,  3064,
     -23,  1959,  -103, -2012, -2012, -2012, -2012, -2012,  -832,  9063,
    8775, -2012,  -188,  -104, -2012,  -815, -2012,   283, -2012,   152,
     604, -2012,  -234,  -306, -2012, -2012, -2012, -2012,  -195,  9532,
   -1108,   837,   512,  1173, -2012,  -456,  -486,  1397,  4069,  2320,
    -643,  -231,   865,   393,  -439,  -324,  -326,  -629,  1284, -2012,
    1645,   226, -1227,  1411, -2012, -2012,   674, -2012, -1231,  -156,
     -10,  -672, -2012,  -164, -2012, -2012, -1111, -1164, -2012, -2012,
   -2012, -2012, -1065, -2012,  -681, -1151,   -30, -2012, -2012, -2012,
   -2012, -2012, -2012,  -180, -1172,  -550, -1899,     4,  2028,  1899,
       2,   622,  2330, -2012,  3414,   -79,  -329,  -316,  -300,    34,
     -63,   -62,   -58,  1202,   -70,   -65,   -55,  -264,   478,  -255,
    -252,  -250,   541,  -244,  -220,  -203,  -290,  -635,  -585,  -580,
    -274,  -222,  -570, -2012, -2012,  -807,  1495,  1496,  1497,  1215,
   -2012,   780,  8024, -2012,  -571,  -546,  -517,  -500,  -509, -2012,
   -1846, -1949, -1883, -1882,  -578,  1415,  -232,  -271, -2012,   -69,
      -6,   -68, -2012, 10933,  2444,  -694,  -515
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,  1011,    31,   427,   338,   210,   211,   116,   117,  1470,
     212,   213,   214,  1402,  1403,   215,  1213,  1214,  1215,  1422,
     216,   217,   218,   219,   220,   474,   475,   221,   222,   223,
     224,   225,   226,   227,   228,   229,   230,   231,   232,  1072,
     233,   234,   235,   476,  1668,   517,   342,   519,   237,   903,
    1471,  1472,  1473,  1474,  1475,  1230,  1231,  2235,  1476,  1477,
    1778,  2070,  2071,  1988,  1989,  1990,  2203,  2204,  1478,  1797,
    1798,  2095,  1910,  1911,  2016,  1479,  1480,  1481,  1482,  1483,
    1939,  1943,  1686,  1678,  1484,  1485,  1685,  1679,  1486,  1487,
    1488,  1489,  1490,  1491,  1492,  1818,  2111,  1819,  1820,  2025,
    1493,  1494,  1495,  1671,  2121,  2122,  2123,  2260,  2272,  2149,
    2150,   433,   434,   935,   936,  1228,   119,   120,   121,   122,
     123,   124,   289,   322,   127,   128,   129,   130,   358,   359,
     436,   415,   291,   479,   292,   133,   480,   135,   136,   268,
     294,   295,   140,   141,   142,   254,   143,  1253,   296,   325,
     146,   382,   147,   326,   391,   298,   577,   300,   327,   238,
     152,   153,   303,   154,   820,  1391,  1389,  1390,  1748,   304,
     305,   157,   158,  1392,  1758,  1884,  1885,  1886,  2054,  2055,
    1759,  1966,  1978,  1887,   159,  1259,  1516,   252,  1262,   306,
    1263,  1264,  1706,  1013,   826,  1283,   307,   308,   827,   310,
     311,   312,   829,   600,   601,   343,   716,   717,   718,   719,
     720,   565,  1514,   566,  1251,  1249,   950,   567,   591,   592,
     569,   602,   161,   257,   258,   162,  1244,  1245,  1246,  1247,
       4,     5,  1378,  1379,   941,  1501,   163,   555,   556,   393,
     405,   799,   164,   346,   165,   771,  1073,   788,  1380,     7,
       8,    26,    27,    28,   166,   773,   362,   363,   364,   774,
     168,   169,   170,   171,   172,   173,   174,   367,   775,   369,
     370,   371,   776,   373,   374,   375,  1029,   653,   654,   655,
    1030,   376,   658,   659,   660,   875,   876,   877,   878,   752,
    1123,  1368,   313,  1613,   662,   663,   664,   665,   666,   667,
    2057,  2058,  2059,  2060,   640,   314,   315,   316,   317,   483,
     333,   177,   178,   179,   319,   884,   668
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      23,   652,    38,   938,   399,   138,    23,  1504,   239,    32,
     896,  1015,   138,   904,   481,   856,   722,   656,   547,   596,
     579,   779,   673,  1077,  1515,   176,   869,   733,    23,  1682,
      23,   589,   176,    23,  1383,    23,   595,    23,   278,  1059,
     734,  1060,  1673,   279,   435,   275,   276,   125,  2073,   332,
     277,  1083,   361,   280,   125,   131,   735,   594,   588,  2078,
    1499,  1016,   131,   489,  1217,   167,  1017,  1193,   241,   657,
     260,   137,   167,   758,   138,   796,  1018,   542,   137,  1031,
    1507,   687,  1672,  1074,  1387,   693,  2072,   282,   456,   568,
    2079,  2145,   736,  1053,   176,    23,    23,    23,  1226,  1822,
    1019,   737,   793,  2132,   738,    23,   739,   443,  1424,   551,
    1142,    23,   740,  2153,   805,    23,   125,  1456,   522,   523,
    2144,   344,  1149,    23,   131,   543,   148,  1824,    23,  1020,
     349,    23,   176,   148,   167,    23,   741,  1035,  2202,  1781,
     137,  1038,  1034,  1781,  1781,   438,  1021,  1044,  1041,  1517,
     386,     9,   329,   742,   400,   347,  1985,  1986,    25,  1330,
    1781,   240,   344,  2132,    25,   365,  -826,  2202,   394,  2133,
    2134,  1502,   406,    23,  1062,  1702,    23,  1023,  1773,  1088,
    2115,  1067,  1985,  1986,     1,   757,  1823,   522,    25,  1833,
     672,    25,   765,  2238,    29,   148,   429,  1638,  1639,  1496,
    -827,  2080,  -863,   610,  1825,   181,    23,  2137,   611,    36,
     607,   608,   549,  1429,   792,   609,   846,   631,   612,  1507,
    1134,   144,  2074,  1657,  2154,  1066,   804,  1598,   144,  2133,
    2134,   478,  1140,   403,     2,  1327,  1604,   176,  1068,   435,
     240,   345,    23,  -863,   806,     1,  2192,  2072,  1987,     3,
      23,  1430,     1,   256,   259,  1079,  1080,  1086,   624,   552,
    1456,  1236,   821,   443,  2146,  2147,   148,  2137,   435,    23,
      23,  1643,  2078,   251,  2020,    32,  1081,  1221,   745,     1,
     435,   348,    23,  1696,  1697,     1,   278,    23,    23,  2198,
     144,   279,  1365,   275,   276,     2,  1832,   244,   277,  1672,
    1835,   280,     2,   240,   673,   240,   329,   833,  2078,   852,
       3,    23,   947,   900,   196,    23,   786,     3,  1367,  1602,
    1775,    23,  1640,   104,   105,    23,  1630,  1632,  1634,     2,
      32,   445, -1163,   336,   446,     2,  2198,   713,  1137,  1139,
     438,   428,    23,  1261,     3,   750,   948,   949,   585,    23,
       3,  1690,   673,    23,    23,    56,   864,   361,   699,  2119,
     964,   144,   705,   749,   673,   754,   846,   865,   866,   438,
    1799,   332,   762,   522,    23,   725,  1168,  1015,   750,   110,
    1332,   438,    23,  1412,    23,  1413,  1799,   963,  1031,   995,
     649,   329,  1078,    23,     1,    23,  1198,  1205,    23,    37,
     564,   815,  1091,  1199,   332,    23,     1,  1673,   438,   339,
    1642,  1019,  1781,     1,  1709,  1092,  1707,   797,  1184,   329,
     340,  1053,    23,    23,   910,    23,  1049,  1016,   657,    23,
    1124,  1093,  1017,    23,   568,   800,   341,   568,  1128,   733,
    1020,    23,  1296,   595,     2,   348,   428,  1672,   613,   845,
     847,  1742,   734,  1061,   245,  1313,     2,  1021,  1328,     3,
    1064,   148,   832,     2,  1680,  1384,  1019,  1094,   735,   689,
     835,     3,   672,   696,    35,    36,  1095,   478,     3,  1096,
     478,  1097,  -830,   337,  1941,  1071,  1329,  1098,  1681,    23,
     148,    23,   447,   478,    23,  1020,    36,  1330,    23,  1676,
     724,    23,   148,  -882,   736,  1034,  1284,  1419,  1148,   250,
    1420,  1099,  1021,   737,  1680,  1826,   738,   478,   739,  1942,
     672,  1922,  1085,   815,   740,  1284,  1915,  1434,  1100,   148,
    1267,  1260,   672,   403,  1293,  1294,  1272,  1683,  1681,   491,
    1322,  1945,   348,    23,   492,  1507,   624,   493,   741,   478,
      23,   794,   494,   795, -1021,  1295,   144,  1677,   954,    23,
      23,  1684,  1007,   256,    23,   742,  1913,  1362,  1975,  1522,
    1417,  1921,   672,   281,   105,  1976,  1512,  1437,  1126,    23,
    1284,   256,   954,   672,  2127,   144,  1916,    23,   261,  1629,
      23,  1917,  1842,  1843,  1977,   482,  1438,   144,     1,   844,
    1284,   574,    23,    23, -1021,    32,   262,   624,    32,   368,
    1325,  1779,  1946,  1144,  1337,  1779,  1800,    23,    23, -1021,
    1147,   528,   529,  1508,   144,  1151,   871,   871,     1,   631,
    1339,    34,  1800,  -427,  -427,  2003,  2086,  2098,  1799,   699,
     705,    32,  1509,    23,   624,  1781,  1781,    23,     2,  2010,
    2015, -1021,   525,   536,   537,   180,   899,   871, -1021,   526,
     527,   610,   271,     3,  1111,  1114,   611,  2174,   607,   608,
       1,    23,   372,   609,   435,  1510,   612,    23,     2,  1015,
      -5,  1564,  1284,  1437,  1313,  2002,  -831,   282,  -700,   350,
     745,   871,  1505,     3,  1511,  -700,   911,   912,   913,  1261,
     351,  2176,  1689,  1224,   585,   275,   276,    23,  1879,   713,
     277,  1201,  1397,   350,  1204,  2042,  -427,   944,   946,  1880,
       2,   672,   953,  1625,   995,    23,    23,  1365,    23,  1016,
    2089,  2090,  1947,    32,  1017,     3,  1311,  1892,  1317,   351,
      23,    23,   644,  1366,  1018,   564,    32,   352,   564,   871,
    1333,   871,  1312,  1367,  1318,  1892,  1893,   624,  1608,  1178,
       1,  2209,  1287,   353,  1182,    23,    23,    23,  1923,  1270,
    2088,  1976,  1265,   786,  1969,   438,   377,   699,   705,    23,
     673,    23,  1200,    32,   271,   398,  1125,  1624,  2105,   574,
    2051,   -25,  1545,  1548,  1129,   631,   568,  1770,  2062,  1771,
       1,  1091,  1310,   329,   657, -1022,   657,  1284,  1284,   246,
       2,  1240,   247,   248,  1092,   249,   968,  2063,   995,   970,
     574,  1143,  -502,  1699,   871,     3,  1976,   424,  1614,  1055,
    1093,  1928,    32,  1150,  2211,   746,  1917,  1535,  1536,   871,
      23,   849,  2031,    32,   853,  2064,   855,  2032,  1167,   759,
       2,  1284,    32,   750,   595, -1022,   426,   858,  2084,  2187,
     860,   861,   862,  2165,  2188,     3,  1094,  2251,  2138,   870,
   -1022,    24,  2252,   871,   428,  1095,   149,    24,  1096,   450,
    1097,  1573,     1,   149,  1800,   453,  1098,  2139,  1869,  1059,
    1060,    23,   454,     1,   271,    23,   148,   457,   747,   522,
     672,    24, -1022,   713,    24,   530,   531,   672,   242, -1022,
    1099,  1235,  1637,   908,   368,   138,   681,   682,    50,    51,
      52,    53,    54,  1015,   428,   495,   613,  1100,   750,    23,
    1767,    23,     2,   951,  1061,   176,   524,   952,    23,   176,
      23,   538,   972,     2,  1550,   149,   973,     3,   672,    23,
      50,    51,    52,    53,    54,   139,   539,   125,     3,  1393,
    1693,  1241,   139,   541,     1,   131,    24,    24,  1386,   540,
     759,   624,  1104,  1016,   750,   167,   974,   836,  1017,  -882,
     975,   137,   320,  1032,     1,   613,  -882,   647,  1296,   546,
    1810,   144,   532,   533,    24,     1,   976,   988,  1166,  1435,
     544,   750,  1566,    50,    51,    52,    53,    54,  1571,    23,
    1159,  1240,   240,  1849,     2,  1135,   149,   613,  1158,   750,
     574,    23,  1159,    23,   139,    23,   613,  1805,   750,     3,
      32,   545,   378,   379,     2,   380,   148,  1959,  1960,  1961,
     574,   381,   534,   535,    24,     2,   548,    24,   574,     3,
    -507,     1,  1870,  1111,  1114,     1,  1985,  1986,   389,  1963,
       3,  1968,   550,  1518,  1519,    23,    23,   607,   608,     1,
     683,   684,   609,   713,  1695,  1872,    23,   490,  -849,  1405,
    1406,  1407,  -506,   590,   271,  1568,     1,  1569,   563,  1227,
     593,  1266,  -882,  1061,   657,   952,  1414,  1415,  1237,  1268,
     196,     2,   595,   952,   595,     2,   614,   564,   615,     1,
    1240,  1924,     1,    24,    23,   616,     3,  2205,  2206,     2,
       3,   586,    50,    51,    52,    53,    54,    23,   617,    23,
      23,   144,    32,   618,     3,    32,     2,  1789,  1790,   176,
     672,  1791,  1792,  1871,   880,   881,  1277,  1772,  1949,  1950,
     750,     3,   619,   627,   632,  1371,    23,   644,    24,     2,
     669,  1241,     2,  -505,  1908,  1336,   699,   705,   645,   975,
    1396,    23,   107,  1381,     3,   984,  1385,     3,   695,   750,
    1388,   428,    24,   814,   105,   750,    24,    50,    51,    52,
      53,    54,   721,   759,    32,   708,   713,   750,   873,  1374,
     723,  1032,   750,   871,   574,   647,  1498,    23,  1376,   112,
     113,   149,   871,    24,   726,  1135,   727,   389,  1952,   750,
     730,  1112,  1115,  1326,  1617,     1,   176,  1694,  1618,   713,
      23,   975,    50,    51,    52,    53,    54,  1904,   176,    23,
     149,   871,  1925,  1170,   955,   424,   871,   746,   125,  1240,
    1926,   107,   149,  2043,   975,    24,   131,   777,  2046,   246,
    1241,    23,   247,   248,    24,   249,    24,  1736,   748,   107,
    1311,  1317,   137,   347,  1927,     2,    24,   873,   871,   149,
     481,   750,   871,  1225,  1113,  1116,  1312,  1318,   112,   113,
       3,     1,   791,   818,    24,   957,   823,   644,   886,   647,
     242,    10,  1061,   344,    23,  1962,   112,   958,   778,   975,
     747,    23,    24,    23,  1039,   803,   107,  -504,   647,   649,
     330,  1698,  2036,   683,  1165,   798,   871,   148,    50,    51,
      52,    53,    54,   366,   104,   105,   395,  1310,   657,   657,
     407,     2,   957,   456,   834,  2081,   647,   574,   107,   871,
     848,  2191,  1600,   112,   958,   871,     3,  1218,  1219,  2269,
     890,   389,   892,  2266,   399,   895,  1591,  2275,   872,    24,
     882,  2276,   905,   883,   873,  1676,  1677,   176,   750,     1,
    1135,   886,    11,  2222,   750,   112,   113,  2226,  1227,   885,
     110,  1240,     1,   887,    50,    51,    52,    53,    54,  1241,
     934,    12,   107,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,   713,   933,  1603,   888,  1498,   906,  1523,
      23,   942,   144,  1323,  1324,    23,    23,    23,   957,     2,
      24,    24,   647,   943,    23,    24,   961,   176,   966,   112,
     958,   969,     2,   713,     3,  1780,    23,    23,    23,  1780,
     962,    23,  1369,  1370,   713,   971,    33,     3,    24,   125,
       1,    24,  1848,   918,   919,   920,   921,   131,  1273,  1427,
    1428,  1641,   750,   978,   330,   979,  2114,  1431,  1428,   595,
     980,  1350,  1024,   137,  1004,   750,  2034,  2035,   991,   627,
    1666,    23,   176,   626,    23,    23,   981,   478,   107,   389,
      23,  1744,  1745,  1746,   982,    23,   983,    23,  1436,  1428,
       2,  1567,  1428,   386,   400,   574,  1005,  1782,   707,   176,
    1867,  1782,  1399,   107,  2052,     3,   700,   -20,   750,   649,
     706,   394,   406,  1737,   871,   112,   113,  1075,   148,  1076,
    1760,  1241,    24,  1769,  1428,  2125,  1084,   389,  1057,   873,
    1898,  1428,   733,   750,  1101,    23,    23,    23,    23,   330,
     112,   113,     1,    32,  1089,   734,  -503,  1899,  1428,  1404,
    1741,  1740,  1985,  1986,   138,  1240,    50,    51,    52,    53,
      54,   735,   761,  1979,  1979,  1979,  1102,   330,  2266,  2267,
    1425,  1426,  1636,   403,   176,  1103,  1087,    23,  1105,   798,
    1106,   825,  1107,   801,  1651,  1653,  1655,   914,   915,  1661,
    1112,  1115,     2,   916,   917,  1108,   125,   736,  1784,  1109,
    1110,  1935,  1784,  1784,   131,  1118,   737,     3,    23,   738,
      23,   739,  1145,   144,   167,   922,   923,   740,   176,  1784,
     137,  1091,   176,   176,   846,  1958,   149,     1,  1834,  1836,
     399,  1354,   627,   713,  1092,   750,  1980,  1981,  1691,   176,
     125,   741,  1146,  1692,   125,   125,   580,  1774,  1776,  1155,
    1093,  1156,  1157,  1113,  1116,   713,   713,  1889,   742,  1163,
    1162,   125,     1,  1760,   137,    23,  1164,  1965,   137,   137,
    1169,  1171,  1172,  1173,   879,   148,  1174,     2,  1175,  1176,
      23,  1177,   641,   176,   797,   137,  1094,  1179,  1180,  1181,
    1866,    24,     3,  1196,   138,  1095,  1837,  1197,  1096,  1202,
    1097,  1207,   800,  1208,  1216,  1241,  1098,  1220,  1938,     1,
    2030,  1222,     2,  1233,   176,  1223,  1358,    23,     1,   148,
     750,  1248,  2069,   148,   148,  1238,  1252,     3,   389,  1254,
    1099,  1257,   585,    23,  1289,  1269,   125,    23,  1290,   595,
     148,   798,  1291,  1297,   131,  1789,  1790,  1100,  1298,  1791,
    1792,  1539,  1299,  1321,   167,   750,  1300,  1301,   176,     2,
     137,  1302,   871,   733,   713,  1303,   149,  1304,     2,  1331,
     144,   713,  1908,  -169,     3,  1889,   734,  1889,  1335,   386,
     400,  1914,    24,     3,   745,  1338,  1967,   700,   706,  1341,
    1342,    24,   735,  1343,  1344,  1537,  1345,  1346,  1543,  1347,
      24,  1348,   647,  1349,   713,  1363,  1553,  1546,  2120,  1418,
     986,   647,  1372,   989,   144,   148,   840,  1375,   144,   144,
    1377,  1398,  1408,  1409,   713,  1565,  1929,  1410,   736,    50,
      51,    52,    53,    54,   580,   144,  1411,   737,  1845,  1575,
     738,   482,   739,  2028,  1416,   139,  1421,   840,   740,   684,
    1503,    23,     1,    23,  1513,  1521,  1532,  1526,    23,   403,
      23,  1524,  1525,  1529,  2180,   580,  1530,  1531,  1533,    23,
    1559,  1784,   741,  1534,  1111,  1114,  1538,   713,   713,  1562,
    1563,  2200,  1570,  2069,   713,  1626,  1627,  1615,     1,   742,
    1605,   176,  1606,   713,  1607,  1894,     1,  1612,   595,   797,
     595,  1616,     2,   713,   672,   713,  1619,   798,  1628,  1635,
     144,  1644,  1127,   125,  2124,  1645,    24,     3,   713,  1648,
     713,   713,   713,  2224,  1649,   700,   706,  1658,     1,  1662,
    1663,   761,  1665,  1670,   871,  2028,  1704,   137,     2,  2076,
     595,  1576,   -24,  1674,     1,   750,     2,    23,  1675,  1705,
      23,    23,    32,     3,  1700,   389,    32,  1701,   107,  1710,
    2120,     3,  1711,  1717,  2120,  2120,   713,  1889,   641,    -5,
     713,  1724,  2108,   613,  1726,   713,  1747,  1580,     2,  1727,
    1729,   750,  1733,  1734,  2052,  1584,  1739,   273,   750,   750,
    1735,  1404,   148,     3,     2,   112,   113,   328,  2268,  2249,
    1743,  1761,  1762,    23,  1768,    23,  1637,  1395,     6,     3,
    1786,  1801,  1677,    30,   745,   595,  1802,  1620,  1804,  1806,
    1808,   750,  1400,  1809,  2259,  1828,  1186,  1982,  2259,   275,
     276,  1829,  1821,  2230,   277,  1715,  1716,   750,  1456,  1838,
    1840,  1841,  2270,    23,   713,  1324,  1851,   840,   243,  1868,
    1852,  1853,  1723,  1854,  1725,  1855,   713,   149,  1897,  1058,
     713,  1858,  1859,  1860,  1862,   580,   641,   840,   879,   879,
      23,  1863,  1864,  1728,  1896,   840,   713,  1878,  1900,  1901,
      24,    24,  1953,  1111,  1114,   580,   345,   144,  1905,    23,
      23,  1907,  1932,   580,  1934,  1789,  1790,  1882,   253,  1791,
    1792,  1956,  1957,  1993,  1784,  1784,  -127,  -127,  -127,  -127,
    -127,  -127,    24,  1998,    50,    51,    52,    53,    54,  2019,
    1999,  2024,  1908,  2017,   176,   176,  2029,   329,  2033,  2038,
     713,  1909,  2044,  2045,  2061,  2067,   139,  1280,  2083,  2085,
    2092,  1281,  2099,   397,  2097,  2117,   125,   125,   410,  2106,
     603,   606,    55,   634,   414,  2110,  2118,  2128,   423,    23,
      23,  2131,  2148,   750,   425,    32,   639,  2157,  2173,   713,
     137,   137,  2175,  2177,  1275,  2181,   430,  1278,   431,   685,
       1,  2185,  2184,   691,  2186,  1319,  2189,  2190,  2195,  2196,
    2221,  2212,  1320,  2197,  2208,  2210,   699,   705,    90,    91,
      92,    93,    94,    95,    96,    97,   713,  1305,  1813,  1814,
    1815,  1816,  1817,   713,  1890,    50,    51,    52,    53,    54,
    2220,  1856,  1857,  2225,  2223,   148,   148,  2229,  2237,  2247,
       2,   840,  2239,   713,  2246,   731,   744,  2248,   496,  2250,
     497,   498,   499,   107,    23,     3,  2261,  2262,  2274,   580,
    2264,  2273,  2277,    23,  -126,  -126,  -126,  -126,  -126,  -126,
     582,   781,  1764,   924,   606,   867,   925,   931,   149,   957,
     926,   928,  1440,   647,   927,   500,  1669,  1891,   501,   502,
     112,   958,   518,   503,   504,    24,  2258,  1788,   825,   761,
    2021,   620,  1812,  2219,    24,  2201,  1352,    24,    24,    24,
    1356,  2009,    24,  2193,  1360,    24,  2091,   830,  1944,   679,
    2179,  2109,  1930,   680,  1931,  2227,  1234,  2263,  2178,   841,
     144,   144,  1753,  1500,  1754,  1755,   255,   850,   418,  2142,
     790,  2234,  1890,   603,  1890,   702,  1877,  1951,  1703,  1687,
    1334,   746,   823,  1250,  1082,  1839,   182,   139,  1187,  1188,
    1189,    24,   841,     0,     0,  1738,    24,     0,   728,   729,
      50,    51,    52,    53,    54,   868,     0,     0,   751,  1285,
       0,   755,   756,     0,   840,   760,     0,     0,   763,   764,
       0,   766,     0,   767,     0,   879,     0,   879,  1285,     0,
       0,     0,   580,     0,   641,  1891,   789,  1891,     0,     0,
       0,   389,  2271,     0,   747,    50,    51,    52,    53,    54,
    1183,  2278,   802,     0,     0,     0,     0,   807,     0,   810,
       0,     0,     0,     0,   269,   149,  1527,     0,     0,   813,
    1528,  1112,  1115,     0,     0,     0,     0,     0,     0,   634,
     831,     0,     0,  1285,     0,     0,     0,  1659,     0,     0,
       0,     0,   639,     0,     0,     0,     0,     0,     0,     0,
     409,     0,     0,  1285,     0,   411,     0,     0,   416,   149,
     421,     0,     0,   149,   149,    12,     0,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,   993,     0,     0,
     149,     0,     0,  1541,  1113,  1116,     0,     0,     0,     0,
       0,     0,     0,  1588,   139,  1009,     0,  1589,     0,     0,
       0,  1590,   377,   462,     0,   263,    42,    43,    44,    45,
      46,    47,    48,    49,  1890,   830,    24,    24,     0,  1028,
       0,     0,     0,     0,     0,     0,  1578,  1052,     0,  1582,
       0,     0,   335,  1586,     0,  1285,     0,     0,   139,     0,
       0,    24,   139,   139,     0,  1660,     0,     0,  1063,     0,
     580,     0,     0,     0,     0,   149,     0,     0,     0,   139,
       0,   746,     0,     0,     0,     0,     0,    50,    51,    52,
      53,    54,     0,   408,     0,     0,   679,  1891,     0,     0,
       0,     0,   798,     0,   965,     0,     0,   967,     0,     0,
       0,     0,   603,     0,  1622,     0,     0,     0,    24,     0,
       0,     0,     0,     0,     0,     0,   409,   411,     0,   677,
       0,   421,    12,   985,   354,   355,    15,    16,    17,    18,
      19,    20,    21,    22,   747,    24,     0,     0,   830,     0,
    1112,  1115,    24,     1,   139,     0,     0,  1130,  1133,     0,
       0,   107,     0,     0,     0,     0,   603,   603,  1027,  1033,
       0,     0,  1036,  1037,     0,  1040,     0,  1042,  1043,     0,
    1285,  1285,  1045,  1046,     0,    24,     0,  1881,   109,   395,
     407,     0,     0,     0,  1882,     0,     0,   389,   112,   113,
       0,     0,  1718,     2,   132,   604,  1719,     0,     0,  1720,
     114,   132,     0,  1113,  1116,     0,   107,     0,     3,     0,
     409,   643,     0,     0,  1285,    12,     0,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,     0,   828,  1730,
       0,     0,  2052,  1731,     0,     0,   750,  1732,     0,   270,
      24,     0,   149,   112,   113,     0,     0,     0,    24,    24,
     477,     0,   830,  1192,     0,    24,  1117,     0,     0,     0,
       0,     0,     0,   132,    24,     0,     0,     0,     0,     0,
     390,     0,     0,  1766,    24,     0,    24,     0,     0,     0,
     751,   323,   413,   417,     0,     0,     0,     0,     0,    24,
       0,    24,    24,    24,    12,  1650,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,     0,   798,     0,   787,
       0,  1242,     0,  1789,  1790,  2011,  2012,  1791,  1792,  2013,
    2014,   139,   993,     0,  1152,  1153,  1154,     0,   463,     0,
     871,     0,     0,     0,   132,     0,   830,    24,   441,     0,
    1908,    24,     0,     0,     0,     0,    24,     0,   603,  -201,
     714,     0,     0,     0,   603,     0,     0,     0,     0,     0,
    1286,     0,     0,   830,   335,  1052,     0,   830,     0,     0,
     801,     0,   851,     0,  1652,     0,  2116,     0,   604,     0,
       0,     0,  1191,     0,     0,  1194,  1195,     0,     0,     0,
       0,   830,     0,     0,    12,     0,  1141,   335,    15,    16,
      17,    18,    19,    20,    21,    22,   553,     0,     0,  1713,
       0,   830,     0,     0,     0,    24,   993,     0,   830,     0,
       0,     0,     0,     0,     0,     0,     0,    24,    12,   390,
       0,    24,    15,    16,    17,    18,    19,    20,    21,    22,
       0,  2166,     0,  1232,   909,    12,   828,    24,   628,    15,
      16,    17,    18,    19,    20,    21,    22,   893,     0,     0,
     390,     0,     0,     0,     0,     0,     0,  1027,     0,     0,
    1256,     0,  1258,     0,     0,   149,   149,     0,     0,     0,
       0,     0,     0,  1307,   109,     0,     0,  1274,     0,     0,
       0,   703,     0,     0,   441,     0,   477,   643,     0,   477,
       0,    24,     0,     0,   894,     0,   114,     0,     0,     0,
       0,  1242,   477,     0,   830,     0,     0,     0,     0,     0,
       0,     0,     1,     0,     0,     0,     0,   977,     0,   132,
     462,     0,  1308,     0,     0,     0,   477,   390,     0,     0,
      24,     0,     0,     0,     0,     0,   782,     0,   785,     0,
       0,     0,     0,     0,   139,   139,    12,     0,   132,   828,
      15,    16,    17,    18,    19,    20,    21,    22,   477,     0,
     132,     0,     2,   390,     0,   151,     0,    24,   390,     0,
       0,     0,   151,     0,    24,   107,     0,     3,     0,     0,
       0,   930,     0,     0,     0,  1054,     0,   132,     0,     0,
       0,     0,     0,   323,    24,     0,     0,     0,     0,  1351,
    1242,   108,   109,  1355,     0,    24,   390,  1359,     0,   409,
       0,     0,   112,   113,    24,     0,     0,   603,     0,     0,
    1161,     0,     0,     0,   114,     0,   323,     0,     0,     0,
       0,     0,     0,     0,   151,     0,     0,   604,     0,     0,
       0,     0,   830,     0,     0,   302,   830,     0,     0,  2056,
       0,     0,   151,     0,     0,  1394,     0,     0,     0,     0,
      10,     0,     0,   828,     0,     0,     0,     0,   392,     0,
       0,     0,   151,     0,     0,     0,     0,     0,     0,     0,
     390,     0,     0,     0,     0,  1561,     0,     0,     0,    12,
       0,   604,   604,    15,    16,    17,    18,    19,    20,    21,
      22,   390,     0,     0,     0,   151,     0,     0,     0,   151,
    1070,  2056,   714,     0,     0,     0,     0,     0,     0,   830,
       0,     0,     0,   830,     0,     0,     0,   830,  1232,     0,
       0,     0,     0,     0,     0,     0,   302,  2056,  2056,  1242,
       0,    11,     0,     0,     0,   109,     0,   828,  1069,   390,
       0,     0,  1308,     0,     0,   390,   787,     0,     0,     0,
      12,   390,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,     0,     0,   828,     0,     0,   151,   828,     0,
       0,     0,     0,  1006,   570,   151,     0,   587,     0,     0,
       0,     0,     0,     0,     0,     0,  1540,  2056,  2056,  1544,
    1547,   390,   828,     0,     0,   302,     0,  1556,  1557,     0,
       0,     0,   390,     0,     0,  1136,  1138,     0,   302,   151,
    2056,   302,   828,     0,     0,     0,     0,   392,   151,   828,
     390,     0,     0,     0,  1572,   463,  1574,     0,     0,  1577,
       0,     0,  1581,     0,     0,     0,  1585,   151,     0,     0,
     628,   151,     0,     0,     0,   302,     0,     0,   392,     0,
       0,     0,   151,     0,     0,   151,     0,  2056,     0,     0,
       0,     0,     0,  2254,     0,     0,  1121,  1599,     0,     0,
    2232,  1242,   603,   604,  2056,     0,     0,     0,     0,   604,
    1609,     0,     0,     0,  1212,     0,     0,     0,  1212,     0,
     151,     0,     0,     0,     0,     0,   714,  1621,   830,     0,
       0,     0,   830,  1054,     0,   830,     0,   151,   151,   151,
       0,     0,     0,     0,     0,   828,     0,     0,     0,   151,
       0,     0,  1212,     0,   390,   392,     0,     0,     0,     0,
       0,   151,     0,     0,     0,   830,     0,     0,     0,   830,
     390,     0,     0,   830,   132,     0,     0,     0,     0,   819,
       0,     0,   151,   390,     0,     0,     0,     0,   151,     0,
     557,   392,   151,   302,   151,     0,   392,     0,   558,   559,
     560,   561,   302,     0,   553,   302,     0,   151,   151,   830,
       0,     0,   274,     0,   703,   785,     0,     0,   302,     0,
       0,   151,   151,   151,   302,     0,     0,   302,     0,     0,
       0,     0,     0,     0,   392,   360,   628,     0,     0,    12,
      55,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,   603,     0,     0,     0,    12,  1544,   354,   355,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,    12,     0,     0,   302,    15,    16,    17,    18,    19,
      20,    21,    22,   828,   107,  1242,     0,   828,     0,  1212,
      94,    95,    96,    97,     0,     0,   714,     0,   562,  1070,
       0,     0,     0,     0,     0,     0,     0,     0,   392,     0,
    1881,   109,     0,     0,   132,     0,   563,  1882,     0,     0,
       0,   112,   113,     0,     0,     0,   302,   151,     0,   392,
       0,     0,     0,   114,     0,    12,   603,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,   897,     0,   390,
       0,     0,   302,     0,     0,     0,   390,   151,     0,     0,
     828,     0,     0,     0,   828,     0,     0,     0,   828,     0,
       0,     0,     0,     0,   587,   151,     0,  1001,   302,     0,
       0,     0,   604,   392,   151,     0,     0,   302,     0,   392,
       0,     0,  1827,   151,   898,   651,     0,   390,     0,     0,
     151,     0,     0,     0,  1611,     0,     0,     0,     0,     0,
      50,    51,    52,    53,    54,     0,     0,     0,     0,   714,
       0,     0,  1846,     0,   302,   151,     0,     0,     0,   392,
    1212,     0,     0,     0,   819,     0,   151,     0,   151,     0,
     392,     0,     0,     0,   302,     0,   151,  1861,     0,     0,
       0,   151,   151,     0,     0,     0,     0,     0,   392,     0,
     732,   360,  1609,  1609,  1609,     0,  1873,   243,     0,  1875,
    1970,     0,  1974,     0,     0,     0,     1,   302,     0,  1895,
      12,   772,   299,     0,    15,    16,    17,    18,    19,    20,
      21,    22,    12,     0,   583,   584,    15,    16,    17,    18,
      19,    20,    21,    22,  1122,     0,     0,     0,  2026,   107,
      12,     0,     0,   151,    15,    16,    17,    18,    19,    20,
      21,    22,     0,     0,     0,     0,     2,     0,     0,     0,
       0,   302,   302,     0,     0,  1558,   109,   302,     0,   107,
     772,     3,     0,     0,     0,     0,   112,   113,     0,   828,
     110,     0,     0,   828,     0,   151,   828,     0,   114,  1948,
       0,     0,   585,     0,     0,   108,   109,     0,     0,  1954,
    1955,     0,     0,     0,     0,     0,   112,   113,   392,     0,
       0,     0,     0,     0,   151,   151,   828,     0,   114,   390,
     828,   392,     0,     0,   828,   151,   151,     0,     0,     0,
    2026,     0,     0,     0,   151,   132,   751,     0,   302,     0,
       0,     0,     0,     0,     0,     0,     0,   151,     0,     0,
       0,     0,     0,     0,  1593,     0,     0,   151,     0,     0,
     828,     0,     0,     0,     0,     0,   714,     0,     0,     0,
       0,     0,     0,  1212,     0,     0,     0,   604,  1212,  1212,
    1212,     0,     0,     0,     0,   299,     0,   772,   637,     0,
       0,     0,     0,     0,     0,   676,     0,     0,     0,     0,
       0,  2047,     0,     0,     0,     0,     0,  2048,  2049,  2050,
       0,     0,     0,     0,   637,     0,     0,     0,   637,     0,
       0,     0,   299,   151,  2066,   151,     0,     0,     0,   151,
       0,  1130,  2183,     0,     0,     0,     0,     0,     0,     0,
     151,   151,     0,     0,     0,     0,  1596,     0,     0,     0,
       0,     0,     0,   151,     0,     0,   477,     0,     0,   302,
       0,     0,     0,     0,     0,     0,     0,     0,   151,     0,
       0,     0,   151,     0,     0,     0,   151,     0,   151,     0,
    1014,   302,     0,   302,   651,   299,     0,  1001,     0,     0,
       0,     0,  2129,  2130,   392,   151,   151,     0,     0,     0,
       0,  2136,     0,     0,     0,     0,     0,  2140,  2141,     0,
       0,     0,     0,     0,     0,     0,   151,     0,     0,     0,
       0,     0,     0,   151,  2151,     0,   604,     0,     0,     0,
       0,     0,     0,     0,   151,  1001,     0,     0,     0,     0,
     299,     0,     0,     0,     0,     0,   132,     0,     0,   830,
     151,     0,     0,     0,     0,     0,  2151,     0,     0,     0,
       0,  2136,     0,   772,     0,  1090,     0,     0,     0,     0,
     151,   299,     0,   390,     0,     0,  2194,   360,   360,     0,
       0,  1212,     0,  1212,     0,    12,  2199,   354,   355,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
     309,     0,  1131,   772,   772,     0,   714,     0,     0,     0,
       0,   604,     0,     0,     0,     0,   772,     0,     0,   151,
       0,     0,     0,     0,     0,     1,     0,     0,     0,   151,
       0,     0,  1777,  1785,     0,     0,  1777,  1796,     0,     0,
       0,     0,  1803,  2231,     0,     0,  1807,     0,     0,   151,
    2236,  1811,   151,  1796,     0,   585,   151,     0,     0,    12,
     151,   354,   355,    15,    16,    17,    18,    19,    20,    21,
      22,     0,     0,   299,   637,     2,     0,     0,     0,     0,
       0,     0,     0,  2256,     0,     0,  2236,     0,     0,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     0,   299,
       0,     0,     0,   132,     0,     0,  2256,     0,     0,     0,
       0,     0,  1756,     0,  1307,   109,   302,     0,     0,     0,
       0,     0,   637,     0,     0,   676,     0,   392,   151,     0,
       0,     0,   302,     0,   299,     0,     0,   114,     0,     0,
     637,     0,     0,     0,  1520,     0,     0,   132,     0,     0,
       0,   132,   132,     0,     0,     0,     0,   151,     0,     0,
       0,   151,     0,     0,   390,     0,     0,  1902,   132,     0,
       0,   299,   637,   309,     0,     0,     0,   151,   151,     0,
       0,     0,     0,     0,     0,     0,     1,  1918,  1920,     0,
       0,   299,     0,   637,     0,     0,     0,   714,     0,   299,
       0,     0,     0,     0,     0,     0,     0,     0,   151,     0,
     309,     0,     0,     0,     0,     0,     0,  1940,     0,     0,
      12,   151,   354,   355,    15,    16,    17,    18,    19,    20,
      21,    22,     0,   390,   151,     0,     2,     0,   151,     0,
       0,     0,   151,   132,     0,     0,  1014,     0,     0,   107,
       0,     3,     0,  1874,  1597,  1876,     0,     0,  1309,   390,
     651,     0,   651,     0,     0,     0,     0,   151,     0,     0,
       0,     0,     0,   309,     0,  2112,   109,     0,   151,   750,
       0,     0,     0,     0,   299,     0,   112,   113,     0,     0,
       0,     0,     0,  1992,     0,     0,     0,  1995,   114,  1997,
       0,     0,  2001,  2007,     0,  1796,     0,     0,     0,    12,
    2018,    13,    14,    15,    16,    17,    18,    19,    20,    21,
      22,     0,     0,     0,     0,   134,     0,   302,     0,     0,
       0,     0,   134,    12,     0,   354,   355,    15,    16,    17,
      18,    19,    20,    21,    22,     0,     0,     0,     0,     0,
       0,   772,   302,   302,     0,   299,     0,     0,     0,     0,
       0,     0,   107,     0,  1373,     0,   772,   772,     0,     0,
     828,     0,     0,     0,     0,  1654,  1647,     0,  1964,   390,
       0,   392,   151,     0,     0,     0,     0,  1664,  2112,   109,
       0,  2094,   750,     0,   134,     0,     0,   151,  2101,   112,
     113,     0,     0,  2103,  2104,   293,     0,     0,     0,   151,
       0,   114,   324,     0,     0,     0,   151,   151,     0,     0,
     132,     0,     0,   151,     0,     0,  2126,   151,     0,     0,
     151,     0,   401,    12,     0,   354,   355,    15,    16,    17,
      18,    19,    20,    21,    22,    12,   637,    13,    14,    15,
      16,    17,    18,    19,    20,    21,    22,   637,     0,     0,
     151,   309,     0,     0,   151,   437,     0,     0,   151,   442,
       0,     0,  2156,     0,  2159,     1,   772,  2161,  2163,  2164,
       0,     0,     0,     0,   151,  2169,  2171,   309,  1307,   109,
       0,     0,     0,   151,     0,   637,     0,     0,   299,     0,
     637,     0,   151,   390,   151,     0,     0,     0,   945,    12,
       0,   114,     0,    15,    16,    17,    18,    19,    20,    21,
      22,     0,   309,     0,     0,     2,     0,     0,   302,     0,
       0,     0,   302,   302,     0,     0,     0,   554,     0,     0,
       3,  1555,     0,     0,     0,   575,   302,     0,  1014,   302,
     651,   637,   263,    42,    43,    44,    45,    46,    47,    48,
      49,     0,  2214,  2216,  2218,     0,   605,     0,     0,     0,
       0,   309,   392,   309,     0,     0,     0,     0,   623,   629,
       0,   635,     0,     0,     0,     0,  1844,     0,   675,     0,
       0,     0,     0,   151,     0,     0,     0,  2241,  2243,  2245,
       0,     0,     0,     0,     0,     0,     0,   686,  1830,  1831,
       0,   686,     0,     0,     0,   293,     0,     0,     0,     0,
       0,     0,   704,     0,   151,   629,     0,     0,     0,     0,
       0,   302,     0,     0,   151,     0,   151,     0,     0,     0,
       0,   392,     0,     0,     0,     0,   637,     0,     0,     0,
       0,   743,     0,   132,   132,     0,     0,     0,     0,     0,
     437,     0,     0,     0,     0,     0,     0,   392,   151,     0,
       0,   770,   309,     0,     0,     0,   780,   704,   293,   324,
       0,     0,     0,     0,     0,     0,     0,    12,     0,   437,
       0,    15,    16,    17,    18,    19,    20,    21,    22,  1209,
       0,   437,     0,     0,  1210,   808,  1211,     0,     0,     0,
     811,     0,     0,     0,     0,   812,     0,  1906,     0,  1160,
       0,     0,   824,     0,  1912,     0,     0,     0,   437,     0,
      12,     0,   837,   575,    15,    16,    17,    18,    19,    20,
      21,    22,  1209,   109,   772,   637,  1423,  1210,     0,  1211,
       0,     0,     0,     0,     0,     0,     0,  1937,     0,   151,
       0,     0,     0,     0,   575,     0,     0,    12,     0,   354,
     355,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,     0,     0,   109,   392,     0,  1631,
       0,     0,  1014,  1309,   651,   651,   107,     0,   302,     0,
     191,   302,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,   151,     0,     0,     0,
       0,     0,   356,   109,     0,     0,     0,     0,   309,     0,
    1983,  1984,     0,   112,   113,     0,     0,  1994,     0,     0,
       0,   145,     0,     0,     0,   114,  2008,     0,   145,     0,
       0,     0,     0,     0,     0,     0,  2022,   488,  2023,     0,
     110,   203,     0,     0,     0,     0,   623,   635,     0,     0,
       0,  2037,     0,  2039,  2040,  2041,     0,     0,     0,     0,
       0,   309,     0,     0,   637,     0,     0,  2096,   392,     0,
       0,     0,   293,     0,     0,     0,     0,     0,     0,     0,
     309,     0,     0,     0,     0,     0,     0,     0,   151,     0,
     145,   392,     0,     0,     0,   686,     0,     0,  1003,  2077,
       0,   297,     0,  2082,   629,     0,     0,   623,  2087,     0,
       0,     0,     0,  1010,     0,     0,     0,     0,     0,     0,
     824,     0,     0,     0,  1026,     0,     0,     0,   402,     0,
       0,     0,     0,     0,     0,   309,     0,     0,     0,     0,
       0,     0,     0,     0,  1051,   635,     0,     0,     0,     0,
     299,  1056,     0,     0,     0,   309,   293,  2167,   293,     0,
       1,   145,     0,     0,   575,     0,   686,     0,     0,     0,
    1888,   629,   575,     0,     0,     0,    12,  2143,   281,   105,
      15,    16,    17,    18,    19,    20,    21,    22,     0,  2152,
       0,     0,     0,  2155,    12,     0,   354,   355,    15,    16,
      17,    18,    19,    20,    21,    22,   637,     0,     0,  2172,
       2,     0,     0,     0,   151,   151,    50,    51,    52,    53,
      54,     0,     0,   107,     0,     3,     0,     0,     0,     0,
       0,     0,   109,   824,     0,  1069,     0,   309,     0,     0,
       0,   576,     0,  1132,     0,     0,     0,     0,     0,   356,
     109,     0,     0,     0,     0,     0,     0,   623,     0,     0,
     112,   113,     0,  2207,     0,     0,     0,     0,     0,     0,
       0,     0,   114,     0,   297,   437,     0,   636,     0,     0,
       0,     0,     0,     0,   402,     0,     0,     0,  1888,     0,
    1888,     0,     0,     0,   151,  1971,     0,  1888,     0,     0,
       0,     0,  2228,   636,   824,   704,     0,   636,     0,     0,
       0,   297,     0,     0,     0,   704,    12,     0,   354,   355,
      15,    16,    17,    18,    19,    20,    21,    22,   575,     0,
       0,     0,     0,     0,     0,     0,     0,   629,     0,  2253,
       0,     0,     0,     0,     0,   107,  2257,   824,  1190,     0,
       0,     0,     0,     0,     0,     0,   145,     0,     0,     0,
       0,     0,   309,   309,     0,     0,  2265,     0,     0,     0,
       0,   646,   109,     0,   297,   647,     0,     0,     0,     0,
       0,     0,   112,   648,  2065,   145,     0,  1888,  1888,     0,
     637,     0,     0,   309,   114,     0,     0,   145,     0,     0,
       0,   809,     0,     0,     0,     0,   309,     0,     0,     1,
       0,     0,     0,   293,     0,   134,     0,     0,   576,  1243,
       0,     0,     0,     0,   145,     0,     0,     0,   402,   297,
     686,   824,     0,  1255,     0,     0,     0,     0,     0,     0,
     772,     0,     0,    12,     0,   354,   355,    15,    16,    17,
      18,    19,    20,    21,    22,     0,     0,     0,   824,     2,
     297,     0,   824,  1610,     0,     0,   293,     0,   686,     0,
       0,  1051,   107,   635,     3,     0,     0,     0,     0,     0,
    1888,     0,     0,    12,     0,   293,   824,    15,    16,    17,
      18,    19,    20,    21,    22,  1209,     0,  1306,   768,   109,
    1210,     0,  1211,     0,     0,     0,   824,     0,     0,   112,
     113,     0,     0,   824,     0,     0,     0,     0,     0,     0,
       0,   114,     0,     0,   686,     0,  1131,   772,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   309,   109,
     293,     0,  1633,     0,     0,     0,     0,     0,     0,   449,
       0,   452,     0,     0,   455,     0,     0,     0,     0,     0,
     293,     0,   297,   636,     0,     0,   465,   466,     0,     0,
       0,    12,     0,   354,   355,    15,    16,    17,    18,    19,
      20,    21,    22,   521,   455,   455,     0,     0,   297,     0,
       0,     0,     0,     0,     0,     0,   772,   772,     0,     0,
     107,   309,   309,     0,     0,     0,     0,     0,     0,   824,
       0,   636,     0,   637,   402,     0,     0,     0,     0,  1243,
       0,     0,     0,   297,     0,     0,   768,   109,   455,   636,
      50,    51,    52,    53,    54,     0,   576,   112,   113,   575,
       0,     0,   293,     0,     0,     0,   824,     0,     0,   114,
       0,     0,     0,   455,     0,     0,     0,     0,    12,     0,
     297,   636,    15,    16,    17,    18,    19,    20,    21,    22,
    1209,     0,   576,     0,   576,  1210,     0,  1211,     0,     0,
     297,     0,   636,     0,     0,     0,     0,     0,   297,     0,
       0,     0,     0,     0,     0,     0,     1,     0,     0,     0,
       0,     0,     0,     0,     0,   637,  1497,     0,     0,     0,
       0,     0,     0,     0,   109,     0,     0,    12,  1243,   814,
     105,    15,    16,    17,    18,    19,    20,    21,    22,     0,
      12,     0,   354,   355,    15,    16,    17,    18,    19,    20,
      21,    22,     0,     0,     0,     0,     2,   824,     0,   576,
       0,   824,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     3,     0,     0,     0,     0,     0,   293,   293,     0,
       0,     0,     0,   297,     0,     0,   932,     0,     0,   780,
       0,     0,     0,     0,     0,   646,   109,     0,     0,   647,
    1560,   145,     0,     0,     0,     0,   112,   648,   293,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   114,     0,
       0,   293,     0,     0,     0,     0,     0,     0,     0,     0,
     576,     0,     0,     0,   824,     0,     0,     0,   824,     0,
       0,     0,   824,     0,     0,     0,     0,     0,     0,   455,
       0,     0,     0,     0,   297,     0,     0,     0,    50,    51,
      52,    53,    54,     0,     0,     0,     0,  1243,     0,     0,
       0,     0,     0,   576,     0,     0,     0,    12,   824,   816,
     817,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     455,   455,   455,   455,   455,   455,   455,   455,   455,   455,
     455,   455,   455,   455,   455,   455,   455,   455,   455,     0,
       0,     0,     0,     0,     1,     0,     0,  1497,     0,     0,
       0,     0,     0,     0,     0,   110,     0,     0,     0,   576,
       0,   145,     0,     0,     0,   636,     0,     0,     0,     0,
       0,     0,     0,   293,     0,     0,   636,   576,    12,     0,
     354,   355,    15,    16,    17,    18,    19,    20,    21,    22,
       0,     0,     0,     0,     2,     0,     0,     0,     0,     0,
       0,     0,  1688,     0,   576,     0,     0,   107,   576,     3,
       0,     0,   576,     0,   636,     0,     0,   297,     0,   636,
     150,     0,     0,     0,     0,     0,     0,   150,     0,  1243,
       0,   576,   576,   356,   109,     0,   293,   293,     0,     0,
       0,     0,     0,   824,   112,   113,     0,   824,     0,     0,
     824,     0,   576,     0,     0,     0,   114,     0,     0,   576,
       0,     0,     0,     0,     0,   455,     0,     0,     0,     0,
     636,    50,    51,    52,    53,    54,     0,     0,     0,     0,
     824,     0,     0,     0,   824,     0,   576,     0,   824,   150,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     301,     0,     0,     0,   134,     0,   576,     0,     0,     0,
       0,     0,     0,  1757,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,   824,     0,     0,   404,     0,     0,
       0,     0,     0,     0,     0,     0,    12,     1,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,  1783,     0,
       0,     0,  1783,  1783,     0,   576,     0,     0,     0,     0,
     150,     0,     0,     0,     0,   636,     0,     0,     0,  1783,
       0,    12,     0,   354,   355,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,   576,     0,     2,   576,     0,
       0,     0,   576,     0,   110,     0,     0,     0,     0,     0,
     107,    12,     3,    13,    14,    15,    16,    17,    18,    19,
      20,    21,    22,  1243,    12,     0,   354,   355,    15,    16,
      17,    18,    19,    20,    21,    22,  2112,   109,     0,     0,
     750,     0,     0,     0,     0,     0,     0,   112,   113,     0,
     578,     0,     0,   107,   134,     0,     0,     0,     0,   114,
       0,     0,   145,     0,   704,   455,   645,     0,     0,     0,
     455,     0,     0,     0,   636,     0,     0,     0,     0,   768,
     109,     0,     0,   301,     0,     0,   638,     0,   455,     0,
     112,   113,     0,   404,     0,     0,     0,     0,  1903,     0,
       0,     0,   114,   576,     0,     0,     0,   576,     0,     0,
       0,     0,   638,     0,     0,     0,   638,     0,     0,     0,
     301,     0,     0,   576,   576,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   347,   455,     0,     0,     0,     0,
      12,  2068,    13,    14,    15,    16,    17,    18,    19,    20,
      21,    22,     0,    12,   576,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,   150,     0,   576,   505,   506,
     507,   508,   509,   510,   511,   512,   513,   514,   515,     0,
     576,     0,     0,   301,   576,     0,     0,   339,   576,     0,
       0,     0,     0,     0,   150,   183,     0,  1973,   184,     0,
     185,   186,     0,   187,     0,   455,   150,     0,   695,     0,
       0,     0,     0,   636,   516,     0,     0,     0,     0,     0,
     188,     0,     0,     0,   576,     0,     0,   578,  2004,     0,
       0,  1783,     0,   150,     0,     0,     0,   404,   301,     0,
       0,     0,     0,     0,     0,     0,  2027,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,   301,
     197,   198,   199,   145,   200,   201,     0,     0,     0,     0,
       0,     0,   107,     0,     0,     0,   455,    12,     0,   354,
     355,    15,    16,    17,    18,    19,    20,    21,    22,   297,
       0,     0,     0,     0,     0,     0,     0,     0,   202,     0,
       0,   110,   203,     0,     0,     0,   107,     0,   204,   112,
     113,   205,   206,   207,   208,     0,     0,     0,   402,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  2027,     0,
       0,     0,   646,   109,     0,     0,   647,     0,     0,     0,
       0,     0,     0,   112,   648,   636,     0,     0,     0,     0,
       0,     0,   576,   576,     0,   114,   649,  2135,     0,   576,
       0,   301,   638,   576,     0,     0,   576,     0,     0,     0,
       0,     0,     0,     0,     0,   455,   455,   455,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   301,     0,     0,
       0,     0,   455,   455,     0,     0,   576,     0,     0,     0,
     576,     0,     0,     0,   576,     0,     0,     0,     0,     0,
     638,     0,     0,   404,     0,     0,     0,  2182,     0,     0,
     145,   455,   301,     0,     0,     0,     0,     0,   638,     0,
       0,     0,     0,     0,     0,   578,     0,     0,     0,     0,
     576,     0,     0,     0,     0,     0,     0,    50,    51,    52,
      53,    54,     0,     0,  1783,  1783,     0,     0,     0,   301,
     638,     0,     0,     0,   145,     0,     0,     0,   145,   145,
       0,   578,     0,   578,     0,     0,     0,     0,     0,   301,
       0,   638,     0,     0,     0,   145,    12,   301,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,     0,     0,
       0,     0,     0,     0,   126,     0,     0,     0,     0,     0,
       0,   126,     0,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   455,     0,   455,     0,     0,   636,
       0,     0,     0,     0,   824,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,   578,   354,
     355,    15,    16,    17,    18,    19,    20,    21,    22,     0,
     145,     0,     0,     2,     0,     0,     0,     0,     0,     0,
       0,     0,   301,   126,     0,     0,   107,     0,     3,     0,
       0,     0,     0,     0,   290,     0,     0,     0,     0,     0,
     150,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   768,   109,   402,     0,     0,   387,     0,     0,
       0,   126,     0,   112,   113,     0,     0,     0,     0,   578,
       0,     0,     0,     0,     0,   114,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   455,
       0,     0,     0,   301,    39,     0,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,   578,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,  -508,  -508,     0,  -508,
      88,     0,    89,     0,     0,  -508,     0,    90,    91,    92,
      93,    94,    95,    96,    97,    98,   126,     0,     0,    99,
       0,     0,     0,   100,     0,     0,     0,     0,   578,     0,
     150,     0,     0,     0,   638,     0,     0,   145,     0,     0,
       0,     0,     0,     0,     0,   638,   578,     0,     0,     0,
     101,     0,   636,     0,     0,   102,   103,   290,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,     0,     0,
       0,   106,     0,   578,     0,     0,     0,   578,     0,     0,
       0,   578,     0,   638,     0,   107,   301,     0,   638,     0,
       0,     0,     0,     0,   290,     0,     0,     0,     0,     0,
     578,   578,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   108,   109,     0,   110,   111,     0,     0,     0,     0,
       0,   578,   112,   113,     0,     0,     0,     0,   578,     0,
       0,     0,     0,     0,   114,     0,   115,     0,     0,   638,
       0,     0,     0,     0,   636,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   578,     0,   290,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   387,   578,     0,     0,     0,     0,
    1551,     0,     0,     0,     0,     0,     0,     0,    50,    51,
      52,    53,    54,     0,     0,     0,     0,     0,     0,     0,
       0,   455,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   578,     0,     0,     0,     0,     0,
     183,     0,     0,   184,   638,   185,   186,     0,   187,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   578,   188,     0,   578,     0,     0,
       0,   578,     0,     0,     0,     0,     0,     0,     0,     0,
     145,   145,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,     0,   200,
     201,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,   150,     0,     0,   455,     0,     0,   940,     0,     0,
       0,     0,     0,   638,     0,     0,     0,     0,     0,     0,
       0,   118,     0,   202,     0,   290,   110,   203,   118,     0,
     576,     0,     0,   204,  1552,   113,   205,   206,   207,   208,
       0,     0,   578,     0,     0,     0,   578,     0,     0,     0,
       0,   290,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   578,   578,    50,    51,    52,    53,    54,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    50,
      51,    52,    53,    54,     0,     0,   290,     0,     0,     0,
     118,     0,     0,   578,     0,     0,     0,     0,   272,     0,
       0,   287,     0,     0,     0,     0,   578,     0,   118,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   578,
       0,   357,     0,   578,   385,     0,     0,   578,   118,     0,
       1,     0,     0,     0,     0,   290,     0,   290,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   638,     0,     0,     0,     0,     0,     0,     0,
       0,   455,     0,   578,    12,   440,   354,   355,    15,    16,
      17,    18,    19,    20,    21,    22,     0,     0,     0,    12,
       2,   354,   355,    15,    16,    17,    18,    19,    20,    21,
      22,     0,   473,   107,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   107,     0,
     455,     0,   150,     0,     0,     0,     0,     0,     0,  1314,
     109,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     112,  1315,     0,   440,   356,   109,   290,     0,   301,     0,
       0,   573,   114,     0,     0,   112,   113,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   114,     0,     0,
       0,   473,   272,   272,     0,     0,     0,   404,     0,     0,
       0,     0,     0,     0,   287,   440,     0,   287,     0,     0,
       0,   650,     0,   671,     0,     0,   455,     0,     0,     0,
       0,     0,     0,     0,   638,     0,     0,     0,     0,     0,
       0,   578,   578,   573,     0,     0,     0,   573,   578,     0,
       0,   287,   578,     0,   385,   578,     0,     0,   272,     0,
     455,   440,   455,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   578,   357,   357,     0,   578,
       0,     0,     0,   578,     0,     0,     0,     0,     0,     0,
       0,     0,   455,     0,     0,     0,     0,   769,     0,   150,
       0,     0,     0,     0,   573,   118,     0,     0,     0,     0,
       0,     0,    50,    51,    52,    53,    54,     0,     0,   578,
       0,   385,   290,     0,   126,     0,     0,     0,   126,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   150,     0,     0,     0,   150,   150,     0,
       0,     0,     0,     0,     0,     0,   769,     0,     0,   287,
     440,     0,   842,     0,   150,     0,     0,   455,   473,     0,
       0,   473,     0,   440,   440,   290,     0,     0,     1,     0,
       0,     0,     0,     0,   473,     0,     0,   440,   440,   440,
     287,     0,     0,   473,   290,     0,     0,     0,     0,   874,
     842,    50,    51,    52,    53,    54,     0,     0,   638,     0,
       0,     0,    12,     0,   354,   355,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     2,    50,
      51,    52,    53,    54,     0,     0,     0,     0,     0,   150,
     473,   107,     0,     3,     0,     0,     0,     0,     0,   290,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1881,   109,   290,
       0,     0,     0,   769,   842,     0,     0,     0,   112,   113,
       0,     0,     0,   404,     0,     0,     0,     0,     0,     0,
     114,     0,   287,   573,   959,   671,     0,     0,     0,     0,
       0,    12,     0,   354,   355,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,     0,     0,     0,   287,     0,
       0,     0,     0,   440,     0,   272,   272,     0,   126,    12,
     107,   354,   355,    15,    16,    17,    18,    19,    20,    21,
      22,   573,     0,   999,     0,     0,     0,     0,     0,   842,
     440,   290,     0,   287,     0,   671,  2112,   109,   107,     0,
     750,     0,     0,     0,     0,     0,   650,   112,   113,     0,
     650,     0,     0,     0,     0,     0,     0,     0,     0,   114,
       0,     0,     0,     0,  1314,   109,     0,     0,     0,     0,
     287,   573,     0,     0,     0,   112,  1315,     0,     0,     0,
       0,     0,   573,     0,   573,     0,   671,   114,     0,     0,
     287,     0,   573,  2005,     0,   126,   150,   440,   573,     0,
       0,     0,     0,     0,     0,     0,   940,   126,     0,     0,
       0,   638,    50,    51,    52,    53,    54,     0,     0,     0,
       0,     0,     0,   473,     0,     0,     0,     0,     0,   769,
       0,   357,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   357,   357,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   290,   290,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   769,   769,
     769,     0,     0,     0,     0,     0,     0,   473,   473,     0,
       0,     0,   769,   287,     0,     0,     0,   290,     0,     0,
       0,     0,     0,     0,     0,     1,     0,     0,     0,     0,
     290,     0,     0,   638,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,   354,   355,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,    12,
       0,   354,   355,    15,    16,    17,    18,    19,    20,    21,
      22,   107,   440,     0,     0,     2,   126,     0,     0,     0,
     440,     0,     0,     0,   287,     0,     0,     0,   107,     0,
       3,     0,     0,   440,     0,     0,     0,  1881,   109,     0,
       0,     0,     0,   874,   874,   175,     0,     0,   112,   113,
       0,     0,   175,     0,  1314,   109,     0,     0,     0,     0,
     114,     0,     0,     0,     0,   112,  1315,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   126,   114,     0,     0,
       0,     0,     0,    50,    51,    52,    53,    54,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   150,
     150,     0,   290,     0,     0,     0,     0,     0,     0,   573,
       0,   118,     0,     0,   175,   440,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   573,   959,     0,   959,
     387,   126,   331,     0,     0,     0,     0,     0,     0,   440,
       0,     0,     0,     0,     0,   473,     0,     0,     0,     0,
       0,     0,   175,     0,     0,     0,     0,     0,   126,     0,
       0,     0,   573,     1,   573,   290,   290,   287,     0,   287,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   578,
       0,   573,   650,    12,     0,     0,     0,    15,    16,    17,
      18,    19,    20,    21,    22,     0,   650,    12,  1316,   354,
     355,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,   107,     2,     0,     0,   331,     0,     0,     0,
     573,   999,     0,     0,     0,     0,   107,     0,     3,     0,
       0,     0,     0,   126,     0,     0,   573,     0,   108,   109,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   112,
     113,     0,  1881,   109,     0,     0,   573,   175,     0,     0,
       0,   114,     0,   112,   113,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   114,     0,   126,     0,     0,
       0,   126,   126,     0,     0,   331,     0,   769,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   126,   630,
     769,     0,   769,   769,     0,   661,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   440,     0,     0,     0,     0,
       0,   387,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   692,     0,     0,     0,   573,     0,     0,   573,     0,
       0,     0,   126,     0,     0,     0,   440,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     874,     0,   874,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   183,   126,   753,   184,     0,   185,   186,     0,
     187,   753,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,   331,
       0,     0,   473,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   769,   842,   440,     0,   387,   126,   473,     0,
       0,     0,     0,     0,     0,   959,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
       0,   200,   201,     0,   331,     0,     0,     0,     0,   107,
       0,     0,   331,   573,   573,   331,     0,   331,   331,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   331,     0,
       0,   331,   331,   331,  1316,   202,  1316,   331,   110,   203,
       0,     0,     0,   753,   573,   204,   112,   113,   205,   206,
     207,   208,     0,     0,     0,     0,     0,   573,   209,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   331,     0,   387,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     126,     0,     0,   440,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   959,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   661,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   473,     0,     0,     0,   331,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   753,
     987,     0,   753,   990,     0,   994,     0,     0,   473,   287,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     661,     0,     0,     0,   661,   661,     0,   385,   118,     0,
       0,   661,     0,     0,     0,     0,     0,     0,     0,     0,
     769,  1047,     0,   440,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   440,     0,     0,     0,     0,
       0,     0,   573,   573,     0,     0,     0,     0,     0,     0,
       0,   630,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1316,     0,
    1316,  1316,     0,     0,     0,     0,     0,   331,     0,     0,
       0,     0,     0,     0,     0,     0,   753,     0,     0,     0,
     753,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     118,     0,     0,     0,     0,     0,     0,     0,     0,   440,
       0,     0,     0,   126,   126,     0,   753,     0,   440,     0,
       0,   331,   331,     0,     0,     0,   156,     0,     0,     0,
       0,     0,     0,   156,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   473,     0,     0,     0,   473,   473,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   473,     0,     0,   473,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   331,     0,   385,     0,
       0,     0,     0,     0,   331,   156,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   630,     0,   440,
       0,     0,     0,   156,     0,     0,     0,   753,   753,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   156,     0,     0,     0,     0,     0,     0,
     118,     0,   419,     0,     0,     0,     0,   473,     0,     0,
       0,     0,   440,     0,     0,     0,  1883,   842,     0,     0,
       0,     0,     0,     0,     0,     0,   156,     0,     0,     0,
     156,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   842,   440,   175,     0,     0,     0,   175,
       0,     0,     0,     0,     0,     0,     0,   156,     0,     0,
     994,   661,     0,   661,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   331,     0,     0,     0,     0,     0,   331,
       0,     0,     0,   753,  1276,     0,   753,  1279,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   156,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   661,     0,   661,     0,
       0,     0,     0,     0,     0,     0,   156,     0,     0,     0,
     661,     0,     0,     0,  1883,   440,  1883,     0,     0,     0,
     156,  1883,     0,  1883,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   994,     0,     0,     0,     0,     0,
       0,     0,     0,   842,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   155,     0,     0,   473,     0,     0,
       0,   155,     0,   156,     0,     0,   156,     0,     0,     0,
       0,     0,   440,     0,   753,     0,     0,     0,   753,     0,
       0,     0,     0,     0,     0,   753,  1353,     0,     0,   753,
    1357,     0,     0,   753,  1361,     0,     0,     0,     0,     0,
       0,   156,     0,  1364,  2053,     0,     0,     0,     0,     0,
    1883,     0,     0,  1883,  1883,   753,     0,     0,   156,     0,
     156,     0,     0,   155,     0,     0,   156,     0,     0,   175,
     156,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   155,   156,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     331,   155,     0,     0,   440,     0,  2113,   842,     0,   156,
       0,     0,     0,   156,   753,   156,   753,     0,     0,     0,
       0,     0,     0,   156,     0,     0,   156,     0,   156,   156,
       0,     0,  2053,  2053,   155,     0,     0,     0,   155,   156,
       0,     0,   156,   156,   156,     0,  1883,     0,   156,     0,
       0,     0,     0,     0,     0,     0,   175,     0,     0,     0,
       0,     0,     0,     0,     0,   155,     0,     0,   175,     0,
       0,     0,   331,     0,     0,     0,     0,     0,     0,   661,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  2113,  2113,     0,   156,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   155,     0,     0,     0,
       0,     0,     0,     0,     0,  2053,     0,     0,     0,     0,
       0,     0,   753,  1542,     0,   661,   661,  1549,     0,     0,
     440,   440,     0,     0,   155,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   155,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  2113,   769,     0,   753,  1579,     0,   753,  1583,
       0,     0,   753,  1587,     0,     0,     0,     0,   156,  2053,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   155,     0,     0,   155,     0,     0,   753,     0,     0,
       0,     0,     0,     0,     0,   156,     0,   175,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   661,     0,     0,     0,     0,     0,   155,
       0,     0,     0,   753,  1623,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   155,     0,   155,     0,
       0,     0,     0,     0,   155,     0,     0,     0,   155,     0,
       0,     0,     0,     0,     0,     0,     0,   175,     0,     0,
     155,     0,   156,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   331,     0,     0,     0,     0,   155,   156,     0,
       0,   155,     0,   155,     0,     0,     0,     0,     0,     0,
       0,   155,     0,     0,   155,     0,   155,   155,     0,     0,
       0,     0,   175,     0,     0,     0,     0,   155,     0,     0,
     155,   155,   155,     0,     0,     0,   155,   331,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   175,
       0,     0,   156,   156,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   160,     0,     0,     0,     0,     0,     0,
     160,     0,     0,     0,     0,     0,   156,     0,     0,     0,
       0,     0,     0,   155,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   156,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   156,   156,     0,     0,
       0,     0,     0,     0,   175,   156,     0,     0,     0,     0,
       0,     0,   160,   331,     0,     0,     0,     0,   156,     0,
       0,     0,   331,     0,     0,     0,     0,     0,     0,     0,
     160,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   155,     0,   175,     0,
     160,     0,   175,   175,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   331,     0,     0,   175,
       0,     0,     0,   155,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   160,     0,     0,     0,   160,     0,     0,
       0,     0,     0,     0,     0,     0,   156,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   175,   160,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   156,     0,     0,     0,     0,     0,
     156,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     155,     0,     0,     0,   175,     0,     0,     0,     0,     0,
       0,   331,     0,     0,     0,   160,   331,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   155,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   160,     0,     0,     0,     0,   175,     0,
       0,     0,     0,     0,     0,     0,     0,   160,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     155,   155,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     160,     0,     0,   160,   155,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   155,     0,     0,     0,   160,   331,
       0,     0,     0,     0,   155,   155,     0,     0,     0,     0,
       0,     0,   753,   155,     0,   160,     0,   160,     0,     0,
       0,     0,     0,   160,     0,     0,   155,   160,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   160,
       0,   175,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   156,     0,     0,     0,     0,   331,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   160,     0,     0,     0,
     160,     0,   160,     0,     0,     0,     0,     0,     0,     0,
     160,     0,     0,   160,     0,   160,   160,     0,   753,     0,
       0,     0,     0,     0,     0,   753,   160,     0,     0,   160,
     160,   160,     0,     0,   155,   160,     0,   156,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   156,     0,     0,     0,     0,     0,     0,
       0,     0,   155,     0,     0,     0,     0,   267,   155,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   331,     0,
     753,     0,   160,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   388,     0,
       0,     0,     0,     0,     0,     0,   753,   753,     0,     0,
     412,     0,   420,     0,   422,     0,     0,     0,     0,   753,
      56,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,     0,     0,   183,   461,    88,   184,    89,
     185,   186,     0,   187,     0,   160,   753,   753,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,   753,
       0,     0,   160,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   175,   175,     0,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,     0,
     197,   198,   199,     0,   200,   201,   753,     0,     0,     0,
       0,     0,   107,     0,     0,     0,     0,     0,   156,   753,
    2233,     0,     0,   753,     0,     0,     0,   388,     0,   160,
       0,     0,     0,   678,     0,   422,     0,     0,   202,   155,
       0,   110,   203,   156,     0,     0,     0,     0,   204,   112,
     113,   205,   206,   207,   208,   160,     0,     0,   388,     0,
     420,   422,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   156,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   156,     0,
       0,     0,     0,     0,     0,   155,   236,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   160,
     160,   155,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   160,     0,   388,     0,   420,   422,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   160,     0,     0,     0,     0,     0,     0,
     288,   388,     0,   160,   160,   156,   388,     0,     0,     0,
       0,     0,   160,     0,   156,     0,     0,     0,     0,     0,
       0,     0,     0,   156,     0,   160,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   388,     0,     0,     0,     0,   156,
       0,     0,     0,   156,   156,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   156,     0,     0,
     156,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   487,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   160,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   388,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   160,     0,     0,     0,     0,   155,   160,     0,   388,
       0,   678,   422,     0,     0,   156,     0,     0,     0,     0,
       0,     0,   156,     0,     0,   156,     0,   156,     0,     0,
     599,   155,     0,     0,     0,     0,     0,     0,     0,   388,
       0,     0,     0,   622,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   388,     0,   156,
       0,   155,     0,   388,     0,     0,     0,     0,     0,   388,
       0,   678,   422,     0,     0,     0,   155,     0,     0,     0,
     288,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   715,
       0,   715,     0,     0,     0,     0,     0,     0,     0,   388,
     678,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     388,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   388,     0,
       0,     0,     0,   461,     0,     0,     0,     0,     0,     0,
     156,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   155,     0,     0,     0,     0,     0,     0,
       0,     0,   155,     0,     0,     0,     0,   388,     0,     0,
       0,   155,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   156,     0,     0,     0,     0,     0,   839,     0,
       0,     0,     0,     0,   388,     0,     0,   155,   160,     0,
       0,   155,   155,     0,   388,     0,   857,     0,     0,     0,
       0,     0,     0,     0,     0,   155,     0,     0,   155,   622,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   388,     0,     0,   388,   388,     0,     0,     0,
     889,     0,   891,     0,     0,     0,     0,     0,   388,   236,
       0,   902,   236,     0,   160,     0,     0,     0,     0,     0,
       0,   388,     0,     0,     0,     0,   907,     0,     0,     0,
     160,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   155,   236,     0,     0,     0,     0,   937,
     155,     0,     0,   155,     0,   155,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   288,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   155,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   622,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1000,  1002,     0,   288,     0,     0,     0,     0,
       0,     0,   622,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   288,     0,  1025,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   156,   156,     0,     0,   288,
       0,     0,     0,     0,     0,     0,     0,   388,   155,     0,
       0,     0,     0,     0,   388,     0,     0,     0,     0,   288,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     388,     0,     0,     0,     0,     0,   839,     0,     0,     0,
     487,   715,     0,     0,     0,   160,     0,   715,     0,     0,
     155,     0,   599,     0,     0,   388,     0,     0,     0,     0,
     388,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     160,     0,     0,     0,     0,     0,     0,     0,     0,  1120,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     160,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   288,     0,   318,   160,     0,     0,     0,     0,
       0,   334,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,   388,   396,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,     0,     0,    88,     0,    89,   444,     0,
       0,     0,     0,   288,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   388,     0,     0,     0,     0,     0,
    1185,     0,   160,     0,     0,   484,     0,     0,     0,     0,
       0,   160,   902,     0,     0,   902,   236,  1206,     0,     0,
     160,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   458,     0,   459,   460,   715,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   160,   388,     0,     0,
     160,   160,     0,     0,   581,     0,     0,     0,     0,   388,
       0,     0,     0,     0,   160,     0,     0,   160,     0,     0,
       0,     0,     0,     0,   334,     0,     0,     0,     0,     0,
       0,     0,     0,   155,   155,     0,     0,   318,     0,     0,
     642,     0,     0,   -19,     0,     0,   674,     0,     0,     0,
       0,     0,     0,     0,  1271,     0,     0,     0,     0,     0,
     388,     0,     0,     0,     0,     0,   688,     0,     0,     0,
     694,     0,     0,     0,   318,     0,   622,   701,     0,     0,
       0,     0,  1292,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   160,     0,     0,     0,     0,     0,     0,   160,
       0,     0,   160,     0,   160,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1000,     0,     0,     0,     0,     0,   160,   318,   334,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   396,   715,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1340,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   581,   334,     0,   843,     0,     0,     0,     0,
       0,   334,     0,     0,   484,     0,   484,   334,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   484,     0,     0,
     484,   484,   484,   581,     0,     0,   334,   160,     0,     0,
       0,     0,     0,   701,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   388,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   160,
       0,     0,     0,   334,     0,     0,     0,     0,   715,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1433,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   388,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   318,   642,   599,   960,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   318,     0,     0,     0,     0,   334,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   996,     0,   674,     0,     0,     0,
       0,     0,     0,     0,     0,  1554,   318,     0,  1008,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   715,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   581,   642,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   318,     0,   318,     0,     0,
       0,     0,   388,   581,     0,  1065,     0,  1594,     0,  1595,
       0,   581,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,  1936,     0,     0,
       0,     0,     0,     0,     0,     0,   334,     0,     0,     0,
       0,     0,     0,     0,  1646,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   715,     0,  1433,     0,     0,
       0,     0,   160,   160,     0,     0,     0,     0,     0,     0,
       0,   388,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   183,     0,     0,   184,     0,   185,   186,     0,   187,
     334,   334,     0,     0,     0,     0,   318,   388,     0,     0,
       0,  1667,  1667,     0,  1442,     0,   188,  1444,   622,  1445,
       0,     0,  1446,  1447,  1448,  1449,  1450,  1451,  1452,  1453,
    1454,  1455,  1456,  -360,  -360,  1457,  1458,  1459,  1460,  1461,
    1462,  1463,     0,  1464,     0,   189,   190,   191,     0,   709,
     193,  1465,  1466,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,  1467,   334,   197,   198,   199,     0,
     200,   201,     0,   484,     0,     0,     0,   581,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  1721,
    1722,     0,     0,     0,  1468,     0,     0,   110,   203,     0,
       0,     0,   240,     0,   204,   112,   113,   205,   206,   207,
     208,     0,     0,     0,     0,     0,     0,   388,     0,  -200,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   318,     0,     0,  1763,     0,     0,  1765,     0,
       0,     0,     0,     0,     0,     0,     0,   183,     0,   996,
     184,     0,   185,   186,     0,   187,   236,     0,     0,     0,
       0,     0,   334,     0,     0,     0,     0,   388,   334,     0,
       0,     0,   188,     0,     0,   715,     0,     0,     0,     0,
       0,   599,     0,     0,     0,   318,     0,  1288,     0,     0,
     581,     0,   642,     0,     0,     0,     0,     0,   388,   422,
       0,   189,   190,   191,   318,   192,   193,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,   194,   195,   196,
       0,   388,   197,   198,   199,     0,   200,   201,     0,     0,
       0,     0,     0,     0,   107,     0,     0,     0,     0,     0,
       0,     0,     0,   996,   960,     0,     0,     0,     0,  1789,
    1790,     0,     0,  1791,  1792,     0,     0,  1865,     0,   318,
     202,  2000,     0,   110,   203,     0,   599,     0,     0,     0,
     204,   112,   113,   205,   206,   207,   208,     0,     0,   318,
      50,    51,    52,    53,    54,     0,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,     0,     0,    88,     0,    89,     0,     0,   388,
    2255,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1933,     0,     0,     0,     0,     1,  -882,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   581,     0,
       0,   318,     0,     0,     0,     0,   715,     0,     0,   484,
       0,     0,     0,     0,     0,     0,   388,     0,     0,     0,
       0,     0,     0,     0,  -882,     0,     0,  -882,     0,  -882,
    -882,     0,  -882,     0,     0,     0,     2,     0,     0,     0,
       0,  1972,     0,     0,     0,     0,     0,  -882,     1,  -882,
    -882,     3,  -882,  -261,  -261,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,     0,  -882,     0,  -882,  -882,
    -882,   334,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,     2,  -882,
    -882,  -882,     0,  -882,  -882,     0,     0,     0,     0,     0,
       0,  -882,     0,     3,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   318,   318,     0,  2255,
       0,     0,     0,     0,     0,     0,  -261,  -882,     0,     0,
    -882,  -882,     0,     0,     0,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,     0,     0,     0,   318,     0,     0,
       0,     0,  -882,     0,     0,     0,     0,     0,     0,     0,
     318,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -882,     0,     0,  -882,     0,  -882,  -882,
       0,  -882,     0,     0,   236,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  -882,     1,  -882,  -882,
       0,  -882,  -262,  -262,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,     0,  -882,     0,  -882,  -882,  -882,
       0,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,     2,  -882,  -882,
    -882,     0,  -882,  -882,     0,     0,     0,     0,     0,     0,
    -882,     0,     3,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -262,  -882,     0,     0,  -882,
    -882,   484,   318,     0,  -882,     0,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,     0,     0,     0,     0,     0,     0,     0,
       0,  -882,     0,     0,     0,     0,     0,     0,     0,     0,
     396,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   334,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   318,   318,     0,     0,     0,
      40,   263,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   183,     0,    88,   184,    89,   185,   186,     0,
     187,    90,    91,    92,    93,    94,    95,    96,    97,    98,
       0,     0,   484,    99,     0,     0,     0,   188,     0,     0,
       0,   484,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   189,   190,   191,   102,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,   334,     0,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   396,     0,     0,  1789,  1790,     0,     0,  1791,  1792,
       0,     0,     0,     0,     0,   202,  1793,  1794,   110,  1469,
       0,     0,     0,     0,     0,   204,   112,   113,   205,   206,
     207,   208,     0,     0,     0,     0,     0,     0,     0,     0,
    1795,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     334,     0,     0,     0,     0,   484,     0,     0,     0,     0,
     701,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   263,    42,    43,    44,    45,
      46,    47,    48,    49,     0,     0,   396,     0,     0,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,   378,   379,     0,   380,    88,     0,
      89,     0,     0,   381,     0,     0,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   484,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   396,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   484,     0,     0,     0,     0,
       0,  -484,     0,     0,     0,     0,  1229,     0,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -484,  -882,     0,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,     0,     0,
       0,  -882,     0,  -882,     1,  -882,  -882,   484,  -882,     0,
     701,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,     0,  -882,     0,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,     2,  -882,  -882,  -882,     0,  -882,
    -882,     0,     0,     0,     0,     0,     0,  -882,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1439,  -882,     0,     0,  -882,  -882,     0,     0,
       0,  -882,     0,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
       0,     0,     0,     0,     0,     0,     0,  1229,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,     0,  -882,     0,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,     0,
       0,     0,  -882,     0,  -882,     1,  -882,  -882,     0,  -882,
       0,     0,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,     0,  -882,     0,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,  -882,  -882,  -882,  -882,     2,  -882,  -882,  -882,     0,
    -882,  -882,     0,     0,     0,     0,     0,     0,  -882,     0,
       3,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -882,     0,     0,  -882,  -882,     0,
       0,     0,  -882,     0,  -882,  -882,  -882,  -882,  -882,  -882,
    -882,     0,     0,     0,     0,     0,     0,     0,     0,  -882,
      40,   263,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,  1441,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   183,     0,    88,   184,    89,   185,   186,     0,
     187,    90,    91,    92,    93,    94,    95,    96,    97,    98,
       0,     0,     0,    99,     0,  1442,     0,  1443,  1444,     0,
    1445,     0,     0,  1446,  1447,  1448,  1449,  1450,  1451,  1452,
    1453,  1454,  1455,  1456,  -360,  -360,  1457,  1458,  1459,  1460,
    1461,  1462,  1463,     0,  1464,     0,   189,   190,   191,   102,
     709,   193,  1465,  1466,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,  1467,     0,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1468,     0,     0,   110,  1469,
       0,     0,     0,   240,     0,   204,   112,   113,   205,   206,
     207,   208,     0,     0,     0,     0,     0,     0,     0,     0,
    -200,    40,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,   183,     0,    88,   184,    89,   185,   186,
       0,   187,    90,    91,    92,    93,    94,    95,    96,    97,
      98,     0,     0,     0,    99,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   189,   190,   191,
     102,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,     0,   200,   201,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1789,  1790,     0,     0,  1791,
    1792,     0,     0,     0,     0,     0,   202,  1793,     0,   110,
    1469,     0,     0,     0,     0,     0,   204,   112,   113,   205,
     206,   207,   208,     0,     0,     0,     0,     0,     0,     0,
       0,  1795,   383,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,  -509,  -509,     0,  -509,    88,     0,    89,     0,
       0,  -509,    50,    51,    52,    53,    54,     0,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,  -509,  -509,     0,  -509,    88,     0,    89,     0,
       0,  -509,    12,     0,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,   109,     0,
     110,   384,     0,     0,     0,  -851,     0,     0,   112,   113,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     114,   383,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,     0,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,  -509,  -509,     0,  -509,    88,     0,    89,     0,     0,
    -509,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,     0,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   108,   109,     0,   110,
     384,     0,     0,     0,     0,     0,     0,   112,   113,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   114,
     263,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,     0,
       0,     0,     0,    88,     0,    89,     0,     0,     0,     0,
     283,    91,    92,    93,    94,    95,    96,    97,     0,     0,
       0,     0,     0,     0,     0,     1,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    12,
       0,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,     0,     0,     0,     0,     2,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   107,     0,
       3,   822,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1012,   109,  -710,   110,   647,     0,
       0,     0,     0,     0,     0,   112,   113,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   114,   263,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,    56,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,  -509,  -509,     0,
    -509,    88,     0,    89,     0,     0,  -509,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    12,     0,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   108,   109,     0,   110,   384,     0,     0,     0,
    -855,     0,     0,   112,   113,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   114,   263,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
       0,     0,    56,     0,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,  -509,  -509,     0,  -509,    88,
       0,    89,     0,     0,  -509,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,     0,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     108,   109,     0,   110,   384,     0,     0,     0,     0,     0,
       0,   112,   113,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   114,    40,   263,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,   183,     0,    88,   184,
      89,   185,   186,     0,   187,    90,    91,    92,    93,    94,
      95,    96,    97,    98,     0,     0,     0,    99,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,   190,   191,   102,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       0,   197,   198,   199,     0,   200,   201,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
       0,  1787,   110,  1469,     0,     0,     0,     0,     0,   204,
     112,   113,   205,   206,   207,   208,    40,   263,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,   183,     0,
      88,   184,    89,   185,   186,     0,   187,    90,    91,    92,
      93,    94,    95,    96,    97,    98,     0,     0,     0,    99,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   189,   190,   191,   102,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     0,   197,   198,   199,     0,   200,   201,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   202,     0,     0,   110,  1469,     0,     0,     0,     0,
       0,   204,   112,   113,   205,   206,   207,   208,   263,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     0,    56,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,     0,     0,   183,
       0,    88,   184,    89,   185,   186,     0,   187,   283,    91,
      92,    93,    94,    95,    96,    97,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   189,   190,   191,     0,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,     0,   200,   201,
       0,     0,     0,     0,     0,     0,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   202,   485,     0,   110,   284,   621,   486,     0,
       0,     0,   204,   286,   113,   205,   206,   207,   208,   263,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
     183,     0,    88,   184,    89,   185,   186,     0,   187,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,     0,   200,
     201,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,   485,     0,   110,   633,   863,   486,
       0,     0,     0,   204,   286,   113,   205,   206,   207,   208,
     263,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,     0,
       0,   183,     0,    88,   184,    89,   185,   186,     0,   187,
     283,    91,    92,    93,    94,    95,    96,    97,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   189,   190,   191,     0,   192,
     193,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,     0,     0,   197,   198,   199,     0,
     200,   201,     0,     0,     0,     0,     0,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   202,   485,     0,   110,   284,   697,
     486,     0,     0,     0,   204,   286,   113,   205,   206,   207,
     208,   263,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   183,     0,    88,   184,    89,   185,   186,     0,
     187,   283,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,   485,     0,   110,   284,
     956,   486,     0,     0,     0,   204,   286,   113,   205,   206,
     207,   208,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,   183,     0,    88,   184,    89,   185,   186,
       0,   187,   283,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,     0,   200,   201,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   202,   485,     0,   110,
     633,  1050,   486,     0,     0,     0,   204,   286,   113,   205,
     206,   207,   208,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   183,     0,    88,   184,    89,   185,
     186,     0,   187,   283,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     0,   197,
     198,   199,     0,   200,   201,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,   485,     0,
     110,   284,   285,   486,     0,     0,     0,   204,   286,   113,
     205,   206,   207,   208,   263,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,     0,     0,   183,     0,    88,   184,    89,
     185,   186,     0,   187,   283,    91,    92,    93,    94,    95,
      96,    97,     0,     0,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,     0,
     197,   198,   199,     0,   200,   201,     0,     0,     0,     0,
       0,     0,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   202,     0,
       0,   110,   284,   285,     0,     0,     0,     0,   204,   286,
     113,   205,   206,   207,   208,   263,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,   183,     0,    88,   184,
      89,   185,   186,     0,   187,   283,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,   190,   191,     0,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       0,   197,   198,   199,     0,   200,   201,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
       0,     0,   110,   284,   697,     0,     0,     0,     0,   204,
     286,   113,   205,   206,   207,   208,   263,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    56,     0,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,     0,     0,   183,     0,    88,
     184,    89,   185,   186,     0,   187,   283,    91,    92,    93,
      94,    95,    96,    97,     0,     0,     0,     0,     0,     0,
       0,     0,   188,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   189,   190,   191,     0,   192,   193,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,   194,   195,   196,
       0,     0,   197,   198,   199,     0,   200,   201,     0,     0,
       0,     0,     0,     0,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     202,     0,     0,   110,   284,   956,     0,     0,     0,     0,
     204,   286,   113,   205,   206,   207,   208,   263,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,   183,     0,
      88,   184,    89,   185,   186,     0,   187,   283,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   189,   190,   191,     0,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     0,   197,   198,   199,     0,   200,   201,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   202,     0,     0,   110,   633,  1050,     0,     0,     0,
       0,   204,   286,   113,   205,   206,   207,   208,   263,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     0,    56,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,     0,     0,   183,
       0,    88,   184,    89,   185,   186,     0,   187,   283,    91,
      92,    93,    94,    95,    96,    97,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   189,   190,   191,     0,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,     0,   200,   201,
       0,     0,     0,     0,     0,     0,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   202,     0,     0,   110,   284,   621,     0,     0,
       0,     0,   204,   286,   113,   205,   206,   207,   208,   263,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
     183,     0,    88,   184,    89,   185,   186,     0,   187,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,     0,   200,
     201,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,     0,     0,   110,   633,   863,     0,
       0,     0,     0,   204,   286,   113,   205,   206,   207,   208,
     263,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,     0,
       0,   183,     0,    88,   184,    89,   185,   186,     0,   187,
     283,    91,    92,    93,    94,    95,    96,    97,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   189,   190,   191,     0,   192,
     193,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,     0,     0,   197,   198,   199,     0,
     200,   201,     0,     0,     0,     0,     0,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   202,     0,     0,   110,   633,     0,
       0,     0,     0,     0,   204,   838,   113,   205,   206,   207,
     208,   263,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   183,     0,    88,   184,    89,   185,   186,     0,
     187,   283,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,     0,     0,   110,   997,
       0,     0,     0,     0,     0,   204,   998,   113,   205,   206,
     207,   208,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,   183,     0,    88,   184,    89,   185,   186,
       0,   187,   283,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,     0,   200,   201,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   202,     0,     0,   110,
     633,     0,     0,     0,     0,     0,   204,   286,   113,   205,
     206,   207,   208,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   183,     0,    88,   184,    89,   185,
     186,     0,   187,   283,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     0,   197,
     198,   199,     0,   200,   201,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,     0,     0,
     110,   203,     0,     0,     0,     0,     0,   204,   112,   113,
     205,   206,   207,   208,  2075,     0,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,     0,    -3,     0,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,     0,    -3,
      -3,     0,    -3,     0,     0,    -3,     0,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,     0,     0,     0,    -3,
       0,     0,    -3,     0,     0,     0,     0,    -3,    -3,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    -3,     0,     0,    -3,    -3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    -3,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    -3,     0,    -3,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      -3,     0,     0,     0,    -3,    -3,     0,     0,     0,     0,
       0,     0,    -3,    -3,  2107,     0,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,     0,    -3,     0,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,    -3,    -3,     0,    -3,
      -3,     0,    -3,     0,     0,    -3,     0,    -3,    -3,    -3,
      -3,    -3,    -3,    -3,    -3,    -3,     0,     0,     0,    -3,
       0,     0,    -3,     0,     0,     0,     0,    -3,    -3,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    -3,     0,     0,    -3,    -3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    -3,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    -3,     0,    -3,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      -3,     0,  1239,     0,    -3,    -3,     0,     0,     0,     0,
       0,     0,    -3,    -3,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,  -508,  -508,     0,  -508,    88,     0,
      89,     0,     0,  -508,     0,   283,    91,    92,    93,    94,
      95,    96,    97,     0,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,   104,   105,    88,     0,
      89,     0,     0,     0,     0,   283,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  1382,     0,
    1239,     0,   110,   111,     0,     0,   104,   105,     0,     0,
     112,   113,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,  -508,  -508,     0,  -508,    88,     0,    89,     0,
       0,  -508,   110,   283,    91,    92,    93,    94,    95,    96,
      97,     0,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   104,   105,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1506,     0,  1239,     0,
     110,   111,     0,     0,   104,   105,     0,     0,   112,   113,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
    -508,  -508,     0,  -508,    88,     0,    89,     0,     0,  -508,
     110,   283,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   104,   105,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  1601,     0,  1239,     0,   110,   111,
       0,     0,     0,     0,     0,     0,   112,   113,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,  -508,  -508,
       0,  -508,    88,     0,    89,     0,     0,  -508,     0,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     104,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,  1708,     0,  1239,     0,   110,   111,     0,     0,
       0,     0,     0,     0,   112,   113,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,  -508,  -508,     0,  -508,
      88,     0,    89,     0,     0,  -508,     0,   283,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   104,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
    1850,     0,  1239,     0,   110,   111,     0,     0,     0,     0,
       0,     0,   112,   113,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,  -508,  -508,     0,  -508,    88,     0,
      89,     0,     0,  -508,     0,   283,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   104,   105,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   110,   111,     0,     0,     0,     0,     0,     0,
     112,   113,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     2,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     3,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,   109,     0,
     110,   321,     0,     0,     0,     0,     0,     0,   112,   113,
       0,     0,    50,    51,    52,    53,    54,    55,     0,    56,
     114,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     2,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     3,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,   109,     0,
     110,     0,     0,     0,     0,     0,     0,     0,   112,   113,
       0,     0,    50,    51,    52,    53,    54,    55,     0,    56,
     114,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,   109,     0,
     110,   111,     0,     0,     0,  -853,     0,     0,   112,   113,
       0,     0,    50,    51,    52,    53,    54,    55,     0,    56,
     114,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,   109,     0,
     110,   111,     0,     0,     0,     0,     0,     0,   112,   113,
       0,     0,    50,    51,    52,    53,    54,     0,     0,    56,
     114,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,  -509,  -509,     0,  -509,    88,     0,    89,     0,
       0,  -509,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    12,     0,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   108,   109,     0,
     110,   698,     0,     0,     0,     0,     0,     0,   112,   113,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     114,    40,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,     0,     0,    88,     0,    89,     0,     0,
       0,     0,    90,    91,    92,    93,    94,    95,    96,    97,
      98,     0,     0,     0,    99,     0,     0,     0,     0,     0,
       0,     0,  -428,  -428,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     102,     0,     0,   104,   105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  -428,     0,     0,     0,   110,
     111,     0,     0,     0,     0,     0,     0,   112,   113,    40,
     263,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,     0,
       0,     0,     0,    88,     0,    89,     0,     0,     0,     0,
      90,    91,    92,    93,    94,    95,    96,    97,    98,     0,
       0,     0,    99,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   102,     0,
       0,   104,   105,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   110,   111,     0,
    1749,     0,  1750,     0,     0,   112,   113,  1751,     0,     0,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,     0,     0,    88,     0,    89,     0,     0,     0,
       0,    90,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,  1752,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   102,
       0,     0,   104,   105,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   110,   854,
       0,     0,     0,     0,     0,     0,   112,   113,   383,   263,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,     0,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,  -509,  -509,
     383,  -509,    88,     0,    89,     0,     0,  -509,     0,     0,
      50,    51,    52,    53,    54,     0,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
    -509,  -509,     0,  -509,    88,     0,    89,     0,     0,  -509,
     104,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   104,   105,     0,     0,   110,   384,     0,     0,
       0,     0,     0,     0,   112,   113,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   110,   698,
       0,     0,     0,     0,     0,     0,   112,   113,   263,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     0,    56,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,     0,     0,     0,
       0,    88,     0,    89,     0,     0,     0,     0,   283,    91,
      92,    93,    94,    95,    96,    97,     0,     0,     0,     0,
       0,     0,     0,     1,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   104,
     105,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     2,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   107,     0,     3,   822,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   873,     0,  -710,   110,   750,     0,     0,     0,
       0,     0,     0,   112,   113,   263,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,     0,     0,    88,     0,
      89,     0,     0,     0,     0,   283,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   104,   105,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       2,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,     0,     3,   822,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   957,
       0,  -710,   110,   647,     0,     0,     0,     0,     0,     0,
     112,   113,   263,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    55,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,     0,     0,    88,     0,    89,     0,     0,
       0,     0,   283,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   104,   105,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     107,     0,     0,  1282,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,  -720,   110,
     783,     0,     0,     0,     0,     0,     0,   112,   113,   263,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    88,     0,    89,     0,     0,     0,     0,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     104,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   571,   110,   572,     0,     0,
       0,     0,     0,     0,   112,   113,   263,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,     0,    56,     0,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,     0,     0,     0,     0,    88,
       0,    89,     0,     0,     0,     0,   283,    91,    92,    93,
      94,    95,    96,    97,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   104,   105,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   110,   783,   784,     0,     0,     0,     0,
       0,   112,   113,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,   283,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   104,   105,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,  1712,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,   783,     0,     0,     0,     0,     0,     0,   112,   113,
     263,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    55,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,     0,
       0,     0,     0,    88,     0,    89,     0,     0,     0,     0,
     283,    91,    92,    93,    94,    95,    96,    97,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   104,   105,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   107,     0,
       0,  1714,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   110,   783,     0,
       0,     0,     0,     0,     0,   112,   113,   263,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,     0,     0,
      88,     0,    89,     0,     0,     0,     0,   283,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   104,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   110,   670,     0,     0,     0,     0,
       0,     0,   112,   113,   263,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    55,     0,
      56,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,     0,     0,     0,     0,    88,     0,    89,
       0,     0,     0,     0,   283,    91,    92,    93,    94,    95,
      96,    97,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   104,   105,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   110,   783,     0,     0,     0,     0,     0,     0,   112,
     113,   263,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,     0,     0,    88,     0,    89,     0,     0,     0,
       0,   283,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   104,   105,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   110,   572,
       0,     0,     0,     0,     0,     0,   112,   113,   263,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,     0,     0,    56,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,  -509,  -509,     0,
    -509,    88,     0,    89,     0,     0,  -509,     0,     0,   263,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,   104,
     105,     0,    88,     0,    89,  1646,     0,     0,     0,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   183,     0,     0,   184,     0,   185,   186,     0,
     187,     0,     0,     0,     0,   110,   384,     0,     0,     0,
     104,   105,     0,   112,   113,  1442,     0,   188,  1444,     0,
    1445,  1985,  1986,  1446,  1447,  1448,  1449,  1450,  1451,  1452,
    1453,  1454,  1455,  1456,     0,     0,  1457,  1458,  1459,  1460,
    1461,  1462,  1463,     0,  1464,     0,   189,   190,   191,     0,
     709,   193,  1465,  1466,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,  1467,   110,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,  1646,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,  1468,     0,     0,   110,   203,
       0,     0,     0,   240,     0,   204,   112,   113,   205,   206,
     207,   208,     0,   183,     0,     0,   184,     0,   185,   186,
    -200,   187,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1442,     0,   188,  1444,
       0,  1445,     0,     0,  1446,  1447,  1448,  1449,  1450,  1451,
    1452,  1453,  1454,  1455,  1456,     0,     0,  1457,  1458,  1459,
    1460,  1461,  1462,  1463,     0,  1464,     0,   189,   190,   191,
       0,   709,   193,  1465,  1466,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,  1467,     0,   197,   198,
     199,     0,   200,   201,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,  1468,     0,     0,   110,
     203,     0,     0,     0,   240,     0,   204,   112,   113,   205,
     206,   207,   208,     0,     0,     0,     0,     0,     0,     0,
       0,  -200,   432,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,  -431,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   104,   105,     0,   432,   263,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    55,     0,    56,     0,    57,    58,    59,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    72,    73,    74,    75,    76,    77,    78,    79,    80,
      81,    82,    83,    84,    85,    86,    87,     0,     0,     0,
     110,    88,     0,    89,     0,  -431,     0,     0,    90,    91,
      92,    93,    94,    95,    96,    97,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -432,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   104,
     105,     0,   432,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,   110,    88,     0,    89,     0,
    -432,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   104,   105,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,     0,     0,     0,     0,  -431,    50,    51,    52,    53,
      54,    55,   467,    56,   468,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,   183,     0,
      88,   184,    89,   185,   186,     0,   187,    90,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   469,     0,     0,     0,  1456,
       0,  -360,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   189,   190,   191,     0,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     0,   197,   198,   199,     0,   200,   201,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1468,     0,     0,   110,   470,     0,     0,     0,   240,
       0,   204,   112,   113,   471,   472,   207,   208,    50,    51,
      52,    53,    54,    55,   467,    56,   468,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
     183,     0,    88,   184,    89,   185,   186,     0,   187,    90,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   469,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,     0,   200,
     201,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,     0,     0,   110,   470,     0,     0,
       0,   240,     0,   204,   112,   113,   471,   472,   207,   208,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   183,     0,    88,   184,    89,   185,   186,     0,
     187,    90,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     1,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     2,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     3,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,     0,   597,   110,   598,
       0,     0,     0,     0,     0,   204,   112,   113,   205,   206,
     207,   208,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   183,     0,    88,   184,    89,   185,
     186,     0,   187,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     1,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     2,   197,
     198,   199,     0,   200,   201,     0,     0,     0,     0,     0,
       0,   107,     0,     3,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,     0,     0,
     110,   598,     0,     0,     0,   240,     0,   204,   112,   113,
     205,   206,   207,   208,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,   183,     0,    88,   184,
      89,   185,   186,     0,   187,   283,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       1,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,   190,   191,     0,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       2,   197,   198,   199,     0,   200,   201,     0,     0,     0,
       0,     0,     0,   107,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
       0,     0,   110,   633,     0,     0,     0,     0,     0,   204,
     112,   113,   205,   206,   207,   208,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,   183,     0,
      88,   184,    89,   185,   186,     0,   187,    90,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     1,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   189,   190,   191,     0,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     2,   197,   198,   199,     0,   200,   201,     0,
       0,     0,     0,     0,     0,   107,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   202,     0,     0,   110,   598,     0,     0,     0,     0,
       0,   204,   112,   113,   205,   206,   207,   208,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
     183,     0,    88,   184,    89,   185,   186,     0,   187,    90,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,     0,   200,
     201,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,     0,     0,   110,   470,     0,     0,
       0,   240,     0,   204,   112,   113,   205,   206,   207,   208,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   183,     0,    88,   184,    89,   185,   186,     0,
     187,    90,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
       0,   200,   201,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,     0,     0,   110,   598,
       0,     0,     0,   240,     0,   204,   112,   113,   205,   206,
     207,   208,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   183,     0,    88,   184,    89,   185,
     186,     0,   187,   283,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     0,   197,
     198,   199,     0,   200,   201,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,     0,     0,
     110,   203,     0,     0,     0,     0,     0,   204,   112,   113,
     205,   206,   207,   208,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,   183,     0,    88,   184,
      89,   185,   186,     0,   187,   283,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     189,   190,   191,     0,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       0,   197,   198,   199,     0,   200,   201,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
       0,     0,   110,   633,     0,     0,     0,     0,     0,   204,
     112,   113,   205,   206,   207,   208,   383,   263,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,     0,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,  -509,  -509,     0,  -509,
      88,     0,    89,     0,     0,  -509,     0,   263,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,   104,   105,
      88,     0,    89,     0,     0,     0,     0,   283,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   110,     0,    12,     0,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,     0,     0,
       0,     0,     2,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     3,   822,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,  -710,   110,    50,    51,    52,    53,    54,
      55,     0,    56,     0,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,     0,     0,     0,     0,    88,
       0,    89,     0,     0,     0,     0,   283,    91,    92,    93,
      94,    95,    96,    97,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    12,     0,   625,   105,    15,
      16,    17,    18,    19,    20,    21,    22,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     992,     0,     0,   110,   690,     0,     0,     0,     0,     0,
       0,   112,   113,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,   283,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     1,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   104,   105,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     2,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     3,   822,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -710,
     110,    50,    51,    52,    53,    54,    55,     0,    56,     0,
      57,    58,    59,    60,    61,    62,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    73,    74,    75,    76,
      77,    78,    79,    80,    81,    82,    83,    84,    85,    86,
      87,     0,     0,     0,     0,    88,     0,    89,     0,     0,
       0,     0,    90,    91,    92,    93,    94,    95,    96,    97,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    12,     0,   625,   105,    15,    16,    17,    18,    19,
      20,    21,    22,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   110,
     626,     0,     0,     0,     0,     0,     0,   112,   113,   263,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    88,     0,    89,     0,     0,     0,     0,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     1,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     104,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     2,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    50,    51,
      52,    53,    54,    55,     0,    56,   110,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    88,     0,    89,     0,     0,     0,     0,    90,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     1,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     104,   105,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     2,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     3,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,   321,     0,     0,
       0,     0,     0,     0,   112,   113,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,     0,     0,
      88,     0,    89,     0,     0,     0,     0,   283,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     1,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   104,   105,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     2,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   107,     0,     3,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   110,   572,     0,     0,     0,     0,
       0,     0,   112,   113,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,     0,     0,    88,     0,
      89,     0,     0,     0,     0,    90,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       1,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   104,   105,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       2,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,     0,     3,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   110,   854,     0,     0,     0,     0,     0,     0,
     112,   113,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   104,   105,    88,     0,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   859,
     110,   854,     0,     0,   104,   105,     0,     0,   112,   113,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   992,     0,     0,
     110,   626,     0,     0,     0,     0,     0,     0,   112,   113,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,     0,     0,    88,     0,    89,     0,     0,     0,
       0,   283,    91,    92,    93,    94,    95,    96,    97,     0,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   104,   105,    88,     0,    89,     0,     0,     0,
       0,    90,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,  1401,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   992,     0,     0,   110,   690,
       0,     0,   104,   105,     0,     0,   112,   113,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   110,   854,
       0,     0,     0,     0,     0,     0,   112,   113,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    88,     0,    89,     0,     0,     0,     0,    90,
      91,    92,    93,    94,    95,    96,    97,     0,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
     104,   105,    88,     0,    89,     0,     0,     0,     0,    90,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,   439,     0,     0,
     104,   105,     0,     0,   112,   113,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,   111,     0,     0,
       0,     0,     0,     0,   112,   113,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,     0,     0,
      88,     0,    89,     0,     0,     0,     0,   283,    91,    92,
      93,    94,    95,    96,    97,     0,    50,    51,    52,    53,
      54,    55,     0,    56,     0,    57,    58,    59,    60,    61,
      62,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
      82,    83,    84,    85,    86,    87,     0,     0,   104,   105,
      88,     0,    89,     0,     0,     0,     0,   283,    91,    92,
      93,    94,    95,    96,    97,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   110,   439,     0,     0,   104,   105,
       0,     0,   112,   113,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   110,   690,     0,     0,     0,     0,
       0,     0,   112,   113,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,     0,     0,    88,     0,
      89,     0,     0,     0,     0,    90,    91,    92,    93,    94,
      95,    96,    97,     0,    50,    51,    52,    53,    54,    55,
       0,    56,     0,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    73,
      74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
      84,    85,    86,    87,     0,     0,   104,   105,    88,     0,
      89,     0,     0,     0,     0,    90,    91,    92,    93,    94,
      95,    96,    97,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   110,   707,     0,     0,   104,   105,     0,     0,
     112,   113,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   110,   854,     0,     0,     0,     0,     0,     0,
     112,   113,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,   283,    91,    92,    93,    94,    95,    96,
      97,     0,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   104,   105,    88,     0,    89,     0,
       0,     0,     0,   283,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,   670,     0,     0,   104,   105,     0,     0,   112,   113,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,   572,     0,     0,     0,     0,     0,     0,   112,   113,
     263,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,     0,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,  -509,
    -509,     0,  -509,    88,     0,    89,     0,     0,  -509,     0,
       0,     0,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,   104,   105,    90,    91,    92,    93,    94,    95,    96,
      97,     0,    50,    51,    52,    53,    54,    55,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,   104,   105,    88,   110,    89,     0,
       0,     0,     0,    90,    91,    92,    93,    94,    95,    96,
      97,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,   321,     0,     0,   104,   105,     0,     0,   112,   113,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     110,   626,     0,     0,     0,     0,     0,     0,   112,   113,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,     0,     0,    88,     0,    89,     0,     0,     0,
       0,   283,    91,    92,    93,    94,    95,    96,    97,     0,
      50,    51,    52,    53,    54,    55,     0,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
       0,     0,   104,   105,    88,     0,    89,     0,     0,     0,
       0,   283,    91,    92,    93,    94,    95,    96,    97,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   110,   939,
       0,     0,   104,   105,     0,     0,   112,   113,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   110,   854,
       0,     0,     0,     0,     0,     0,   112,   113,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
       0,     0,    88,     0,    89,     0,     0,     0,     0,   283,
      91,    92,    93,    94,    95,    96,    97,     0,    50,    51,
      52,    53,    54,    55,     0,    56,     0,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
      80,    81,    82,    83,    84,    85,    86,    87,     0,     0,
     104,   105,    88,     0,    89,     0,     0,     0,     0,   283,
      91,    92,    93,    94,    95,    96,    97,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,   698,     0,     0,
     104,   105,     0,     0,   112,   113,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   110,     0,     0,    50,
      51,    52,    53,    54,   112,   113,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,  -509,
    -509,     0,  -509,    88,     0,    89,     0,     0,  -509,    50,
      51,    52,    53,    54,     0,     0,    56,     0,    57,    58,
      59,    60,    61,    62,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,    80,    81,    82,    83,    84,    85,    86,    87,  -509,
    -509,     0,  -509,    88,     0,    89,     0,     0,  -509,     0,
       0,   104,   105,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   104,   105,     0,     0,     0,     0,   110,   698,     0,
       0,     0,     0,     0,     0,   112,   113,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   110,   939,     0,
      50,    51,    52,    53,    54,   112,   113,    56,     0,    57,
      58,    59,    60,    61,    62,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
    -509,  -509,     0,  -509,    88,     0,    89,     0,     0,  -509,
      56,     0,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,     0,     0,   183,     0,    88,   184,    89,
     185,   186,     0,   187,     0,     0,     0,     0,     0,     0,
       0,     0,   104,   105,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,   110,     0,
     197,   198,   199,     0,   200,   201,   112,   113,     0,     0,
       0,     0,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   202,     0,
       0,   110,   203,  1119,     0,     0,     0,     0,   204,   286,
     113,   205,   206,   207,   208,    50,    51,    52,    53,    54,
      55,     0,    56,     0,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,     0,     0,     0,     0,    88,
       0,    89,     0,     0,     0,     0,   283,    91,    92,    93,
      94,    95,    96,    97,     0,     0,     0,     0,     0,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   104,   105,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
      55,     0,    56,   110,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,     0,     0,     0,     0,    88,
       0,    89,     0,     0,     0,     0,    90,    91,    92,    93,
      94,    95,    96,    97,     0,     0,     0,     0,     0,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   104,   105,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     2,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     3,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    50,    51,    52,    53,    54,
       0,     0,    56,   110,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,  -509,  -509,     0,  -509,    88,
       0,    89,     0,     0,  -509,    50,    51,    52,    53,    54,
       0,     0,    56,     0,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    87,  -508,  -508,     0,  -508,    88,
       0,    89,     0,     0,  -508,     0,     0,   104,   105,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     1,     0,     0,     0,     0,     0,     0,     0,   183,
       0,     0,   184,     0,   185,   186,     0,   187,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,   110,     0,     0,     0,     0,     0,     0,
       0,     2,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   189,   190,   191,     3,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,     0,   200,   201,
       0,     0,   183,     0,     0,   184,   107,   185,   186,     0,
     187,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,  1789,  1790,     0,     0,  1791,  1792,   188,     0,     0,
       0,     0,   202,  1919,     0,   110,   203,     0,     0,     0,
       0,     0,   204,   112,   113,   205,   206,   207,   208,     0,
       0,     0,     0,     0,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
     183,   200,   201,   184,     0,   185,   186,     0,   187,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,   485,     0,   110,   203,
       0,   486,     0,     0,     0,   204,   112,   113,   205,   206,
     207,   208,     0,     0,   189,   190,   191,     0,   709,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,   183,   200,
     201,   184,     0,   185,   186,     0,   187,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,   109,     0,   710,   711,     0,     0,
       0,   712,     0,   204,   112,   113,   205,   206,   207,   208,
       0,     0,   189,   190,   191,     0,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     0,   197,   198,   199,   183,   200,   201,   184,
       0,   185,   186,     0,   187,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   202,  1198,     0,   110,   203,     0,     0,     0,  1199,
       0,   204,   112,   113,   205,   206,   207,   208,     0,     0,
     189,   190,   191,     0,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       0,   197,   198,   199,   183,   200,   201,   184,     0,   185,
     186,     0,   187,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
     901,     0,   110,   203,  1203,     0,     0,     0,     0,   204,
     112,   113,   205,   206,   207,   208,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     0,   197,
     198,   199,   183,   200,   201,   184,     0,   185,   186,     0,
     187,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,     0,     0,
     110,   203,     0,     0,     0,   712,     0,   204,   112,   113,
     205,   206,   207,   208,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
     183,   200,   201,   184,     0,   185,   186,     0,   187,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,     0,     0,   110,   203,
       0,     0,     0,   240,     0,   204,   112,   113,   205,   206,
     207,   208,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,   183,   200,
     201,   184,     0,   185,   186,     0,   187,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,   901,     0,   110,   203,     0,     0,
       0,     0,     0,   204,   112,   113,   205,   206,   207,   208,
       0,     0,   189,   190,   191,     0,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     0,   197,   198,   199,   183,   200,   201,   184,
       0,   185,   186,     0,   187,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   202,     0,     0,   110,   203,     0,     0,   929,     0,
       0,   204,   112,   113,   205,   206,   207,   208,     0,     0,
     189,   190,   191,     0,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       0,   197,   198,   199,   183,   200,   201,   184,     0,   185,
     186,     0,   187,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
       0,     0,   110,   203,  1022,     0,     0,     0,     0,   204,
     286,   113,   205,   206,   207,   208,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     0,   197,
     198,   199,   183,   200,   201,   184,     0,   185,   186,     0,
     187,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,     0,     0,
     110,   203,  1048,     0,     0,     0,     0,   204,   112,   113,
     205,   206,   207,   208,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
     183,   200,   201,   184,     0,   185,   186,     0,   187,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,  1432,     0,   110,   203,
       0,     0,     0,     0,     0,   204,   112,   113,   205,   206,
     207,   208,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,   183,   200,
     201,   184,     0,   185,   186,     0,   187,   107,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   188,     0,     0,     0,     0,     0,     0,
       0,     0,     0,   202,     0,     0,   110,   203,  1592,     0,
       0,     0,     0,   204,   112,   113,   205,   206,   207,   208,
       0,     0,   189,   190,   191,     0,   192,   193,   104,   105,
      15,    16,    17,    18,    19,    20,    21,    22,   194,   195,
     196,     0,     0,   197,   198,   199,   183,   200,   201,   184,
       0,   185,   186,     0,   187,   107,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   188,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   202,     0,     0,   110,   203,     0,     0,     0,  1656,
       0,   204,   112,   113,   205,   206,   207,   208,     0,     0,
     189,   190,   191,     0,   192,   193,   104,   105,    15,    16,
      17,    18,    19,    20,    21,    22,   194,   195,   196,     0,
       0,   197,   198,   199,   183,   200,   201,   184,     0,   185,
     186,     0,   187,   107,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   188,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   202,
       0,     0,   110,   203,     0,     0,     0,  1847,     0,   204,
     112,   113,   205,   206,   207,   208,     0,     0,   189,   190,
     191,     0,   192,   193,   104,   105,    15,    16,    17,    18,
      19,    20,    21,    22,   194,   195,   196,     0,     0,   197,
     198,   199,   183,   200,   201,   184,     0,   185,   186,     0,
     187,   107,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   188,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   202,     0,  1991,
     110,   203,     0,     0,     0,     0,     0,   204,   112,   113,
     205,   206,   207,   208,     0,     0,   189,   190,   191,     0,
     192,   193,   104,   105,    15,    16,    17,    18,    19,    20,
      21,    22,   194,   195,   196,     0,     0,   197,   198,   199,
     183,   200,   201,   184,     0,   185,   186,     0,   187,   107,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   188,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   202,  1996,     0,   110,   203,
       0,     0,     0,     0,     0,   204,   112,   113,   205,   206,
     207,   208,     0,     0,   189,   190,   191,     0,   192,   193,
     104,   105,    15,    16,    17,    18,    19,    20,    21,    22,
     194,   195,   196,     0,     0,   197,   198,   199,     0,   200,
     201,     0,     0,   183,     0,     0,   184,   107,   185,   186,
       0,   187,  2093,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,   202,  2006,     0,   110,   203,     0,     0,
       0,     0,     0,   204,   112,   113,   205,   206,   207,   208,
       0,     0,     0,     0,     0,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,   183,   200,   201,   184,     0,   185,   186,     0,   187,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   202,     0,     0,   110,
     203,     0,     0,     0,     0,     0,   204,   112,   113,   205,
     206,   207,   208,     0,     0,   189,   190,   191,     0,   192,
     193,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,     0,     0,   197,   198,   199,   183,
     200,   201,   184,     0,   185,   186,     0,   187,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   202,  2100,     0,   110,   203,     0,
       0,     0,     0,     0,   204,   112,   113,   205,   206,   207,
     208,     0,     0,   189,   190,   191,     0,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,   183,   200,   201,
     184,     0,   185,   186,     0,   187,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   188,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   202,  2102,     0,   110,   203,     0,     0,     0,
       0,     0,   204,   112,   113,   205,   206,   207,   208,     0,
       0,   189,   190,   191,     0,   192,   193,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,   194,   195,   196,
       0,     0,   197,   198,   199,   183,   200,   201,   184,     0,
     185,   186,     0,   187,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     202,  2158,     0,   110,   203,     0,     0,     0,     0,     0,
     204,   112,   113,   205,   206,   207,   208,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,     0,
     197,   198,   199,   183,   200,   201,   184,     0,   185,   186,
       0,   187,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   202,  2160,
       0,   110,   203,     0,     0,     0,     0,     0,   204,   112,
     113,   205,   206,   207,   208,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,   183,   200,   201,   184,     0,   185,   186,     0,   187,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   202,  2162,     0,   110,
     203,     0,     0,     0,     0,     0,   204,   112,   113,   205,
     206,   207,   208,     0,     0,   189,   190,   191,     0,   192,
     193,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,     0,     0,   197,   198,   199,   183,
     200,   201,   184,     0,   185,   186,     0,   187,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   202,  2168,     0,   110,   203,     0,
       0,     0,     0,     0,   204,   112,   113,   205,   206,   207,
     208,     0,     0,   189,   190,   191,     0,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,   183,   200,   201,
     184,     0,   185,   186,     0,   187,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   188,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   202,  2170,     0,   110,   203,     0,     0,     0,
       0,     0,   204,   112,   113,   205,   206,   207,   208,     0,
       0,   189,   190,   191,     0,   192,   193,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,   194,   195,   196,
       0,     0,   197,   198,   199,   183,   200,   201,   184,     0,
     185,   186,     0,   187,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     202,  2213,     0,   110,   203,     0,     0,     0,     0,     0,
     204,   112,   113,   205,   206,   207,   208,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,     0,
     197,   198,   199,   183,   200,   201,   184,     0,   185,   186,
       0,   187,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   202,  2215,
       0,   110,   203,     0,     0,     0,     0,     0,   204,   112,
     113,   205,   206,   207,   208,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,   183,   200,   201,   184,     0,   185,   186,     0,   187,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   202,  2217,     0,   110,
     203,     0,     0,     0,     0,     0,   204,   112,   113,   205,
     206,   207,   208,     0,     0,   189,   190,   191,     0,   192,
     193,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,     0,     0,   197,   198,   199,   183,
     200,   201,   184,     0,   185,   186,     0,   187,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   202,  2240,     0,   110,   203,     0,
       0,     0,     0,     0,   204,   112,   113,   205,   206,   207,
     208,     0,     0,   189,   190,   191,     0,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,   183,   200,   201,
     184,     0,   185,   186,     0,   187,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   188,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   202,  2242,     0,   110,   203,     0,     0,     0,
       0,     0,   204,   112,   113,   205,   206,   207,   208,     0,
       0,   189,   190,   191,     0,   192,   193,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,   194,   195,   196,
       0,     0,   197,   198,   199,   183,   200,   201,   184,     0,
     185,   186,     0,   187,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     202,  2244,     0,   110,   203,     0,     0,     0,     0,     0,
     204,   112,   113,   205,   206,   207,   208,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,     0,
     197,   198,   199,   183,   200,   201,   184,     0,   185,   186,
       0,   187,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   448,     0,
       0,   110,   203,     0,     0,     0,     0,     0,   204,   112,
     113,   205,   206,   207,   208,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,   183,   200,   201,   184,     0,   185,   186,     0,   187,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   188,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   451,     0,     0,   110,
     203,     0,     0,     0,     0,     0,   204,   112,   113,   205,
     206,   207,   208,     0,     0,   189,   190,   191,     0,   192,
     193,   104,   105,    15,    16,    17,    18,    19,    20,    21,
      22,   194,   195,   196,     0,     0,   197,   198,   199,   183,
     200,   201,   184,     0,   185,   186,     0,   187,   107,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   188,     0,     0,     0,     0,     0,
       0,     0,     0,     0,   202,     0,     0,   110,   203,     0,
       0,     0,     0,     0,   204,   112,   113,   205,   206,   207,
     208,     0,     0,   189,   190,   191,     0,   192,   193,   104,
     105,    15,    16,    17,    18,    19,    20,    21,    22,   194,
     195,   196,     0,     0,   197,   198,   199,   183,   200,   201,
     184,     0,   185,   186,     0,   187,   107,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   188,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   464,     0,     0,   110,   203,     0,     0,     0,
       0,     0,   204,   112,   113,   205,   206,   207,   208,     0,
       0,   189,   190,   191,     0,   192,   193,   104,   105,    15,
      16,    17,    18,    19,    20,    21,    22,   194,   195,   196,
       0,     0,   197,   198,   199,   183,   200,   201,   184,     0,
     185,   186,     0,   187,   107,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     188,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     520,     0,     0,   110,   203,     0,     0,     0,     0,     0,
     204,   112,   113,   205,   206,   207,   208,     0,     0,   189,
     190,   191,     0,   192,   193,   104,   105,    15,    16,    17,
      18,    19,    20,    21,    22,   194,   195,   196,     0,     0,
     197,   198,   199,   183,   200,   201,   184,     0,   185,   186,
       0,   187,   107,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   188,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   202,     0,
       0,   110,   203,     0,     0,     0,     0,     0,   204,   286,
     113,   205,   206,   207,   208,     0,     0,   189,   190,   191,
       0,   192,   193,   104,   105,    15,    16,    17,    18,    19,
      20,    21,    22,   194,   195,   196,     0,     0,   197,   198,
     199,     0,   200,   201,     0,     0,     0,     0,     0,     0,
     107,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   202,     0,     0,   110,
     203,     0,     0,     0,     0,     0,   204,   838,   113,   205,
     206,   207,   208,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,     0,     0,     0,     0,    88,     0,    89,     0,
       0,     0,     0,   263,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,     0,     0,    56,
       0,    57,    58,    59,    60,    61,    62,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    72,    73,    74,    75,
      76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
      86,    87,   264,     0,   265,   266,    88,     0,    89,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   458,     0,   459,   460
};

static const yytype_int16 yycheck[] =
{
       3,   291,    30,   553,   138,    31,     9,  1238,    38,     7,
     500,   646,    38,   503,   202,   454,   342,   291,   239,   262,
     251,   382,   293,   717,  1251,    31,   475,   356,    31,  1484,
      33,   253,    38,    36,  1145,    38,   262,    40,   108,   682,
     356,   684,  1468,   108,   171,   108,   108,    31,     1,   118,
     108,   723,   131,   108,    38,    31,   356,   261,   253,  1989,
    1232,   646,    38,   204,   898,    31,   646,   874,    39,   291,
     100,    31,    38,   366,   100,   399,   646,   143,    38,   650,
    1244,   313,  1468,   712,  1149,   317,  1985,   110,   188,   245,
      82,     1,   356,   671,   100,    98,    99,   100,   932,   106,
     646,   356,   395,  2052,   356,   108,   356,   175,  1210,   243,
     791,   114,   356,    82,   407,   118,   100,    96,   218,   219,
     145,    95,   803,   126,   100,   191,    31,    91,   131,   646,
     126,   134,   138,    38,   100,   138,   356,   652,  2149,  1644,
     100,   656,   651,  1648,  1649,   171,   646,   662,   657,  1257,
     134,   168,   118,   356,   138,   107,    84,    85,     3,   184,
    1665,   176,    95,  2112,     9,   131,     0,  2178,   134,  2052,
    2052,  1236,   138,   176,   689,    10,   179,   647,   193,   729,
    2026,   696,    84,    85,    79,   365,   193,   287,    33,   168,
     293,    36,   372,  2204,     0,   100,   167,    68,    69,  1232,
       0,   193,   176,   273,   168,   170,   209,  2053,   273,   174,
     273,   273,   240,   133,   394,   273,   168,   285,   273,  1383,
     770,    31,   175,  1452,   193,   695,   406,  1378,    38,  2112,
    2112,   202,   782,   138,   129,  1069,  1387,   243,   708,   366,
     176,   174,   245,   176,   408,    79,    82,  2146,   176,   144,
     253,   171,    79,    98,    99,   171,   172,   727,   284,   243,
      96,   942,   426,   331,   174,   175,   171,  2113,   395,   272,
     273,  1443,  2202,   168,   176,   273,   192,   906,   357,    79,
     407,   126,   285,  1514,  1515,    79,   356,   290,   291,  2135,
     100,   356,   158,   356,   356,   129,  1682,   127,   356,  1685,
    1686,   356,   129,   176,   575,   176,   272,   434,  2238,   450,
     144,   314,   145,   501,   127,   318,   385,   144,   184,  1384,
     193,   324,   193,   115,   116,   328,  1428,  1429,  1430,   129,
     328,   176,   167,   193,   179,   129,  2182,   340,   777,   778,
     366,   168,   345,   178,   144,   172,   179,   180,   183,   352,
     144,  1502,   623,   356,   357,    20,   170,   436,   324,   172,
     582,   171,   328,   359,   635,   361,   168,   181,   182,   395,
    1649,   440,   368,   473,   377,   346,   846,  1012,   172,   171,
    1074,   407,   385,  1190,   387,  1192,  1665,   582,   959,   621,
     192,   357,   718,   396,    79,   398,   169,   887,   401,   168,
     245,   424,   731,   176,   473,   408,    79,  1833,   434,   165,
    1443,   957,  1917,    79,  1525,   731,  1524,   401,   867,   385,
     176,   999,   425,   426,   524,   428,   669,  1012,   650,   432,
     754,   731,  1012,   436,   590,   401,   192,   593,   762,   768,
     957,   444,  1012,   669,   129,   290,   168,  1833,   170,   445,
     446,  1602,   768,   684,   168,  1026,   129,   957,   145,   144,
     692,   366,   433,   129,   119,  1146,  1012,   731,   768,   314,
     436,   144,   575,   318,   173,   174,   731,   448,   144,   731,
     451,   731,   167,   193,    82,   711,   173,   731,   143,   492,
     395,   494,   170,   464,   497,  1012,   174,   184,   501,    97,
     345,   504,   407,   176,   768,  1014,   992,  1201,   801,   168,
    1204,   731,  1012,   768,   119,   170,   768,   488,   768,   117,
     623,  1800,   726,   546,   768,  1011,  1796,  1221,   731,   434,
     969,    10,   635,   438,  1004,  1005,   975,   118,   143,   172,
    1055,    82,   387,   546,   177,  1709,   572,   172,   768,   520,
     553,   396,   177,   398,    79,  1011,   366,    98,   568,   562,
     563,   142,   630,   408,   567,   768,  1795,  1117,   167,  1263,
    1199,  1800,   675,   115,   116,   174,  1248,   174,   758,   582,
    1066,   426,   592,   686,  2039,   395,   170,   590,   168,  1423,
     593,   175,  1700,  1701,   193,   202,   193,   407,    79,   444,
    1086,   251,   605,   606,   129,   603,   168,   633,   606,   131,
    1066,  1644,  1839,   793,  1084,  1648,  1649,   620,   621,   144,
     800,   181,   182,   174,   434,   805,   174,   174,    79,   697,
    1086,     9,  1665,    84,    85,  1914,   184,   184,  1917,   605,
     606,   639,   193,   646,   670,  2150,  2151,   650,   129,  1919,
    1920,   176,   179,   140,   141,    33,   501,   174,   183,   186,
     187,   731,   168,   144,   743,   744,   731,   184,   731,   731,
      79,   674,   131,   731,   801,   174,   731,   680,   129,  1314,
     174,  1324,  1168,   174,  1255,  1914,   167,   710,   167,   174,
     769,   174,  1242,   144,   193,   174,   525,   526,   527,   178,
     174,   184,   193,   929,   183,   768,   768,   710,   193,   712,
     768,   883,  1168,   174,   886,  1946,   167,   562,   563,   193,
     129,   824,   567,  1417,   956,   728,   729,   158,   731,  1314,
    2000,  2001,  1840,   731,  1314,   144,  1026,   174,  1028,   174,
     743,   744,   173,   174,  1314,   590,   744,   174,   593,   174,
    1076,   174,  1026,   184,  1028,   174,   193,   783,   167,   859,
      79,   184,   994,   193,   864,   768,   769,   770,   193,   973,
    1999,   174,   967,   842,   193,   801,     3,   743,   744,   782,
    1051,   784,   882,   781,   168,    76,   757,  1416,  2017,   439,
     193,   175,  1301,  1302,   765,   863,   952,  1631,   174,  1633,
      79,  1130,  1024,   769,  1026,    79,  1028,  1293,  1294,    65,
     129,   945,    68,    69,  1130,    71,   590,   193,  1050,   593,
     470,   792,     3,  1517,   174,   144,   174,   171,  1399,   674,
    1130,   170,   830,   804,   184,   357,   175,  1293,  1294,   174,
     843,   448,   170,   841,   451,   193,   453,   175,   844,   168,
     129,  1337,   850,   172,  1080,   129,   174,   464,   193,   170,
     467,   468,   469,  2092,   175,   144,  1130,   170,   174,   170,
     144,     3,   175,   174,   168,  1130,    31,     9,  1130,   168,
    1130,  1337,    79,    38,  1917,   168,  1130,   193,   167,  1532,
    1533,   894,   168,    79,   168,   898,   801,   168,   357,   999,
    1003,    33,   176,   906,    36,   136,   137,  1010,    40,   183,
    1130,   941,   168,   520,   436,   941,   173,   174,    13,    14,
      15,    16,    17,  1558,   168,   127,   170,  1130,   172,   932,
    1624,   934,   129,   170,  1165,   941,   185,   174,   941,   945,
     943,   180,   170,   129,  1305,   100,   174,   144,  1051,   952,
      13,    14,    15,    16,    17,    31,   178,   941,   144,   145,
    1510,   945,    38,   142,    79,   941,    98,    99,  1148,   190,
     168,   997,   170,  1558,   172,   941,   170,   436,  1558,   176,
     174,   941,   114,   168,    79,   170,   183,   172,  1558,   171,
    1662,   801,   138,   139,   126,    79,   603,   168,   843,  1225,
     170,   172,  1326,    13,    14,    15,    16,    17,  1334,  1012,
     174,  1145,   176,  1707,   129,   168,   171,   170,   170,   172,
     670,  1024,   174,  1026,   100,  1028,   170,  1656,   172,   144,
    1028,   174,    53,    54,   129,    56,   941,  1869,  1870,  1871,
     690,    62,   188,   189,   176,   129,   167,   179,   698,   144,
     145,    79,   167,  1132,  1133,    79,    84,    85,   134,  1874,
     144,  1876,   192,  1258,  1259,  1068,  1069,  1130,  1130,    79,
     173,   174,  1130,  1076,  1513,  1747,  1079,   209,   176,  1179,
    1180,  1181,   145,   168,   168,  1328,    79,  1330,   190,   934,
     168,   170,   176,  1324,  1316,   174,  1196,  1197,   943,   170,
     127,   129,  1328,   174,  1330,   129,   170,   952,   170,    79,
    1244,  1805,    79,   245,  1117,   170,   144,  2150,  2151,   129,
     144,   253,    13,    14,    15,    16,    17,  1130,   170,  1132,
    1133,   941,  1130,   170,   144,  1133,   129,   157,   158,  1145,
    1243,   161,   162,   167,   179,   180,   168,  1637,  1842,  1843,
     172,   144,   170,   285,   173,  1126,  1159,   173,   290,   129,
     145,  1145,   129,   145,   184,   170,  1132,  1133,   168,   174,
    1166,  1174,   142,  1144,   144,   168,  1147,   144,   168,   172,
    1151,   168,   314,   115,   116,   172,   318,    13,    14,    15,
      16,    17,    22,   168,  1192,   174,  1199,   172,   168,   170,
     168,   168,   172,   174,   854,   172,  1232,  1210,   170,   179,
     180,   366,   174,   345,   168,   168,   168,   293,  1847,   172,
     352,   743,   744,  1068,   170,    79,  1232,   170,   174,  1232,
    1233,   174,    13,    14,    15,    16,    17,   170,  1244,  1242,
     395,   174,   170,   850,   170,   171,   174,   769,  1232,  1383,
     170,   142,   407,  1947,   174,   387,  1232,   168,  1952,    65,
    1244,  1264,    68,    69,   396,    71,   398,  1591,   174,   142,
    1560,  1561,  1232,   107,   170,   129,   408,   168,   174,   434,
    1468,   172,   174,   175,   743,   744,  1560,  1561,   179,   180,
     144,    79,   176,   425,   426,   168,   428,   173,   174,   172,
     432,    13,  1533,    95,  1307,   170,   179,   180,   168,   174,
     769,  1314,   444,  1316,   168,   176,   142,     3,   172,   192,
     118,  1516,   170,   173,   174,   401,   174,  1232,    13,    14,
      15,    16,    17,   131,   115,   116,   134,  1559,  1560,  1561,
     138,   129,   168,  1443,   193,   170,   172,   997,   142,   174,
     170,   170,  1380,   179,   180,   174,   144,   173,   174,   170,
     492,   437,   494,   174,  1498,   497,  1362,   170,   170,   501,
     170,   174,   504,   174,   168,    97,    98,  1383,   172,    79,
     168,   174,    94,  2184,   172,   179,   180,  2188,  1233,   173,
     171,  1525,    79,   176,    13,    14,    15,    16,    17,  1383,
      77,   113,   142,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,  1416,   546,  1386,   192,  1443,   176,  1264,
    1423,   176,  1232,   173,   174,  1428,  1429,  1430,   168,   129,
     562,   563,   172,   168,  1437,   567,   170,  1443,   176,   179,
     180,   192,   129,  1446,   144,  1644,  1449,  1450,  1451,  1648,
     582,  1454,   173,   174,  1457,   170,   168,   144,   590,  1443,
      79,   593,  1705,   532,   533,   534,   535,  1443,   168,   173,
     174,  1442,   172,   170,   272,   170,  2026,   173,   174,  1705,
     170,   168,    18,  1443,   168,   172,   174,   175,   620,   621,
    1461,  1494,  1498,   172,  1497,  1498,   170,  1468,   142,   575,
    1503,  1605,  1606,  1607,   170,  1508,   170,  1510,   173,   174,
     129,   173,   174,  1497,  1498,  1165,   168,  1644,   172,  1525,
    1741,  1648,  1172,   142,   168,   144,   324,   175,   172,   192,
     328,  1497,  1498,   173,   174,   179,   180,   175,  1443,   174,
    1609,  1525,   674,   173,   174,  2035,   168,   623,   680,   168,
     173,   174,  1881,   172,   170,  1558,  1559,  1560,  1561,   357,
     179,   180,    79,  1561,   192,  1881,     3,   173,   174,  1176,
    1600,  1599,    84,    85,  1600,  1709,    13,    14,    15,    16,
      17,  1881,   367,  1889,  1890,  1891,   170,   385,   174,   175,
    1211,  1212,  1437,  1498,  1600,   170,   728,  1600,   170,   675,
     170,   428,   170,   401,  1449,  1450,  1451,   528,   529,  1454,
    1132,  1133,   129,   530,   531,   170,  1600,  1881,  1644,   170,
     170,  1825,  1648,  1649,  1600,   192,  1881,   144,  1631,  1881,
    1633,  1881,   176,  1443,  1600,   536,   537,  1881,  1644,  1665,
    1600,  1970,  1648,  1649,   168,  1866,   801,    79,  1685,  1686,
    1784,   168,   784,  1656,  1970,   172,  1890,  1891,  1503,  1665,
    1644,  1881,   176,  1508,  1648,  1649,   251,  1638,  1639,   176,
    1970,   170,   174,  1132,  1133,  1678,  1679,  1756,  1881,   173,
     193,  1665,    79,  1752,  1644,  1688,   173,  1875,  1648,  1649,
     170,   170,   174,   170,   479,  1600,   174,   129,   170,   174,
    1703,   170,   287,  1709,  1688,  1665,  1970,   170,   170,   170,
    1740,   843,   144,   170,  1740,  1970,  1687,   170,  1970,   173,
    1970,   173,  1688,   173,   170,  1709,  1970,   167,  1828,    79,
    1934,   170,   129,    77,  1740,   170,   168,  1740,    79,  1644,
     172,   168,  1985,  1648,  1649,   173,    85,   144,   824,    18,
    1970,   176,   183,  1756,   173,   193,  1740,  1760,   173,  1985,
    1665,   837,   173,   170,  1740,   157,   158,  1970,   170,   161,
     162,   168,   170,   173,  1740,   172,   170,   170,  1784,   129,
    1740,   170,   174,  2112,  1787,   170,   941,   170,   129,   173,
    1600,  1794,   184,   174,   144,  1874,  2112,  1876,   167,  1783,
    1784,   193,   934,   144,  1883,   170,  1875,   605,   606,   170,
     170,   943,  2112,   170,   170,  1295,   170,   170,   168,   170,
     952,   170,   172,   170,  1827,    22,  1306,   168,  2032,   174,
     615,   172,   170,   618,  1644,  1740,   439,   170,  1648,  1649,
     170,   170,   170,   170,  1847,  1325,  1817,   170,  2112,    13,
      14,    15,    16,    17,   439,  1665,   170,  2112,  1703,  1339,
    2112,  1468,  2112,  1932,   176,   941,   167,   470,  2112,   174,
     170,  1874,    79,  1876,   192,   174,   174,   170,  1881,  1784,
    1883,   176,   176,   170,  2110,   470,   170,   170,   174,  1892,
      18,  1917,  2112,   173,  1973,  1974,   170,  1900,  1901,   170,
     170,  2144,   167,  2146,  1907,   173,   173,   170,    79,  2112,
     176,  1917,   176,  1916,   176,  1760,    79,  1397,  2144,  1903,
    2146,   175,   129,  1926,  2027,  1928,   175,  1003,   131,   167,
    1740,   168,   759,  1917,  2034,   168,  1068,   144,  1941,   168,
    1943,  1944,  1945,  2186,   168,   743,   744,   168,    79,   168,
     168,   736,   168,    14,   174,  2024,   167,  1917,   129,  1987,
    2186,   168,   175,   175,    79,   172,   129,  1970,   193,   192,
    1973,  1974,  1970,   144,   176,  1051,  1974,   176,   142,   170,
    2184,   144,   170,   170,  2188,  2189,  1989,  2066,   573,   173,
    1993,   173,  2020,   170,   170,  1998,   168,   168,   129,   173,
     170,   172,   170,   170,   168,   168,   167,   108,   172,   172,
     170,  1618,  1917,   144,   129,   179,   180,   118,  2261,  2223,
     167,   170,   170,  2026,   167,  2028,   168,  1159,     0,   144,
      87,   193,    98,     5,  2113,  2261,   193,   168,   193,   193,
     193,   172,  1174,   193,  2248,   168,   873,  1892,  2252,  2112,
    2112,   168,   193,   168,  2112,  1535,  1536,   172,    96,   167,
     176,   167,  2266,  2066,  2067,   174,   170,   670,    40,   167,
     170,   170,  1552,   170,  1554,   170,  2079,  1232,   167,   682,
    2083,   173,   173,   170,   170,   670,   671,   690,   873,   874,
    2093,   170,   170,  1573,   170,   698,  2099,   193,   170,   170,
    1232,  1233,   167,  2182,  2183,   690,   174,  1917,   168,  2112,
    2113,   170,   168,   698,   168,   157,   158,   175,    90,   161,
     162,   173,   173,    82,  2150,  2151,    13,    14,    15,    16,
      17,    18,  1264,    82,    13,    14,    15,    16,    17,   167,
     193,   168,   184,   193,  2150,  2151,   193,  2113,   170,   168,
    2153,   193,   167,   167,   193,    82,  1232,   984,    82,   184,
     193,   988,    82,   135,   184,   175,  2150,  2151,   140,   193,
     271,   272,    18,   287,   146,   193,   193,   167,   150,  2182,
    2183,   167,   167,   172,   156,  2183,   287,   169,   184,  2192,
    2150,  2151,   184,   167,   979,   170,   168,   982,   170,   313,
      79,   113,   175,   317,   168,  1032,   174,   170,   170,   170,
    2181,   169,  1039,   170,   184,   184,  2182,  2183,    64,    65,
      66,    67,    68,    69,    70,    71,  2229,     5,   120,   121,
     122,   123,   124,  2236,  1756,    13,    14,    15,    16,    17,
     170,  1721,  1722,   193,   173,  2150,  2151,    82,   167,  2220,
     129,   854,   169,  2256,   167,   356,   357,   175,   131,   170,
     133,   134,   135,   142,  2267,   144,   168,   193,   193,   854,
     170,   170,   193,  2276,    13,    14,    15,    16,    17,    18,
     252,   382,  1618,   538,   385,   474,   539,   545,  1443,   168,
     540,   542,  1230,   172,   541,   168,  1463,  1756,   171,   172,
     179,   180,   216,   176,   177,  1437,  2238,  1648,  1135,  1094,
    1925,   283,  1665,  2178,  1446,  2146,  1101,  1449,  1450,  1451,
    1105,  1917,  1454,  2127,  1109,  1457,  2001,   428,  1832,   301,
    2109,  2024,  1818,   305,  1818,  2189,   939,  2252,  2108,   440,
    2150,  2151,  1609,  1233,  1609,  1609,    91,   448,   147,  2066,
     391,  2199,  1874,   454,  1876,   327,  1752,  1845,  1521,  1494,
    1076,  1883,  1494,   952,   719,  1691,    36,  1443,   873,   873,
     873,  1503,   473,    -1,    -1,  1595,  1508,    -1,   350,   351,
      13,    14,    15,    16,    17,    18,    -1,    -1,   360,   992,
      -1,   363,   364,    -1,   997,   367,    -1,    -1,   370,   371,
      -1,   373,    -1,   375,    -1,  1190,    -1,  1192,  1011,    -1,
      -1,    -1,   997,    -1,   999,  1874,   388,  1876,    -1,    -1,
      -1,  1497,  2267,    -1,  1883,    13,    14,    15,    16,    17,
      18,  2276,   404,    -1,    -1,    -1,    -1,   409,    -1,   411,
      -1,    -1,    -1,    -1,   103,  1600,  1273,    -1,    -1,   421,
    1277,  1973,  1974,    -1,    -1,    -1,    -1,    -1,    -1,   573,
     432,    -1,    -1,  1066,    -1,    -1,    -1,    85,    -1,    -1,
      -1,    -1,   573,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     139,    -1,    -1,  1086,    -1,   144,    -1,    -1,   147,  1644,
     149,    -1,    -1,  1648,  1649,   113,    -1,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   621,    -1,    -1,
    1665,    -1,    -1,  1298,  1973,  1974,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1350,  1600,   639,    -1,  1354,    -1,    -1,
      -1,  1358,     3,   192,    -1,     4,     5,     6,     7,     8,
       9,    10,    11,    12,  2066,   646,  1678,  1679,    -1,   650,
      -1,    -1,    -1,    -1,    -1,    -1,  1341,   671,    -1,  1344,
      -1,    -1,   118,  1348,    -1,  1168,    -1,    -1,  1644,    -1,
      -1,  1703,  1648,  1649,    -1,   193,    -1,    -1,   692,    -1,
    1165,    -1,    -1,    -1,    -1,  1740,    -1,    -1,    -1,  1665,
      -1,  2113,    -1,    -1,    -1,    -1,    -1,    13,    14,    15,
      16,    17,    -1,    72,    -1,    -1,   578,  2066,    -1,    -1,
      -1,    -1,  1688,    -1,   586,    -1,    -1,   589,    -1,    -1,
      -1,    -1,   723,    -1,  1409,    -1,    -1,    -1,  1760,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   295,   296,    -1,   298,
      -1,   300,   113,   615,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,  2113,  1787,    -1,    -1,   759,    -1,
    2182,  2183,  1794,    79,  1740,    -1,    -1,   768,   769,    -1,
      -1,   142,    -1,    -1,    -1,    -1,   777,   778,   650,   651,
      -1,    -1,   654,   655,    -1,   657,    -1,   659,   660,    -1,
    1293,  1294,   664,   665,    -1,  1827,    -1,   168,   169,  1497,
    1498,    -1,    -1,    -1,   175,    -1,    -1,  1783,   179,   180,
      -1,    -1,  1539,   129,    31,   271,  1543,    -1,    -1,  1546,
     191,    38,    -1,  2182,  2183,    -1,   142,    -1,   144,    -1,
     389,   287,    -1,    -1,  1337,   113,    -1,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,   428,  1576,
      -1,    -1,   168,  1580,    -1,    -1,   172,  1584,    -1,   103,
    1892,    -1,  1917,   179,   180,    -1,    -1,    -1,  1900,  1901,
     202,    -1,   873,   874,    -1,  1907,   748,    -1,    -1,    -1,
      -1,    -1,    -1,   100,  1916,    -1,    -1,    -1,    -1,    -1,
     134,    -1,    -1,  1620,  1926,    -1,  1928,    -1,    -1,    -1,
     772,   118,   146,   147,    -1,    -1,    -1,    -1,    -1,  1941,
      -1,  1943,  1944,  1945,   113,   193,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,    -1,  1903,    -1,   385,
      -1,   945,    -1,   157,   158,   159,   160,   161,   162,   163,
     164,  1917,   956,    -1,   816,   817,   818,    -1,   192,    -1,
     174,    -1,    -1,    -1,   171,    -1,   957,  1989,   175,    -1,
     184,  1993,    -1,    -1,    -1,    -1,  1998,    -1,   969,   193,
     340,    -1,    -1,    -1,   975,    -1,    -1,    -1,    -1,    -1,
     994,    -1,    -1,   984,   440,   999,    -1,   988,    -1,    -1,
    1688,    -1,   448,    -1,   193,    -1,  2028,    -1,   454,    -1,
      -1,    -1,   874,    -1,    -1,   877,   878,    -1,    -1,    -1,
      -1,  1012,    -1,    -1,   113,    -1,   115,   473,   117,   118,
     119,   120,   121,   122,   123,   124,   243,    -1,    -1,  1532,
      -1,  1032,    -1,    -1,    -1,  2067,  1050,    -1,  1039,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  2079,   113,   293,
      -1,  2083,   117,   118,   119,   120,   121,   122,   123,   124,
      -1,  2093,    -1,   935,   520,   113,   646,  2099,   285,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,    -1,
     324,    -1,    -1,    -1,    -1,    -1,    -1,   959,    -1,    -1,
     962,    -1,   964,    -1,    -1,  2150,  2151,    -1,    -1,    -1,
      -1,    -1,    -1,   168,   169,    -1,    -1,   979,    -1,    -1,
      -1,   328,    -1,    -1,   331,    -1,   448,   573,    -1,   451,
      -1,  2153,    -1,    -1,   172,    -1,   191,    -1,    -1,    -1,
      -1,  1145,   464,    -1,  1135,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,   603,    -1,   366,
     709,    -1,  1024,    -1,    -1,    -1,   488,   401,    -1,    -1,
    2192,    -1,    -1,    -1,    -1,    -1,   383,    -1,   385,    -1,
      -1,    -1,    -1,    -1,  2150,  2151,   113,    -1,   395,   759,
     117,   118,   119,   120,   121,   122,   123,   124,   520,    -1,
     407,    -1,   129,   437,    -1,    31,    -1,  2229,   442,    -1,
      -1,    -1,    38,    -1,  2236,   142,    -1,   144,    -1,    -1,
      -1,   543,    -1,    -1,    -1,   671,    -1,   434,    -1,    -1,
      -1,    -1,    -1,   440,  2256,    -1,    -1,    -1,    -1,  1101,
    1244,   168,   169,  1105,    -1,  2267,   480,  1109,    -1,   798,
      -1,    -1,   179,   180,  2276,    -1,    -1,  1248,    -1,    -1,
     830,    -1,    -1,    -1,   191,    -1,   473,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   100,    -1,    -1,   723,    -1,    -1,
      -1,    -1,  1273,    -1,    -1,   111,  1277,    -1,    -1,  1964,
      -1,    -1,   118,    -1,    -1,  1157,    -1,    -1,    -1,    -1,
      13,    -1,    -1,   873,    -1,    -1,    -1,    -1,   134,    -1,
      -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     554,    -1,    -1,    -1,    -1,  1316,    -1,    -1,    -1,   113,
      -1,   777,   778,   117,   118,   119,   120,   121,   122,   123,
     124,   575,    -1,    -1,    -1,   171,    -1,    -1,    -1,   175,
     710,  2026,   712,    -1,    -1,    -1,    -1,    -1,    -1,  1350,
      -1,    -1,    -1,  1354,    -1,    -1,    -1,  1358,  1230,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   202,  2052,  2053,  1383,
      -1,    94,    -1,    -1,    -1,   169,    -1,   957,   172,   623,
      -1,    -1,  1254,    -1,    -1,   629,   842,    -1,    -1,    -1,
     113,   635,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,   984,    -1,    -1,   243,   988,    -1,
      -1,    -1,    -1,   630,   250,   251,    -1,   253,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1298,  2112,  2113,  1301,
    1302,   675,  1012,    -1,    -1,   271,    -1,  1309,  1310,    -1,
      -1,    -1,   686,    -1,    -1,   777,   778,    -1,   284,   285,
    2135,   287,  1032,    -1,    -1,    -1,    -1,   293,   294,  1039,
     704,    -1,    -1,    -1,  1336,   709,  1338,    -1,    -1,  1341,
      -1,    -1,  1344,    -1,    -1,    -1,  1348,   313,    -1,    -1,
     697,   317,    -1,    -1,    -1,   321,    -1,    -1,   324,    -1,
      -1,    -1,   328,    -1,    -1,   331,    -1,  2182,    -1,    -1,
      -1,    -1,    -1,  2230,    -1,    -1,   750,  1379,    -1,    -1,
    2195,  1525,  1513,   969,  2199,    -1,    -1,    -1,    -1,   975,
    1392,    -1,    -1,    -1,   894,    -1,    -1,    -1,   898,    -1,
     366,    -1,    -1,    -1,    -1,    -1,   906,  1409,  1539,    -1,
      -1,    -1,  1543,   999,    -1,  1546,    -1,   383,   384,   385,
      -1,    -1,    -1,    -1,    -1,  1135,    -1,    -1,    -1,   395,
      -1,    -1,   932,    -1,   808,   401,    -1,    -1,    -1,    -1,
      -1,   407,    -1,    -1,    -1,  1576,    -1,    -1,    -1,  1580,
     824,    -1,    -1,  1584,   801,    -1,    -1,    -1,    -1,   425,
      -1,    -1,   428,   837,    -1,    -1,    -1,    -1,   434,    -1,
      64,   437,   438,   439,   440,    -1,   442,    -1,    72,    73,
      74,    75,   448,    -1,   831,   451,    -1,   453,   454,  1620,
      -1,    -1,   108,    -1,   841,   842,    -1,    -1,   464,    -1,
      -1,   467,   468,   469,   470,    -1,    -1,   473,    -1,    -1,
      -1,    -1,    -1,    -1,   480,   131,   863,    -1,    -1,   113,
      18,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,  1662,    -1,    -1,    -1,   113,  1538,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
      -1,   113,    -1,    -1,   520,   117,   118,   119,   120,   121,
     122,   123,   124,  1273,   142,  1709,    -1,  1277,    -1,  1069,
      68,    69,    70,    71,    -1,    -1,  1076,    -1,   172,  1079,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   554,    -1,
     168,   169,    -1,    -1,   941,    -1,   190,   175,    -1,    -1,
      -1,   179,   180,    -1,    -1,    -1,   572,   573,    -1,   575,
      -1,    -1,    -1,   191,    -1,   113,  1747,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,    -1,  1003,
      -1,    -1,   598,    -1,    -1,    -1,  1010,   603,    -1,    -1,
    1350,    -1,    -1,    -1,  1354,    -1,    -1,    -1,  1358,    -1,
      -1,    -1,    -1,    -1,   620,   621,    -1,   623,   624,    -1,
      -1,    -1,  1248,   629,   630,    -1,    -1,   633,    -1,   635,
      -1,    -1,  1674,   639,   172,   291,    -1,  1051,    -1,    -1,
     646,    -1,    -1,    -1,  1394,    -1,    -1,    -1,    -1,    -1,
      13,    14,    15,    16,    17,    -1,    -1,    -1,    -1,  1199,
      -1,    -1,  1704,    -1,   670,   671,    -1,    -1,    -1,   675,
    1210,    -1,    -1,    -1,   680,    -1,   682,    -1,   684,    -1,
     686,    -1,    -1,    -1,   690,    -1,   692,  1729,    -1,    -1,
      -1,   697,   698,    -1,    -1,    -1,    -1,    -1,   704,    -1,
     356,   357,  1744,  1745,  1746,    -1,  1748,  1749,    -1,  1751,
    1881,    -1,  1883,    -1,    -1,    -1,    79,   723,    -1,  1761,
     113,   377,   111,    -1,   117,   118,   119,   120,   121,   122,
     123,   124,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   750,    -1,    -1,    -1,  1932,   142,
     113,    -1,    -1,   759,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,
      -1,   777,   778,    -1,    -1,   168,   169,   783,    -1,   142,
     436,   144,    -1,    -1,    -1,    -1,   179,   180,    -1,  1539,
     171,    -1,    -1,  1543,    -1,   801,  1546,    -1,   191,  1841,
      -1,    -1,   183,    -1,    -1,   168,   169,    -1,    -1,  1851,
    1852,    -1,    -1,    -1,    -1,    -1,   179,   180,   824,    -1,
      -1,    -1,    -1,    -1,   830,   831,  1576,    -1,   191,  1243,
    1580,   837,    -1,    -1,  1584,   841,   842,    -1,    -1,    -1,
    2024,    -1,    -1,    -1,   850,  1232,  1888,    -1,   854,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   863,    -1,    -1,
      -1,    -1,    -1,    -1,  1366,    -1,    -1,   873,    -1,    -1,
    1620,    -1,    -1,    -1,    -1,    -1,  1416,    -1,    -1,    -1,
      -1,    -1,    -1,  1423,    -1,    -1,    -1,  1513,  1428,  1429,
    1430,    -1,    -1,    -1,    -1,   284,    -1,   553,   287,    -1,
      -1,    -1,    -1,    -1,    -1,   294,    -1,    -1,    -1,    -1,
      -1,  1953,    -1,    -1,    -1,    -1,    -1,  1959,  1960,  1961,
      -1,    -1,    -1,    -1,   313,    -1,    -1,    -1,   317,    -1,
      -1,    -1,   321,   939,  1976,   941,    -1,    -1,    -1,   945,
      -1,  2112,  2113,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     956,   957,    -1,    -1,    -1,    -1,  1370,    -1,    -1,    -1,
      -1,    -1,    -1,   969,    -1,    -1,  1468,    -1,    -1,   975,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   984,    -1,
      -1,    -1,   988,    -1,    -1,    -1,   992,    -1,   994,    -1,
     646,   997,    -1,   999,   650,   384,    -1,  1003,    -1,    -1,
      -1,    -1,  2044,  2045,  1010,  1011,  1012,    -1,    -1,    -1,
      -1,  2053,    -1,    -1,    -1,    -1,    -1,  2059,  2060,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1032,    -1,    -1,    -1,
      -1,    -1,    -1,  1039,  2076,    -1,  1662,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1050,  1051,    -1,    -1,    -1,    -1,
     439,    -1,    -1,    -1,    -1,    -1,  1443,    -1,    -1,  2230,
    1066,    -1,    -1,    -1,    -1,    -1,  2108,    -1,    -1,    -1,
      -1,  2113,    -1,   729,    -1,   731,    -1,    -1,    -1,    -1,
    1086,   470,    -1,  1497,    -1,    -1,  2128,   743,   744,    -1,
      -1,  1631,    -1,  1633,    -1,   113,  2138,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
     111,    -1,   768,   769,   770,    -1,  1656,    -1,    -1,    -1,
      -1,  1747,    -1,    -1,    -1,    -1,   782,    -1,    -1,  1135,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,  1145,
      -1,    -1,  1644,  1645,    -1,    -1,  1648,  1649,    -1,    -1,
      -1,    -1,  1654,  2195,    -1,    -1,  1658,    -1,    -1,  1165,
    2202,  1663,  1168,  1665,    -1,   183,  1172,    -1,    -1,   113,
    1176,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,   572,   573,   129,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  2235,    -1,    -1,  2238,    -1,    -1,    -1,
     144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   598,
      -1,    -1,    -1,  1600,    -1,    -1,  2258,    -1,    -1,    -1,
      -1,    -1,  1609,    -1,   168,   169,  1232,    -1,    -1,    -1,
      -1,    -1,   621,    -1,    -1,   624,    -1,  1243,  1244,    -1,
      -1,    -1,  1248,    -1,   633,    -1,    -1,   191,    -1,    -1,
     639,    -1,    -1,    -1,  1260,    -1,    -1,  1644,    -1,    -1,
      -1,  1648,  1649,    -1,    -1,    -1,    -1,  1273,    -1,    -1,
      -1,  1277,    -1,    -1,  1688,    -1,    -1,  1779,  1665,    -1,
      -1,   670,   671,   284,    -1,    -1,    -1,  1293,  1294,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    79,  1799,  1800,    -1,
      -1,   690,    -1,   692,    -1,    -1,    -1,  1847,    -1,   698,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1324,    -1,
     321,    -1,    -1,    -1,    -1,    -1,    -1,  1829,    -1,    -1,
     113,  1337,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,  1757,  1350,    -1,   129,    -1,  1354,    -1,
      -1,    -1,  1358,  1740,    -1,    -1,  1012,    -1,    -1,   142,
      -1,   144,    -1,  1750,  1370,  1752,    -1,    -1,  1024,  1783,
    1026,    -1,  1028,    -1,    -1,    -1,    -1,  1383,    -1,    -1,
      -1,    -1,    -1,   384,    -1,   168,   169,    -1,  1394,   172,
      -1,    -1,    -1,    -1,   783,    -1,   179,   180,    -1,    -1,
      -1,    -1,    -1,  1905,    -1,    -1,    -1,  1909,   191,  1911,
      -1,    -1,  1914,  1915,    -1,  1917,    -1,    -1,    -1,   113,
    1922,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,    -1,    -1,    31,    -1,  1443,    -1,    -1,
      -1,    -1,    38,   113,    -1,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,
      -1,  1117,  1468,  1469,    -1,   854,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,  1130,    -1,  1132,  1133,    -1,    -1,
    2230,    -1,    -1,    -1,    -1,   179,  1446,    -1,  1875,  1903,
      -1,  1497,  1498,    -1,    -1,    -1,    -1,  1457,   168,   169,
      -1,  2003,   172,    -1,   100,    -1,    -1,  1513,  2010,   179,
     180,    -1,    -1,  2015,  2016,   111,    -1,    -1,    -1,  1525,
      -1,   191,   118,    -1,    -1,    -1,  1532,  1533,    -1,    -1,
    1917,    -1,    -1,  1539,    -1,    -1,  2038,  1543,    -1,    -1,
    1546,    -1,   138,   113,    -1,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   113,   945,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   956,    -1,    -1,
    1576,   572,    -1,    -1,  1580,   171,    -1,    -1,  1584,   175,
      -1,    -1,  2084,    -1,  2086,    79,  1242,  2089,  2090,  2091,
      -1,    -1,    -1,    -1,  1600,  2097,  2098,   598,   168,   169,
      -1,    -1,    -1,  1609,    -1,   994,    -1,    -1,   997,    -1,
     999,    -1,  1618,  2027,  1620,    -1,    -1,    -1,   176,   113,
      -1,   191,    -1,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,   633,    -1,    -1,   129,    -1,    -1,  1644,    -1,
      -1,    -1,  1648,  1649,    -1,    -1,    -1,   243,    -1,    -1,
     144,  1307,    -1,    -1,    -1,   251,  1662,    -1,  1314,  1665,
    1316,  1050,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    -1,  2174,  2175,  2176,    -1,   272,    -1,    -1,    -1,
      -1,   682,  1688,   684,    -1,    -1,    -1,    -1,   284,   285,
      -1,   287,    -1,    -1,    -1,    -1,  1702,    -1,   294,    -1,
      -1,    -1,    -1,  1709,    -1,    -1,    -1,  2209,  2210,  2211,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   313,  1678,  1679,
      -1,   317,    -1,    -1,    -1,   321,    -1,    -1,    -1,    -1,
      -1,    -1,   328,    -1,  1740,   331,    -1,    -1,    -1,    -1,
      -1,  1747,    -1,    -1,  1750,    -1,  1752,    -1,    -1,    -1,
      -1,  1757,    -1,    -1,    -1,    -1,  1145,    -1,    -1,    -1,
      -1,   357,    -1,  2150,  2151,    -1,    -1,    -1,    -1,    -1,
     366,    -1,    -1,    -1,    -1,    -1,    -1,  1783,  1784,    -1,
      -1,   377,   783,    -1,    -1,    -1,   382,   383,   384,   385,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,   395,
      -1,   117,   118,   119,   120,   121,   122,   123,   124,   125,
      -1,   407,    -1,    -1,   130,   411,   132,    -1,    -1,    -1,
     416,    -1,    -1,    -1,    -1,   421,    -1,  1787,    -1,   830,
      -1,    -1,   428,    -1,  1794,    -1,    -1,    -1,   434,    -1,
     113,    -1,   438,   439,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   169,  1510,  1244,   172,   130,    -1,   132,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1827,    -1,  1875,
      -1,    -1,    -1,    -1,   470,    -1,    -1,   113,    -1,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   169,  1903,    -1,   172,
      -1,    -1,  1558,  1559,  1560,  1561,   142,    -1,  1914,    -1,
     111,  1917,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,  1932,    -1,    -1,    -1,
      -1,    -1,   168,   169,    -1,    -1,    -1,    -1,   939,    -1,
    1900,  1901,    -1,   179,   180,    -1,    -1,  1907,    -1,    -1,
      -1,    31,    -1,    -1,    -1,   191,  1916,    -1,    38,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1926,   168,  1928,    -1,
     171,   172,    -1,    -1,    -1,    -1,   572,   573,    -1,    -1,
      -1,  1941,    -1,  1943,  1944,  1945,    -1,    -1,    -1,    -1,
      -1,   992,    -1,    -1,  1383,    -1,    -1,  2003,  2004,    -1,
      -1,    -1,   598,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1011,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2024,    -1,
     100,  2027,    -1,    -1,    -1,   621,    -1,    -1,   624,  1989,
      -1,   111,    -1,  1993,   630,    -1,    -1,   633,  1998,    -1,
      -1,    -1,    -1,   639,    -1,    -1,    -1,    -1,    -1,    -1,
     646,    -1,    -1,    -1,   650,    -1,    -1,    -1,   138,    -1,
      -1,    -1,    -1,    -1,    -1,  1066,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   670,   671,    -1,    -1,    -1,    -1,
    1469,   677,    -1,    -1,    -1,  1086,   682,  2093,   684,    -1,
      79,   171,    -1,    -1,   690,    -1,   692,    -1,    -1,    -1,
    1756,   697,   698,    -1,    -1,    -1,   113,  2067,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,  2079,
      -1,    -1,    -1,  2083,   113,    -1,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,  1525,    -1,    -1,  2099,
     129,    -1,    -1,    -1,  2150,  2151,    13,    14,    15,    16,
      17,    -1,    -1,   142,    -1,   144,    -1,    -1,    -1,    -1,
      -1,    -1,   169,   759,    -1,   172,    -1,  1168,    -1,    -1,
      -1,   251,    -1,   769,    -1,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,    -1,    -1,    -1,    -1,   783,    -1,    -1,
     179,   180,    -1,  2153,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   191,    -1,   284,   801,    -1,   287,    -1,    -1,
      -1,    -1,    -1,    -1,   294,    -1,    -1,    -1,  1874,    -1,
    1876,    -1,    -1,    -1,  2230,  1881,    -1,  1883,    -1,    -1,
      -1,    -1,  2192,   313,   830,   831,    -1,   317,    -1,    -1,
      -1,   321,    -1,    -1,    -1,   841,   113,    -1,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   854,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   863,    -1,  2229,
      -1,    -1,    -1,    -1,    -1,   142,  2236,   873,   874,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   366,    -1,    -1,    -1,
      -1,    -1,  1293,  1294,    -1,    -1,  2256,    -1,    -1,    -1,
      -1,   168,   169,    -1,   384,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,  1970,   395,    -1,  1973,  1974,    -1,
    1709,    -1,    -1,  1324,   191,    -1,    -1,   407,    -1,    -1,
      -1,   411,    -1,    -1,    -1,    -1,  1337,    -1,    -1,    79,
      -1,    -1,    -1,   939,    -1,   941,    -1,    -1,   428,   945,
      -1,    -1,    -1,    -1,   434,    -1,    -1,    -1,   438,   439,
     956,   957,    -1,   959,    -1,    -1,    -1,    -1,    -1,    -1,
    2026,    -1,    -1,   113,    -1,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,    -1,    -1,    -1,   984,   129,
     470,    -1,   988,  1394,    -1,    -1,   992,    -1,   994,    -1,
      -1,   997,   142,   999,   144,    -1,    -1,    -1,    -1,    -1,
    2066,    -1,    -1,   113,    -1,  1011,  1012,   117,   118,   119,
     120,   121,   122,   123,   124,   125,    -1,  1023,   168,   169,
     130,    -1,   132,    -1,    -1,    -1,  1032,    -1,    -1,   179,
     180,    -1,    -1,  1039,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   191,    -1,    -1,  1050,    -1,  2112,  2113,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1469,   169,
    1066,    -1,   172,    -1,    -1,    -1,    -1,    -1,    -1,   183,
      -1,   185,    -1,    -1,   188,    -1,    -1,    -1,    -1,    -1,
    1086,    -1,   572,   573,    -1,    -1,   200,   201,    -1,    -1,
      -1,   113,    -1,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   217,   218,   219,    -1,    -1,   598,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  2182,  2183,    -1,    -1,
     142,  1532,  1533,    -1,    -1,    -1,    -1,    -1,    -1,  1135,
      -1,   621,    -1,  1932,   624,    -1,    -1,    -1,    -1,  1145,
      -1,    -1,    -1,   633,    -1,    -1,   168,   169,   262,   639,
      13,    14,    15,    16,    17,    -1,   646,   179,   180,  1165,
      -1,    -1,  1168,    -1,    -1,    -1,  1172,    -1,    -1,   191,
      -1,    -1,    -1,   287,    -1,    -1,    -1,    -1,   113,    -1,
     670,   671,   117,   118,   119,   120,   121,   122,   123,   124,
     125,    -1,   682,    -1,   684,   130,    -1,   132,    -1,    -1,
     690,    -1,   692,    -1,    -1,    -1,    -1,    -1,   698,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  2024,  1232,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   169,    -1,    -1,   113,  1244,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
     113,    -1,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,    -1,    -1,    -1,   129,  1273,    -1,   759,
      -1,  1277,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,   144,    -1,    -1,    -1,    -1,    -1,  1293,  1294,    -1,
      -1,    -1,    -1,   783,    -1,    -1,   172,    -1,    -1,  1305,
      -1,    -1,    -1,    -1,    -1,   168,   169,    -1,    -1,   172,
    1316,   801,    -1,    -1,    -1,    -1,   179,   180,  1324,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,    -1,
      -1,  1337,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     830,    -1,    -1,    -1,  1350,    -1,    -1,    -1,  1354,    -1,
      -1,    -1,  1358,    -1,    -1,    -1,    -1,    -1,    -1,   473,
      -1,    -1,    -1,    -1,   854,    -1,    -1,    -1,    13,    14,
      15,    16,    17,    -1,    -1,    -1,    -1,  1383,    -1,    -1,
      -1,    -1,    -1,   873,    -1,    -1,    -1,   113,  1394,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     524,   525,   526,   527,   528,   529,   530,   531,   532,   533,
     534,   535,   536,   537,   538,   539,   540,   541,   542,    -1,
      -1,    -1,    -1,    -1,    79,    -1,    -1,  1443,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   171,    -1,    -1,    -1,   939,
      -1,   941,    -1,    -1,    -1,   945,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1469,    -1,    -1,   956,   957,   113,    -1,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
      -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1498,    -1,   984,    -1,    -1,   142,   988,   144,
      -1,    -1,   992,    -1,   994,    -1,    -1,   997,    -1,   999,
      31,    -1,    -1,    -1,    -1,    -1,    -1,    38,    -1,  1525,
      -1,  1011,  1012,   168,   169,    -1,  1532,  1533,    -1,    -1,
      -1,    -1,    -1,  1539,   179,   180,    -1,  1543,    -1,    -1,
    1546,    -1,  1032,    -1,    -1,    -1,   191,    -1,    -1,  1039,
      -1,    -1,    -1,    -1,    -1,   669,    -1,    -1,    -1,    -1,
    1050,    13,    14,    15,    16,    17,    -1,    -1,    -1,    -1,
    1576,    -1,    -1,    -1,  1580,    -1,  1066,    -1,  1584,   100,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     111,    -1,    -1,    -1,  1600,    -1,  1086,    -1,    -1,    -1,
      -1,    -1,    -1,  1609,    13,    14,    15,    16,    17,    -1,
      -1,    -1,    -1,    -1,  1620,    -1,    -1,   138,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   113,    79,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,  1644,    -1,
      -1,    -1,  1648,  1649,    -1,  1135,    -1,    -1,    -1,    -1,
     171,    -1,    -1,    -1,    -1,  1145,    -1,    -1,    -1,  1665,
      -1,   113,    -1,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,  1165,    -1,   129,  1168,    -1,
      -1,    -1,  1172,    -1,   171,    -1,    -1,    -1,    -1,    -1,
     142,   113,   144,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,  1709,   113,    -1,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   168,   169,    -1,    -1,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,    -1,
     251,    -1,    -1,   142,  1740,    -1,    -1,    -1,    -1,   191,
      -1,    -1,  1232,    -1,  1750,   859,   168,    -1,    -1,    -1,
     864,    -1,    -1,    -1,  1244,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,   284,    -1,    -1,   287,    -1,   882,    -1,
     179,   180,    -1,   294,    -1,    -1,    -1,    -1,  1784,    -1,
      -1,    -1,   191,  1273,    -1,    -1,    -1,  1277,    -1,    -1,
      -1,    -1,   313,    -1,    -1,    -1,   317,    -1,    -1,    -1,
     321,    -1,    -1,  1293,  1294,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   107,   929,    -1,    -1,    -1,    -1,
     113,     1,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,    -1,   113,  1324,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   366,    -1,  1337,   146,   147,
     148,   149,   150,   151,   152,   153,   154,   155,   156,    -1,
    1350,    -1,    -1,   384,  1354,    -1,    -1,   165,  1358,    -1,
      -1,    -1,    -1,    -1,   395,    55,    -1,  1883,    58,    -1,
      60,    61,    -1,    63,    -1,   999,   407,    -1,   168,    -1,
      -1,    -1,    -1,  1383,   192,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,  1394,    -1,    -1,   428,  1914,    -1,
      -1,  1917,    -1,   434,    -1,    -1,    -1,   438,   439,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1932,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,   470,
     130,   131,   132,  1443,   134,   135,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,  1080,   113,    -1,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,  1469,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,
      -1,   171,   172,    -1,    -1,    -1,   142,    -1,   178,   179,
     180,   181,   182,   183,   184,    -1,    -1,    -1,  1498,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2024,    -1,
      -1,    -1,   168,   169,    -1,    -1,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,   180,  1525,    -1,    -1,    -1,    -1,
      -1,    -1,  1532,  1533,    -1,   191,   192,  2053,    -1,  1539,
      -1,   572,   573,  1543,    -1,    -1,  1546,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1179,  1180,  1181,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   598,    -1,    -1,
      -1,    -1,  1196,  1197,    -1,    -1,  1576,    -1,    -1,    -1,
    1580,    -1,    -1,    -1,  1584,    -1,    -1,    -1,    -1,    -1,
     621,    -1,    -1,   624,    -1,    -1,    -1,  2113,    -1,    -1,
    1600,  1225,   633,    -1,    -1,    -1,    -1,    -1,   639,    -1,
      -1,    -1,    -1,    -1,    -1,   646,    -1,    -1,    -1,    -1,
    1620,    -1,    -1,    -1,    -1,    -1,    -1,    13,    14,    15,
      16,    17,    -1,    -1,  2150,  2151,    -1,    -1,    -1,   670,
     671,    -1,    -1,    -1,  1644,    -1,    -1,    -1,  1648,  1649,
      -1,   682,    -1,   684,    -1,    -1,    -1,    -1,    -1,   690,
      -1,   692,    -1,    -1,    -1,  1665,   113,   698,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
      -1,    -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,
      -1,    38,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1328,    -1,  1330,    -1,    -1,  1709,
      -1,    -1,    -1,    -1,  2230,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,   759,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
    1740,    -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   783,   100,    -1,    -1,   142,    -1,   144,    -1,
      -1,    -1,    -1,    -1,   111,    -1,    -1,    -1,    -1,    -1,
     801,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,   169,  1784,    -1,    -1,   134,    -1,    -1,
      -1,   138,    -1,   179,   180,    -1,    -1,    -1,    -1,   830,
      -1,    -1,    -1,    -1,    -1,   191,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1443,
      -1,    -1,    -1,   854,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,   873,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    -1,    -1,    62,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    72,   243,    -1,    -1,    76,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,   939,    -1,
     941,    -1,    -1,    -1,   945,    -1,    -1,  1917,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   956,   957,    -1,    -1,    -1,
     107,    -1,  1932,    -1,    -1,   112,   113,   284,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
      -1,   128,    -1,   984,    -1,    -1,    -1,   988,    -1,    -1,
      -1,   992,    -1,   994,    -1,   142,   997,    -1,   999,    -1,
      -1,    -1,    -1,    -1,   321,    -1,    -1,    -1,    -1,    -1,
    1011,  1012,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,  1032,   179,   180,    -1,    -1,    -1,    -1,  1039,    -1,
      -1,    -1,    -1,    -1,   191,    -1,   193,    -1,    -1,  1050,
      -1,    -1,    -1,    -1,  2024,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1066,    -1,   384,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   401,  1086,    -1,    -1,    -1,    -1,
       5,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,    14,
      15,    16,    17,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1705,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1135,    -1,    -1,    -1,    -1,    -1,
      55,    -1,    -1,    58,  1145,    60,    61,    -1,    63,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1165,    80,    -1,  1168,    -1,    -1,
      -1,  1172,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    2150,  2151,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,  1232,    -1,    -1,  1828,    -1,    -1,   554,    -1,    -1,
      -1,    -1,    -1,  1244,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    31,    -1,   168,    -1,   572,   171,   172,    38,    -1,
    2230,    -1,    -1,   178,   179,   180,   181,   182,   183,   184,
      -1,    -1,  1273,    -1,    -1,    -1,  1277,    -1,    -1,    -1,
      -1,   598,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1293,  1294,    13,    14,    15,    16,    17,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,
      14,    15,    16,    17,    -1,    -1,   633,    -1,    -1,    -1,
     100,    -1,    -1,  1324,    -1,    -1,    -1,    -1,   108,    -1,
      -1,   111,    -1,    -1,    -1,    -1,  1337,    -1,   118,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1350,
      -1,   131,    -1,  1354,   134,    -1,    -1,  1358,   138,    -1,
      79,    -1,    -1,    -1,    -1,   682,    -1,   684,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1383,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1985,    -1,  1394,   113,   175,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,    -1,    -1,    -1,   113,
     129,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,   202,   142,    -1,   144,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
    2034,    -1,  1443,    -1,    -1,    -1,    -1,    -1,    -1,   168,
     169,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,    -1,   243,   168,   169,   783,    -1,  1469,    -1,
      -1,   251,   191,    -1,    -1,   179,   180,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,    -1,    -1,
      -1,   271,   272,   273,    -1,    -1,    -1,  1498,    -1,    -1,
      -1,    -1,    -1,    -1,   284,   285,    -1,   287,    -1,    -1,
      -1,   291,    -1,   293,    -1,    -1,  2110,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1525,    -1,    -1,    -1,    -1,    -1,
      -1,  1532,  1533,   313,    -1,    -1,    -1,   317,  1539,    -1,
      -1,   321,  1543,    -1,   324,  1546,    -1,    -1,   328,    -1,
    2144,   331,  2146,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1576,   356,   357,    -1,  1580,
      -1,    -1,    -1,  1584,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  2186,    -1,    -1,    -1,    -1,   377,    -1,  1600,
      -1,    -1,    -1,    -1,   384,   385,    -1,    -1,    -1,    -1,
      -1,    -1,    13,    14,    15,    16,    17,    -1,    -1,  1620,
      -1,   401,   939,    -1,   941,    -1,    -1,    -1,   945,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1644,    -1,    -1,    -1,  1648,  1649,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   436,    -1,    -1,   439,
     440,    -1,   442,    -1,  1665,    -1,    -1,  2261,   448,    -1,
      -1,   451,    -1,   453,   454,   992,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,   464,    -1,    -1,   467,   468,   469,
     470,    -1,    -1,   473,  1011,    -1,    -1,    -1,    -1,   479,
     480,    13,    14,    15,    16,    17,    -1,    -1,  1709,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,    13,
      14,    15,    16,    17,    -1,    -1,    -1,    -1,    -1,  1740,
     520,   142,    -1,   144,    -1,    -1,    -1,    -1,    -1,  1066,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,  1086,
      -1,    -1,    -1,   553,   554,    -1,    -1,    -1,   179,   180,
      -1,    -1,    -1,  1784,    -1,    -1,    -1,    -1,    -1,    -1,
     191,    -1,   572,   573,   574,   575,    -1,    -1,    -1,    -1,
      -1,   113,    -1,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,    -1,    -1,    -1,   598,    -1,
      -1,    -1,    -1,   603,    -1,   605,   606,    -1,  1145,   113,
     142,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   621,    -1,   623,    -1,    -1,    -1,    -1,    -1,   629,
     630,  1168,    -1,   633,    -1,   635,   168,   169,   142,    -1,
     172,    -1,    -1,    -1,    -1,    -1,   646,   179,   180,    -1,
     650,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,
      -1,    -1,    -1,    -1,   168,   169,    -1,    -1,    -1,    -1,
     670,   671,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,
      -1,    -1,   682,    -1,   684,    -1,   686,   191,    -1,    -1,
     690,    -1,   692,  1914,    -1,  1232,  1917,   697,   698,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1243,  1244,    -1,    -1,
      -1,  1932,    13,    14,    15,    16,    17,    -1,    -1,    -1,
      -1,    -1,    -1,   723,    -1,    -1,    -1,    -1,    -1,   729,
      -1,   731,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   743,   744,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1293,  1294,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   768,   769,
     770,    -1,    -1,    -1,    -1,    -1,    -1,   777,   778,    -1,
      -1,    -1,   782,   783,    -1,    -1,    -1,  1324,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
    1337,    -1,    -1,  2024,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,   113,
      -1,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   142,   842,    -1,    -1,   129,  1383,    -1,    -1,    -1,
     850,    -1,    -1,    -1,   854,    -1,    -1,    -1,   142,    -1,
     144,    -1,    -1,   863,    -1,    -1,    -1,   168,   169,    -1,
      -1,    -1,    -1,   873,   874,    31,    -1,    -1,   179,   180,
      -1,    -1,    38,    -1,   168,   169,    -1,    -1,    -1,    -1,
     191,    -1,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1443,   191,    -1,    -1,
      -1,    -1,    -1,    13,    14,    15,    16,    17,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2150,
    2151,    -1,  1469,    -1,    -1,    -1,    -1,    -1,    -1,   939,
      -1,   941,    -1,    -1,   100,   945,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   956,   957,    -1,   959,
    1497,  1498,   118,    -1,    -1,    -1,    -1,    -1,    -1,   969,
      -1,    -1,    -1,    -1,    -1,   975,    -1,    -1,    -1,    -1,
      -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,  1525,    -1,
      -1,    -1,   992,    79,   994,  1532,  1533,   997,    -1,   999,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2230,
      -1,  1011,  1012,   113,    -1,    -1,    -1,   117,   118,   119,
     120,   121,   122,   123,   124,    -1,  1026,   113,  1028,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,   142,   129,    -1,    -1,   202,    -1,    -1,    -1,
    1050,  1051,    -1,    -1,    -1,    -1,   142,    -1,   144,    -1,
      -1,    -1,    -1,  1600,    -1,    -1,  1066,    -1,   168,   169,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   179,
     180,    -1,   168,   169,    -1,    -1,  1086,   243,    -1,    -1,
      -1,   191,    -1,   179,   180,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   191,    -1,  1644,    -1,    -1,
      -1,  1648,  1649,    -1,    -1,   271,    -1,  1117,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1665,   285,
    1130,    -1,  1132,  1133,    -1,   291,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1145,    -1,    -1,    -1,    -1,
      -1,  1688,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   317,    -1,    -1,    -1,  1165,    -1,    -1,  1168,    -1,
      -1,    -1,  1709,    -1,    -1,    -1,  1176,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1190,    -1,  1192,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,  1740,   360,    58,    -1,    60,    61,    -1,
      63,   367,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,   385,
      -1,    -1,  1232,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1242,  1243,  1244,    -1,  1783,  1784,  1248,    -1,
      -1,    -1,    -1,    -1,    -1,  1255,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      -1,   134,   135,    -1,   440,    -1,    -1,    -1,    -1,   142,
      -1,    -1,   448,  1293,  1294,   451,    -1,   453,   454,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   464,    -1,
      -1,   467,   468,   469,  1314,   168,  1316,   473,   171,   172,
      -1,    -1,    -1,   479,  1324,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,    -1,    -1,    -1,  1337,   191,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   520,    -1,  1903,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1917,    -1,    -1,  1383,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1399,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   574,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1443,    -1,    -1,    -1,   603,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   615,
     616,    -1,   618,   619,    -1,   621,    -1,    -1,  1468,  1469,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     646,    -1,    -1,    -1,   650,   651,    -1,  1497,  1498,    -1,
      -1,   657,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1510,   667,    -1,  1513,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1525,    -1,    -1,    -1,    -1,
      -1,    -1,  1532,  1533,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   697,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1558,    -1,
    1560,  1561,    -1,    -1,    -1,    -1,    -1,   723,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   732,    -1,    -1,    -1,
     736,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1600,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1609,
      -1,    -1,    -1,  2150,  2151,    -1,   772,    -1,  1618,    -1,
      -1,   777,   778,    -1,    -1,    -1,    31,    -1,    -1,    -1,
      -1,    -1,    -1,    38,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1644,    -1,    -1,    -1,  1648,  1649,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1662,    -1,    -1,  1665,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   842,    -1,  1688,    -1,
      -1,    -1,    -1,    -1,   850,   100,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   863,    -1,  1709,
      -1,    -1,    -1,   118,    -1,    -1,    -1,   873,   874,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   138,    -1,    -1,    -1,    -1,    -1,    -1,
    1740,    -1,   147,    -1,    -1,    -1,    -1,  1747,    -1,    -1,
      -1,    -1,  1752,    -1,    -1,    -1,  1756,  1757,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,    -1,    -1,    -1,
     175,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1783,  1784,   941,    -1,    -1,    -1,   945,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   202,    -1,    -1,
     956,   957,    -1,   959,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   969,    -1,    -1,    -1,    -1,    -1,   975,
      -1,    -1,    -1,   979,   980,    -1,   982,   983,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   243,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1012,    -1,  1014,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   271,    -1,    -1,    -1,
    1026,    -1,    -1,    -1,  1874,  1875,  1876,    -1,    -1,    -1,
     285,  1881,    -1,  1883,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1050,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1903,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    31,    -1,    -1,  1917,    -1,    -1,
      -1,    38,    -1,   328,    -1,    -1,   331,    -1,    -1,    -1,
      -1,    -1,  1932,    -1,  1090,    -1,    -1,    -1,  1094,    -1,
      -1,    -1,    -1,    -1,    -1,  1101,  1102,    -1,    -1,  1105,
    1106,    -1,    -1,  1109,  1110,    -1,    -1,    -1,    -1,    -1,
      -1,   366,    -1,  1119,  1964,    -1,    -1,    -1,    -1,    -1,
    1970,    -1,    -1,  1973,  1974,  1131,    -1,    -1,   383,    -1,
     385,    -1,    -1,   100,    -1,    -1,   391,    -1,    -1,  1145,
     395,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   118,   407,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1176,   138,    -1,    -1,  2024,    -1,  2026,  2027,    -1,   434,
      -1,    -1,    -1,   438,  1190,   440,  1192,    -1,    -1,    -1,
      -1,    -1,    -1,   448,    -1,    -1,   451,    -1,   453,   454,
      -1,    -1,  2052,  2053,   171,    -1,    -1,    -1,   175,   464,
      -1,    -1,   467,   468,   469,    -1,  2066,    -1,   473,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1232,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   202,    -1,    -1,  1244,    -1,
      -1,    -1,  1248,    -1,    -1,    -1,    -1,    -1,    -1,  1255,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  2112,  2113,    -1,   520,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   243,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  2135,    -1,    -1,    -1,    -1,
      -1,    -1,  1298,  1299,    -1,  1301,  1302,  1303,    -1,    -1,
    2150,  2151,    -1,    -1,   271,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   285,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  2182,  2183,    -1,  1341,  1342,    -1,  1344,  1345,
      -1,    -1,  1348,  1349,    -1,    -1,    -1,    -1,   603,  2199,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   328,    -1,    -1,   331,    -1,    -1,  1373,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   630,    -1,  1383,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1399,    -1,    -1,    -1,    -1,    -1,   366,
      -1,    -1,    -1,  1409,  1410,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   383,    -1,   385,    -1,
      -1,    -1,    -1,    -1,   391,    -1,    -1,    -1,   395,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1443,    -1,    -1,
     407,    -1,   697,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1468,    -1,    -1,    -1,    -1,   434,   723,    -1,
      -1,   438,    -1,   440,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   448,    -1,    -1,   451,    -1,   453,   454,    -1,    -1,
      -1,    -1,  1498,    -1,    -1,    -1,    -1,   464,    -1,    -1,
     467,   468,   469,    -1,    -1,    -1,   473,  1513,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1525,
      -1,    -1,   777,   778,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    31,    -1,    -1,    -1,    -1,    -1,    -1,
      38,    -1,    -1,    -1,    -1,    -1,   801,    -1,    -1,    -1,
      -1,    -1,    -1,   520,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   831,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   841,   842,    -1,    -1,
      -1,    -1,    -1,    -1,  1600,   850,    -1,    -1,    -1,    -1,
      -1,    -1,   100,  1609,    -1,    -1,    -1,    -1,   863,    -1,
      -1,    -1,  1618,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     118,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   603,    -1,  1644,    -1,
     138,    -1,  1648,  1649,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1662,    -1,    -1,  1665,
      -1,    -1,    -1,   630,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   171,    -1,    -1,    -1,   175,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   941,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1709,   202,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   969,    -1,    -1,    -1,    -1,    -1,
     975,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     697,    -1,    -1,    -1,  1740,    -1,    -1,    -1,    -1,    -1,
      -1,  1747,    -1,    -1,    -1,   243,  1752,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   723,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   271,    -1,    -1,    -1,    -1,  1784,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   285,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     777,   778,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     328,    -1,    -1,   331,   801,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   831,    -1,    -1,    -1,   366,  1875,
      -1,    -1,    -1,    -1,   841,   842,    -1,    -1,    -1,    -1,
      -1,    -1,  1888,   850,    -1,   383,    -1,   385,    -1,    -1,
      -1,    -1,    -1,   391,    -1,    -1,   863,   395,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   407,
      -1,  1917,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1176,    -1,    -1,    -1,    -1,  1932,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   434,    -1,    -1,    -1,
     438,    -1,   440,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     448,    -1,    -1,   451,    -1,   453,   454,    -1,  1964,    -1,
      -1,    -1,    -1,    -1,    -1,  1971,   464,    -1,    -1,   467,
     468,   469,    -1,    -1,   941,   473,    -1,  1232,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1248,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   969,    -1,    -1,    -1,    -1,   103,   975,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2024,    -1,
    2026,    -1,   520,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   134,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  2052,  2053,    -1,    -1,
     146,    -1,   148,    -1,   150,    -1,    -1,    -1,    -1,  2065,
      20,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    -1,    55,   192,    57,    58,    59,
      60,    61,    -1,    63,    -1,   603,  2112,  2113,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  2135,
      -1,    -1,   630,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  2150,  2151,    -1,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
     130,   131,   132,    -1,   134,   135,  2182,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,  1443,  2195,
    2196,    -1,    -1,  2199,    -1,    -1,    -1,   293,    -1,   697,
      -1,    -1,    -1,   299,    -1,   301,    -1,    -1,   168,  1176,
      -1,   171,   172,  1468,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,   723,    -1,    -1,   324,    -1,
     326,   327,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1498,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1513,    -1,
      -1,    -1,    -1,    -1,    -1,  1232,    37,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   777,
     778,  1248,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   801,    -1,   401,    -1,   403,   404,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   831,    -1,    -1,    -1,    -1,    -1,    -1,
     111,   437,    -1,   841,   842,  1600,   442,    -1,    -1,    -1,
      -1,    -1,   850,    -1,  1609,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1618,    -1,   863,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   480,    -1,    -1,    -1,    -1,  1644,
      -1,    -1,    -1,  1648,  1649,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1662,    -1,    -1,
    1665,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   203,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   941,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   554,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   969,    -1,    -1,    -1,    -1,  1443,   975,    -1,   575,
      -1,   577,   578,    -1,    -1,  1740,    -1,    -1,    -1,    -1,
      -1,    -1,  1747,    -1,    -1,  1750,    -1,  1752,    -1,    -1,
     271,  1468,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   605,
      -1,    -1,    -1,   284,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   623,    -1,  1784,
      -1,  1498,    -1,   629,    -1,    -1,    -1,    -1,    -1,   635,
      -1,   637,   638,    -1,    -1,    -1,  1513,    -1,    -1,    -1,
     321,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   340,
      -1,   342,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   675,
     676,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     686,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   704,    -1,
      -1,    -1,    -1,   709,    -1,    -1,    -1,    -1,    -1,    -1,
    1875,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1600,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1609,    -1,    -1,    -1,    -1,   743,    -1,    -1,
      -1,  1618,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1917,    -1,    -1,    -1,    -1,    -1,   439,    -1,
      -1,    -1,    -1,    -1,   770,    -1,    -1,  1644,  1176,    -1,
      -1,  1648,  1649,    -1,   780,    -1,   457,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1662,    -1,    -1,  1665,   470,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   808,    -1,    -1,   811,   812,    -1,    -1,    -1,
     491,    -1,   493,    -1,    -1,    -1,    -1,    -1,   824,   500,
      -1,   502,   503,    -1,  1232,    -1,    -1,    -1,    -1,    -1,
      -1,   837,    -1,    -1,    -1,    -1,   517,    -1,    -1,    -1,
    1248,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1740,   545,    -1,    -1,    -1,    -1,   550,
    1747,    -1,    -1,  1750,    -1,  1752,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   572,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1784,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   598,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   623,   624,    -1,   626,    -1,    -1,    -1,    -1,
      -1,    -1,   633,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   647,    -1,   649,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  2150,  2151,    -1,    -1,   670,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,  1003,  1875,    -1,
      -1,    -1,    -1,    -1,  1010,    -1,    -1,    -1,    -1,   690,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1026,    -1,    -1,    -1,    -1,    -1,   707,    -1,    -1,    -1,
     711,   712,    -1,    -1,    -1,  1443,    -1,   718,    -1,    -1,
    1917,    -1,   723,    -1,    -1,  1051,    -1,    -1,    -1,    -1,
    1056,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1468,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   750,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1498,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   783,    -1,   111,  1513,    -1,    -1,    -1,    -1,
      -1,   118,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,  1132,   134,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    -1,    -1,    57,    -1,    59,   175,    -1,
      -1,    -1,    -1,   854,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1190,    -1,    -1,    -1,    -1,    -1,
     871,    -1,  1600,    -1,    -1,   202,    -1,    -1,    -1,    -1,
      -1,  1609,   883,    -1,    -1,   886,   887,   888,    -1,    -1,
    1618,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,    -1,   115,   116,   906,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1644,  1243,    -1,    -1,
    1648,  1649,    -1,    -1,   251,    -1,    -1,    -1,    -1,  1255,
      -1,    -1,    -1,    -1,  1662,    -1,    -1,  1665,    -1,    -1,
      -1,    -1,    -1,    -1,   271,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  2150,  2151,    -1,    -1,   284,    -1,    -1,
     287,    -1,    -1,   175,    -1,    -1,   293,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   975,    -1,    -1,    -1,    -1,    -1,
    1306,    -1,    -1,    -1,    -1,    -1,   313,    -1,    -1,    -1,
     317,    -1,    -1,    -1,   321,    -1,   997,   324,    -1,    -1,
      -1,    -1,  1003,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,  1740,    -1,    -1,    -1,    -1,    -1,    -1,  1747,
      -1,    -1,  1750,    -1,  1752,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1051,    -1,    -1,    -1,    -1,    -1,  1784,   384,   385,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   401,  1076,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1089,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   439,   440,    -1,   442,    -1,    -1,    -1,    -1,
      -1,   448,    -1,    -1,   451,    -1,   453,   454,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   464,    -1,    -1,
     467,   468,   469,   470,    -1,    -1,   473,  1875,    -1,    -1,
      -1,    -1,    -1,   480,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1497,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1917,
      -1,    -1,    -1,   520,    -1,    -1,    -1,    -1,  1199,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1219,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,  1560,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   572,   573,  1248,   575,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   598,    -1,    -1,    -1,    -1,   603,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   621,    -1,   623,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1306,   633,    -1,   635,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1334,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   670,   671,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   682,    -1,   684,    -1,    -1,
      -1,    -1,  1688,   690,    -1,   692,    -1,  1368,    -1,  1370,
      -1,   698,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   723,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1416,    -1,  1418,    -1,    -1,
      -1,    -1,  2150,  2151,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1757,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    55,    -1,    -1,    58,    -1,    60,    61,    -1,    63,
     777,   778,    -1,    -1,    -1,    -1,   783,  1783,    -1,    -1,
      -1,  1462,  1463,    -1,    78,    -1,    80,    81,  1469,    83,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,   107,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   842,   130,   131,   132,    -1,
     134,   135,    -1,   850,    -1,    -1,    -1,   854,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1550,
    1551,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,    -1,
      -1,    -1,   176,    -1,   178,   179,   180,   181,   182,   183,
     184,    -1,    -1,    -1,    -1,    -1,    -1,  1903,    -1,   193,
      -1,    -1,    -1,    -1,    -1,    13,    14,    15,    16,    17,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   939,    -1,    -1,  1616,    -1,    -1,  1619,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,    -1,   956,
      58,    -1,    60,    61,    -1,    63,  1637,    -1,    -1,    -1,
      -1,    -1,   969,    -1,    -1,    -1,    -1,  1973,   975,    -1,
      -1,    -1,    80,    -1,    -1,  1656,    -1,    -1,    -1,    -1,
      -1,  1662,    -1,    -1,    -1,   992,    -1,   994,    -1,    -1,
     997,    -1,   999,    -1,    -1,    -1,    -1,    -1,  2004,  2005,
      -1,   109,   110,   111,  1011,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,  2027,   130,   131,   132,    -1,   134,   135,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,  1050,  1051,    -1,    -1,    -1,    -1,   157,
     158,    -1,    -1,   161,   162,    -1,    -1,  1738,    -1,  1066,
     168,   169,    -1,   171,   172,    -1,  1747,    -1,    -1,    -1,
     178,   179,   180,   181,   182,   183,   184,    -1,    -1,  1086,
      13,    14,    15,    16,    17,    -1,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,  2135,
       1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1822,    -1,    -1,    -1,    -1,    79,    18,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1165,    -1,
      -1,  1168,    -1,    -1,    -1,    -1,  1847,    -1,    -1,  1176,
      -1,    -1,    -1,    -1,    -1,    -1,  2182,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    55,    -1,    -1,    58,    -1,    60,
      61,    -1,    63,    -1,    -1,    -1,   129,    -1,    -1,    -1,
      -1,  1882,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,
      81,   144,    83,    84,    85,    86,    87,    88,    89,    90,
      91,    92,    93,    94,    95,    96,    97,    98,    99,   100,
     101,   102,   103,   104,   105,    -1,   107,    -1,   109,   110,
     111,  1248,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,   128,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1293,  1294,    -1,     1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,   168,    -1,    -1,
     171,   172,    -1,    -1,    -1,   176,    18,   178,   179,   180,
     181,   182,   183,   184,    -1,    -1,    -1,  1324,    -1,    -1,
      -1,    -1,   193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1337,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    55,    -1,    -1,    58,    -1,    60,    61,
      -1,    63,    -1,    -1,  2035,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    78,    79,    80,    81,
      -1,    83,    84,    85,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
     102,   103,   104,   105,    -1,   107,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,   129,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   167,   168,    -1,    -1,   171,
     172,  1468,  1469,    -1,   176,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1497,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1513,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1532,  1533,    -1,    -1,    -1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    55,    -1,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,  1609,    76,    -1,    -1,    -1,    80,    -1,    -1,
      -1,  1618,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,  1662,    -1,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,  1688,    -1,    -1,   157,   158,    -1,    -1,   161,   162,
      -1,    -1,    -1,    -1,    -1,   168,   169,   170,   171,   172,
      -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    1747,    -1,    -1,    -1,    -1,  1752,    -1,    -1,    -1,    -1,
    1757,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    -1,    -1,  1783,    -1,    -1,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    56,    57,    -1,
      59,    -1,    -1,    62,    -1,    -1,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,  1875,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,  1903,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,  1932,    -1,    -1,    -1,    -1,
      -1,   170,    -1,    -1,    -1,    -1,     1,    -1,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,   193,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    -1,    -1,
      -1,    76,    -1,    78,    79,    80,    81,  2024,    83,    -1,
    2027,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,    -1,   107,    -1,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,   144,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   167,   168,    -1,    -1,   171,   172,    -1,    -1,
      -1,   176,    -1,   178,   179,   180,   181,   182,   183,   184,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     1,   193,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    58,    59,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    -1,
      -1,    -1,    76,    -1,    78,    79,    80,    81,    -1,    83,
      -1,    -1,    86,    87,    88,    89,    90,    91,    92,    93,
      94,    95,    96,    97,    98,    99,   100,   101,   102,   103,
     104,   105,    -1,   107,    -1,   109,   110,   111,   112,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,   128,   129,   130,   131,   132,    -1,
     134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
     144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,    -1,
      -1,    -1,   176,    -1,   178,   179,   180,   181,   182,   183,
     184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   193,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    55,    -1,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    72,
      -1,    -1,    -1,    76,    -1,    78,    -1,    80,    81,    -1,
      83,    -1,    -1,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    97,    98,    99,   100,   101,   102,
     103,   104,   105,    -1,   107,    -1,   109,   110,   111,   112,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,    -1,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,
      -1,    -1,    -1,   176,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     193,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    55,    -1,    57,    58,    59,    60,    61,
      -1,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    76,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,
     112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   157,   158,    -1,    -1,   161,
     162,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    -1,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    -1,    59,    -1,
      -1,    62,    13,    14,    15,    16,    17,    -1,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    -1,    59,    -1,
      -1,    62,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,   172,    -1,    -1,    -1,   176,    -1,    -1,   179,   180,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     191,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    -1,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    53,    54,    -1,    56,    57,    -1,    59,    -1,    -1,
      62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,    -1,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    -1,
      -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,
      64,    65,    66,    67,    68,    69,    70,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,
      -1,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,    -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
     144,   145,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,   169,   170,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   191,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    -1,
      56,    57,    -1,    59,    -1,    -1,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   113,    -1,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,
     176,    -1,    -1,   179,   180,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   191,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      -1,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    56,    57,
      -1,    59,    -1,    -1,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   113,    -1,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,   169,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,   180,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   191,     3,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    55,    -1,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    72,    -1,    -1,    -1,    76,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     109,   110,   111,   112,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,   182,   183,   184,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    55,    -1,
      57,    58,    59,    60,    61,    -1,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,   112,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,   178,   179,   180,   181,   182,   183,   184,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    -1,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    -1,    55,
      -1,    57,    58,    59,    60,    61,    -1,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    -1,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,   169,    -1,   171,   172,   173,   174,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      55,    -1,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,   169,    -1,   171,   172,   173,   174,
      -1,    -1,    -1,   178,   179,   180,   181,   182,   183,   184,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    -1,
      -1,    55,    -1,    57,    58,    59,    60,    61,    -1,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,   130,   131,   132,    -1,
     134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,   173,
     174,    -1,    -1,    -1,   178,   179,   180,   181,   182,   183,
     184,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    55,    -1,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,
     173,   174,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    55,    -1,    57,    58,    59,    60,    61,
      -1,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,
     172,   173,   174,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    55,    -1,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,   172,   173,   174,    -1,    -1,    -1,   178,   179,   180,
     181,   182,   183,   184,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    -1,
      20,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    -1,    55,    -1,    57,    58,    59,
      60,    61,    -1,    63,    64,    65,    66,    67,    68,    69,
      70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
     130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,
      -1,   171,   172,   173,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    55,    -1,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     109,   110,   111,    -1,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,    -1,   171,   172,   173,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,   182,   183,   184,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    -1,    -1,    55,    -1,    57,
      58,    59,    60,    61,    -1,    63,    64,    65,    66,    67,
      68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   109,   110,   111,    -1,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,   130,   131,   132,    -1,   134,   135,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,    -1,    -1,   171,   172,   173,    -1,    -1,    -1,    -1,
     178,   179,   180,   181,   182,   183,   184,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    55,    -1,
      57,    58,    59,    60,    61,    -1,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,    -1,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,    -1,    -1,   171,   172,   173,    -1,    -1,    -1,
      -1,   178,   179,   180,   181,   182,   183,   184,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    -1,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    -1,    55,
      -1,    57,    58,    59,    60,    61,    -1,    63,    64,    65,
      66,    67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    -1,   134,   135,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,    -1,    -1,   171,   172,   173,    -1,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      55,    -1,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,    -1,    -1,   171,   172,   173,    -1,
      -1,    -1,    -1,   178,   179,   180,   181,   182,   183,   184,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    -1,
      -1,    55,    -1,    57,    58,    59,    60,    61,    -1,    63,
      64,    65,    66,    67,    68,    69,    70,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,   130,   131,   132,    -1,
     134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,   183,
     184,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    55,    -1,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    55,    -1,    57,    58,    59,    60,    61,
      -1,    63,    64,    65,    66,    67,    68,    69,    70,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    55,    -1,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,
     181,   182,   183,   184,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    -1,    -1,    62,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    84,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   112,    -1,    -1,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,   144,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,    -1,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,     1,    -1,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    -1,    -1,    62,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    72,    -1,    -1,    -1,    76,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    84,    85,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   112,    -1,    -1,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,   144,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,    -1,     1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    56,    57,    -1,
      59,    -1,    -1,    62,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,   115,   116,    57,    -1,
      59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,    -1,
       1,    -1,   171,   172,    -1,    -1,   115,   116,    -1,    -1,
     179,   180,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    -1,    59,    -1,
      -1,    62,   171,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,   115,   116,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   167,    -1,     1,    -1,
     171,   172,    -1,    -1,   115,   116,    -1,    -1,   179,   180,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,
     171,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   167,    -1,     1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,    -1,   179,   180,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      -1,    56,    57,    -1,    59,    -1,    -1,    62,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   167,    -1,     1,    -1,   171,   172,    -1,    -1,
      -1,    -1,    -1,    -1,   179,   180,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    -1,    -1,    62,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     167,    -1,     1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    53,    54,    -1,    56,    57,    -1,
      59,    -1,    -1,    62,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      -1,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
     191,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,   129,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      -1,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
     191,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,   172,    -1,    -1,    -1,   176,    -1,    -1,   179,   180,
      -1,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
     191,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      -1,    -1,    13,    14,    15,    16,    17,    -1,    -1,    20,
     191,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    53,    54,    -1,    56,    57,    -1,    59,    -1,
      -1,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     191,     3,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,
      -1,    -1,    64,    65,    66,    67,    68,    69,    70,    71,
      72,    -1,    -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    84,    85,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     112,    -1,    -1,   115,   116,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   167,    -1,    -1,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,     3,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    -1,
      -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    -1,
      -1,    -1,    76,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,    -1,
      -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,
       3,    -1,     5,    -1,    -1,   179,   180,    10,    -1,    -1,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   112,
      -1,    -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,    -1,   179,   180,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    -1,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
       3,    56,    57,    -1,    59,    -1,    -1,    62,    -1,    -1,
      13,    14,    15,    16,    17,    -1,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,   116,    -1,    -1,   171,   172,    -1,    -1,
      -1,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,    -1,   179,   180,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    -1,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    -1,    -1,
      -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,    65,
      66,    67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,
     116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,   144,   145,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,    -1,   170,   171,   172,    -1,    -1,    -1,
      -1,    -1,    -1,   179,   180,     4,     5,     6,     7,     8,
       9,    10,    11,    12,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,    -1,
      59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,   144,   145,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,   170,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,     4,     5,     6,     7,     8,     9,    10,    11,
      12,    13,    14,    15,    16,    17,    18,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,
      -1,    -1,    64,    65,    66,    67,    68,    69,    70,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   115,   116,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,   145,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   170,   171,   172,    -1,    -1,
      -1,    -1,    -1,    -1,   179,   180,     4,     5,     6,     7,
       8,     9,    10,    11,    12,    13,    14,    15,    16,    17,
      18,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,
      -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,
      68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   171,   172,   173,    -1,    -1,    -1,    -1,
      -1,   179,   180,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   115,   116,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,   145,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    18,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    -1,
      -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,
      64,    65,    66,    67,    68,    69,    70,    71,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,   145,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,   180,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,
      57,    -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,     4,     5,     6,     7,     8,     9,
      10,    11,    12,    13,    14,    15,    16,    17,    18,    -1,
      20,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,
      -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,    69,
      70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   115,   116,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,
     180,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,    -1,   179,   180,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    -1,    -1,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    53,    54,    -1,
      56,    57,    -1,    59,    -1,    -1,    62,    -1,    -1,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,   115,
     116,    -1,    57,    -1,    59,    18,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    55,    -1,    -1,    58,    -1,    60,    61,    -1,
      63,    -1,    -1,    -1,    -1,   171,   172,    -1,    -1,    -1,
     115,   116,    -1,   179,   180,    78,    -1,    80,    81,    -1,
      83,    84,    85,    86,    87,    88,    89,    90,    91,    92,
      93,    94,    95,    96,    -1,    -1,    99,   100,   101,   102,
     103,   104,   105,    -1,   107,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,   128,   171,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    18,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,
      -1,    -1,    -1,   176,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    55,    -1,    -1,    58,    -1,    60,    61,
     193,    63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    78,    -1,    80,    81,
      -1,    83,    -1,    -1,    86,    87,    88,    89,    90,    91,
      92,    93,    94,    95,    96,    -1,    -1,    99,   100,   101,
     102,   103,   104,   105,    -1,   107,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,   128,    -1,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,
     172,    -1,    -1,    -1,   176,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   193,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    95,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   115,   116,    -1,     3,     4,     5,
       6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
      16,    17,    18,    -1,    20,    -1,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,    44,    45,
      46,    47,    48,    49,    50,    51,    52,    -1,    -1,    -1,
     171,    57,    -1,    59,    -1,   176,    -1,    -1,    64,    65,
      66,    67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    95,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,
     116,    -1,     3,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,   171,    57,    -1,    59,    -1,
     176,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   115,   116,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,    -1,    -1,    -1,    -1,   176,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    55,    -1,
      57,    58,    59,    60,    61,    -1,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    92,    -1,    -1,    -1,    96,
      -1,    98,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,    -1,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,    -1,    -1,   171,   172,    -1,    -1,    -1,   176,
      -1,   178,   179,   180,   181,   182,   183,   184,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      55,    -1,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    92,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,    -1,    -1,   171,   172,    -1,    -1,
      -1,   176,    -1,   178,   179,   180,   181,   182,   183,   184,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    55,    -1,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    79,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,   129,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,   170,   171,   172,
      -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    55,    -1,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,   129,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,   144,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,
     171,   172,    -1,    -1,    -1,   176,    -1,   178,   179,   180,
     181,   182,   183,   184,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    55,    -1,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      79,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     109,   110,   111,    -1,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
     129,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,   144,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,   182,   183,   184,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    55,    -1,
      57,    58,    59,    60,    61,    -1,    63,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   109,   110,   111,    -1,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,   129,   130,   131,   132,    -1,   134,   135,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,   144,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,   178,   179,   180,   181,   182,   183,   184,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      55,    -1,    57,    58,    59,    60,    61,    -1,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,    -1,    -1,   171,   172,    -1,    -1,
      -1,   176,    -1,   178,   179,   180,   181,   182,   183,   184,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    55,    -1,    57,    58,    59,    60,    61,    -1,
      63,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,
      -1,    -1,    -1,   176,    -1,   178,   179,   180,   181,   182,
     183,   184,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    55,    -1,    57,    58,    59,    60,
      61,    -1,    63,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,   130,
     131,   132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,
     181,   182,   183,   184,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    55,    -1,    57,    58,
      59,    60,    61,    -1,    63,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     109,   110,   111,    -1,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,   130,   131,   132,    -1,   134,   135,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,   182,   183,   184,     3,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    -1,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    53,    54,    -1,    56,
      57,    -1,    59,    -1,    -1,    62,    -1,     4,     5,     6,
       7,     8,     9,    10,    11,    12,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,   115,   116,
      57,    -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   171,    -1,   113,    -1,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,    -1,    -1,
      -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,   145,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   170,   171,    13,    14,    15,    16,    17,
      18,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,
      -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,
      68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   113,    -1,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
      -1,   179,   180,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    79,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   115,   116,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   129,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   144,   145,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,
     171,    13,    14,    15,    16,    17,    18,    -1,    20,    -1,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    46,    47,    48,    49,    50,    51,
      52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,
      -1,    -1,    64,    65,    66,    67,    68,    69,    70,    71,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   113,    -1,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   144,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    13,    14,
      15,    16,    17,    18,    -1,    20,   171,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,   144,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,    -1,
      -1,    -1,    -1,    -1,   179,   180,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,
      57,    -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,   144,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,    -1,
      59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,   144,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,   115,   116,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   170,
     171,   172,    -1,    -1,   115,   116,    -1,    -1,   179,   180,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,   115,   116,    57,    -1,    59,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    85,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,
      -1,    -1,   115,   116,    -1,    -1,   179,   180,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,    -1,   179,   180,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
     115,   116,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,    -1,
     115,   116,    -1,    -1,   179,   180,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,    -1,
      -1,    -1,    -1,    -1,   179,   180,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,
      57,    -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    13,    14,    15,    16,
      17,    18,    -1,    20,    -1,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    44,    45,    46,
      47,    48,    49,    50,    51,    52,    -1,    -1,   115,   116,
      57,    -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,
      67,    68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   171,   172,    -1,    -1,   115,   116,
      -1,    -1,   179,   180,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,
      -1,    -1,   179,   180,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,    -1,
      59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    13,    14,    15,    16,    17,    18,
      -1,    20,    -1,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    32,    33,    34,    35,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
      49,    50,    51,    52,    -1,    -1,   115,   116,    57,    -1,
      59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,    68,
      69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   171,   172,    -1,    -1,   115,   116,    -1,    -1,
     179,   180,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,    -1,
     179,   180,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,   115,   116,    57,    -1,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,   172,    -1,    -1,   115,   116,    -1,    -1,   179,   180,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
       4,     5,     6,     7,     8,     9,    10,    11,    12,    13,
      14,    15,    16,    17,    -1,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,    -1,
      -1,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,   115,   116,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    13,    14,    15,    16,    17,    18,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,   115,   116,    57,   171,    59,    -1,
      -1,    -1,    -1,    64,    65,    66,    67,    68,    69,    70,
      71,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,   172,    -1,    -1,   115,   116,    -1,    -1,   179,   180,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     171,   172,    -1,    -1,    -1,    -1,    -1,    -1,   179,   180,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,    -1,    -1,    57,    -1,    59,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      13,    14,    15,    16,    17,    18,    -1,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      -1,    -1,   115,   116,    57,    -1,    59,    -1,    -1,    -1,
      -1,    64,    65,    66,    67,    68,    69,    70,    71,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,
      -1,    -1,   115,   116,    -1,    -1,   179,   180,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,    -1,   179,   180,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
      -1,    -1,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    13,    14,
      15,    16,    17,    18,    -1,    20,    -1,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    -1,    -1,
     115,   116,    57,    -1,    59,    -1,    -1,    -1,    -1,    64,
      65,    66,    67,    68,    69,    70,    71,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,    -1,
     115,   116,    -1,    -1,   179,   180,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   171,    -1,    -1,    13,
      14,    15,    16,    17,   179,   180,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,    13,
      14,    15,    16,    17,    -1,    -1,    20,    -1,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
      34,    35,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,    -1,
      -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   115,   116,    -1,    -1,    -1,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,    -1,   179,   180,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   171,   172,    -1,
      13,    14,    15,    16,    17,   179,   180,    20,    -1,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    32,
      33,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
      53,    54,    -1,    56,    57,    -1,    59,    -1,    -1,    62,
      20,    -1,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    -1,    -1,    55,    -1,    57,    58,    59,
      60,    61,    -1,    63,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   115,   116,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,   171,    -1,
     130,   131,   132,    -1,   134,   135,   179,   180,    -1,    -1,
      -1,    -1,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,
      -1,   171,   172,   173,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,    13,    14,    15,    16,    17,
      18,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,
      -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,
      68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   144,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    13,    14,    15,    16,    17,
      18,    -1,    20,   171,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    -1,    -1,    -1,    -1,    57,
      -1,    59,    -1,    -1,    -1,    -1,    64,    65,    66,    67,
      68,    69,    70,    71,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   115,   116,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   144,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    13,    14,    15,    16,    17,
      -1,    -1,    20,   171,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    56,    57,
      -1,    59,    -1,    -1,    62,    13,    14,    15,    16,    17,
      -1,    -1,    20,    -1,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    32,    33,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    -1,    56,    57,
      -1,    59,    -1,    -1,    62,    -1,    -1,   115,   116,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    79,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    55,
      -1,    -1,    58,    -1,    60,    61,    -1,    63,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   171,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   129,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   109,   110,   111,   144,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    -1,   134,   135,
      -1,    -1,    55,    -1,    -1,    58,   142,    60,    61,    -1,
      63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   157,   158,    -1,    -1,   161,   162,    80,    -1,    -1,
      -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      55,   134,   135,    58,    -1,    60,    61,    -1,    63,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,
      -1,   174,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    55,   134,
     135,    58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,
      -1,   176,    -1,   178,   179,   180,   181,   182,   183,   184,
      -1,    -1,   109,   110,   111,    -1,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,   130,   131,   132,    55,   134,   135,    58,
      -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,   176,
      -1,   178,   179,   180,   181,   182,   183,   184,    -1,    -1,
     109,   110,   111,    -1,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,   130,   131,   132,    55,   134,   135,    58,    -1,    60,
      61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
     169,    -1,   171,   172,   173,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,   182,   183,   184,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,   130,
     131,   132,    55,   134,   135,    58,    -1,    60,    61,    -1,
      63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,
     171,   172,    -1,    -1,    -1,   176,    -1,   178,   179,   180,
     181,   182,   183,   184,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      55,   134,   135,    58,    -1,    60,    61,    -1,    63,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,
      -1,    -1,    -1,   176,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    55,   134,
     135,    58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,
      -1,    -1,    -1,   178,   179,   180,   181,   182,   183,   184,
      -1,    -1,   109,   110,   111,    -1,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,   130,   131,   132,    55,   134,   135,    58,
      -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,    -1,    -1,   171,   172,    -1,    -1,   175,    -1,
      -1,   178,   179,   180,   181,   182,   183,   184,    -1,    -1,
     109,   110,   111,    -1,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,   130,   131,   132,    55,   134,   135,    58,    -1,    60,
      61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,    -1,   171,   172,   173,    -1,    -1,    -1,    -1,   178,
     179,   180,   181,   182,   183,   184,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,   130,
     131,   132,    55,   134,   135,    58,    -1,    60,    61,    -1,
      63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,
     171,   172,   173,    -1,    -1,    -1,    -1,   178,   179,   180,
     181,   182,   183,   184,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      55,   134,   135,    58,    -1,    60,    61,    -1,    63,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    55,   134,
     135,    58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   168,    -1,    -1,   171,   172,   173,    -1,
      -1,    -1,    -1,   178,   179,   180,   181,   182,   183,   184,
      -1,    -1,   109,   110,   111,    -1,   113,   114,   115,   116,
     117,   118,   119,   120,   121,   122,   123,   124,   125,   126,
     127,    -1,    -1,   130,   131,   132,    55,   134,   135,    58,
      -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   168,    -1,    -1,   171,   172,    -1,    -1,    -1,   176,
      -1,   178,   179,   180,   181,   182,   183,   184,    -1,    -1,
     109,   110,   111,    -1,   113,   114,   115,   116,   117,   118,
     119,   120,   121,   122,   123,   124,   125,   126,   127,    -1,
      -1,   130,   131,   132,    55,   134,   135,    58,    -1,    60,
      61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,
      -1,    -1,   171,   172,    -1,    -1,    -1,   176,    -1,   178,
     179,   180,   181,   182,   183,   184,    -1,    -1,   109,   110,
     111,    -1,   113,   114,   115,   116,   117,   118,   119,   120,
     121,   122,   123,   124,   125,   126,   127,    -1,    -1,   130,
     131,   132,    55,   134,   135,    58,    -1,    60,    61,    -1,
      63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,   170,
     171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,
     181,   182,   183,   184,    -1,    -1,   109,   110,   111,    -1,
     113,   114,   115,   116,   117,   118,   119,   120,   121,   122,
     123,   124,   125,   126,   127,    -1,    -1,   130,   131,   132,
      55,   134,   135,    58,    -1,    60,    61,    -1,    63,   142,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,
      -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,
     183,   184,    -1,    -1,   109,   110,   111,    -1,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,    -1,    -1,   130,   131,   132,    -1,   134,
     135,    -1,    -1,    55,    -1,    -1,    58,   142,    60,    61,
      -1,    63,    64,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,
      -1,    -1,    -1,   178,   179,   180,   181,   182,   183,   184,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    55,   134,   135,    58,    -1,    60,    61,    -1,    63,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,   130,   131,   132,    55,
     134,   135,    58,    -1,    60,    61,    -1,    63,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,   183,
     184,    -1,    -1,   109,   110,   111,    -1,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    55,   134,   135,
      58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,    -1,
      -1,   109,   110,   111,    -1,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,   130,   131,   132,    55,   134,   135,    58,    -1,
      60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,   169,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
     178,   179,   180,   181,   182,   183,   184,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
     130,   131,   132,    55,   134,   135,    58,    -1,    60,    61,
      -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,
      -1,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    55,   134,   135,    58,    -1,    60,    61,    -1,    63,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,   130,   131,   132,    55,
     134,   135,    58,    -1,    60,    61,    -1,    63,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,   183,
     184,    -1,    -1,   109,   110,   111,    -1,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    55,   134,   135,
      58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,    -1,
      -1,   109,   110,   111,    -1,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,   130,   131,   132,    55,   134,   135,    58,    -1,
      60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,   169,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
     178,   179,   180,   181,   182,   183,   184,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
     130,   131,   132,    55,   134,   135,    58,    -1,    60,    61,
      -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,   169,
      -1,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    55,   134,   135,    58,    -1,    60,    61,    -1,    63,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,   169,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,   130,   131,   132,    55,
     134,   135,    58,    -1,    60,    61,    -1,    63,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,   169,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,   183,
     184,    -1,    -1,   109,   110,   111,    -1,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    55,   134,   135,
      58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,   169,    -1,   171,   172,    -1,    -1,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,    -1,
      -1,   109,   110,   111,    -1,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,   130,   131,   132,    55,   134,   135,    58,    -1,
      60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,   169,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
     178,   179,   180,   181,   182,   183,   184,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
     130,   131,   132,    55,   134,   135,    58,    -1,    60,    61,
      -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,
      -1,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    55,   134,   135,    58,    -1,    60,    61,    -1,    63,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,    -1,    -1,   109,   110,   111,    -1,   113,
     114,   115,   116,   117,   118,   119,   120,   121,   122,   123,
     124,   125,   126,   127,    -1,    -1,   130,   131,   132,    55,
     134,   135,    58,    -1,    60,    61,    -1,    63,   142,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   168,    -1,    -1,   171,   172,    -1,
      -1,    -1,    -1,    -1,   178,   179,   180,   181,   182,   183,
     184,    -1,    -1,   109,   110,   111,    -1,   113,   114,   115,
     116,   117,   118,   119,   120,   121,   122,   123,   124,   125,
     126,   127,    -1,    -1,   130,   131,   132,    55,   134,   135,
      58,    -1,    60,    61,    -1,    63,   142,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   168,    -1,    -1,   171,   172,    -1,    -1,    -1,
      -1,    -1,   178,   179,   180,   181,   182,   183,   184,    -1,
      -1,   109,   110,   111,    -1,   113,   114,   115,   116,   117,
     118,   119,   120,   121,   122,   123,   124,   125,   126,   127,
      -1,    -1,   130,   131,   132,    55,   134,   135,    58,    -1,
      60,    61,    -1,    63,   142,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     168,    -1,    -1,   171,   172,    -1,    -1,    -1,    -1,    -1,
     178,   179,   180,   181,   182,   183,   184,    -1,    -1,   109,
     110,   111,    -1,   113,   114,   115,   116,   117,   118,   119,
     120,   121,   122,   123,   124,   125,   126,   127,    -1,    -1,
     130,   131,   132,    55,   134,   135,    58,    -1,    60,    61,
      -1,    63,   142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,
      -1,   171,   172,    -1,    -1,    -1,    -1,    -1,   178,   179,
     180,   181,   182,   183,   184,    -1,    -1,   109,   110,   111,
      -1,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   125,   126,   127,    -1,    -1,   130,   131,
     132,    -1,   134,   135,    -1,    -1,    -1,    -1,    -1,    -1,
     142,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   168,    -1,    -1,   171,
     172,    -1,    -1,    -1,    -1,    -1,   178,   179,   180,   181,
     182,   183,   184,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    -1,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,    -1,    -1,    -1,    -1,    57,    -1,    59,    -1,
      -1,    -1,    -1,     4,     5,     6,     7,     8,     9,    10,
      11,    12,    13,    14,    15,    16,    17,    -1,    -1,    20,
      -1,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
      51,    52,   113,    -1,   115,   116,    57,    -1,    59,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   113,    -1,   115,   116
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,    79,   129,   144,   424,   425,   442,   443,   444,   168,
      13,    94,   113,   115,   116,   117,   118,   119,   120,   121,
     122,   123,   124,   200,   201,   203,   445,   446,   447,     0,
     442,   196,   444,   168,   445,   173,   174,   168,   196,     1,
       3,     4,     5,     6,     7,     8,     9,    10,    11,    12,
      13,    14,    15,    16,    17,    18,    20,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    57,    59,
      64,    65,    66,    67,    68,    69,    70,    71,    72,    76,
      80,   107,   112,   113,   115,   116,   128,   142,   168,   169,
     171,   172,   179,   180,   191,   193,   201,   202,   216,   310,
     311,   312,   313,   314,   315,   316,   317,   318,   319,   320,
     321,   324,   327,   329,   330,   331,   332,   333,   334,   335,
     336,   337,   338,   340,   342,   343,   344,   346,   347,   351,
     352,   353,   354,   355,   357,   363,   364,   365,   366,   378,
     383,   416,   419,   430,   436,   438,   448,   453,   454,   455,
     456,   457,   458,   459,   460,   486,   504,   505,   506,   507,
     445,   170,   446,    55,    58,    60,    61,    63,    80,   109,
     110,   111,   113,   114,   125,   126,   127,   130,   131,   132,
     134,   135,   168,   172,   178,   181,   182,   183,   184,   191,
     199,   200,   204,   205,   206,   209,   214,   215,   216,   217,
     218,   221,   222,   223,   224,   225,   226,   227,   228,   229,
     230,   231,   232,   234,   235,   236,   237,   242,   353,   430,
     176,   248,   201,   442,   127,   168,    65,    68,    69,    71,
     168,   168,   381,   442,   339,   340,   203,   417,   418,   203,
     430,   168,   168,     4,   113,   115,   116,   331,   333,   336,
     337,   168,   216,   443,   448,   454,   455,   456,   458,   459,
     460,   115,   354,    64,   172,   173,   179,   216,   237,   316,
     317,   326,   328,   330,   334,   335,   342,   343,   349,   350,
     351,   352,   353,   356,   363,   364,   383,   390,   391,   392,
     393,   394,   395,   486,   499,   500,   501,   502,   507,   508,
     201,   172,   317,   327,   330,   343,   347,   352,   443,   453,
     457,   486,   503,   504,   507,   508,   193,   193,   198,   165,
     176,   192,   240,   399,    95,   174,   437,   107,   203,   441,
     174,   174,   174,   193,   115,   116,   168,   216,   322,   323,
     448,   449,   450,   451,   452,   453,   457,   461,   462,   463,
     464,   465,   466,   467,   468,   469,   475,     3,    53,    54,
      56,    62,   345,     3,   172,   216,   316,   317,   331,   335,
     337,   348,   353,   433,   453,   457,   507,   442,    76,   314,
     316,   330,   343,   347,   352,   434,   453,   457,    72,   336,
     442,   336,   331,   337,   442,   325,   336,   337,   345,   364,
     331,   336,   331,   442,   171,   442,   174,   197,   168,   248,
     442,   442,     3,   305,   306,   321,   324,   330,   334,   172,
     216,   327,   330,   505,   507,   203,   203,   170,   168,   214,
     168,   168,   214,   168,   168,   214,   218,   168,   113,   115,
     116,   331,   336,   337,   168,   214,   214,    19,    21,    92,
     172,   181,   182,   216,   219,   220,   237,   244,   248,   327,
     330,   366,   397,   503,   507,   169,   174,   237,   168,   206,
     201,   172,   177,   172,   177,   127,   131,   133,   134,   135,
     168,   171,   172,   176,   177,   146,   147,   148,   149,   150,
     151,   152,   153,   154,   155,   156,   192,   239,   240,   241,
     168,   214,   218,   218,   185,   179,   186,   187,   181,   182,
     136,   137,   138,   139,   188,   189,   140,   141,   180,   178,
     190,   142,   143,   191,   170,   174,   171,   197,   167,   196,
     192,   314,   316,   327,   330,   431,   432,    64,    72,    73,
      74,    75,   172,   190,   203,   405,   407,   411,   413,   414,
     353,   170,   172,   216,   326,   330,   343,   350,   352,   395,
     499,   507,   442,   115,   116,   183,   201,   353,   382,   475,
     168,   412,   413,   168,   204,   232,   233,   170,   172,   237,
     397,   398,   415,   443,   508,   330,   443,   454,   455,   456,
     458,   459,   460,   170,   170,   170,   170,   170,   170,   170,
     442,   173,   237,   330,   334,   115,   172,   201,   327,   330,
     486,   505,   173,   172,   328,   330,   343,   350,   352,   443,
     498,   499,   507,   508,   173,   168,   168,   172,   180,   192,
     216,   448,   470,   471,   472,   473,   474,   475,   476,   477,
     478,   486,   488,   489,   490,   491,   492,   493,   510,   145,
     172,   216,   356,   501,   507,   330,   350,   336,   331,   442,
     442,   173,   174,   173,   174,   328,   330,   500,   507,   203,
     172,   328,   486,   500,   507,   168,   203,   173,   172,   453,
     457,   507,   442,   327,   330,   453,   457,   172,   174,   113,
     171,   172,   176,   200,   202,   237,   400,   401,   402,   403,
     404,    22,   400,   168,   203,   248,   168,   168,   442,   442,
     201,   443,   448,   450,   451,   452,   461,   463,   464,   465,
     467,   468,   469,   330,   443,   449,   462,   466,   174,   441,
     172,   442,   483,   486,   441,   442,   442,   437,   305,   168,
     442,   483,   441,   442,   442,   437,   442,   442,   168,   216,
     330,   439,   448,   449,   453,   462,   466,   168,   168,   329,
     330,   443,   327,   172,   173,   327,   503,   508,   441,   442,
     355,   176,   437,   305,   203,   203,   399,   316,   335,   435,
     453,   457,   442,   176,   437,   305,   417,   442,   330,   343,
     442,   330,   330,   442,   115,   354,   115,   116,   201,   353,
     358,   417,   145,   201,   330,   387,   388,   392,   393,   396,
     443,   442,   248,   321,   193,   453,   466,   330,   179,   237,
     391,   443,   216,   507,   203,   441,   168,   441,   170,   397,
     443,   508,   206,   397,   172,   397,   398,   237,   397,   170,
     397,   397,   397,   173,   170,   181,   182,   220,    18,   332,
     170,   174,   170,   168,   216,   479,   480,   481,   482,   483,
     179,   180,   170,   174,   509,   173,   174,   176,   192,   237,
     201,   237,   201,   125,   172,   201,   234,   125,   172,   203,
     366,   169,   237,   243,   234,   201,   176,   237,   397,   508,
     218,   221,   221,   221,   222,   222,   223,   223,   224,   224,
     224,   224,   225,   225,   226,   227,   228,   229,   230,   175,
     244,   236,   172,   201,    77,   307,   308,   237,   439,   172,
     317,   428,   176,   168,   203,   176,   203,   145,   179,   180,
     410,   170,   174,   203,   414,   170,   173,   168,   180,   216,
     507,   170,   201,   382,   475,   442,   176,   442,   405,   192,
     405,   170,   170,   174,   170,   174,   397,   508,   170,   170,
     170,   170,   170,   170,   168,   442,   483,   486,   168,   483,
     486,   201,   168,   328,   486,   500,   507,   172,   179,   216,
     237,   353,   237,   330,   168,   168,   327,   505,   507,   328,
     330,   195,   168,   387,   448,   471,   472,   473,   476,   489,
     490,   491,   173,   195,    18,   237,   330,   442,   443,   470,
     474,   488,   168,   442,   492,   510,   442,   442,   510,   168,
     442,   492,   442,   442,   510,   442,   442,   486,   173,   233,
     173,   330,   328,   498,   508,   203,   330,   201,   391,   394,
     394,   395,   510,   328,   500,   507,   195,   510,   195,   172,
     202,   232,   233,   440,   401,   175,   174,   509,   400,   171,
     172,   192,   404,   415,   168,   204,   195,   201,   439,   192,
     448,   450,   451,   452,   461,   463,   464,   465,   467,   468,
     469,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     170,   449,   462,   466,   449,   462,   466,   442,   192,   173,
     237,   337,   353,   484,   399,   248,   437,   387,   399,   248,
     443,   448,   330,   443,   439,   168,   244,   398,   244,   398,
     439,   115,   428,   248,   437,   176,   176,   437,   305,   428,
     248,   437,   442,   442,   442,   176,   170,   174,   170,   174,
     392,   393,   193,   173,   173,   174,   203,   441,   195,   170,
     397,   170,   174,   170,   174,   170,   174,   170,   218,   170,
     170,   170,   218,    18,   332,   237,   387,   480,   481,   482,
     330,   442,   443,   479,   442,   442,   170,   170,   169,   176,
     218,   243,   173,   173,   243,   234,   237,   173,   173,   125,
     130,   132,   202,   210,   211,   212,   170,   210,   173,   174,
     167,   401,   170,   170,   232,   175,   210,   203,   309,     1,
     249,   250,   442,    77,   391,   430,   428,   203,   173,     1,
     314,   316,   328,   330,   420,   421,   422,   423,   168,   409,
     407,   408,    85,   341,    18,   330,   442,   176,   442,   379,
      10,   178,   382,   384,   385,   382,   170,   398,   170,   193,
     204,   237,   398,   168,   442,   483,   486,   168,   483,   486,
     387,   387,   145,   389,   390,   391,   328,   500,   507,   173,
     173,   173,   237,   195,   195,   389,   476,   170,   170,   170,
     170,   170,   170,   170,   170,     5,   330,   168,   442,   448,
     475,   470,   474,   488,   168,   180,   216,   470,   474,   387,
     387,   173,   510,   173,   174,   389,   203,   210,   145,   173,
     184,   173,   509,   400,   402,   167,   170,   195,   170,   389,
     237,   170,   170,   170,   170,   170,   170,   170,   170,   170,
     168,   442,   483,   486,   168,   442,   483,   486,   168,   442,
     483,   486,   439,    22,   486,   158,   174,   184,   485,   173,
     174,   248,   170,   448,   170,   170,   170,   170,   426,   427,
     442,   248,   167,   420,   428,   248,   437,   426,   248,   360,
     361,   359,   367,   145,   442,   201,   441,   389,   170,   326,
     201,    85,   207,   208,   397,   218,   218,   218,   170,   170,
     170,   170,   479,   479,   218,   218,   176,   401,   174,   509,
     509,   167,   213,   172,   211,   213,   213,   173,   174,   133,
     171,   173,   169,   237,   509,   232,   173,   174,   193,   167,
     250,    18,    78,    80,    81,    83,    86,    87,    88,    89,
      90,    91,    92,    93,    94,    95,    96,    99,   100,   101,
     102,   103,   104,   105,   107,   115,   116,   128,   168,   172,
     203,   244,   245,   246,   247,   248,   252,   253,   262,   269,
     270,   271,   272,   273,   278,   279,   282,   283,   284,   285,
     286,   287,   288,   294,   295,   296,   310,   330,   334,   438,
     309,   429,   426,   170,   412,   439,   167,   421,   174,   193,
     174,   193,   415,   192,   406,   406,   380,   384,   382,   382,
     353,   174,   509,   203,   176,   176,   170,   387,   387,   170,
     170,   170,   174,   174,   173,   389,   389,   198,   170,   168,
     442,   483,   486,   168,   442,   492,   168,   442,   492,   486,
     329,     5,   179,   198,   237,   448,   442,   442,   168,    18,
     330,   443,   170,   170,   394,   198,   399,   173,   233,   233,
     167,   400,   442,   389,   442,   198,   168,   442,   483,   486,
     168,   442,   483,   486,   168,   442,   483,   486,   387,   387,
     387,   441,   173,   244,   237,   237,   337,   353,   429,   442,
     196,   167,   426,   248,   429,   176,   176,   176,   167,   442,
     392,   393,   198,   487,   488,   170,   175,   170,   174,   175,
     168,   442,   483,   486,   401,   509,   173,   173,   131,   210,
     211,   172,   211,   172,   211,   167,   203,   168,    68,    69,
     193,   248,   310,   438,   168,   168,    18,   246,   168,   168,
     193,   203,   193,   203,   179,   203,   176,   245,   168,    85,
     193,   203,   168,   168,   246,   168,   248,   237,   238,   238,
      14,   297,   273,   284,   175,   193,    97,    98,   277,   281,
     119,   143,   276,   118,   142,   280,   276,   396,   330,   193,
     429,   203,   203,   439,   170,   398,   412,   412,   382,   509,
     176,   176,    10,   385,   167,   192,   386,   384,   167,   420,
     170,   170,   145,   391,   145,   198,   198,   170,   387,   387,
     387,   237,   237,   198,   173,   198,   170,   173,   198,   170,
     387,   387,   387,   170,   170,   170,   399,   173,   485,   167,
     196,   430,   429,   167,   367,   367,   367,   168,   362,     3,
       5,    10,    80,   311,   318,   319,   327,   330,   368,   374,
     503,   170,   170,   237,   208,   237,   387,   509,   167,   173,
     210,   210,   234,   193,   248,   193,   248,   244,   254,   310,
     312,   315,   321,   330,   334,   244,    87,   170,   254,   157,
     158,   161,   162,   169,   170,   193,   244,   263,   264,   266,
     310,   193,   193,   244,   193,   401,   193,   244,   193,   193,
     415,   244,   263,   120,   121,   122,   123,   124,   289,   291,
     292,   193,   106,   193,    91,   168,   170,   442,   168,   168,
     246,   246,   273,   168,   283,   273,   283,   248,   167,   410,
     176,   167,   384,   384,   353,   203,   442,   176,   233,   509,
     167,   170,   170,   170,   170,   170,   198,   198,   173,   173,
     170,   442,   170,   170,   170,   237,   430,   197,   167,   167,
     167,   167,   415,   442,   327,   442,   327,   374,   193,   193,
     193,   168,   175,   216,   369,   370,   371,   377,   448,   449,
     462,   466,   174,   193,   203,   442,   170,   167,   173,   173,
     170,   170,   244,   330,   170,   168,   246,   170,   184,   193,
     266,   267,   246,   245,   193,   267,   170,   175,   244,   169,
     244,   245,   266,   193,   509,   170,   170,   170,   170,   248,
     291,   292,   168,   237,   168,   204,     1,   246,   218,   274,
     244,    82,   117,   275,   277,    82,   406,   384,   442,   509,
     509,   386,   401,   167,   442,   442,   173,   173,   197,   362,
     362,   362,   170,   369,   327,   366,   375,   503,   369,   193,
     443,   448,   237,   330,   443,   167,   174,   193,   376,   377,
     376,   376,   203,   246,   246,    84,    85,   176,   257,   258,
     259,   170,   244,    82,   246,   244,   169,   244,    82,   193,
     169,   244,   245,   266,   330,   352,   169,   244,   246,   264,
     267,   159,   160,   163,   164,   267,   268,   193,   244,   167,
     176,   259,   246,   246,   168,   293,   328,   330,   503,   193,
     204,   170,   175,   170,   174,   175,   170,   246,   168,   246,
     246,   246,   412,   509,   167,   167,   509,   442,   442,   442,
     442,   193,   168,   216,   372,   373,   483,   494,   495,   496,
     497,   193,   174,   193,   193,   448,   442,    82,     1,   233,
     255,   256,   440,     1,   175,     1,   196,   246,   257,    82,
     193,   170,   246,    82,   193,   184,   184,   246,   245,   267,
     267,   268,   193,    64,   244,   265,   353,   184,   184,    82,
     169,   244,   169,   244,   244,   245,   193,     1,   196,   293,
     193,   290,   168,   216,   439,   494,   201,   175,   193,   172,
     204,   298,   299,   300,   218,   234,   244,   276,   167,   442,
     442,   167,   495,   496,   497,   330,   442,   494,   174,   193,
     442,   442,   371,   246,   145,     1,   174,   175,   167,   303,
     304,   442,   246,    82,   193,   246,   244,   169,   169,   244,
     169,   244,   169,   244,   244,   245,   201,   353,   169,   244,
     169,   244,   246,   184,   184,   184,   184,   167,   303,   290,
     232,   170,   330,   443,   175,   113,   168,   170,   175,   174,
     170,   170,    82,   272,   442,   170,   170,   170,   494,   442,
     233,   255,   258,   260,   261,   310,   310,   246,   184,   184,
     184,   184,   169,   169,   244,   169,   244,   169,   244,   260,
     170,   248,   298,   173,   233,   193,   298,   300,   246,    82,
     168,   442,   483,   486,   373,   251,   442,   167,   258,   169,
     169,   244,   169,   244,   169,   244,   167,   248,   175,   204,
     170,   170,   175,   246,   387,     1,   442,   246,   251,   204,
     301,   168,   193,   301,   170,   246,   174,   175,   233,   170,
     204,   203,   302,   170,   193,   170,   174,   193,   203
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   194,   195,   196,   197,   198,   199,   199,   199,   199,
     199,   200,   200,   200,   200,   200,   200,   200,   200,   201,
     201,   202,   202,   203,   203,   203,   204,   205,   205,   206,
     206,   206,   206,   206,   206,   206,   206,   206,   206,   206,
     206,   206,   206,   206,   207,   207,   208,   208,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   209,   209,   209,   209,   209,   209,   209,   209,   209,
     209,   210,   210,   211,   211,   211,   211,   211,   211,   211,
     212,   212,   212,   213,   213,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   214,   214,   214,   214,   214,   214,
     214,   214,   214,   214,   215,   215,   216,   216,   216,   217,
     217,   217,   217,   218,   218,   218,   218,   218,   218,   218,
     218,   218,   219,   219,   219,   219,   220,   220,   221,   221,
     222,   222,   222,   222,   223,   223,   223,   224,   224,   224,
     225,   225,   225,   225,   225,   226,   226,   226,   227,   227,
     228,   228,   229,   229,   230,   230,   231,   231,   232,   232,
     232,   233,   234,   234,   235,   235,   236,   236,   236,   237,
     237,   237,   238,   238,   239,   239,   240,   240,   241,   241,
     241,   241,   241,   241,   241,   241,   241,   241,   241,   242,
     242,   242,   242,   242,   243,   243,   243,   243,   244,   244,
     245,   245,   246,   246,   246,   246,   246,   246,   246,   246,
     246,   246,   246,   246,   246,   246,   246,   246,   247,   247,
     248,   248,   249,   249,   250,   250,   250,   250,   250,   250,
     251,   251,   251,   252,   253,   253,   253,   253,   253,   253,
     253,   253,   254,   254,   254,   254,   255,   255,   255,   256,
     256,   257,   257,   257,   257,   257,   258,   258,   259,   260,
     260,   261,   261,   262,   262,   262,   262,   262,   262,   262,
     262,   262,   262,   262,   262,   263,   263,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   264,   264,   264,   264,   264,   264,
     264,   264,   264,   264,   265,   265,   265,   266,   266,   266,
     266,   267,   267,   268,   268,   268,   268,   269,   269,   269,
     269,   269,   269,   269,   269,   269,   269,   269,   269,   269,
     269,   269,   269,   269,   269,   269,   269,   270,   271,   272,
     273,   273,   274,   274,   275,   276,   276,   277,   277,   278,
     278,   278,   278,   278,   278,   279,   280,   280,   281,   282,
     282,   283,   283,   284,   284,   284,   285,   286,   287,   288,
     288,   288,   289,   289,   290,   290,   291,   291,   291,   291,
     292,   293,   293,   293,   293,   293,   294,   295,   295,   296,
     296,   296,   296,   296,   297,   297,   298,   298,   299,   299,
     300,   300,   301,   301,   301,   302,   302,   303,   303,   304,
     304,   305,   305,   306,   306,   307,   307,   308,   308,   309,
     309,   310,   310,   310,   311,   311,   312,   312,   312,   312,
     312,   313,   313,   313,   314,   314,   314,   314,   314,   314,
     315,   315,   315,   315,   315,   316,   316,   316,   316,   317,
     317,   318,   318,   318,   319,   319,   319,   319,   319,   320,
     320,   321,   321,   321,   321,   322,   322,   322,   322,   322,
     323,   323,   324,   324,   324,   324,   325,   325,   325,   326,
     326,   326,   327,   327,   327,   328,   328,   328,   329,   329,
     330,   330,   331,   332,   332,   332,   332,   332,   333,   334,
     334,   334,   335,   335,   336,   336,   336,   336,   336,   336,
     336,   336,   336,   337,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   338,   338,
     338,   338,   338,   338,   338,   338,   338,   338,   339,   339,
     340,   341,   341,   342,   342,   342,   342,   342,   343,   343,
     343,   344,   344,   344,   344,   345,   345,   345,   345,   345,
     345,   346,   346,   346,   346,   347,   348,   347,   347,   349,
     349,   349,   349,   350,   350,   350,   351,   351,   351,   351,
     352,   352,   352,   353,   353,   353,   353,   353,   353,   354,
     354,   354,   355,   355,   356,   356,   358,   357,   359,   357,
     360,   357,   361,   357,   357,   362,   362,   363,   363,   364,
     364,   365,   365,   365,   366,   366,   366,   366,   366,   366,
     366,   366,   367,   367,   368,   368,   368,   368,   368,   368,
     368,   368,   368,   368,   368,   368,   369,   369,   370,   370,
     371,   371,   371,   371,   372,   372,   372,   373,   374,   374,
     375,   375,   376,   376,   377,   378,   378,   379,   378,   378,
     380,   378,   378,   378,   381,   381,   382,   382,   383,   383,
     384,   384,   384,   384,   384,   385,   385,   386,   386,   386,
     387,   387,   387,   387,   388,   388,   388,   388,   388,   388,
     389,   389,   389,   389,   389,   389,   389,   390,   390,   390,
     390,   391,   391,   392,   392,   393,   393,   394,   394,   394,
     394,   394,   395,   395,   395,   395,   395,   396,   396,   397,
     397,   397,   398,   398,   398,   398,   399,   399,   399,   399,
     400,   400,   401,   401,   401,   401,   401,   402,   402,   403,
     403,   404,   404,   404,   404,   404,   405,   405,   406,   406,
     408,   407,   409,   407,   407,   407,   407,   410,   410,   410,
     410,   411,   411,   411,   411,   412,   412,   413,   413,   414,
     414,   415,   415,   415,   415,   416,   416,   416,   417,   417,
     418,   418,   419,   419,   419,   419,   420,   420,   421,   421,
     422,   422,   422,   423,   423,   423,   424,   424,   425,   425,
     426,   426,   427,   427,   428,   429,   430,   430,   430,   430,
     430,   430,   430,   430,   430,   430,   430,   431,   430,   432,
     430,   433,   430,   434,   430,   435,   430,   430,   430,   430,
     436,   436,   436,   437,   437,   438,   438,   438,   438,   438,
     438,   438,   438,   438,   438,   439,   439,   439,   439,   440,
     441,   441,   442,   442,   443,   443,   444,   444,   444,   444,
     445,   445,   446,   446,   446,   447,   447,   447,   448,   448,
     448,   449,   449,   449,   449,   450,   450,   450,   450,   450,
     451,   451,   451,   451,   451,   451,   451,   452,   452,   452,
     452,   453,   453,   453,   454,   454,   454,   454,   454,   455,
     455,   455,   455,   455,   456,   456,   456,   456,   456,   456,
     457,   457,   457,   458,   458,   458,   458,   458,   459,   459,
     459,   459,   459,   460,   460,   460,   460,   460,   460,   461,
     461,   462,   462,   462,   462,   463,   463,   463,   463,   463,
     464,   464,   464,   464,   464,   464,   464,   465,   465,   465,
     465,   466,   466,   466,   467,   467,   467,   467,   467,   468,
     468,   468,   468,   468,   469,   469,   469,   469,   469,   469,
     470,   470,   470,   470,   470,   471,   471,   471,   471,   472,
     472,   472,   472,   473,   473,   473,   474,   474,   474,   474,
     474,   475,   475,   476,   476,   476,   476,   477,   477,   478,
     478,   479,   479,   479,   480,   480,   480,   480,   480,   480,
     481,   481,   481,   481,   482,   482,   482,   483,   483,   483,
     483,   483,   483,   484,   484,   484,   484,   484,   484,   485,
     485,   486,   486,   486,   486,   487,   487,   488,   488,   488,
     488,   489,   489,   489,   489,   489,   490,   490,   490,   490,
     491,   491,   491,   492,   492,   492,   493,   493,   493,   493,
     493,   493,   494,   494,   494,   495,   495,   495,   495,   495,
     496,   496,   496,   496,   497,   497,   498,   498,   498,   499,
     499,   499,   500,   500,   500,   500,   500,   500,   500,   501,
     501,   501,   501,   501,   501,   501,   501,   501,   501,   501,
     501,   501,   501,   501,   502,   502,   502,   502,   503,   503,
     503,   504,   504,   505,   505,   505,   505,   505,   505,   505,
     506,   506,   506,   506,   506,   506,   507,   507,   507,   508,
     508,   508,   509,   509,   510,   510
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     0,     0,     0,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     2,     1,
       1,     1,     1,     3,     3,     3,     5,     6,     2,     2,
       2,     2,     2,     2,     1,     3,     3,     3,     1,     4,
       4,     4,     4,     4,     7,     3,     3,     3,     3,     3,
       2,     5,     3,     3,     3,     5,     2,     2,     7,     8,
       5,     1,     3,     1,     2,     4,     3,     5,     3,     5,
       2,     2,     2,     0,     2,     1,     1,     1,     2,     2,
       2,     2,     2,     2,     4,     5,     2,     4,     4,     4,
       6,     4,     2,     4,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     4,     5,     5,     4,     5,     5,
       5,     4,     2,     2,     3,     3,     1,     1,     1,     3,
       1,     3,     3,     3,     1,     3,     3,     1,     3,     3,
       1,     3,     3,     3,     3,     1,     3,     3,     1,     3,
       1,     3,     1,     3,     1,     3,     1,     3,     1,     5,
       4,     1,     0,     1,     1,     3,     1,     4,     1,     1,
       3,     6,     0,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     3,
       4,     4,     6,     6,     1,     1,     3,     3,     1,     3,
       0,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     4,     4,
       2,     5,     1,     2,     2,     3,     2,     3,     2,     1,
       2,     3,     2,     2,     5,     7,     5,     9,     7,     5,
       9,     7,     1,     1,     1,     2,     1,     3,     1,     1,
       3,     2,     3,     3,     2,     2,     1,     2,     2,     0,
       1,     2,     3,     4,     6,     5,     7,     6,     7,     7,
       8,     4,     6,     5,     7,     1,     3,     4,     5,     4,
       3,     5,     1,     2,     3,     3,     3,     5,     5,     5,
       5,     3,     5,     5,     5,     3,     4,     5,     5,     5,
       5,     5,     7,     7,     7,     7,     7,     7,     7,     2,
       3,     4,     4,     4,     4,     6,     6,     6,     6,     6,
       6,     6,     3,     4,     1,     2,     2,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     4,     2,
       3,     3,     2,     3,     2,     3,     3,     6,     2,     2,
       3,     3,     3,     3,     3,     3,     5,     5,     5,     4,
       0,     1,     1,     3,     4,     1,     1,     4,     6,     3,
       5,     5,     5,     8,     9,     1,     1,     1,     4,     3,
       3,     1,     3,     1,     3,     5,     1,     2,     5,     3,
       3,     4,     6,     7,     0,     2,     1,     1,     1,     1,
       2,     1,     2,     2,     2,     1,     3,     1,     1,     6,
       8,    10,    12,    14,     0,     1,     0,     1,     1,     3,
       4,     7,     0,     1,     3,     1,     3,     0,     1,     2,
       2,     0,     1,     2,     3,     0,     1,     3,     4,     1,
       3,     2,     2,     2,     6,     4,     1,     1,     1,     1,
       1,     2,     3,     6,     3,     3,     4,     5,     2,     3,
       1,     2,     2,     3,     8,     9,     9,     8,     8,     3,
       5,     3,     3,     4,     4,     4,     4,     3,     4,     4,
       5,     2,     1,     1,     1,     3,     3,     2,     4,     6,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     1,     1,     1,     0,     1,
       2,     3,     1,     1,     1,     1,     1,     1,     4,     1,
       2,     3,     2,     3,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0,     1,
       5,     0,     1,     1,     2,     3,     3,     3,     2,     3,
       3,     1,     2,     2,     2,     4,     4,     4,     4,     1,
       1,     1,     2,     2,     3,     1,     0,     3,     2,     1,
       2,     2,     3,     1,     2,     2,     2,     3,     3,     3,
       1,     2,     2,     1,     2,     3,     1,     2,     3,     1,
       3,     4,     1,     1,     1,     1,     0,     8,     0,    10,
       0,    10,     0,    10,     1,     0,     3,     3,     3,     1,
       1,     2,     1,     1,     1,     2,     1,     2,     1,     2,
       1,     2,     0,     3,     3,     3,     4,     4,     5,     4,
       2,     2,     3,     4,     2,     2,     0,     1,     1,     4,
       1,     2,     2,     2,     0,     1,     4,     1,     2,     3,
       1,     2,     0,     1,     2,     8,     9,     0,    11,    10,
       0,    12,    11,     1,     2,     3,     0,     1,     3,     3,
       0,     3,     2,     5,     4,     1,     1,     0,     2,     5,
       0,     1,     1,     3,     1,     2,     1,     2,     4,     4,
       0,     1,     1,     1,     3,     3,     3,     1,     3,     3,
       5,     1,     3,     3,     3,     2,     3,     1,     3,     3,
       4,     1,     1,     1,     1,     2,     1,     1,     3,     1,
       2,     1,     1,     2,     1,     2,     0,     2,     2,     4,
       1,     4,     0,     1,     2,     3,     4,     2,     2,     1,
       2,     2,     3,     3,     5,     4,     1,     3,     0,     2,
       0,     5,     0,     5,     4,     1,     8,     0,     1,     1,
       1,     1,     1,     1,     1,     0,     1,     1,     2,     5,
       4,     1,     1,     3,     3,     2,     3,     3,     2,     4,
       1,     4,     7,     5,     8,     6,     1,     2,     2,     2,
       1,     1,     3,     2,     3,     1,     0,     1,     4,     5,
       0,     1,     4,     5,     0,     0,     1,     1,     2,     2,
       2,     2,     2,     2,     1,     2,     5,     0,     6,     0,
       8,     0,     7,     0,     7,     0,     8,     1,     1,     2,
       1,     2,     3,     0,     5,     3,     4,     4,     4,     4,
       5,     5,     5,     5,     6,     1,     1,     1,     1,     3,
       0,     5,     0,     1,     1,     2,     6,     4,     3,     1,
       1,     3,     0,     1,     4,     1,     1,     1,     1,     2,
       3,     2,     1,     2,     2,     2,     3,     3,     4,     5,
       2,     4,     5,     4,     5,     3,     4,     6,     7,     3,
       4,     2,     1,     2,     4,     6,     7,     3,     4,     2,
       3,     3,     4,     5,     4,     5,     4,     5,     3,     4,
       1,     1,     1,     4,     6,     7,     3,     4,     2,     3,
       3,     3,     4,     4,     5,     4,     5,     3,     4,     1,
       3,     2,     1,     2,     2,     2,     3,     3,     4,     5,
       2,     4,     5,     4,     5,     3,     4,     6,     7,     3,
       4,     2,     1,     2,     4,     6,     7,     3,     4,     2,
       3,     3,     4,     5,     4,     5,     4,     5,     3,     4,
       2,     4,     1,     2,     2,     2,     3,     3,     4,     2,
       4,     4,     3,     4,     6,     3,     2,     4,     1,     2,
       2,     1,     1,     2,     3,     3,     4,     2,     4,     4,
       6,     1,     2,     2,     2,     2,     2,     3,     3,     4,
       1,     4,     4,     3,     3,     6,     3,     2,     3,     4,
       5,     3,     1,     1,     1,     3,     3,     3,     5,     1,
       1,     3,     3,     4,     4,     0,     1,     1,     3,     2,
       2,     2,     2,     2,     3,     4,     1,     4,     4,     3,
       3,     6,     3,     1,     2,     1,     2,     6,     5,     6,
       7,     7,     1,     2,     2,     2,     2,     2,     3,     4,
       1,     4,     4,     3,     6,     3,     1,     1,     2,     1,
       1,     2,     2,     3,     3,     2,     3,     2,     3,     3,
       3,     2,     2,     4,     4,     3,     3,     2,     2,     3,
       2,     4,     3,     2,     4,     4,     4,     5,     1,     2,
       1,     1,     1,     2,     3,     3,     2,     3,     2,     3,
       3,     4,     2,     3,     4,     2,     3,     4,     5,     5,
       6,     6,     0,     1,     0,     2
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]));
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Location data for the lookahead symbol.  */
YYLTYPE yylloc
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
/* Number of syntax errors so far.  */
int yynerrs;




/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* push: %empty  */
#line 751 "Parser/parser.yy"
                { typedefTable.enterScope(); }
#line 10551 "Parser/parser.cc"
    break;

  case 3: /* recovery_push: %empty  */
#line 757 "Parser/parser.yy"
                { enterRecoveryScope(); }
#line 10557 "Parser/parser.cc"
    break;

  case 4: /* recovery_pop: %empty  */
#line 761 "Parser/parser.yy"
                { leaveRecoveryScope(); }
#line 10563 "Parser/parser.cc"
    break;

  case 5: /* pop: %empty  */
#line 765 "Parser/parser.yy"
                { typedefTable.leaveScope(); }
#line 10569 "Parser/parser.cc"
    break;

  case 6: /* constant: INTEGERconstant  */
#line 772 "Parser/parser.yy"
                                                                                { (yyval.expr) = new ExpressionNode( build_constantInteger( (yyloc), *(yyvsp[0].tok) ) ); }
#line 10575 "Parser/parser.cc"
    break;

  case 7: /* constant: FLOATING_DECIMALconstant  */
#line 773 "Parser/parser.yy"
                                                                        { (yyval.expr) = new ExpressionNode( build_constantFloat( (yyloc), *(yyvsp[0].tok) ) ); }
#line 10581 "Parser/parser.cc"
    break;

  case 8: /* constant: FLOATING_FRACTIONconstant  */
#line 774 "Parser/parser.yy"
                                                                        { (yyval.expr) = new ExpressionNode( build_constantFloat( (yyloc), *(yyvsp[0].tok) ) ); }
#line 10587 "Parser/parser.cc"
    break;

  case 9: /* constant: FLOATINGconstant  */
#line 775 "Parser/parser.yy"
                                                                                { (yyval.expr) = new ExpressionNode( build_constantFloat( (yyloc), *(yyvsp[0].tok) ) ); }
#line 10593 "Parser/parser.cc"
    break;

  case 10: /* constant: CHARACTERconstant  */
#line 776 "Parser/parser.yy"
                                                                                { (yyval.expr) = new ExpressionNode( build_constantChar( (yyloc), *(yyvsp[0].tok) ) ); }
#line 10599 "Parser/parser.cc"
    break;

  case 22: /* identifier_at: '@'  */
#line 798 "Parser/parser.yy"
                { Token tok = { new string( DeclarationNode::anonymous.newName() ), yylval.tok.loc }; (yyval.tok) = tok; }
#line 10605 "Parser/parser.cc"
    break;

  case 26: /* string_literal: string_literal_list  */
#line 808 "Parser/parser.yy"
                                                                                { (yyval.expr) = new ExpressionNode( build_constantStr( (yyloc), *(yyvsp[0].str) ) ); }
#line 10611 "Parser/parser.cc"
    break;

  case 27: /* string_literal_list: STRINGliteral  */
#line 812 "Parser/parser.yy"
                                                                                { (yyval.str) = (yyvsp[0].tok); }
#line 10617 "Parser/parser.cc"
    break;

  case 28: /* string_literal_list: string_literal_list STRINGliteral  */
#line 814 "Parser/parser.yy"
                {
			if ( ! appendStr( *(yyvsp[-1].str), *(yyvsp[0].tok) ) ) YYERROR;		// append 2nd juxtaposed string to 1st
			delete (yyvsp[0].tok);									// allocated by lexer
			(yyval.str) = (yyvsp[-1].str);									// conversion from tok to str
		}
#line 10627 "Parser/parser.cc"
    break;

  case 29: /* primary_expression: IDENTIFIER  */
#line 825 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_varref( (yyloc), (yyvsp[0].tok) ) ); }
#line 10633 "Parser/parser.cc"
    break;

  case 30: /* primary_expression: quasi_keyword  */
#line 827 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_varref( (yyloc), (yyvsp[0].tok) ) ); }
#line 10639 "Parser/parser.cc"
    break;

  case 31: /* primary_expression: TYPEDIMname  */
#line 829 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_dimensionref( (yyloc), (yyvsp[0].tok) ) ); }
#line 10645 "Parser/parser.cc"
    break;

  case 33: /* primary_expression: '(' comma_expression ')'  */
#line 832 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 10651 "Parser/parser.cc"
    break;

  case 34: /* primary_expression: '(' compound_statement ')'  */
#line 834 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::StmtExpr( (yyloc), dynamic_cast<ast::CompoundStmt *>( maybeMoveBuild( (yyvsp[-1].stmt) ) ) ) ); }
#line 10657 "Parser/parser.cc"
    break;

  case 35: /* primary_expression: type_name '.' identifier  */
#line 836 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_qualified_expr( (yyloc), DeclarationNode::newFromTypeData( (yyvsp[-2].type) ), build_varref( (yylsp[0]), (yyvsp[0].tok) ) ) ); }
#line 10663 "Parser/parser.cc"
    break;

  case 36: /* primary_expression: type_name '.' '[' field_name_list ']'  */
#line 838 "Parser/parser.yy"
                { SemanticError( (yyloc), "Qualified name is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 10669 "Parser/parser.cc"
    break;

  case 37: /* primary_expression: GENERIC '(' assignment_expression ',' generic_assoc_list ')'  */
#line 840 "Parser/parser.yy"
                {
			// add the missing control expression to the GenericExpr and return it
			(yyvsp[-1].genexpr)->control = maybeMoveBuild( (yyvsp[-3].expr) );
			(yyval.expr) = new ExpressionNode( (yyvsp[-1].genexpr) );
		}
#line 10679 "Parser/parser.cc"
    break;

  case 38: /* primary_expression: IDENTIFIER IDENTIFIER  */
#line 850 "Parser/parser.yy"
                { IdentifierBeforeIdentifier( *(yyvsp[-1].tok).str, *(yyvsp[0].tok).str, "expression" ); YYERROR; }
#line 10685 "Parser/parser.cc"
    break;

  case 39: /* primary_expression: IDENTIFIER type_qualifier  */
#line 852 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type qualifier" ); YYERROR; }
#line 10691 "Parser/parser.cc"
    break;

  case 40: /* primary_expression: IDENTIFIER storage_class  */
#line 854 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "storage class" ); YYERROR; }
#line 10697 "Parser/parser.cc"
    break;

  case 41: /* primary_expression: IDENTIFIER basic_type_name  */
#line 856 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type" ); YYERROR; }
#line 10703 "Parser/parser.cc"
    break;

  case 42: /* primary_expression: IDENTIFIER TYPEDEFname  */
#line 858 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type" ); YYERROR; }
#line 10709 "Parser/parser.cc"
    break;

  case 43: /* primary_expression: IDENTIFIER TYPEGENname  */
#line 860 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type" ); YYERROR; }
#line 10715 "Parser/parser.cc"
    break;

  case 45: /* generic_assoc_list: generic_assoc_list ',' generic_association  */
#line 866 "Parser/parser.yy"
                {
			// steal the association node from the singleton and delete the wrapper
			assert( 1 == (yyvsp[0].genexpr)->associations.size() );
			(yyvsp[-2].genexpr)->associations.push_back( (yyvsp[0].genexpr)->associations.front() );
			delete (yyvsp[0].genexpr);
			(yyval.genexpr) = (yyvsp[-2].genexpr);
		}
#line 10727 "Parser/parser.cc"
    break;

  case 46: /* generic_association: type_no_function ':' assignment_expression  */
#line 877 "Parser/parser.yy"
                {
			// create a GenericExpr wrapper with one association pair
			(yyval.genexpr) = new ast::GenericExpr( (yyloc), nullptr, { { maybeMoveBuildType( (yyvsp[-2].decl) ), maybeMoveBuild( (yyvsp[0].expr) ) } } );
		}
#line 10736 "Parser/parser.cc"
    break;

  case 47: /* generic_association: DEFAULT ':' assignment_expression  */
#line 882 "Parser/parser.yy"
                { (yyval.genexpr) = new ast::GenericExpr( (yyloc), nullptr, { { maybeMoveBuild( (yyvsp[0].expr) ) } } ); }
#line 10742 "Parser/parser.cc"
    break;

  case 49: /* postfix_expression: postfix_expression '[' tuple_expression_list ']'  */
#line 891 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Index, (yyvsp[-3].expr), new ExpressionNode( build_tuple( (yyloc), (yyvsp[-1].expr) ) ) ) ); }
#line 10748 "Parser/parser.cc"
    break;

  case 50: /* postfix_expression: constant '[' assignment_expression ']'  */
#line 893 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Index, (yyvsp[-3].expr), (yyvsp[-1].expr) ) ); }
#line 10754 "Parser/parser.cc"
    break;

  case 51: /* postfix_expression: string_literal '[' assignment_expression ']'  */
#line 895 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Index, (yyvsp[-3].expr), (yyvsp[-1].expr) ) ); }
#line 10760 "Parser/parser.cc"
    break;

  case 52: /* postfix_expression: postfix_expression '{' argument_expression_list_opt '}'  */
#line 897 "Parser/parser.yy"
                {
			Token fn;
			fn.str = new std::string( "?{}" );			// location undefined - use location of '{'?
			(yyval.expr) = new ExpressionNode( new ast::ConstructorExpr( (yyloc), build_func( (yyloc), new ExpressionNode( build_varref( (yyloc), fn ) ), (yyvsp[-3].expr)->set_last( (yyvsp[-1].expr) ) ) ) );
		}
#line 10770 "Parser/parser.cc"
    break;

  case 53: /* postfix_expression: postfix_expression '(' argument_expression_list_opt ')'  */
#line 903 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_func( (yyloc), (yyvsp[-3].expr), (yyvsp[-1].expr) ) ); }
#line 10776 "Parser/parser.cc"
    break;

  case 54: /* postfix_expression: VA_ARG '(' primary_expression ',' declaration_specifier_nobody abstract_parameter_declarator_opt ')'  */
#line 905 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_va_arg( (yyloc), (yyvsp[-4].expr), ( (yyvsp[-1].decl) ? (yyvsp[-1].decl)->addType( (yyvsp[-2].decl) ) : (yyvsp[-2].decl) ) ) ); }
#line 10782 "Parser/parser.cc"
    break;

  case 55: /* postfix_expression: postfix_expression '`' identifier  */
#line 907 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_func( (yyloc), new ExpressionNode( build_varref( (yylsp[0]), build_postfix_name( (yyvsp[0].tok) ) ) ), (yyvsp[-2].expr) ) ); }
#line 10788 "Parser/parser.cc"
    break;

  case 56: /* postfix_expression: constant '`' identifier  */
#line 909 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_func( (yyloc), new ExpressionNode( build_varref( (yylsp[0]), build_postfix_name( (yyvsp[0].tok) ) ) ), (yyvsp[-2].expr) ) ); }
#line 10794 "Parser/parser.cc"
    break;

  case 57: /* postfix_expression: string_literal '`' identifier  */
#line 911 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_func( (yyloc), new ExpressionNode( build_varref( (yylsp[0]), build_postfix_name( (yyvsp[0].tok) ) ) ), (yyvsp[-2].expr) ) ); }
#line 10800 "Parser/parser.cc"
    break;

  case 58: /* postfix_expression: postfix_expression '.' identifier_or_type_name  */
#line 931 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yylsp[0]), (yyvsp[-2].expr), build_varref( (yylsp[0]), (yyvsp[0].tok) ) ) ); }
#line 10806 "Parser/parser.cc"
    break;

  case 59: /* postfix_expression: postfix_expression '.' INTEGERconstant  */
#line 934 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), (yyvsp[-2].expr), build_constantInteger( (yyloc), *(yyvsp[0].tok) ) ) ); }
#line 10812 "Parser/parser.cc"
    break;

  case 60: /* postfix_expression: postfix_expression FLOATING_FRACTIONconstant  */
#line 936 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), (yyvsp[-1].expr), build_field_name_FLOATING_FRACTIONconstant( (yyloc), *(yyvsp[0].tok) ) ) ); }
#line 10818 "Parser/parser.cc"
    break;

  case 61: /* postfix_expression: postfix_expression '.' '[' field_name_list ']'  */
#line 938 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), (yyvsp[-4].expr), build_tuple( (yyloc), (yyvsp[-1].expr) ) ) ); }
#line 10824 "Parser/parser.cc"
    break;

  case 62: /* postfix_expression: postfix_expression '.' aggregate_control  */
#line 940 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_keyword_cast( (yyloc), (yyvsp[0].aggKey), (yyvsp[-2].expr) ) ); }
#line 10830 "Parser/parser.cc"
    break;

  case 63: /* postfix_expression: postfix_expression ARROW identifier  */
#line 942 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_pfieldSel( (yylsp[0]), (yyvsp[-2].expr), build_varref( (yylsp[0]), (yyvsp[0].tok) ) ) ); }
#line 10836 "Parser/parser.cc"
    break;

  case 64: /* postfix_expression: postfix_expression ARROW INTEGERconstant  */
#line 944 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_pfieldSel( (yyloc), (yyvsp[-2].expr), build_constantInteger( (yyloc), *(yyvsp[0].tok) ) ) ); }
#line 10842 "Parser/parser.cc"
    break;

  case 65: /* postfix_expression: postfix_expression ARROW '[' field_name_list ']'  */
#line 946 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_pfieldSel( (yyloc), (yyvsp[-4].expr), build_tuple( (yyloc), (yyvsp[-1].expr) ) ) ); }
#line 10848 "Parser/parser.cc"
    break;

  case 66: /* postfix_expression: postfix_expression ICR  */
#line 948 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_unary_val( (yyloc), OperKinds::IncrPost, (yyvsp[-1].expr) ) ); }
#line 10854 "Parser/parser.cc"
    break;

  case 67: /* postfix_expression: postfix_expression DECR  */
#line 950 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_unary_val( (yyloc), OperKinds::DecrPost, (yyvsp[-1].expr) ) ); }
#line 10860 "Parser/parser.cc"
    break;

  case 68: /* postfix_expression: '(' type_no_function ')' '{' initializer_list_opt comma_opt '}'  */
#line 952 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_compoundLiteral( (yyloc), (yyvsp[-5].decl), new InitializerNode( (yyvsp[-2].init), true ) ) ); }
#line 10866 "Parser/parser.cc"
    break;

  case 69: /* postfix_expression: '(' type_no_function ')' '@' '{' initializer_list_opt comma_opt '}'  */
#line 954 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_compoundLiteral( (yyloc), (yyvsp[-6].decl), (new InitializerNode( (yyvsp[-2].init), true ))->set_maybeConstructed( false ) ) ); }
#line 10872 "Parser/parser.cc"
    break;

  case 70: /* postfix_expression: '^' primary_expression '{' argument_expression_list_opt '}'  */
#line 956 "Parser/parser.yy"
                {
			Token fn;
			fn.str = new string( "^?{}" );				// location undefined
			(yyval.expr) = new ExpressionNode( build_func( (yyloc), new ExpressionNode( build_varref( (yyloc), fn ) ), (yyvsp[-3].expr)->set_last( (yyvsp[-1].expr) ) ) );
		}
#line 10882 "Parser/parser.cc"
    break;

  case 72: /* field_name_list: field_name_list ',' field  */
#line 965 "Parser/parser.yy"
                                                                        { (yyval.expr) = (yyvsp[-2].expr)->set_last( (yyvsp[0].expr) ); }
#line 10888 "Parser/parser.cc"
    break;

  case 74: /* field: FLOATING_DECIMALconstant field  */
#line 971 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), new ExpressionNode( build_field_name_FLOATING_DECIMALconstant( (yyloc), *(yyvsp[-1].tok) ) ), maybeMoveBuild( (yyvsp[0].expr) ) ) ); }
#line 10894 "Parser/parser.cc"
    break;

  case 75: /* field: FLOATING_DECIMALconstant '[' field_name_list ']'  */
#line 973 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), new ExpressionNode( build_field_name_FLOATING_DECIMALconstant( (yyloc), *(yyvsp[-3].tok) ) ), build_tuple( (yyloc), (yyvsp[-1].expr) ) ) ); }
#line 10900 "Parser/parser.cc"
    break;

  case 76: /* field: field_name '.' field  */
#line 975 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), (yyvsp[-2].expr), maybeMoveBuild( (yyvsp[0].expr) ) ) ); }
#line 10906 "Parser/parser.cc"
    break;

  case 77: /* field: field_name '.' '[' field_name_list ']'  */
#line 977 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_fieldSel( (yyloc), (yyvsp[-4].expr), build_tuple( (yyloc), (yyvsp[-1].expr) ) ) ); }
#line 10912 "Parser/parser.cc"
    break;

  case 78: /* field: field_name ARROW field  */
#line 979 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_pfieldSel( (yyloc), (yyvsp[-2].expr), maybeMoveBuild( (yyvsp[0].expr) ) ) ); }
#line 10918 "Parser/parser.cc"
    break;

  case 79: /* field: field_name ARROW '[' field_name_list ']'  */
#line 981 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_pfieldSel( (yyloc), (yyvsp[-4].expr), build_tuple( (yyloc), (yyvsp[-1].expr) ) ) ); }
#line 10924 "Parser/parser.cc"
    break;

  case 80: /* field_name: INTEGERconstant fraction_constants_opt  */
#line 986 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_field_name_fraction_constants( (yyloc), build_constantInteger( (yyloc), *(yyvsp[-1].tok) ), (yyvsp[0].expr) ) ); }
#line 10930 "Parser/parser.cc"
    break;

  case 81: /* field_name: FLOATINGconstant fraction_constants_opt  */
#line 988 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_field_name_fraction_constants( (yyloc), build_field_name_FLOATINGconstant( (yyloc), *(yyvsp[-1].tok) ), (yyvsp[0].expr) ) ); }
#line 10936 "Parser/parser.cc"
    break;

  case 82: /* field_name: identifier_at fraction_constants_opt  */
#line 990 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_field_name_fraction_constants( (yyloc), build_varref( (yylsp[-1]), (yyvsp[-1].tok) ), (yyvsp[0].expr) ) );	}
#line 10942 "Parser/parser.cc"
    break;

  case 83: /* fraction_constants_opt: %empty  */
#line 995 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 10948 "Parser/parser.cc"
    break;

  case 84: /* fraction_constants_opt: fraction_constants_opt FLOATING_FRACTIONconstant  */
#line 997 "Parser/parser.yy"
                {
			ast::Expr * constant = build_field_name_FLOATING_FRACTIONconstant( (yyloc), *(yyvsp[0].tok) );
			(yyval.expr) = (yyvsp[-1].expr) != nullptr ? new ExpressionNode( build_fieldSel( (yyloc), (yyvsp[-1].expr), constant ) ) : new ExpressionNode( constant );
		}
#line 10957 "Parser/parser.cc"
    break;

  case 87: /* unary_expression: string_literal  */
#line 1009 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[0].expr); }
#line 10963 "Parser/parser.cc"
    break;

  case 88: /* unary_expression: EXTENSION cast_expression  */
#line 1011 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[0].expr)->set_extension( true ); }
#line 10969 "Parser/parser.cc"
    break;

  case 89: /* unary_expression: ptrref_operator cast_expression  */
#line 1016 "Parser/parser.yy"
                {
			switch ( (yyvsp[-1].oper) ) {
			case OperKinds::AddressOf:
				(yyval.expr) = new ExpressionNode( new ast::AddressExpr( maybeMoveBuild( (yyvsp[0].expr) ) ) );
				break;
			case OperKinds::PointTo:
				(yyval.expr) = new ExpressionNode( build_unary_val( (yyloc), (yyvsp[-1].oper), (yyvsp[0].expr) ) );
				break;
			case OperKinds::And:
				(yyval.expr) = new ExpressionNode( new ast::AddressExpr( new ast::AddressExpr( maybeMoveBuild( (yyvsp[0].expr) ) ) ) );
				break;
			default:
				assert( false );
			}
		}
#line 10989 "Parser/parser.cc"
    break;

  case 90: /* unary_expression: unary_operator cast_expression  */
#line 1032 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_unary_val( (yyloc), (yyvsp[-1].oper), (yyvsp[0].expr) ) ); }
#line 10995 "Parser/parser.cc"
    break;

  case 91: /* unary_expression: ICR unary_expression  */
#line 1034 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_unary_val( (yyloc), OperKinds::Incr, (yyvsp[0].expr) ) ); }
#line 11001 "Parser/parser.cc"
    break;

  case 92: /* unary_expression: DECR unary_expression  */
#line 1036 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_unary_val( (yyloc), OperKinds::Decr, (yyvsp[0].expr) ) ); }
#line 11007 "Parser/parser.cc"
    break;

  case 93: /* unary_expression: SIZEOF unary_expression  */
#line 1038 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::SizeofExpr( (yyloc), new ast::TypeofType( maybeMoveBuild( (yyvsp[0].expr) ) ) ) ); }
#line 11013 "Parser/parser.cc"
    break;

  case 94: /* unary_expression: SIZEOF '(' type_no_function ')'  */
#line 1040 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::SizeofExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl) ) ) ); }
#line 11019 "Parser/parser.cc"
    break;

  case 95: /* unary_expression: SIZEOF '(' attribute_list type_no_function ')'  */
#line 1042 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::SizeofExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ) ) ) ); }
#line 11025 "Parser/parser.cc"
    break;

  case 96: /* unary_expression: alignof_operator unary_expression  */
#line 1044 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::AlignofExpr( (yyloc), new ast::TypeofType( maybeMoveBuild( (yyvsp[0].expr) ) ),
					(yyvsp[-1].oper) == OperKinds::AlignOf ? ast::AlignofExpr::Alignof : ast::AlignofExpr::__Alignof ) ); }
#line 11032 "Parser/parser.cc"
    break;

  case 97: /* unary_expression: alignof_operator '(' type_no_function ')'  */
#line 1047 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::AlignofExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl) ),
					(yyvsp[-3].oper) == OperKinds::AlignOf ? ast::AlignofExpr::Alignof : ast::AlignofExpr::__Alignof ) ); }
#line 11039 "Parser/parser.cc"
    break;

  case 98: /* unary_expression: SIZEOF '(' cfa_abstract_function ')'  */
#line 1053 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::SizeofExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl) ) ) ); }
#line 11045 "Parser/parser.cc"
    break;

  case 99: /* unary_expression: alignof_operator '(' cfa_abstract_function ')'  */
#line 1055 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::AlignofExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl) ),
					(yyvsp[-3].oper) == OperKinds::AlignOf ? ast::AlignofExpr::Alignof : ast::AlignofExpr::__Alignof ) ); }
#line 11052 "Parser/parser.cc"
    break;

  case 100: /* unary_expression: OFFSETOF '(' type_no_function ',' identifier ')'  */
#line 1058 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_offsetOf( (yyloc), (yyvsp[-3].decl), build_varref( (yylsp[-1]), (yyvsp[-1].tok) ) ) ); }
#line 11058 "Parser/parser.cc"
    break;

  case 101: /* unary_expression: TYPEID '(' type ')'  */
#line 1060 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "typeid name is currently unimplemented." ); (yyval.expr) = nullptr;
			// $$ = new ExpressionNode( build_offsetOf( $3, build_varref( $5 ) ) );
		}
#line 11067 "Parser/parser.cc"
    break;

  case 102: /* unary_expression: COUNTOF unary_expression  */
#line 1065 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::CountofExpr( (yyloc), new ast::TypeofType( maybeMoveBuild( (yyvsp[0].expr) ) ) ) ); }
#line 11073 "Parser/parser.cc"
    break;

  case 103: /* unary_expression: COUNTOF '(' type_no_function ')'  */
#line 1067 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::CountofExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl) ) ) ); }
#line 11079 "Parser/parser.cc"
    break;

  case 104: /* alignof_operator: ALIGNOF  */
#line 1071 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::AlignOf; }
#line 11085 "Parser/parser.cc"
    break;

  case 105: /* alignof_operator: __ALIGNOF  */
#line 1072 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::__AlignOf; }
#line 11091 "Parser/parser.cc"
    break;

  case 106: /* ptrref_operator: '*'  */
#line 1076 "Parser/parser.yy"
                                                                                                { (yyval.oper) = OperKinds::PointTo; }
#line 11097 "Parser/parser.cc"
    break;

  case 107: /* ptrref_operator: '&'  */
#line 1077 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::AddressOf; }
#line 11103 "Parser/parser.cc"
    break;

  case 108: /* ptrref_operator: ANDAND  */
#line 1079 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::And; }
#line 11109 "Parser/parser.cc"
    break;

  case 109: /* unary_operator: '+'  */
#line 1083 "Parser/parser.yy"
                                                                                                { (yyval.oper) = OperKinds::UnPlus; }
#line 11115 "Parser/parser.cc"
    break;

  case 110: /* unary_operator: '-'  */
#line 1084 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::UnMinus; }
#line 11121 "Parser/parser.cc"
    break;

  case 111: /* unary_operator: '!'  */
#line 1085 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::Neg; }
#line 11127 "Parser/parser.cc"
    break;

  case 112: /* unary_operator: '~'  */
#line 1086 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::BitNeg; }
#line 11133 "Parser/parser.cc"
    break;

  case 114: /* cast_expression: '(' type_no_function ')' cast_expression  */
#line 1092 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_cast( (yyloc), (yyvsp[-2].decl), (yyvsp[0].expr) ) ); }
#line 11139 "Parser/parser.cc"
    break;

  case 115: /* cast_expression: '(' aggregate_control '&' ')' cast_expression  */
#line 1094 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_keyword_cast( (yyloc), (yyvsp[-3].aggKey), (yyvsp[0].expr) ) ); }
#line 11145 "Parser/parser.cc"
    break;

  case 116: /* cast_expression: '(' aggregate_control '*' ')' cast_expression  */
#line 1096 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_keyword_cast( (yyloc), (yyvsp[-3].aggKey), (yyvsp[0].expr) ) ); }
#line 11151 "Parser/parser.cc"
    break;

  case 117: /* cast_expression: '(' VIRTUAL ')' cast_expression  */
#line 1098 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::VirtualCastExpr( (yyloc), maybeMoveBuild( (yyvsp[0].expr) ), nullptr ) ); }
#line 11157 "Parser/parser.cc"
    break;

  case 118: /* cast_expression: '(' VIRTUAL type_no_function ')' cast_expression  */
#line 1100 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::VirtualCastExpr( (yyloc), maybeMoveBuild( (yyvsp[0].expr) ), maybeMoveBuildType( (yyvsp[-2].decl) ) ) ); }
#line 11163 "Parser/parser.cc"
    break;

  case 119: /* cast_expression: '(' RETURN type_no_function ')' cast_expression  */
#line 1102 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_cast( (yyloc), (yyvsp[-2].decl), (yyvsp[0].expr), ast::ReturnCast ) ); }
#line 11169 "Parser/parser.cc"
    break;

  case 120: /* cast_expression: '(' COERCE type_no_function ')' cast_expression  */
#line 1104 "Parser/parser.yy"
                { SemanticError( (yyloc), "Coerce cast is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11175 "Parser/parser.cc"
    break;

  case 121: /* cast_expression: '(' qualifier_cast_list ')' cast_expression  */
#line 1106 "Parser/parser.yy"
                { SemanticError( (yyloc), "Qualifier cast is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11181 "Parser/parser.cc"
    break;

  case 129: /* exponential_expression: exponential_expression '\\' cast_expression  */
#line 1126 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Exp, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11187 "Parser/parser.cc"
    break;

  case 131: /* multiplicative_expression: multiplicative_expression '*' exponential_expression  */
#line 1132 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Mul, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11193 "Parser/parser.cc"
    break;

  case 132: /* multiplicative_expression: multiplicative_expression '/' exponential_expression  */
#line 1134 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Div, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11199 "Parser/parser.cc"
    break;

  case 133: /* multiplicative_expression: multiplicative_expression '%' exponential_expression  */
#line 1136 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Mod, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11205 "Parser/parser.cc"
    break;

  case 135: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 1142 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Plus, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11211 "Parser/parser.cc"
    break;

  case 136: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 1144 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Minus, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11217 "Parser/parser.cc"
    break;

  case 138: /* shift_expression: shift_expression LS additive_expression  */
#line 1150 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::LShift, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11223 "Parser/parser.cc"
    break;

  case 139: /* shift_expression: shift_expression RS additive_expression  */
#line 1152 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::RShift, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11229 "Parser/parser.cc"
    break;

  case 141: /* relational_expression: relational_expression '<' shift_expression  */
#line 1158 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::LThan, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11235 "Parser/parser.cc"
    break;

  case 142: /* relational_expression: relational_expression '>' shift_expression  */
#line 1160 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::GThan, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11241 "Parser/parser.cc"
    break;

  case 143: /* relational_expression: relational_expression LE shift_expression  */
#line 1162 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::LEThan, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11247 "Parser/parser.cc"
    break;

  case 144: /* relational_expression: relational_expression GE shift_expression  */
#line 1164 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::GEThan, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11253 "Parser/parser.cc"
    break;

  case 146: /* equality_expression: equality_expression EQ relational_expression  */
#line 1170 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Eq, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11259 "Parser/parser.cc"
    break;

  case 147: /* equality_expression: equality_expression NE relational_expression  */
#line 1172 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Neq, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11265 "Parser/parser.cc"
    break;

  case 149: /* AND_expression: AND_expression '&' equality_expression  */
#line 1178 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::BitAnd, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11271 "Parser/parser.cc"
    break;

  case 151: /* exclusive_OR_expression: exclusive_OR_expression '^' AND_expression  */
#line 1184 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::Xor, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11277 "Parser/parser.cc"
    break;

  case 153: /* inclusive_OR_expression: inclusive_OR_expression '|' exclusive_OR_expression  */
#line 1190 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), OperKinds::BitOr, (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11283 "Parser/parser.cc"
    break;

  case 155: /* logical_AND_expression: logical_AND_expression ANDAND inclusive_OR_expression  */
#line 1196 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_and_or( (yyloc), (yyvsp[-2].expr), (yyvsp[0].expr), ast::AndExpr ) ); }
#line 11289 "Parser/parser.cc"
    break;

  case 157: /* logical_OR_expression: logical_OR_expression OROR logical_AND_expression  */
#line 1202 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_and_or( (yyloc), (yyvsp[-2].expr), (yyvsp[0].expr), ast::OrExpr ) ); }
#line 11295 "Parser/parser.cc"
    break;

  case 159: /* conditional_expression: logical_OR_expression '?' comma_expression ':' conditional_expression  */
#line 1208 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_cond( (yyloc), (yyvsp[-4].expr), (yyvsp[-2].expr), (yyvsp[0].expr) ) ); }
#line 11301 "Parser/parser.cc"
    break;

  case 160: /* conditional_expression: logical_OR_expression '?' ':' conditional_expression  */
#line 1210 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_cond( (yyloc), (yyvsp[-3].expr), nullptr, (yyvsp[0].expr) ) ); }
#line 11307 "Parser/parser.cc"
    break;

  case 162: /* argument_expression_list_opt: %empty  */
#line 1219 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 11313 "Parser/parser.cc"
    break;

  case 165: /* argument_expression_list: argument_expression_list ',' argument_expression  */
#line 1227 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( (yyvsp[0].expr) ); }
#line 11319 "Parser/parser.cc"
    break;

  case 166: /* argument_expression: '?'  */
#line 1233 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_constantInteger( (yyloc), *new string( "2" ) ) ); }
#line 11325 "Parser/parser.cc"
    break;

  case 167: /* argument_expression: '?' identifier '=' assignment_expression  */
#line 1236 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[0].expr); }
#line 11331 "Parser/parser.cc"
    break;

  case 170: /* assignment_expression: unary_expression assignment_operator assignment_expression  */
#line 1244 "Parser/parser.yy"
                {
//			if ( $2 == OperKinds::AtAssn ) {
//				SemanticError( @$, "C @= assignment is currently unimplemented." ); $$ = nullptr;
//			} else {
				(yyval.expr) = new ExpressionNode( build_binary_val( (yyloc), (yyvsp[-1].oper), (yyvsp[-2].expr), (yyvsp[0].expr) ) );
//			} // if
		}
#line 11343 "Parser/parser.cc"
    break;

  case 171: /* assignment_expression: unary_expression '=' '{' initializer_list_opt comma_opt '}'  */
#line 1252 "Parser/parser.yy"
                { SemanticError( (yyloc), "Initializer assignment is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11349 "Parser/parser.cc"
    break;

  case 172: /* assignment_expression_opt: %empty  */
#line 1257 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 11355 "Parser/parser.cc"
    break;

  case 176: /* simple_assignment_operator: '='  */
#line 1267 "Parser/parser.yy"
                                                                                                { (yyval.oper) = OperKinds::Assign; }
#line 11361 "Parser/parser.cc"
    break;

  case 177: /* simple_assignment_operator: ATassign  */
#line 1268 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::AtAssn; }
#line 11367 "Parser/parser.cc"
    break;

  case 178: /* compound_assignment_operator: EXPassign  */
#line 1272 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::ExpAssn; }
#line 11373 "Parser/parser.cc"
    break;

  case 179: /* compound_assignment_operator: MULTassign  */
#line 1273 "Parser/parser.yy"
                                                                                { (yyval.oper) = OperKinds::MulAssn; }
#line 11379 "Parser/parser.cc"
    break;

  case 180: /* compound_assignment_operator: DIVassign  */
#line 1274 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::DivAssn; }
#line 11385 "Parser/parser.cc"
    break;

  case 181: /* compound_assignment_operator: MODassign  */
#line 1275 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::ModAssn; }
#line 11391 "Parser/parser.cc"
    break;

  case 182: /* compound_assignment_operator: PLUSassign  */
#line 1276 "Parser/parser.yy"
                                                                                { (yyval.oper) = OperKinds::PlusAssn; }
#line 11397 "Parser/parser.cc"
    break;

  case 183: /* compound_assignment_operator: MINUSassign  */
#line 1277 "Parser/parser.yy"
                                                                                { (yyval.oper) = OperKinds::MinusAssn; }
#line 11403 "Parser/parser.cc"
    break;

  case 184: /* compound_assignment_operator: LSassign  */
#line 1278 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::LSAssn; }
#line 11409 "Parser/parser.cc"
    break;

  case 185: /* compound_assignment_operator: RSassign  */
#line 1279 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::RSAssn; }
#line 11415 "Parser/parser.cc"
    break;

  case 186: /* compound_assignment_operator: ANDassign  */
#line 1280 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::AndAssn; }
#line 11421 "Parser/parser.cc"
    break;

  case 187: /* compound_assignment_operator: ERassign  */
#line 1281 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::ERAssn; }
#line 11427 "Parser/parser.cc"
    break;

  case 188: /* compound_assignment_operator: ORassign  */
#line 1282 "Parser/parser.yy"
                                                                                        { (yyval.oper) = OperKinds::OrAssn; }
#line 11433 "Parser/parser.cc"
    break;

  case 189: /* tuple: '[' ',' ']'  */
#line 1290 "Parser/parser.yy"
                { SemanticError( (yyloc), "Empty tuple is meaningless." ); (yyval.expr) = nullptr; }
#line 11439 "Parser/parser.cc"
    break;

  case 190: /* tuple: '[' assignment_expression ',' ']'  */
#line 1292 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_tuple( (yyloc), (yyvsp[-2].expr) ) ); }
#line 11445 "Parser/parser.cc"
    break;

  case 191: /* tuple: '[' '@' comma_opt ']'  */
#line 1294 "Parser/parser.yy"
                { SemanticError( (yyloc), "Eliding tuple element with '@' is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11451 "Parser/parser.cc"
    break;

  case 192: /* tuple: '[' assignment_expression ',' tuple_expression_list comma_opt ']'  */
#line 1296 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_tuple( (yyloc), (yyvsp[-4].expr)->set_last( (yyvsp[-2].expr) ) ) ); }
#line 11457 "Parser/parser.cc"
    break;

  case 193: /* tuple: '[' '@' ',' tuple_expression_list comma_opt ']'  */
#line 1298 "Parser/parser.yy"
                { SemanticError( (yyloc), "Eliding tuple element with '@' is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11463 "Parser/parser.cc"
    break;

  case 195: /* tuple_expression_list: '@'  */
#line 1304 "Parser/parser.yy"
                { SemanticError( (yyloc), "Eliding tuple element with '@' is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11469 "Parser/parser.cc"
    break;

  case 196: /* tuple_expression_list: tuple_expression_list ',' assignment_expression  */
#line 1306 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( (yyvsp[0].expr) ); }
#line 11475 "Parser/parser.cc"
    break;

  case 197: /* tuple_expression_list: tuple_expression_list ',' '@'  */
#line 1308 "Parser/parser.yy"
                { SemanticError( (yyloc), "Eliding tuple element with '@' is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 11481 "Parser/parser.cc"
    break;

  case 199: /* comma_expression: comma_expression ',' assignment_expression  */
#line 1314 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::CommaExpr( (yyloc), maybeMoveBuild( (yyvsp[-2].expr) ), maybeMoveBuild( (yyvsp[0].expr) ) ) ); }
#line 11487 "Parser/parser.cc"
    break;

  case 200: /* comma_expression_opt: %empty  */
#line 1319 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 11493 "Parser/parser.cc"
    break;

  case 215: /* statement: enable_disable_statement  */
#line 1340 "Parser/parser.yy"
                { SemanticError( (yyloc), "enable/disable statement is currently unimplemented." ); (yyval.stmt) = nullptr; }
#line 11499 "Parser/parser.cc"
    break;

  case 217: /* statement: DIRECTIVE  */
#line 1343 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_directive( (yyloc), (yyvsp[0].tok) ) ); }
#line 11505 "Parser/parser.cc"
    break;

  case 218: /* labelled_statement: identifier_or_type_name ':' attribute_list_opt statement  */
#line 1349 "Parser/parser.yy"
                { (yyval.stmt) = (yyvsp[0].stmt)->add_label( (yyloc), (yyvsp[-3].tok), (yyvsp[-1].decl) ); }
#line 11511 "Parser/parser.cc"
    break;

  case 219: /* labelled_statement: identifier_or_type_name ':' attribute_list_opt error  */
#line 1351 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "syntx error, label \"%s\" must be associated with a statement, "
						   "where a declaration, case, or default is not a statement.\n"
						   "Move the label or terminate with a semicolon.", (yyvsp[-3].tok).str->c_str() );
			(yyval.stmt) = nullptr;
		}
#line 11522 "Parser/parser.cc"
    break;

  case 220: /* compound_statement: '{' '}'  */
#line 1361 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_compound( (yyloc), (StatementNode *)0 ) ); }
#line 11528 "Parser/parser.cc"
    break;

  case 221: /* compound_statement: '{' recovery_push local_label_declaration_opt statement_decl_list '}'  */
#line 1366 "Parser/parser.yy"
                { leaveRecoveryScope(); (yyval.stmt) = new StatementNode( build_compound( (yyloc), (yyvsp[-1].stmt) ) ); }
#line 11534 "Parser/parser.cc"
    break;

  case 223: /* statement_decl_list: statement_decl_list statement_decl  */
#line 1372 "Parser/parser.yy"
                { assert( (yyvsp[-1].stmt) ); (yyvsp[-1].stmt)->set_last( (yyvsp[0].stmt) ); (yyval.stmt) = (yyvsp[-1].stmt); }
#line 11540 "Parser/parser.cc"
    break;

  case 224: /* statement_decl: attribute_list_opt declaration  */
#line 1377 "Parser/parser.yy"
                { distAttr( (yyvsp[-1].decl), (yyvsp[0].decl) ); (yyval.stmt) = new StatementNode( (yyvsp[0].decl) ); }
#line 11546 "Parser/parser.cc"
    break;

  case 225: /* statement_decl: attribute_list_opt EXTENSION declaration  */
#line 1379 "Parser/parser.yy"
                { distAttr( (yyvsp[-2].decl), (yyvsp[0].decl) ); distExt( (yyvsp[0].decl) ); (yyval.stmt) = new StatementNode( (yyvsp[0].decl) ); }
#line 11552 "Parser/parser.cc"
    break;

  case 226: /* statement_decl: attribute_list_opt function_definition  */
#line 1381 "Parser/parser.yy"
                { distAttr( (yyvsp[-1].decl), (yyvsp[0].decl) ); (yyval.stmt) = new StatementNode( setExtent( (yyvsp[0].decl), (yylsp[0]) ) ); }
#line 11558 "Parser/parser.cc"
    break;

  case 227: /* statement_decl: attribute_list_opt EXTENSION function_definition  */
#line 1383 "Parser/parser.yy"
                { distAttr( (yyvsp[-2].decl), (yyvsp[0].decl) ); distExt( (yyvsp[0].decl) ); (yyval.stmt) = new StatementNode( setExtent( (yyvsp[0].decl), (yylsp[0]) ) ); }
#line 11564 "Parser/parser.cc"
    break;

  case 228: /* statement_decl: attribute_list_opt statement  */
#line 1385 "Parser/parser.yy"
                { (yyval.stmt) = (yyvsp[0].stmt)->addQualifiers( (yyvsp[-1].decl) ); }
#line 11570 "Parser/parser.cc"
    break;

  case 229: /* statement_decl: error  */
#line 1387 "Parser/parser.yy"
                { if ( ! LSP::enabled ) YYABORT; recoverFromSyntaxError(); (yyval.stmt) = new StatementNode( build_expr( (yyloc), nullptr ) ); }
#line 11576 "Parser/parser.cc"
    break;

  case 230: /* statement_list_nodecl: attribute_list_opt statement  */
#line 1392 "Parser/parser.yy"
                { (yyval.stmt) = (yyvsp[0].stmt)->addQualifiers( (yyvsp[-1].decl) ); }
#line 11582 "Parser/parser.cc"
    break;

  case 231: /* statement_list_nodecl: statement_list_nodecl attribute_list_opt statement  */
#line 1394 "Parser/parser.yy"
                { assert( (yyvsp[-2].stmt) ); (yyvsp[-2].stmt)->set_last( (yyvsp[0].stmt)->addQualifiers( (yyvsp[-1].decl) ) ); (yyval.stmt) = (yyvsp[-2].stmt); }
#line 11588 "Parser/parser.cc"
    break;

  case 232: /* statement_list_nodecl: statement_list_nodecl error  */
#line 1396 "Parser/parser.yy"
                {
			syntaxError( (yyloc), "illegal syntax, declarations only allowed at the start of the switch body,"
						 " i.e., after the '{'." );
			recoverFromSyntaxError();
			(yyval.stmt) = (yyvsp[-1].stmt);
		}
#line 11599 "Parser/parser.cc"
    break;

  case 233: /* expression_statement: comma_expression_opt ';'  */
#line 1406 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_expr( (yyloc), (yyvsp[-1].expr) ) ); }
#line 11605 "Parser/parser.cc"
    break;

  case 234: /* selection_statement: IF '(' conditional_declaration ')' statement  */
#line 1436 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_if( (yyloc), (yyvsp[-2].ifctrl), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ), nullptr ) ); }
#line 11611 "Parser/parser.cc"
    break;

  case 235: /* selection_statement: IF '(' conditional_declaration ')' statement ELSE statement  */
#line 1438 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_if( (yyloc), (yyvsp[-4].ifctrl), maybe_build_compound( (yyloc), (yyvsp[-2].stmt) ), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11617 "Parser/parser.cc"
    break;

  case 236: /* selection_statement: SWITCH '(' comma_expression ')' case_clause  */
#line 1440 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_switch( (yyloc), true, (yyvsp[-2].expr), (yyvsp[0].clause) ) ); }
#line 11623 "Parser/parser.cc"
    break;

  case 237: /* selection_statement: SWITCH '(' comma_expression ')' '{' recovery_push declaration_list_opt switch_clause_list_opt '}'  */
#line 1442 "Parser/parser.yy"
                {
			leaveRecoveryScope();
			StatementNode *sw = new StatementNode( build_switch( (yyloc), true, (yyvsp[-6].expr), (yyvsp[-1].clause) ) );
			// The semantics of the declaration list is changed to include associated initialization, which is performed
			// *before* the transfer to the appropriate case clause by hoisting the declarations into a compound
			// statement around the switch.  Statements after the initial declaration list can never be executed, and
			// therefore, are removed from the grammar even though C allows it. The change also applies to choose
			// statement.
			(yyval.stmt) = (yyvsp[-2].decl) ? new StatementNode( build_compound( (yyloc), (new StatementNode( (yyvsp[-2].decl) ))->set_last( sw ) ) ) : sw;
		}
#line 11638 "Parser/parser.cc"
    break;

  case 238: /* selection_statement: SWITCH '(' comma_expression ')' '{' error '}'  */
#line 1453 "Parser/parser.yy"
                { SemanticError( (yyloc), "synatx error, declarations can only appear before the list of case clauses." ); (yyval.stmt) = nullptr; }
#line 11644 "Parser/parser.cc"
    break;

  case 239: /* selection_statement: CHOOSE '(' comma_expression ')' case_clause  */
#line 1455 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_switch( (yyloc), false, (yyvsp[-2].expr), (yyvsp[0].clause) ) ); }
#line 11650 "Parser/parser.cc"
    break;

  case 240: /* selection_statement: CHOOSE '(' comma_expression ')' '{' recovery_push declaration_list_opt switch_clause_list_opt '}'  */
#line 1457 "Parser/parser.yy"
                {
			leaveRecoveryScope();
			StatementNode *sw = new StatementNode( build_switch( (yyloc), false, (yyvsp[-6].expr), (yyvsp[-1].clause) ) );
			(yyval.stmt) = (yyvsp[-2].decl) ? new StatementNode( build_compound( (yyloc), (new StatementNode( (yyvsp[-2].decl) ))->set_last( sw ) ) ) : sw;
		}
#line 11660 "Parser/parser.cc"
    break;

  case 241: /* selection_statement: CHOOSE '(' comma_expression ')' '{' error '}'  */
#line 1463 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, declarations can only appear before the list of case clauses." ); (yyval.stmt) = nullptr; }
#line 11666 "Parser/parser.cc"
    break;

  case 242: /* conditional_declaration: comma_expression  */
#line 1468 "Parser/parser.yy"
                { (yyval.ifctrl) = new CondCtrl( nullptr, (yyvsp[0].expr) ); }
#line 11672 "Parser/parser.cc"
    break;

  case 243: /* conditional_declaration: c_declaration  */
#line 1470 "Parser/parser.yy"
                { (yyval.ifctrl) = new CondCtrl( (yyvsp[0].decl), nullptr ); }
#line 11678 "Parser/parser.cc"
    break;

  case 244: /* conditional_declaration: cfa_declaration  */
#line 1472 "Parser/parser.yy"
                { (yyval.ifctrl) = new CondCtrl( (yyvsp[0].decl), nullptr ); }
#line 11684 "Parser/parser.cc"
    break;

  case 245: /* conditional_declaration: declaration comma_expression  */
#line 1474 "Parser/parser.yy"
                { (yyval.ifctrl) = new CondCtrl( (yyvsp[-1].decl), (yyvsp[0].expr) ); }
#line 11690 "Parser/parser.cc"
    break;

  case 246: /* case_value: constant_expression  */
#line 1481 "Parser/parser.yy"
                                                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 11696 "Parser/parser.cc"
    break;

  case 247: /* case_value: constant_expression ELLIPSIS constant_expression  */
#line 1483 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::RangeExpr( (yyloc), maybeMoveBuild( (yyvsp[-2].expr) ), maybeMoveBuild( (yyvsp[0].expr) ) ) ); }
#line 11702 "Parser/parser.cc"
    break;

  case 249: /* case_value_list: case_value  */
#line 1488 "Parser/parser.yy"
                                                                                        { (yyval.clause) = new ClauseNode( build_case( (yyloc), (yyvsp[0].expr) ) ); }
#line 11708 "Parser/parser.cc"
    break;

  case 250: /* case_value_list: case_value_list ',' case_value  */
#line 1490 "Parser/parser.yy"
                                                                { (yyval.clause) = (yyvsp[-2].clause)->set_last( new ClauseNode( build_case( (yyloc), (yyvsp[0].expr) ) ) ); }
#line 11714 "Parser/parser.cc"
    break;

  case 251: /* case_label: CASE error  */
#line 1495 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, case list missing after case." ); (yyval.clause) = nullptr; }
#line 11720 "Parser/parser.cc"
    break;

  case 252: /* case_label: CASE case_value_list ':'  */
#line 1496 "Parser/parser.yy"
                                                                        { (yyval.clause) = (yyvsp[-1].clause); }
#line 11726 "Parser/parser.cc"
    break;

  case 253: /* case_label: CASE case_value_list error  */
#line 1498 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, colon missing after case list." ); (yyval.clause) = nullptr; }
#line 11732 "Parser/parser.cc"
    break;

  case 254: /* case_label: DEFAULT ':'  */
#line 1499 "Parser/parser.yy"
                                                                                { (yyval.clause) = new ClauseNode( build_default( (yyloc) ) ); }
#line 11738 "Parser/parser.cc"
    break;

  case 255: /* case_label: DEFAULT error  */
#line 1502 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, colon missing after default." ); (yyval.clause) = nullptr; }
#line 11744 "Parser/parser.cc"
    break;

  case 257: /* case_label_list: case_label_list case_label  */
#line 1507 "Parser/parser.yy"
                                                                { (yyval.clause) = (yyvsp[-1].clause)->set_last( (yyvsp[0].clause) ); }
#line 11750 "Parser/parser.cc"
    break;

  case 258: /* case_clause: case_label_list statement  */
#line 1511 "Parser/parser.yy"
                                                                        { (yyval.clause) = (yyvsp[-1].clause)->append_last_case( maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 11756 "Parser/parser.cc"
    break;

  case 259: /* switch_clause_list_opt: %empty  */
#line 1516 "Parser/parser.yy"
                { (yyval.clause) = nullptr; }
#line 11762 "Parser/parser.cc"
    break;

  case 261: /* switch_clause_list: case_label_list statement_list_nodecl  */
#line 1522 "Parser/parser.yy"
                { (yyval.clause) = (yyvsp[-1].clause)->append_last_case( new StatementNode( build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11768 "Parser/parser.cc"
    break;

  case 262: /* switch_clause_list: switch_clause_list case_label_list statement_list_nodecl  */
#line 1524 "Parser/parser.yy"
                { (yyval.clause) = (yyvsp[-2].clause)->set_last( (yyvsp[-1].clause)->append_last_case( new StatementNode( build_compound( (yyloc), (yyvsp[0].stmt) ) ) ) ); }
#line 11774 "Parser/parser.cc"
    break;

  case 263: /* iteration_statement: WHILE '(' ')' statement  */
#line 1529 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_while( (yyloc), new CondCtrl( nullptr, NEW_ONE ), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11780 "Parser/parser.cc"
    break;

  case 264: /* iteration_statement: WHILE '(' ')' statement ELSE statement  */
#line 1531 "Parser/parser.yy"
                {
			(yyval.stmt) = new StatementNode( build_while( (yyloc), new CondCtrl( nullptr, NEW_ONE ), maybe_build_compound( (yyloc), (yyvsp[-2].stmt) ) ) );
			SemanticWarning( (yyloc), Warning::SuperfluousElse );
		}
#line 11789 "Parser/parser.cc"
    break;

  case 265: /* iteration_statement: WHILE '(' conditional_declaration ')' statement  */
#line 1536 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_while( (yyloc), (yyvsp[-2].ifctrl), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11795 "Parser/parser.cc"
    break;

  case 266: /* iteration_statement: WHILE '(' conditional_declaration ')' statement ELSE statement  */
#line 1538 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_while( (yyloc), (yyvsp[-4].ifctrl), maybe_build_compound( (yyloc), (yyvsp[-2].stmt) ), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11801 "Parser/parser.cc"
    break;

  case 267: /* iteration_statement: DO statement WHILE '(' ')' ';'  */
#line 1540 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_do_while( (yyloc), NEW_ONE, maybe_build_compound( (yyloc), (yyvsp[-4].stmt) ) ) ); }
#line 11807 "Parser/parser.cc"
    break;

  case 268: /* iteration_statement: DO statement WHILE '(' ')' ELSE statement  */
#line 1542 "Parser/parser.yy"
                {
			(yyval.stmt) = new StatementNode( build_do_while( (yyloc), NEW_ONE, maybe_build_compound( (yyloc), (yyvsp[-5].stmt) ) ) );
			SemanticWarning( (yyloc), Warning::SuperfluousElse );
		}
#line 11816 "Parser/parser.cc"
    break;

  case 269: /* iteration_statement: DO statement WHILE '(' comma_expression ')' ';'  */
#line 1547 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_do_while( (yyloc), (yyvsp[-2].expr), maybe_build_compound( (yyloc), (yyvsp[-5].stmt) ) ) ); }
#line 11822 "Parser/parser.cc"
    break;

  case 270: /* iteration_statement: DO statement WHILE '(' comma_expression ')' ELSE statement  */
#line 1549 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_do_while( (yyloc), (yyvsp[-3].expr), maybe_build_compound( (yyloc), (yyvsp[-6].stmt) ), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11828 "Parser/parser.cc"
    break;

  case 271: /* iteration_statement: FOR '(' ')' statement  */
#line 1551 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_for( (yyloc), new ForCtrl( nullptr, nullptr, nullptr ), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11834 "Parser/parser.cc"
    break;

  case 272: /* iteration_statement: FOR '(' ')' statement ELSE statement  */
#line 1553 "Parser/parser.yy"
                {
			(yyval.stmt) = new StatementNode( build_for( (yyloc), new ForCtrl( nullptr, nullptr, nullptr ), maybe_build_compound( (yyloc), (yyvsp[-2].stmt) ) ) );
			SemanticWarning( (yyloc), Warning::SuperfluousElse );
		}
#line 11843 "Parser/parser.cc"
    break;

  case 273: /* iteration_statement: FOR '(' for_control_expression_list ')' statement  */
#line 1558 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_for( (yyloc), (yyvsp[-2].forctrl), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11849 "Parser/parser.cc"
    break;

  case 274: /* iteration_statement: FOR '(' for_control_expression_list ')' statement ELSE statement  */
#line 1560 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_for( (yyloc), (yyvsp[-4].forctrl), maybe_build_compound( (yyloc), (yyvsp[-2].stmt) ), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 11855 "Parser/parser.cc"
    break;

  case 276: /* for_control_expression_list: for_control_expression_list ':' for_control_expression  */
#line 1570 "Parser/parser.yy"
                {
			(yyvsp[-2].forctrl)->init->set_last( (yyvsp[0].forctrl)->init );
			if ( (yyvsp[-2].forctrl)->condition ) {
				if ( (yyvsp[0].forctrl)->condition ) {
					(yyvsp[-2].forctrl)->condition->expr.reset( new ast::LogicalExpr( (yyloc), (yyvsp[-2].forctrl)->condition->expr.release(), (yyvsp[0].forctrl)->condition->expr.release(), ast::AndExpr ) );
				} // if
			} else (yyvsp[-2].forctrl)->condition = (yyvsp[0].forctrl)->condition;
			if ( (yyvsp[-2].forctrl)->change ) {
				if ( (yyvsp[0].forctrl)->change ) {
					(yyvsp[-2].forctrl)->change->expr.reset( new ast::CommaExpr( (yyloc), (yyvsp[-2].forctrl)->change->expr.release(), (yyvsp[0].forctrl)->change->expr.release() ) );
				} // if
			} else (yyvsp[-2].forctrl)->change = (yyvsp[0].forctrl)->change;
			(yyval.forctrl) = (yyvsp[-2].forctrl);
		}
#line 11874 "Parser/parser.cc"
    break;

  case 277: /* for_control_expression: ';' comma_expression_opt ';' comma_expression_opt  */
#line 1588 "Parser/parser.yy"
                { (yyval.forctrl) = new ForCtrl( nullptr, (yyvsp[-2].expr), (yyvsp[0].expr) ); }
#line 11880 "Parser/parser.cc"
    break;

  case 278: /* for_control_expression: comma_expression ';' comma_expression_opt ';' comma_expression_opt  */
#line 1590 "Parser/parser.yy"
                {
			(yyval.forctrl) = new ForCtrl( (yyvsp[-4].expr) ? new StatementNode( new ast::ExprStmt( (yyloc), maybeMoveBuild( (yyvsp[-4].expr) ) ) ) : nullptr, (yyvsp[-2].expr), (yyvsp[0].expr) );
		}
#line 11888 "Parser/parser.cc"
    break;

  case 279: /* for_control_expression: declaration comma_expression_opt ';' comma_expression_opt  */
#line 1594 "Parser/parser.yy"
                { (yyval.forctrl) = new ForCtrl( new StatementNode( (yyvsp[-3].decl) ), (yyvsp[-2].expr), (yyvsp[0].expr) ); }
#line 11894 "Parser/parser.cc"
    break;

  case 280: /* for_control_expression: '@' ';' comma_expression  */
#line 1597 "Parser/parser.yy"
                { (yyval.forctrl) = new ForCtrl( nullptr, (yyvsp[0].expr), nullptr ); }
#line 11900 "Parser/parser.cc"
    break;

  case 281: /* for_control_expression: '@' ';' comma_expression ';' comma_expression  */
#line 1599 "Parser/parser.yy"
                { (yyval.forctrl) = new ForCtrl( nullptr, (yyvsp[-2].expr), (yyvsp[0].expr) ); }
#line 11906 "Parser/parser.cc"
    break;

  case 282: /* for_control_expression: comma_expression  */
#line 1602 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[0].expr), new string( DeclarationNode::anonymous.newName() ), NEW_ZERO, OperKinds::LThan, (yyvsp[0].expr)->clone(), NEW_ONE ); }
#line 11912 "Parser/parser.cc"
    break;

  case 283: /* for_control_expression: updown comma_expression  */
#line 1604 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[0].expr), new string( DeclarationNode::anonymous.newName() ), UPDOWN( (yyvsp[-1].oper), NEW_ZERO, (yyvsp[0].expr)->clone() ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), NEW_ZERO ), NEW_ONE ); }
#line 11918 "Parser/parser.cc"
    break;

  case 284: /* for_control_expression: comma_expression updownS comma_expression  */
#line 1607 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), new string( DeclarationNode::anonymous.newName() ), UPDOWN( (yyvsp[-1].oper), (yyvsp[-2].expr)->clone(), (yyvsp[0].expr) ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), (yyvsp[-2].expr)->clone() ), NEW_ONE ); }
#line 11924 "Parser/parser.cc"
    break;

  case 285: /* for_control_expression: '@' updownS comma_expression  */
#line 1609 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::LThan || (yyvsp[-1].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[0].expr), new string( DeclarationNode::anonymous.newName() ), (yyvsp[0].expr)->clone(), (yyvsp[-1].oper), nullptr, NEW_ONE );
		}
#line 11933 "Parser/parser.cc"
    break;

  case 286: /* for_control_expression: comma_expression updownS '@'  */
#line 1614 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::LThan || (yyvsp[-1].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
			else { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
		}
#line 11942 "Parser/parser.cc"
    break;

  case 287: /* for_control_expression: comma_expression updownS comma_expression '~' comma_expression  */
#line 1620 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-4].expr), new string( DeclarationNode::anonymous.newName() ), UPDOWN( (yyvsp[-3].oper), (yyvsp[-4].expr)->clone(), (yyvsp[-2].expr) ), (yyvsp[-3].oper), UPDOWN( (yyvsp[-3].oper), (yyvsp[-2].expr)->clone(), (yyvsp[-4].expr)->clone() ), (yyvsp[0].expr) ); }
#line 11948 "Parser/parser.cc"
    break;

  case 288: /* for_control_expression: '@' updownS comma_expression '~' comma_expression  */
#line 1622 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::LThan || (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), new string( DeclarationNode::anonymous.newName() ), (yyvsp[-2].expr)->clone(), (yyvsp[-3].oper), nullptr, (yyvsp[0].expr) );
		}
#line 11957 "Parser/parser.cc"
    break;

  case 289: /* for_control_expression: comma_expression updownS '@' '~' comma_expression  */
#line 1627 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::LThan || (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
			else { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
		}
#line 11966 "Parser/parser.cc"
    break;

  case 290: /* for_control_expression: comma_expression updownS comma_expression '~' '@'  */
#line 1632 "Parser/parser.yy"
                { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
#line 11972 "Parser/parser.cc"
    break;

  case 291: /* for_control_expression: '@' updownS '@'  */
#line 1634 "Parser/parser.yy"
                { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
#line 11978 "Parser/parser.cc"
    break;

  case 292: /* for_control_expression: '@' updownS comma_expression '~' '@'  */
#line 1636 "Parser/parser.yy"
                { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
#line 11984 "Parser/parser.cc"
    break;

  case 293: /* for_control_expression: comma_expression updownS '@' '~' '@'  */
#line 1638 "Parser/parser.yy"
                { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
#line 11990 "Parser/parser.cc"
    break;

  case 294: /* for_control_expression: '@' updownS '@' '~' '@'  */
#line 1640 "Parser/parser.yy"
                { SemanticError( (yyloc), MISSING_ANON_FIELD ); (yyval.forctrl) = nullptr; }
#line 11996 "Parser/parser.cc"
    break;

  case 295: /* for_control_expression: comma_expression ';' comma_expression  */
#line 1645 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[0].expr), (yyvsp[-2].expr), NEW_ZERO, OperKinds::LThan, (yyvsp[0].expr)->clone(), NEW_ONE ); }
#line 12002 "Parser/parser.cc"
    break;

  case 296: /* for_control_expression: comma_expression ';' updown comma_expression  */
#line 1647 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[0].expr), (yyvsp[-3].expr), UPDOWN( (yyvsp[-1].oper), NEW_ZERO, (yyvsp[0].expr)->clone() ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), NEW_ZERO ), NEW_ONE ); }
#line 12008 "Parser/parser.cc"
    break;

  case 297: /* for_control_expression: comma_expression ';' comma_expression updownS comma_expression  */
#line 1650 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), (yyvsp[-4].expr), UPDOWN( (yyvsp[-1].oper), (yyvsp[-2].expr)->clone(), (yyvsp[0].expr) ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), (yyvsp[-2].expr)->clone() ), NEW_ONE ); }
#line 12014 "Parser/parser.cc"
    break;

  case 298: /* for_control_expression: comma_expression ';' '@' updownS comma_expression  */
#line 1652 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::LThan || (yyvsp[-1].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[0].expr), (yyvsp[-4].expr), (yyvsp[0].expr)->clone(), (yyvsp[-1].oper), nullptr, NEW_ONE );
		}
#line 12023 "Parser/parser.cc"
    break;

  case 299: /* for_control_expression: comma_expression ';' comma_expression updownS '@'  */
#line 1657 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::GThan || (yyvsp[-1].oper) == OperKinds::GEThan ) { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
			else if ( (yyvsp[-1].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), "illegal syntax, equality with missing high value is meaningless. Use \"~\"." ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), (yyvsp[-4].expr), (yyvsp[-2].expr)->clone(), (yyvsp[-1].oper), nullptr, NEW_ONE );
		}
#line 12033 "Parser/parser.cc"
    break;

  case 300: /* for_control_expression: comma_expression ';' '@' updownS '@'  */
#line 1663 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, missing low/high value for ascending/descending range so index is uninitialized." ); (yyval.forctrl) = nullptr; }
#line 12039 "Parser/parser.cc"
    break;

  case 301: /* for_control_expression: comma_expression ';' comma_expression updownEq comma_expression  */
#line 1666 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), (yyvsp[-4].expr), UPDOWN( (yyvsp[-1].oper), (yyvsp[-2].expr)->clone(), (yyvsp[0].expr) ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), (yyvsp[-2].expr)->clone() ), NEW_ONE ); }
#line 12045 "Parser/parser.cc"
    break;

  case 302: /* for_control_expression: comma_expression ';' comma_expression updownS comma_expression '~' comma_expression  */
#line 1669 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-4].expr), (yyvsp[-6].expr), UPDOWN( (yyvsp[-3].oper), (yyvsp[-4].expr)->clone(), (yyvsp[-2].expr) ), (yyvsp[-3].oper), UPDOWN( (yyvsp[-3].oper), (yyvsp[-2].expr)->clone(), (yyvsp[-4].expr)->clone() ), (yyvsp[0].expr) ); }
#line 12051 "Parser/parser.cc"
    break;

  case 303: /* for_control_expression: comma_expression ';' '@' updownS comma_expression '~' comma_expression  */
#line 1671 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::LThan || (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), (yyvsp[-6].expr), (yyvsp[-2].expr)->clone(), (yyvsp[-3].oper), nullptr, (yyvsp[0].expr) );
		}
#line 12060 "Parser/parser.cc"
    break;

  case 304: /* for_control_expression: comma_expression ';' comma_expression updownS '@' '~' comma_expression  */
#line 1676 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::GThan || (yyvsp[-3].oper) == OperKinds::GEThan ) { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
			else if ( (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), "illegal syntax, equality with missing high value is meaningless. Use \"~\"." ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-4].expr), (yyvsp[-6].expr), (yyvsp[-4].expr)->clone(), (yyvsp[-3].oper), nullptr, (yyvsp[0].expr) );
		}
#line 12070 "Parser/parser.cc"
    break;

  case 305: /* for_control_expression: comma_expression ';' comma_expression updownS comma_expression '~' '@'  */
#line 1682 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-4].expr), (yyvsp[-6].expr), UPDOWN( (yyvsp[-3].oper), (yyvsp[-4].expr)->clone(), (yyvsp[-2].expr) ), (yyvsp[-3].oper), UPDOWN( (yyvsp[-3].oper), (yyvsp[-2].expr)->clone(), (yyvsp[-4].expr)->clone() ), nullptr ); }
#line 12076 "Parser/parser.cc"
    break;

  case 306: /* for_control_expression: comma_expression ';' '@' updownS comma_expression '~' '@'  */
#line 1684 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::LThan || (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].expr), (yyvsp[-6].expr), (yyvsp[-2].expr)->clone(), (yyvsp[-3].oper), nullptr, nullptr );
		}
#line 12085 "Parser/parser.cc"
    break;

  case 307: /* for_control_expression: comma_expression ';' comma_expression updownS '@' '~' '@'  */
#line 1689 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::GThan || (yyvsp[-3].oper) == OperKinds::GEThan ) { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
			else if ( (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), "illegal syntax, equality with missing high value is meaningless. Use \"~\"." ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-4].expr), (yyvsp[-6].expr), (yyvsp[-4].expr)->clone(), (yyvsp[-3].oper), nullptr, nullptr );
		}
#line 12095 "Parser/parser.cc"
    break;

  case 308: /* for_control_expression: comma_expression ';' '@' updownS '@' '~' '@'  */
#line 1695 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, missing low/high value for ascending/descending range so index is uninitialized." ); (yyval.forctrl) = nullptr; }
#line 12101 "Parser/parser.cc"
    break;

  case 309: /* for_control_expression: declaration comma_expression  */
#line 1698 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-1].decl), NEW_ZERO, OperKinds::LThan, (yyvsp[0].expr), NEW_ONE ); }
#line 12107 "Parser/parser.cc"
    break;

  case 310: /* for_control_expression: declaration updown comma_expression  */
#line 1700 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-2].decl), UPDOWN( (yyvsp[-1].oper), NEW_ZERO, (yyvsp[0].expr) ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), NEW_ZERO ), NEW_ONE ); }
#line 12113 "Parser/parser.cc"
    break;

  case 311: /* for_control_expression: declaration comma_expression updownS comma_expression  */
#line 1703 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-3].decl), UPDOWN( (yyvsp[-1].oper), (yyvsp[-2].expr)->clone(), (yyvsp[0].expr) ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), (yyvsp[-2].expr)->clone() ), NEW_ONE ); }
#line 12119 "Parser/parser.cc"
    break;

  case 312: /* for_control_expression: declaration '@' updownS comma_expression  */
#line 1705 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::LThan || (yyvsp[-1].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-3].decl), (yyvsp[0].expr), (yyvsp[-1].oper), nullptr, NEW_ONE );
		}
#line 12128 "Parser/parser.cc"
    break;

  case 313: /* for_control_expression: declaration comma_expression updownS '@'  */
#line 1710 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::GThan || (yyvsp[-1].oper) == OperKinds::GEThan ) { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
			else if ( (yyvsp[-1].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), "illegal syntax, equality with missing high value is meaningless. Use \"~\"." ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-3].decl), (yyvsp[-2].expr), (yyvsp[-1].oper), nullptr, NEW_ONE );
		}
#line 12138 "Parser/parser.cc"
    break;

  case 314: /* for_control_expression: declaration comma_expression updownEq comma_expression  */
#line 1717 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-3].decl), UPDOWN( (yyvsp[-1].oper), (yyvsp[-2].expr)->clone(), (yyvsp[0].expr) ), (yyvsp[-1].oper), UPDOWN( (yyvsp[-1].oper), (yyvsp[0].expr)->clone(), (yyvsp[-2].expr)->clone() ), NEW_ONE ); }
#line 12144 "Parser/parser.cc"
    break;

  case 315: /* for_control_expression: declaration comma_expression updownS comma_expression '~' comma_expression  */
#line 1720 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-5].decl), UPDOWN( (yyvsp[-3].oper), (yyvsp[-4].expr), (yyvsp[-2].expr) ), (yyvsp[-3].oper), UPDOWN( (yyvsp[-3].oper), (yyvsp[-2].expr)->clone(), (yyvsp[-4].expr)->clone() ), (yyvsp[0].expr) ); }
#line 12150 "Parser/parser.cc"
    break;

  case 316: /* for_control_expression: declaration '@' updownS comma_expression '~' comma_expression  */
#line 1722 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::LThan || (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-5].decl), (yyvsp[-2].expr), (yyvsp[-3].oper), nullptr, (yyvsp[0].expr) );
		}
#line 12159 "Parser/parser.cc"
    break;

  case 317: /* for_control_expression: declaration comma_expression updownS '@' '~' comma_expression  */
#line 1727 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::GThan || (yyvsp[-3].oper) == OperKinds::GEThan ) { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
			else if ( (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), "illegal syntax, equality with missing high value is meaningless. Use \"~\"." ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-5].decl), (yyvsp[-4].expr), (yyvsp[-3].oper), nullptr, (yyvsp[0].expr) );
		}
#line 12169 "Parser/parser.cc"
    break;

  case 318: /* for_control_expression: declaration comma_expression updownS comma_expression '~' '@'  */
#line 1733 "Parser/parser.yy"
                { (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-5].decl), UPDOWN( (yyvsp[-3].oper), (yyvsp[-4].expr), (yyvsp[-2].expr) ), (yyvsp[-3].oper), UPDOWN( (yyvsp[-3].oper), (yyvsp[-2].expr)->clone(), (yyvsp[-4].expr)->clone() ), nullptr ); }
#line 12175 "Parser/parser.cc"
    break;

  case 319: /* for_control_expression: declaration '@' updownS comma_expression '~' '@'  */
#line 1735 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::LThan || (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), MISSING_LOW ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-5].decl), (yyvsp[-2].expr), (yyvsp[-3].oper), nullptr, nullptr );
		}
#line 12184 "Parser/parser.cc"
    break;

  case 320: /* for_control_expression: declaration comma_expression updownS '@' '~' '@'  */
#line 1740 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].oper) == OperKinds::GThan || (yyvsp[-3].oper) == OperKinds::GEThan ) { SemanticError( (yyloc), MISSING_HIGH ); (yyval.forctrl) = nullptr; }
			else if ( (yyvsp[-3].oper) == OperKinds::LEThan ) { SemanticError( (yyloc), "illegal syntax, equality with missing high value is meaningless. Use \"~\"." ); (yyval.forctrl) = nullptr; }
			else (yyval.forctrl) = forCtrl( (yyloc), (yyvsp[-5].decl), (yyvsp[-4].expr), (yyvsp[-3].oper), nullptr, nullptr );
		}
#line 12194 "Parser/parser.cc"
    break;

  case 321: /* for_control_expression: declaration '@' updownS '@' '~' '@'  */
#line 1746 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, missing low/high value for ascending/descending range so index is uninitialized." ); (yyval.forctrl) = nullptr; }
#line 12200 "Parser/parser.cc"
    break;

  case 322: /* for_control_expression: comma_expression ';' type_type_specifier  */
#line 1749 "Parser/parser.yy"
                {
			(yyval.forctrl) = enumRangeCtrl( (yyvsp[-2].expr), OperKinds::LEThan, new ExpressionNode( new ast::TypeExpr( (yyloc), (yyvsp[0].decl)->clone()->buildType() ) ), (yyvsp[0].decl) );
		}
#line 12208 "Parser/parser.cc"
    break;

  case 323: /* for_control_expression: comma_expression ';' updown enum_key  */
#line 1753 "Parser/parser.yy"
                {
			if ( (yyvsp[-1].oper) == OperKinds::GThan ) {
				SemanticError( (yyloc), "all enumeration ranges are equal (all values). Add an equal, e.g., ~=, -~=." ); (yyval.forctrl) = nullptr;
				(yyvsp[-1].oper) = OperKinds::GEThan;
			} // if
			(yyval.forctrl) = enumRangeCtrl( (yyvsp[-3].expr), (yyvsp[-1].oper), new ExpressionNode( new ast::TypeExpr( (yyloc), (yyvsp[0].decl)->clone()->buildType() ) ), (yyvsp[0].decl) );
		}
#line 12220 "Parser/parser.cc"
    break;

  case 324: /* enum_key: type_name  */
#line 1764 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[0].type)->symbolic.name, "enum_type_nobody 1" );
			(yyval.decl) = DeclarationNode::newEnum( (yyvsp[0].type)->symbolic.name, nullptr, false, false );
		}
#line 12229 "Parser/parser.cc"
    break;

  case 325: /* enum_key: ENUM identifier  */
#line 1769 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[0].tok), "enum_type_nobody 2" );
			(yyval.decl) = DeclarationNode::newEnum( (yyvsp[0].tok), nullptr, false, false );
		}
#line 12238 "Parser/parser.cc"
    break;

  case 326: /* enum_key: ENUM type_name  */
#line 1774 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[0].type)->symbolic.name, "enum_type_nobody 3" );
			(yyval.decl) = DeclarationNode::newEnum( (yyvsp[0].type)->symbolic.name, nullptr, false, false );
		}
#line 12247 "Parser/parser.cc"
    break;

  case 327: /* updown: ErangeUpLt  */
#line 1785 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::LThan; }
#line 12253 "Parser/parser.cc"
    break;

  case 328: /* updown: ErangeDownGt  */
#line 1787 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::GThan; }
#line 12259 "Parser/parser.cc"
    break;

  case 329: /* updown: ErangeUpLe  */
#line 1789 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::LEThan; }
#line 12265 "Parser/parser.cc"
    break;

  case 330: /* updown: ErangeDownGe  */
#line 1791 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::GEThan; }
#line 12271 "Parser/parser.cc"
    break;

  case 331: /* updownS: '~'  */
#line 1796 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::LThan; }
#line 12277 "Parser/parser.cc"
    break;

  case 333: /* updownEq: ErangeEq  */
#line 1802 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::Eq; }
#line 12283 "Parser/parser.cc"
    break;

  case 334: /* updownEq: ErangeNe  */
#line 1804 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::Neq; }
#line 12289 "Parser/parser.cc"
    break;

  case 335: /* updownEq: ErangeDownEq  */
#line 1806 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::Eq; }
#line 12295 "Parser/parser.cc"
    break;

  case 336: /* updownEq: ErangeDownNe  */
#line 1808 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::Neq; }
#line 12301 "Parser/parser.cc"
    break;

  case 337: /* jump_statement: GOTO identifier_or_type_name ';'  */
#line 1813 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), (yyvsp[-1].tok), ast::BranchStmt::Goto ) ); }
#line 12307 "Parser/parser.cc"
    break;

  case 338: /* jump_statement: GOTO '*' comma_expression ';'  */
#line 1817 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_computedgoto( (yyvsp[-1].expr) ) ); }
#line 12313 "Parser/parser.cc"
    break;

  case 339: /* jump_statement: FALLTHROUGH ';'  */
#line 1820 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), ast::BranchStmt::FallThrough ) ); }
#line 12319 "Parser/parser.cc"
    break;

  case 340: /* jump_statement: FALLTHROUGH identifier_or_type_name ';'  */
#line 1822 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), (yyvsp[-1].tok), ast::BranchStmt::FallThrough ) ); }
#line 12325 "Parser/parser.cc"
    break;

  case 341: /* jump_statement: FALLTHROUGH DEFAULT ';'  */
#line 1824 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), ast::BranchStmt::FallThroughDefault ) ); }
#line 12331 "Parser/parser.cc"
    break;

  case 342: /* jump_statement: CONTINUE ';'  */
#line 1827 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), ast::BranchStmt::Continue ) ); }
#line 12337 "Parser/parser.cc"
    break;

  case 343: /* jump_statement: CONTINUE identifier_or_type_name ';'  */
#line 1831 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), (yyvsp[-1].tok), ast::BranchStmt::Continue ) ); }
#line 12343 "Parser/parser.cc"
    break;

  case 344: /* jump_statement: BREAK ';'  */
#line 1834 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), ast::BranchStmt::Break ) ); }
#line 12349 "Parser/parser.cc"
    break;

  case 345: /* jump_statement: BREAK identifier_or_type_name ';'  */
#line 1838 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_branch( (yyloc), (yyvsp[-1].tok), ast::BranchStmt::Break ) ); }
#line 12355 "Parser/parser.cc"
    break;

  case 346: /* jump_statement: RETURN comma_expression_opt ';'  */
#line 1840 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_return( (yyloc), (yyvsp[-1].expr) ) ); }
#line 12361 "Parser/parser.cc"
    break;

  case 347: /* jump_statement: RETURN '{' initializer_list_opt comma_opt '}' ';'  */
#line 1842 "Parser/parser.yy"
                { SemanticError( (yyloc), "Initializer return is currently unimplemented." ); (yyval.stmt) = nullptr; }
#line 12367 "Parser/parser.cc"
    break;

  case 348: /* jump_statement: SUSPEND ';'  */
#line 1844 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_suspend( (yyloc), nullptr, ast::SuspendStmt::None ) ); }
#line 12373 "Parser/parser.cc"
    break;

  case 349: /* jump_statement: SUSPEND compound_statement  */
#line 1846 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_suspend( (yyloc), (yyvsp[0].stmt), ast::SuspendStmt::None ) ); }
#line 12379 "Parser/parser.cc"
    break;

  case 350: /* jump_statement: SUSPEND COROUTINE ';'  */
#line 1848 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_suspend( (yyloc), nullptr, ast::SuspendStmt::Coroutine ) ); }
#line 12385 "Parser/parser.cc"
    break;

  case 351: /* jump_statement: SUSPEND COROUTINE compound_statement  */
#line 1850 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_suspend( (yyloc), (yyvsp[0].stmt), ast::SuspendStmt::Coroutine ) ); }
#line 12391 "Parser/parser.cc"
    break;

  case 352: /* jump_statement: SUSPEND GENERATOR ';'  */
#line 1852 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_suspend( (yyloc), nullptr, ast::SuspendStmt::Generator ) ); }
#line 12397 "Parser/parser.cc"
    break;

  case 353: /* jump_statement: SUSPEND GENERATOR compound_statement  */
#line 1854 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_suspend( (yyloc), (yyvsp[0].stmt), ast::SuspendStmt::Generator ) ); }
#line 12403 "Parser/parser.cc"
    break;

  case 354: /* jump_statement: THROW assignment_expression_opt ';'  */
#line 1856 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_throw( (yyloc), (yyvsp[-1].expr) ) ); }
#line 12409 "Parser/parser.cc"
    break;

  case 355: /* jump_statement: THROWRESUME assignment_expression_opt ';'  */
#line 1858 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_resume( (yyloc), (yyvsp[-1].expr) ) ); }
#line 12415 "Parser/parser.cc"
    break;

  case 356: /* jump_statement: THROWRESUME assignment_expression_opt AT assignment_expression ';'  */
#line 1860 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_resume_at( (yyvsp[-3].expr), (yyvsp[-1].expr) ) ); }
#line 12421 "Parser/parser.cc"
    break;

  case 357: /* with_statement: WITH '(' type_list ')' statement  */
#line 1865 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_with( (yyloc), (yyvsp[-2].expr), (yyvsp[0].stmt) ) ); }
#line 12427 "Parser/parser.cc"
    break;

  case 358: /* mutex_statement: MUTEX '(' argument_expression_list_opt ')' statement  */
#line 1871 "Parser/parser.yy"
                {
			if ( ! (yyvsp[-2].expr) ) { SemanticError( (yyloc), "illegal syntax, mutex argument list cannot be empty." ); (yyval.stmt) = nullptr; }
			(yyval.stmt) = new StatementNode( build_mutex( (yyloc), (yyvsp[-2].expr), (yyvsp[0].stmt) ) );
		}
#line 12436 "Parser/parser.cc"
    break;

  case 359: /* when_clause: WHEN '(' comma_expression ')'  */
#line 1878 "Parser/parser.yy"
                                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 12442 "Parser/parser.cc"
    break;

  case 360: /* when_clause_opt: %empty  */
#line 1883 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 12448 "Parser/parser.cc"
    break;

  case 363: /* cast_expression_list: cast_expression_list ',' cast_expression  */
#line 1890 "Parser/parser.yy"
                { SemanticError( (yyloc), "List of mutex member is currently unimplemented." ); (yyval.expr) = nullptr; }
#line 12454 "Parser/parser.cc"
    break;

  case 364: /* timeout: TIMEOUT '(' comma_expression ')'  */
#line 1894 "Parser/parser.yy"
                                                                { (yyval.expr) = (yyvsp[-1].expr); }
#line 12460 "Parser/parser.cc"
    break;

  case 367: /* waitfor: WAITFOR '(' cast_expression ')'  */
#line 1903 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 12466 "Parser/parser.cc"
    break;

  case 368: /* waitfor: WAITFOR '(' cast_expression_list ':' argument_expression_list_opt ')'  */
#line 1905 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-3].expr)->set_last( (yyvsp[-1].expr) ); }
#line 12472 "Parser/parser.cc"
    break;

  case 369: /* wor_waitfor_clause: when_clause_opt waitfor statement  */
#line 1911 "Parser/parser.yy"
                { (yyval.wfs) = build_waitfor( (yyloc), new ast::WaitForStmt( (yyloc) ), (yyvsp[-2].expr), (yyvsp[-1].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12478 "Parser/parser.cc"
    break;

  case 370: /* wor_waitfor_clause: wor_waitfor_clause wor when_clause_opt waitfor statement  */
#line 1913 "Parser/parser.yy"
                { (yyval.wfs) = build_waitfor( (yyloc), (yyvsp[-4].wfs), (yyvsp[-2].expr), (yyvsp[-1].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12484 "Parser/parser.cc"
    break;

  case 371: /* wor_waitfor_clause: wor_waitfor_clause wor when_clause_opt ELSE statement  */
#line 1915 "Parser/parser.yy"
                { (yyval.wfs) = build_waitfor_else( (yyloc), (yyvsp[-4].wfs), (yyvsp[-2].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12490 "Parser/parser.cc"
    break;

  case 372: /* wor_waitfor_clause: wor_waitfor_clause wor when_clause_opt timeout statement  */
#line 1917 "Parser/parser.yy"
                { (yyval.wfs) = build_waitfor_timeout( (yyloc), (yyvsp[-4].wfs), (yyvsp[-2].expr), (yyvsp[-1].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12496 "Parser/parser.cc"
    break;

  case 373: /* wor_waitfor_clause: wor_waitfor_clause wor when_clause_opt timeout statement wor ELSE statement  */
#line 1920 "Parser/parser.yy"
                { SemanticError( (yyloc), "illegal syntax, else clause must be conditional after timeout or timeout never triggered." ); (yyval.wfs) = nullptr; }
#line 12502 "Parser/parser.cc"
    break;

  case 374: /* wor_waitfor_clause: wor_waitfor_clause wor when_clause_opt timeout statement wor when_clause ELSE statement  */
#line 1922 "Parser/parser.yy"
                { (yyval.wfs) = build_waitfor_else( (yyloc), build_waitfor_timeout( (yyloc), (yyvsp[-8].wfs), (yyvsp[-6].expr), (yyvsp[-5].expr), maybe_build_compound( (yyloc), (yyvsp[-4].stmt) ) ), (yyvsp[-2].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12508 "Parser/parser.cc"
    break;

  case 375: /* waitfor_statement: wor_waitfor_clause  */
#line 1927 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( (yyvsp[0].wfs) ); }
#line 12514 "Parser/parser.cc"
    break;

  case 378: /* waituntil: WAITUNTIL '(' comma_expression ')'  */
#line 1937 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 12520 "Parser/parser.cc"
    break;

  case 379: /* waituntil_clause: when_clause_opt waituntil statement  */
#line 1942 "Parser/parser.yy"
                { (yyval.wucn) = build_waituntil_clause( (yyloc), (yyvsp[-2].expr), (yyvsp[-1].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12526 "Parser/parser.cc"
    break;

  case 380: /* waituntil_clause: '(' wor_waituntil_clause ')'  */
#line 1944 "Parser/parser.yy"
                { (yyval.wucn) = (yyvsp[-1].wucn); }
#line 12532 "Parser/parser.cc"
    break;

  case 381: /* wand_waituntil_clause: waituntil_clause  */
#line 1949 "Parser/parser.yy"
                { (yyval.wucn) = (yyvsp[0].wucn); }
#line 12538 "Parser/parser.cc"
    break;

  case 382: /* wand_waituntil_clause: waituntil_clause wand wand_waituntil_clause  */
#line 1951 "Parser/parser.yy"
                { (yyval.wucn) = new ast::WaitUntilStmt::ClauseNode( ast::WaitUntilStmt::ClauseNode::Op::AND, (yyvsp[-2].wucn), (yyvsp[0].wucn) ); }
#line 12544 "Parser/parser.cc"
    break;

  case 383: /* wor_waituntil_clause: wand_waituntil_clause  */
#line 1956 "Parser/parser.yy"
                { (yyval.wucn) = (yyvsp[0].wucn); }
#line 12550 "Parser/parser.cc"
    break;

  case 384: /* wor_waituntil_clause: wor_waituntil_clause wor wand_waituntil_clause  */
#line 1958 "Parser/parser.yy"
                { (yyval.wucn) = new ast::WaitUntilStmt::ClauseNode( ast::WaitUntilStmt::ClauseNode::Op::OR, (yyvsp[-2].wucn), (yyvsp[0].wucn) ); }
#line 12556 "Parser/parser.cc"
    break;

  case 385: /* wor_waituntil_clause: wor_waituntil_clause wor when_clause_opt ELSE statement  */
#line 1960 "Parser/parser.yy"
                { (yyval.wucn) = new ast::WaitUntilStmt::ClauseNode( ast::WaitUntilStmt::ClauseNode::Op::LEFT_OR, (yyvsp[-4].wucn), build_waituntil_else( (yyloc), (yyvsp[-2].expr), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 12562 "Parser/parser.cc"
    break;

  case 386: /* waituntil_statement: wor_waituntil_clause  */
#line 1965 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_waituntil_stmt( (yyloc), (yyvsp[0].wucn) ) );	}
#line 12568 "Parser/parser.cc"
    break;

  case 387: /* corun_statement: CORUN statement  */
#line 1970 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_corun( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12574 "Parser/parser.cc"
    break;

  case 388: /* cofor_statement: COFOR '(' for_control_expression_list ')' statement  */
#line 1975 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_cofor( (yyloc), (yyvsp[-2].forctrl), maybe_build_compound( (yyloc), (yyvsp[0].stmt) ) ) ); }
#line 12580 "Parser/parser.cc"
    break;

  case 389: /* exception_statement: TRY compound_statement handler_clause  */
#line 1980 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_try( (yyloc), (yyvsp[-1].stmt), (yyvsp[0].clause), nullptr ) ); }
#line 12586 "Parser/parser.cc"
    break;

  case 390: /* exception_statement: TRY compound_statement finally_clause  */
#line 1982 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_try( (yyloc), (yyvsp[-1].stmt), nullptr, (yyvsp[0].clause) ) ); }
#line 12592 "Parser/parser.cc"
    break;

  case 391: /* exception_statement: TRY compound_statement handler_clause finally_clause  */
#line 1984 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_try( (yyloc), (yyvsp[-2].stmt), (yyvsp[-1].clause), (yyvsp[0].clause) ) ); }
#line 12598 "Parser/parser.cc"
    break;

  case 392: /* handler_clause: handler_key '(' exception_declaration handler_predicate_opt ')' compound_statement  */
#line 1989 "Parser/parser.yy"
                { (yyval.clause) = new ClauseNode( build_catch( (yyloc), (yyvsp[-5].except_kind), (yyvsp[-3].decl), (yyvsp[-2].expr), (yyvsp[0].stmt) ) ); }
#line 12604 "Parser/parser.cc"
    break;

  case 393: /* handler_clause: handler_clause handler_key '(' exception_declaration handler_predicate_opt ')' compound_statement  */
#line 1991 "Parser/parser.yy"
                { (yyval.clause) = (yyvsp[-6].clause)->set_last( new ClauseNode( build_catch( (yyloc), (yyvsp[-5].except_kind), (yyvsp[-3].decl), (yyvsp[-2].expr), (yyvsp[0].stmt) ) ) ); }
#line 12610 "Parser/parser.cc"
    break;

  case 394: /* handler_predicate_opt: %empty  */
#line 1996 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 12616 "Parser/parser.cc"
    break;

  case 395: /* handler_predicate_opt: ';' conditional_expression  */
#line 1997 "Parser/parser.yy"
                                                                { (yyval.expr) = (yyvsp[0].expr); }
#line 12622 "Parser/parser.cc"
    break;

  case 396: /* handler_key: CATCH  */
#line 2001 "Parser/parser.yy"
                                                                                        { (yyval.except_kind) = ast::Terminate; }
#line 12628 "Parser/parser.cc"
    break;

  case 397: /* handler_key: RECOVER  */
#line 2002 "Parser/parser.yy"
                                                                                        { (yyval.except_kind) = ast::Terminate; }
#line 12634 "Parser/parser.cc"
    break;

  case 398: /* handler_key: CATCHRESUME  */
#line 2003 "Parser/parser.yy"
                                                                                { (yyval.except_kind) = ast::Resume; }
#line 12640 "Parser/parser.cc"
    break;

  case 399: /* handler_key: FIXUP  */
#line 2004 "Parser/parser.yy"
                                                                                        { (yyval.except_kind) = ast::Resume; }
#line 12646 "Parser/parser.cc"
    break;

  case 400: /* finally_clause: FINALLY compound_statement  */
#line 2008 "Parser/parser.yy"
                                                                        { (yyval.clause) = new ClauseNode( build_finally( (yyloc), (yyvsp[0].stmt) ) ); }
#line 12652 "Parser/parser.cc"
    break;

  case 402: /* exception_declaration: type_specifier_nobody declarator  */
#line 2015 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addType( (yyvsp[-1].decl) ); }
#line 12658 "Parser/parser.cc"
    break;

  case 403: /* exception_declaration: type_specifier_nobody variable_abstract_declarator  */
#line 2017 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addType( (yyvsp[-1].decl) ); }
#line 12664 "Parser/parser.cc"
    break;

  case 404: /* exception_declaration: cfa_abstract_declarator_tuple identifier  */
#line 2019 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-1].decl)->addName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 12670 "Parser/parser.cc"
    break;

  case 409: /* asm_statement: ASM asm_volatile_opt '(' string_literal ')' ';'  */
#line 2034 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_asm( (yyloc), (yyvsp[-4].is_volatile), (yyvsp[-2].expr), nullptr ) ); }
#line 12676 "Parser/parser.cc"
    break;

  case 410: /* asm_statement: ASM asm_volatile_opt '(' string_literal ':' asm_operands_opt ')' ';'  */
#line 2036 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_asm( (yyloc), (yyvsp[-6].is_volatile), (yyvsp[-4].expr), (yyvsp[-2].expr) ) ); }
#line 12682 "Parser/parser.cc"
    break;

  case 411: /* asm_statement: ASM asm_volatile_opt '(' string_literal ':' asm_operands_opt ':' asm_operands_opt ')' ';'  */
#line 2038 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_asm( (yyloc), (yyvsp[-8].is_volatile), (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].expr) ) ); }
#line 12688 "Parser/parser.cc"
    break;

  case 412: /* asm_statement: ASM asm_volatile_opt '(' string_literal ':' asm_operands_opt ':' asm_operands_opt ':' asm_clobbers_list_opt ')' ';'  */
#line 2040 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_asm( (yyloc), (yyvsp[-10].is_volatile), (yyvsp[-8].expr), (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].expr) ) ); }
#line 12694 "Parser/parser.cc"
    break;

  case 413: /* asm_statement: ASM asm_volatile_opt GOTO '(' string_literal ':' ':' asm_operands_opt ':' asm_clobbers_list_opt ':' asm_label_list ')' ';'  */
#line 2042 "Parser/parser.yy"
                { (yyval.stmt) = new StatementNode( build_asm( (yyloc), (yyvsp[-12].is_volatile), (yyvsp[-9].expr), nullptr, (yyvsp[-6].expr), (yyvsp[-4].expr), (yyvsp[-2].labels) ) ); }
#line 12700 "Parser/parser.cc"
    break;

  case 414: /* asm_volatile_opt: %empty  */
#line 2047 "Parser/parser.yy"
                { (yyval.is_volatile) = false; }
#line 12706 "Parser/parser.cc"
    break;

  case 415: /* asm_volatile_opt: VOLATILE  */
#line 2049 "Parser/parser.yy"
                { (yyval.is_volatile) = true; }
#line 12712 "Parser/parser.cc"
    break;

  case 416: /* asm_operands_opt: %empty  */
#line 2054 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 12718 "Parser/parser.cc"
    break;

  case 419: /* asm_operands_list: asm_operands_list ',' asm_operand  */
#line 2061 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( (yyvsp[0].expr) ); }
#line 12724 "Parser/parser.cc"
    break;

  case 420: /* asm_operand: string_literal '(' constant_expression ')'  */
#line 2066 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::AsmExpr( (yyloc), "", maybeMoveBuild( (yyvsp[-3].expr) ), maybeMoveBuild( (yyvsp[-1].expr) ) ) ); }
#line 12730 "Parser/parser.cc"
    break;

  case 421: /* asm_operand: '[' IDENTIFIER ']' string_literal '(' constant_expression ')'  */
#line 2068 "Parser/parser.yy"
                {
			(yyval.expr) = new ExpressionNode( new ast::AsmExpr( (yyloc), *(yyvsp[-5].tok).str, maybeMoveBuild( (yyvsp[-3].expr) ), maybeMoveBuild( (yyvsp[-1].expr) ) ) );
			delete (yyvsp[-5].tok).str;
		}
#line 12739 "Parser/parser.cc"
    break;

  case 422: /* asm_clobbers_list_opt: %empty  */
#line 2076 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 12745 "Parser/parser.cc"
    break;

  case 423: /* asm_clobbers_list_opt: string_literal  */
#line 2078 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[0].expr); }
#line 12751 "Parser/parser.cc"
    break;

  case 424: /* asm_clobbers_list_opt: asm_clobbers_list_opt ',' string_literal  */
#line 2080 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( (yyvsp[0].expr) ); }
#line 12757 "Parser/parser.cc"
    break;

  case 425: /* asm_label_list: identifier_or_type_name  */
#line 2085 "Parser/parser.yy"
                { (yyval.labels) = new LabelNode(); (yyval.labels)->labels.emplace_back( (yyloc), *(yyvsp[0].tok) ); delete (yyvsp[0].tok); }
#line 12763 "Parser/parser.cc"
    break;

  case 426: /* asm_label_list: asm_label_list ',' identifier_or_type_name  */
#line 2087 "Parser/parser.yy"
                { (yyval.labels) = (yyvsp[-2].labels); (yyvsp[-2].labels)->labels.emplace_back( (yyloc), *(yyvsp[0].tok) ); delete (yyvsp[0].tok); }
#line 12769 "Parser/parser.cc"
    break;

  case 427: /* declaration_list_opt: %empty  */
#line 2094 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 12775 "Parser/parser.cc"
    break;

  case 429: /* declaration_list: attribute_list_opt declaration  */
#line 2100 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 12781 "Parser/parser.cc"
    break;

  case 430: /* declaration_list: declaration_list declaration  */
#line 2102 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->set_last( (yyvsp[0].decl) ); }
#line 12787 "Parser/parser.cc"
    break;

  case 431: /* KR_parameter_list_opt: %empty  */
#line 2107 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 12793 "Parser/parser.cc"
    break;

  case 433: /* KR_parameter_list: c_declaration ';'  */
#line 2113 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 12799 "Parser/parser.cc"
    break;

  case 434: /* KR_parameter_list: KR_parameter_list c_declaration ';'  */
#line 2115 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[-1].decl) ); }
#line 12805 "Parser/parser.cc"
    break;

  case 441: /* declaration: c_declaration ';'  */
#line 2135 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-1].decl), (yyloc) ); }
#line 12811 "Parser/parser.cc"
    break;

  case 442: /* declaration: cfa_declaration ';'  */
#line 2137 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-1].decl), (yyloc) ); }
#line 12817 "Parser/parser.cc"
    break;

  case 444: /* static_assert: STATICASSERT '(' constant_expression ',' string_literal ')'  */
#line 2143 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStaticAssert( (yyvsp[-3].expr), maybeMoveBuild( (yyvsp[-1].expr) ) ); }
#line 12823 "Parser/parser.cc"
    break;

  case 445: /* static_assert: STATICASSERT '(' constant_expression ')'  */
#line 2145 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStaticAssert( (yyvsp[-1].expr), build_constantStr( (yyloc), *new string( "\"\"" ) ) ); }
#line 12829 "Parser/parser.cc"
    break;

  case 449: /* cfa_declaration: type_declaring_list  */
#line 2163 "Parser/parser.yy"
                { SemanticError( (yyloc), "otype declaration is currently unimplemented." ); (yyval.decl) = nullptr; }
#line 12835 "Parser/parser.cc"
    break;

  case 451: /* cfa_variable_declaration: cfa_variable_specifier initializer_opt  */
#line 2169 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addInitializer( (yyvsp[0].init) ); }
#line 12841 "Parser/parser.cc"
    break;

  case 452: /* cfa_variable_declaration: declaration_qualifier_list cfa_variable_specifier initializer_opt  */
#line 2173 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) )->addInitializer( (yyvsp[0].init) ); }
#line 12847 "Parser/parser.cc"
    break;

  case 453: /* cfa_variable_declaration: cfa_variable_declaration pop ',' push identifier_or_type_name initializer_opt  */
#line 2175 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-5].decl)->set_last( setNameLoc( (yyvsp[-5].decl)->cloneType( (yyvsp[-1].tok) ), (yylsp[-1]) )->addInitializer( (yyvsp[0].init) ) ); }
#line 12853 "Parser/parser.cc"
    break;

  case 454: /* cfa_variable_specifier: cfa_abstract_declarator_no_tuple identifier_or_type_name asm_name_opt  */
#line 2182 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addName( (yyvsp[-1].tok) ), (yylsp[-1]) )->addAsmName( (yyvsp[0].decl) ); }
#line 12859 "Parser/parser.cc"
    break;

  case 455: /* cfa_variable_specifier: cfa_abstract_tuple identifier_or_type_name asm_name_opt  */
#line 2184 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addName( (yyvsp[-1].tok) ), (yylsp[-1]) )->addAsmName( (yyvsp[0].decl) ); }
#line 12865 "Parser/parser.cc"
    break;

  case 456: /* cfa_variable_specifier: multi_array_dimension cfa_abstract_tuple identifier_or_type_name asm_name_opt  */
#line 2186 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addNewArray( (yyvsp[-3].decl) )->addName( (yyvsp[-1].tok) ), (yylsp[-1]) )->addAsmName( (yyvsp[0].decl) ); }
#line 12871 "Parser/parser.cc"
    break;

  case 457: /* cfa_variable_specifier: multi_array_dimension type_qualifier_list cfa_abstract_tuple identifier_or_type_name asm_name_opt  */
#line 2188 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addNewArray( (yyvsp[-4].decl) )->addQualifiers( (yyvsp[-3].decl) )->addName( (yyvsp[-1].tok) ), (yylsp[-1]) )->addAsmName( (yyvsp[0].decl) ); }
#line 12877 "Parser/parser.cc"
    break;

  case 458: /* cfa_variable_specifier: cfa_function_return asm_name_opt  */
#line 2196 "Parser/parser.yy"
                { SemanticError( (yyloc), "tuple-element declarations is currently unimplemented." ); (yyval.decl) = nullptr; }
#line 12883 "Parser/parser.cc"
    break;

  case 459: /* cfa_variable_specifier: type_qualifier_list cfa_function_return asm_name_opt  */
#line 2198 "Parser/parser.yy"
                { SemanticError( (yyloc), "tuple variable declaration is currently unimplemented." ); (yyval.decl) = nullptr; }
#line 12889 "Parser/parser.cc"
    break;

  case 461: /* cfa_function_declaration: type_qualifier_list cfa_function_specifier  */
#line 2204 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 12895 "Parser/parser.cc"
    break;

  case 462: /* cfa_function_declaration: declaration_qualifier_list cfa_function_specifier  */
#line 2206 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 12901 "Parser/parser.cc"
    break;

  case 463: /* cfa_function_declaration: declaration_qualifier_list type_qualifier_list cfa_function_specifier  */
#line 2208 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-2].decl) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 12907 "Parser/parser.cc"
    break;

  case 464: /* cfa_function_declaration: cfa_function_declaration ',' identifier_or_type_name '(' push cfa_parameter_list_ellipsis_opt pop ')'  */
#line 2210 "Parser/parser.yy"
                {
			// Append the return type at the start (left-hand-side) to each identifier in the list.
			DeclarationNode * ret = new DeclarationNode;
			ret->type = maybeCopy( (yyvsp[-7].decl)->type->base );
			(yyval.decl) = (yyvsp[-7].decl)->set_last( setNameLoc( DeclarationNode::newFunction( (yyvsp[-5].tok), ret, (yyvsp[-2].decl), nullptr ), (yylsp[-5]) ) );
		}
#line 12918 "Parser/parser.cc"
    break;

  case 465: /* cfa_function_specifier: '[' ']' identifier '(' push cfa_parameter_list_ellipsis_opt pop ')' attribute_list_opt  */
#line 2220 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newFunction( (yyvsp[-6].tok),  DeclarationNode::newTuple( nullptr ), (yyvsp[-3].decl), nullptr ), (yylsp[-6]) )->addQualifiers( (yyvsp[0].decl) ); }
#line 12924 "Parser/parser.cc"
    break;

  case 466: /* cfa_function_specifier: '[' ']' TYPEDEFname '(' push cfa_parameter_list_ellipsis_opt pop ')' attribute_list_opt  */
#line 2222 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newFunction( (yyvsp[-6].tok),  DeclarationNode::newTuple( nullptr ), (yyvsp[-3].decl), nullptr ), (yylsp[-6]) )->addQualifiers( (yyvsp[0].decl) ); }
#line 12930 "Parser/parser.cc"
    break;

  case 467: /* cfa_function_specifier: cfa_abstract_tuple identifier_or_type_name '(' push cfa_parameter_list_ellipsis_opt pop ')' attribute_list_opt  */
#line 2235 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newFunction( (yyvsp[-6].tok), (yyvsp[-7].decl), (yyvsp[-3].decl), nullptr ), (yylsp[-6]) )->addQualifiers( (yyvsp[0].decl) ); }
#line 12936 "Parser/parser.cc"
    break;

  case 468: /* cfa_function_specifier: cfa_function_return identifier_or_type_name '(' push cfa_parameter_list_ellipsis_opt pop ')' attribute_list_opt  */
#line 2237 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newFunction( (yyvsp[-6].tok), (yyvsp[-7].decl), (yyvsp[-3].decl), nullptr ), (yylsp[-6]) )->addQualifiers( (yyvsp[0].decl) ); }
#line 12942 "Parser/parser.cc"
    break;

  case 469: /* cfa_function_return: '[' cfa_parameter_list ']'  */
#line 2242 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTuple( (yyvsp[-1].decl) ); }
#line 12948 "Parser/parser.cc"
    break;

  case 470: /* cfa_function_return: '[' cfa_parameter_list ',' cfa_abstract_parameter_list ']'  */
#line 2245 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTuple( (yyvsp[-3].decl)->set_last( (yyvsp[-1].decl) ) ); }
#line 12954 "Parser/parser.cc"
    break;

  case 471: /* cfa_typedef_declaration: TYPEDEF attribute_list_opt cfa_variable_specifier  */
#line 2250 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[0].decl)->name, TYPEDEFname, "cfa_typedef_declaration 1" );
			(yyval.decl) = (yyvsp[0].decl)->addTypedef()->addQualifiers( (yyvsp[-1].decl) );
		}
#line 12963 "Parser/parser.cc"
    break;

  case 472: /* cfa_typedef_declaration: TYPEDEF attribute_list_opt cfa_function_specifier  */
#line 2255 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[0].decl)->name, TYPEDEFname, "cfa_typedef_declaration 2" );
			(yyval.decl) = (yyvsp[0].decl)->addTypedef()->addQualifiers( (yyvsp[-1].decl) );
		}
#line 12972 "Parser/parser.cc"
    break;

  case 473: /* cfa_typedef_declaration: cfa_typedef_declaration ',' attribute_list_opt identifier  */
#line 2260 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[0].tok), TYPEDEFname, "cfa_typedef_declaration 3" );
			(yyval.decl) = (yyvsp[-3].decl)->set_last( setNameLoc( (yyvsp[-3].decl)->cloneType( (yyvsp[0].tok) ), (yylsp[0]) )->addQualifiers( (yyvsp[-1].decl) ) );
		}
#line 12981 "Parser/parser.cc"
    break;

  case 474: /* typedef_declaration: TYPEDEF attribute_list_opt type_specifier declarator  */
#line 2271 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[0].decl)->name, TYPEDEFname, "typedef_declaration 1" );
			if ( (yyvsp[-1].decl)->type->forall || ((yyvsp[-1].decl)->type->kind == TypeData::Aggregate && (yyvsp[-1].decl)->type->aggregate.params) ) {
				SemanticError( (yyloc), "forall qualifier in typedef is currently unimplemented." ); (yyval.decl) = nullptr;
			} else (yyval.decl) = (yyvsp[0].decl)->addType( (yyvsp[-1].decl) )->addTypedef()->addQualifiers( (yyvsp[-2].decl) ); // watchout frees $3 and $4
		}
#line 12992 "Parser/parser.cc"
    break;

  case 475: /* typedef_declaration: typedef_declaration ',' attribute_list_opt declarator  */
#line 2278 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[0].decl)->name, TYPEDEFname, "typedef_declaration 2" );
			(yyval.decl) = (yyvsp[-3].decl)->set_last( (yyvsp[-3].decl)->cloneBaseType( (yyvsp[0].decl) )->addTypedef()->addQualifiers( (yyvsp[-1].decl) ) );
		}
#line 13001 "Parser/parser.cc"
    break;

  case 476: /* typedef_declaration: type_qualifier_list TYPEDEF type_specifier declarator  */
#line 2283 "Parser/parser.yy"
                { SemanticError( (yyloc), "Type qualifiers/specifiers before TYPEDEF is deprecated, move after TYPEDEF." ); (yyval.decl) = nullptr; }
#line 13007 "Parser/parser.cc"
    break;

  case 477: /* typedef_declaration: type_specifier TYPEDEF declarator  */
#line 2285 "Parser/parser.yy"
                { SemanticError( (yyloc), "Type qualifiers/specifiers before TYPEDEF is deprecated, move after TYPEDEF." ); (yyval.decl) = nullptr; }
#line 13013 "Parser/parser.cc"
    break;

  case 478: /* typedef_declaration: type_specifier TYPEDEF type_qualifier_list declarator  */
#line 2287 "Parser/parser.yy"
                { SemanticError( (yyloc), "Type qualifiers/specifiers before TYPEDEF is deprecated, move after TYPEDEF." ); (yyval.decl) = nullptr; }
#line 13019 "Parser/parser.cc"
    break;

  case 479: /* typedef_expression: TYPEDEF identifier '=' assignment_expression  */
#line 2293 "Parser/parser.yy"
                { SemanticError( (yyloc), "TYPEDEF expression is deprecated, use typeof(...) instead." ); (yyval.decl) = nullptr; }
#line 13025 "Parser/parser.cc"
    break;

  case 480: /* typedef_expression: typedef_expression ',' identifier '=' assignment_expression  */
#line 2295 "Parser/parser.yy"
                { SemanticError( (yyloc), "TYPEDEF expression is deprecated, use typeof(...) instead." ); (yyval.decl) = nullptr; }
#line 13031 "Parser/parser.cc"
    break;

  case 481: /* c_declaration: declaration_specifier declaring_list  */
#line 2300 "Parser/parser.yy"
                { (yyval.decl) = distTypeSpec( (yyvsp[-1].decl), (yyvsp[0].decl) ); }
#line 13037 "Parser/parser.cc"
    break;

  case 484: /* c_declaration: sue_declaration_specifier  */
#line 2304 "Parser/parser.yy"
                {
			assert( (yyvsp[0].decl)->type );
			if ( (yyvsp[0].decl)->type->qualifiers.any() ) {			// CV qualifiers ?
				SemanticError( (yyloc), "illegal syntax, useless type qualifier(s) in empty declaration." ); (yyval.decl) = nullptr;
			}
			// enums are never empty declarations because there must have at least one enumeration.
			if ( (yyvsp[0].decl)->type->kind == TypeData::AggregateInst && (yyvsp[0].decl)->storageClasses.any() ) { // storage class ?
				SemanticError( (yyloc), "illegal syntax, useless storage qualifier(s) in empty aggregate declaration." ); (yyval.decl) = nullptr;
			}
		}
#line 13052 "Parser/parser.cc"
    break;

  case 485: /* declaring_list: variable_declarator asm_name_opt initializer_opt  */
#line 2320 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addAsmName( (yyvsp[-1].decl) )->addInitializer( (yyvsp[0].init) ); }
#line 13058 "Parser/parser.cc"
    break;

  case 486: /* declaring_list: variable_type_redeclarator asm_name_opt initializer_opt  */
#line 2322 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addAsmName( (yyvsp[-1].decl) )->addInitializer( (yyvsp[0].init) ); }
#line 13064 "Parser/parser.cc"
    break;

  case 487: /* declaring_list: general_function_declarator asm_name_opt  */
#line 2325 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addAsmName( (yyvsp[0].decl) )->addInitializer( nullptr ); }
#line 13070 "Parser/parser.cc"
    break;

  case 488: /* declaring_list: general_function_declarator asm_name_opt '=' VOID  */
#line 2327 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addAsmName( (yyvsp[-2].decl) )->addInitializer( new InitializerNode( true ) ); }
#line 13076 "Parser/parser.cc"
    break;

  case 489: /* declaring_list: declaring_list ',' attribute_list_opt declarator asm_name_opt initializer_opt  */
#line 2330 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-5].decl)->set_last( (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addAsmName( (yyvsp[-1].decl) )->addInitializer( (yyvsp[0].init) ) ); }
#line 13082 "Parser/parser.cc"
    break;

  case 495: /* declaration_specifier: sue_declaration_specifier invalid_types  */
#line 2343 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "illegal syntax, expecting ';' at end of \"%s\" declaration.",
						   ast::AggregateDecl::aggrString( (yyvsp[-1].decl)->type->aggregate.kind ) );
			(yyval.decl) = nullptr;
		}
#line 13092 "Parser/parser.cc"
    break;

  case 508: /* type_qualifier_list_opt: %empty  */
#line 2386 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 13098 "Parser/parser.cc"
    break;

  case 510: /* type_qualifier_list: type_qualifier attribute_list_opt  */
#line 2397 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13104 "Parser/parser.cc"
    break;

  case 511: /* type_qualifier_list: type_qualifier_list type_qualifier attribute_list_opt  */
#line 2399 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13110 "Parser/parser.cc"
    break;

  case 512: /* type_qualifier: type_qualifier_name  */
#line 2404 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( (yyvsp[0].type) ); }
#line 13116 "Parser/parser.cc"
    break;

  case 513: /* type_qualifier_name: CONST  */
#line 2409 "Parser/parser.yy"
                { (yyval.type) = build_type_qualifier( ast::CV::Const ); }
#line 13122 "Parser/parser.cc"
    break;

  case 514: /* type_qualifier_name: RESTRICT  */
#line 2411 "Parser/parser.yy"
                { (yyval.type) = build_type_qualifier( ast::CV::Restrict ); }
#line 13128 "Parser/parser.cc"
    break;

  case 515: /* type_qualifier_name: VOLATILE  */
#line 2413 "Parser/parser.yy"
                { (yyval.type) = build_type_qualifier( ast::CV::Volatile ); }
#line 13134 "Parser/parser.cc"
    break;

  case 516: /* type_qualifier_name: ATOMIC  */
#line 2415 "Parser/parser.yy"
                { (yyval.type) = build_type_qualifier( ast::CV::Atomic ); }
#line 13140 "Parser/parser.cc"
    break;

  case 517: /* type_qualifier_name: forall  */
#line 2422 "Parser/parser.yy"
                { (yyval.type) = build_forall( (yyvsp[0].decl) ); }
#line 13146 "Parser/parser.cc"
    break;

  case 518: /* forall: FORALL '(' type_parameter_list ')'  */
#line 2427 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 13152 "Parser/parser.cc"
    break;

  case 520: /* declaration_qualifier_list: type_qualifier_list storage_class_list  */
#line 2433 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13158 "Parser/parser.cc"
    break;

  case 521: /* declaration_qualifier_list: declaration_qualifier_list type_qualifier_list storage_class_list  */
#line 2435 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13164 "Parser/parser.cc"
    break;

  case 522: /* storage_class_list: storage_class attribute_list_opt  */
#line 2445 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13170 "Parser/parser.cc"
    break;

  case 523: /* storage_class_list: storage_class_list storage_class attribute_list_opt  */
#line 2447 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13176 "Parser/parser.cc"
    break;

  case 524: /* storage_class: EXTERN  */
#line 2452 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStorageClass( ast::Storage::Extern ); }
#line 13182 "Parser/parser.cc"
    break;

  case 525: /* storage_class: STATIC  */
#line 2454 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStorageClass( ast::Storage::Static ); }
#line 13188 "Parser/parser.cc"
    break;

  case 526: /* storage_class: AUTO  */
#line 2456 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStorageClass( ast::Storage::Auto ); }
#line 13194 "Parser/parser.cc"
    break;

  case 527: /* storage_class: REGISTER  */
#line 2458 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStorageClass( ast::Storage::Register ); }
#line 13200 "Parser/parser.cc"
    break;

  case 528: /* storage_class: THREADLOCALGCC  */
#line 2460 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStorageClass( ast::Storage::ThreadLocalGcc ); }
#line 13206 "Parser/parser.cc"
    break;

  case 529: /* storage_class: THREADLOCALC11  */
#line 2462 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newStorageClass( ast::Storage::ThreadLocalC11 ); }
#line 13212 "Parser/parser.cc"
    break;

  case 530: /* storage_class: INLINE  */
#line 2465 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFuncSpecifier( ast::Function::Inline ); }
#line 13218 "Parser/parser.cc"
    break;

  case 531: /* storage_class: FORTRAN  */
#line 2467 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFuncSpecifier( ast::Function::Fortran ); }
#line 13224 "Parser/parser.cc"
    break;

  case 532: /* storage_class: NORETURN  */
#line 2469 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFuncSpecifier( ast::Function::Noreturn ); }
#line 13230 "Parser/parser.cc"
    break;

  case 533: /* basic_type_name: basic_type_name_type  */
#line 2474 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( (yyvsp[0].type) ); }
#line 13236 "Parser/parser.cc"
    break;

  case 534: /* basic_type_name_type: VOID  */
#line 2480 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Void ); }
#line 13242 "Parser/parser.cc"
    break;

  case 535: /* basic_type_name_type: BOOL  */
#line 2482 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Bool ); }
#line 13248 "Parser/parser.cc"
    break;

  case 536: /* basic_type_name_type: CHAR  */
#line 2484 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Char ); }
#line 13254 "Parser/parser.cc"
    break;

  case 537: /* basic_type_name_type: INT  */
#line 2486 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Int ); }
#line 13260 "Parser/parser.cc"
    break;

  case 538: /* basic_type_name_type: INT128  */
#line 2488 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Int128 ); }
#line 13266 "Parser/parser.cc"
    break;

  case 539: /* basic_type_name_type: UINT128  */
#line 2490 "Parser/parser.yy"
                { (yyval.type) = addType( build_basic_type( TypeData::Int128 ), build_signedness( TypeData::Unsigned ) ); }
#line 13272 "Parser/parser.cc"
    break;

  case 540: /* basic_type_name_type: FLOAT  */
#line 2492 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float ); }
#line 13278 "Parser/parser.cc"
    break;

  case 541: /* basic_type_name_type: DOUBLE  */
#line 2494 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Double ); }
#line 13284 "Parser/parser.cc"
    break;

  case 542: /* basic_type_name_type: FLOAT80  */
#line 2496 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float80 ); }
#line 13290 "Parser/parser.cc"
    break;

  case 543: /* basic_type_name_type: uuFLOAT128  */
#line 2498 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::uuFloat128 ); }
#line 13296 "Parser/parser.cc"
    break;

  case 544: /* basic_type_name_type: FLOAT16  */
#line 2500 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float16 ); }
#line 13302 "Parser/parser.cc"
    break;

  case 545: /* basic_type_name_type: FLOAT32  */
#line 2502 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float32 ); }
#line 13308 "Parser/parser.cc"
    break;

  case 546: /* basic_type_name_type: FLOAT32X  */
#line 2504 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float32x ); }
#line 13314 "Parser/parser.cc"
    break;

  case 547: /* basic_type_name_type: FLOAT64  */
#line 2506 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float64 ); }
#line 13320 "Parser/parser.cc"
    break;

  case 548: /* basic_type_name_type: FLOAT64X  */
#line 2508 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float64x ); }
#line 13326 "Parser/parser.cc"
    break;

  case 549: /* basic_type_name_type: FLOAT128  */
#line 2510 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float128 ); }
#line 13332 "Parser/parser.cc"
    break;

  case 550: /* basic_type_name_type: FLOAT128X  */
#line 2513 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float128x ); }
#line 13338 "Parser/parser.cc"
    break;

  case 551: /* basic_type_name_type: FLOAT32X4  */
#line 2515 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float32x4 ); }
#line 13344 "Parser/parser.cc"
    break;

  case 552: /* basic_type_name_type: FLOAT64X2  */
#line 2517 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Float64x2 ); }
#line 13350 "Parser/parser.cc"
    break;

  case 553: /* basic_type_name_type: SVFLOAT32  */
#line 2519 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Svfloat32 ); }
#line 13356 "Parser/parser.cc"
    break;

  case 554: /* basic_type_name_type: SVFLOAT64  */
#line 2521 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Svfloat64 ); }
#line 13362 "Parser/parser.cc"
    break;

  case 555: /* basic_type_name_type: SVBOOL  */
#line 2523 "Parser/parser.yy"
                { (yyval.type) = build_basic_type( TypeData::Svbool ); }
#line 13368 "Parser/parser.cc"
    break;

  case 556: /* basic_type_name_type: DECIMAL32  */
#line 2525 "Parser/parser.yy"
                { SemanticError( (yyloc), "_Decimal32 is currently unimplemented." ); (yyval.type) = nullptr; }
#line 13374 "Parser/parser.cc"
    break;

  case 557: /* basic_type_name_type: DECIMAL64  */
#line 2527 "Parser/parser.yy"
                { SemanticError( (yyloc), "_Decimal64 is currently unimplemented." ); (yyval.type) = nullptr; }
#line 13380 "Parser/parser.cc"
    break;

  case 558: /* basic_type_name_type: DECIMAL128  */
#line 2529 "Parser/parser.yy"
                { SemanticError( (yyloc), "_Decimal128 is currently unimplemented." ); (yyval.type) = nullptr; }
#line 13386 "Parser/parser.cc"
    break;

  case 559: /* basic_type_name_type: COMPLEX  */
#line 2531 "Parser/parser.yy"
                { (yyval.type) = build_complex_type( TypeData::Complex ); }
#line 13392 "Parser/parser.cc"
    break;

  case 560: /* basic_type_name_type: IMAGINARY  */
#line 2533 "Parser/parser.yy"
                { (yyval.type) = build_complex_type( TypeData::Imaginary ); }
#line 13398 "Parser/parser.cc"
    break;

  case 561: /* basic_type_name_type: SIGNED  */
#line 2535 "Parser/parser.yy"
                { (yyval.type) = build_signedness( TypeData::Signed ); }
#line 13404 "Parser/parser.cc"
    break;

  case 562: /* basic_type_name_type: UNSIGNED  */
#line 2537 "Parser/parser.yy"
                { (yyval.type) = build_signedness( TypeData::Unsigned ); }
#line 13410 "Parser/parser.cc"
    break;

  case 563: /* basic_type_name_type: SHORT  */
#line 2539 "Parser/parser.yy"
                { (yyval.type) = build_length( TypeData::Short ); }
#line 13416 "Parser/parser.cc"
    break;

  case 564: /* basic_type_name_type: LONG  */
#line 2541 "Parser/parser.yy"
                { (yyval.type) = build_length( TypeData::Long ); }
#line 13422 "Parser/parser.cc"
    break;

  case 565: /* basic_type_name_type: VA_LIST  */
#line 2543 "Parser/parser.yy"
                { (yyval.type) = build_builtin_type( TypeData::Valist ); }
#line 13428 "Parser/parser.cc"
    break;

  case 566: /* basic_type_name_type: AUTO_TYPE  */
#line 2545 "Parser/parser.yy"
                { (yyval.type) = build_builtin_type( TypeData::AutoType ); }
#line 13434 "Parser/parser.cc"
    break;

  case 568: /* vtable_opt: %empty  */
#line 2551 "Parser/parser.yy"
                { (yyval.type) = nullptr; }
#line 13440 "Parser/parser.cc"
    break;

  case 570: /* vtable: VTABLE '(' type_name ')' default_opt  */
#line 2557 "Parser/parser.yy"
                { (yyval.type) = build_vtable_type( (yyvsp[-2].type) ); }
#line 13446 "Parser/parser.cc"
    break;

  case 571: /* default_opt: %empty  */
#line 2562 "Parser/parser.yy"
                { (yyval.type) = nullptr; }
#line 13452 "Parser/parser.cc"
    break;

  case 572: /* default_opt: DEFAULT  */
#line 2564 "Parser/parser.yy"
                { SemanticError( (yyloc), "vtable default is currently unimplemented." ); (yyval.type) = nullptr; }
#line 13458 "Parser/parser.cc"
    break;

  case 574: /* basic_declaration_specifier: declaration_qualifier_list basic_type_specifier  */
#line 2571 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 13464 "Parser/parser.cc"
    break;

  case 575: /* basic_declaration_specifier: basic_declaration_specifier storage_class attribute_list_opt  */
#line 2573 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13470 "Parser/parser.cc"
    break;

  case 576: /* basic_declaration_specifier: basic_declaration_specifier storage_class type_qualifier_list  */
#line 2575 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13476 "Parser/parser.cc"
    break;

  case 577: /* basic_declaration_specifier: basic_declaration_specifier storage_class basic_type_specifier  */
#line 2577 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) )->addType( (yyvsp[-2].decl) ); }
#line 13482 "Parser/parser.cc"
    break;

  case 578: /* basic_type_specifier: direct_type attribute_list_opt  */
#line 2582 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13488 "Parser/parser.cc"
    break;

  case 579: /* basic_type_specifier: type_qualifier_list_opt indirect_type attribute_list  */
#line 2585 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13494 "Parser/parser.cc"
    break;

  case 580: /* basic_type_specifier: type_qualifier_list_opt indirect_type type_qualifier_list_opt  */
#line 2587 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13500 "Parser/parser.cc"
    break;

  case 582: /* direct_type: type_qualifier_list basic_type_name  */
#line 2593 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 13506 "Parser/parser.cc"
    break;

  case 583: /* direct_type: direct_type type_qualifier  */
#line 2595 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13512 "Parser/parser.cc"
    break;

  case 584: /* direct_type: direct_type basic_type_name  */
#line 2597 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addType( (yyvsp[0].decl) ); }
#line 13518 "Parser/parser.cc"
    break;

  case 585: /* indirect_type: TYPEOF '(' type ')'  */
#line 2602 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 13524 "Parser/parser.cc"
    break;

  case 586: /* indirect_type: TYPEOF '(' comma_expression ')'  */
#line 2604 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTypeof( (yyvsp[-1].expr) ); }
#line 13530 "Parser/parser.cc"
    break;

  case 587: /* indirect_type: BASETYPEOF '(' type ')'  */
#line 2606 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTypeof( new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[-1].decl) ) ) ), true ); }
#line 13536 "Parser/parser.cc"
    break;

  case 588: /* indirect_type: BASETYPEOF '(' comma_expression ')'  */
#line 2608 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTypeof( (yyvsp[-1].expr), true ); }
#line 13542 "Parser/parser.cc"
    break;

  case 589: /* indirect_type: ZERO_T  */
#line 2610 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( build_builtin_type( TypeData::Zero ) ); }
#line 13548 "Parser/parser.cc"
    break;

  case 590: /* indirect_type: ONE_T  */
#line 2612 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( build_builtin_type( TypeData::One ) ); }
#line 13554 "Parser/parser.cc"
    break;

  case 592: /* sue_declaration_specifier: declaration_qualifier_list sue_type_specifier  */
#line 2618 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 13560 "Parser/parser.cc"
    break;

  case 593: /* sue_declaration_specifier: sue_declaration_specifier storage_class  */
#line 2620 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13566 "Parser/parser.cc"
    break;

  case 594: /* sue_declaration_specifier: sue_declaration_specifier storage_class type_qualifier_list  */
#line 2622 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13572 "Parser/parser.cc"
    break;

  case 596: /* $@1: %empty  */
#line 2628 "Parser/parser.yy"
                { if ( (yyvsp[0].decl)->type != nullptr && (yyvsp[0].decl)->type->forall ) forall = true; }
#line 13578 "Parser/parser.cc"
    break;

  case 597: /* sue_type_specifier: type_qualifier_list $@1 elaborated_type  */
#line 2630 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 13584 "Parser/parser.cc"
    break;

  case 598: /* sue_type_specifier: sue_type_specifier type_qualifier  */
#line 2632 "Parser/parser.yy"
                {
			if ( (yyvsp[0].decl)->type != nullptr && (yyvsp[0].decl)->type->forall ) forall = true; // remember generic type
			(yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) );
		}
#line 13593 "Parser/parser.cc"
    break;

  case 600: /* sue_declaration_specifier_nobody: declaration_qualifier_list sue_type_specifier_nobody  */
#line 2641 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 13599 "Parser/parser.cc"
    break;

  case 601: /* sue_declaration_specifier_nobody: sue_declaration_specifier_nobody storage_class  */
#line 2643 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13605 "Parser/parser.cc"
    break;

  case 602: /* sue_declaration_specifier_nobody: sue_declaration_specifier_nobody storage_class type_qualifier_list  */
#line 2645 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13611 "Parser/parser.cc"
    break;

  case 604: /* sue_type_specifier_nobody: type_qualifier_list elaborated_type_nobody  */
#line 2651 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 13617 "Parser/parser.cc"
    break;

  case 605: /* sue_type_specifier_nobody: sue_type_specifier_nobody type_qualifier  */
#line 2653 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13623 "Parser/parser.cc"
    break;

  case 606: /* type_declaration_specifier: type_type_specifier attribute_list_opt  */
#line 2658 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13629 "Parser/parser.cc"
    break;

  case 607: /* type_declaration_specifier: declaration_qualifier_list type_type_specifier attribute_list_opt  */
#line 2660 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13635 "Parser/parser.cc"
    break;

  case 608: /* type_declaration_specifier: type_declaration_specifier storage_class attribute_list_opt  */
#line 2662 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13641 "Parser/parser.cc"
    break;

  case 609: /* type_declaration_specifier: type_declaration_specifier storage_class type_qualifier_list  */
#line 2664 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-1].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13647 "Parser/parser.cc"
    break;

  case 610: /* type_type_specifier: type_name  */
#line 2669 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( (yyvsp[0].type) ); }
#line 13653 "Parser/parser.cc"
    break;

  case 611: /* type_type_specifier: type_qualifier_list type_name  */
#line 2671 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( (yyvsp[0].type) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 13659 "Parser/parser.cc"
    break;

  case 612: /* type_type_specifier: type_type_specifier type_qualifier  */
#line 2673 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 13665 "Parser/parser.cc"
    break;

  case 613: /* type_name: TYPEDEFname  */
#line 2678 "Parser/parser.yy"
                { (yyval.type) = setTypeNameLoc( build_typedef( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 13671 "Parser/parser.cc"
    break;

  case 614: /* type_name: '.' TYPEDEFname  */
#line 2680 "Parser/parser.yy"
                { (yyval.type) = build_qualified_type( build_global_scope(), setTypeNameLoc( build_typedef( (yyvsp[0].tok) ), (yylsp[0]) ) ); }
#line 13677 "Parser/parser.cc"
    break;

  case 615: /* type_name: type_name '.' TYPEDEFname  */
#line 2682 "Parser/parser.yy"
                { (yyval.type) = build_qualified_type( (yyvsp[-2].type), setTypeNameLoc( build_typedef( (yyvsp[0].tok) ), (yylsp[0]) ) ); }
#line 13683 "Parser/parser.cc"
    break;

  case 617: /* type_name: '.' typegen_name  */
#line 2685 "Parser/parser.yy"
                { (yyval.type) = build_qualified_type( build_global_scope(), (yyvsp[0].type) ); }
#line 13689 "Parser/parser.cc"
    break;

  case 618: /* type_name: type_name '.' typegen_name  */
#line 2687 "Parser/parser.yy"
                { (yyval.type) = build_qualified_type( (yyvsp[-2].type), (yyvsp[0].type) ); }
#line 13695 "Parser/parser.cc"
    break;

  case 619: /* typegen_name: TYPEGENname  */
#line 2692 "Parser/parser.yy"
                { (yyval.type) = setTypeNameLoc( build_type_gen( (yyvsp[0].tok), nullptr ), (yylsp[0]) ); }
#line 13701 "Parser/parser.cc"
    break;

  case 620: /* typegen_name: TYPEGENname '(' ')'  */
#line 2694 "Parser/parser.yy"
                { (yyval.type) = setTypeNameLoc( build_type_gen( (yyvsp[-2].tok), nullptr ), (yylsp[-2]) ); }
#line 13707 "Parser/parser.cc"
    break;

  case 621: /* typegen_name: TYPEGENname '(' type_list ')'  */
#line 2696 "Parser/parser.yy"
                { (yyval.type) = setTypeNameLoc( build_type_gen( (yyvsp[-3].tok), (yyvsp[-1].expr) ), (yylsp[-3]) ); }
#line 13713 "Parser/parser.cc"
    break;

  case 626: /* $@2: %empty  */
#line 2713 "Parser/parser.yy"
                { forall = false; }
#line 13719 "Parser/parser.cc"
    break;

  case 627: /* aggregate_type: aggregate_key attribute_list_opt $@2 '{' field_declaration_list_opt '}' type_parameters_opt attribute_list_opt  */
#line 2715 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newAggregate( (yyvsp[-7].aggKey), nullptr, (yyvsp[-1].expr), (yyvsp[-3].decl), true ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-2]) ) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 13725 "Parser/parser.cc"
    break;

  case 628: /* $@3: %empty  */
#line 2717 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[-1].tok), forall || typedefTable.getEnclForall() ? TYPEGENname : TYPEDEFname, "aggregate_type: 1" );
			forall = false;								// reset
		}
#line 13734 "Parser/parser.cc"
    break;

  case 629: /* aggregate_type: aggregate_key attribute_list_opt identifier attribute_list_opt $@3 '{' field_declaration_list_opt '}' type_parameters_opt attribute_list_opt  */
#line 2722 "Parser/parser.yy"
                {
			(yyval.decl) = setAggrLocs( DeclarationNode::newAggregate( (yyvsp[-9].aggKey), (yyvsp[-7].tok), (yyvsp[-1].expr), (yyvsp[-3].decl), true ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-2]) ) )->addQualifiers( (yyvsp[-8].decl) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) );
		}
#line 13742 "Parser/parser.cc"
    break;

  case 630: /* $@4: %empty  */
#line 2726 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[-1].tok), forall || typedefTable.getEnclForall() ? TYPEGENname : TYPEDEFname, "aggregate_type: 2" );
			forall = false;								// reset
		}
#line 13751 "Parser/parser.cc"
    break;

  case 631: /* aggregate_type: aggregate_key attribute_list_opt TYPEDEFname attribute_list_opt $@4 '{' field_declaration_list_opt '}' type_parameters_opt attribute_list_opt  */
#line 2731 "Parser/parser.yy"
                {
			DeclarationNode::newFromTypeData( build_typedef( (yyvsp[-7].tok) ) );
			(yyval.decl) = setAggrLocs( DeclarationNode::newAggregate( (yyvsp[-9].aggKey), (yyvsp[-7].tok), (yyvsp[-1].expr), (yyvsp[-3].decl), true ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-2]) ) )->addQualifiers( (yyvsp[-8].decl) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) );
		}
#line 13760 "Parser/parser.cc"
    break;

  case 632: /* $@5: %empty  */
#line 2736 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[-1].tok), forall || typedefTable.getEnclForall() ? TYPEGENname : TYPEDEFname, "aggregate_type: 3" );
			forall = false;								// reset
		}
#line 13769 "Parser/parser.cc"
    break;

  case 633: /* aggregate_type: aggregate_key attribute_list_opt TYPEGENname attribute_list_opt $@5 '{' field_declaration_list_opt '}' type_parameters_opt attribute_list_opt  */
#line 2741 "Parser/parser.yy"
                {
			DeclarationNode::newFromTypeData( build_type_gen( (yyvsp[-7].tok), nullptr ) );
			(yyval.decl) = setAggrLocs( DeclarationNode::newAggregate( (yyvsp[-9].aggKey), (yyvsp[-7].tok), (yyvsp[-1].expr), (yyvsp[-3].decl), true ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-2]) ) )->addQualifiers( (yyvsp[-8].decl) )->addQualifiers( (yyvsp[0].decl) );
		}
#line 13778 "Parser/parser.cc"
    break;

  case 635: /* type_parameters_opt: %empty  */
#line 2750 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 13784 "Parser/parser.cc"
    break;

  case 636: /* type_parameters_opt: '(' type_list ')'  */
#line 2752 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 13790 "Parser/parser.cc"
    break;

  case 637: /* aggregate_type_nobody: aggregate_key attribute_list_opt identifier  */
#line 2757 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[0].tok), forall || typedefTable.getEnclForall() ? TYPEGENname : TYPEDEFname, "aggregate_type_nobody" );
			forall = false;								// reset
			(yyval.decl) = setAggrLocs( DeclarationNode::newAggregate( (yyvsp[-2].aggKey), (yyvsp[0].tok), nullptr, nullptr, false ), (yylsp[0]), (yyloc), CodeLocation() )->addQualifiers( (yyvsp[-1].decl) );
		}
#line 13800 "Parser/parser.cc"
    break;

  case 638: /* aggregate_type_nobody: aggregate_key attribute_list_opt type_name  */
#line 2763 "Parser/parser.yy"
                {
			forall = false;								// reset
			// Create new generic declaration with same name as previous forward declaration, where the IDENTIFIER is
			// switched to a TYPEGENname. Link any generic arguments from typegen_name to new generic declaration and
			// delete newFromTypeGen.
			if ( (yyvsp[0].type)->kind == TypeData::SymbolicInst && ! (yyvsp[0].type)->symbolic.isTypedef ) {
				(yyval.decl) = DeclarationNode::newFromTypeData( setTypeNameLoc( (yyvsp[0].type), (yylsp[0]) ) )->addQualifiers( (yyvsp[-1].decl) );
			} else {
				(yyval.decl) = setAggrLocs( DeclarationNode::newAggregate( (yyvsp[-2].aggKey), (yyvsp[0].type)->symbolic.name, (yyvsp[0].type)->symbolic.actuals, nullptr, false ), (yylsp[0]), (yyloc), CodeLocation() )->addQualifiers( (yyvsp[-1].decl) );
				(yyvsp[0].type)->symbolic.name = nullptr;			// copied to $$
				(yyvsp[0].type)->symbolic.actuals = nullptr;
				delete (yyvsp[0].type);
			}
		}
#line 13819 "Parser/parser.cc"
    break;

  case 641: /* aggregate_data: STRUCT vtable_opt  */
#line 2786 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Struct; }
#line 13825 "Parser/parser.cc"
    break;

  case 642: /* aggregate_data: UNION  */
#line 2788 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Union; }
#line 13831 "Parser/parser.cc"
    break;

  case 643: /* aggregate_data: EXCEPTION  */
#line 2790 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Exception; }
#line 13837 "Parser/parser.cc"
    break;

  case 644: /* aggregate_control: MONITOR  */
#line 2795 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Monitor; }
#line 13843 "Parser/parser.cc"
    break;

  case 645: /* aggregate_control: MUTEX STRUCT  */
#line 2797 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Monitor; }
#line 13849 "Parser/parser.cc"
    break;

  case 646: /* aggregate_control: GENERATOR  */
#line 2799 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Generator; }
#line 13855 "Parser/parser.cc"
    break;

  case 647: /* aggregate_control: MUTEX GENERATOR  */
#line 2801 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "monitor generator is currently unimplemented." );
			(yyval.aggKey) = ast::AggregateDecl::NoAggregate;
		}
#line 13864 "Parser/parser.cc"
    break;

  case 648: /* aggregate_control: COROUTINE  */
#line 2806 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Coroutine; }
#line 13870 "Parser/parser.cc"
    break;

  case 649: /* aggregate_control: MUTEX COROUTINE  */
#line 2808 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "monitor coroutine is currently unimplemented." );
			(yyval.aggKey) = ast::AggregateDecl::NoAggregate;
		}
#line 13879 "Parser/parser.cc"
    break;

  case 650: /* aggregate_control: THREAD  */
#line 2813 "Parser/parser.yy"
                { (yyval.aggKey) = ast::AggregateDecl::Thread; }
#line 13885 "Parser/parser.cc"
    break;

  case 651: /* aggregate_control: MUTEX THREAD  */
#line 2815 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "monitor thread is currently unimplemented." );
			(yyval.aggKey) = ast::AggregateDecl::NoAggregate;
		}
#line 13894 "Parser/parser.cc"
    break;

  case 652: /* field_declaration_list_opt: %empty  */
#line 2823 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 13900 "Parser/parser.cc"
    break;

  case 653: /* field_declaration_list_opt: field_declaration_list_opt attribute_list_opt field_declaration  */
#line 2825 "Parser/parser.yy"
                { distAttr( (yyvsp[-1].decl), (yyvsp[0].decl) ); (yyval.decl) = (yyvsp[-2].decl) ? (yyvsp[-2].decl)->set_last( (yyvsp[0].decl) ) : (yyvsp[0].decl); }
#line 13906 "Parser/parser.cc"
    break;

  case 654: /* field_declaration: type_specifier field_declaring_list_opt ';'  */
#line 2830 "Parser/parser.yy"
                { (yyval.decl) = setExtent( fieldDecl( (yyvsp[-2].decl), (yyvsp[-1].decl) ), (yyloc) ); }
#line 13912 "Parser/parser.cc"
    break;

  case 655: /* field_declaration: type_specifier field_declaring_list_opt '}'  */
#line 2832 "Parser/parser.yy"
                {
			SemanticError( (yyloc), "illegal syntax, expecting ';' at end of previous declaration." );
			(yyval.decl) = nullptr;
		}
#line 13921 "Parser/parser.cc"
    break;

  case 656: /* field_declaration: EXTENSION type_specifier field_declaring_list_opt ';'  */
#line 2837 "Parser/parser.yy"
                { (yyval.decl) = setExtent( fieldDecl( (yyvsp[-2].decl), (yyvsp[-1].decl) ), (yyloc) ); distExt( (yyval.decl) ); }
#line 13927 "Parser/parser.cc"
    break;

  case 657: /* field_declaration: STATIC type_specifier field_declaring_list_opt ';'  */
#line 2839 "Parser/parser.yy"
                { SemanticError( (yyloc), "STATIC aggregate field qualifier currently unimplemented." ); (yyval.decl) = nullptr; }
#line 13933 "Parser/parser.cc"
    break;

  case 658: /* field_declaration: INLINE attribute_list_opt type_specifier field_abstract_list_opt ';'  */
#line 2841 "Parser/parser.yy"
                {
			if ( ! (yyvsp[-1].decl) ) {								// field declarator ?
				(yyvsp[-1].decl) = DeclarationNode::newName( nullptr );
			} // if
			(yyvsp[-1].decl)->inLine = true;
			(yyval.decl) = distTypeSpec( (yyvsp[-2].decl), (yyvsp[-1].decl) );				// mark all fields in list
			distInl( (yyvsp[-1].decl) );
		}
#line 13946 "Parser/parser.cc"
    break;

  case 659: /* field_declaration: INLINE attribute_list_opt aggregate_control ';'  */
#line 2850 "Parser/parser.yy"
                { SemanticError( (yyloc), "INLINE aggregate control currently unimplemented." ); (yyval.decl) = nullptr; }
#line 13952 "Parser/parser.cc"
    break;

  case 661: /* field_declaration: cfa_field_declaring_list ';'  */
#line 2853 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-1].decl), (yyloc) ); }
#line 13958 "Parser/parser.cc"
    break;

  case 662: /* field_declaration: EXTENSION cfa_field_declaring_list ';'  */
#line 2855 "Parser/parser.yy"
                { distExt( (yyvsp[-1].decl) ); (yyval.decl) = (yyvsp[-1].decl); }
#line 13964 "Parser/parser.cc"
    break;

  case 663: /* field_declaration: INLINE attribute_list_opt cfa_field_abstract_list ';'  */
#line 2857 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 13970 "Parser/parser.cc"
    break;

  case 666: /* field_declaring_list_opt: %empty  */
#line 2864 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 13976 "Parser/parser.cc"
    break;

  case 669: /* field_declaring_list: field_declaring_list_opt ',' attribute_list_opt field_declarator  */
#line 2871 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->set_last( (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ) ); }
#line 13982 "Parser/parser.cc"
    break;

  case 670: /* field_declarator: bit_subrange_size  */
#line 2876 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newBitfield( (yyvsp[0].expr) ); }
#line 13988 "Parser/parser.cc"
    break;

  case 671: /* field_declarator: variable_declarator bit_subrange_size_opt  */
#line 2879 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addBitfield( (yyvsp[0].expr) ); }
#line 13994 "Parser/parser.cc"
    break;

  case 672: /* field_declarator: variable_type_redeclarator bit_subrange_size_opt  */
#line 2882 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addBitfield( (yyvsp[0].expr) ); }
#line 14000 "Parser/parser.cc"
    break;

  case 673: /* field_declarator: function_type_redeclarator bit_subrange_size_opt  */
#line 2885 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addBitfield( (yyvsp[0].expr) ); }
#line 14006 "Parser/parser.cc"
    break;

  case 674: /* field_abstract_list_opt: %empty  */
#line 2890 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14012 "Parser/parser.cc"
    break;

  case 676: /* field_abstract_list_opt: field_abstract_list_opt ',' attribute_list_opt field_abstract  */
#line 2893 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->set_last( (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ) ); }
#line 14018 "Parser/parser.cc"
    break;

  case 678: /* cfa_field_declaring_list: cfa_abstract_declarator_tuple identifier_or_type_name  */
#line 2903 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-1].decl)->addName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 14024 "Parser/parser.cc"
    break;

  case 679: /* cfa_field_declaring_list: cfa_field_declaring_list ',' identifier_or_type_name  */
#line 2905 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( setNameLoc( (yyvsp[-2].decl)->cloneType( (yyvsp[0].tok) ), (yylsp[0]) ) ); }
#line 14030 "Parser/parser.cc"
    break;

  case 681: /* cfa_field_abstract_list: cfa_field_abstract_list ','  */
#line 2912 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->set_last( (yyvsp[-1].decl)->cloneType( 0 ) ); }
#line 14036 "Parser/parser.cc"
    break;

  case 682: /* bit_subrange_size_opt: %empty  */
#line 2917 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 14042 "Parser/parser.cc"
    break;

  case 684: /* bit_subrange_size: ':' assignment_expression  */
#line 2923 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[0].expr); }
#line 14048 "Parser/parser.cc"
    break;

  case 685: /* enum_type: ENUM attribute_list_opt hide_opt '{' enumerator_list comma_opt '}' attribute_list_opt  */
#line 2931 "Parser/parser.yy"
                {
			if ( (yyvsp[-5].enum_hiding) == EnumHiding::Hide ) {
				SemanticError( (yyloc), "illegal syntax, hiding ('!') the enumerator names of an anonymous enumeration means the names are inaccessible." ); (yyval.decl) = nullptr;
			} // if
			(yyval.decl) = setAggrLocs( DeclarationNode::newEnum( nullptr, (yyvsp[-3].decl), true, false ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-1]) ) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) );
		}
#line 14059 "Parser/parser.cc"
    break;

  case 686: /* enum_type: ENUM enumerator_type attribute_list_opt hide_opt '{' enumerator_list comma_opt '}' attribute_list_opt  */
#line 2938 "Parser/parser.yy"
                {
			if ( (yyvsp[-7].decl) && ((yyvsp[-7].decl)->storageClasses.val != 0 || (yyvsp[-7].decl)->type->qualifiers.any()) ) {
				SemanticError( (yyloc), "illegal syntax, storage-class and CV qualifiers are not meaningful for enumeration constants, which are const." );
			}
			if ( (yyvsp[-5].enum_hiding) == EnumHiding::Hide ) {
				SemanticError( (yyloc), "illegal syntax, hiding ('!') the enumerator names of an anonymous enumeration means the names are inaccessible." ); (yyval.decl) = nullptr;
			} // if
			(yyval.decl) = setAggrLocs( DeclarationNode::newEnum( nullptr, (yyvsp[-3].decl), true, true, (yyvsp[-7].decl) ), (yylsp[-8]), (yyloc), span( (yylsp[-4]), (yylsp[-1]) ) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) );
		}
#line 14073 "Parser/parser.cc"
    break;

  case 687: /* $@6: %empty  */
#line 2950 "Parser/parser.yy"
                { typedefTable.makeTypedef( *(yyvsp[-1].tok), "enum_type 1" ); }
#line 14079 "Parser/parser.cc"
    break;

  case 688: /* enum_type: ENUM attribute_list_opt identifier attribute_list_opt $@6 hide_opt '{' enumerator_list comma_opt '}' attribute_list_opt  */
#line 2952 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newEnum( (yyvsp[-8].tok), (yyvsp[-3].decl), true, false, nullptr, (yyvsp[-5].enum_hiding) ), (yylsp[-8]), (yyloc), span( (yylsp[-4]), (yylsp[-1]) ) )->addQualifiers( (yyvsp[-9].decl) ->addQualifiers( (yyvsp[-7].decl) ))->addQualifiers( (yyvsp[0].decl) ); }
#line 14085 "Parser/parser.cc"
    break;

  case 689: /* enum_type: ENUM attribute_list_opt typedef_name attribute_list_opt hide_opt '{' enumerator_list comma_opt '}' attribute_list_opt  */
#line 2954 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newEnum( (yyvsp[-7].decl)->name, (yyvsp[-3].decl), true, false, nullptr, (yyvsp[-5].enum_hiding) ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-1]) ) )->addQualifiers( (yyvsp[-8].decl) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 14091 "Parser/parser.cc"
    break;

  case 690: /* $@7: %empty  */
#line 2956 "Parser/parser.yy"
                {
			if ( (yyvsp[-3].decl) && ((yyvsp[-3].decl)->storageClasses.any() || (yyvsp[-3].decl)->type->qualifiers.val != 0) ) {
				SemanticError( (yyloc), "illegal syntax, storage-class and CV qualifiers are not meaningful for enumeration constants, which are const." );
			}
			typedefTable.makeTypedef( *(yyvsp[-1].tok), "enum_type 2" );
		}
#line 14102 "Parser/parser.cc"
    break;

  case 691: /* enum_type: ENUM enumerator_type attribute_list_opt identifier attribute_list_opt $@7 hide_opt '{' enumerator_list comma_opt '}' attribute_list_opt  */
#line 2963 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newEnum( (yyvsp[-8].tok), (yyvsp[-3].decl), true, true, (yyvsp[-10].decl), (yyvsp[-5].enum_hiding) ), (yylsp[-8]), (yyloc), span( (yylsp[-4]), (yylsp[-1]) ) )->addQualifiers( (yyvsp[-9].decl) )->addQualifiers( (yyvsp[-7].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 14108 "Parser/parser.cc"
    break;

  case 692: /* enum_type: ENUM enumerator_type attribute_list_opt typedef_name attribute_list_opt hide_opt '{' enumerator_list comma_opt '}' attribute_list_opt  */
#line 2965 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newEnum( (yyvsp[-7].decl)->name, (yyvsp[-3].decl), true, true, (yyvsp[-9].decl), (yyvsp[-5].enum_hiding) ), (yylsp[-7]), (yyloc), span( (yylsp[-4]), (yylsp[-1]) ) )->addQualifiers( (yyvsp[-8].decl) )->addQualifiers( (yyvsp[-6].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 14114 "Parser/parser.cc"
    break;

  case 694: /* enumerator_type: '(' ')'  */
#line 2973 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14120 "Parser/parser.cc"
    break;

  case 695: /* enumerator_type: '(' cfa_abstract_parameter_declaration ')'  */
#line 2975 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 14126 "Parser/parser.cc"
    break;

  case 696: /* hide_opt: %empty  */
#line 2980 "Parser/parser.yy"
                { (yyval.enum_hiding) = EnumHiding::Visible; }
#line 14132 "Parser/parser.cc"
    break;

  case 697: /* hide_opt: '!'  */
#line 2982 "Parser/parser.yy"
                { (yyval.enum_hiding) = EnumHiding::Hide; }
#line 14138 "Parser/parser.cc"
    break;

  case 698: /* enum_type_nobody: ENUM attribute_list_opt identifier  */
#line 2987 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[0].tok), "enum_type_nobody 1" );
			(yyval.decl) = setAggrLocs( DeclarationNode::newEnum( (yyvsp[0].tok), nullptr, false, false ), (yylsp[0]), (yyloc), CodeLocation() )->addQualifiers( (yyvsp[-1].decl) );
		}
#line 14147 "Parser/parser.cc"
    break;

  case 699: /* enum_type_nobody: ENUM attribute_list_opt type_name  */
#line 2992 "Parser/parser.yy"
                {
			typedefTable.makeTypedef( *(yyvsp[0].type)->symbolic.name, "enum_type_nobody 2" );
			(yyval.decl) = setAggrLocs( DeclarationNode::newEnum( (yyvsp[0].type)->symbolic.name, nullptr, false, false ), (yylsp[0]), (yyloc), CodeLocation() )->addQualifiers( (yyvsp[-1].decl) );
		}
#line 14156 "Parser/parser.cc"
    break;

  case 700: /* enumerator_list: %empty  */
#line 3000 "Parser/parser.yy"
                { SemanticError( (yyloc), "enumeration must have a minimum of one enumerator, empty enumerator list is meaningless." );  (yyval.decl) = nullptr; }
#line 14162 "Parser/parser.cc"
    break;

  case 701: /* enumerator_list: visible_hide_opt identifier_or_type_name enumerator_value_opt  */
#line 3002 "Parser/parser.yy"
                { (yyval.decl) = setExtent( setNameLoc( DeclarationNode::newEnumValueGeneric( (yyvsp[-1].tok), (yyvsp[0].init) ), (yylsp[-1]) ), span( (yylsp[-1]), (yylsp[0]) ) ); }
#line 14168 "Parser/parser.cc"
    break;

  case 702: /* enumerator_list: INLINE type_name  */
#line 3004 "Parser/parser.yy"
                {
			(yyval.decl) = DeclarationNode::newEnumInLine( (yyvsp[0].type)->symbolic.name );
			(yyvsp[0].type)->symbolic.name = nullptr;
			delete (yyvsp[0].type);
		}
#line 14178 "Parser/parser.cc"
    break;

  case 703: /* enumerator_list: enumerator_list ',' visible_hide_opt identifier_or_type_name enumerator_value_opt  */
#line 3010 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->set_last( setExtent( setNameLoc( DeclarationNode::newEnumValueGeneric( (yyvsp[-1].tok), (yyvsp[0].init) ), (yylsp[-1]) ), span( (yylsp[-1]), (yylsp[0]) ) ) ); }
#line 14184 "Parser/parser.cc"
    break;

  case 704: /* enumerator_list: enumerator_list ',' INLINE type_name  */
#line 3012 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->set_last( DeclarationNode::newEnumInLine( (yyvsp[0].type)->symbolic.name )  ); }
#line 14190 "Parser/parser.cc"
    break;

  case 706: /* visible_hide_opt: '^'  */
#line 3018 "Parser/parser.yy"
                { (yyval.enum_hiding) = EnumHiding::Visible; }
#line 14196 "Parser/parser.cc"
    break;

  case 707: /* enumerator_value_opt: %empty  */
#line 3023 "Parser/parser.yy"
                { (yyval.init) = nullptr; }
#line 14202 "Parser/parser.cc"
    break;

  case 708: /* enumerator_value_opt: '=' constant_expression  */
#line 3024 "Parser/parser.yy"
                                                                        { (yyval.init) = new InitializerNode( (yyvsp[0].expr) ); }
#line 14208 "Parser/parser.cc"
    break;

  case 709: /* enumerator_value_opt: '=' '{' initializer_list_opt comma_opt '}'  */
#line 3025 "Parser/parser.yy"
                                                     { (yyval.init) = new InitializerNode( (yyvsp[-2].init), true ); }
#line 14214 "Parser/parser.cc"
    break;

  case 710: /* parameter_list_ellipsis_opt: %empty  */
#line 3034 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( build_basic_type( TypeData::Void ) ); }
#line 14220 "Parser/parser.cc"
    break;

  case 711: /* parameter_list_ellipsis_opt: ELLIPSIS  */
#line 3036 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14226 "Parser/parser.cc"
    break;

  case 713: /* parameter_list_ellipsis_opt: parameter_list ',' ELLIPSIS  */
#line 3039 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addVarArgs(); }
#line 14232 "Parser/parser.cc"
    break;

  case 715: /* parameter_list: attribute_list parameter_declaration  */
#line 3045 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 14238 "Parser/parser.cc"
    break;

  case 717: /* parameter_list: attribute_list abstract_parameter_declaration  */
#line 3048 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 14244 "Parser/parser.cc"
    break;

  case 718: /* parameter_list: parameter_list ',' attribute_list_opt parameter_declaration  */
#line 3050 "Parser/parser.yy"
                { (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); (yyval.decl) = (yyvsp[-3].decl)->set_last( (yyvsp[0].decl) ); }
#line 14250 "Parser/parser.cc"
    break;

  case 719: /* parameter_list: parameter_list ',' attribute_list_opt abstract_parameter_declaration  */
#line 3052 "Parser/parser.yy"
                { (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); (yyval.decl) = (yyvsp[-3].decl)->set_last( (yyvsp[0].decl) ); }
#line 14256 "Parser/parser.cc"
    break;

  case 720: /* cfa_parameter_list_ellipsis_opt: %empty  */
#line 3057 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFromTypeData( build_basic_type( TypeData::Void ) ); }
#line 14262 "Parser/parser.cc"
    break;

  case 721: /* cfa_parameter_list_ellipsis_opt: ELLIPSIS  */
#line 3059 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14268 "Parser/parser.cc"
    break;

  case 724: /* cfa_parameter_list_ellipsis_opt: cfa_parameter_list ',' cfa_abstract_parameter_list  */
#line 3063 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[0].decl) ); }
#line 14274 "Parser/parser.cc"
    break;

  case 725: /* cfa_parameter_list_ellipsis_opt: cfa_parameter_list ',' ELLIPSIS  */
#line 3065 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addVarArgs(); }
#line 14280 "Parser/parser.cc"
    break;

  case 726: /* cfa_parameter_list_ellipsis_opt: cfa_abstract_parameter_list ',' ELLIPSIS  */
#line 3067 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addVarArgs(); }
#line 14286 "Parser/parser.cc"
    break;

  case 728: /* cfa_parameter_list: cfa_abstract_parameter_list ',' cfa_parameter_declaration  */
#line 3075 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[0].decl) ); }
#line 14292 "Parser/parser.cc"
    break;

  case 729: /* cfa_parameter_list: cfa_parameter_list ',' cfa_parameter_declaration  */
#line 3077 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[0].decl) ); }
#line 14298 "Parser/parser.cc"
    break;

  case 730: /* cfa_parameter_list: cfa_parameter_list ',' cfa_abstract_parameter_list ',' cfa_parameter_declaration  */
#line 3079 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->set_last( (yyvsp[-2].decl) )->set_last( (yyvsp[0].decl) ); }
#line 14304 "Parser/parser.cc"
    break;

  case 732: /* cfa_abstract_parameter_list: cfa_abstract_parameter_list ',' cfa_abstract_parameter_declaration  */
#line 3085 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[0].decl) ); }
#line 14310 "Parser/parser.cc"
    break;

  case 733: /* parameter_declaration: declaration_specifier_nobody identifier_parameter_declarator default_initializer_opt  */
#line 3094 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-1].decl)->addType( (yyvsp[-2].decl) ), (yyloc) )->addInitializer( (yyvsp[0].expr) ? new InitializerNode( (yyvsp[0].expr) ) : nullptr ); }
#line 14316 "Parser/parser.cc"
    break;

  case 734: /* parameter_declaration: declaration_specifier_nobody type_parameter_redeclarator default_initializer_opt  */
#line 3096 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-1].decl)->addType( (yyvsp[-2].decl) ), (yyloc) )->addInitializer( (yyvsp[0].expr) ? new InitializerNode( (yyvsp[0].expr) ) : nullptr ); }
#line 14322 "Parser/parser.cc"
    break;

  case 735: /* abstract_parameter_declaration: declaration_specifier_nobody default_initializer_opt  */
#line 3101 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addInitializer( (yyvsp[0].expr) ? new InitializerNode( (yyvsp[0].expr) ) : nullptr ); }
#line 14328 "Parser/parser.cc"
    break;

  case 736: /* abstract_parameter_declaration: declaration_specifier_nobody abstract_parameter_declarator default_initializer_opt  */
#line 3103 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addType( (yyvsp[-2].decl) )->addInitializer( (yyvsp[0].expr) ? new InitializerNode( (yyvsp[0].expr) ) : nullptr ); }
#line 14334 "Parser/parser.cc"
    break;

  case 738: /* cfa_parameter_declaration: cfa_identifier_parameter_declarator_no_tuple identifier_or_type_name default_initializer_opt  */
#line 3109 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addName( (yyvsp[-1].tok) ), (yylsp[-1]) ); }
#line 14340 "Parser/parser.cc"
    break;

  case 739: /* cfa_parameter_declaration: cfa_abstract_tuple identifier_or_type_name default_initializer_opt  */
#line 3112 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addName( (yyvsp[-1].tok) ), (yylsp[-1]) ); }
#line 14346 "Parser/parser.cc"
    break;

  case 740: /* cfa_parameter_declaration: type_qualifier_list cfa_abstract_tuple identifier_or_type_name default_initializer_opt  */
#line 3114 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( (yyvsp[-2].decl)->addName( (yyvsp[-1].tok) ), (yylsp[-1]) )->addQualifiers( (yyvsp[-3].decl) ); }
#line 14352 "Parser/parser.cc"
    break;

  case 745: /* cfa_abstract_parameter_declaration: type_qualifier_list cfa_abstract_tuple  */
#line 3124 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 14358 "Parser/parser.cc"
    break;

  case 747: /* identifier_list: identifier  */
#line 3134 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 14364 "Parser/parser.cc"
    break;

  case 748: /* identifier_list: identifier_list ',' identifier  */
#line 3136 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( setNameLoc( DeclarationNode::newName( (yyvsp[0].tok) ), (yylsp[0]) ) ); }
#line 14370 "Parser/parser.cc"
    break;

  case 750: /* type_no_function: type_specifier abstract_declarator  */
#line 3142 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addType( (yyvsp[-1].decl) ); }
#line 14376 "Parser/parser.cc"
    break;

  case 753: /* type: attribute_list type_no_function  */
#line 3149 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 14382 "Parser/parser.cc"
    break;

  case 755: /* type: attribute_list cfa_abstract_function  */
#line 3152 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 14388 "Parser/parser.cc"
    break;

  case 756: /* initializer_opt: %empty  */
#line 3157 "Parser/parser.yy"
                { (yyval.init) = nullptr; }
#line 14394 "Parser/parser.cc"
    break;

  case 757: /* initializer_opt: simple_assignment_operator initializer  */
#line 3158 "Parser/parser.yy"
                                                        { (yyval.init) = (yyvsp[-1].oper) == OperKinds::Assign ? (yyvsp[0].init) : (yyvsp[0].init)->set_maybeConstructed( false ); }
#line 14400 "Parser/parser.cc"
    break;

  case 758: /* initializer_opt: '=' VOID  */
#line 3159 "Parser/parser.yy"
                                                                                        { (yyval.init) = new InitializerNode( true ); }
#line 14406 "Parser/parser.cc"
    break;

  case 759: /* initializer_opt: '{' initializer_list_opt comma_opt '}'  */
#line 3160 "Parser/parser.yy"
                                                        { (yyval.init) = new InitializerNode( (yyvsp[-2].init), true ); }
#line 14412 "Parser/parser.cc"
    break;

  case 760: /* initializer: assignment_expression  */
#line 3164 "Parser/parser.yy"
                                                                        { (yyval.init) = new InitializerNode( (yyvsp[0].expr) ); }
#line 14418 "Parser/parser.cc"
    break;

  case 761: /* initializer: '{' initializer_list_opt comma_opt '}'  */
#line 3165 "Parser/parser.yy"
                                                        { (yyval.init) = new InitializerNode( (yyvsp[-2].init), true ); }
#line 14424 "Parser/parser.cc"
    break;

  case 762: /* initializer_list_opt: %empty  */
#line 3170 "Parser/parser.yy"
                { (yyval.init) = nullptr; }
#line 14430 "Parser/parser.cc"
    break;

  case 764: /* initializer_list_opt: designation initializer  */
#line 3172 "Parser/parser.yy"
                                                                        { (yyval.init) = (yyvsp[0].init)->set_designators( (yyvsp[-1].expr) ); }
#line 14436 "Parser/parser.cc"
    break;

  case 765: /* initializer_list_opt: initializer_list_opt ',' initializer  */
#line 3173 "Parser/parser.yy"
                                                        { (yyval.init) = (yyvsp[-2].init)->set_last( (yyvsp[0].init) ); }
#line 14442 "Parser/parser.cc"
    break;

  case 766: /* initializer_list_opt: initializer_list_opt ',' designation initializer  */
#line 3174 "Parser/parser.yy"
                                                           { (yyval.init) = (yyvsp[-3].init)->set_last( (yyvsp[0].init)->set_designators( (yyvsp[-1].expr) ) ); }
#line 14448 "Parser/parser.cc"
    break;

  case 768: /* designation: identifier_at ':'  */
#line 3190 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_varref( (yylsp[-1]), (yyvsp[-1].tok) ) ); }
#line 14454 "Parser/parser.cc"
    break;

  case 770: /* designator_list: designator_list designator  */
#line 3196 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr)->set_last( (yyvsp[0].expr) ); }
#line 14460 "Parser/parser.cc"
    break;

  case 771: /* designator: '.' identifier_at  */
#line 3201 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( build_varref( (yylsp[0]), (yyvsp[0].tok) ) ); }
#line 14466 "Parser/parser.cc"
    break;

  case 772: /* designator: '[' constant_expression ']'  */
#line 3203 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 14472 "Parser/parser.cc"
    break;

  case 773: /* designator: '[' subrange ']'  */
#line 3205 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 14478 "Parser/parser.cc"
    break;

  case 774: /* designator: '[' constant_expression ELLIPSIS constant_expression ']'  */
#line 3207 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::RangeExpr( (yyloc), maybeMoveBuild( (yyvsp[-3].expr) ), maybeMoveBuild( (yyvsp[-1].expr) ) ) ); }
#line 14484 "Parser/parser.cc"
    break;

  case 775: /* designator: '.' '[' field_name_list ']'  */
#line 3209 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-1].expr); }
#line 14490 "Parser/parser.cc"
    break;

  case 777: /* type_parameter_list: type_parameter_list ',' type_parameter  */
#line 3233 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[0].decl) ); }
#line 14496 "Parser/parser.cc"
    break;

  case 778: /* type_initializer_opt: %empty  */
#line 3238 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14502 "Parser/parser.cc"
    break;

  case 779: /* type_initializer_opt: '=' type  */
#line 3240 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl); }
#line 14508 "Parser/parser.cc"
    break;

  case 780: /* $@8: %empty  */
#line 3245 "Parser/parser.yy"
                { typedefTable.addToScope( *(yyvsp[0].tok), TYPEDEFname, "type_parameter 1" ); }
#line 14514 "Parser/parser.cc"
    break;

  case 781: /* type_parameter: type_class identifier_or_type_name $@8 type_initializer_opt assertion_list_opt  */
#line 3247 "Parser/parser.yy"
                { (yyval.decl) = setExtent( setNameLoc( DeclarationNode::newTypeParam( (yyvsp[-4].tclass), (yyvsp[-3].tok) ), (yylsp[-3]) ), (yyloc) )->addTypeInitializer( (yyvsp[-1].decl) )->addAssertions( (yyvsp[0].decl) ); }
#line 14520 "Parser/parser.cc"
    break;

  case 782: /* $@9: %empty  */
#line 3249 "Parser/parser.yy"
                { typedefTable.addToScope( *(yyvsp[-1].tok), TYPEDEFname, "type_parameter 2" ); }
#line 14526 "Parser/parser.cc"
    break;

  case 783: /* type_parameter: identifier_or_type_name new_type_class $@9 type_initializer_opt assertion_list_opt  */
#line 3251 "Parser/parser.yy"
                { (yyval.decl) = setExtent( setNameLoc( DeclarationNode::newTypeParam( (yyvsp[-3].tclass), (yyvsp[-4].tok) ), (yylsp[-4]) ), (yyloc) )->addTypeInitializer( (yyvsp[-1].decl) )->addAssertions( (yyvsp[0].decl) ); }
#line 14532 "Parser/parser.cc"
    break;

  case 784: /* type_parameter: '[' identifier_or_type_name ']' assertion_list_opt  */
#line 3253 "Parser/parser.yy"
                {
			typedefTable.addToScope( *(yyvsp[-2].tok), TYPEDIMname, "type_parameter 3" );
			(yyval.decl) = setExtent( setNameLoc( DeclarationNode::newTypeParam( ast::TypeDecl::Dimension, (yyvsp[-2].tok) ), (yylsp[-2]) ), (yyloc) )->addAssertions( (yyvsp[0].decl) );
		}
#line 14541 "Parser/parser.cc"
    break;

  case 785: /* type_parameter: assertion_list  */
#line 3260 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTypeParam( ast::TypeDecl::Dtype, new string( "" ) )->addAssertions( (yyvsp[0].decl) ); }
#line 14547 "Parser/parser.cc"
    break;

  case 786: /* type_parameter: ENUM '(' identifier_or_type_name ')' identifier_or_type_name new_type_class type_initializer_opt assertion_list_opt  */
#line 3262 "Parser/parser.yy"
                {	
			typedefTable.addToScope( *(yyvsp[-5].tok), TYPEDIMname, "type_parameter 4" );
			typedefTable.addToScope( *(yyvsp[-3].tok), TYPEDIMname, "type_parameter 5" );
			(yyval.decl) = setExtent( setNameLoc( DeclarationNode::newTypeParam( (yyvsp[-2].tclass), (yyvsp[-3].tok) ), (yylsp[-3]) ), (yyloc) )->addTypeInitializer( (yyvsp[-1].decl) )->addAssertions( (yyvsp[0].decl) );
		}
#line 14557 "Parser/parser.cc"
    break;

  case 787: /* new_type_class: %empty  */
#line 3271 "Parser/parser.yy"
                { (yyval.tclass) = ast::TypeDecl::Otype; }
#line 14563 "Parser/parser.cc"
    break;

  case 788: /* new_type_class: '&'  */
#line 3273 "Parser/parser.yy"
                { (yyval.tclass) = ast::TypeDecl::Dtype; }
#line 14569 "Parser/parser.cc"
    break;

  case 789: /* new_type_class: '*'  */
#line 3275 "Parser/parser.yy"
                { (yyval.tclass) = ast::TypeDecl::DStype; }
#line 14575 "Parser/parser.cc"
    break;

  case 790: /* new_type_class: ELLIPSIS  */
#line 3279 "Parser/parser.yy"
                { (yyval.tclass) = ast::TypeDecl::Ttype; }
#line 14581 "Parser/parser.cc"
    break;

  case 791: /* type_class: OTYPE  */
#line 3284 "Parser/parser.yy"
                { SemanticError( (yyloc), "otype keyword is deprecated, use T " ); }
#line 14587 "Parser/parser.cc"
    break;

  case 792: /* type_class: DTYPE  */
#line 3286 "Parser/parser.yy"
                { SemanticError( (yyloc), "dtype keyword is deprecated, use T &" ); }
#line 14593 "Parser/parser.cc"
    break;

  case 793: /* type_class: FTYPE  */
#line 3288 "Parser/parser.yy"
                { (yyval.tclass) = ast::TypeDecl::Ftype; }
#line 14599 "Parser/parser.cc"
    break;

  case 794: /* type_class: TTYPE  */
#line 3290 "Parser/parser.yy"
                { SemanticError( (yyloc), "ttype keyword is deprecated, use T ..." ); }
#line 14605 "Parser/parser.cc"
    break;

  case 795: /* assertion_list_opt: %empty  */
#line 3295 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14611 "Parser/parser.cc"
    break;

  case 798: /* assertion_list: assertion_list assertion  */
#line 3302 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->set_last( (yyvsp[0].decl) ); }
#line 14617 "Parser/parser.cc"
    break;

  case 799: /* assertion: '|' identifier_or_type_name '(' type_list ')'  */
#line 3307 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTraitUse( (yyvsp[-3].tok), (yyvsp[-1].expr) ); setTypeNameLoc( (yyval.decl)->type->aggInst.aggregate, (yylsp[-3]) ); }
#line 14623 "Parser/parser.cc"
    break;

  case 800: /* assertion: '|' '{' trait_declaration_list '}'  */
#line 3309 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 14629 "Parser/parser.cc"
    break;

  case 801: /* type_list: type  */
#line 3316 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[0].decl) ) ) ); }
#line 14635 "Parser/parser.cc"
    break;

  case 803: /* type_list: type_list ',' type  */
#line 3319 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[0].decl) ) ) ) ); }
#line 14641 "Parser/parser.cc"
    break;

  case 804: /* type_list: type_list ',' assignment_expression  */
#line 3321 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( (yyvsp[0].expr) ); }
#line 14647 "Parser/parser.cc"
    break;

  case 805: /* type_declaring_list: OTYPE type_declarator  */
#line 3326 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl); }
#line 14653 "Parser/parser.cc"
    break;

  case 806: /* type_declaring_list: storage_class_list OTYPE type_declarator  */
#line 3328 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 14659 "Parser/parser.cc"
    break;

  case 807: /* type_declaring_list: type_declaring_list ',' type_declarator  */
#line 3330 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[0].decl)->copySpecifiers( (yyvsp[-2].decl) ) ); }
#line 14665 "Parser/parser.cc"
    break;

  case 808: /* type_declarator: type_declarator_name assertion_list_opt  */
#line 3335 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addAssertions( (yyvsp[0].decl) ); }
#line 14671 "Parser/parser.cc"
    break;

  case 809: /* type_declarator: type_declarator_name assertion_list_opt '=' type  */
#line 3337 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addAssertions( (yyvsp[-2].decl) )->addType( (yyvsp[0].decl) ); }
#line 14677 "Parser/parser.cc"
    break;

  case 810: /* type_declarator_name: identifier_or_type_name  */
#line 3342 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[0].tok), TYPEDEFname, "type_declarator_name 1" );
			(yyval.decl) = setNameLoc( DeclarationNode::newTypeDecl( (yyvsp[0].tok), nullptr ), (yylsp[0]) );
		}
#line 14686 "Parser/parser.cc"
    break;

  case 811: /* type_declarator_name: identifier_or_type_name '(' type_parameter_list ')'  */
#line 3347 "Parser/parser.yy"
                {
			typedefTable.addToEnclosingScope( *(yyvsp[-3].tok), TYPEGENname, "type_declarator_name 2" );
			(yyval.decl) = setNameLoc( DeclarationNode::newTypeDecl( (yyvsp[-3].tok), (yyvsp[-1].decl) ), (yylsp[-3]) );
		}
#line 14695 "Parser/parser.cc"
    break;

  case 812: /* trait_specifier: TRAIT identifier_or_type_name '(' type_parameter_list ')' '{' '}'  */
#line 3355 "Parser/parser.yy"
                {
			SemanticWarning( (yyloc), Warning::DeprecTraitSyntax );
			(yyval.decl) = setAggrLocs( DeclarationNode::newTrait( (yyvsp[-5].tok), (yyvsp[-3].decl), nullptr ), (yylsp[-5]), (yyloc), span( (yylsp[-1]), (yylsp[0]) ) );
		}
#line 14704 "Parser/parser.cc"
    break;

  case 813: /* trait_specifier: forall TRAIT identifier_or_type_name '{' '}'  */
#line 3360 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newTrait( (yyvsp[-2].tok), (yyvsp[-4].decl), nullptr ), (yylsp[-2]), (yyloc), span( (yylsp[-1]), (yylsp[0]) ) ); }
#line 14710 "Parser/parser.cc"
    break;

  case 814: /* trait_specifier: TRAIT identifier_or_type_name '(' type_parameter_list ')' '{' trait_declaration_list '}'  */
#line 3362 "Parser/parser.yy"
                {
			SemanticWarning( (yyloc), Warning::DeprecTraitSyntax );
			(yyval.decl) = setAggrLocs( DeclarationNode::newTrait( (yyvsp[-6].tok), (yyvsp[-4].decl), (yyvsp[-1].decl) ), (yylsp[-6]), (yyloc), span( (yylsp[-2]), (yylsp[0]) ) );
		}
#line 14719 "Parser/parser.cc"
    break;

  case 815: /* trait_specifier: forall TRAIT identifier_or_type_name '{' trait_declaration_list '}'  */
#line 3367 "Parser/parser.yy"
                { (yyval.decl) = setAggrLocs( DeclarationNode::newTrait( (yyvsp[-3].tok), (yyvsp[-5].decl), (yyvsp[-1].decl) ), (yylsp[-3]), (yyloc), span( (yylsp[-2]), (yylsp[0]) ) ); }
#line 14725 "Parser/parser.cc"
    break;

  case 817: /* trait_declaration_list: trait_declaration_list trait_declaration  */
#line 3373 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->set_last( (yyvsp[0].decl) ); }
#line 14731 "Parser/parser.cc"
    break;

  case 822: /* cfa_trait_declaring_list: cfa_trait_declaring_list ',' identifier_or_type_name  */
#line 3385 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( setNameLoc( (yyvsp[-2].decl)->cloneType( (yyvsp[0].tok) ), (yylsp[0]) ) ); }
#line 14737 "Parser/parser.cc"
    break;

  case 823: /* trait_declaring_list: type_specifier_nobody declarator  */
#line 3391 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addType( (yyvsp[-1].decl) ); }
#line 14743 "Parser/parser.cc"
    break;

  case 824: /* trait_declaring_list: trait_declaring_list ',' declarator  */
#line 3393 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->set_last( (yyvsp[-2].decl)->cloneBaseType( (yyvsp[0].decl) ) ); }
#line 14749 "Parser/parser.cc"
    break;

  case 825: /* trait_declaring_list: error  */
#line 3395 "Parser/parser.yy"
                { SemanticError( (yyloc), "Possible cause is declaring an aggregate or enumeration type in a trait." ); (yyval.decl) = nullptr; }
#line 14755 "Parser/parser.cc"
    break;

  case 828: /* top_definition_list: attribute_list_opt recovery_push external_definition recovery_pop  */
#line 3407 "Parser/parser.yy"
                { distAttr( (yyvsp[-3].decl), (yyvsp[-1].decl) ); addToParseTree( (yyvsp[-1].decl) ); (yyval.decl) = (yyvsp[-1].decl); }
#line 14761 "Parser/parser.cc"
    break;

  case 829: /* top_definition_list: top_definition_list attribute_list_opt recovery_push external_definition recovery_pop  */
#line 3409 "Parser/parser.yy"
                {
			distAttr( (yyvsp[-3].decl), (yyvsp[-1].decl) );
			if ( ! (yyvsp[-4].decl) && (yyvsp[-1].decl) ) (yyvsp[-1].decl)->addQualifiers( (yyvsp[-3].decl) );
			addToParseTree( (yyvsp[-1].decl) );
			(yyval.decl) = (yyvsp[-4].decl) ? (yyvsp[-4].decl) : (yyvsp[-1].decl);
		}
#line 14772 "Parser/parser.cc"
    break;

  case 830: /* external_definition_list_opt: %empty  */
#line 3419 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14778 "Parser/parser.cc"
    break;

  case 832: /* external_definition_list: attribute_list_opt recovery_push external_definition recovery_pop  */
#line 3425 "Parser/parser.yy"
                { distAttr( (yyvsp[-3].decl), (yyvsp[-1].decl) ); (yyval.decl) = (yyvsp[-1].decl); }
#line 14784 "Parser/parser.cc"
    break;

  case 833: /* external_definition_list: external_definition_list attribute_list_opt recovery_push external_definition recovery_pop  */
#line 3427 "Parser/parser.yy"
                { distAttr( (yyvsp[-3].decl), (yyvsp[-1].decl) ); (yyval.decl) = (yyvsp[-4].decl) ? (yyvsp[-4].decl)->set_last( (yyvsp[-1].decl) ) : (yyvsp[-1].decl)->addQualifiers( (yyvsp[-3].decl) ); }
#line 14790 "Parser/parser.cc"
    break;

  case 834: /* up: %empty  */
#line 3431 "Parser/parser.yy"
                { typedefTable.up( forall ); forall = false; }
#line 14796 "Parser/parser.cc"
    break;

  case 835: /* down: %empty  */
#line 3435 "Parser/parser.yy"
                { typedefTable.down(); }
#line 14802 "Parser/parser.cc"
    break;

  case 836: /* external_definition: DIRECTIVE  */
#line 3440 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newDirectiveStmt( new StatementNode( build_directive( (yyloc), (yyvsp[0].tok) ) ) ); }
#line 14808 "Parser/parser.cc"
    break;

  case 837: /* external_definition: declaration  */
#line 3442 "Parser/parser.yy"
                {
			// Variable declarations of anonymous types requires creating a unique type-name across multiple translation
			// unit, which is a dubious task, especially because C uses name rather than structural typing; hence it is
			// disallowed at the moment.
			if ( (yyvsp[0].decl)->linkage == ast::Linkage::Cforall && ! (yyvsp[0].decl)->storageClasses.is_static &&
				 (yyvsp[0].decl)->type && (yyvsp[0].decl)->type->kind == TypeData::AggregateInst ) {
				if ( (yyvsp[0].decl)->type->aggInst.aggregate->aggregate.anon ) {
					SemanticError( (yyloc), "extern anonymous aggregate is currently unimplemented." ); (yyval.decl) = nullptr;
				}
			}
		}
#line 14824 "Parser/parser.cc"
    break;

  case 838: /* external_definition: IDENTIFIER IDENTIFIER  */
#line 3454 "Parser/parser.yy"
                { IdentifierBeforeIdentifier( *(yyvsp[-1].tok).str, *(yyvsp[0].tok).str, " declaration" ); YYERROR; }
#line 14830 "Parser/parser.cc"
    break;

  case 839: /* external_definition: IDENTIFIER type_qualifier  */
#line 3456 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type qualifier" ); YYERROR; }
#line 14836 "Parser/parser.cc"
    break;

  case 840: /* external_definition: IDENTIFIER storage_class  */
#line 3458 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "storage class" ); YYERROR; }
#line 14842 "Parser/parser.cc"
    break;

  case 841: /* external_definition: IDENTIFIER basic_type_name  */
#line 3460 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type" ); YYERROR; }
#line 14848 "Parser/parser.cc"
    break;

  case 842: /* external_definition: IDENTIFIER TYPEDEFname  */
#line 3462 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type" ); YYERROR; }
#line 14854 "Parser/parser.cc"
    break;

  case 843: /* external_definition: IDENTIFIER TYPEGENname  */
#line 3464 "Parser/parser.yy"
                { IdentifierBeforeType( *(yyvsp[-1].tok).str, "type" ); YYERROR; }
#line 14860 "Parser/parser.cc"
    break;

  case 845: /* external_definition: EXTENSION external_definition  */
#line 3467 "Parser/parser.yy"
                {
			distExt( (yyvsp[0].decl) );								// mark all fields in list
			(yyval.decl) = (yyvsp[0].decl);
		}
#line 14869 "Parser/parser.cc"
    break;

  case 846: /* external_definition: ASM '(' string_literal ')' ';'  */
#line 3472 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newAsmStmt( new StatementNode( build_asm( (yyloc), false, (yyvsp[-2].expr), nullptr ) ) ); }
#line 14875 "Parser/parser.cc"
    break;

  case 847: /* $@10: %empty  */
#line 3474 "Parser/parser.yy"
                {
			linkageStack.push( linkage );				// handle nested extern "C"/"Cforall"
			linkage = ast::Linkage::update( (yyloc), linkage, (yyvsp[0].tok) );
		}
#line 14884 "Parser/parser.cc"
    break;

  case 848: /* external_definition: EXTERN STRINGliteral $@10 up external_definition down  */
#line 3479 "Parser/parser.yy"
                {
			linkage = linkageStack.top();
			linkageStack.pop();
			(yyval.decl) = (yyvsp[-1].decl);
		}
#line 14894 "Parser/parser.cc"
    break;

  case 849: /* $@11: %empty  */
#line 3485 "Parser/parser.yy"
                {
			linkageStack.push( linkage );				// handle nested extern "C"/"Cforall"
			linkage = ast::Linkage::update( (yyloc), linkage, (yyvsp[0].tok) );
		}
#line 14903 "Parser/parser.cc"
    break;

  case 850: /* external_definition: EXTERN STRINGliteral $@11 '{' up external_definition_list_opt down '}'  */
#line 3490 "Parser/parser.yy"
                {
			linkage = linkageStack.top();
			linkageStack.pop();
			(yyval.decl) = (yyvsp[-2].decl);
		}
#line 14913 "Parser/parser.cc"
    break;

  case 851: /* $@12: %empty  */
#line 3497 "Parser/parser.yy"
                {
			if ( (yyvsp[0].decl)->type->qualifiers.any() ) {
				SemanticError( (yyloc), "illegal syntax, CV qualifiers cannot be distributed; only storage-class and forall qualifiers." );
			}
			if ( (yyvsp[0].decl)->type->forall ) forall = true;		// remember generic type
		}
#line 14924 "Parser/parser.cc"
    break;

  case 852: /* external_definition: type_qualifier_list $@12 '{' up external_definition_list_opt down '}'  */
#line 3504 "Parser/parser.yy"
                {
			distQual( (yyvsp[-2].decl), (yyvsp[-6].decl) );
			forall = false;
			(yyval.decl) = (yyvsp[-2].decl);
		}
#line 14934 "Parser/parser.cc"
    break;

  case 853: /* $@13: %empty  */
#line 3510 "Parser/parser.yy"
                {
			if ( (yyvsp[0].decl)->type && (yyvsp[0].decl)->type->qualifiers.any() ) {
				SemanticError( (yyloc), "illegal syntax, CV qualifiers cannot be distributed; only storage-class and forall qualifiers." );
			}
			if ( (yyvsp[0].decl)->type && (yyvsp[0].decl)->type->forall ) forall = true; // remember generic type
		}
#line 14945 "Parser/parser.cc"
    break;

  case 854: /* external_definition: declaration_qualifier_list $@13 '{' up external_definition_list_opt down '}'  */
#line 3517 "Parser/parser.yy"
                {
			distQual( (yyvsp[-2].decl), (yyvsp[-6].decl) );
			forall = false;
			(yyval.decl) = (yyvsp[-2].decl);
		}
#line 14955 "Parser/parser.cc"
    break;

  case 855: /* $@14: %empty  */
#line 3523 "Parser/parser.yy"
                {
			if ( ((yyvsp[-1].decl)->type && (yyvsp[-1].decl)->type->qualifiers.any()) || ((yyvsp[0].decl)->type && (yyvsp[0].decl)->type->qualifiers.any()) ) {
				SemanticError( (yyloc), "illegal syntax, CV qualifiers cannot be distributed; only storage-class and forall qualifiers." );
			}
			if ( ((yyvsp[-1].decl)->type && (yyvsp[-1].decl)->type->forall) || ((yyvsp[0].decl)->type && (yyvsp[0].decl)->type->forall) ) forall = true; // remember generic type
		}
#line 14966 "Parser/parser.cc"
    break;

  case 856: /* external_definition: declaration_qualifier_list type_qualifier_list $@14 '{' up external_definition_list_opt down '}'  */
#line 3530 "Parser/parser.yy"
                {
			distQual( (yyvsp[-2].decl), (yyvsp[-7].decl)->addQualifiers( (yyvsp[-6].decl) ) );
			forall = false;
			(yyval.decl) = (yyvsp[-2].decl);
		}
#line 14976 "Parser/parser.cc"
    break;

  case 857: /* external_definition: ';'  */
#line 3536 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 14982 "Parser/parser.cc"
    break;

  case 858: /* external_definition: error  */
#line 3538 "Parser/parser.yy"
                { if ( ! LSP::enabled ) YYABORT; recoverFromSyntaxError(); (yyval.decl) = nullptr; }
#line 14988 "Parser/parser.cc"
    break;

  case 859: /* external_definition: error compound_statement  */
#line 3540 "Parser/parser.yy"
                { if ( ! LSP::enabled ) YYABORT; recoverFromSyntaxError(); (yyval.decl) = nullptr; }
#line 14994 "Parser/parser.cc"
    break;

  case 860: /* external_function_definition: function_definition  */
#line 3545 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[0].decl), (yyloc) ); }
#line 15000 "Parser/parser.cc"
    break;

  case 861: /* external_function_definition: function_declarator compound_statement  */
#line 3552 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-1].decl)->addFunctionBody( (yyvsp[0].stmt) ), (yyloc) ); }
#line 15006 "Parser/parser.cc"
    break;

  case 862: /* external_function_definition: KR_function_declarator KR_parameter_list_opt compound_statement  */
#line 3554 "Parser/parser.yy"
                { (yyval.decl) = setExtent( (yyvsp[-2].decl)->addOldDeclList( (yyvsp[-1].decl) )->addFunctionBody( (yyvsp[0].stmt) ), (yyloc) ); }
#line 15012 "Parser/parser.cc"
    break;

  case 863: /* with_clause_opt: %empty  */
#line 3559 "Parser/parser.yy"
                { (yyval.expr) = nullptr; forall = false; }
#line 15018 "Parser/parser.cc"
    break;

  case 864: /* with_clause_opt: WITH '(' type_list ')' attribute_list_opt  */
#line 3561 "Parser/parser.yy"
                {
			(yyval.expr) = (yyvsp[-2].expr); forall = false;
			if ( (yyvsp[0].decl) ) {
				SemanticError( (yyloc), "illegal syntax, attributes cannot be associated with function body. Move attribute(s) before \"with\" clause." );
				(yyval.expr) = nullptr;
			} // if
		}
#line 15030 "Parser/parser.cc"
    break;

  case 865: /* function_definition: cfa_function_declaration with_clause_opt compound_statement  */
#line 3572 "Parser/parser.yy"
                {
			// Add the function body to the last identifier in the function definition list, i.e., foo3:
			//   [const double] foo1(), foo2( int ), foo3( double ) { return 3.0; }
			(yyvsp[-2].decl)->get_last()->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) );
			(yyval.decl) = (yyvsp[-2].decl);
		}
#line 15041 "Parser/parser.cc"
    break;

  case 866: /* function_definition: declaration_specifier function_declarator with_clause_opt compound_statement  */
#line 3579 "Parser/parser.yy"
                {
			rebindForall( (yyvsp[-3].decl), (yyvsp[-2].decl) );
			(yyval.decl) = (yyvsp[-2].decl)->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addType( (yyvsp[-3].decl) );
		}
#line 15050 "Parser/parser.cc"
    break;

  case 867: /* function_definition: declaration_specifier function_type_redeclarator with_clause_opt compound_statement  */
#line 3584 "Parser/parser.yy"
                {
			rebindForall( (yyvsp[-3].decl), (yyvsp[-2].decl) );
			(yyval.decl) = (yyvsp[-2].decl)->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addType( (yyvsp[-3].decl) );
		}
#line 15059 "Parser/parser.cc"
    break;

  case 868: /* function_definition: type_qualifier_list function_declarator with_clause_opt compound_statement  */
#line 3590 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addQualifiers( (yyvsp[-3].decl) ); }
#line 15065 "Parser/parser.cc"
    break;

  case 869: /* function_definition: declaration_qualifier_list function_declarator with_clause_opt compound_statement  */
#line 3593 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addQualifiers( (yyvsp[-3].decl) ); }
#line 15071 "Parser/parser.cc"
    break;

  case 870: /* function_definition: declaration_qualifier_list type_qualifier_list function_declarator with_clause_opt compound_statement  */
#line 3596 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addQualifiers( (yyvsp[-3].decl) )->addQualifiers( (yyvsp[-4].decl) ); }
#line 15077 "Parser/parser.cc"
    break;

  case 871: /* function_definition: declaration_specifier KR_function_declarator KR_parameter_list_opt with_clause_opt compound_statement  */
#line 3600 "Parser/parser.yy"
                {
			rebindForall( (yyvsp[-4].decl), (yyvsp[-3].decl) );
			(yyval.decl) = (yyvsp[-3].decl)->addOldDeclList( (yyvsp[-2].decl) )->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addType( (yyvsp[-4].decl) );
		}
#line 15086 "Parser/parser.cc"
    break;

  case 872: /* function_definition: type_qualifier_list KR_function_declarator KR_parameter_list_opt with_clause_opt compound_statement  */
#line 3606 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addOldDeclList( (yyvsp[-2].decl) )->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addQualifiers( (yyvsp[-4].decl) ); }
#line 15092 "Parser/parser.cc"
    break;

  case 873: /* function_definition: declaration_qualifier_list KR_function_declarator KR_parameter_list_opt with_clause_opt compound_statement  */
#line 3609 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addOldDeclList( (yyvsp[-2].decl) )->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addQualifiers( (yyvsp[-4].decl) ); }
#line 15098 "Parser/parser.cc"
    break;

  case 874: /* function_definition: declaration_qualifier_list type_qualifier_list KR_function_declarator KR_parameter_list_opt with_clause_opt compound_statement  */
#line 3612 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addOldDeclList( (yyvsp[-2].decl) )->addFunctionBody( (yyvsp[0].stmt), (yyvsp[-1].expr) )->addQualifiers( (yyvsp[-4].decl) )->addQualifiers( (yyvsp[-5].decl) ); }
#line 15104 "Parser/parser.cc"
    break;

  case 879: /* subrange: constant_expression '~' constant_expression  */
#line 3624 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::RangeExpr( (yyloc), maybeMoveBuild( (yyvsp[-2].expr) ), maybeMoveBuild( (yyvsp[0].expr) ) ) ); }
#line 15110 "Parser/parser.cc"
    break;

  case 880: /* asm_name_opt: %empty  */
#line 3631 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 15116 "Parser/parser.cc"
    break;

  case 881: /* asm_name_opt: ASM '(' string_literal ')' attribute_list_opt  */
#line 3633 "Parser/parser.yy"
                {
			DeclarationNode * name = new DeclarationNode();
			name->asmName = maybeMoveBuild( (yyvsp[-2].expr) );
			(yyval.decl) = name->addQualifiers( (yyvsp[0].decl) );
		}
#line 15126 "Parser/parser.cc"
    break;

  case 882: /* attribute_list_opt: %empty  */
#line 3644 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 15132 "Parser/parser.cc"
    break;

  case 885: /* attribute_list: attribute_list attribute  */
#line 3651 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 15138 "Parser/parser.cc"
    break;

  case 886: /* attribute: ATTRIBUTE '(' '(' attribute_name_list ')' ')'  */
#line 3656 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl); }
#line 15144 "Parser/parser.cc"
    break;

  case 887: /* attribute: ATTRIBUTE '(' attribute_name_list ')'  */
#line 3658 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15150 "Parser/parser.cc"
    break;

  case 888: /* attribute: ATTR attribute_name_list ']'  */
#line 3660 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15156 "Parser/parser.cc"
    break;

  case 889: /* attribute: C23_ATTRIBUTE  */
#line 3662 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newAttribute( (yyvsp[0].tok) ); }
#line 15162 "Parser/parser.cc"
    break;

  case 891: /* attribute_name_list: attribute_name_list ',' attribute_name  */
#line 3668 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15168 "Parser/parser.cc"
    break;

  case 892: /* attribute_name: %empty  */
#line 3673 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 15174 "Parser/parser.cc"
    break;

  case 893: /* attribute_name: attr_name  */
#line 3675 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newAttribute( (yyvsp[0].tok) ); }
#line 15180 "Parser/parser.cc"
    break;

  case 894: /* attribute_name: attr_name '(' argument_expression_list_opt ')'  */
#line 3677 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newAttribute( (yyvsp[-3].tok), (yyvsp[-1].expr) ); }
#line 15186 "Parser/parser.cc"
    break;

  case 896: /* attr_name: FALLTHROUGH  */
#line 3683 "Parser/parser.yy"
                { (yyval.tok) = Token{ new string( "fallthrough" ), { nullptr, -1 } }; }
#line 15192 "Parser/parser.cc"
    break;

  case 897: /* attr_name: CONST  */
#line 3685 "Parser/parser.yy"
                { (yyval.tok) = Token{ new string( "__const__" ), { nullptr, -1 } }; }
#line 15198 "Parser/parser.cc"
    break;

  case 898: /* paren_identifier: identifier_at  */
#line 3720 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 15204 "Parser/parser.cc"
    break;

  case 899: /* paren_identifier: '?' identifier  */
#line 3723 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 15210 "Parser/parser.cc"
    break;

  case 900: /* paren_identifier: '(' paren_identifier ')'  */
#line 3725 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15216 "Parser/parser.cc"
    break;

  case 901: /* variable_declarator: paren_identifier attribute_list_opt  */
#line 3730 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15222 "Parser/parser.cc"
    break;

  case 903: /* variable_declarator: variable_array attribute_list_opt  */
#line 3733 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15228 "Parser/parser.cc"
    break;

  case 904: /* variable_declarator: variable_function attribute_list_opt  */
#line 3735 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15234 "Parser/parser.cc"
    break;

  case 905: /* variable_ptr: ptrref_operator variable_declarator  */
#line 3740 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15240 "Parser/parser.cc"
    break;

  case 906: /* variable_ptr: ptrref_operator attribute_list variable_declarator  */
#line 3742 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15246 "Parser/parser.cc"
    break;

  case 907: /* variable_ptr: ptrref_operator type_qualifier_list variable_declarator  */
#line 3744 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15252 "Parser/parser.cc"
    break;

  case 908: /* variable_ptr: '(' variable_ptr ')' attribute_list_opt  */
#line 3746 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15258 "Parser/parser.cc"
    break;

  case 909: /* variable_ptr: '(' attribute_list variable_ptr ')' attribute_list_opt  */
#line 3748 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15264 "Parser/parser.cc"
    break;

  case 910: /* variable_array: paren_identifier array_dimension  */
#line 3753 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addArray( (yyvsp[0].decl) ); }
#line 15270 "Parser/parser.cc"
    break;

  case 911: /* variable_array: '(' variable_ptr ')' array_dimension  */
#line 3755 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15276 "Parser/parser.cc"
    break;

  case 912: /* variable_array: '(' attribute_list variable_ptr ')' array_dimension  */
#line 3757 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15282 "Parser/parser.cc"
    break;

  case 913: /* variable_array: '(' variable_array ')' multi_array_dimension  */
#line 3759 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15288 "Parser/parser.cc"
    break;

  case 914: /* variable_array: '(' attribute_list variable_array ')' multi_array_dimension  */
#line 3761 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15294 "Parser/parser.cc"
    break;

  case 915: /* variable_array: '(' variable_array ')'  */
#line 3763 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15300 "Parser/parser.cc"
    break;

  case 916: /* variable_array: '(' attribute_list variable_array ')'  */
#line 3765 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15306 "Parser/parser.cc"
    break;

  case 917: /* variable_function: '(' variable_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3770 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15312 "Parser/parser.cc"
    break;

  case 918: /* variable_function: '(' attribute_list variable_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3772 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addQualifiers( (yyvsp[-5].decl) )->addParamList( (yyvsp[-1].decl) ); }
#line 15318 "Parser/parser.cc"
    break;

  case 919: /* variable_function: '(' variable_function ')'  */
#line 3774 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15324 "Parser/parser.cc"
    break;

  case 920: /* variable_function: '(' attribute_list variable_function ')'  */
#line 3776 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15330 "Parser/parser.cc"
    break;

  case 921: /* function_declarator: function_no_ptr attribute_list_opt  */
#line 3785 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15336 "Parser/parser.cc"
    break;

  case 923: /* function_declarator: function_array attribute_list_opt  */
#line 3788 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15342 "Parser/parser.cc"
    break;

  case 924: /* function_no_ptr: paren_identifier '(' parameter_list_ellipsis_opt ')'  */
#line 3793 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15348 "Parser/parser.cc"
    break;

  case 925: /* function_no_ptr: '(' function_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3795 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15354 "Parser/parser.cc"
    break;

  case 926: /* function_no_ptr: '(' attribute_list function_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3797 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addQualifiers( (yyvsp[-5].decl) )->addParamList( (yyvsp[-1].decl) ); }
#line 15360 "Parser/parser.cc"
    break;

  case 927: /* function_no_ptr: '(' function_no_ptr ')'  */
#line 3799 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15366 "Parser/parser.cc"
    break;

  case 928: /* function_no_ptr: '(' attribute_list function_no_ptr ')'  */
#line 3801 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15372 "Parser/parser.cc"
    break;

  case 929: /* function_ptr: ptrref_operator function_declarator  */
#line 3806 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15378 "Parser/parser.cc"
    break;

  case 930: /* function_ptr: ptrref_operator attribute_list function_declarator  */
#line 3808 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15384 "Parser/parser.cc"
    break;

  case 931: /* function_ptr: ptrref_operator type_qualifier_list function_declarator  */
#line 3810 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15390 "Parser/parser.cc"
    break;

  case 932: /* function_ptr: '(' function_ptr ')' attribute_list_opt  */
#line 3812 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15396 "Parser/parser.cc"
    break;

  case 933: /* function_ptr: '(' attribute_list function_ptr ')' attribute_list_opt  */
#line 3814 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15402 "Parser/parser.cc"
    break;

  case 934: /* function_array: '(' function_ptr ')' array_dimension  */
#line 3819 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15408 "Parser/parser.cc"
    break;

  case 935: /* function_array: '(' attribute_list function_ptr ')' array_dimension  */
#line 3821 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15414 "Parser/parser.cc"
    break;

  case 936: /* function_array: '(' function_array ')' multi_array_dimension  */
#line 3823 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15420 "Parser/parser.cc"
    break;

  case 937: /* function_array: '(' attribute_list function_array ')' multi_array_dimension  */
#line 3825 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15426 "Parser/parser.cc"
    break;

  case 938: /* function_array: '(' function_array ')'  */
#line 3827 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15432 "Parser/parser.cc"
    break;

  case 939: /* function_array: '(' attribute_list function_array ')'  */
#line 3829 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15438 "Parser/parser.cc"
    break;

  case 943: /* KR_function_no_ptr: paren_identifier '(' identifier_list ')'  */
#line 3847 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addIdList( (yyvsp[-1].decl) ); }
#line 15444 "Parser/parser.cc"
    break;

  case 944: /* KR_function_no_ptr: '(' KR_function_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3849 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15450 "Parser/parser.cc"
    break;

  case 945: /* KR_function_no_ptr: '(' attribute_list KR_function_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3851 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addQualifiers( (yyvsp[-5].decl) )->addParamList( (yyvsp[-1].decl) ); }
#line 15456 "Parser/parser.cc"
    break;

  case 946: /* KR_function_no_ptr: '(' KR_function_no_ptr ')'  */
#line 3853 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15462 "Parser/parser.cc"
    break;

  case 947: /* KR_function_no_ptr: '(' attribute_list KR_function_no_ptr ')'  */
#line 3855 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15468 "Parser/parser.cc"
    break;

  case 948: /* KR_function_ptr: ptrref_operator KR_function_declarator  */
#line 3860 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15474 "Parser/parser.cc"
    break;

  case 949: /* KR_function_ptr: ptrref_operator attribute_list KR_function_declarator  */
#line 3862 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15480 "Parser/parser.cc"
    break;

  case 950: /* KR_function_ptr: ptrref_operator type_qualifier_list KR_function_declarator  */
#line 3864 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15486 "Parser/parser.cc"
    break;

  case 951: /* KR_function_ptr: '(' KR_function_ptr ')'  */
#line 3866 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15492 "Parser/parser.cc"
    break;

  case 952: /* KR_function_ptr: '(' attribute_list KR_function_ptr ')'  */
#line 3868 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15498 "Parser/parser.cc"
    break;

  case 953: /* KR_function_array: '(' KR_function_ptr ')' array_dimension  */
#line 3873 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15504 "Parser/parser.cc"
    break;

  case 954: /* KR_function_array: '(' attribute_list KR_function_ptr ')' array_dimension  */
#line 3875 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15510 "Parser/parser.cc"
    break;

  case 955: /* KR_function_array: '(' KR_function_array ')' multi_array_dimension  */
#line 3877 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15516 "Parser/parser.cc"
    break;

  case 956: /* KR_function_array: '(' attribute_list KR_function_array ')' multi_array_dimension  */
#line 3879 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15522 "Parser/parser.cc"
    break;

  case 957: /* KR_function_array: '(' KR_function_array ')'  */
#line 3881 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15528 "Parser/parser.cc"
    break;

  case 958: /* KR_function_array: '(' attribute_list KR_function_array ')'  */
#line 3883 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15534 "Parser/parser.cc"
    break;

  case 959: /* paren_type: typedef_name  */
#line 3895 "Parser/parser.yy"
                {
			// hide type name in enclosing scope by variable name
			typedefTable.addToEnclosingScope( *(yyvsp[0].decl)->name, IDENTIFIER, "paren_type" );
		}
#line 15543 "Parser/parser.cc"
    break;

  case 960: /* paren_type: '(' paren_type ')'  */
#line 3900 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15549 "Parser/parser.cc"
    break;

  case 961: /* variable_type_redeclarator: paren_type attribute_list_opt  */
#line 3905 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15555 "Parser/parser.cc"
    break;

  case 963: /* variable_type_redeclarator: variable_type_array attribute_list_opt  */
#line 3908 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15561 "Parser/parser.cc"
    break;

  case 964: /* variable_type_redeclarator: variable_type_function attribute_list_opt  */
#line 3910 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15567 "Parser/parser.cc"
    break;

  case 965: /* variable_type_ptr: ptrref_operator variable_type_redeclarator  */
#line 3915 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15573 "Parser/parser.cc"
    break;

  case 966: /* variable_type_ptr: ptrref_operator attribute_list variable_type_redeclarator  */
#line 3917 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15579 "Parser/parser.cc"
    break;

  case 967: /* variable_type_ptr: ptrref_operator type_qualifier_list variable_type_redeclarator  */
#line 3919 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15585 "Parser/parser.cc"
    break;

  case 968: /* variable_type_ptr: '(' variable_type_ptr ')' attribute_list_opt  */
#line 3921 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15591 "Parser/parser.cc"
    break;

  case 969: /* variable_type_ptr: '(' attribute_list variable_type_ptr ')' attribute_list_opt  */
#line 3923 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15597 "Parser/parser.cc"
    break;

  case 970: /* variable_type_array: paren_type array_dimension  */
#line 3928 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addArray( (yyvsp[0].decl) ); }
#line 15603 "Parser/parser.cc"
    break;

  case 971: /* variable_type_array: '(' variable_type_ptr ')' array_dimension  */
#line 3930 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15609 "Parser/parser.cc"
    break;

  case 972: /* variable_type_array: '(' attribute_list variable_type_ptr ')' array_dimension  */
#line 3932 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15615 "Parser/parser.cc"
    break;

  case 973: /* variable_type_array: '(' variable_type_array ')' multi_array_dimension  */
#line 3934 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15621 "Parser/parser.cc"
    break;

  case 974: /* variable_type_array: '(' attribute_list variable_type_array ')' multi_array_dimension  */
#line 3936 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15627 "Parser/parser.cc"
    break;

  case 975: /* variable_type_array: '(' variable_type_array ')'  */
#line 3938 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15633 "Parser/parser.cc"
    break;

  case 976: /* variable_type_array: '(' attribute_list variable_type_array ')'  */
#line 3940 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15639 "Parser/parser.cc"
    break;

  case 977: /* variable_type_function: '(' variable_type_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3945 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15645 "Parser/parser.cc"
    break;

  case 978: /* variable_type_function: '(' attribute_list variable_type_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3947 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addQualifiers( (yyvsp[-5].decl) )->addParamList( (yyvsp[-1].decl) ); }
#line 15651 "Parser/parser.cc"
    break;

  case 979: /* variable_type_function: '(' variable_type_function ')'  */
#line 3949 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15657 "Parser/parser.cc"
    break;

  case 980: /* variable_type_function: '(' attribute_list variable_type_function ')'  */
#line 3951 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15663 "Parser/parser.cc"
    break;

  case 981: /* function_type_redeclarator: function_type_no_ptr attribute_list_opt  */
#line 3960 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15669 "Parser/parser.cc"
    break;

  case 983: /* function_type_redeclarator: function_type_array attribute_list_opt  */
#line 3963 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15675 "Parser/parser.cc"
    break;

  case 984: /* function_type_no_ptr: paren_type '(' parameter_list_ellipsis_opt ')'  */
#line 3968 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15681 "Parser/parser.cc"
    break;

  case 985: /* function_type_no_ptr: '(' function_type_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3970 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15687 "Parser/parser.cc"
    break;

  case 986: /* function_type_no_ptr: '(' attribute_list function_type_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 3972 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addQualifiers( (yyvsp[-5].decl) )->addParamList( (yyvsp[-1].decl) ); }
#line 15693 "Parser/parser.cc"
    break;

  case 987: /* function_type_no_ptr: '(' function_type_no_ptr ')'  */
#line 3974 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15699 "Parser/parser.cc"
    break;

  case 988: /* function_type_no_ptr: '(' attribute_list function_type_no_ptr ')'  */
#line 3976 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15705 "Parser/parser.cc"
    break;

  case 989: /* function_type_ptr: ptrref_operator function_type_redeclarator  */
#line 3981 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15711 "Parser/parser.cc"
    break;

  case 990: /* function_type_ptr: ptrref_operator attribute_list function_type_redeclarator  */
#line 3983 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15717 "Parser/parser.cc"
    break;

  case 991: /* function_type_ptr: ptrref_operator type_qualifier_list function_type_redeclarator  */
#line 3985 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15723 "Parser/parser.cc"
    break;

  case 992: /* function_type_ptr: '(' function_type_ptr ')' attribute_list_opt  */
#line 3987 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15729 "Parser/parser.cc"
    break;

  case 993: /* function_type_ptr: '(' attribute_list function_type_ptr ')' attribute_list_opt  */
#line 3989 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15735 "Parser/parser.cc"
    break;

  case 994: /* function_type_array: '(' function_type_ptr ')' array_dimension  */
#line 3994 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15741 "Parser/parser.cc"
    break;

  case 995: /* function_type_array: '(' attribute_list function_type_ptr ')' array_dimension  */
#line 3996 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15747 "Parser/parser.cc"
    break;

  case 996: /* function_type_array: '(' function_type_array ')' multi_array_dimension  */
#line 3998 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15753 "Parser/parser.cc"
    break;

  case 997: /* function_type_array: '(' attribute_list function_type_array ')' multi_array_dimension  */
#line 4000 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) )->addArray( (yyvsp[0].decl) ); }
#line 15759 "Parser/parser.cc"
    break;

  case 998: /* function_type_array: '(' function_type_array ')'  */
#line 4002 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15765 "Parser/parser.cc"
    break;

  case 999: /* function_type_array: '(' attribute_list function_type_array ')'  */
#line 4004 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[-2].decl) ); }
#line 15771 "Parser/parser.cc"
    break;

  case 1000: /* identifier_parameter_declarator: paren_identifier attribute_list_opt  */
#line 4014 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15777 "Parser/parser.cc"
    break;

  case 1001: /* identifier_parameter_declarator: '&' MUTEX paren_identifier attribute_list_opt  */
#line 4016 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addPointer( DeclarationNode::newPointer( DeclarationNode::newFromTypeData( build_type_qualifier( ast::CV::Mutex ) ),
															OperKinds::AddressOf ) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15784 "Parser/parser.cc"
    break;

  case 1003: /* identifier_parameter_declarator: identifier_parameter_array attribute_list_opt  */
#line 4020 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15790 "Parser/parser.cc"
    break;

  case 1004: /* identifier_parameter_declarator: identifier_parameter_function attribute_list_opt  */
#line 4022 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15796 "Parser/parser.cc"
    break;

  case 1005: /* identifier_parameter_ptr: ptrref_operator identifier_parameter_declarator  */
#line 4027 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15802 "Parser/parser.cc"
    break;

  case 1006: /* identifier_parameter_ptr: ptrref_operator attribute_list identifier_parameter_declarator  */
#line 4029 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15808 "Parser/parser.cc"
    break;

  case 1007: /* identifier_parameter_ptr: ptrref_operator type_qualifier_list identifier_parameter_declarator  */
#line 4031 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15814 "Parser/parser.cc"
    break;

  case 1008: /* identifier_parameter_ptr: '(' identifier_parameter_ptr ')' attribute_list_opt  */
#line 4033 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15820 "Parser/parser.cc"
    break;

  case 1009: /* identifier_parameter_array: paren_identifier array_parameter_dimension  */
#line 4038 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addArray( (yyvsp[0].decl) ); }
#line 15826 "Parser/parser.cc"
    break;

  case 1010: /* identifier_parameter_array: '(' identifier_parameter_ptr ')' array_dimension  */
#line 4040 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15832 "Parser/parser.cc"
    break;

  case 1011: /* identifier_parameter_array: '(' identifier_parameter_array ')' multi_array_dimension  */
#line 4042 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15838 "Parser/parser.cc"
    break;

  case 1012: /* identifier_parameter_array: '(' identifier_parameter_array ')'  */
#line 4044 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15844 "Parser/parser.cc"
    break;

  case 1013: /* identifier_parameter_function: paren_identifier '(' parameter_list_ellipsis_opt ')'  */
#line 4049 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15850 "Parser/parser.cc"
    break;

  case 1014: /* identifier_parameter_function: '(' identifier_parameter_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 4051 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15856 "Parser/parser.cc"
    break;

  case 1015: /* identifier_parameter_function: '(' identifier_parameter_function ')'  */
#line 4053 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 15862 "Parser/parser.cc"
    break;

  case 1016: /* type_parameter_redeclarator: typedef_name attribute_list_opt  */
#line 4067 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15868 "Parser/parser.cc"
    break;

  case 1017: /* type_parameter_redeclarator: '&' MUTEX typedef_name attribute_list_opt  */
#line 4069 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addPointer( DeclarationNode::newPointer( DeclarationNode::newFromTypeData( build_type_qualifier( ast::CV::Mutex ) ),
															OperKinds::AddressOf ) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15875 "Parser/parser.cc"
    break;

  case 1019: /* type_parameter_redeclarator: type_parameter_array attribute_list_opt  */
#line 4073 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15881 "Parser/parser.cc"
    break;

  case 1020: /* type_parameter_redeclarator: type_parameter_function attribute_list_opt  */
#line 4075 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15887 "Parser/parser.cc"
    break;

  case 1021: /* typedef_name: TYPEDEFname  */
#line 4080 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 15893 "Parser/parser.cc"
    break;

  case 1022: /* typedef_name: TYPEGENname  */
#line 4082 "Parser/parser.yy"
                { (yyval.decl) = setNameLoc( DeclarationNode::newName( (yyvsp[0].tok) ), (yylsp[0]) ); }
#line 15899 "Parser/parser.cc"
    break;

  case 1023: /* type_parameter_ptr: ptrref_operator type_parameter_redeclarator  */
#line 4087 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15905 "Parser/parser.cc"
    break;

  case 1024: /* type_parameter_ptr: ptrref_operator attribute_list type_parameter_redeclarator  */
#line 4089 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 15911 "Parser/parser.cc"
    break;

  case 1025: /* type_parameter_ptr: ptrref_operator type_qualifier_list type_parameter_redeclarator  */
#line 4091 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15917 "Parser/parser.cc"
    break;

  case 1026: /* type_parameter_ptr: '(' type_parameter_ptr ')' attribute_list_opt  */
#line 4093 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15923 "Parser/parser.cc"
    break;

  case 1027: /* type_parameter_array: typedef_name array_parameter_dimension  */
#line 4098 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addArray( (yyvsp[0].decl) ); }
#line 15929 "Parser/parser.cc"
    break;

  case 1028: /* type_parameter_array: '(' type_parameter_ptr ')' array_parameter_dimension  */
#line 4100 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 15935 "Parser/parser.cc"
    break;

  case 1029: /* type_parameter_function: typedef_name '(' parameter_list_ellipsis_opt ')'  */
#line 4105 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15941 "Parser/parser.cc"
    break;

  case 1030: /* type_parameter_function: '(' type_parameter_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 4107 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 15947 "Parser/parser.cc"
    break;

  case 1032: /* abstract_declarator: abstract_array attribute_list_opt  */
#line 4125 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15953 "Parser/parser.cc"
    break;

  case 1033: /* abstract_declarator: abstract_function attribute_list_opt  */
#line 4127 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15959 "Parser/parser.cc"
    break;

  case 1034: /* abstract_ptr: ptrref_operator attribute_list_opt  */
#line 4132 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) )->addQualifiers( (yyvsp[0].decl) ); }
#line 15965 "Parser/parser.cc"
    break;

  case 1035: /* abstract_ptr: ptrref_operator type_qualifier_list  */
#line 4134 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( (yyvsp[0].decl), (yyvsp[-1].oper) ); }
#line 15971 "Parser/parser.cc"
    break;

  case 1036: /* abstract_ptr: ptrref_operator abstract_declarator  */
#line 4136 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 15977 "Parser/parser.cc"
    break;

  case 1037: /* abstract_ptr: ptrref_operator attribute_list abstract_declarator  */
#line 4138 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) )->addQualifiers( (yyvsp[-1].decl) ) ); }
#line 15983 "Parser/parser.cc"
    break;

  case 1038: /* abstract_ptr: ptrref_operator type_qualifier_list abstract_declarator  */
#line 4140 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 15989 "Parser/parser.cc"
    break;

  case 1039: /* abstract_ptr: '(' abstract_ptr ')' attribute_list_opt  */
#line 4142 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 15995 "Parser/parser.cc"
    break;

  case 1041: /* abstract_array: '(' abstract_ptr ')' array_dimension  */
#line 4148 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 16001 "Parser/parser.cc"
    break;

  case 1042: /* abstract_array: '(' abstract_array ')' multi_array_dimension  */
#line 4150 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 16007 "Parser/parser.cc"
    break;

  case 1043: /* abstract_array: '(' abstract_array ')'  */
#line 4152 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 16013 "Parser/parser.cc"
    break;

  case 1044: /* abstract_function: '(' parameter_list_ellipsis_opt ')'  */
#line 4157 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFunction( nullptr, nullptr, (yyvsp[-1].decl), nullptr ); }
#line 16019 "Parser/parser.cc"
    break;

  case 1045: /* abstract_function: '(' abstract_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 4159 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 16025 "Parser/parser.cc"
    break;

  case 1046: /* abstract_function: '(' abstract_function ')'  */
#line 4161 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 16031 "Parser/parser.cc"
    break;

  case 1047: /* array_dimension: '[' ']'  */
#line 4167 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( nullptr, nullptr, false ); }
#line 16037 "Parser/parser.cc"
    break;

  case 1048: /* array_dimension: '[' ']' multi_array_dimension  */
#line 4169 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( nullptr, nullptr, false )->addArray( (yyvsp[0].decl) ); }
#line 16043 "Parser/parser.cc"
    break;

  case 1049: /* array_dimension: '[' assignment_expression ',' ']'  */
#line 4172 "Parser/parser.yy"
                { SemanticError( (yyloc), "New array dimension is currently unimplemented." ); (yyval.decl) = nullptr; }
#line 16049 "Parser/parser.cc"
    break;

  case 1050: /* array_dimension: '[' assignment_expression ',' comma_expression ']'  */
#line 4175 "Parser/parser.yy"
                { SemanticError( (yyloc), "New array dimension is currently unimplemented." ); (yyval.decl) = nullptr; }
#line 16055 "Parser/parser.cc"
    break;

  case 1051: /* array_dimension: '[' array_type_list ']'  */
#line 4182 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-1].expr), nullptr, false ); }
#line 16061 "Parser/parser.cc"
    break;

  case 1053: /* array_type_list: basic_type_name  */
#line 4193 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[0].decl) ) ) ); }
#line 16067 "Parser/parser.cc"
    break;

  case 1054: /* array_type_list: type_name  */
#line 4195 "Parser/parser.yy"
                { (yyval.expr) = new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[0].type) ) ) ); }
#line 16073 "Parser/parser.cc"
    break;

  case 1056: /* array_type_list: array_type_list ',' basic_type_name  */
#line 4198 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[0].decl) ) ) ) ); }
#line 16079 "Parser/parser.cc"
    break;

  case 1057: /* array_type_list: array_type_list ',' type_name  */
#line 4200 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[-2].expr)->set_last( new ExpressionNode( new ast::TypeExpr( (yyloc), maybeMoveBuildType( (yyvsp[0].type) ) ) ) ); }
#line 16085 "Parser/parser.cc"
    break;

  case 1059: /* upupeq: '~'  */
#line 4206 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::LThan; }
#line 16091 "Parser/parser.cc"
    break;

  case 1060: /* upupeq: ErangeUpLe  */
#line 4208 "Parser/parser.yy"
                { (yyval.oper) = OperKinds::LEThan; }
#line 16097 "Parser/parser.cc"
    break;

  case 1061: /* multi_array_dimension: '[' assignment_expression ']'  */
#line 4213 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-1].expr), nullptr, false ); }
#line 16103 "Parser/parser.cc"
    break;

  case 1062: /* multi_array_dimension: '[' '*' ']'  */
#line 4215 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newVarArray( 0 ); }
#line 16109 "Parser/parser.cc"
    break;

  case 1063: /* multi_array_dimension: multi_array_dimension '[' assignment_expression ']'  */
#line 4217 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addArray( DeclarationNode::newArray( (yyvsp[-1].expr), nullptr, false ) ); }
#line 16115 "Parser/parser.cc"
    break;

  case 1064: /* multi_array_dimension: multi_array_dimension '[' '*' ']'  */
#line 4219 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-3].decl)->addArray( DeclarationNode::newVarArray( 0 ) ); }
#line 16121 "Parser/parser.cc"
    break;

  case 1065: /* abstract_parameter_declarator_opt: %empty  */
#line 4253 "Parser/parser.yy"
                { (yyval.decl) = nullptr; }
#line 16127 "Parser/parser.cc"
    break;

  case 1068: /* abstract_parameter_declarator: '&' MUTEX attribute_list_opt  */
#line 4260 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( DeclarationNode::newFromTypeData( build_type_qualifier( ast::CV::Mutex ) ),
											OperKinds::AddressOf )->addQualifiers( (yyvsp[0].decl) ); }
#line 16134 "Parser/parser.cc"
    break;

  case 1069: /* abstract_parameter_declarator: abstract_parameter_array attribute_list_opt  */
#line 4263 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 16140 "Parser/parser.cc"
    break;

  case 1070: /* abstract_parameter_declarator: abstract_parameter_function attribute_list_opt  */
#line 4265 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 16146 "Parser/parser.cc"
    break;

  case 1071: /* abstract_parameter_ptr: ptrref_operator attribute_list_opt  */
#line 4270 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) )->addQualifiers( (yyvsp[0].decl) ); }
#line 16152 "Parser/parser.cc"
    break;

  case 1072: /* abstract_parameter_ptr: ptrref_operator type_qualifier_list  */
#line 4272 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( (yyvsp[0].decl), (yyvsp[-1].oper) ); }
#line 16158 "Parser/parser.cc"
    break;

  case 1073: /* abstract_parameter_ptr: ptrref_operator abstract_parameter_declarator  */
#line 4274 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16164 "Parser/parser.cc"
    break;

  case 1074: /* abstract_parameter_ptr: ptrref_operator type_qualifier_list abstract_parameter_declarator  */
#line 4276 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 16170 "Parser/parser.cc"
    break;

  case 1075: /* abstract_parameter_ptr: '(' abstract_parameter_ptr ')' attribute_list_opt  */
#line 4278 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 16176 "Parser/parser.cc"
    break;

  case 1077: /* abstract_parameter_array: '(' abstract_parameter_ptr ')' array_parameter_dimension  */
#line 4284 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 16182 "Parser/parser.cc"
    break;

  case 1078: /* abstract_parameter_array: '(' abstract_parameter_array ')' multi_array_dimension  */
#line 4286 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 16188 "Parser/parser.cc"
    break;

  case 1079: /* abstract_parameter_array: '(' abstract_parameter_array ')'  */
#line 4288 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 16194 "Parser/parser.cc"
    break;

  case 1080: /* abstract_parameter_function: '(' parameter_list_ellipsis_opt ')'  */
#line 4293 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFunction( nullptr, nullptr, (yyvsp[-1].decl), nullptr ); }
#line 16200 "Parser/parser.cc"
    break;

  case 1081: /* abstract_parameter_function: '(' abstract_parameter_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 4295 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 16206 "Parser/parser.cc"
    break;

  case 1082: /* abstract_parameter_function: '(' abstract_parameter_function ')'  */
#line 4297 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 16212 "Parser/parser.cc"
    break;

  case 1084: /* array_parameter_dimension: array_parameter_1st_dimension multi_array_dimension  */
#line 4304 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addArray( (yyvsp[0].decl) ); }
#line 16218 "Parser/parser.cc"
    break;

  case 1086: /* array_parameter_1st_dimension: '[' ']'  */
#line 4315 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( nullptr, nullptr, false ); }
#line 16224 "Parser/parser.cc"
    break;

  case 1087: /* array_parameter_1st_dimension: '[' push type_qualifier_list '*' pop ']'  */
#line 4318 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newVarArray( (yyvsp[-3].decl) ); }
#line 16230 "Parser/parser.cc"
    break;

  case 1088: /* array_parameter_1st_dimension: '[' push type_qualifier_list pop ']'  */
#line 4320 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( nullptr, (yyvsp[-2].decl), false ); }
#line 16236 "Parser/parser.cc"
    break;

  case 1089: /* array_parameter_1st_dimension: '[' push type_qualifier_list assignment_expression pop ']'  */
#line 4323 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-2].expr), (yyvsp[-3].decl), false ); }
#line 16242 "Parser/parser.cc"
    break;

  case 1090: /* array_parameter_1st_dimension: '[' push STATIC type_qualifier_list_opt assignment_expression pop ']'  */
#line 4325 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-2].expr), (yyvsp[-3].decl), true ); }
#line 16248 "Parser/parser.cc"
    break;

  case 1091: /* array_parameter_1st_dimension: '[' push type_qualifier_list STATIC assignment_expression pop ']'  */
#line 4327 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-2].expr), (yyvsp[-4].decl), true ); }
#line 16254 "Parser/parser.cc"
    break;

  case 1093: /* variable_abstract_declarator: variable_abstract_array attribute_list_opt  */
#line 4342 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 16260 "Parser/parser.cc"
    break;

  case 1094: /* variable_abstract_declarator: variable_abstract_function attribute_list_opt  */
#line 4344 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 16266 "Parser/parser.cc"
    break;

  case 1095: /* variable_abstract_ptr: ptrref_operator attribute_list_opt  */
#line 4349 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) )->addQualifiers( (yyvsp[0].decl) ); }
#line 16272 "Parser/parser.cc"
    break;

  case 1096: /* variable_abstract_ptr: ptrref_operator type_qualifier_list  */
#line 4351 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newPointer( (yyvsp[0].decl), (yyvsp[-1].oper) ); }
#line 16278 "Parser/parser.cc"
    break;

  case 1097: /* variable_abstract_ptr: ptrref_operator variable_abstract_declarator  */
#line 4353 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16284 "Parser/parser.cc"
    break;

  case 1098: /* variable_abstract_ptr: ptrref_operator type_qualifier_list variable_abstract_declarator  */
#line 4355 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addPointer( DeclarationNode::newPointer( (yyvsp[-1].decl), (yyvsp[-2].oper) ) ); }
#line 16290 "Parser/parser.cc"
    break;

  case 1099: /* variable_abstract_ptr: '(' variable_abstract_ptr ')' attribute_list_opt  */
#line 4357 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addQualifiers( (yyvsp[0].decl) ); }
#line 16296 "Parser/parser.cc"
    break;

  case 1101: /* variable_abstract_array: '(' variable_abstract_ptr ')' array_dimension  */
#line 4363 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 16302 "Parser/parser.cc"
    break;

  case 1102: /* variable_abstract_array: '(' variable_abstract_array ')' multi_array_dimension  */
#line 4365 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-2].decl)->addArray( (yyvsp[0].decl) ); }
#line 16308 "Parser/parser.cc"
    break;

  case 1103: /* variable_abstract_array: '(' variable_abstract_array ')'  */
#line 4367 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 16314 "Parser/parser.cc"
    break;

  case 1104: /* variable_abstract_function: '(' variable_abstract_ptr ')' '(' parameter_list_ellipsis_opt ')'  */
#line 4372 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-4].decl)->addParamList( (yyvsp[-1].decl) ); }
#line 16320 "Parser/parser.cc"
    break;

  case 1105: /* variable_abstract_function: '(' variable_abstract_function ')'  */
#line 4374 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[-1].decl); }
#line 16326 "Parser/parser.cc"
    break;

  case 1108: /* cfa_identifier_parameter_declarator_tuple: type_qualifier_list cfa_abstract_tuple  */
#line 4384 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 16332 "Parser/parser.cc"
    break;

  case 1111: /* cfa_identifier_parameter_declarator_no_tuple: type_qualifier_list cfa_identifier_parameter_array  */
#line 4391 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 16338 "Parser/parser.cc"
    break;

  case 1112: /* cfa_identifier_parameter_ptr: ptrref_operator type_specifier_nobody  */
#line 4397 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16344 "Parser/parser.cc"
    break;

  case 1113: /* cfa_identifier_parameter_ptr: ptrref_operator attribute_list type_specifier_nobody  */
#line 4399 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 16350 "Parser/parser.cc"
    break;

  case 1114: /* cfa_identifier_parameter_ptr: type_qualifier_list ptrref_operator type_specifier_nobody  */
#line 4401 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( (yyvsp[-2].decl), (yyvsp[-1].oper) ) ); }
#line 16356 "Parser/parser.cc"
    break;

  case 1115: /* cfa_identifier_parameter_ptr: ptrref_operator cfa_abstract_function  */
#line 4403 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16362 "Parser/parser.cc"
    break;

  case 1116: /* cfa_identifier_parameter_ptr: type_qualifier_list ptrref_operator cfa_abstract_function  */
#line 4405 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( (yyvsp[-2].decl), (yyvsp[-1].oper) ) ); }
#line 16368 "Parser/parser.cc"
    break;

  case 1117: /* cfa_identifier_parameter_ptr: ptrref_operator cfa_identifier_parameter_declarator_tuple  */
#line 4407 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16374 "Parser/parser.cc"
    break;

  case 1118: /* cfa_identifier_parameter_ptr: type_qualifier_list ptrref_operator cfa_identifier_parameter_declarator_tuple  */
#line 4409 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( (yyvsp[-2].decl), (yyvsp[-1].oper) ) ); }
#line 16380 "Parser/parser.cc"
    break;

  case 1119: /* cfa_identifier_parameter_array: '[' ']' type_specifier_nobody  */
#line 4416 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16386 "Parser/parser.cc"
    break;

  case 1120: /* cfa_identifier_parameter_array: '[' ']' cfa_abstract_tuple  */
#line 4418 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16392 "Parser/parser.cc"
    break;

  case 1121: /* cfa_identifier_parameter_array: cfa_array_parameter_1st_dimension type_specifier_nobody  */
#line 4420 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16398 "Parser/parser.cc"
    break;

  case 1122: /* cfa_identifier_parameter_array: cfa_array_parameter_1st_dimension cfa_abstract_tuple  */
#line 4422 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16404 "Parser/parser.cc"
    break;

  case 1123: /* cfa_identifier_parameter_array: '[' ']' multi_array_dimension type_specifier_nobody  */
#line 4424 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16410 "Parser/parser.cc"
    break;

  case 1124: /* cfa_identifier_parameter_array: '[' ']' multi_array_dimension cfa_abstract_tuple  */
#line 4426 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16416 "Parser/parser.cc"
    break;

  case 1125: /* cfa_identifier_parameter_array: cfa_array_parameter_1st_dimension multi_array_dimension type_specifier_nobody  */
#line 4428 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( (yyvsp[-2].decl) ); }
#line 16422 "Parser/parser.cc"
    break;

  case 1126: /* cfa_identifier_parameter_array: cfa_array_parameter_1st_dimension multi_array_dimension cfa_abstract_tuple  */
#line 4430 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( (yyvsp[-2].decl) ); }
#line 16428 "Parser/parser.cc"
    break;

  case 1127: /* cfa_identifier_parameter_array: multi_array_dimension type_specifier_nobody  */
#line 4432 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16434 "Parser/parser.cc"
    break;

  case 1128: /* cfa_identifier_parameter_array: multi_array_dimension cfa_abstract_tuple  */
#line 4434 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16440 "Parser/parser.cc"
    break;

  case 1129: /* cfa_identifier_parameter_array: '[' ']' cfa_identifier_parameter_ptr  */
#line 4437 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16446 "Parser/parser.cc"
    break;

  case 1130: /* cfa_identifier_parameter_array: cfa_array_parameter_1st_dimension cfa_identifier_parameter_ptr  */
#line 4439 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16452 "Parser/parser.cc"
    break;

  case 1131: /* cfa_identifier_parameter_array: '[' ']' multi_array_dimension cfa_identifier_parameter_ptr  */
#line 4441 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16458 "Parser/parser.cc"
    break;

  case 1132: /* cfa_identifier_parameter_array: cfa_array_parameter_1st_dimension multi_array_dimension cfa_identifier_parameter_ptr  */
#line 4443 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( (yyvsp[-2].decl) ); }
#line 16464 "Parser/parser.cc"
    break;

  case 1133: /* cfa_identifier_parameter_array: multi_array_dimension cfa_identifier_parameter_ptr  */
#line 4445 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16470 "Parser/parser.cc"
    break;

  case 1134: /* cfa_array_parameter_1st_dimension: '[' type_qualifier_list '*' ']'  */
#line 4450 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newVarArray( (yyvsp[-2].decl) ); }
#line 16476 "Parser/parser.cc"
    break;

  case 1135: /* cfa_array_parameter_1st_dimension: '[' type_qualifier_list assignment_expression ']'  */
#line 4452 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-1].expr), (yyvsp[-2].decl), false ); }
#line 16482 "Parser/parser.cc"
    break;

  case 1136: /* cfa_array_parameter_1st_dimension: '[' declaration_qualifier_list assignment_expression ']'  */
#line 4457 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-1].expr), (yyvsp[-2].decl), true ); }
#line 16488 "Parser/parser.cc"
    break;

  case 1137: /* cfa_array_parameter_1st_dimension: '[' declaration_qualifier_list type_qualifier_list assignment_expression ']'  */
#line 4459 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newArray( (yyvsp[-1].expr), (yyvsp[-2].decl)->addQualifiers( (yyvsp[-3].decl) ), true ); }
#line 16494 "Parser/parser.cc"
    break;

  case 1139: /* cfa_abstract_declarator_tuple: type_qualifier_list cfa_abstract_tuple  */
#line 4486 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addQualifiers( (yyvsp[-1].decl) ); }
#line 16500 "Parser/parser.cc"
    break;

  case 1143: /* cfa_abstract_ptr: ptrref_operator type_specifier  */
#line 4497 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16506 "Parser/parser.cc"
    break;

  case 1144: /* cfa_abstract_ptr: ptrref_operator attribute_list type_specifier  */
#line 4499 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-2].oper) ) )->addQualifiers( (yyvsp[-1].decl) ); }
#line 16512 "Parser/parser.cc"
    break;

  case 1145: /* cfa_abstract_ptr: type_qualifier_list ptrref_operator type_specifier  */
#line 4501 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( (yyvsp[-2].decl), (yyvsp[-1].oper) ) ); }
#line 16518 "Parser/parser.cc"
    break;

  case 1146: /* cfa_abstract_ptr: ptrref_operator cfa_abstract_function  */
#line 4503 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16524 "Parser/parser.cc"
    break;

  case 1147: /* cfa_abstract_ptr: type_qualifier_list ptrref_operator cfa_abstract_function  */
#line 4505 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( (yyvsp[-2].decl), (yyvsp[-1].oper) ) ); }
#line 16530 "Parser/parser.cc"
    break;

  case 1148: /* cfa_abstract_ptr: ptrref_operator cfa_abstract_declarator_tuple  */
#line 4507 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( nullptr, (yyvsp[-1].oper) ) ); }
#line 16536 "Parser/parser.cc"
    break;

  case 1149: /* cfa_abstract_ptr: type_qualifier_list ptrref_operator cfa_abstract_declarator_tuple  */
#line 4509 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewPointer( DeclarationNode::newPointer( (yyvsp[-2].decl), (yyvsp[-1].oper) ) ); }
#line 16542 "Parser/parser.cc"
    break;

  case 1150: /* cfa_abstract_array: '[' ']' type_specifier  */
#line 4516 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16548 "Parser/parser.cc"
    break;

  case 1151: /* cfa_abstract_array: '[' ']' multi_array_dimension type_specifier  */
#line 4518 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16554 "Parser/parser.cc"
    break;

  case 1152: /* cfa_abstract_array: multi_array_dimension type_specifier  */
#line 4520 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16560 "Parser/parser.cc"
    break;

  case 1153: /* cfa_abstract_array: '[' ']' cfa_abstract_ptr  */
#line 4522 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16566 "Parser/parser.cc"
    break;

  case 1154: /* cfa_abstract_array: '[' ']' multi_array_dimension cfa_abstract_ptr  */
#line 4524 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) )->addNewArray( DeclarationNode::newArray( nullptr, nullptr, false ) ); }
#line 16572 "Parser/parser.cc"
    break;

  case 1155: /* cfa_abstract_array: multi_array_dimension cfa_abstract_ptr  */
#line 4526 "Parser/parser.yy"
                { (yyval.decl) = (yyvsp[0].decl)->addNewArray( (yyvsp[-1].decl) ); }
#line 16578 "Parser/parser.cc"
    break;

  case 1156: /* cfa_abstract_tuple: '[' cfa_abstract_parameter_list ']'  */
#line 4531 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newTuple( (yyvsp[-1].decl) ); }
#line 16584 "Parser/parser.cc"
    break;

  case 1157: /* cfa_abstract_tuple: '[' type_specifier_nobody ELLIPSIS ']'  */
#line 4533 "Parser/parser.yy"
                { SemanticError( (yyloc), "Tuple array currently unimplemented." ); (yyval.decl) = nullptr; }
#line 16590 "Parser/parser.cc"
    break;

  case 1158: /* cfa_abstract_tuple: '[' type_specifier_nobody ELLIPSIS constant_expression ']'  */
#line 4535 "Parser/parser.yy"
                { SemanticError( (yyloc), "Tuple array currently unimplemented." ); (yyval.decl) = nullptr; }
#line 16596 "Parser/parser.cc"
    break;

  case 1159: /* cfa_abstract_function: '[' ']' '(' cfa_parameter_list_ellipsis_opt ')'  */
#line 4540 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFunction( nullptr, DeclarationNode::newTuple( nullptr ), (yyvsp[-1].decl), nullptr ); }
#line 16602 "Parser/parser.cc"
    break;

  case 1160: /* cfa_abstract_function: cfa_abstract_tuple '(' push cfa_parameter_list_ellipsis_opt pop ')'  */
#line 4542 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFunction( nullptr, (yyvsp[-5].decl), (yyvsp[-2].decl), nullptr ); }
#line 16608 "Parser/parser.cc"
    break;

  case 1161: /* cfa_abstract_function: cfa_function_return '(' push cfa_parameter_list_ellipsis_opt pop ')'  */
#line 4544 "Parser/parser.yy"
                { (yyval.decl) = DeclarationNode::newFunction( nullptr, (yyvsp[-5].decl), (yyvsp[-2].decl), nullptr ); }
#line 16614 "Parser/parser.cc"
    break;

  case 1164: /* default_initializer_opt: %empty  */
#line 4568 "Parser/parser.yy"
                { (yyval.expr) = nullptr; }
#line 16620 "Parser/parser.cc"
    break;

  case 1165: /* default_initializer_opt: '=' assignment_expression  */
#line 4570 "Parser/parser.yy"
                { (yyval.expr) = (yyvsp[0].expr); }
#line 16626 "Parser/parser.cc"
    break;


#line 16630 "Parser/parser.cc"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

#line 4573 "Parser/parser.yy"


// ----end of grammar----

// Local Variables: //
// mode: c++ //
// tab-width: 4 //
// compile-command: "bison -Wcounterexamples parser.yy" //
// End: //
