#pragma once
#include "lexer.hpp"
#include "ast/ast.hpp"

namespace Ru::parse
{
	Ru::ast::ExpressionPtr parse(Ru::lexer::token_generator);
}
