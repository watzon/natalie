#pragma once

#include <assert.h>

#include "natalie/class_value.hpp"
#include "natalie/forward.hpp"
#include "natalie/global_env.hpp"
#include "natalie/macros.hpp"
#include "natalie/string_value.hpp"
#include "natalie/value.hpp"

namespace Natalie {

struct SymbolValue : Value {
    static SymbolValue *intern(Env *, const char *);

    const char *c_str() { return m_name; }

    StringValue *to_s(Env *env) { return new StringValue { env, m_name }; }
    StringValue *inspect(Env *);

private:
    SymbolValue(Env *env, const char *name)
        : Value { Value::Type::Symbol, NAT_OBJECT->const_get(env, "Symbol", true)->as_class() }
        , m_name { name } {
        assert(m_name);
    }

    const char *m_name { nullptr };
};

}
