 #include <stdio.h>
 #include "Builtins.c"
 int main(int argc, char **argv) {
obj_Obj tmp__0;
obj_Int tmp__1;
 tmp__1 = int_literal(42); // LOAD constant value
obj_Int tmp__2;
 tmp__2 = int_literal(42); // LOAD constant value
obj_Nothing tmp__3;
 tmp__3 = tmp__2->clazz->PRINT(tmp__2);
obj_Int tmp__4;
 tmp__4 = int_literal(1); // LOAD constant value
obj_Int tmp__5;
 tmp__5 = int_literal(2); // LOAD constant value
obj_Int tmp__6;
 tmp__6 = tmp__4->clazz->PLUS(tmp__4, tmp__5);
obj_Nothing tmp__7;
 tmp__7 = tmp__6->clazz->PRINT(tmp__6);
 obj_Int var_Main_Main_s; // Source variable Main_Main_s
obj_Int tmp__8;
 tmp__8 = str_literal("hello");
 var_Main_Main_s = tmp__8;
obj_Int tmp__9;
 tmp__9 = var_Main_Main_s; // Load existing variable 
obj_Nothing tmp__10;
 tmp__10 = tmp__9->clazz->PRINT(tmp__9);
 obj_Int var_Main_Main_x; // Source variable Main_Main_x
obj_Int tmp__11;
 tmp__11 = int_literal(4); // LOAD constant value
 var_Main_Main_x = tmp__11;
obj_Boolean tmp__12;
obj_Int tmp__13;
 tmp__13 = var_Main_Main_x; // Load existing variable 
obj_Int tmp__14;
 tmp__14 = int_literal(5); // LOAD constant value
 tmp__12 = tmp__13->clazz->LESS(tmp__13, tmp__14);
 if (tmp__12->value) goto and_4;
 goto else_2;
 and_4: ;
obj_Boolean tmp__15;
obj_Int tmp__16;
 tmp__16 = var_Main_Main_x; // Load existing variable 
obj_Int tmp__17;
 tmp__17 = int_literal(6); // LOAD constant value
 tmp__15 = tmp__16->clazz->GREATER(tmp__16, tmp__17);
 if (tmp__15->value) goto then_1;
 goto else_2;
 then_1: ;
 obj_Obj var_Main_Main_y; // Source variable Main_Main_y
obj_Int tmp__18;
 tmp__18 = int_literal(42); // LOAD constant value
 var_Main_Main_y = tmp__18;
 goto endif_3;
 else_2: ;
obj_Int tmp__19;
 tmp__19 = str_literal("forty-two");
 var_Main_Main_y = tmp__19;
 endif_3: ;
obj_Obj tmp__20;
 tmp__20 = var_Main_Main_y; // Load existing variable 
obj_Nothing tmp__21;
 tmp__21 = tmp__20->clazz->PRINT(tmp__20);
obj_Boolean tmp__22;
obj_Int tmp__23;
 tmp__23 = var_Main_Main_x; // Load existing variable 
obj_Int tmp__24;
 tmp__24 = int_literal(0); // LOAD constant value
 tmp__22 = tmp__23->clazz->GREATER(tmp__23, tmp__24);
 if (tmp__22->value) goto while_5;
 goto endwhile_6;
 while_5: ;
obj_Int tmp__25;
 tmp__25 = var_Main_Main_x; // Load existing variable 
obj_Nothing tmp__26;
 tmp__26 = tmp__25->clazz->PRINT(tmp__25);
obj_Int tmp__27;
 tmp__27 = var_Main_Main_x; // Load existing variable 
obj_Int tmp__28;
 tmp__28 = int_literal(1); // LOAD constant value
obj_Int tmp__29;
 tmp__29 = tmp__27->clazz->MINUS(tmp__27, tmp__28);
 var_Main_Main_x = tmp__29;
obj_Boolean tmp__30;
obj_Int tmp__31;
 tmp__31 = var_Main_Main_x; // Load existing variable 
obj_Int tmp__32;
 tmp__32 = int_literal(0); // LOAD constant value
 tmp__30 = tmp__31->clazz->GREATER(tmp__31, tmp__32);
 if (tmp__30->value) goto while_5;
 goto endwhile_6;
 goto while_5;
 endwhile_6: ;
 }
