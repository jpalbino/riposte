
#include "internal.h"
#include "compiler.h"
#include "parser.h"
#include "library.h"
#include <math.h>
#include <fstream>

const MaxOp<TComplex>::A MaxOp<TComplex>::Base = std::complex<double>(0,0);
const MinOp<TComplex>::A MinOp<TComplex>::Base = std::complex<double>(0,0);

void checkNumArgs(List const& args, uint64_t nargs) {
	if(args.length > nargs) _error("unused argument(s)");
	else if(args.length < nargs) _error("too few arguments");
}

uint64_t library(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);

	Character from = As<Character>(state, force(state, args[0]));
	if(from.length > 0) {
		Environment* global = state.global;
		state.global = new Environment(state.path.back(), state.path.back());
		loadLibrary(state, from[0].toString(state));
		state.path.push_back(state.global);
		global->init(state.global, state.global);
		state.global = global;
	}
	state.registers[0] = Null::singleton;
	return 1;
}

uint64_t function(State& state, Call const& call, List const& args) {
	Value parameters = force(state, args[0]);
	Value body = args[1];
	state.registers[0] = 	
		Function(parameters, body, Character::NA()/*force(state, args[2])*/, state.global);
	return 1;
}

uint64_t rm(State& state, Call const& call, List const& args) {
	for(uint64_t i = 0; i < args.length; i++) 
		if(expression(args[i]).type != Type::R_symbol && expression(args[i]).type != Type::R_character) 
			_error("rm() arguments must be symbols or character vectors");
	for(uint64_t i = 0; i < args.length; i++) {
		state.global->rm(expression(args[i]));
	}
	state.registers[0] = Null::singleton;
	return 1;
}

uint64_t sequence(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 3);

	Value from = force(state, args[0]);
	Value by   = force(state, args[1]);
	Value len  = force(state, args[2]);

	double f = asReal1(from);
	double b = asReal1(by);
	double l = asReal1(len);

	state.registers[0] = Sequence(f, b, l);	
	return 1;
}

uint64_t repeat(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 3);
	Value from = force(state, args[0]);
	assert(args.length == 3);
	
	Value vec  = force(state, args[0]);
	Value each = force(state, args[1]);
	Value len  = force(state, args[2]);
	
	double v = asReal1(vec);
	//double e = asReal1(each);
	double l = asReal1(len);
	
	Double r(l);
	for(uint64_t i = 0; i < l; i++) {
		r[i] = v;
	}
	state.registers[0] = r;
	return 1;
}

uint64_t typeOf(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Character c(1);
	c[0] = Symbol(state, force(state, args[0]).type.toString());
	state.registers[0] = c;
	return 1;
}

uint64_t mode(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Character c(1);
	Value v = force(state, args[0]);
	if(v.type == Type::R_integer || v.type == Type::R_double)
		c[0] = Symbol(state, "numeric");
	else if(v.type == Type::R_symbol)
		c[0] = Symbol(state, "name");
	else
		c[0] = Symbol(state, v.type.toString());
	state.registers[0] = c;
	return 1;
}

uint64_t klass(State& state, Call const& call, List const& args)
{
	checkNumArgs(args, 1);
	Value v = force(state, args[0]);
	Vector r = getClass(v.attributes);	
	if(r.type == Type::R_null) {
		Character c(1);
		c[0] = Symbol(state, (v).type.toString());
		state.registers[0] = c;
	}
	else {
		state.registers[0] = r;
	}
	return 1;
}

uint64_t assignKlass(State& state, Call const& call, List const& args)
{
	checkNumArgs(args, 2);
	Value v = force(state, args[0]);
	Value k = force(state, args[1]);
	setClass(v.attributes, k);
	state.registers[0] = v;
	return 1;
}

uint64_t names(State& state, Call const& call, List const& args)
{
	checkNumArgs(args, 1);
	Value v = force(state, args[0]);
	Value r = getNames(v.attributes);
	state.registers[0] = r;	
	return 1;
}

uint64_t assignNames(State& state, Call const& call, List const& args)
{
	checkNumArgs(args, 2);
	Value v = force(state, args[0]);
	Value k = force(state, args[1]);
	setNames(v.attributes, As<Character>(state,k));
	state.registers[0] = v;
	return 1;
}

uint64_t dim(State& state, Call const& call, List const& args)
{
	checkNumArgs(args, 1);
	Value v = force(state, args[0]);
	Value r = getDim(v.attributes);
	state.registers[0]= r;	
	return 1;
}

uint64_t assignDim(State& state, Call const& call, List const& args)
{
	checkNumArgs(args, 2);
	Value v = force(state, args[0]);
	Value k = force(state, args[1]);
	setDim(v.attributes, k);
	state.registers[0] = v;
	return 1;
}

Type cTypeCast(Value const& v, Type t)
{
	Type r;
	r.v = std::max(v.type.Enum(), t.Enum());
	return r;
}

uint64_t c(State& state, Call const& call, List const& Args) {
	uint64_t total = 0;
	Type type = Type::R_null;
	std::vector<Vector> args;
	
	for(uint64_t i = 0; i < Args.length; i++) {
		args.push_back(Vector(force(state, Args[i])));
		total += args[i].length;
		type = cTypeCast(args[i], type);
	}
	Vector out(type, total);
	uint64_t j = 0;
	for(uint64_t i = 0; i < args.size(); i++) {
		Insert(state, args[i], 0, out, j, args[i].length);
		j += args[i].length;
	}
	
	Vector n = getNames(Args.attributes);
	if(n.type != Type::R_null)
	{
		Character names(n);
		Character outnames(total);
		uint64_t j = 0;
		for(uint64_t i = 0; i < args.size(); i++) {
			for(uint64_t m = 0; m < args[i].length; m++, j++) {
				// NYI: R makes these names distinct
				outnames[j] = names[i];
			}
		}
		setNames(out.attributes, outnames);
	}
	state.registers[0] = out;
	return 1;
}

uint64_t list(State& state, Call const& call, List const& args) {
	List out(args.length);
	for(uint64_t i = 0; i < args.length; i++) out[i] = force(state, args[i]);
	Vector n = getNames(args.attributes);
	if(n.type != Type::R_null)
		setNames(out.attributes, n);
	state.registers[0] = out;
	return 1;
}

uint64_t flatten(State& state, Call const& call, List const& args) {
	List from = force(state, args[0]);
	uint64_t length = 0;
	for(uint64_t i = 0; i < from.length; i++) length += from[i].length;
	Vector out = List(length);
	uint64_t j = 0;
	for(uint64_t i = 0; i < from.length; i++) {
		Insert(state, from[i], 0, out, j, from[i].length);
		j += from[i].length;
	}
	state.registers[0] = out;
	return 1;
}

uint64_t vector(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 2);
	Value mode = force(state, args[0]);
	Value length = force(state, args[1]);
	double l = asReal1(length);
	Symbol m = Character(mode)[0];
	if(m == Symbol::Logical) state.registers[0] = Logical((uint64_t)l);
	else if(m == Symbol::Integer) state.registers[0] = Integer((uint64_t)l);
	else if(m == Symbol::Double || m == Symbol::Numeric) state.registers[0] =  Double((uint64_t)l);
	else if(m == Symbol::Complex) state.registers[0] =  Complex((uint64_t)l);
	else if(m == Symbol::Character) state.registers[0] =  Character((uint64_t)l);
	else if(m == Symbol::Raw) state.registers[0] =  Raw((uint64_t)l);
	else _error("cannot make a vector of mode '" + m.toString(state) + "'");
	return 1;
}

/*
uint64_t UseMethod(State& state, uint64_t nargs)
{
	return 0;
}

uint64_t plusOp(State& state, uint64_t nargs) {
	if(nargs == 1)
		return unaryArith<Zip1, PosOp>(state, nargs);
	else
		return binaryArith<Zip2, AddOp>(state, nargs);
}

uint64_t minusOp(State& state, uint64_t nargs) {
	if(nargs == 1)
		return unaryArith<Zip1, NegOp>(state, nargs);
	else
		return binaryArith<Zip2, SubOp>(state, nargs);
}
*/

Vector Subset(State& state, Vector const& a, Vector const& i)	{
	if(i.type == Type::R_double || i.type == Type::R_integer) {
		Integer index = As<Integer>(state, i);
		uint64_t positive = 0, negative = 0;
		for(uint64_t i = 0; i < index.length; i++) {
			if(index[i] > 0 || Integer::isNA(index[i])) positive++;
			else if(index[i] < 0) negative++;
		}
		if(positive > 0 && negative > 0)
			_error("mixed subscripts not allowed");
		else if(positive > 0) {
			switch(a.type.Enum()) {
				case Type::E_R_double: return SubsetInclude<Double>::eval(state, a, index, positive); break;
				case Type::E_R_integer: return SubsetInclude<Integer>::eval(state, a, index, positive); break;
				case Type::E_R_logical: return SubsetInclude<Logical>::eval(state, a, index, positive); break;
				case Type::E_R_character: return SubsetInclude<Character>::eval(state, a, index, positive); break;
				case Type::E_R_list: return SubsetInclude<List>::eval(state, a, index, positive); break;
				default: _error("NYI"); break;
			};
		}
		else if(negative > 0) {
			switch(a.type.Enum()) {
				case Type::E_R_double: return SubsetExclude<Double>::eval(state, a, index, negative); break;
				case Type::E_R_integer: return SubsetExclude<Integer>::eval(state, a, index, negative); break;
				case Type::E_R_logical: return SubsetExclude<Logical>::eval(state, a, index, negative); break;
				case Type::E_R_character: return SubsetExclude<Character>::eval(state, a, index, negative); break;
				case Type::E_R_list: return SubsetExclude<List>::eval(state, a, index, negative); break;
				default: _error("NYI"); break;
			};	
		}
		else {
			return Vector(a.type, 0);
		}
	}
	else if(i.type == Type::R_logical) {
		Logical index = Logical(i);
		switch(a.type.Enum()) {
			case Type::E_R_double: return SubsetLogical<Double>::eval(state, a, index); break;
			case Type::E_R_integer: return SubsetLogical<Integer>::eval(state, a, index); break;
			case Type::E_R_logical: return SubsetLogical<Logical>::eval(state, a, index); break;
			case Type::E_R_character: return SubsetLogical<Character>::eval(state, a, index); break;
			case Type::E_R_list: return SubsetLogical<List>::eval(state, a, index); break;
			default: _error("NYI"); break;
		};	
	}
	_error("NYI indexing type");
	return Null::singleton;
}

uint64_t subset(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 2);
        Vector a = Vector(force(state, args[0]));
        Vector i = Vector(force(state, args[1]));
	state.registers[0] = Subset(state, a,i);
        return 1;
}

uint64_t subset2(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 2);

        Value a = force(state, args[0]);
        Value b = force(state, args[1]);
	if(b.type == Type::R_character) {
		Symbol i = Character(b)[0];
		Value r = getNames(a.attributes);
		if(r.type != Type::R_null) {
			Character c(r);
			uint64_t j = 0;
			for(;j < c.length; j++) {
				if(c[j] == i)
					break;
			}
			if(j < c.length) {
				state.registers[0] = Element2(a, j);
				return 1;
			}
		}
	}
	else if(b.type == Type::R_integer) {
		state.registers[0] = Element2(a, Integer(b)[0]-1);
		return 1;
	}
	else if(b.type == Type::R_double) {
		state.registers[0] = Element2(a, (uint64_t)Double(b)[0]-1);
		return 1;
	}
	state.registers[0] = Null::singleton;
	return 1;
} 

uint64_t dollar(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 2);

        Value a = force(state, args[0]);
        uint64_t i = Symbol(expression(args[1])).i;
	Value r = getNames(a.attributes);
	if(r.type != Type::R_null) {
		Character c(r);
		uint64_t j = 0;
		for(;j < c.length; j++) {
			if(c[j] == i)
				break;
		}
		if(j < c.length) {
			state.registers[0] = Element2(a, j);
			return 1;
		}
	}
	state.registers[0] = Null::singleton;
	return 1;
} 

uint64_t length(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Vector a = force(state, args[0]);
	Integer i(1);
	i[0] = a.length;
	state.registers[0] = i;
	return 1;
}

/*uint64_t stop(State& state, Call const& call, List const& args) {
	state.stopped = true;
	return 0;
}
*/
uint64_t quote(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	state.registers[0] = expression(args[0]);
	return 1;
}

uint64_t eval_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 2);
	Value expr = force(state, args[0]);
	Value envir = force(state, args[1]);
	//Value enclos = force(state, call[3]);
	Closure closure = Compiler::compile(state, expr);
	closure.bind(REnvironment(envir).ptr());
	eval(state, closure);
	return 1;
}

uint64_t lapply(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 2);
	List x = As<List>(state, force(state, args[0]));
	Value func = force(state, args[1]);

	Call apply(2);
	apply[0] = func;

	List result(x.length);
	
	for(uint64_t i = 0; i < x.length; i++) {
		apply[1] = x[i];
		Closure closure = Compiler::compile(state, apply);
		eval(state, closure);
		result[i] = state.registers[0];
	}

	state.registers[0] = result;
	return 1;
}

uint64_t source(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value file = force(state, args[0]);
	std::ifstream t(Character(file)[0].toString(state).c_str());
	std::stringstream buffer;
	buffer << t.rdbuf();
	std::string code = buffer.str();

	Parser parser(state);
	Value value;
	parser.execute(code.c_str(), code.length(), true, value);	
	
	Closure closure = Compiler::compile(state, value);
	eval(state, closure);
	return 1;
}

uint64_t switch_fn(State& state, Call const& call, List const& args) {
	Value one = force(state, args[0]);
	if(one.type == Type::R_integer && Integer(one).length == 1) {
		int64_t i = Integer(one)[0];
		if(i >= 1 && (uint64_t)i <= args.length) {state.registers[0] = force(state, args[i]); return 1; }
	} else if(one.type == Type::R_double && Double(one).length == 1) {
		int64_t i = (int64_t)Double(one)[0];
		if(i >= 1 && (uint64_t)i <= args.length) {state.registers[0] = force(state, args[i]); return 1; }
	} else if(one.type == Type::R_character && Character(one).length == 1 && 
			getNames(args.attributes).type != Type::R_null) {
		Character names(getNames(args.attributes));
		for(uint64_t i = 1; i < args.length; i++) {
			if(names[i] == Character(one)[0]) {
				while(args[i].type == Type::I_nil && i < args.length) i++;
				state.registers[0] = i < args.length ? force(state, args[i]) : (Value)(Null::singleton);
				return 1;
			}
		}
		for(uint64_t i = 1; args.length; i++) {
			if(names[i] == Symbol::empty) {
				state.registers[0] = force(state, args[i]);
				return 1;
			}
		}
	}
	state.registers[0] = Null::singleton;
	return 1;
}

uint64_t environment(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value e = force(state, args[0]);
	if(e.type == Type::R_null) {
		state.registers[0] = REnvironment(state.global);
		return 1;
	}
	else if(e.type == Type::R_function) {
		state.registers[0] = REnvironment(Function(e).s());
		return 1;
	}
	state.registers[0] = Null::singleton;
	return 1;
}

uint64_t parentframe(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	uint64_t i = (uint64_t)asReal1(force(state, args[0]));
	Environment* e = state.global;
	for(uint64_t j = 0; j < i-1 && e != NULL; j++) {
		e = e->dynamicParent();
	}
	state.registers[0] = REnvironment(e);
	return 1;
}

uint64_t stop_fn(State& state, Call const& call, List const& args) {
	// this should stop whether or not the arguments are correct...
	std::string message = "user stop";
	if(args.length > 0) {
		if(args[0].type == Type::R_character && Character(args[0]).length > 0) {
			message = Character(args[0])[0].toString(state);
		}
	}
	_error(message);
	return 0;
}

uint64_t warning_fn(State& state, Call const& call, List const& args) {
	std::string message = "user warning";
	if(args.length > 0) {
		if(args[0].type == Type::R_character && Character(args[0]).length > 0) {
			message = Character(args[0])[0].toString(state);
		}
	}
	_warning(state, message);
	state.registers[0] = Character::c(state, message);
	return 1;
} 

uint64_t missing(State& state, Call const& call, List const& args) {
	Symbol s = expression(args[0]); 
	Value v;
	bool success = state.global->getRaw(s, v);
	state.registers[0] =  (!success || v.type == Type::I_default) ? Logical::True() : Logical::False();
	return 1;
}

uint64_t max_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryArith<FoldLeft, MaxOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t min_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryArith<FoldLeft, MinOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t sum_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryArith<FoldLeft, SumOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t prod_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryArith<FoldLeft, ProdOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t isna_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryFilter<Zip1, IsNAOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t isnan_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryFilter<Zip1, IsNaNOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t isfinite_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryFilter<Zip1, IsFiniteOp>(state, a, state.registers[0]);
	return 1;
}

uint64_t isinfinite_fn(State& state, Call const& call, List const& args) {
	checkNumArgs(args, 1);
	Value a = force(state, args[0]);
	unaryFilter<Zip1, IsInfiniteOp>(state, a, state.registers[0]);
	return 1;
}

void addMathOps(State& state)
{
	Value v;
	Environment* env = state.path[0];

	CFunction(max_fn).toValue(v);
	env->assign(Symbol(state, "max"), v);
	CFunction(min_fn).toValue(v);
	env->assign(Symbol(state, "min"), v);
	CFunction(sum_fn).toValue(v);
	env->assign(Symbol(state, "sum"), v);
	CFunction(prod_fn).toValue(v);
	env->assign(Symbol(state, "prod"), v);
	
	CFunction(isna_fn).toValue(v);
	env->assign(Symbol(state, "is.na"), v);
	CFunction(isnan_fn).toValue(v);
	env->assign(Symbol(state, "is.nan"), v);
	CFunction(isfinite_fn).toValue(v);
	env->assign(Symbol(state, "is.finite"), v);
	CFunction(isinfinite_fn).toValue(v);
	env->assign(Symbol(state, "is.infinite"), v);
	
	CFunction(library).toValue(v);
	env->assign(Symbol(state, "library"), v);
	CFunction(function).toValue(v);
	env->assign(Symbol(state, "function"), v);
	CFunction(rm).toValue(v);
	env->assign(Symbol(state, "rm"), v);
	CFunction(typeOf).toValue(v);
	env->assign(Symbol(state, "typeof"), v);
	env->assign(Symbol(state, "storage.mode"), v);
	CFunction(mode).toValue(v);
	env->assign(Symbol(state, "mode"), v);

	CFunction(sequence).toValue(v);
	env->assign(Symbol(state, "seq"), v);
	CFunction(repeat).toValue(v);
	env->assign(Symbol(state, "rep"), v);
	
	CFunction(klass).toValue(v);
	env->assign(Symbol(state, "class"), v);
	CFunction(assignKlass).toValue(v);
	env->assign(Symbol(state, "class<-"), v);
	CFunction(names).toValue(v);
	env->assign(Symbol(state, "names"), v);
	CFunction(assignNames).toValue(v);
	env->assign(Symbol(state, "names<-"), v);
	CFunction(dim).toValue(v);
	env->assign(Symbol(state, "dim"), v);
	CFunction(assignDim).toValue(v);
	env->assign(Symbol(state, "dim<-"), v);
	
	CFunction(c).toValue(v);
	env->assign(Symbol(state, "c"), v);
	CFunction(list).toValue(v);
	env->assign(Symbol(state, "list"), v);
	CFunction(flatten).toValue(v);
	env->assign(Symbol(state, "flatten"), v);

	CFunction(length).toValue(v);
	env->assign(Symbol(state, "length"), v);
	
	CFunction(vector).toValue(v);
	env->assign(Symbol(state, "vector"), v);
	
	CFunction(subset).toValue(v);
	env->assign(Symbol(state, "["), v);
	CFunction(subset2).toValue(v);
	env->assign(Symbol(state, "[["), v);
	CFunction(dollar).toValue(v);
	env->assign(Symbol(state, "$"), v);
/*
	CFunction(stop).toValue(v);
	env->assign(Symbol(state, "stop"), v);
*/
	
	CFunction(switch_fn).toValue(v);
	env->assign(Symbol(state, "switch"), v);
	
	CFunction(eval_fn).toValue(v);
	env->assign(Symbol(state, "eval"), v);
	CFunction(quote).toValue(v);
	env->assign(Symbol(state, "quote"), v);
	CFunction(lapply).toValue(v);
	env->assign(Symbol(state, "lapply"), v);
	CFunction(source).toValue(v);
	env->assign(Symbol(state, "source"), v);

	CFunction(environment).toValue(v);
	env->assign(Symbol(state, "environment"), v);
	CFunction(parentframe).toValue(v);
	env->assign(Symbol(state, "parent.frame"), v);
	
	CFunction(missing).toValue(v);
	env->assign(Symbol(state, "missing"), v);
	
	CFunction(stop_fn).toValue(v);
	env->assign(Symbol(state, "stop"), v);
	CFunction(warning_fn).toValue(v);
	env->assign(Symbol(state, "warning"), v);
}

