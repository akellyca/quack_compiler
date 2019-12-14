//
// Created by Michal Young on 9/12/18.
//

#include "ASTNode.h"
#include "semantics.cxx"

#include <algorithm>
#include <vector>

using namespace std;

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    string Program::infer_type(Semantics *s, Whereami whereami){ 
        // infer classes
        classes_.infer_type(s, whereami);
        // for (string c: s->all_types){
        //     s->propagate_instance_var_types(c);
        // }

        // then do same for main body
        whereami.classname = "Main";
        whereami.methodname = "Main";
        statements_.infer_type(s, whereami);
        return "Program";
    }

    string Class::infer_type(Semantics *s, Whereami whereami){
        whereami.classname = name_.text_;
        whereami.methodname = name_.text_;
        s->hierarchy[name_.text_].methods[name_.text_].types["this"] = name_.text_;
        constructor_.infer_type(s, whereami);
        
        // all "this" vars should now be instantiated. Pass down types!
        s->propagate_instance_var_types(name_.text_);

        for (AST::Method *m: methods_.elements_){
            whereami.methodname = m->name_.text_;
            m->infer_type(s, whereami);
        }
        return name_.text_;
    }

    string Method::infer_type(Semantics *s, Whereami whereami){
        MethodNode *local = &((s->hierarchy)[whereami.classname].methods[whereami.methodname]);
        //cerr<<"Processing method "<<whereami.methodname<<" in "<<whereami.classname<<" from "<<local->inherited_from<<endl;

        formals_.infer_type(s, whereami);

        statements_.infer_type(s, whereami);
        return local->returns; // should already be filled in & checked
    }


    string Assign::infer_type(Semantics *s, Whereami whereami){
        string vname = lexpr_.get_name();
        string lhs_type = lexpr_.infer_type(s, whereami);
        string rhs_type = rexpr_.infer_type(s, whereami);
        string new_type = s->type_union(lhs_type, rhs_type);
        s->unique_update(vname, new_type, whereami);
        return new_type;
    }

    string AssignDeclare::infer_type(Semantics *s, Whereami whereami){
        //if (whereami.classname=="__main__"){cout<<"HERE"<<endl;exit(1);}
        string vname = lexpr_.get_name();
        string rhs_type = rexpr_.infer_type(s, whereami);
        MethodNode *local = &((s->hierarchy)[whereami.classname].methods[whereami.methodname]);

        // Don't do propagating up of type!!!
        int is_subtype = s->is_subtype(rhs_type, static_type_.text_);
        if (!is_subtype) { 
            cerr<<"Type error! Cannot declare "<<vname<<" as "<<static_type_.text_<<endl;
            exit(1);
        }
        s->unique_update(vname, static_type_.text_, whereami);
        return static_type_.text_;
    }

    string Dot::infer_type(Semantics *s, Whereami whereami){
        MethodNode *local = &((s->hierarchy)[whereami.classname].methods[whereami.methodname]);
        string vname = "this."+right_.get_name();//left_.get_name()+"."+right_.get_name();
        whereami.classname = left_.infer_type(s, whereami);
        string type = s->get_curr_type(vname, whereami);
        return type;
    }

    string If::infer_type(Semantics *s, Whereami whereami){
        // Take all paths! (After checking condition)
        string c = cond_.infer_type(s, whereami);
        if (c!="Boolean"){
            cerr<<"Type error: If condition not Boolean."<<endl;
            exit(1);
        }
        truepart_.infer_type(s, whereami);
        falsepart_.infer_type(s, whereami);
        return "If";
    }

    string While::infer_type(Semantics *s, Whereami whereami){
        // Take all paths! (After checking condition)
        string c = cond_.infer_type(s, whereami);
        if (c!="Boolean"){
            cerr<<"Type error: If condition not Boolean."<<endl;
            exit(1);
        }
        body_.infer_type(s, whereami);
        return "While";
    }

    string Load::infer_type(Semantics *s, Whereami whereami){
        string vname = loc_.get_name();
        // if (vname.find("this")!=string::npos){
        //     s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname] = 
        // }
        string type = loc_.infer_type(s, whereami);
        //cout<<vname<<" "<<loc_.get_type()<<" "<<type<<endl;
        //cout << "Inferring load of "<<vname<<" as "<<type<<" in "<<whereami.classname<<"."<<whereami.methodname<<endl;
        return type;
    }

    string Return::infer_type(Semantics *s, Whereami whereami){
        // Question: should we check parent return type if returns==Nothing?
        string rtype = expr_.infer_type(s, whereami);
        // Now ensure that it complies with stated return type
        MethodNode *local = &((s->hierarchy)[whereami.classname].methods[whereami.methodname]);
        if (rtype!=local->returns){ 
            cerr << "Type error: Incompatible return type for method "<<whereami.classname<<"."<<whereami.methodname<<endl;
            cerr << "Trying to return "<<rtype<<" but should be "<<local->returns<<endl;
            exit(1);
        }
        return rtype;
    }

    string Call::infer_type(Semantics *s, Whereami whereami){
        // // Expr receiver_, Ident method_, Actuals actuals_

        string inclass = whereami.classname;
        string inmethod = whereami.methodname;
        string call_class = receiver_.infer_type(s, whereami);
        string mname = method_.get_name();
        //cerr<<"Processing call "<<call_class<<"."<<mname<<endl;
        int has_method = 0;
        for (string m: s->hierarchy[call_class].methods_list){
            //if(call_class=="Blah"){cerr<<m<<endl;}
            if (m==mname){
                has_method=1;
                // Now check proper actuals types
                MethodNode *mn = &((s->hierarchy)[call_class].methods[mname]);
                if (mn->inherited_from!=call_class){
                    //cerr<<"Changing call loc to "<<mn->inherited_from<<endl;
                    mn = &((s->hierarchy)[mn->inherited_from].methods[mname]);
                    //cerr<<(s->hierarchy)[mn->inherited_from].methods[mname].formals.size()<<endl;
                }
                int n_expected = mn->formals.size();
                int n_provided = actuals_.elements_.size();
                if (n_expected!=n_provided){
                    cerr<<"Type error: Expected "<<n_expected<<" args to call ";
                    cerr<<call_class<<"."<<mname<<". Provided "<<n_provided<<"."<<endl;
                    exit(1);
                }
                for (int i=0; i<n_expected; i++){
                    string expected = mn->types[mn->formals[i]];
                    string provided = (actuals_.elements_[i])->infer_type(s, whereami);
                    if (expected!=provided){
                    cerr<<"Type error: Expected type "<<expected;
                    cerr<<" for call to "<<call_class<<"."<<mname;
                    cerr<<". Provided type "<<provided<<"."<<endl;
                    exit(1);
                    }
                }
                break;
            }
        }
        // if (method_.get_name()=="PLUS"){
        //     cout << "Done with call on "<<receiver_.get_type()<<". Found: "<<has_method<<endl;
        //     cout<<"\t with call class: "<<call_class<<", on method: "<<method_.get_name()<<endl;
        // }
        if (!has_method){
            cerr << "Type error: Class "<<call_class<<" has no method "<<mname<<endl;
            exit(1);
        }
        string rtype = s->hierarchy[call_class].methods[mname].returns;
        return rtype; // pass back proper type
    }

    string Construct::infer_type(Semantics *s, Whereami whereami){
        string cname = method_.get_name();
        MethodNode *constr = &((s->hierarchy)[cname].methods[cname]);
        vector<string> formals = constr->formals;
        int n_expected = constr->formals.size();
        int n_provided = actuals_.elements_.size();
        if (n_expected!=n_provided){
            cerr<<"Type error: Expected "<<n_expected<<" args to constructor ";
            cerr<<cname<<". Provided "<<n_provided<<"."<<endl;
            exit(1);
        }
        for (int i=0; i<n_expected; i++){
            string expected = constr->types[constr->formals[i]];
            string provided = (actuals_.elements_[i])->infer_type(s, whereami);
            if (expected!=provided){
            cerr<<"Type error: Expected type "<<expected<<" for variable ";
            cerr<<constr->formals[i]<<" for constructor "<<cname;
            cerr<<". Provided type "<<provided<<"."<<endl;
            exit(1);
            }
        }
        string rtype = constr->returns;
        return rtype;
    }

    string Formal::infer_type(Semantics *s, Whereami whereami){
        MethodNode *local = &((s->hierarchy)[whereami.classname].methods[whereami.methodname]);
        // these should just be calls to Ident::get_name() but should be same?
        string var_name = var_.get_name();
        string type = type_.get_name();
        // FORMAL VARIABLE TYPES CANNOT BE CHANGED
            // BUT assigning to as subtype is fine
        local->types[var_name] = type;
        return "Formal";
    }

    string And::infer_type(Semantics *s, Whereami whereami){
        string ltype = left_.infer_type(s,whereami);
        string rtype = right_.infer_type(s,whereami);
        if (ltype!="Boolean"||rtype!="Boolean"){
            cerr<<"Type error: Attempting boolean operation 'and' ";
            cerr<<"on non-boolean input."<<endl;
            exit(1);
        }
        return "Boolean";
    }
    string Or::infer_type(Semantics *s, Whereami whereami){
        string ltype = left_.infer_type(s,whereami);
        string rtype = right_.infer_type(s,whereami);
        if (ltype!="Boolean"||rtype!="Boolean"){
            cerr<<"Type error: Attempting boolean operation 'or' ";
            cerr<<"on non-boolean input."<<endl;
            exit(1);
        }
        return "Boolean";
    }
    string Not::infer_type(Semantics *s, Whereami whereami){
        string ltype = left_.infer_type(s,whereami);
        if (ltype!="Boolean"){
            cerr<<"Type error: Attempting boolean operation 'not' ";
            cerr<<"on non-boolean input."<<endl;
            exit(1);
        }
        return "Boolean";
    }

    string Ident::infer_type(Semantics *s, Whereami whereami){
        return s->get_curr_type(text_, whereami);
    }

    string IntConst::infer_type(Semantics *s, Whereami whereami){
        return "Int";
    }
    string StrConst::infer_type(Semantics *s, Whereami whereami){
        return "String";
    }

    //===================================================//
    //===================================================//
            //=========  CODE GENERATiON ========//
    //===================================================//
    //===================================================//


    string Program::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        ctxt.emit("#include <stdio.h>");
        ctxt.emit("#include \"Builtins.c\"");

        classes_.emit_obj(ctxt, s, whereami); // ensure namespace exists
        classes_.gen_rval(ctxt, s, whereami);

        ctxt.emit("int main(int argc, char **argv) {");
        whereami.classname = "Main";
        whereami.methodname = "Main";
        //CodegenContext *bodyctxt = new CodegenContext(ctxt);
        CodegenContext bodyctxt(cout);
        //target_reg = ctxt.alloc_reg("Obj");
        statements_.gen_rval(bodyctxt, s, whereami);
        ctxt.emit("}");
        return "";
    }

    void Class::emit_obj(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        ctxt.define_class_structs(name_.text_);
        ctxt.emit("");
    }

    string Class::gen_rval(CodegenContext &octxt, Semantics *s, Whereami whereami){
        CodegenContext ctxt(cout);
        string cname = name_.text_;
        whereami.classname = cname;
        ctxt.emit("typedef struct obj_"+cname+"_struct {");
        ctxt.emit("class_"+cname+" clazz;");
        string type, fullname, loc;
        vector<string> insts = s->hierarchy[cname].instance_vars;
        for (string v:s->hierarchy[cname].instance_vars){
            type = s->hierarchy[cname].methods[cname].types[v];
            string ivar = ctxt.get_var(v, type);
        }
        ctxt.emit("} *obj_"+cname+";");
        ctxt.emit("");
        ctxt.emit("struct class_"+cname+"_struct {");
        for (string m: s->hierarchy[cname].methods_list){
            whereami.methodname = m;
            s->emit_method_sig(ctxt, whereami);
        }
        ctxt.emit("};");
        ctxt.emit("");
        ctxt.emit("struct class_"+cname+"_struct the_class_"+cname+"_struct;");
        ctxt.emit("class_"+cname+" the_class_"+cname+";");
        ctxt.emit("");

        whereami.methodname = cname;
        //CodegenContext *construct_con = new CodegenContext(ctxt);
        constructor_.gen_rval(ctxt, s, whereami);
        methods_.gen_rval(ctxt, s, whereami);

        ctxt.emit("struct class_"+cname+"_struct the_class_"+cname+"_struct = {");
        ctxt.emit("//print out methods - based on where inherited from!!!");
        s->emit_class_struct(ctxt, cname);
        ctxt.emit("};");

        ctxt.emit("class_"+cname+" the_class_"+cname+" = &the_class_"+cname+"_struct;");
        ctxt.emit("");
        return "";
    }

    string Method::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        //CodegenContext mctxt(cout);
        CodegenContext mctxt = ctxt;
        string cname = whereami.classname;
        string mname = name_.text_;//whereami.methodname;
        whereami.methodname = mname;
        MethodNode local = s->hierarchy[cname].methods[mname];
        if (local.inherited_from!=whereami.classname){
            local = s->hierarchy[local.inherited_from].methods[whereami.methodname];
        }
        string returns = local.returns;

        for (string f: local.formals){
            string internal = "var_"+f;
            mctxt.set_var(f, internal);
        }
        if (cname==mname){ // in constructor
            mctxt.emit("obj_"+cname+" new_"+cname+"("+s->emit_full_sig(mctxt,whereami)+") {");
            mctxt.emit("obj_"+cname+" this = (obj_"+cname+") malloc(sizeof(struct obj_"+cname+"_struct));");
            mctxt.emit("this->clazz = the_class_"+cname+";");
            statements_.gen_rval(mctxt, s, whereami);
            mctxt.emit("return this;");
        } else {
            mctxt.emit("obj_"+returns+" "+cname+"_method_"+mname+"("+s->emit_full_sig(mctxt,whereami)+") {");
            statements_.gen_rval(mctxt, s, whereami);
            if (returns=="Nothing"){mctxt.emit("return nothing;");}
        }
        mctxt.emit("};");
        mctxt.emit("");
        return "";
    }

    string Return::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        string type = expr_.infer_type(s,whereami);
        string target = expr_.gen_rval(ctxt, s, whereami);
        ctxt.emit("return "+target+";");
        return target;
    }

    string Assign::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string vname = lexpr_.get_name();
        //string vtype = lexpr_.infer_type(s,whereami);
        string vtype = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        string loc = lexpr_.gen_lval(ctxt, s, whereami);
        string target = rexpr_.gen_rval(ctxt, s, whereami);
        ctxt.emit(loc + " = " + target + ";");
        return target;
    }

    string Construct::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string cname = method_.get_name();
        string toemit = actuals_.gen_lval(ctxt, s, whereami);
        string target = ctxt.alloc_reg(cname);
        ctxt.emit(target+" = new_"+cname+"("+toemit+"); // Construct");
        return target;
    }

    string Load::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        string vname = loc_.get_name();
        //string type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        string type = loc_.infer_type(s, whereami);
        // if(whereami.classname=="Pt"&&whereami.methodname=="PLUS"){
        //     cout<<"LOAD: "<<vname<<" " <<type<<endl;
        // }
        string fullname;
        string target;
        if (vname=="true"||vname=="false"){
            fullname="lit_"+vname; type="Boolean";
            target = ctxt.alloc_reg("Boolean");
            ctxt.emit(target+" = "+fullname+"; // Load true/false ");
        }
        else {
            fullname = whereami.classname+"_"+whereami.methodname+"_"+vname;
            //type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
            string loc = ctxt.get_var(vname, type);
            target = ctxt.alloc_reg(type);
        //     if(whereami.classname=="Pt"&&whereami.methodname=="PLUS"){
        //     cout<<"TYPE: "<<target<<" " <<type<<endl;
        // }
            //ctxt.emit(target+" = "+fullname+"; // Load existing variable ");
            ctxt.emit(target+" = "+loc+"; // Load existing variable ");
        }
        return target;
    }

    string Call::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        // if(whereami.classname=="Pt"&&whereami.methodname=="PLUS"){
        //     cout<<"CALL: "<<receiver_.get_type()<<endl;
        // }
        string cname = receiver_.infer_type(s, whereami);
        //string cname = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        string mname = method_.get_name();
        string rtype = s->hierarchy[cname].methods[mname].returns;
        string target = ctxt.alloc_reg(rtype);
        string rloc = receiver_.gen_rval(ctxt, s, whereami);
        string actuals = rloc;
        if (actuals_.elements_.size()!=0){actuals+=", ";}
        actuals = actuals + actuals_.gen_lval(ctxt, s, whereami);
        ctxt.emit(target+" = "+rloc+"->clazz->"+mname+"("+actuals+");");
        return target;
    }
    void Call::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami){
        string cname = receiver_.infer_type(s, whereami);
        string mname = method_.get_name();
        string rtype = s->hierarchy[cname].methods[mname].returns;
        string target = ctxt.alloc_reg(rtype);
        string rloc = receiver_.gen_rval(ctxt, s, whereami);
        string actuals = rloc;
        if (actuals_.elements_.size()!=0){actuals+=", ";}
        actuals = actuals + actuals_.gen_lval(ctxt, s, whereami);
        ctxt.emit(target+" = "+rloc+"->clazz->"+mname+"("+actuals+");");
        ctxt.emit(string("if (") + target + "->value) goto " + true_branch + ";");
        ctxt.emit(string("goto ") + false_branch + ";");
    }

    string If::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        string thenpart = ctxt.new_branch_label("then");
        string elsepart = ctxt.new_branch_label("else");
        string endpart = ctxt.new_branch_label("endif");
        cond_.gen_branch(ctxt, thenpart, elsepart, s, whereami);

        string target = ctxt.alloc_reg("Boolean");
        ctxt.emit(thenpart + ": ;");
        truepart_.gen_rval(ctxt, s, whereami);
        ctxt.emit(string("goto ") + endpart + ";");
        ctxt.emit(elsepart + ": ;");
        falsepart_.gen_rval(ctxt, s, whereami);
        ctxt.emit(endpart + ": ;");
        return target;
    }

    string While::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string truepart = ctxt.new_branch_label("while");
        string endpart = ctxt.new_branch_label("endwhile");
        cond_.gen_branch(ctxt, truepart, endpart, s, whereami);
        
        string target = ctxt.alloc_reg("Boolean");
        ctxt.emit(truepart + ": ;");
        string b = body_.gen_rval(ctxt, s, whereami);
        cond_.gen_branch(ctxt, truepart, endpart, s, whereami);
        ctxt.emit(string("goto ")+truepart+";");
        ctxt.emit(endpart + ": ;");
        return target;
    }

    void Load::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami){
        string vname = loc_.get_name();
        string type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        //string type = loc_.infer_type(s, whereami);
        string fullname;
        string target = ctxt.alloc_reg(type);
        if (vname=="true"||vname=="false"){
            fullname="lit_"+vname; type="Boolean";
            ctxt.emit(target+" = "+fullname+"; // Load true/false ");
        }
        else {
            fullname = whereami.classname+"_"+whereami.methodname+"_"+vname;
            type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
            string loc = ctxt.get_var(fullname, type);
            ctxt.emit(target+" = "+loc+"; // Load existing variable ");
        }
        ctxt.emit(string("if (") + target + "->value) goto " + true_branch + ";");
        ctxt.emit(string("goto ") + false_branch + ";");
    }


    string Load::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        string vname = loc_.get_name();
        string vtype = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        //vname = whereami.classname+"_"+whereami.methodname+"_"+vname;
        return ctxt.get_var(vname, vtype);
    }


    string Dot::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        string vname = this->get_name();
        string vtype = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        // if (whereami.classname==whereami.methodname){
        //     return "new_thing->"+ctxt.get_var(vname,vtype);
        // }
        return ctxt.get_var(vname,vtype);
    }

    string Actuals::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami){
        vector<string> locs;
        for (Expr *a: elements_){
            string type = a->infer_type(s, whereami);
            //string loc = ctxt.alloc_reg(type);
            //cout<<"Actual "<<loc<<" "<<a->get_type()<<endl;
            string loc = a->gen_rval(ctxt, s, whereami);
            locs.push_back(loc);
        }
        string toemit;
        for (string l: locs){
            toemit = toemit+l+", ";
        }
        if (locs.size()!=0){toemit = toemit.substr(0, toemit.size()-2);}
        return toemit;
    }

    string And::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string lv = left_.gen_rval(ctxt, s, whereami);
        string rv = right_.gen_rval(ctxt, s, whereami);
        string target = ctxt.alloc_reg("Boolean");

        string thenpart = ctxt.new_branch_label("then");
        string elsepart = ctxt.new_branch_label("else");
        string endpart = ctxt.new_branch_label("endif");
        ctxt.emit(string("if (")+lv+"->value && "+rv+"->value) goto "+thenpart+";");
        ctxt.emit(string("goto ")+elsepart+";");
        ctxt.emit(thenpart+": ;");
        ctxt.emit(target+" = lit_true;");
        ctxt.emit(string("goto ")+endpart+";");
        ctxt.emit(elsepart+": ;");
        ctxt.emit(target+" = lit_false;");
        ctxt.emit(endpart + ": ;");
        ctxt.free_reg(lv);
        ctxt.free_reg(rv);
        return target;
    }
    void And::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
        string right_part = ctxt.new_branch_label("and");
        left_.gen_branch(ctxt, right_part, false_branch, s, whereami);
        ctxt.emit(right_part + ": ;");
        right_.gen_branch(ctxt, true_branch, false_branch, s, whereami);
    }

    string Or::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string lv = left_.gen_rval(ctxt, s, whereami);
        string rv = right_.gen_rval(ctxt, s, whereami);
        string target = ctxt.alloc_reg("Boolean");

        string thenpart = ctxt.new_branch_label("then");
        string elsepart = ctxt.new_branch_label("else");
        string endpart = ctxt.new_branch_label("endif");
        ctxt.emit(string("if (")+lv+"->value || "+rv+"->value) goto "+thenpart+";");
        ctxt.emit(string("goto ")+elsepart+";");
        ctxt.emit(thenpart+": ;");
        ctxt.emit(target+" = lit_true;");
        ctxt.emit(string("goto ")+endpart+";");
        ctxt.emit(elsepart+": ;");
        ctxt.emit(target+" = lit_false;");
        ctxt.emit(endpart + ": ;");
        ctxt.free_reg(lv);
        ctxt.free_reg(rv);
        return target;
    }
    void Or::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
        string right_part = ctxt.new_branch_label("or");
        left_.gen_branch(ctxt, true_branch, right_part, s, whereami);
        ctxt.emit(right_part + ": ;");
        right_.gen_branch(ctxt, true_branch, false_branch, s, whereami);
    }

    string Not::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string lv = left_.gen_rval(ctxt, s, whereami);
        string target = ctxt.alloc_reg("Boolean");

        string thenpart = ctxt.new_branch_label("then");
        string elsepart = ctxt.new_branch_label("else");
        string endpart = ctxt.new_branch_label("endif");
        ctxt.emit(string("if (!")+lv+"->value) goto "+thenpart+";");
        ctxt.emit(string("goto ")+elsepart+";");
        ctxt.emit(thenpart+": ;");
        ctxt.emit(target+" = lit_true;");
        ctxt.emit(string("goto ")+endpart+";");
        ctxt.emit(elsepart+": ;");
        ctxt.emit(target+" = lit_false;");
        ctxt.emit(endpart + ": ;");
        ctxt.free_reg(lv);
        return target;
    }
    void Not::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
        left_.gen_branch(ctxt, false_branch, true_branch, s, whereami);
    }

    string Ident::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string fullname, type;
        if (text_=="true"){fullname="lit_true"; type="Boolean";}
        if (text_=="false"){fullname="lit_false"; type="Boolean";}
        else{
            fullname = whereami.classname+"_"+whereami.methodname+"_"+text_;
            type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[text_];
        }
        //return ctxt.get_var(fullname, type);
        return ctxt.get_var(text_, type);
    }

    string IntConst::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        //if(target_reg==""){target_reg = ctxt.alloc_reg("Int");}
        string target = ctxt.alloc_reg("Int");
        ctxt.emit(target + " = int_literal(" + to_string(value_) + ");");
        return target;
        //ctxt.emit(target_reg + " = int_literal(" + to_string(value_) + ");");
    }
    string IntConst::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        return "int_literal("+to_string(value_)+")";
    }

    string StrConst::gen_rval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        //if(target_reg==""){target_reg = ctxt.alloc_reg("String");}
        string target = ctxt.alloc_reg("String");
        ctxt.emit(target + " = str_literal(\"" + value_ + "\");");
        return target;
    }
    string StrConst::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        return "str_literal(\""+value_+"\")";
    }


    //===================================================//
    //===================================================//
            //=========  JSON PRINTING ========//
    //===================================================//
    //===================================================//

    // JSON representation of all the concrete node types.
    // This might be particularly useful if I want to do some
    // tree manipulation in Python or another language.  We'll
    // do this by emitting into a stream.

    // --- Utility functions used by node-specific json output methods

    /* Indent to a given level */
    void ASTNode::json_indent(std::ostream& out, AST_print_context& ctx) {
        if (ctx.indent_ > 0) {
            out << std::endl;
        }
        for (int i=0; i < ctx.indent_; ++i) {
            out << "    ";
        }
    }

    /* The head element looks like { "kind" : "block", */
    void ASTNode::json_head(std::string node_kind, std::ostream& out, AST_print_context& ctx) {
        json_indent(out, ctx);
        out << "{ \"kind\" : \"" << node_kind << "\"," ;
        ctx.indent();  // one level more for children
        return;
    }

    void ASTNode::json_close(std::ostream& out, AST_print_context& ctx) {
        // json_indent(out, ctx);
        out << "}";
        ctx.dedent();
    }

    void ASTNode::json_child(std::string field, ASTNode& child, std::ostream& out, AST_print_context& ctx, char sep) {
        json_indent(out, ctx);
        out << "\"" << field << "\" : ";
        child.json(out, ctx);
        out << sep;
    }

    void Stub::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Stub", out, ctx);
        json_indent(out, ctx);
        out  << "\"rule\": \"" <<  name_ << "\"";
        json_close(out, ctx);
    }


    void Program::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Program", out, ctx);
        json_child("classes_", classes_, out, ctx);
        json_child("statements_", statements_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Formal::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Formal", out, ctx);
        json_child("var_", var_, out, ctx);
        json_child("type_", type_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Method::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Method", out, ctx);
        json_child("name_", name_, out, ctx);
        json_child("formals_", formals_, out, ctx);
        json_child("returns_", returns_, out, ctx);
        json_child("statements_", statements_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Assign::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Assign", out, ctx);
        json_child("lexpr_", lexpr_, out, ctx);
        json_child("rexpr_", rexpr_, out, ctx, ' ');
        json_close(out, ctx);
     }

    void AssignDeclare::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Assign", out, ctx);
        json_child("lexpr_", lexpr_, out, ctx);
        json_child("rexpr_", rexpr_, out, ctx);
        json_child("static_type_", static_type_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Return::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Return", out, ctx);
        json_child("expr_", expr_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void If::json(std::ostream& out, AST_print_context& ctx) {
        json_head("If", out, ctx);
        json_child("cond_", cond_, out, ctx);
        json_child("truepart_", truepart_, out, ctx);
        json_child("falsepart_", falsepart_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void While::json(std::ostream& out, AST_print_context& ctx) {
        json_head("While", out, ctx);
        json_child("cond_", cond_, out, ctx);
        json_child("body_", body_, out, ctx, ' ');
        json_close(out, ctx);
    }


    void Typecase::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Typecase", out, ctx);
        json_child("expr_", expr_, out, ctx);
        json_child("cases_", cases_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Type_Alternative::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Type_Alternative", out, ctx);
        json_child("ident_", ident_, out, ctx);
        json_child("classname_", classname_, out, ctx);
        json_child("block_", block_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Load::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Load", out, ctx);
        json_child("loc_", loc_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Ident::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Ident", out, ctx);
        out << "\"text_\" : \"" << text_ << "\"";
        json_close(out, ctx);
    }

    void Class::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Class", out, ctx);
        json_child("name_", name_, out, ctx);
        json_child("super_", super_, out, ctx);
        json_child("constructor_", constructor_, out, ctx);
        json_child("methods_", methods_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Call::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Call", out, ctx);
        json_child("obj_", receiver_, out, ctx);
        json_child("method_", method_, out, ctx);
        json_child("actuals_", actuals_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Construct::json(std::ostream &out, AST::AST_print_context &ctx) {
        json_head("Construct", out, ctx);
        json_child("method_", method_, out, ctx);
        json_child("actuals_", actuals_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void IntConst::json(std::ostream& out, AST_print_context& ctx) {
        json_head("IntConst", out, ctx);
        out << "\"value_\" : " << value_ ;
        json_close(out, ctx);
    }

    void StrConst::json(std::ostream& out, AST_print_context& ctx) {
        json_head("StrConst", out, ctx);
        out << "\"value_\" : \"" << value_  << "\"";
        json_close(out, ctx);
    }


    void BinOp::json(std::ostream& out, AST_print_context& ctx) {
        json_head(opsym, out, ctx);
        json_child("left_", left_, out, ctx);
        json_child("right_", right_, out, ctx, ' ');
        json_close(out, ctx);
    }


    void Not::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Not", out, ctx);
        json_child("left_", left_, out, ctx, ' ');
        json_close(out, ctx);
    }

    void Dot::json(std::ostream& out, AST_print_context& ctx) {
        json_head("Dot", out, ctx);
        json_child("left_", left_, out, ctx);
        json_child("right_", right_, out, ctx, ' ');
        json_close(out, ctx);
    }


    /* Convenience factory for operations like +, -, *, / */
    Call* Call::binop(std::string opname, Expr& receiver, Expr& arg) {
        Ident* method = new Ident(opname);
        Actuals* actuals = new Actuals();
        actuals->append(&arg);
        return new Call(receiver, *method, *actuals);
    }

}




















