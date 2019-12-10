//
// Created by Michal Young on 9/12/18.
//

#include "ASTNode.h"
#include "semantics.cxx"
//#include "CodegenContext.h"
//#include "EvalContext.h"

#include <algorithm>
#include <vector>

using namespace std;

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    string Program::infer_type(Semantics *s, Whereami whereami){ 
        // infer classes
        classes_.infer_type(s, whereami);
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
        string type = loc_.infer_type(s, whereami);
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
        int has_method = 0;
        for (string m: s->hierarchy[call_class].methods_list){
            if (m==mname){
                has_method=1;
                // Now check proper actuals types
                MethodNode *mn = &((s->hierarchy)[call_class].methods[mname]);
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


    string Program::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        whereami.classname = "Main";
        whereami.methodname = "Main";
        classes_.gen_rval(ctxt, target_reg, s, whereami);
        statements_.gen_rval(ctxt, target_reg, s, whereami);
        return "Program";
    }

    string Assign::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string vname = lexpr_.get_name();
        string vtype = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
        string loc = lexpr_.gen_lval(ctxt, s, whereami);
        string target = rexpr_.gen_rval(ctxt, target_reg, s, whereami);
        ctxt.emit(loc + " = " + target + ";");
        return loc;
    }

    string Call::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string receiver_type = receiver_.infer_type(s,whereami);
        string mname = method_.get_name();
        string returns = s->hierarchy[receiver_type].methods[mname].returns;

        vector<string> ts = {receiver_.gen_rval(ctxt,target_reg,s,whereami)};
        for (int i=0; i<actuals_.elements_.size(); i++){
            ts.push_back(actuals_.elements_[i]->gen_rval(ctxt,target_reg,s,whereami));
        }
        string target = ctxt.alloc_reg(returns);
        string toprint = target+" = "+ts[0]+"->clazz->"+mname+"(";
        for (int i=0; i<ts.size()-1; i++){
            toprint = toprint+ts[i]+", ";
        }
        toprint = toprint+ts[ts.size()-1]+");";
        ctxt.emit(toprint);
        return target;
    }

    string Call::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami){
        string receiver_type = receiver_.infer_type(s,whereami);
        string mname = method_.get_name();
        string returns = s->hierarchy[receiver_type].methods[mname].returns;
        
        string target = ctxt.alloc_reg(returns);

        vector<string> ts = {receiver_.gen_rval(ctxt,target,s,whereami)};
        for (int i=0; i<actuals_.elements_.size(); i++){
            ts.push_back(actuals_.elements_[i]->gen_rval(ctxt,target,s,whereami));
        }
        string toprint = target+" = "+ts[0]+"->clazz->"+mname+"(";
        for (int i=0; i<ts.size()-1; i++){
            toprint = toprint+ts[i]+", ";
        }
        toprint = toprint+ts[ts.size()-1]+");";
        ctxt.emit(toprint);

        ctxt.emit(string("if (") + target + "->value) goto " + true_branch + ";");
        ctxt.emit(string("goto ") + false_branch + ";");
        return target;
    }

    // string Construct::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami){
    //     cout <<  "IN CONSTRUCT" << endl;
    //     string type = method_.infer_type(s, whereami);
    //     cout << type << endl;
    //     exit(1);
    //     return "c";
    // }


    string If::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string thenpart = ctxt.new_branch_label("then");
        string elsepart = ctxt.new_branch_label("else");
        string endpart = ctxt.new_branch_label("endif");
        cond_.gen_branch(ctxt, thenpart, elsepart, s, whereami);
        
        ctxt.emit(thenpart + ": ;");
        truepart_.gen_rval(ctxt, target_reg, s, whereami);
        ctxt.emit(string("goto ") + endpart + ";");
        ctxt.emit(elsepart + ": ;");
        falsepart_.gen_rval(ctxt, target_reg, s, whereami);
        ctxt.emit(endpart + ": ;");
        return target_reg;
    }
    string While::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string truepart = ctxt.new_branch_label("while");
        string endpart = ctxt.new_branch_label("endwhile");
        string target = cond_.gen_branch(ctxt, truepart, endpart, s, whereami);
        ctxt.emit(truepart + ": ;");
        string b = body_.gen_rval(ctxt, target, s, whereami);
        cond_.gen_branch(ctxt, truepart, endpart, s, whereami);
        ctxt.emit(string("goto ")+truepart+";");
        ctxt.emit(endpart + ": ;");
        return target_reg;
    }

    string Load::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami){
        string vname = loc_.get_name();
        string type = loc_.infer_type(s, whereami);
        string fullname;
        string target = ctxt.alloc_reg(type);
        if (vname=="true"||vname=="false"){
            fullname="lit_"+vname; type="Boolean";
            ctxt.emit(target+" = "+fullname+"; // Load true/false ");
            return target;
        }
        else {
            fullname = whereami.classname+"_"+whereami.methodname+"_"+vname;
            type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[vname];
            string loc = ctxt.get_var(fullname, type);
            ctxt.emit(target+" = "+loc+"; // Load existing variable ");
            return target;
        }
    }

    string Load::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami){
        string vname = loc_.get_name();
        string type = loc_.infer_type(s, whereami);
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
        return target;
    }

    string And::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string lv = left_.gen_rval(ctxt, target_reg, s, whereami);
        string rv = right_.gen_rval(ctxt, target_reg, s, whereami);
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
    string And::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
        string right_part = ctxt.new_branch_label("and");
        left_.gen_branch(ctxt, right_part, false_branch, s, whereami);
        ctxt.emit(right_part + ": ;");
        right_.gen_branch(ctxt, true_branch, false_branch, s, whereami);
        return right_part;
    }

    string Or::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string lv = left_.gen_rval(ctxt, target_reg, s, whereami);
        string rv = right_.gen_rval(ctxt, target_reg, s, whereami);
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
    string Or::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
        string right_part = ctxt.new_branch_label("or");
        left_.gen_branch(ctxt, true_branch, right_part, s, whereami);
        ctxt.emit(right_part + ": ;");
        right_.gen_branch(ctxt, true_branch, false_branch, s, whereami);
        return right_part;
    }

    string Not::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string lv = left_.gen_rval(ctxt, target_reg, s, whereami);
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

    string Not::gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
        left_.gen_branch(ctxt, false_branch, true_branch, s, whereami);
        return "";
    }

    string Ident::gen_lval(CodegenContext &ctxt, Semantics *s, Whereami whereami) {
        string fullname, type;
        if (text_=="true"){fullname="lit_true"; type="Boolean";}
        if (text_=="false"){fullname="lit_false"; type="Boolean";}
        else{
            fullname = whereami.classname+"_"+whereami.methodname+"_"+text_;
            type = s->hierarchy[whereami.classname].methods[whereami.methodname].types[text_];
        }
        return ctxt.get_var(fullname, type);
    }

    string IntConst::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string target = ctxt.alloc_reg("Int");
        ctxt.emit(target + " = int_literal(" + to_string(value_)
            + "); // LOAD constant value");
        return target;
    }
    string StrConst::gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) {
        string target = ctxt.alloc_reg("String");
        ctxt.emit(target + " = str_literal(\"" + value_
            + "\");"); // LOAD constant value");
        return target;
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




















