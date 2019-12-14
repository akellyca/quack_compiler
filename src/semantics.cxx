#include "ASTNode.h"
#include <iostream>
#include <map>
#include <vector>
#include <set>
#include <algorithm>

using namespace std;

struct MethodNode {
	MethodNode(){}
	MethodNode(string n){
		name = n;
	}
	string name;
	string returns;
	string inherited_from;
	vector<string> formals;
	vector<string> vars;
	map<string,string> types;
};

struct TypeNode {
	TypeNode(){}
	TypeNode(string n, string p){
		name = n; parent = p;
	}
	string name;
	string parent;
	vector<string> instance_vars;
	map<string,MethodNode> methods;
	vector<string> methods_list;
};


class Semantics {
public:
	AST::Program *root;
	vector<string> all_types; // topo-sorted vector of types
	set<string> all_methods; // for use populating
	set<string> fake_global; // for use with instantiation check
	map<string,TypeNode> hierarchy;
	int changed = 1;

	Semantics(AST::Program *rootptr){ root = rootptr; }

	//================================================//
	//================================================//

	//void* check_semantics(){
	void check_semantics(){
		// Main function for checking static semantics.
		// Builds hierarchy, then checks for validity of
			// class structure, instantiation, and types
		int built = this->build_hierarchy();
		//if (!built){ return 0; }
		if (!built){exit(1);}

		int instantiated = this->check_instantiation();
		if (!instantiated){
			cerr<<"Error: Improper variable instantiation!"<<endl;
			exit(1);//return 0;
		}

		int ok_types = this->check_types();
		if (!ok_types){
			cerr<<"Error: Inconsistency in types!"<<endl;
			exit(1);//return 0;
		}
	}

	//================================================//
	//================================================//
	int build_hierarchy(){
		// Add all built in and user defined classes to hierarchy.
		// Sort that hierarchy and check if it is valid.

		//// Create built-in type nodes
		TypeNode type = TypeNode("Obj", "TOP");
		this->add_type(type);
		type = TypeNode("Int", "Obj");
		this->add_type(type);
		type = TypeNode("String", "Obj");
		this->add_type(type);
		type = TypeNode("Boolean", "Obj");
		this->add_type(type);
		type = TypeNode("Nothing", "Obj");
		this->add_type(type);
		
		MethodNode builtin_constr;
		vector<string> constrs = {"Obj","Nothing"};
		for (string t: constrs){
			builtin_constr = MethodNode(t);
			builtin_constr.returns = t;
			hierarchy[t].methods[t] = builtin_constr;
			hierarchy[t].methods_list.push_back(t);
		}

		constrs = {"Int","String","Boolean"};
		for (string t: constrs){
			builtin_constr = MethodNode(t);
			builtin_constr.returns = t;
			builtin_constr.formals.push_back("x");
			builtin_constr.types["x"] = t;
			hierarchy[t].methods[t] = builtin_constr;
			hierarchy[t].methods_list.push_back(t);
		}

		MethodNode basic_method = MethodNode("STR");
		basic_method.returns = "String";
		basic_method.inherited_from = "Obj";
		hierarchy["Obj"].methods["STR"] = basic_method;
		hierarchy["Obj"].methods_list.push_back("STR");
		all_methods.insert("STR");

		basic_method = MethodNode("PRINT");
		basic_method.returns = "Nothing";
		basic_method.inherited_from = "Obj";
		hierarchy["Obj"].methods["PRINT"] = basic_method;
		hierarchy["Obj"].methods_list.push_back("PRINT");
		all_methods.insert("PRINT");

		basic_method = MethodNode("PLUS");
		basic_method.returns = "String";
		basic_method.inherited_from = "String";
		basic_method.formals = {"x"};
		basic_method.vars = {"x"};
		basic_method.types["x"] = "String";
		//basic_method.local_vars["x"] = "String";
		hierarchy["String"].methods["PLUS"] = basic_method;
		hierarchy["String"].methods_list.push_back("PLUS");
		basic_method.returns = "Int";
		basic_method.inherited_from = "Int";
		basic_method.formals = {"x"};
		basic_method.vars = {"x"};
		basic_method.types["x"] = "Int";
		//basic_method.local_vars["x"] = "Int";
		hierarchy["Int"].methods["PLUS"] = basic_method;
		hierarchy["Int"].methods_list.push_back("PLUS");
		all_methods.insert("PLUS");

		vector<string> basics = {"LESS", "GREATER", "EQUALS", "ATMOST", "ATLEAST",
										"TIMES", "DIVIDE", "MINUS"};
		for (string b: basics){
			basic_method = MethodNode(b);
			if (b=="TIMES"||b=="DIVIDE"||b=="MINUS"){ basic_method.returns = "Int";}
			else { basic_method.returns = "Boolean";}
			basic_method.inherited_from = "Int";
			basic_method.formals = {"x"};
			basic_method.vars = {"x"};
			basic_method.types["x"] = "Int";
			//basic_method.local_vars["x"] = "Int";
			hierarchy["Int"].methods[b] = basic_method;
			hierarchy["Int"].methods_list.push_back(b);
			all_methods.insert(b);
		}

		//// Add in user-defined classes
		vector<AST::Class*> classes = root->classes_.elements_;
		string name, super;
		for (AST::Class *c: classes){
			name = c->name_.text_;
			super = c->super_.text_;
			if ((find(all_types.begin(),all_types.end(),name)!=all_types.end())||name=="Obj"){
				cerr << "Error: cannot re-define class "<<name<<"!" << endl; exit(1);
			}
			type = TypeNode(name, super);

			// Build constructor
			AST::Method *constructor = &(c->constructor_);
			MethodNode cons = MethodNode(name);
			cons.returns = name;
			cons.inherited_from = name;
			AST::Formals *formals_node = &(constructor->formals_);
			vector<AST::Formal*> formals = formals_node->elements_;
			for (AST::Formal *f: formals){
				AST::Ident *fname = (AST::Ident*) &(f->var_);
				cons.formals.push_back(fname->text_);
				cons.vars.push_back(fname->text_);
			}
			type.methods[name] = cons;
			type.methods_list.push_back(name);
			all_methods.insert(name);

			// Add in methods
			AST::Methods *method_node = &(c->methods_);
			vector<AST::Method*> methods =  method_node->elements_;
			for (AST::Method *m: methods){
				string mname = m->name_.text_;
				MethodNode mn = MethodNode(mname);
				mn.returns = m->returns_.text_;
				mn.inherited_from = name;
				AST::Formals *formals_node = &(m->formals_);
				vector<AST::Formal*> formals = formals_node->elements_;
				for (AST::Formal *f: formals){
					AST::Ident *fname = (AST::Ident*) &(f->var_);
					mn.formals.push_back(fname->text_);
					mn.vars.push_back(fname->text_);
				}
				type.methods[m->name_.text_] = mn;
				type.methods_list.push_back(m->name_.text_);
				all_methods.insert(m->name_.text_);
			}
			this->add_type(type);
		}

		if (this->is_cyclic()){ //// Check that there are no cycles
			cerr << "Error: Class structure contains a cycle!" << endl;
			exit(1);
		}
		this->topoSort();
		this->propagate_methods();

		return 1;
	}

	//================================================//
	//================================================//
	int check_instantiation(){
		// For each class and the main body:
		// check variable instantiation.
		vector<AST::Class*> classes = root->classes_.elements_;
		vector<string> tmp;

		for (AST::Class *c: classes){
			string name = c->name_.text_;
			// check constructor instantiation
			AST::Method *constructor = &(c->constructor_);
			AST::Block *sts_node = &(constructor->statements_);
			vector<AST::Statement*> *sts = (vector<AST::Statement*> *) &(sts_node->elements_);
			vector<string> *fs = &(hierarchy[name].methods[name].formals);

			tmp = vector<string>();
			for (string f: *fs){ unique_push_back(&tmp, f); } //
			for (string t: all_types){ unique_push_back(&tmp, t); }
			for (string m: all_methods){ unique_push_back(&tmp, m); }
			unique_push_back(&tmp, "this");
			unique_push_back(&tmp, "True");
			unique_push_back(&tmp, "False");

			for (AST::Statement *st: *sts){
				int success = st->init_check(&tmp);
				if (!success){return 0;}
			}
			for (string v: tmp){
				if (v.find("this")!=string::npos){
					unique_push_back(&hierarchy[name].instance_vars, v);
				}
			}	

			// check instantiation of each method
			AST::Methods *method_node = &(c->methods_);
			vector<AST::Method*> methods =  method_node->elements_;
			for (AST::Method *m: methods){
				string mname = m->name_.text_;
				tmp = vector<string>();
				for (string v: hierarchy[name].instance_vars){
					unique_push_back(&tmp, v);
					unique_push_back(&hierarchy[name].methods[mname].vars, v);
				}
				vector<string> *mformals = &(hierarchy[name].methods[mname].formals);
				for (string f: *mformals){ unique_push_back(&tmp, f); }
				for (string t: all_types){ unique_push_back(&tmp, t); }
				for (string m: all_methods){ unique_push_back(&tmp, m); }
				unique_push_back(&tmp, "this");
				unique_push_back(&tmp, "True");
				unique_push_back(&tmp, "False");
				sts_node =  &(m->statements_);
				sts = (vector<AST::Statement*> *) &(sts_node->elements_);
				for (AST::Statement *st: *sts){
					int success = st->init_check(&tmp);
					if (!success){return 0;}
				}
			}
		}

		// then also check instantiation of body
		AST::Block *body = &(root->statements_);
		vector<AST::Statement*> *body_sts = (vector<AST::Statement*> *) &(body->elements_);
		vector<string> body_vars;
		for (string t: all_types){ unique_push_back(&body_vars, t); }
		for (string m: all_methods){ unique_push_back(&body_vars, m); }
		unique_push_back(&body_vars, "True");
		unique_push_back(&body_vars, "False");

		for (AST::Statement *st: *body_sts){
			int success = st->init_check(&body_vars);
			if (!success){return 0;}
		}

		return 1;
	}

	//================================================//
	//================================================//
	int check_types(){
		// Do type inference
		Whereami whereami = Whereami("Main", "Main");
		TypeNode main = TypeNode("Main", "Obj");// parent???
        hierarchy["Main"] = TypeNode("Main", "Main");
        hierarchy["Main"].methods_list.push_back("Main");
        hierarchy["Main"].methods["Main"] = MethodNode("Main");

		while (changed){
			changed = 0;
			root->infer_type(this, whereami);
		}
		return 1;
	}

	//================================================//
	//================================================//
	// HELPER FUNCTIONS //
	//================================================//
	//================================================//

	void unique_update(string vname, string type, Whereami whereami){
		// NOTE: This method should only be used
			// after finding least common ancestor type!!!
		MethodNode *local = &((hierarchy)[whereami.classname].methods[whereami.methodname]);
		int found = 0;

		for (string v: local->vars){
			if (v==vname){
				found = 1;
				if (local->types[v]!=type){
					// if (vname.find("this.")!=string::npos&&whereami.classname==whereami.methodname){
					
					// 	cerr<<"Type Error: Attempting to change type of instance variable!"<<endl;
					// 	exit(1);
					// }
					local->types[v] = type;
					changed=1; 
				}
				break;
			}
		}
		if (!found){
			local->vars.push_back(vname);
			local->types[vname] = type;
			changed = 1;
		}
	}

	string get_curr_type(string vname, Whereami whereami){
		MethodNode *local = &((hierarchy)[whereami.classname].methods[whereami.methodname]);
		map<string,string>::iterator it;

		if (vname=="this"){return whereami.classname;}

		if (vname=="true"||vname=="false"){return "Boolean";}

        for (it=local->types.begin(); it!=local->types.end(); it++) {
            if (it->first==vname){
            	return it->second;
            }
        }
        
        map<string,string> *class_vars = &(hierarchy[whereami.classname].methods[whereami.classname].types);
        for (it=class_vars->begin(); it!=class_vars->end(); it++) {
            if (it->first=="this."+vname && (it->first).find("this")!=string::npos){ //load the this.vname if possible
            	return it->second;
            }
        }
		return "BOTTOM";
	}

	string type_union(string t1, string t2){
		if (t1==t2){ return t1; }
		if (t1=="BOTTOM"){ return t2; } // Knew nothing before
		if (t2=="BOTTOM"){ return t1; }
		if (t1=="TOP"||t2=="TOP"){ return "TOP"; }

		vector<string> t1_ancestors = vector<string>();
		vector<string> t2_ancestors = vector<string>();

		string parent = hierarchy[t1].name;
		while (parent!="TOP"){
			t1_ancestors.push_back(parent);
			parent = hierarchy[parent].parent;
		}

		parent = hierarchy[t2].name;
		while (parent!="TOP"){
			t2_ancestors.push_back(parent);
			parent = hierarchy[parent].parent;
		}

		int min_parents;
		if (t1_ancestors.size()<t2_ancestors.size()){
			min_parents = t1_ancestors.size(); }
		else { min_parents = t2_ancestors.size(); }

		string newtype = "TOP";
		string t1a, t2a;
		for (int i=0; i<min_parents; i++){
			t1a = t1_ancestors[t1_ancestors.size()-i-1];
			t2a = t2_ancestors[t2_ancestors.size()-i-1];
			if (t1a==t2a){ newtype = t1a; }
			else { break; }
		}
		return newtype;
	}

	int is_subtype(string subtype, string supertype){
		if (subtype=="BOTTOM"){return 1;}
		if (supertype=="TOP"){ return 1; }
		
		string parent = hierarchy[subtype].name;
		while (parent!="TOP"){
			if (parent==supertype){ return 1; }
			parent = hierarchy[parent].parent;
		}
		return 0;
	}

	int check_formals(vector<string> expected, vector<string> provided, Whereami whereami){

		return 1;
	}

	void emit_method_sig(CodegenContext &ctxt, Whereami whereami){
        MethodNode local = this->hierarchy[whereami.classname].methods[whereami.methodname];
        string toprint, type;
        if (local.inherited_from!=whereami.classname){
        	local = this->hierarchy[local.inherited_from].methods[whereami.methodname];
        	whereami.classname = local.inherited_from;//???
        }
        if (whereami.classname==whereami.methodname){
            toprint = "obj_"+local.returns+" (*constructor) (";
        } else {
            toprint = "obj_"+local.returns+" (*"+whereami.methodname+") (";
     		//if (local.inherited_from!=whereami.classname){
     			//toprint = toprint+"obj_"+local.inherited_from;
     		//}else{
        	toprint = toprint+"obj_"+whereami.classname;//added
        //}
        }
        if (local.formals.size()!=0){
        	if (whereami.classname!=whereami.methodname){toprint=toprint+", ";}
        	for (int i=0; i<local.formals.size()-1; i++){
            	type = local.types[local.formals[i]];
            	toprint = toprint+"obj_"+type+", ";
        	}
        	type = local.types[local.formals[local.formals.size()-1]];
        	toprint = toprint+"obj_"+type;
        }
        ctxt.emit(toprint+");");
    }
    
    string emit_full_sig(CodegenContext &ctxt, Whereami whereami){
    	MethodNode local = this->hierarchy[whereami.classname].methods[whereami.methodname];
        string toprint, type;
        if (whereami.methodname!=whereami.classname){
        	toprint = toprint+"obj_"+whereami.classname+" this";
        }
        if (local.formals.size()!=0){
        	if (whereami.methodname!=whereami.classname){toprint=toprint+", ";}
        	for (int i=0; i<local.formals.size()-1; i++){
            	type = local.types[local.formals[i]];
            	toprint = toprint+"obj_"+type+" var_"+local.formals[i]+", ";
        	}
        	type = local.types[local.formals[local.formals.size()-1]];
        	toprint = toprint+"obj_"+type+" var_"+local.formals[local.formals.size()-1];
        }
        return toprint;
    }

    void emit_class_struct(CodegenContext &ctxt, string cname){
    	vector<string> methods = this->hierarchy[cname].methods_list;
    	for (string m: methods){
    		MethodNode method = this->hierarchy[cname].methods[m];
    		if (m==cname){ctxt.emit("new_"+cname+", // Constructor");}
    		else{
    			ctxt.emit(method.inherited_from+"_method_"+m+",");
    		}
    	}
    	cout<<endl;
    }

	//================================================//
	//================================================//
	void propagate_instance_var_types(string clazz){
		// pass down "this" vars to all local method scopes
		vector<string> ivs = hierarchy[clazz].instance_vars;
		Whereami here;
		here.classname = clazz;
		for (string m: hierarchy[clazz].methods_list){
			if (m==clazz){continue;}//don't need to share with myself
			here.methodname = m;
			for (string iv: ivs){
				unique_update(iv, hierarchy[clazz].methods[clazz].types[iv], here);
			}
		}
	}

	void propagate_methods(){
		for (string clazz: all_types){
			TypeNode type = hierarchy[clazz];
			vector<string> children = get_children(clazz);
			map<string,MethodNode>::iterator it;
			for (string m: hierarchy[clazz].methods_list){
				if (m==clazz){ continue; } //dont propagate constructor
				for (string c: children){
					int found = 0;
					for (string childmethod: hierarchy[c].methods_list){
						if (childmethod==m){ 
							// dont need to add, but do need to typecheck!
							// TYPECHECK THIS: TODO
							found = 1; break;
						}
					}
					if (!found){ 
						MethodNode mn = MethodNode(hierarchy[clazz].methods[m]);
						//mn.inherited_from = clazz;
						mn.inherited_from = hierarchy[clazz].methods[m].inherited_from;
						hierarchy[c].methods[mn.name] = mn;
						hierarchy[c].methods_list.push_back(mn.name);
					}
				}				
			}
		}
	}

	//================================================//
	//================================================//
	private:

	int rec_check_cyclic(int ind, vector<int> visited, vector<int> to_recurse){

		visited[ind] = 1;
		to_recurse[ind] = 1;

		int j;
		if (hierarchy[all_types[ind]].parent=="TOP"){return 0;}
		for (int k=0; k<all_types.size(); k++){
			if (all_types[k]==hierarchy[all_types[ind]].parent){ j = k; break; }
		}

		if  (!visited[j] && rec_check_cyclic(j, visited, to_recurse)){
			return 1;
		} else if (to_recurse[j]){
			return 1;
		}
		return 0;
	}

	int is_cyclic(){
		vector<int> visited;
		vector<int> to_recurse;

		for (int i=0; i<all_types.size(); i++){
			visited.push_back(0);
			to_recurse.push_back(0);
		}
		for (int i=0; i<all_types.size(); i++){
			if (visited[i]){ continue; }
			if ( rec_check_cyclic(i, visited, to_recurse) ){
				return 1; // found a cycle
			}
		}
		return 0;
	}

	void topoSortUtil(int ind, vector<int> &visited, vector<int> &order) { 
		visited[ind] = 1;
		order.push_back(ind);
		// recurse for each child
		string curr_type = all_types[ind];
		vector<int> children;
		for (int i=0; i<all_types.size(); i++){
			if (hierarchy[all_types[i]].parent==curr_type){
				children.push_back(i);
			}
		}
		for (int i: children){
			if (!visited[i]){ topoSortUtil(i, visited, order); }
			else { break; } // found a cycle! shouldn't happen!
		}
	} 
	void topoSort() { 
		vector<int> visited; //ordered by all_types
		vector<int> order; // toposorted classes ordered as above

		for (int i=0; i<all_types.size(); i++){ visited.push_back(0); }
		for (int i=0; i<all_types.size(); i++){
			if (!visited[i]){ topoSortUtil(i, visited, order); }	
		}
		if (order.size()!=all_types.size()){
			cerr<<"Issue with toposort! Wrong number of classes output."<<endl;
			exit(1);
		}
		for (int i=0; i<order.size(); i++){
			this->all_types[i] = all_types[order[i]];
		}
	} 

	vector<string> get_children(string type){
		vector<string> children;
		for (string t: all_types){
			if (hierarchy[t].parent==type){ children.push_back(t); }
		}
		return children;
	}

	void unique_push_back(vector<string> *vec, string val){
		if (std::find(vec->begin(), vec->end(), val)==vec->end()){ 
					vec->push_back(val); }
	}

	void add_type(TypeNode type){
		this->hierarchy[type.name] = type;
		this->all_types.push_back(type.name);
	}


};




























