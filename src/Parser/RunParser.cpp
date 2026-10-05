//
// Cforall Version 1.0.0 Copyright (C) 2018 University of Waterloo
//
// The contents of this file are covered under the licence agreement in the
// file "LICENCE" distributed with Cforall.
//
// RunParser.cpp -- External interface to the parser.
//
// Author           : Andrew Beach
// Created On       : Mon Dec 19 11:00:00 2022
// Last Modified By : Andrew Beach
// Last Modified On : Mon Mar  6  9:42:00 2023
// Update Count     : 3
//

#include "RunParser.hpp"

#include "AST/TranslationUnit.hpp"          // for TranslationUnit
#include "Common/CodeLocationTools.hpp"     // for forceFillCodeLocations
#include "Parser/DeclarationNode.hpp"       // for DeclarationNode, buildList
#include "Parser/TypedefTable.hpp"          // for TypedefTable
#include "Common/ErrorObjects.hpp"         // for SemanticErrorException
#include "LSP/Lsp.hpp"                      // for LSP::enabled, LSP::syntaxErrorsFound

// Variables global to the parsing code.
ast::Linkage::Spec linkage = ast::Linkage::Cforall;
TypedefTable typedefTable;
DeclarationNode * parseTree = nullptr;

void parse( FILE * input, ast::Linkage::Spec linkage, bool alwaysExit, bool countLines ) {
	extern int yyparse( void );
	extern FILE * yyin;
	extern int yylineno;
	extern int yypline;

	// Set global information.
	::linkage = linkage;
	yyin = input;
	yylineno = 1;
	yypline = countLines ? 1 : 0;

	if ( LSP::enabled && ! alwaysExit ) {
		// The parser records syntax errors and recovers from them where it can. An error it cannot recover from, or
		// one an action throws, ends the parse, but parseTree keeps the definitions parsed before it. Translation
		// goes on with what was parsed.
		int parseStatus = 0;
		try {
			parseStatus = yyparse();
		} catch ( SemanticErrorException & e ) {
			LSP::addErrors( e );
			parseStatus = 1;
		} // try
		fclose( input );
		if ( parseStatus != 0 || LSP::hasErrors() ) LSP::syntaxErrorsFound();
		return;
	} // if

	int parseStatus = yyparse();
	fclose( input );
	if ( alwaysExit || parseStatus != 0 ) {
		exit( parseStatus );
	} // if
} // parse

ast::TranslationUnit buildUnit(void) {
	std::vector<ast::ptr<ast::Decl>> decls;
	buildList( parseTree, decls );
	delete parseTree;
	parseTree = nullptr;

	ast::TranslationUnit transUnit;
	for ( auto decl : decls ) {
		transUnit.decls.emplace_back( std::move( decl ) );
	}
	return transUnit;
}

// Local Variables: //
// tab-width: 4 //
// mode: c++ //
// compile-command: "make install" //
// End: //
