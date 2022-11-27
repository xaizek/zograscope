// Copyright (C) 2021 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of version 3 of the GNU Affero General Public License as
// published by the Free Software Foundation.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "ts/lua/TSLuaLanguage.hpp"

#include <cassert>

#include "ts/lua/TSLuaSType.hpp"
#include "ts/TSTransformer.hpp"
#include "TreeBuilder.hpp"
#include "mtypes.hpp"
#include "tree.hpp"
#include "types.hpp"

using namespace tslua;

extern "C" const TSLanguage *tree_sitter_lua(void);

TsLuaLanguage::TsLuaLanguage() : tsLanguage(*tree_sitter_lua())
{
    stypes = {
        { "separator", +TSLuaSType::Separator },
        { "comment", +TSLuaSType::Comment },

        { "chunk", +TSLuaSType::Program },

        { "binary_expression", +TSLuaSType::BinaryExpression },
        { "condition_expression", +TSLuaSType::ConditionExpression },
        { "expression", +TSLuaSType::Expression },
        { "parenthesized_expression", +TSLuaSType::ParenthesizedExpression },
        { "prefix_expression", +TSLuaSType::PrefixExpression },
        { "unary_expression", +TSLuaSType::UnaryExpression },

        { "table", +TSLuaSType::Table },
        { "expression_list", +TSLuaSType::ExpressionList },

        { "statement", +TSLuaSType::Statement },
        { "call_statement", +TSLuaSType::CallStatement },
        { "do_statement", +TSLuaSType::DoStatement },
        { "empty_statement", +TSLuaSType::EmptyStatement },
        { "for_generic_statement", +TSLuaSType::ForGenericStatement },
        { "for_numeric_statement", +TSLuaSType::ForNumericStatement },
        { "function_definition_statement",
          +TSLuaSType::FunctionDefinitionStatement },
        { "goto_statement", +TSLuaSType::GotoStatement },
        { "label_statement", +TSLuaSType::LabelStatement },
        { "local_function_definition_statement",
          +TSLuaSType::LocalFunctionDefinitionStatement },
        { "repeat_statement", +TSLuaSType::RepeatStatement },
        { "return_statement", +TSLuaSType::ReturnStatement },
        { "while_statement", +TSLuaSType::WhileStatement },

        { "if_statement", +TSLuaSType::IfStatement },
        { "else_clause", +TSLuaSType::ElseClause },
        { "elseif_clause", +TSLuaSType::ElseifClause },

        { "function_body", +TSLuaSType::FunctionBody },
        { "function_definition", +TSLuaSType::FunctionDefinition },

        { "local_variable_declaration", +TSLuaSType::LocalVariableDeclaration },
        { "variable", +TSLuaSType::Variable },
        { "variable_assignment", +TSLuaSType::VariableAssignment },
        { "variable_list", +TSLuaSType::VariableList },

        { "parameter", +TSLuaSType::Parameter },
        { "parameter_list", +TSLuaSType::ParameterList },

        { "argument_list", +TSLuaSType::ArgumentList },
        { "attribute", +TSLuaSType::Attribute },
        { "block", +TSLuaSType::Block },
        { "call", +TSLuaSType::Call },
        { "field", +TSLuaSType::Field },
        { "field_list", +TSLuaSType::FieldList },

        { "not", +TSLuaSType::UnaryOperator },
        { "and", +TSLuaSType::BinaryOperator },
        { "or", +TSLuaSType::BinaryOperator },
    };

    types = {
        { "comment", Type::Comments },
        { "shebang", Type::Directives },

        { "local", Type::Specifiers },

        { "do", Type::Keywords },
        { "else", Type::Keywords },
        { "elseif", Type::Keywords },
        { "end", Type::Keywords },
        { "false", Type::Keywords },
        { "for", Type::Keywords },
        { "function", Type::Keywords },
        { "if", Type::Keywords },
        { "nil", Type::Keywords },
        { "repeat", Type::Keywords },
        { "return", Type::Keywords },
        { "then", Type::Keywords },
        { "true", Type::Keywords },
        { "until", Type::Keywords },
        { "while", Type::Keywords },

        { "break_statement", Type::Jumps },
        { "goto", Type::Jumps },

        { "and", Type::Keywords },
        { "in", Type::Keywords  },
        { "not", Type::Keywords },
        { "or", Type::Keywords },

        { "identifier", Type::Identifiers },
        { "number", Type::IntConstants },
        { "string", Type::StrConstants },

        { "<=", Type::Comparisons },
        { "<", Type::Comparisons },
        { ">", Type::Comparisons },
        { "==", Type::Comparisons },
        { ">=", Type::Comparisons },
        { "~=", Type::Comparisons },

        { "=", Type::Assignments },

        { "(", Type::LeftBrackets },
        { "{", Type::LeftBrackets },
        { "[", Type::LeftBrackets },

        { ")", Type::RightBrackets },
        { "}", Type::RightBrackets },
        { "]", Type::RightBrackets },

        { "~", Type::Operators },
        { "#", Type::Operators },
        { "-", Type::Operators },
        { "+", Type::Operators },
        { "%", Type::Operators },
        { "*", Type::Operators },
        { "/", Type::Operators },
        { "//", Type::Operators },
        { "^", Type::Operators },
        { "|", Type::Operators },
        { "&", Type::Operators },
        { "<<", Type::Operators },
        { ">>", Type::Operators },
        { "..", Type::Operators },

        { ",", Type::Other },
        { ":", Type::Other },
        { "::", Type::Other },
        { ";", Type::Other },
        { ".", Type::Other },
        { "vararg_expression", Type::Other },
    };

}

Type
TsLuaLanguage::mapToken(int token) const
{ return static_cast<Type>(token); }

TreeBuilder
TsLuaLanguage::parse(const std::string &contents,
                     const std::string &/*fileName*/, int tabWidth, bool debug,
                     cpp17::pmr::monolithic &mr) const
{
    TreeBuilder tb(mr);
    TSTransformer t(contents, tsLanguage, tb, stypes, types, badNodes, tabWidth,
                    debug);
    t.transform();

    return tb;
}

bool
TsLuaLanguage::isTravellingNode(const Node */*x*/) const
{ return false; }

bool
TsLuaLanguage::hasFixedStructure(const Node */*x*/) const
{ return false; }

bool
TsLuaLanguage::canBeFlattened(const Node */*parent*/, const Node *child,
                              int level) const
{
    switch (level) {
        case 0:
        case 1:
        case 2:
            return false;
        default:
            return -child->stype != TSLuaSType::Call
                && -child->stype != TSLuaSType::LocalVariableDeclaration
                && -child->stype != TSLuaSType::Parameter;
    }
}

bool
TsLuaLanguage::isUnmovable(const Node */*x*/) const
{ return false; }

bool
TsLuaLanguage::isContainer(const Node *x) const
{
    return -x->stype == TSLuaSType::FunctionBody
        || -x->stype == TSLuaSType::DoStatement;
}

bool
TsLuaLanguage::isDiffable(const Node *x) const
{ return Language::isDiffable(x); }

bool
TsLuaLanguage::isStructural(const Node *x) const
{
    return Language::isStructural(x)
        || x->label == "end"
        || x->label == ","
        || x->label == ";";
}

bool
TsLuaLanguage::isEolContinuation(const Node */*x*/) const
{ return false; }

bool
TsLuaLanguage::alwaysMatches(const Node *x) const
{ return (-x->stype == TSLuaSType::Program); }

bool
TsLuaLanguage::isPseudoParameter(const Node */*x*/) const
{ return false; }

bool
TsLuaLanguage::shouldSplice(SType parent, const Node *childNode) const
{
    TSLuaSType child = -childNode->stype;

    // Splice all but first BinaryExpression node of a nesting chain.
    if (-parent == TSLuaSType::BinaryExpression &&
        child == TSLuaSType::BinaryExpression) {
        bool oneLevel = true;
        const Node *n = (childNode->next && !childNode->next->last)
                      ? childNode->next
                      : childNode;
        for (const Node *node : n->children) {
            if (-node->stype == TSLuaSType::BinaryExpression) {
                oneLevel = false;
                break;
            }
        }
        if (!oneLevel) {
            return true;
        }
    }

    if (child == TSLuaSType::ParameterList) {
        return true;
    }
    return false;
}

bool
TsLuaLanguage::isValueNode(SType stype) const
{
    return -stype == TSLuaSType::ConditionExpression;
}

bool
TsLuaLanguage::isLayerBreak(SType /*parent*/, SType stype) const
{
    return -stype == TSLuaSType::Call
        || -stype == TSLuaSType::LocalFunctionDefinitionStatement
        || -stype == TSLuaSType::LocalVariableDeclaration
        || -stype == TSLuaSType::FunctionDefinition
        || -stype == TSLuaSType::FunctionDefinitionStatement
        || -stype == TSLuaSType::Field
        || -stype == TSLuaSType::Parameter
        || -stype == TSLuaSType::ReturnStatement
        || -stype == TSLuaSType::UnaryExpression
        || -stype == TSLuaSType::BinaryExpression
        || isValueNode(stype);
}

bool
TsLuaLanguage::shouldDropLeadingWS(SType /*stype*/) const
{ return false; }

bool
TsLuaLanguage::isSatellite(SType stype) const
{ return (-stype == TSLuaSType::Separator); }

MType
TsLuaLanguage::classify(SType stype) const
{
    switch (-stype) {
        case TSLuaSType::LocalVariableDeclaration:
            return MType::Declaration;

        case TSLuaSType::IfStatement:
        case TSLuaSType::ElseifClause:
        case TSLuaSType::ElseClause:
        case TSLuaSType::CallStatement:
        case TSLuaSType::RepeatStatement:
        case TSLuaSType::WhileStatement:
        case TSLuaSType::EmptyStatement:
        case TSLuaSType::GotoStatement:
        case TSLuaSType::LabelStatement:
        case TSLuaSType::ReturnStatement:
        case TSLuaSType::ForGenericStatement:
        case TSLuaSType::ForNumericStatement:
        case TSLuaSType::VariableAssignment:
            return MType::Statement;

        case TSLuaSType::FunctionDefinitionStatement:
        case TSLuaSType::LocalFunctionDefinitionStatement:
        case TSLuaSType::FunctionDefinition:
            return MType::Function;

        case TSLuaSType::Call:
            return MType::Call;

        case TSLuaSType::Parameter:
            return MType::Parameter;

        case TSLuaSType::Comment:
            return MType::Comment;

        case TSLuaSType::FunctionBody:
        case TSLuaSType::DoStatement:
            return MType::Block;

        default:
            return MType::Other;
    }
}

const char *
TsLuaLanguage::toString(SType stype) const
{
#define CASE(item) case TSLuaSType::item: return "TSLuaSType::" #item

    switch (-stype) {
        CASE(None);

        CASE(Separator);
        CASE(Comment);

        CASE(Program);

        CASE(Expression);
        CASE(PrefixExpression);
        CASE(Statement);
        CASE(ArgumentList);
        CASE(Attribute);
        CASE(BinaryExpression);
        CASE(ConditionExpression);
        CASE(Block);
        CASE(Call);
        CASE(DoStatement);
        CASE(ElseClause);
        CASE(ElseifClause);
        CASE(EmptyStatement);
        CASE(ExpressionList);
        CASE(Field);
        CASE(FieldList);
        CASE(ForGenericStatement);
        CASE(ForNumericStatement);
        CASE(FunctionBody);
        CASE(FunctionDefinition);
        CASE(FunctionDefinitionStatement);
        CASE(GotoStatement);
        CASE(IfStatement);
        CASE(LabelStatement);
        CASE(LocalFunctionDefinitionStatement);
        CASE(LocalVariableDeclaration);
        CASE(Parameter);
        CASE(ParameterList);
        CASE(ParenthesizedExpression);
        CASE(CallStatement);
        CASE(RepeatStatement);
        CASE(ReturnStatement);
        CASE(Table);
        CASE(UnaryExpression);
        CASE(Variable);
        CASE(VariableAssignment);
        CASE(VariableList);
        CASE(WhileStatement);
        CASE(UnaryOperator);
        CASE(BinaryOperator);
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";

#undef CASE
}
