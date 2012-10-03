
#ifndef JIT_H
#define JIT_H

#include <map>
#include <vector>
#include <set>
#include <tr1/unordered_map>
#include <assert.h>

#include "enum.h"
#include "bc.h"
#include "type.h"
#include "value.h"

#define TRACE_ENUM(_) \
		MAP_BYTECODES(_) \
		FOLD_BYTECODES(_) \
		SCAN_BYTECODES(_) \
		GENERATOR_BYTECODES(_) \
        _(loop, "LOOP", ___) \
        _(jmp, "JMP", ___) \
        _(exit, "exit", ___) \
        _(nest, "nest", ___) \
        _(constant, "const", ___) \
		_(sload, "sload", ___) /* loads are type guarded */ \
        _(curenv, "curenv", ___) \
        _(newenv, "newenv", ___) \
		_(load, "load", ___) /* loads are type guarded */ \
        _(length, "length", ___) \
        _(encode, "encode", ___) \
        _(decodevl, "decodevl", ___) \
        _(decodena, "decodena", ___) \
        _(store, "store", ___) \
        _(sstore, "sstore", ___) \
        _(box, "box", ___) \
        _(unbox, "unbox", ___) \
        _(gproto,   "gproto", ___) \
		_(gtrue, "gtrue", ___) \
		_(gfalse, "gfalse", ___) \
		_(gather, "gather", ___) \
		_(scatter, "scatter", ___) \
		_(phi, "phi", ___) \
        _(push, "PUSH", ___) \
        _(pop, "POP", ___) \
        _(strip, "STRIP", ___) \
        _(olength, "olength", ___) \
        _(alength, "alength", ___) \
        _(kill, "kill", ___) \
        _(lenv, "lenv", ___) \
        _(denv, "denv", ___) \
        _(cenv, "cenv", ___) \
        _(brcast, "brcast", ___) \
        _(reshape, "reshape", ___) \
        _(loadna, "loadna", ___) \
		_(nop, "NOP", ___)

DECLARE_ENUM(TraceOpCode,TRACE_ENUM)

class Thread;

#define SPECIALIZE_LENGTH 16

class JIT {

public:

    enum Arity {
        NULLARY,
        UNARY,
        BINARY,
        TERNARY
    };

    enum Group {
        NOP,
        GENERATOR,
        MAP,
        FILTER,
        FOLD,
        SPLIT
    };

    enum State {
        OFF,
        RECORDING
    };

	typedef size_t (*Ptr)(Thread& thread);

	typedef uint64_t IRRef;

    struct Variable {
        IRRef env;
        int64_t i;

        bool operator<(Variable const& o) const {
            return env < o.env || (env == o.env && i < o.i);
        }
    };

    struct Shape {
        IRRef length;
        bool constant;
        size_t traceLength;

        bool operator==(Shape const& o) const {
            return length == o.length;
        }
        bool operator!=(Shape const& o) const {
            return length != o.length;
        }
        bool operator<(Shape const& o) const {
            return length < o.length;
        }
        static const Shape Empty;
        static const Shape Scalar;
        Shape(IRRef length, bool constant, size_t traceLength) 
            : length(length)
            , constant(constant)
            , traceLength(traceLength) {}
    };

    std::map<size_t, Shape> shapes;

    struct Reenter {
        Instruction const* reenter;
        bool inScope;
        Reenter() : reenter (0), inScope(false) {}
        Reenter( Instruction const* reenter, bool inScope )
            : reenter( reenter ), inScope( inScope ) {}
    };

	struct IR {
		TraceOpCode::Enum op;
		IRRef a, b, c;

        Type::Enum type;
        Shape in, out;

        double cost;
        short reg;
        bool live;

        bool sunk;

        Reenter reenter;

        void dump() const;

        bool operator==(IR const& o) const {
            return  op == o.op &&
                    a == o.a &&
                    b == o.b &&
                    c == o.c &&
                    type == o.type &&
                    in == o.in &&
                    out == o.out;
        }

        IR()
            : op(TraceOpCode::nop), a(0), b(0), c(0), type(Type::Nil), in(Shape::Empty), out(Shape::Empty), sunk(false) {}
        
        IR(TraceOpCode::Enum op, Type::Enum type, Shape in, Shape out)
            : op(op), a(0), b(0), c(0), type(type), in(in), out(out), sunk(false) {}
        
        IR(TraceOpCode::Enum op, IRRef a, Type::Enum type, Shape in, Shape out)
            : op(op), a(a), b(0), c(0), type(type), in(in), out(out), sunk(false) {}
        
        IR(TraceOpCode::Enum op, IRRef a, IRRef b, Type::Enum type, Shape in, Shape out)
            : op(op), a(a), b(b), c(0), type(type), in(in), out(out), sunk(false) {}
        
        IR(TraceOpCode::Enum op, IRRef a, IRRef b, IRRef c, Type::Enum type, Shape in, Shape out)
            : op(op), a(a), b(b), c(c), type(type), in(in), out(out), sunk(false) {}
	};

    struct Phi {
        IRRef a, b;
    };

	unsigned short counters[1024];

	static const unsigned short RECORD_TRIGGER = 4;

    State state;
	Instruction const* startPC;

    std::vector<Value> constants;
    std::map<Value, size_t> constantsMap;
    std::map<size_t, IRRef> uniqueConstants;

    struct StackFrame {
        IRRef environment;
        Prototype const* prototype;
        Instruction const* returnpc;
        Value* returnbase;
        IRRef env;
        int64_t dest;
    };

    std::vector<IR> trace;
    std::vector<IR> code;
    std::vector<bool> fusable;
    std::vector<StackFrame> frames;


    struct Register {
        Type::Enum type;
        Shape shape;

        bool operator<(Register const& o) const {
            return type < o.type ||
                   (type == o.type && shape < o.shape); 
        }
    };

    std::vector<Register> registers;
    std::multimap<Register, size_t> freeRegisters;
    IRRef Loop;
    IRRef TopLevelEnvironment;
    
    struct Trace;
    Trace* rootTrace;
    Trace* dest;

    struct Snapshot {
        std::vector<StackFrame> stack;
        std::map< int64_t, IRRef > slotValues;
        std::map< int64_t, IRRef > slots;
        std::set<IRRef> memory;
    }; 

	struct Exit {
        Snapshot snapshot;
		Reenter reenter;
        size_t index;
	};
	std::map<size_t, Exit> exits;
    Exit BuildExit(Snapshot const& snapshot, Reenter const& reenter, size_t index); 

    static const IRRef FalseRef;

	JIT() 
		: state(OFF)
	{
	}

	void start_recording(Instruction const* startPC, Environment const* env, Trace* root, Trace* dest) {
		assert(state == OFF);
		state = RECORDING;
		this->startPC = startPC;
        trace.clear();
        envs.clear();
        constants.clear();
        constantsMap.clear();
        uniqueConstants.clear();
        exits.clear();
        shapes.clear();
        rootTrace = root;
        this->dest = dest;

        // insert empty and scalar shapes.
        Emit( makeConstant(Integer::c(0)) );
        Emit( makeConstant(Integer::c(1)) );
        Emit( makeConstant(Logical::c(Logical::FalseElement)) );

        shapes.insert( std::make_pair( 0, Shape::Empty ) );
        shapes.insert( std::make_pair( 1, Shape::Scalar ) );

        TopLevelEnvironment = Emit( IR( TraceOpCode::curenv, Type::Environment, Shape::Empty, Shape::Scalar) );
	    envs[env] = TopLevelEnvironment;
    }

    bool record(Thread& thread, Instruction const* pc, bool branch=false) {
        return EmitIR(thread, *pc, branch);
    }

	bool loop(Thread& thread, Instruction const* pc) {
		if(pc == startPC) {
            EmitIR(thread, *pc, false);
            return true;
        }
        else {
            EmitIR(thread, *pc, false);
            return false;
        }
	}

    struct Trace : public gc
    {
        size_t traceIndex;
        static size_t traceCount;
        Trace* root;
        Ptr ptr;
        void* function;
        std::vector<Trace, traceable_allocator<Trace> > exits;
        
        bool InScope;
        Instruction const* Reenter;
        size_t counter;
    };

	void end_recording(Thread& thread);
	
	void fail_recording() {
		assert(state != OFF);
		state = OFF;
	}

    IRRef Emit(IR const& ir);
    IRRef Emit(IR const& ir, Instruction const* reenter, bool inScope);
    
    IR makeConstant(Value const& v);

    struct Var {
        std::vector<IR>& trace;
        IRRef v;
        IRRef na;
        Type::Enum type;
        Shape s;
        Var(std::vector<IR>& trace, IRRef v, IRRef na) 
            : trace(trace), v(v), na(na), type(trace[v].type), s(trace[v].out) {}
        
        Var(Var const& o)
            : trace(o.trace), v(o.v), na(o.na), type(o.type), s(o.s) {}
    
        void operator=(Var const& o) {
            v = o.v;
            na = o.na;
            type = o.type;
            s = o.s;
        }
    };
    
	Var load(Thread& thread, int64_t a, Instruction const* reenter);
    void store(Thread& thread, Var a, int64_t c);
    Var EmitConstant(Value const& v);
	Var EmitUnary(TraceOpCode::Enum op, Var a, Type::Enum rty);
	Var EmitFold(TraceOpCode::Enum op, Var a, Type::Enum rty);
	Var EmitBinary(TraceOpCode::Enum op, Var a, Var b, Type::Enum rty, Instruction const* inst);
	Var EmitTernary(TraceOpCode::Enum op, Var a, Var b, Var c, Type::Enum rty, Instruction const* inst);
	Var EmitRep(Var l, Var e, Shape target);
	Var EmitBroadcast(Var a, Shape target);
	Var EmitCast(Var a, Type::Enum type);

    Shape SpecializeLength(size_t length, IRRef irlength);
    Shape SpecializeValue(Value const& v, IRRef r);
    IRRef SpecializeNA(Value const& v, IRRef r);
    Shape MergeShapes(Shape a, Shape b, Instruction const* inst);


    void emitCall(Var a, Function const& func, Environment* env, Value const& call, Instruction const* inst) {
        Emit( IR( TraceOpCode::gproto, a.v, (IRRef)func.prototype(), Type::Function, a.s, Shape::Empty ), inst, true );
        IRRef lenv = Emit( IR( TraceOpCode::load, a.v, Type::Environment, a.s, Shape::Scalar ) );
        IRRef denv = Emit( IR( TraceOpCode::curenv, Type::Environment, Shape::Empty, Shape::Scalar ) );
        IRRef ne = Emit( IR( TraceOpCode::newenv, lenv, denv, EmitConstant(call).v, Type::Environment, Shape::Scalar, Shape::Scalar ) );
        envs[env] = ne;
    }

    void emitPush(Thread const& thread);

    void storeArg(Environment* env, String name, Value const& v) {
        estore(EmitConstant(v), env, name);
    }

    std::map<Environment const*, IRRef> envs;

    IRRef getEnv(Environment* env) {
        std::map<Environment const*, IRRef>::const_iterator i;
        i = envs.find(env);
        if(i != envs.end())
            return i->second;
        else {
            Value v;
            return EmitConstant(REnvironment::Init(v, env)).v;
            //_error("Looking up nonexistant environment");
        }
    }

    void estore(Var a, Environment* env, String name) {
        Variable v = { getEnv(env), (int64_t)EmitConstant(Character::c(name)).v };

        IRRef r = Emit( IR( TraceOpCode::encode, a.v, a.na, a.type, a.s, a.s ) );
              r = Emit( IR( TraceOpCode::box, r, Type::Any, a.s, Shape::Scalar ) );	

        trace.push_back(IR(TraceOpCode::store, v.env, v.i, r, Type::Nil, a.s, Shape::Empty));
    }

	void dump(Thread& thread, std::vector<IR> const&);

	void compile(Thread& thread);

	void specialize();
	void schedule();
	void Schedule();

    bool EmitIR(Thread& thread, Instruction const& inst, bool branch);
    bool EmitNest(Thread& thread, Trace* trace);

    IRRef EmitOptIR(Thread& thread, IR ir, std::vector<IR>& code, std::vector<IRRef>& forward, std::tr1::unordered_map<IR, IRRef>& cse, Snapshot& snapshot);
    void Replay(Thread& thread);

    void AssignRegister(size_t index);
    void PreferRegister(size_t index, size_t share);
    void ReleaseRegister(size_t index);
    void AssignSnapshot(Snapshot const& snapshot);
    void RegisterAssignment();
    void RegisterAssign(IRRef i, IR ir);

    IRRef Insert(Thread& thread, std::vector<IR>& code, std::tr1::unordered_map<IR, IRRef>& cse, Snapshot& snapshot, IR ir);
    IR ConstantFold(Thread& thread, IR ir);
    IR StrengthReduce(IR ir);
    IR Normalize(IR ir);
    enum Aliasing {
        NO_ALIAS,
        MAY_ALIAS,
        MUST_ALIAS
    };
    Aliasing Alias(std::vector<IR> const& code, IRRef i, IRRef j);
    IRRef FWD(std::vector<IR> const& code, IRRef i, bool& loopCarried);
    IRRef DSE(std::vector<IR> const& code, IRRef i, bool& crossedExit);
    IRRef DPE(std::vector<IR> const& code, IRRef i);
    bool Ready(IR ir, std::vector<bool>& done);
    void sink(std::vector<bool>& marks, IRRef i);
    void SINK(void); 
    double Opcost(std::vector<IR>& code, IR ir);
    void Liveness();
    bool AlwaysLive(IR const& ir);
    void MarkSnapshot(Snapshot const& snapshot);
    void MarkLiveness(IRRef i, IR ir);
 
	template< template<class X> class Group >
	Var EmitUnary(TraceOpCode::Enum op, Var a) {
        #define EMIT(TA)                  \
        if(a.type == Type::TA)                     \
            return EmitUnary(op,                        \
                EmitCast(a, Group<TA>::MA::VectorType), \
                Group<TA>::R::VectorType); 
        UNARY_TYPES(EMIT)
        #undef EMIT
        _error("Unknown type in EmitUnary");
    }

	template< template<class X> class Group >
	Var EmitFold(TraceOpCode::Enum op, Var a) {
        #define EMIT(TA)                  \
        if(a.type == Type::TA)                     \
            return EmitFold(op,                         \
                EmitCast(a, Group<TA>::MA::VectorType), \
                Group<TA>::R::VectorType); 
        UNARY_TYPES(EMIT)
        #undef EMIT
        _error("Unknown type in EmitFold");
    }

	template< template<class X, class Y> class Group >
	Var EmitBinary(TraceOpCode::Enum op, Var a, Var b, Instruction const* inst) {
        #define EMIT(TA, TB)                 \
        if(a.type == Type::TA && b.type == Type::TB)              \
            return EmitBinary(op,                           \
                EmitCast(a, Group<TA,TB>::MA::VectorType),  \
                EmitCast(b, Group<TA,TB>::MB::VectorType),  \
                Group<TA,TB>::R::VectorType,                \
                inst);
        BINARY_TYPES(EMIT)
        #undef EMIT
        _error("Unknown type pair in EmitBinary");
    }
    
    template< template<class X, class Y, class Z> class Group >
    Var EmitTernary(TraceOpCode::Enum op, Var a, Var b, Var c, Instruction const* inst) {
        #define EMIT(TA, TB, TC)                            \
        if(a.type == Type::TA && b.type == Type::TB && c.type == Type::TC)   \
            return EmitTernary(op,                                  \
                EmitCast(a, Group<TA,TB,TC>::MA::VectorType),       \
                EmitCast(b, Group<TA,TB,TC>::MB::VectorType),       \
                EmitCast(c, Group<TA,TB,TC>::MC::VectorType),       \
                Group<TA,TB,TC>::R::VectorType,                     \
                inst);
        TERNARY_TYPES(EMIT)
        #undef EMIT
        _error("Unknown type pair in EmitTernary");
    }
#undef TYPES_TMP

};

namespace std {
    namespace tr1 {
    template<>
        struct hash<JIT::Shape> {
            size_t operator()(JIT::Shape const& key) const {
                return key.length;
            }
        };
    template<>
        struct hash<JIT::IR> {
            hash<JIT::Shape> sh;
            size_t operator()(JIT::IR const& key) const {
                return key.op ^ key.a ^ key.b ^ key.c ^ key.type ^ sh(key.in) ^ sh(key.out);
            }
        };
    }
}

#endif