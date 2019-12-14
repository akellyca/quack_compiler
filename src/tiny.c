#include <stdio.h>
#include "Builtins.c"
struct obj_Pt_struct;
typedef struct obj_Pt_struct *obj_Pt;
struct class_Pt_struct;
typedef struct class_Pt_struct *class_Pt;

struct obj_Blah_struct;
typedef struct obj_Blah_struct *obj_Blah;
struct class_Blah_struct;
typedef struct class_Blah_struct *class_Blah;

typedef struct obj_Pt_struct {
class_Pt clazz;
obj_Pt var_this; // Source variable this
obj_Int var_this__x; // Source variable this__x
obj_Int var_this__y; // Source variable this__y
} *obj_Pt;

struct class_Pt_struct {
obj_Pt (*constructor) (obj_Int, obj_Int);
obj_String (*STR) (obj_Pt);
obj_Nothing (*incr_x) (obj_Pt, obj_Int);
obj_Nothing (*PRINT) (obj_Obj);
};

struct class_Pt_struct the_class_Pt_struct;
class_Pt the_class_Pt;

obj_Pt new_Pt(obj_Int var_x, obj_Int var_y) {
obj_Pt this = (obj_Pt) malloc(sizeof(struct obj_Pt_struct));
this->clazz = the_class_Pt;
obj_Int tmp__0;
tmp__0 = var_x; // Load existing variable 
this->var_this__x = tmp__0;
obj_Int tmp__1;
tmp__1 = var_y; // Load existing variable 
this->var_this__y = tmp__1;
obj_Int var_z; // Source variable z
obj_Int tmp__2;
tmp__2 = int_literal(10);
var_z = tmp__2;
return this;
};

obj_String Pt_method_STR(obj_Pt this) {
obj_String tmp__0;
obj_String tmp__1;
obj_String tmp__2;
obj_String tmp__3;
obj_String tmp__4;
tmp__4 = str_literal("Pt(");
obj_String tmp__5;
obj_Int tmp__6;
tmp__6 = this->var_this__x; // Load existing variable 
tmp__5 = tmp__6->clazz->STR(tmp__6);
tmp__3 = tmp__4->clazz->PLUS(tmp__4, tmp__5);
obj_String tmp__7;
tmp__7 = str_literal(", ");
tmp__2 = tmp__3->clazz->PLUS(tmp__3, tmp__7);
obj_String tmp__8;
obj_Int tmp__9;
tmp__9 = this->var_this__y; // Load existing variable 
tmp__8 = tmp__9->clazz->STR(tmp__9);
tmp__1 = tmp__2->clazz->PLUS(tmp__2, tmp__8);
obj_String tmp__10;
tmp__10 = str_literal(")");
tmp__0 = tmp__1->clazz->PLUS(tmp__1, tmp__10);
return tmp__0;
};

obj_Nothing Pt_method_incr_x(obj_Pt this, obj_Int var_a) {
obj_Int tmp__0;
obj_Int tmp__1;
tmp__1 = this->var_this__x; // Load existing variable 
obj_Int tmp__2;
tmp__2 = var_a; // Load existing variable 
tmp__0 = tmp__1->clazz->PLUS(tmp__1, tmp__2);
this->var_this__x = tmp__0;
return nothing;
};

struct class_Pt_struct the_class_Pt_struct = {
//print out methods - based on where inherited from!!!
new_Pt, // Constructor
Pt_method_STR,
Pt_method_incr_x,
Obj_method_PRINT,

};
class_Pt the_class_Pt = &the_class_Pt_struct;

typedef struct obj_Blah_struct {
class_Blah clazz;
obj_Blah var_this; // Source variable this
obj_Int var_this__x; // Source variable this__x
obj_Int var_this__y; // Source variable this__y
} *obj_Blah;

struct class_Blah_struct {
obj_Blah (*constructor) (obj_Int);
obj_String (*STR) (obj_Blah);
obj_Nothing (*incr_x) (obj_Pt, obj_Int);
obj_Nothing (*PRINT) (obj_Obj);
};

struct class_Blah_struct the_class_Blah_struct;
class_Blah the_class_Blah;

obj_Blah new_Blah(obj_Int var_x) {
obj_Blah this = (obj_Blah) malloc(sizeof(struct obj_Blah_struct));
this->clazz = the_class_Blah;
obj_Int tmp__0;
tmp__0 = var_x; // Load existing variable 
this->var_this__x = tmp__0;
obj_Int tmp__1;
tmp__1 = int_literal(10);
this->var_this__y = tmp__1;
return this;
};

obj_String Blah_method_STR(obj_Blah this) {
obj_String tmp__0;
obj_String tmp__1;
obj_String tmp__2;
obj_String tmp__3;
obj_String tmp__4;
tmp__4 = str_literal("Blah(");
obj_String tmp__5;
obj_Int tmp__6;
tmp__6 = this->var_this__x; // Load existing variable 
tmp__5 = tmp__6->clazz->STR(tmp__6);
tmp__3 = tmp__4->clazz->PLUS(tmp__4, tmp__5);
obj_String tmp__7;
tmp__7 = str_literal(", ");
tmp__2 = tmp__3->clazz->PLUS(tmp__3, tmp__7);
obj_String tmp__8;
obj_Int tmp__9;
tmp__9 = this->var_this__y; // Load existing variable 
tmp__8 = tmp__9->clazz->STR(tmp__9);
tmp__1 = tmp__2->clazz->PLUS(tmp__2, tmp__8);
obj_String tmp__10;
tmp__10 = str_literal(")");
tmp__0 = tmp__1->clazz->PLUS(tmp__1, tmp__10);
return tmp__0;
};

struct class_Blah_struct the_class_Blah_struct = {
//print out methods - based on where inherited from!!!
new_Blah, // Constructor
Blah_method_STR,
Pt_method_incr_x,
Obj_method_PRINT,

};
class_Blah the_class_Blah = &the_class_Blah_struct;

int main(int argc, char **argv) {
obj_Int tmp__0;
tmp__0 = int_literal(42);
obj_Nothing tmp__1;
obj_Int tmp__2;
tmp__2 = int_literal(42);
tmp__1 = tmp__2->clazz->PRINT(tmp__2);
obj_Nothing tmp__3;
obj_Int tmp__4;
obj_Int tmp__5;
tmp__5 = int_literal(1);
obj_Int tmp__6;
tmp__6 = int_literal(2);
tmp__4 = tmp__5->clazz->PLUS(tmp__5, tmp__6);
tmp__3 = tmp__4->clazz->PRINT(tmp__4);
obj_Int var_x; // Source variable x
obj_Int tmp__7;
tmp__7 = int_literal(4);
var_x = tmp__7;
obj_Pt var_p; // Source variable p
obj_Int tmp__8;
tmp__8 = var_x; // Load existing variable 
obj_Int tmp__9;
tmp__9 = int_literal(2);
obj_Pt tmp__10;
tmp__10 = new_Pt(tmp__8, tmp__9); // Construct
var_p = tmp__10;
obj_Nothing tmp__11;
obj_Pt tmp__12;
tmp__12 = var_p; // Load existing variable 
obj_Int tmp__13;
tmp__13 = int_literal(5);
tmp__11 = tmp__12->clazz->incr_x(tmp__12, tmp__13);
obj_Nothing tmp__14;
obj_Pt tmp__15;
tmp__15 = var_p; // Load existing variable 
tmp__14 = tmp__15->clazz->PRINT(tmp__15);
obj_Blah var_blah; // Source variable blah
obj_Int tmp__16;
tmp__16 = int_literal(4);
obj_Blah tmp__17;
tmp__17 = new_Blah(tmp__16); // Construct
var_blah = tmp__17;
obj_Nothing tmp__18;
obj_Blah tmp__19;
tmp__19 = var_blah; // Load existing variable 
obj_Int tmp__20;
tmp__20 = int_literal(1);
tmp__18 = tmp__19->clazz->incr_x(tmp__19, tmp__20);
obj_Nothing tmp__21;
obj_Blah tmp__22;
tmp__22 = var_blah; // Load existing variable 
tmp__21 = tmp__22->clazz->PRINT(tmp__22);
obj_String var_s; // Source variable s
obj_String tmp__23;
tmp__23 = str_literal("hello");
var_s = tmp__23;
obj_Nothing tmp__24;
obj_String tmp__25;
tmp__25 = var_s; // Load existing variable 
tmp__24 = tmp__25->clazz->PRINT(tmp__25);
obj_Boolean tmp__26;
obj_Int tmp__27;
tmp__27 = var_x; // Load existing variable 
obj_Int tmp__28;
tmp__28 = int_literal(5);
tmp__26 = tmp__27->clazz->LESS(tmp__27, tmp__28);
if (tmp__26->value) goto and_4;
goto else_2;
and_4: ;
obj_Boolean tmp__29;
obj_Int tmp__30;
tmp__30 = var_x; // Load existing variable 
obj_Int tmp__31;
tmp__31 = int_literal(6);
tmp__29 = tmp__30->clazz->LESS(tmp__30, tmp__31);
if (tmp__29->value) goto then_1;
goto else_2;
obj_Boolean tmp__32;
then_1: ;
obj_Obj var_y; // Source variable y
obj_Int tmp__33;
tmp__33 = int_literal(42);
var_y = tmp__33;
goto endif_3;
else_2: ;
obj_String tmp__34;
tmp__34 = str_literal("forty-two");
var_y = tmp__34;
endif_3: ;
obj_Nothing tmp__35;
obj_Obj tmp__36;
tmp__36 = var_y; // Load existing variable 
tmp__35 = tmp__36->clazz->PRINT(tmp__36);
obj_Boolean tmp__37;
obj_Int tmp__38;
tmp__38 = var_x; // Load existing variable 
obj_Int tmp__39;
tmp__39 = int_literal(0);
tmp__37 = tmp__38->clazz->GREATER(tmp__38, tmp__39);
if (tmp__37->value) goto while_5;
goto endwhile_6;
obj_Boolean tmp__40;
while_5: ;
obj_Nothing tmp__41;
obj_Int tmp__42;
tmp__42 = var_x; // Load existing variable 
tmp__41 = tmp__42->clazz->PRINT(tmp__42);
obj_Int tmp__43;
obj_Int tmp__44;
tmp__44 = var_x; // Load existing variable 
obj_Int tmp__45;
tmp__45 = int_literal(1);
tmp__43 = tmp__44->clazz->MINUS(tmp__44, tmp__45);
var_x = tmp__43;
obj_Boolean tmp__46;
obj_Int tmp__47;
tmp__47 = var_x; // Load existing variable 
obj_Int tmp__48;
tmp__48 = int_literal(0);
tmp__46 = tmp__47->clazz->GREATER(tmp__47, tmp__48);
if (tmp__46->value) goto while_5;
goto endwhile_6;
goto while_5;
endwhile_6: ;
}
