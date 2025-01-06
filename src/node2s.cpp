#include "Object.h"
#include "Token.h"
#include "Node.h"
#include "Utils.h"
#include "node2s.h"
#include "alert.h"

//
// get_last_ch
//
// 文字列の最後の文字を取得する
// 最後の空白がある場合はその手前の文字を返す
static char get_last_ch(string str) {
  for (int i = str.size() - 1; i >= 0; i--) {
    if (str[i] != ' ')
      return str[i];
  }

  return 0;
}

string node2s(Node* node) {

  static int _indent = 0;

  static auto indent = []() -> string {
    return string(_indent * 2, ' ');
  };

  assert(node != nullptr);

  switch (node->kind) {

    case ND_Value:
      if (node->tok->is(TokenKind::Character))
        return "'" + node->tok->str + "'";
      else if (node->tok->is(TokenKind::String))
        return "\"" + node->tok->str + "\"";

      return node->tok->str;

    case ND_Identifier: {
      auto s = node->tok->str;

      if (node->nd_id_template_args.size() >= 1) {
        s += "<" + utils::join(", ", node->nd_id_template_args, node2s) + ">";
      }

      return s;
    }

    case ND_ScopeResol: {
      auto s = node2s(node->nd_scope_resol_first);

      for (auto x : node->nd_scope_resol_idlist)
        s += "::" + node2s(x);

      return s;
    }

    case ND_CallConstructor:
      return node2s(node->nd_callctor_ctor_id) + "{" +
             utils::join(", ", node->nd_callctor_initializers, node2s) + "}";

    case ND_CallCtorPair:
      return node->nd_callctor_init_key->str + ": " +
             node2s(node->nd_callctor_init_value);

    case ND_Array:
      return "[" + utils::join(", ", node->nd_array_elements, node2s) + "]";

    case ND_Tuple:
      return "(" + utils::join(", ", node->nd_tuple_elements, node2s) + ")";

    case ND_Dict:
      return "{" + utils::join(", ", node->list, node2s) + "}";

    case ND_DictPair:
      return node2s(node->nd_dict_pair_key) + ": " + node2s(node->nd_dict_pair_value);

    case ND_Not:
      return "not " + node2s(node->nd_lhs);

    case ND_Ref:
      return "ref " + node2s(node->nd_lhs);

    case ND_Cast:
      return "cast<" + node2s(node->nd_cast_to_type) + ">(" +
             node2s(node->nd_cast_from_expr) + ")";

    case ND_Subscript:
      return node2s(node->nd_lhs) + "[" + node2s(node->nd_rhs) + "]";

    case ND_MemberAccess:
      return node2s(node->nd_lhs) + "." + node2s(node->nd_rhs);

    case ND_CallFunc:
      return node2s(node->nd_callfunc_callee) + "(" +
             utils::join(", ", node->nd_callfunc_args, node2s) + ")";

    case ND_ConstructEnumeratorValue:
    case ND_ConstructEnumeratorStruct:
      todo_impl;
      break;

    case ND_Mul:
    case ND_Div:
    case ND_Mod:
    case ND_Add:
    case ND_Sub:
    case ND_LShift:
    case ND_RShift:
    case ND_Compare:
    case ND_Equal:
    case ND_BitAnd:
    case ND_BitOr:
    case ND_BitXor:
    case ND_Or:
    case ND_And:
    case ND_Assign:
      return node2s(node->nd_lhs) + " " + node->tok->str + " " + node2s(node->nd_rhs);

    case ND_Let: {
      string s = "let " + node->nd_let_name->str;

      if (node->nd_let_type)
        s += " : " + node2s(node->nd_let_type);

      if (node->nd_let_init)
        s += " = " + node2s(node->nd_let_init);

      return s;
    }

    case ND_If: {
      string s = "if " + node2s(node->nd_if_cond) + " " + node2s(node->nd_if_then);

      if (node->nd_if_else)
        s += " else " + node2s(node->nd_if_else);

      return s;
    }

    case ND_IfLet:
      todo_impl;
      break;

    case ND_Switch: {
      string s = "switch " + node2s(node->nd_switch_cond) + " {\n";
      _indent++;

      for (auto c : node->nd_switch_cases) {
        s += indent() + "case " + node2s(c->nd_switch_case_cond) + " " +
             node2s(c->nd_switch_case_body) + "\n";
      }

      if (node->nd_switch_default_case) {
        s += indent() + "default " + node2s(node->nd_switch_default_case) + "\n";
      }

      _indent--;
      s += indent() + "}";
      return s;
    }

    case ND_SwitchCase:
      return "case " + node2s(node->nd_switch_case_cond) + " " +
             node2s(node->nd_switch_case_body);

    case ND_Match: {
      auto ind = indent();

      auto s = "match " + node2s(node->nd_match_cond) + " {\n  " + ind;

      _indent++;

      s += utils::join(",\n  " + ind, node->nd_match_cases, node2s) + "\n" + ind + "}";

      _indent--;

      return s;
    }

    case ND_MatchCase:
      return node2s(node->nd_match_case_cond) + " => " + node2s(node->nd_match_case_body);

    case ND_While:
      return "while " + node2s(node->nd_while_cond) + node2s(node->nd_while_body);

    case ND_For: {
      string s = "for ";

      break;
    }

    case ND_Loop: {
      return "loop " + node2s(node->nd_loop_body);
    }

    case ND_Break:
      return "break";

    case ND_Continue:
      return "continue";

    case ND_Return:
      if (auto x = node->nd_return_expr)
        return "return " + node2s(x);
      else
        return "return";

    case ND_Block: {
      auto ind = indent();

      auto s = "{\n  " + ind;

      _indent++;
      s += utils::join("\n" + indent(), node->nd_elements,
                       [](Node* node) -> string {
                         auto str = node2s(node);

                         if (get_last_ch(str) != '}')
                           str += ';';

                         return str;
                       }) +
           "\n" + ind + "}";

      _indent--;

      return s;
    }

    case ND_Function: {
      auto s = "fn " + node->nd_func_name->str + "(" +
               utils::join(",", node->nd_func_args, node2s) + ") ";

      if (auto x = node->nd_func_result_type)
        s += "-> " + node2s(x) + " ";

      return s + node2s(node->nd_func_body);
    }

    case ND_FunctionArg:
      return node->tok->str + ": " + node2s(node->nd_func_arg_type);

    case ND_Enum: {
      auto s = "enum " + node->nd_enum_name->str + " {\n  " + indent();

      _indent++;

      s += utils::join(",\n" + indent(), node->nd_items, node2s) + "\n}";

      _indent--;

      return s;
    }

    case ND_DefEnumerator: {
      auto s = node->nd_enumerator_name->str;

      if (node->nd_enumerator_is_value)
        s += "(" + node2s(node->nd_enumerator_val_type) + ")";
      else if (node->nd_enumerator_is_struct)
        s += "(" + utils::join(", ", node->nd_enumerator_struct_members, node2s) + ")";

      return s;
    }

    case ND_DefEnumeratorStructFields:
      return utils::join(", ", node->list, node2s);

    case ND_Struct:
      return "struct";

    case ND_StructMember:
      return node->tok->str + ": " + node2s(node->nd_struct_member_type);

    case ND_Class:
    case ND_Namespace:
      break;

    case ND_Program:
      return utils::join("\n\n", node->nd_items, node2s);

    case ND_TypeName: {
      auto s = node->tok->str;

      if (node->nd_type_template_args.size() >= 1)
        s += "<" + utils::join(", ", node->nd_type_template_args, node2s) + ">";

      if (node->nd_type_is_mut)
        s += " mut";

      if (node->nd_type_is_ref)
        s += " ref";

      return s;
    }

    case ND_PairNameAndType:
      return node->tok->str + ": " + node2s(node->nd_nametype_pair_type);

    case ND_InitializerList:
      return "{" + utils::join(", ", node->list, node2s) + "}";
  }

  return "<node>";
}
