#pragma once

// general
#define nd_items list
#define nd_elements list

//
// ND_Value
#define nd_value obj

// ND_Variable
#define nd_variable_is_global b1
#define nd_variable_offset size

// ND_Functor

// ND_Array
#define nd_array_elements list

// ND_Tuple
#define nd_tuple_elements list

// ND_Dict
#define nd_dict_pairs list
#define nd_dict_pair_key na
#define nd_dict_pair_value nb

#define nd_id_name tok
#define nd_id_tp_args list
#define nd_id_target nb
#define nd_id_enumerator_index size

//
// if name of enumerator needing initializers,
// pointer to call-func expression.
#define nd_id_enumerator_callctor nb // => ND_CallFunc

#define nd_scope_resol_first na
#define nd_scope_resol_idlist list

#define nd_callctor_ctor_side na
#define nd_callctor_referenced_def nb // => ND_Struct or ND_Class
#define nd_callctor_initializers list

#define nd_callctor_init_key tok  // member-name
#define nd_callctor_init_value na // value

//
// ND_CallFunc
#define nd_callfunc_callee na
#define nd_callfunc_callee_userdef nb
#define nd_callfunc_callee_builtin bfun
#define nd_callfunc_is_method_call b1
#define nd_callfunc_method_self nc
#define nd_callfunc_args list
#define nd_callfunc_enum_ctor_enum nd
#define nd_callfunc_enum_ctor_index size

//
// ND_Cast
#define nd_cast_to_type na
#define nd_cast_from_expr nb

//
// ND_Range
#define nd_range_begin na
#define nd_range_end nb

//
// ND_Type
#define nd_type_id na
#define nd_type_scope_resol list
#define nd_type_tp_args_ptr nb
#define nd_type_is_mut b1
#define nd_type_is_ref b2
#define nd_type_tp_args nb->list

#define nd_block_parent na
#define nd_block_items list

// if
#define nd_if_cond na
#define nd_if_then nb
#define nd_if_else nc

// switch
#define nd_switch_cond na
#define nd_switch_cases list
#define nd_switch_default_case nb

// switch-case
#define nd_switch_case_cond na
#define nd_switch_case_body nb

// match
#define nd_match_cond na
#define nd_match_cases list

// match-case
#define nd_match_case_cond na
#define nd_match_case_body nb

// loop
#define nd_loop_body na

//
// for loop
//
#define nd_for_init na
#define nd_for_cond nb
#define nd_for_step nc
#define nd_for_body nd

// for-range
#define nd_forrange_range na
#define nd_forrange_body nb

// for-each
#define nd_foreach_init na
#define nd_foreach_iter nb
#define nd_foreach_content nc
#define nd_foreach_cond nd
#define nd_foreach_step ne
#define nd_foreach_body nf

// while
#define nd_while_cond na
#define nd_while_body nb

// let
#define nd_let_name tok2
#define nd_let_type na
#define nd_let_init nb
#define nd_let_offset size

#define nd_func_name tok2
#define nd_func_is_method b2
#define nd_func_is_template b3
#define nd_func_is_one_line b4
#define nd_func_tplist nc // template parameters list
#define nd_func_cclist nd // concept tags (if used)
#define nd_func_args list
#define nd_func_result_type na
#define nd_func_is_variable_args b1
#define nd_func_body nb
#define nd_func_lvar_count size
#define nd_func_args_ti v1   // => Vec<TypeInfo>*
#define nd_func_result_ti v2 // => TypeInfo*

#define nd_func_arg_name tok
#define nd_func_arg_type na

#define nd_lhs na
#define nd_rhs nb

#define nd_return_expr na

#define nd_enum_name tok2
#define nd_enum_cclist na
#define nd_enum_tplist nb
#define nd_enum_is_template b1
#define nd_enum_enumerators list

#define nd_enumerator_name tok
#define nd_enumerator_is_value b1
#define nd_enumerator_is_struct b2
#define nd_enumerator_val_type na
#define nd_enumerator_struct_members list

#define nd_class_name tok2
#define nd_class_cclist na
#define nd_class_tplist nb
#define nd_class_is_template b1
#define nd_class_fields nc
#define nd_class_methods nd

#define nd_struct_name tok2
#define nd_struct_cclist na
#define nd_struct_tplist nb
#define nd_struct_is_template b1
#define nd_struct_members list
#define nd_struct_member_name tok
#define nd_struct_member_type nc

#define nd_namespace_name tok2
#define nd_namespace_items list

#define nd_program_main na
#define nd_program_items list
#define nd_program_global_var_size size

#define nd_nametype_pair_name tok
#define nd_nametype_pair_type na

#define nd_concept_tags_list_list list

#define nd_concepttag_name tok

#define nd_concept_cclist na
#define nd_concept_name tok
#define nd_concept_parameters list
#define nd_concept_ccbody nb

#define nd_ccbody_rules list
