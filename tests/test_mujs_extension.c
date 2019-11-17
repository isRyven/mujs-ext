#include "minunit.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <mujs/mujs.h>

js_State *J;

void test_setup(void)
{
	J = js_newstate(NULL, NULL, 0);
	if (js_try(J))
	{
		mu_fail(js_tostring(J, -1));
		return;
	}
}

void test_teardown(void) 
{
	js_endtry(J);
	js_freestate(J);
}

MU_TEST(it_should_offset_bottom_of_stack)
{
	js_pushundefined(J);
	js_pushnumber(J, 999);
	js_pushnumber(J, 998);
	js_pushnumber(J, 997);
	int top = js_gettop(J);
	int offset = js_setbot(J, top);
	mu_assert(js_gettop(J) == 0, "should return adjusted local stack top");
	js_setbot(J, -offset);
	mu_assert(js_gettop(J) == 4, "should restore the stack bottom value");
}

MU_TEST(it_should_rotate_and_pop_remaining_stack_values)
{
	js_pushundefined(J);
	js_pushnumber(J, 1);
	js_pushnumber(J, 2);
	js_pushnumber(J, 3);
	js_pushnumber(J, 4);
	js_pushnumber(J, 5);
	js_pushnumber(J, 6);
	js_pushnumber(J, 0);
	js_rotnpop(J, 4);
	mu_assert_int_eq(0, js_tointeger(J, 4));
	mu_assert_int_eq(5, js_gettop(J));
}

MU_TEST(it_should_copy_range_of_values_to_the_end_of_stack)
{
	js_pushundefined(J);
	js_pushnumber(J, 1);
	js_pushnumber(J, 2);
	js_pushnumber(J, 3);
	js_pushnumber(J, 4);
	js_pushnumber(J, 5);
	js_pushnumber(J, 6);
	js_copyrange(J, 3, 5);
	mu_assert_int_eq(9, js_gettop(J));
	mu_assert_int_eq(3, js_tointeger(J, 7));
	mu_assert_int_eq(4, js_tointeger(J, 8));
}

void testfunc(js_State *J, void *data)
{
	mu_assert_int_eq(3, js_gettop(J));
	mu_assert(js_isundefined(J, 0), "should be undefined");
	mu_assert_int_eq(1, js_tointeger(J, 1));
	mu_assert_int_eq(2, js_tointeger(J, 2));
	js_pushnumber(J, 1);
	js_pushnumber(J, 2);
	js_pushnumber(J, 3);
}

MU_TEST(it_should_call_function_in_scoped_environment)
{
	js_pushnumber(J, 0);
	js_pushnumber(J, 10);
	js_pushnumber(J, 20);
	js_pushnumber(J, 30);
	js_pushundefined(J);
	js_pushnumber(J, 1);
	js_pushnumber(J, 2);
	js_callscoped(J, testfunc, NULL, 2);
	mu_assert_int_eq(5, js_gettop(J));
	mu_assert_int_eq(3, js_tointeger(J, -1));
}

MU_TEST(it_should_seal_object)
{
	js_newobject(J);
	js_pushnumber(J, 13);
	js_setproperty(J, -2, "num");
	js_seal(J);
	js_pushnumber(J, 14);
	js_setproperty(J, -2, "num2");
	js_getproperty(J, -1, "num2");
	mu_assert(js_isundefined(J, -1), "should not allow to define new properties");
	js_pop(J, 1);
	js_pushnumber(J, 14);
	js_setproperty(J, -2, "num");
	js_getproperty(J, -1, "num");
	mu_assert_int_eq(14, js_tointeger(J, -1));
}

MU_TEST(it_should_freeze_object)
{
	js_newobject(J);
	js_pushnumber(J, 13);
	js_setproperty(J, -2, "num");
	js_freeze(J);
	js_pushnumber(J, 14);
	js_setproperty(J, -2, "num2");
	js_getproperty(J, -1, "num2");
	mu_assert(js_isundefined(J, -1), "should not allow to define new properties");
	js_pop(J, 1);
	js_pushnumber(J, 14);
	js_setproperty(J, -2, "num");
	js_getproperty(J, -1, "num");
	mu_assert_int_eq(13, js_tointeger(J, -1));	
}

MU_TEST(it_should_check_if_object_is_sealed)
{
	js_newobject(J);
	js_seal(J);
	js_copy(J, -1);
	int top = js_gettop(J);
	mu_assert(js_issealed(J), "should return positive result");
	mu_assert_int_eq(top, js_gettop(J)); // should not consume or produce anything
	mu_assert(js_strictequal(J), "should not consume or produce anything");
}

MU_TEST(it_should_check_if_object_is_frozen)
{
	js_newobject(J);
	js_freeze(J);
	js_copy(J, -1);
	int top = js_gettop(J);
	mu_assert(js_isfrozen(J), "should return positive result");
	mu_assert_int_eq(top, js_gettop(J)); // should not consume or produce anything
	mu_assert(js_strictequal(J), "should not consume or produce anything");
}

MU_TEST(it_should_load_string_with_predefined_variables)
{
	js_newobject(J);
		js_pushnumber(J, 14);
		js_setproperty(J, -2, "num");
	js_loadstringE(J, "test.js", "if (num !== 14) throw new Error();");
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	mu_assert(js_iscallable(J, -1), "should return function to call");
	js_pushundefined(J);
	js_pcall(J, 0);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
}

MU_TEST(it_should_return_empty_function_if_source_is_also_empty)
{
	js_newobject(J);
	js_loadstringE(J, "test.js", "");
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	mu_assert(js_iscallable(J, -1), "should return function to call");
	js_pushundefined(J);
	js_pcall(J, 0);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));	
}

MU_TEST(it_should_set_exit_restore_point)
{
	if (js_catch_exit(J))
	{
		int status = js_tointeger(J, -1);
		mu_assert_int_eq(1, status);
		return;
	}
	js_exit(J, 1);
	mu_fail("should not get here");
}

MU_TEST(it_should_return_default_double_if_value_is_undefined)
{
	js_pushundefined(J);
	mu_assert_double_eq(13, js_checknumber(J, -1, 13));
	js_pushnumber(J, 14);
	mu_assert_double_eq(14, js_checknumber(J, -1, 13));
}

MU_TEST(it_should_return_default_integer_if_value_is_undefined)
{
	js_pushundefined(J);
	mu_assert_int_eq(13, js_checkinteger(J, -1, 13));
	js_pushnumber(J, 14);
	mu_assert_int_eq(14, js_checkinteger(J, -1, 13));
}

MU_TEST(it_should_store_value_in_local_registry)
{
	js_newobject(J);
	js_pushnumber(J, 13);
	js_setlocalregistry(J, -2, "mysecretvalue");
	mu_assert_int_eq(1, js_gettop(J));
	js_getlocalregistry(J, -1, "mysecretvalue");
	mu_assert_int_eq(2, js_gettop(J));
	js_isnumber(J, -1);
	mu_assert_int_eq(js_tointeger(J, -1), 13);
	js_pop(J, 1);
	js_dellocalregistry(J, -1, "mysecretvalue");
	mu_assert_int_eq(1, js_gettop(J));
	js_getlocalregistry(J, -1, "mysecretvalue");
	mu_assert_int_eq(2, js_gettop(J));
	js_isundefined(J, -1);
}

MU_TEST(it_should_internally_reference_the_object)
{
	js_newobject(J);
	js_copy(J, -1);
	js_newobject(J);
	js_arefb(J);
}

MU_TEST(it_should_internally_unreference_the_object)
{
	js_newobject(J);
	js_newobject(J);
	js_copy(J, -2);
	js_copy(J, -2);
	js_arefb(J);
	js_aunrefb(J);
}

MU_TEST(it_should_properly_create_substring_using_substr_function)
{
	js_pushstring(J, "hello world");
	js_getproperty(J, -1, "substr");
	js_rot2(J);
	js_pushnumber(J, 6);
	js_pushnumber(J, 5);
	js_call(J, 2);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));	
	mu_assert(js_isstring(J, -1), "should return new sub string");
	mu_assert_string_eq("world", js_tostring(J, -1));

	js_pushstring(J, "hello world");
	js_getproperty(J, -1, "substr");
	js_rot2(J);
	js_pushnumber(J, 6);
	js_call(J, 1);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));	
	mu_assert(js_isstring(J, -1), "should return new sub string");
	mu_assert_string_eq("world", js_tostring(J, -1));
}

MU_TEST(it_should_not_throw_error_parsing_regexpr)
{
	js_ploadstring(J, "testfile.js", 
		"var str = 'he{11}o world';\n"
		"var res = str.replace(/{\\d+}/g, 'll');\n"
		"if (res != 'hello world') throw new Error();\n"
		"var str2 = 'hello world';\n"
		"var res2 = str2.replace(/l{2}/g, '11');\n"
		"if (res2 != 'he11o world') throw new Error();\n"
		"var str3 = 'hellllo world';\n"
		"var res3 = str3.replace(/l{2,4}/g, '11');\n"
		"if (res2 != 'he11o world') throw new Error();\n"
	);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	js_pushundefined(J);
	js_pcall(J, 0);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	js_getglobal(J, "res");
	mu_assert_string_eq("hello world", js_tostring(J, -1));
	js_getglobal(J, "res2");
	mu_assert_string_eq("he11o world", js_tostring(J, -1));
	js_getglobal(J, "res3");
	mu_assert_string_eq("he11o world", js_tostring(J, -1));
}

// test instruction size change
MU_TEST(it_should_return_proper_negative_value_from_function)
{
	js_ploadstring(J, "testfile.js", "function getNum() { return -1; }\n");
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	js_pushundefined(J);
	js_pcall(J, 0);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	js_getglobal(J, "getNum");
	js_pushundefined(J);
	js_pcall(J, 0);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
	mu_assert(js_isnumber(J, -1), "should return number");
	mu_assert_double_eq(-1, js_tonumber(J, -1));
}

// test keys lookup optimizations
MU_TEST(it_should_insert_new_entry_into_the_object) 
{
	js_ploadstring(J, "testfile.js", 
		"var ab = {};\n"
		"ab.prop = 13;\n"
		"if (ab.prop != 13) throw new Error();\n"
	);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
} 

MU_TEST(it_should_insert_new_entry_into_the_object_2) 
{
	js_ploadstring(J, "testfile.js", 
		"var ab = {};\n"
		"for (var i = 0; i < 10; i++) ab['key_' + i] = i;\n"
		"if (Object.keys(ab).length != 10) throw new Error();\n"
		"for (var i = 0; i < 10; i++)\n"
		"	if(obj['key_' + i] != i) throw new Error();\n"
	);
	mu_assert(!js_iserror(J, -1), js_tostring(J, -1));
} 

MU_TEST(it_should_swap_stack_values)
{
	js_pushnumber(J, 10);
	js_pushnumber(J, 20);
	js_pushnumber(J, 30);
	js_pushnumber(J, 40);
	mu_assert_int_eq(10, js_toint32(J, 0));
	js_swap(J, 0);
	mu_assert_int_eq(40, js_toint32(J, 0));
}

MU_TEST_SUITE(test_suite) {
	MU_SUITE_CONFIGURE(&test_setup, &test_teardown);
	MU_RUN_TEST(it_should_offset_bottom_of_stack);
	MU_RUN_TEST(it_should_rotate_and_pop_remaining_stack_values);
	MU_RUN_TEST(it_should_copy_range_of_values_to_the_end_of_stack);
	MU_RUN_TEST(it_should_call_function_in_scoped_environment);
	MU_RUN_TEST(it_should_freeze_object);
	MU_RUN_TEST(it_should_seal_object);
	MU_RUN_TEST(it_should_check_if_object_is_sealed);
	MU_RUN_TEST(it_should_check_if_object_is_frozen);
	MU_RUN_TEST(it_should_load_string_with_predefined_variables);
	MU_RUN_TEST(it_should_return_empty_function_if_source_is_also_empty);
	MU_RUN_TEST(it_should_set_exit_restore_point);
	MU_RUN_TEST(it_should_return_default_double_if_value_is_undefined);
	MU_RUN_TEST(it_should_return_default_integer_if_value_is_undefined);
	MU_RUN_TEST(it_should_store_value_in_local_registry);
	MU_RUN_TEST(it_should_internally_reference_the_object);
	MU_RUN_TEST(it_should_internally_unreference_the_object);
	MU_RUN_TEST(it_should_properly_create_substring_using_substr_function);
	MU_RUN_TEST(it_should_not_throw_error_parsing_regexpr);
	MU_RUN_TEST(it_should_return_proper_negative_value_from_function);
	MU_RUN_TEST(it_should_insert_new_entry_into_the_object);
	MU_RUN_TEST(it_should_insert_new_entry_into_the_object_2);
	MU_RUN_TEST(it_should_swap_stack_values);
}

int main(int argc, char **argv) {
	MU_RUN_SUITE(test_suite);
	MU_REPORT();
	return minunit_fail;
}
