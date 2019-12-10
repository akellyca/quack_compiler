//
// Created by Michal Young on 9/12/18.
//

#ifndef ASTNODE_H
#define ASTNODE_H

#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

#include <iostream>
#include "CodegenContext.h"
#include "EvalContext.h"

using namespace std;

class Semantics;
struct TypeNode;
struct MethodNode;
struct Whereami {
    Whereami(){}
    Whereami(string cn, string mn){
        classname = cn; methodname = mn;
    }
    string classname;
    string methodname;
};

namespace AST {
    // Abstract syntax tree.  ASTNode is abstract base class for all other nodes.

    // Json conversion and pretty-printing can pass around a print context object
    // to keep track of indentation, and possibly other things.
    class AST_print_context {
    public:
        int indent_; // Number of spaces to place on left, after each newline
        AST_print_context() : indent_{0} {};
        void indent() { ++indent_; }
        void dedent() { --indent_; }
    };

    class ASTNode {
    public:
        virtual string get_type(){return "ASTNode";}
        virtual string get_name(){return "";}
        virtual string infer_type(Semantics *s, Whereami whereami){return "TOP";}
        virtual int eval(EvalContext &ctxt){return 0;}//immediate eval
        virtual string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami){
            cerr << "*** No rvalue for this node ***" << endl; exit(1);//assert(false);
        }
        virtual string gen_lval(CodegenContext &ctx, Semantics *s, Whereami whereami) {
            cerr << "*** No lvalue for this node ***" << endl; exit(1);//assert(false);
        }
        virtual string gen_branch(CodegenContext &ctx, string true_branch, string false_branch, Semantics *s, Whereami whereami) {
            cerr << "*** No branching on this node ****" << endl; exit(1);//assert(false);
        }
        virtual int init_check(vector<string> *init){return 1;}
        virtual void json(ostream& out, AST_print_context& ctx){;}
        string str() {
            stringstream ss;
            AST_print_context ctx;
            json(ss, ctx);
            return ss.str();
        }
    protected:
        void json_indent(std::ostream& out, AST_print_context& ctx);
        void json_head(std::string node_kind, std::ostream& out, AST_print_context& ctx);
        void json_close(std::ostream& out, AST_print_context& ctx);
        void json_child(std::string field, ASTNode& child, std::ostream& out, AST_print_context& ctx, char sep=',');
    };

    class Stub : public ASTNode {
    public:
        string name_;
        string get_type() override {return "Stub";}
        int init_check(vector<string> *init) override { return 1; }
        explicit Stub(string name) : name_{name} {}
        void json(ostream& out, AST_print_context& ctx) override;
    };


    /*
     * Abstract base class for nodes that have sequences
     * of children, e.g., block of statements, sequence of
     * classes.  These may be able to share some operations,
     * especially when applying a method to the sequence just
     * means applying the method to each element of the sequence.
     * We need a different kind of sequence depending on type of
     * elements if we want to access elements without casting while
     * still having access to their fields.
     * (Will replace 'Seq')
     */
    template<class Kind>
    class Seq : public ASTNode {
    public:
        string kind_;
        vector<Kind *> elements_;
        string get_type() override {return kind_;}
        int init_check(vector<string> *init) override {
            for (Kind *el: elements_){
                if (!el->init_check(init)){return 0;}
            }
            return 1;
        }
        string infer_type(Semantics *s, Whereami whereami) override {
            for (Kind *el: elements_){ el->infer_type(s, whereami); }
            return kind_; 
        }
        string gen_rval(CodegenContext& ctxt, string target_reg, Semantics *s, Whereami whereami) override {
            for (Kind *el: elements_) {
                el->gen_rval(ctxt, target_reg, s, whereami);
            }
            return "success";
        }  
        Seq(string kind) : kind_{kind}, elements_{vector<Kind *>()} {}
        void append(Kind *el) { elements_.push_back(el); }
        void json(ostream &out, AST_print_context &ctx) override {
            json_head(kind_, out, ctx);
            out << "\"elements_\" : [";
            auto sep = "";
            for (Kind *el: elements_) {
                out << sep;
                el->json(out, ctx);
                sep = ", ";
            }
            out << "]";
            json_close(out, ctx);
        }

    };

    /* L_Expr nodes are AST nodes that can be evaluated for location.
     * Most can also be evaluated for value_.  An example of an L_Expr
     * is an identifier, which can appear on the left_ hand or right_ hand
     * side of an assignment.  For example, in x = y, x is evaluated for
     * location and y is evaluated for value_.
     *
     * For now, a location is just a name, because that's what we index
     * the symbol table with.  In a full compiler, locations can be
     * more complex, and typically in code generation we would have
     * LExpr evaluate to an address in a register.
     */
    class LExpr : public ASTNode {  /* Abstract base class */
    public:
        string get_type() override {return "LExpr";}
    };


    /* Identifiers like x and literals like 42 are the
    * leaves of the AST.  A literal can only be evaluated
    * for value_ (the 'eval' method), but an identifier
    * can also be evaluated for location (when we want to
    * store something in it).
    */
    class Ident : public LExpr {
    public:
        string text_;
        string get_type() override {return "Ident";}
        string get_name() override {return this->text_;}
        int init_check(vector<string> *init) override {
            // but what if calling ll in class with this.ll
            if (text_=="true"||text_=="false"){return 1;}
            if (find(init->begin(), init->end(), text_)==init->end()){
                cerr<<"Instantiation error: Variable "<<text_<<" not instantiated!"<<endl; 
                return 0;
            }
            return 1;
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        //string gen_rval(CodegenContext& ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        string gen_lval(CodegenContext& ctxt, Semantics *s, Whereami whereami) override;
        explicit Ident(string txt) : text_{txt} {}
        void json(ostream& out, AST_print_context& ctx) override;
    };


    /* A block is a sequence of statements or expressions.
     * For simplicity we'll just make it a sequence of ASTNode,
     * and leave it to the parser to build valid structures.
     */
    class Block : public Seq<ASTNode> {
    public:
        explicit Block() : Seq("Block") {}
     };



    /* Formal arguments list is a list of
     * identifier: type pairs.
     */
    class Formal : public ASTNode {
    public:
        ASTNode& var_;
        ASTNode& type_;
        std::string get_type() override {return "Formal";}
        // Init check not defined b/c always ok. Only iterating statements.
        string infer_type(Semantics *s, Whereami whereami) override;
        explicit Formal(ASTNode& var, ASTNode& type_) :
            var_{var}, type_{type_} {};
        void json(ostream& out, AST_print_context&ctx) override;
    };

    class Formals : public Seq<Formal> {
    public:
        explicit Formals() : Seq("Formals") {}
    };

    class Method : public ASTNode {
    public:
        Ident& name_;
        Formals& formals_;
        Ident& returns_; //ASTNode& returns_;
        Block& statements_;
        string get_type() override {return "Method";}
        // init_check not defined because manually iterating
        string infer_type(Semantics *s, Whereami whereami) override;
        explicit Method(Ident& name, Formals& formals, Ident& returns, Block& statements) :
          name_{name}, formals_{formals}, returns_{returns}, statements_{statements} {}
        void json(std::ostream& out, AST_print_context&ctx) override;
    };

    class Methods : public Seq<Method> {
    public:
        explicit Methods() : Seq("Methods") {}
    };




    /* An assignment has an lvalue (location to be assigned to)
     * and an expression.  We evaluate the expression and place
     * the value_ in the variable.  An assignment may also place a
     * static type constraint on a variable.  This is logically a
     * distinct operation, and could be represented as a separate statement,
     * but it's convenient to keep it in the assignment since our syntax
     * puts it there.
     */

    class Statement : public ASTNode { 
    public:
        std::string get_type() override {return "Statement";}
        int init_check(std::vector<std::string> *init) override {return 1;}
    };

    class Assign : public Statement {
    public:
        ASTNode &lexpr_;
        ASTNode &rexpr_;
        string get_type() override {return "Assign";}
        int init_check(vector<string> *init) override {
            int success = rexpr_.init_check(init);
            if (!success){
                cerr<<"Instantiation error: RHS of Assign statement not defined."<<endl;
                return 0;
            }
            string lhs = lexpr_.get_name();
            if (find(init->begin(), init->end(), lhs)==init->end()){
                init->push_back(lhs);
            }
            return 1;
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        string gen_rval(CodegenContext& ctxt, string target_reg,Semantics *s, Whereami whereami) override;
        explicit Assign(ASTNode &lexpr, ASTNode &rexpr) :
           lexpr_{lexpr}, rexpr_{rexpr} { }
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class AssignDeclare : public Assign {
    public:
        Ident &static_type_;
        std::string get_type() override {return "AssignDeclare";}
        int init_check(vector<string> *init) override {
            int success = rexpr_.init_check(init);
            if (!success){
                cerr<<"Instantiation error: RHS of AssignDeclare statement not defined."<<endl;
                return 0;
            }
            string lhs = lexpr_.get_name();
            if (find(init->begin(), init->end(), lhs)==init->end()){
                init->push_back(lhs);
            } else {
                cerr<<"Instantiation error: In AssignDeclare. Variable "<<lhs<<" already defined."<<endl;
                return 0;
            }
            return 1;
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        // Inherits gen_rval from Assign
        //string gen_rval(CodegenContext& ctxt, string target_reg,Semantics *s, Whereami whereami) override;
        explicit AssignDeclare(ASTNode &lexpr, ASTNode &rexpr, Ident &static_type) :
            Assign(lexpr, rexpr), static_type_{static_type} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    /* A statement could be just an expression ... but
     * we might want to interpose a node here.
     */
    class Expr : public Statement {
    public:
        std::string get_type() override {return "Expr";} 
    };

    /* When an expression is an LExpr, we
     * the LExpr denotes a location, and we
     * need to load it.
     */
    class Load : public Expr {
    public:
        LExpr &loc_;
        std::string get_type() override {return "Load";}
        std::string get_name() override { return loc_.get_name(); }
        int init_check(std::vector<std::string> *init) override {
            return loc_.init_check(init);
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        string gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) override;
        Load(LExpr &loc) : loc_{loc} {}
        void json(std::ostream &out, AST_print_context &ctx) override;
    };



    /* 'return' statement returns value from method */
    class Return : public Statement {
    public:
        ASTNode &expr_;
        std::string get_type() override {return "Return";}
        int init_check(std::vector<std::string> *init) override {
            int s = expr_.init_check(init);
            return s;
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        explicit Return(ASTNode& expr) : expr_{expr}  {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class If : public Statement {
    public:
        ASTNode &cond_; // The boolean expression to be evaluated
        Seq<ASTNode> &truepart_; // Execute this block if the condition is true
        Seq<ASTNode> &falsepart_; // Execute this block if the condition is false
        std::string get_type() override {return "If";}
        int init_check(std::vector<std::string> *init) override {
            int c = cond_.init_check(init);
            if (!c){
                cerr<<"Instantiation error: in if condition."<<endl;
                return 0; //condition is ill-defined
            }
            std::vector<std::string> ti = (*init);
            if (!truepart_.init_check(&ti)){ //vars in truepart not defined
                cerr<<"Instantiation error: in true part of if."<<endl;return 0;
            }
            
            std::vector<std::string> fi = (*init);
            if (!falsepart_.init_check(&fi)){ //vars in falsepart not defined
                cerr<<"Instantiation error: in false part of if."<<endl;return 0;
            }
            
            for (std::string v: ti){
                if (std::find(fi.begin(), fi.end(), v)==fi.end()){ 
                    cerr<<"Instantiation error: Inconsistent variables defined in true&false part of if."<<endl;
                    return 0; 
                }
                if (std::find(init->begin(), init->end(), v)==init->end()){
                    init->push_back(v); }
            }
            for (std::string v: fi){
                if (std::find(ti.begin(), ti.end(), v)==ti.end()){ 
                    cerr<<"Instantiation error: Inconsistent variables defined in true&false part of if."<<endl;
                    return 0; 
                }
                if (std::find(init->begin(), init->end(), v)==init->end()){
                    init->push_back(v); }
            }
            return 1;
        }
        string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        explicit If(ASTNode& cond, Seq<ASTNode>& truepart, Seq<ASTNode>& falsepart) :
            cond_{cond}, truepart_{truepart}, falsepart_{falsepart} { };
        string infer_type(Semantics *s, Whereami whereami) override;
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class While : public Statement {
    public:
        ASTNode& cond_;  // Loop while this condition is true
        Seq<ASTNode>&  body_;     // Loop body
        std::string get_type() override {return "While";}
        int init_check(std::vector<std::string> *init) override {
            int c = cond_.init_check(init);
            if (!c){
                cerr<<"Instantiation error: in while loop condition."<<endl;
                return 0; //condition is ill-defined
            }
            std::vector<std::string> tmp = (*init);
            return body_.init_check(&tmp);;
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        explicit While(ASTNode& cond, Block& body) :
            cond_{cond}, body_{body} { };
        void json(std::ostream& out, AST_print_context& ctx) override;

    };




    /* A class has a name, a list of arguments, and a body
    * consisting of a block (essentially the constructor)
    * and a list of methods.
    */
    class Class : public ASTNode {
    public:
        Ident& name_;
        Ident& super_;
        Method& constructor_;
        Methods& methods_;
        string get_type() override {return "Class";}
        // Doesn't get a "init_check" because iterating manually through
        string infer_type(Semantics *s, Whereami whereami) override;
        explicit Class(Ident& name, Ident& super,
                 Method& constructor, Methods& methods) :
            name_{name},  super_{super},
            constructor_{constructor}, methods_{methods} {};
        void json(ostream& out, AST_print_context& ctx) override;
    };

    /* A Quack program begins with a sequence of zero or more
     * class definitions.
     */
    class Classes : public Seq<Class> {
    public:
        explicit Classes() : Seq<Class>("Classes") {}
    };

    class IntConst : public Expr {
    public:
        int value_;
        std::string get_type() override { return "IntConst";}
        int init_check(vector<string> *init) override {
            return 1;
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        virtual string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        explicit IntConst(int v) : value_{v} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class Type_Alternative : public ASTNode {
    public:
        Ident& ident_;
        Ident& classname_;
        Block& block_;
        std::string get_type() override { return "Type_Alternative";}
        explicit Type_Alternative(Ident& ident, Ident& classname, Block& block) :
                ident_{ident}, classname_{classname}, block_{block} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class Type_Alternatives : public Seq<Type_Alternative> {
    public:
        explicit Type_Alternatives() : Seq("Type_Alternatives") {}
    };

    class Typecase : public Statement {
    public:
        Expr& expr_; // An expression we want to downcast to a more specific class
        Type_Alternatives& cases_;    // A case for each potential type
        std::string get_type() override {return "Typecase";}
        int init_check(vector<string> *init) override {
            return 1; // TODO
        }
        explicit Typecase(Expr& expr, Type_Alternatives& cases) :
                expr_{expr}, cases_{cases} {};
        void json(std::ostream& out, AST_print_context& ctx) override;
    };


    class StrConst : public Expr {
    public:
        std::string value_;
        std::string get_type() override {return "StrConst";}
        int init_check(vector<string> *init) override {
            return 1;
        }
        explicit StrConst(std::string v) : value_{v} {}
        string infer_type(Semantics *s, Whereami whereami) override;
        virtual string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

    class Actuals : public Seq<Expr> {
    public:
        explicit Actuals() : Seq("Actuals") {}
    };


    /* Constructors are different from other method calls. They
      * are static (not looked up in the vtable), have no receiver
      * object, and have their own type-checking rules.
      */
    class Construct : public Expr {
    public:
        Ident&  method_;           /* Method name is same as class name */
        Actuals& actuals_;    /* Actual arguments to constructor */
        std::string get_type() override {return "Construct";}
        int init_check(vector<string> *init) override{
            if (std::find(init->begin(), init->end(), method_.text_)==init->end()){
                cerr << "Instantiation error: Couldnt find constructor "<<method_.text_<<"!"<<endl;
                return 0; // check method_.text_ not in init vars
            }
            return actuals_.init_check(init);
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        //string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        explicit Construct(Ident& method, Actuals& actuals) :
                method_{method}, actuals_{actuals} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };


    /* Method calls are central to type checking and code
     * generation ... and for us, the operators +, -, etc
     * are method calls to specially named methods.
     */
    class Call : public Expr {
    public:
        Expr& receiver_;        /* Expression computing the receiver object */
        Ident& method_;         /* Identifier of the method */
        Actuals& actuals_;     /* List of actual arguments */
        std::string get_type() override {return "Call";}
        int init_check(vector<string> *init) override{
            int s1 = receiver_.init_check(init);
            int s2 = method_.init_check(init);
            int s3 = actuals_.init_check(init);
            if (s1&&s2&&s3){return 1;} 
            else {
                cerr<<"Instantiation error: Variable in call to "<<method_.get_name()<<" not defined."<<endl;
                return 0;
            }
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        string gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) override;
        explicit Call(Expr& receiver, Ident& method, Actuals& actuals) :
                receiver_{receiver}, method_{method}, actuals_{actuals} {};
        // Convenience factory for the special case of a method
        // created for a binary operator (+, -, etc).
        static Call* binop(std::string opname, Expr& receiver, Expr& arg);
        void json(std::ostream& out, AST_print_context& ctx) override;
    };


    // Virtual base class for binary operations.
    // Does NOT include +, -, *, /, etc, which
    // are method calls.
    // Does include And, Or, Dot, ...
   class BinOp : public Expr {
    public:
        std::string opsym;
        ASTNode &left_;
        ASTNode &right_;
        std::string get_type() override {return "BinOp";}
        BinOp(std::string sym, ASTNode &l, ASTNode &r) :
                opsym{sym}, left_{l}, right_{r} {};
        void json(std::ostream& out, AST_print_context& ctx) override;
    };

   class And : public BinOp {
   public:
        std::string get_type() override {return "And";}
        string infer_type(Semantics *s, Whereami whereami) override;
        int init_check(vector<string> *init) override {
            int lhs = left_.init_check(init);
            int rhs = right_.init_check(init);
            if (lhs&&rhs){return 1;}
            else {
                cerr<<"Instantiaton error: Variables in 'and' not defined."<<endl;
                return 0;
            }
        }
        string gen_rval(CodegenContext& ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        string gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) override;
        explicit And(ASTNode& left, ASTNode& right) :
            BinOp("And", left, right) {}
    
   };

    class Or : public BinOp {
    public:
        std::string get_type() override {return "Or";}
        string infer_type(Semantics *s, Whereami whereami) override;
        int init_check(vector<string> *init) override {
            int lhs = left_.init_check(init);
            int rhs = right_.init_check(init);
            if (lhs&&rhs){return 1;}
            else {
                cerr<<"Instantiaton error: Variables in 'or' not defined."<<endl;
                return 0;
            }
        }
        string gen_rval(CodegenContext& ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        string gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) override;
        explicit Or(ASTNode& left, ASTNode& right) :
                BinOp("Or", left, right) {}
    };

    class Not : public Expr {
    public:
        std::string get_type() override {return "Not";}
        string infer_type(Semantics *s, Whereami whereami) override;
        ASTNode& left_;
        int init_check(vector<string> *init) override {
            if (left_.init_check(init)){return 1;}
            else {
                cerr<<"Instantiaton error: Variables in 'and' not defined."<<endl;
                return 0;
            }
        }
        string gen_rval(CodegenContext& ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        string gen_branch(CodegenContext &ctxt, string true_branch, string false_branch, Semantics *s, Whereami whereami) override;
        explicit Not(ASTNode& left ):
            left_{left}  {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };


    /* Can a field de-reference (expr . IDENT) be a binary
     * operation?  It can be evaluated to a location (l_exp),
     * whereas an operation like * and + cannot, but maybe that's
     * ok.  We'll tentatively group it with Binop and consider
     * changing it later if we need to make the distinction.
     */

    class Dot : public LExpr {
    public:
        Expr& left_;
        Ident& right_;
        std::string get_type() override {return "Dot";}
        std::string get_name() override {
            std::string lhs = left_.get_name();
            std::string rhs = right_.get_name();
            return lhs+"."+rhs;
        }
        int init_check(vector<string> *init) override {
            if (left_.get_name()=="this"){
               if (std::find(init->begin(), init->end(), "this")==init->end()){
                cerr<<"Can't call 'this' outside of class!"<<endl;
                return 0; // not in a class: shouldn't call "this"
                } 
                string full_name = "this."+right_.get_name();
                if (std::find(init->begin(), init->end(), full_name)==init->end()){
                    cerr<<"Instantiation error: "<<left_.get_name()<<"."<<right_.get_name()<<endl;
                    return 0;
                } else { return 1; }
            }
            int s1 = left_.init_check(init);
            int s2 = 1; int s3 = 1;
            if (find(init->begin(),init->end(),right_.text_)==init->end()){
                s2 = 0;
            }
            if (find(init->begin(),init->end(),"this."+right_.text_)==init->end()){
                s3 = 0;
            }
            if(!s1 || !(s2||s3)){
                cerr<<"Instantiation error: "<<left_.get_name()<<"."<<right_.get_name()<<endl;
                return 0;
            } else { return 1;}
        }
        string infer_type(Semantics *s, Whereami whereami) override;
        explicit Dot (Expr& left, Ident& right) :
           left_{left},  right_{right} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };


    /* A program has a set of classes (in any order) and a block of
     * statements.
     */
    class Program : public ASTNode {
    public:
        Classes& classes_;
        Block& statements_;
        virtual string get_type() override {return "Program";}
        string infer_type(Semantics *s, Whereami whereami) override;
        virtual string gen_rval(CodegenContext &ctxt, string target_reg, Semantics *s, Whereami whereami) override;
        explicit Program(Classes& classes, Block& statements) :
                classes_{classes}, statements_{statements} {}
        void json(std::ostream& out, AST_print_context& ctx) override;
    };



}
#endif //ASTNODE_H