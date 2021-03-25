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

        { "program", +TSLuaSType::Program },

        { "function", +TSLuaSType::Function },
        { "function_definition", +TSLuaSType::Function },
        { "local_function", +TSLuaSType::Function },
        { "function_name", +TSLuaSType::FunctionName },
        { "function_name_field", +TSLuaSType::FunctionName },
        { "function_body", +TSLuaSType::FunctionBody },
        { "parameters", +TSLuaSType::Parameters },
        { "parameter", +TSLuaSType::Parameter },

        { "function_call", +TSLuaSType::FunctionCall },
        { "function_call_statement", +TSLuaSType::CallStatement },
        { "arguments", +TSLuaSType::Arguments },

        { "local_variable_declaration_statement", +TSLuaSType::DeclStatement },
        { "variable_declaration_statement", +TSLuaSType::DeclStatement },
        { "local_variable_declaration", +TSLuaSType::VariableDecl },
        { "variable_declaration", +TSLuaSType::VariableDecl },
        { "variable_declarator", +TSLuaSType::VariableDeclarator },

        { "unary_operation", +TSLuaSType::UnaryOperation },
        { "binary_operation", +TSLuaSType::BinaryOperation },

        { "expression", +TSLuaSType::Expression },
        { "condition_expression", +TSLuaSType::ConditionExpression },
        { "loop_expression", +TSLuaSType::LoopExpression },
        { "field_expression", +TSLuaSType::FieldExpression },

        { "if_statement", +TSLuaSType::IfStatement },
        { "elseif", +TSLuaSType::ElseIfStatement },
        { "else", +TSLuaSType::ElseStatement },

        { "do_statement", +TSLuaSType::DoStatement },
        { "repeat_statement", +TSLuaSType::RepeatStatement },
        { "while_statement", +TSLuaSType::WhileStatement },
        { "for_in_statement", +TSLuaSType::ForInStatement },
        { "for_statement", +TSLuaSType::ForStatement },

        { "goto_statement", +TSLuaSType::GotoStatement },
        { "label_statement", +TSLuaSType::LabelStatement },
        { "return_statement", +TSLuaSType::ReturnStatement },

        { "table", +TSLuaSType::Table },
        { "field", +TSLuaSType::Field },
        { "quoted_field", +TSLuaSType::QuotedField },

        { "global_variable", +TSLuaSType::GlobalVariable },

        { "not", +TSLuaSType::UnaryOperator },
        { "and", +TSLuaSType::BinaryOperator },
        { "or", +TSLuaSType::BinaryOperator },
    };

    types = {
        { "local", Type::Specifiers },

        { "comment", Type::Comments },

        { "function", Type::Keywords },
        { "while", Type::Keywords },
        { "repeat", Type::Keywords },
        { "until", Type::Keywords },
        { "if", Type::Keywords },
        { "then", Type::Keywords },
        { "else", Type::Keywords },
        { "elseif", Type::Keywords },
        { "end", Type::Keywords },
        { "for", Type::Keywords },
        { "in", Type::Keywords },
        { "do", Type::Keywords },
        { "true", Type::Keywords },
        { "false", Type::Keywords },
        { "return", Type::Keywords },
        { "nil", Type::Keywords },

        { "break_statement", Type::Jumps },
        { "next", Type::Jumps },
        { "goto", Type::Jumps },

        { "number", Type::IntConstants },
        { "string", Type::StrConstants },

        { "method", Type::Functions },

        { "_G", Type::Identifiers },
        { "_VERSION", Type::Identifiers },
        { "self", Type::Identifiers },
        { "identifier", Type::Identifiers },
        { "property_identifier", Type::Identifiers },

        { "and", Type::Keywords },
        { "or", Type::Keywords },
        { "not", Type::Keywords },

        { "==", Type::Comparisons },
        { "~=", Type::Comparisons },
        { "<", Type::Comparisons },
        { ">", Type::Comparisons },
        { "<=", Type::Comparisons },
        { ">=", Type::Comparisons },

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

        { "=", Type::Assignments },

        { "(", Type::LeftBrackets },
        { "{", Type::LeftBrackets },
        { "[", Type::LeftBrackets },

        { ")", Type::RightBrackets },
        { "}", Type::RightBrackets },
        { "]", Type::RightBrackets },

        { ",", Type::Other },
        { ":", Type::Other },
        { "::", Type::Other },
        { ";", Type::Other },
        { ".", Type::Other },
        { "spread", Type::Other },
    };
}

Type
TsLuaLanguage::mapToken(int token) const
{ return static_cast<Type>(token); }

TreeBuilder
TsLuaLanguage::parse(const std::string &contents,
                     const std::string &/*fileName*/, bool debug,
                     cpp17::pmr::monolithic &mr) const
{
    TreeBuilder tb(mr);
    TSTransformer(contents, tsLanguage, tb, stypes, types, debug).transform();
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
            return -child->stype != TSLuaSType::FunctionCall
                && -child->stype != TSLuaSType::VariableDecl
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

    // Splice all but first BinaryOperation node of a nesting chain.
    if (-parent == TSLuaSType::BinaryOperation &&
        child == TSLuaSType::BinaryOperation) {
        bool oneLevel = true;
        const Node *n = (childNode->next && !childNode->next->last)
                      ? childNode->next
                      : childNode;
        for (const Node *node : n->children) {
            if (-node->stype == TSLuaSType::BinaryOperation) {
                oneLevel = false;
                break;
            }
        }
        if (!oneLevel) {
            return true;
        }
    }

    if (child == TSLuaSType::Parameters ||
        child == TSLuaSType::Arguments) {
        return true;
    }
    return false;
}

bool
TsLuaLanguage::isValueNode(SType stype) const
{
    return -stype == TSLuaSType::ConditionExpression
        || -stype == TSLuaSType::LoopExpression;
}

bool
TsLuaLanguage::isLayerBreak(SType /*parent*/, SType stype) const
{
    return -stype == TSLuaSType::FunctionCall
        || -stype == TSLuaSType::Function
        || -stype == TSLuaSType::VariableDecl
        || -stype == TSLuaSType::Field
        || -stype == TSLuaSType::Parameter
        || -stype == TSLuaSType::ReturnStatement
        || -stype == TSLuaSType::UnaryOperation
        || -stype == TSLuaSType::BinaryOperation
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
        case TSLuaSType::VariableDecl:
            return MType::Declaration;

        case TSLuaSType::IfStatement:
        case TSLuaSType::ElseIfStatement:
        case TSLuaSType::ElseStatement:
        case TSLuaSType::RepeatStatement:
        case TSLuaSType::WhileStatement:
        case TSLuaSType::ForInStatement:
        case TSLuaSType::ForStatement:
        case TSLuaSType::GotoStatement:
        case TSLuaSType::LabelStatement:
        case TSLuaSType::ReturnStatement:
        case TSLuaSType::CallStatement:
        case TSLuaSType::DeclStatement:
            return MType::Statement;

        case TSLuaSType::Function:
            return MType::Function;

        case TSLuaSType::FunctionCall:
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
    switch (-stype) {
        case TSLuaSType::None:         return "TSLuaSType::None";

        case TSLuaSType::Separator:    return "TSLuaSType::Separator";
        case TSLuaSType::Comment:      return "TSLuaSType::Comment";

        case TSLuaSType::Program:      return "TSLuaSType::Program";

        case TSLuaSType::Function:     return "TSLuaSType::Function";
        case TSLuaSType::FunctionName: return "TSLuaSType::FunctionName";
        case TSLuaSType::FunctionBody: return "TSLuaSType::FunctionBody";
        case TSLuaSType::Parameters:   return "TSLuaSType::Parameters";
        case TSLuaSType::Parameter:    return "TSLuaSType::Parameter";

        case TSLuaSType::FunctionCall: return "TSLuaSType::FunctionCall";
        case TSLuaSType::Arguments:    return "TSLuaSType::Arguments";

        case TSLuaSType::VariableDecl: return "TSLuaSType::VariableDecl";
        case TSLuaSType::VariableDeclarator:
                                       return "TSLuaSType::VariableDeclarator";

        case TSLuaSType::UnaryOperation:
                                       return "TSLuaSType::UnaryOperation";
        case TSLuaSType::BinaryOperation:
                                       return "TSLuaSType::BinaryOperation";

        case TSLuaSType::Expression:   return "TSLuaSType::Expression";
        case TSLuaSType::ConditionExpression:
                                       return "TSLuaSType::ConditionExpression";
        case TSLuaSType::LoopExpression:
                                       return "TSLuaSType::LoopExpression";
        case TSLuaSType::FieldExpression:
                                       return "TSLuaSType::FieldExpression";

        case TSLuaSType::IfStatement:  return "TSLuaSType::IfStatement";
        case TSLuaSType::ElseIfStatement:
                                       return "TSLuaSType::ElseIfStatement";
        case TSLuaSType::ElseStatement:
                                       return "TSLuaSType::ElseStatement";

        case TSLuaSType::DoStatement:  return "TSLuaSType::DoStatement";
        case TSLuaSType::RepeatStatement:
                                       return "TSLuaSType::RepeatStatement";
        case TSLuaSType::WhileStatement:
                                       return "TSLuaSType::WhileStatement";
        case TSLuaSType::ForInStatement:
                                       return "TSLuaSType::ForInStatement";
        case TSLuaSType::ForStatement: return "TSLuaSType::ForStatement";

        case TSLuaSType::GotoStatement:
                                       return "TSLuaSType::GotoStatement";
        case TSLuaSType::LabelStatement:
                                       return "TSLuaSType::LabelStatement";
        case TSLuaSType::ReturnStatement:
                                       return "TSLuaSType::ReturnStatement";
        case TSLuaSType::CallStatement:
                                       return "TSLuaSType::CallStatement";
        case TSLuaSType::DeclStatement:
                                       return "TSLuaSType::DeclStatement";

        case TSLuaSType::Table:        return "TSLuaSType::Table";
        case TSLuaSType::Field:        return "TSLuaSType::Field";
        case TSLuaSType::QuotedField:  return "TSLuaSType::QuotedField";

        case TSLuaSType::GlobalVariable:
                                       return "TSLuaSType::GlobalVariable";

        case TSLuaSType::UnaryOperator:
                                       return "TSLuaSType::UnaryOperator";
        case TSLuaSType::BinaryOperator:
                                       return "TSLuaSType::BinaryOperator";
    }

    assert(false && "Unhandled enumeration item");
    return "<UNKNOWN>";
}
