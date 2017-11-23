// Copyright (C) 2017 xaizek <xaizek@posteo.net>
//
// This file is part of zograscope.
//
// zograscope is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// zograscope is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with zograscope.  If not, see <http://www.gnu.org/licenses/>.

#include "stypes.hpp"

#include <cassert>

#include <ostream>

std::ostream &
operator<<(std::ostream &os, SType stype)
{
    switch (stype) {
        case SType::None:                return (os << "None");
        case SType::TranslationUnit:     return (os << "TranslationUnit");
        case SType::Declaration:         return (os << "Declaration");
        case SType::FunctionDeclaration: return (os << "FunctionDeclaration");
        case SType::FunctionDefinition:  return (os << "FunctionDefinition");
        case SType::Directive:           return (os << "Directive");
        case SType::LineGlue:            return (os << "LineGlue");
        case SType::Comment:             return (os << "Comment");
        case SType::Macro:               return (os << "Macro");
        case SType::CompoundStatement:   return (os << "CompoundStatement");
        case SType::Separator:           return (os << "Separator");
        case SType::Punctuation:         return (os << "Punctuation");
        case SType::Statements:          return (os << "Statements");
        case SType::ExprStatement:       return (os << "ExprStatement");
        case SType::IfStmt:              return (os << "IfStmt");
        case SType::IfCond:              return (os << "IfCond");
        case SType::IfThen:              return (os << "IfThen");
        case SType::IfElse:              return (os << "IfElse");
        case SType::WhileStmt:           return (os << "WhileStmt");
        case SType::DoWhileStmt:         return (os << "DoWhileStmt");
        case SType::WhileCond:           return (os << "WhileCond");
        case SType::ForStmt:             return (os << "ForStmt");
        case SType::LabelStmt:           return (os << "LabelStmt");
        case SType::ForHead:             return (os << "ForHead");
        case SType::Expression:          return (os << "Expression");
        case SType::AnyExpression:       return (os << "AnyExpression");
        case SType::Declarator:          return (os << "Declarator");
        case SType::Initializer:         return (os << "Initializer");
        case SType::InitializerList:     return (os << "InitializerList");
        case SType::Specifiers:          return (os << "Specifiers");
        case SType::WithInitializer:     return (os << "WithInitializer");
        case SType::WithoutInitializer:  return (os << "WithoutInitializer");
        case SType::InitializerElement:  return (os << "InitializerElement");
        case SType::SwitchStmt:          return (os << "SwitchStmt");
        case SType::GotoStmt:            return (os << "GotoStmt");
        case SType::ContinueStmt:        return (os << "ContinueStmt");
        case SType::BreakStmt:           return (os << "BreakStmt");
        case SType::ReturnValueStmt:     return (os << "ReturnValueStmt");
        case SType::ReturnNothingStmt:   return (os << "ReturnNothingStmt");
        case SType::ArgumentList:        return (os << "ArgumentList");
        case SType::Argument:            return (os << "Argument");
        case SType::ParameterList:       return (os << "ParameterList");
        case SType::Parameter:           return (os << "Parameter");
        case SType::CallExpr:            return (os << "CallExpr");
        case SType::AssignmentExpr:      return (os << "AssignmentExpr");
        case SType::ConditionExpr:       return (os << "ConditionExpr");
        case SType::ComparisonExpr:      return (os << "ComparisonExpr");
        case SType::AdditiveExpr:        return (os << "AdditiveExpr");
        case SType::PointerDecl:         return (os << "PointerDecl");
        case SType::DirectDeclarator:    return (os << "DirectDeclarator");
        case SType::TemporaryContainer:  return (os << "TemporaryContainer");
        case SType::Bundle:              return (os << "Bundle");
        case SType::BundleComma:         return (os << "BundleComma");
    }

    assert(false && "Unhandled enumeration item");
    return (os << "<UNKNOWN>");
}
