#include "interpreter.h"
#include "vector.h"
#include "ops.h"
#include "sse.h"

#include <stdlib.h>

void Trace::Reset() {
	n_nodes = n_recorded = length = n_outputs = n_output_values = n_pending = 0;
	max_live_register = NULL;
}

std::string Trace::toString(State & state) {
	std::ostringstream out;
	out << "recorded: \n";
	for(size_t j = 0; j < n_nodes; j++) {
		IRNode & node = nodes[j];
		out << "n" << j << " : " << Type::toString(node.type) << " = " << IROpCode::toString(node.op) << "\t";
		switch(node.op) {
#define BINARY(op,...) case IROpCode::op: out << "n" << node.binary.a << "\tn" << node.binary.b; break;
#define UNARY(op,...) case IROpCode::op: out << "n" << node.unary.a; break;
		BINARY_ARITH_MAP_BYTECODES(BINARY)
		UNARY_ARITH_MAP_BYTECODES(UNARY)
		//ARITH_FOLD_BYTECODES(UNARY)
		//ARITH_SCAN_BYTECODES(UNARY)
		UNARY(cast)
		BINARY(seq)
		case IROpCode::loadc: out << ( (node.type == Type::Integer) ? node.loadc.i : node.loadc.d); break;
		case IROpCode::loadv: out << "$" << node.loadv.p; break;
		case IROpCode::storec: /*fallthrough*/
		case IROpCode::storev: out << "o" << node.store.dst - output_values << "\tn" << node.store.a; break;
		}
		out << "\n";
	}
	out << "outputs: \n";
	for(size_t i = 0; i < n_outputs; i++) {

		Output & o = outputs[i];
		switch(o.location.type) {
		case Trace::Location::REG:
			out << "r[" << o.location.reg.offset << "]"; break;
		case Trace::Location::VAR:
			out << state.externStr(o.location.pointer.name); break;
		}
		out << " = o" << o.value - output_values << "\n";
	}
	out << "output_values: \n";
	for(size_t i = 0; i < n_output_values; i++) {
		out << "o" << i << " : " << Type::toString(output_values[i].type) << "[" << output_values[i].length << "]\n";
	}
	return out.str();
}

#define REG(state, i) (*(state.base+i))

static const Value & get_location_value(State & state, const Trace::Location & l) {
	switch(l.type) {
	case Trace::Location::REG:
		return l.reg.base[l.reg.offset];
	default:
	case Trace::Location::VAR:
		return l.pointer.env->get(l.pointer);
	}
}
static void set_location_value(State & state, const Trace::Location & l, const Value & v) {
	switch(l.type) {
	case Trace::Location::REG:
		l.reg.base[l.reg.offset] = v;
		return;
	case Trace::Location::VAR:
		l.pointer.env->assign(l.pointer,v);
		return;
	}
}

static bool is_location_dead(Trace & trace, const Trace::Location & l) {
	bool dead = l.type == Trace::Location::REG &&
	( l.reg.base < trace.max_live_register_base ||
	  ( l.reg.base == trace.max_live_register_base &&
	    l.reg.offset > trace.max_live_register
	  )
	);
	//if(dead)
	//	printf("r%d is dead! long live r%d\n",(int)l.reg.offset,(int)trace.max_live_register);

	return dead;
}

//attempts to find a future at location l, returns true if the location is live and contains a future
static bool get_location_value_if_live(State & state, Trace & trace, const Trace::Location & l, Value & v) {
	if(is_location_dead(trace,l))
		return false;
	v = get_location_value(state,l);
	return v.isFuture();
}

void Trace::InitializeOutputs(State & state) {
	//check list of recorded output locations for live futures
	//for each live future found, replace the future with a concrete object
	//the data for the object will be filled in by the trace interpreter
	Value * values[TRACE_MAX_NODES];
	bzero(values,sizeof(Value *) * n_nodes);
	for(size_t i = 0; i < n_outputs; ) {
		Output & o = outputs[i];
		Value loc;
		if(!get_location_value_if_live(state,*this,o.location,loc)) {
			o = outputs[--n_outputs];
		} else {
			IRef ref = loc.future.ref;
			Type::Enum typ = loc.future.typ;
			if(values[ref] == NULL) { //if this is the first time we see this node as an output we crate a value for it
				Value & v = output_values[n_output_values++];
				Value::Init(v,typ,loc.length); //initialize the type of the output value, the actual value (i.e. v.p) will be set after it is calculed in the trace
				if(loc.length == length) {
					v.p = new (PointerFreeGC) double[length];
					memset(v.p,0xFF,sizeof(double) * length);
					EmitStoreV(typ,&v,ref);
				} else {
					EmitStoreC(typ,&v,ref);
				}
				Commit();
				values[ref] = &v;
			}
			set_location_value(state,o.location,Value::Nil()); //mark this location in the interpreter as already seen
			o.value = values[ref];
			i++;
		}
	}
}

void Trace::WriteOutputs(State & state) {
	for(size_t i = 0; i < n_outputs; i++) {
		set_location_value(state,outputs[i].location,*outputs[i].value);
	}
}

//#define USE_ARBB_TRACE_EXECUTE
#ifndef USE_ARBB_TRACE_EXECUTE
/*
void Trace::execute(State & state) {

	//check list of recorded output locations for live futures
	//for each live future found, replace the future with a concrete object
	//the data for the object will be filled in by the trace interpreter
	for(size_t i = 0; i < n_outputs; ) {
		Output & o = outputs[i];

		Value loc;
		if(!get_location_value_if_live(state,*this,o.location,loc)) {
			o = outputs[--n_outputs];
		} else {
			IRef ref = loc.future.ref;
			o.ref = ref; //record where the value come from
			Type::Enum typ = loc.future.typ;
			if(loc.length == length) nodes[ref].r_external = true;
			if(loc.length == length && nodes[ref].r.p == NULL) {//if this is the first VM value that refers to this output, and it is non-scalar, allocate space for it in the VM
				nodes[ref].r.p = new (PointerFreeGC) double[length];
			}
			Value v;
			Value::Init(v,typ,loc.length); //initialize the type of the output value, the actual value (i.e. v.p) will be set after it is calculed in the trace
			set_location_value(state,o.location,v);
			i++;
		}
	}

	if(state.tracing.verbose)
		printf("executing trace:\n%s\n",toString(state).c_str());

	//register allocate
	//we got backward through the trace so we see all uses before the def
	//when we encounter the first use we allocate a register
	//when we encounter the def, we free that register

	Allocator free_reg;
	Allocator_init(&free_reg);
	for(size_t i = n_nodes; i > 0; i--) {
		IRNode & n = nodes[i - 1];
		if(!n.r_external) { //outputs do not get assigned registers, we use the memory previous allocated for them to hold intermediates
			//handle the def of node n by freeing allocated register

			if(n.r.p == NULL) { //a register was never allocated for this node because this def has no uses.  This means that this op is dead code.
				              // currently we just allocate a register for it, but we can also just replace it with a nop once the interpreter supports nops.
				n.r.p = registers[Allocator_allocate(&free_reg)];
			}
			int reg = ((double*)n.r.p - registers[0]) / TRACE_VECTOR_WIDTH;
			Allocator_free(&free_reg,reg);
		}
		if( n.usesRegA() ){ //a is a register, handle the use of a
			IRNode & def = nodes[n.a.i];
			if(def.r_external) { //since 'a' refers to an output, the interpreter will need to advance the memory reference on each iteration
				                //we set a_external so the interpreter knows to advance the pointer
				n.a_external = true;
			} else if(def.r.p == NULL) { //no register has be assigned to the def. This is the first encountered use so we allocate a register for it
				int reg = Allocator_allocate(&free_reg);
				def.r.p = registers[reg];
			}
			//replace the reference to the node with the pointer to where the values will be stored, this is either a register, part of an input array, or part of an output array
			n.a.p = def.r.p;
		}
		//we handle the use of b similar to the use of a
		if(n.usesRegB()) {
			IRNode & def = nodes[n.b.i];
			if(def.r_external) {
				n.b_external = true;
			} else if (def.r.p == NULL) {
				int reg = Allocator_allocate(&free_reg);
				def.r.p = registers[reg];
			}
			n.b.p = def.r.p;
		}
	}
	
	for(int64_t i = 0; i < length; i += TRACE_VECTOR_WIDTH) {
		for(size_t j = 0; j < n_nodes; j++) {
			IRNode & node = nodes[j];
#define BINARY_CASE(opcode, typea, typeb, sva, svb) \
	((IROpCode::opcode << 4) + (typeb << 3) + (typea << 2) + (svb << 1) + sva)

#define BINARY_IMPL(opcode,nm,OP) \
	case BINARY_CASE(opcode, IROp::T_INT, IROp::T_INT, IROp::E_SCALAR, IROp::E_VECTOR): \
		Map2SV< OP<TInteger>, TRACE_VECTOR_WIDTH >::eval(state, node.a.i, (int64_t*)node.b.p, (OP<TInteger>::R*)node.r.p); break; \
	case BINARY_CASE(opcode, IROp::T_INT, IROp::T_INT, IROp::E_VECTOR, IROp::E_SCALAR): \
		Map2VS< OP<TInteger>, TRACE_VECTOR_WIDTH >::eval(state, (int64_t*)node.a.p, node.b.i, (OP<TInteger>::R*)node.r.p); break; \
	case BINARY_CASE(opcode, IROp::T_INT, IROp::T_INT, IROp::E_VECTOR, IROp::E_VECTOR): \
		Map2VV< OP<TInteger>, TRACE_VECTOR_WIDTH >::eval(state, (int64_t*)node.a.p, (int64_t*)node.b.p, (OP<TInteger>::R*)node.r.p); break; \
	case BINARY_CASE(opcode, IROp::T_DOUBLE, IROp::T_DOUBLE, IROp::E_SCALAR, IROp::E_VECTOR): \
		Map2SV< OP<TDouble>, TRACE_VECTOR_WIDTH >::eval(state, node.a.d, (double*)node.b.p, (OP<TDouble>::R*)node.r.p); break; \
	case BINARY_CASE(opcode, IROp::T_DOUBLE, IROp::T_DOUBLE, IROp::E_VECTOR, IROp::E_SCALAR): \
		Map2VS< OP<TDouble>, TRACE_VECTOR_WIDTH >::eval(state, (double*)node.a.p, node.b.d, (OP<TDouble>::R*)node.r.p); break; \
	case BINARY_CASE(opcode, IROp::T_DOUBLE, IROp::T_DOUBLE, IROp::E_VECTOR, IROp::E_VECTOR): \
		Map2VV< OP<TDouble>, TRACE_VECTOR_WIDTH >::eval(state, (double*)node.a.p, (double*)node.b.p, (OP<TDouble>::R*)node.r.p); break; \

#define UNARY_CASE(opcode, typea) \
	((IROpCode::opcode << 4) + (typea << 2) + 3)

#define UNARY_IMPL(opcode,nm,OP) \
	case UNARY_CASE(opcode, IROp::T_INT): \
		Map1< OP<TInteger>, TRACE_VECTOR_WIDTH >::eval(state, (int64_t*)node.a.p, (OP<TInteger>::R*)node.r.p); break; \
	case UNARY_CASE(opcode, IROp::T_DOUBLE): \
		Map1< OP<TDouble>, TRACE_VECTOR_WIDTH >::eval(state, (double*)node.a.p, (OP<TDouble>::R*)node.r.p); break; 

#define FOLD_IMPL(opcode,nm,OP) \
	case UNARY_CASE(opcode, IROp::T_INT): \
		if(i == 0) node.r.i = OP<TInteger>::base(); node.r.i = FoldLeftT< OP<TInteger>, TRACE_VECTOR_WIDTH >::eval(state, (int64_t*)node.a.p, node.r.i); break; \
	case UNARY_CASE(opcode, IROp::T_DOUBLE): \
		if(i == 0) node.r.d = OP<TDouble>::base(); node.r.d = FoldLeftT< OP<TDouble>, TRACE_VECTOR_WIDTH >::eval(state, (double*)node.a.p, node.r.d); break; 

#define SCAN_IMPL(opcode,nm,OP) \
	case UNARY_CASE(opcode, IROp::T_INT): \
		if(i == 0) node.b.i = OP<TInteger>::base(); node.b.i = ScanLeftT< OP<TInteger>, TRACE_VECTOR_WIDTH >::eval(state, (int64_t*)node.a.p, node.b.i, (OP<TInteger>::R*)node.r.p); break; \
	case UNARY_CASE(opcode, IROp::T_DOUBLE): \
		if(i == 0) node.b.d = OP<TDouble>::base(); node.b.i = ScanLeftT< OP<TDouble>, TRACE_VECTOR_WIDTH >::eval(state, (double*)node.a.p, node.b.d, (OP<TDouble>::R*)node.r.p); break; 

			switch(node.op.op) {
				IR_BINARY(BINARY_IMPL)
				IR_UNARY(UNARY_IMPL)
				IR_FOLD(FOLD_IMPL)
				IR_SCAN(SCAN_IMPL)
				case (IROpCode::coerce << 4) + (IROp::T_DOUBLE << 3) + (IROp::T_INT << 2) + 3:
					Map1< CastOp<Integer, Double> , TRACE_VECTOR_WIDTH>::eval(state, (int64_t*)node.a.p, (double*)node.r.p);
				break;
				case (IROpCode::seq << 4) + (IROp::T_INT << 3) + (IROp::T_INT << 2) + 0:
					Sequence<TRACE_VECTOR_WIDTH>(i*node.b.i+1, node.b.i, (int64_t*)node.r.p); 
				break;
				default:
					printf("%d (%s)(%d)\n",(int)node.op.op, IROpCode::toString(node.op.code), (int) node.op.a_typ);
					_error("Invalid op code short vector machine");
			}
			if(node.r_external) 
				node.r.p += TRACE_VECTOR_WIDTH; 
			if(node.a_external) 
				node.a.p += TRACE_VECTOR_WIDTH; 
			if(node.b_external) 
				node.b.p += TRACE_VECTOR_WIDTH; 
		}
	}


	//write values back to outputs
	for(size_t i = 0; i < n_outputs; i++) {
		Output & o = outputs[i];
		Value v = get_location_value(state,o.location);
		if(v.length > 1)
			v.p = nodes[o.ref].r.p - length; //copy pointer, adjust for shift added by interpreter
		else
			v.p = nodes[o.ref].r.p; //copy scalar value, no adjustment
		set_location_value(state,o.location,v);
	}
}*/
#else



#include <arbb_vmapi.h>



#define ARBB_RUN(fn) \
	do { \
			/*printf("%s:%d: %s\n",__FILE__,__LINE__,#fn);*/ \
			arbb_error_t e = (fn); \
			if(arbb_error_none != e) \
					panic(__FILE__,__LINE__,#fn); \
	} while(0)


//void arbb_set_error_details_null(arbb_error_details_t * d)  { *d = NULL; }
struct ArbbState {

	arbb_context_t ctx;
	arbb_error_details_t details;
	State & state;
	Trace & trace;
	arbb_function_t function;
	std::map<void *,arbb_global_variable_t> output_variables;
	std::map<void *,arbb_global_variable_t> input_variables;
	std::vector<arbb_binding_t> bindings;
	arbb_variable_t result_vars[TRACE_MAX_NODES];

	ArbbState(State & s, Trace & t)
	: state(s), trace(t) {
		arbb_set_error_details_null(&details);
		arbb_get_default_context(&ctx,&details);
	}

	arbb_global_variable_t addGlobal(IROp::Type typ, void * v) {
		arbb_type_t arbb_typ = getType(typ,false);
		uint64_t length = trace.length;
		uint64_t width = sizeof(double);
		arbb_binding_t binding;
		ARBB_RUN(arbb_create_dense_binding(ctx,&binding,v,1,&length,&width,&details));
		arbb_global_variable_t ov;
		ARBB_RUN(arbb_create_global(ctx,&ov,arbb_typ,NULL,binding,NULL,&details));
		bindings.push_back(binding);
		return ov;
	}

	arbb_opcode_t getOp(IROpCode::Enum opcode) {
#define GET_ARBB(op,str,nm) case IROpCode::op : return arbb_op_##op;
		switch(opcode) {
			IR_BINARY(GET_ARBB)
#define arbb_op_ceiling arbb_op_ceil
#define arbb_op_log arbb_op_ln
			IR_UNARY(GET_ARBB)
#undef arbb_op_ceiling
#undef arbb_op_log
			case IROpCode::coerce: return arbb_op_cast;
			default: _error("unknown op");
		}
	}

	arbb_variable_t getVar(IROp::Type typ, IROp::Encoding enc, bool isExternal, IRNode::InputReg & reg) {
		arbb_global_variable_t gv;
		if(isExternal)
			gv = input_variables[reg.p];
		else if(IROp::E_SCALAR == enc) {
			arbb_type_t atyp = getType(typ,true);
			ARBB_RUN(arbb_create_constant(ctx,&gv,atyp,&reg.d,NULL,&details));
		} else {
			return result_vars[reg.i];
		}
		arbb_variable_t v;
		ARBB_RUN(arbb_get_variable_from_global(ctx,&v,gv,&details));
		return v;
	}

	void run() {
		//find live outputs and create arbb global variables to hold them
		for(size_t i = 0; i < trace.n_outputs; ) {
			Trace::Output & o = trace.outputs[i];
			Value loc;
			if(!get_location_value_if_live(state,trace,o.location,loc)) {
				o = trace.outputs[--trace.n_outputs];
			} else {
				IRef ref = loc.future.ref;
				IRNode & def = trace.nodes[ref];
				o.ref = ref; //only for pretty printing...
				Type::Enum typ = loc.future.typ;
				def.r_external = true;
				if(def.r.p == NULL) {
					//if this is the first VM value that refers to this output, allocate space for it in the interpreter, and create a variable bound to it in the arbb vm
					def.r.p = new (PointerFreeGC) double[trace.length];
					output_variables[def.r.p] = addGlobal(typ == Type::Integer ? IROp::T_INT : IROp::T_DOUBLE, def.r.p);
				}
				Value v;
				Value::Init(v,typ,trace.length);
				v.p = def.r.p;
				set_location_value(state,o.location,v);
				i++;
			}
		}

		printf("executing trace:\n%s\n",trace.toString(state).c_str());

		//find inputs and create arbb variables bound to them
		for(size_t i = 0; i < trace.n_nodes; i++) {
			IRNode & node = trace.nodes[i];
			if(node.a_external && input_variables.count(node.a.p) == 0) {
				input_variables[node.a.p] = addGlobal(node.op.a_typ,node.a.p);
			}
			if(node.b_external && input_variables.count(node.b.p) == 0) {
				input_variables[node.b.p] = addGlobal(node.op.b_typ,node.b.p);
			}
		}
		//emit arbb function
		arbb_type_t fn_typ;
		ARBB_RUN(arbb_get_function_type(ctx,&fn_typ,0,NULL,0,NULL,&details));
		ARBB_RUN(arbb_begin_function(ctx,&function,fn_typ,NULL,true,&details));
		for(size_t i = 0; i < trace.n_nodes; i++) {
			IRNode & node = trace.nodes[i];
			arbb_type_t rtyp = getType(std::max(node.op.a_typ,node.op.b_typ),false);
			ARBB_RUN(arbb_create_local(function,&result_vars[i],rtyp,NULL,&details));
			arbb_variable_t inputs[2];
			inputs[0] = getVar(node.op.a_typ,node.op.a_enc,node.a_external,node.a);
			if(IROpCode_is_binary(node.op.code))
				inputs[1] = getVar(node.op.b_typ,node.op.b_enc,node.b_external,node.b);
			ARBB_RUN(arbb_op(function,getOp(node.op.code),&result_vars[i],inputs,NULL,NULL,&details));
			if(node.r_external) {
				arbb_variable_t o;
				ARBB_RUN(arbb_get_variable_from_global(ctx,&o,output_variables[node.r.p],&details));
				ARBB_RUN(arbb_op(function,arbb_op_copy,&o,&result_vars[i],NULL,NULL,&details));
			}
		}


		ARBB_RUN(arbb_end_function(function,&details));
		{
			arbb_string_t fn_str;
			ARBB_RUN(arbb_serialize_function(function,&fn_str,&details));
			printf("function is\n%s\n",arbb_get_c_string(fn_str));
			arbb_free_string(fn_str);
		}

		//compile and warm up the vm
		ARBB_RUN(arbb_execute(function,NULL,NULL,&details));

		//now some timing
		timespec begin = get_time();
		ARBB_RUN(arbb_execute(function,NULL,NULL,&details));
		print_time_elapsed("arbb exec",begin);

	}
	arbb_type_t getType(IROp::Type typ, bool isScalar) {
		arbb_scalar_type_t st;
		switch(typ) {
		case IROp::T_INT: st = arbb_i64; break;
		case IROp::T_DOUBLE: st = arbb_f64; break;
		default: _error("unsupported type");
		}
		arbb_type_t arbb_typ;
		ARBB_RUN(arbb_get_scalar_type(ctx,&arbb_typ,st,&details));
		if(isScalar) {
			return arbb_typ;
		} else {
			arbb_type_t vec_typ;
			ARBB_RUN(arbb_get_dense_type(ctx,&vec_typ,arbb_typ,1,&details));
			return vec_typ;
		}
	}
	void panic(const char * fname, int lineno, const char * line) {
		char err[1024];
		snprintf(err,1024,"%s:%d:%s: %s\n",fname,lineno,line,arbb_get_error_message(details));
		_error(err);
	}
};


void Trace::execute(State & state) {
	ArbbState arbb(state,*this);
	arbb.run();
}
#endif
