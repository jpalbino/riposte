
#include <sys/types.h>
#include <sys/stat.h>

#include "../../../src/runtime.h"
#include "../../../src/compiler.h"
#include "../../../src/parser.h"
#include "../../../src/library.h"
#include "../../../src/coerce.h"

extern "C"
Value parse(Thread& thread, Value const* args)
{
    Character const& c = (Character const&)args[0];
    Integer const& n = (Integer const&)args[1];
    Character const& name = (Character const&)args[2];
    // TODO: support n

    Value result;
    parse(thread.state, name[0], c[0], strlen(c[0]), true, result);
    return result;
}
