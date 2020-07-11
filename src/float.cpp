#include "natalie.hpp"
#include "natalie/builtin.hpp"
#include <math.h>

namespace Natalie {

Value *Float_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    FloatValue *self = self_value->as_float();

    if (self->is_nan()) {
        return new StringValue { env, "NaN" };
    } else if (self->is_positive_infinity()) {
        return new StringValue { env, "Infinity" };
    } else if (self->is_negative_infinity()) {
        return new StringValue { env, "-Infinity" };
    }

    char out[100]; // probably overkill
    snprintf(out, 100, "%.15f", self->to_double());
    int len = strlen(out);
    while (len > 1 && out[len - 1] == '0' && out[len - 2] != '.') {
        out[--len] = '\0';
    }

    return new StringValue { env, out };
}

Value *Float_eql(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);
    Value *other = args[0];
    if (other->is_float() && other->as_float()->to_double() == self->to_double()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Float_cmp(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);

    Value *lhs = self;
    Value *rhs = args[0];

    if (!rhs->is_float()) {
        auto coerced = coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "<=>", 1, &rhs);
    if (!rhs->is_float()) return NAT_NIL;

    if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {
        return NAT_NIL;
    }

    double lhs_d = lhs->as_float()->to_double();
    double rhs_d = rhs->as_float()->to_double();

    if (lhs_d < rhs_d) {
        return new IntegerValue { env, -1 };
    } else if (lhs_d == rhs_d) {
        return new IntegerValue { env, 0 };
    } else {
        return new IntegerValue { env, 1 };
    }
}

Value *Float_coerce(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);
    ArrayValue *ary = new ArrayValue { env };
    Value *arg = args[0];
    switch (NAT_TYPE(arg)) {
    case Value::Type::Float:
        ary->push(arg);
        ary->push(self);
        break;
    case Value::Type::Integer:
        ary->push(new FloatValue { env, arg->as_integer()->to_int64_t() });
        ary->push(self);
        break;
    case Value::Type::String:
        printf("TODO\n");
        abort();
        break;
    default:
        NAT_RAISE(env, "ArgumentError", "invalid value for Float(): %s", NAT_INSPECT(arg));
    }
    return ary;
}

Value *Float_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new IntegerValue { env, static_cast<int64_t>(floor(self->as_float()->to_double())) };
}

Value *Float_nan(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    if (self->as_float()->is_nan()) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *Float_add(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);

    Value *lhs = self_value;
    Value *rhs = args[0];

    if (!rhs->is_float()) {
        auto coerced = coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "+", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double addend1 = self->to_double();
    double addend2 = rhs->as_float()->to_double();
    return new FloatValue { env, addend1 + addend2 };
}

Value *Float_sub(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);

    Value *lhs = self_value;
    Value *rhs = args[0];

    if (!rhs->is_float()) {
        auto coerced = coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "-", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double minuend = self->to_double();
    double subtrahend = rhs->as_float()->to_double();
    return new FloatValue { env, minuend - subtrahend };
}

Value *Float_mul(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);

    Value *lhs = self_value;
    Value *rhs = args[0];

    if (!rhs->is_float()) {
        auto coerced = coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "*", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double multiplicand = self->to_double();
    double mulitiplier = rhs->as_float()->to_double();
    return new FloatValue { env, multiplicand * mulitiplier };
}

Value *Float_div(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    NAT_ASSERT_ARGC(1);

    Value *lhs = self_value;
    Value *rhs = args[0];

    if (!rhs->is_float()) {
        auto coerced = coerce(env, rhs, lhs);
        lhs = coerced.first;
        rhs = coerced.second;
    }

    if (!lhs->is_float()) return lhs->send(env, "/", 1, &rhs);
    if (!rhs->is_float()) NAT_ASSERT_TYPE(rhs, Value::Type::Float, "Float");

    double dividend = self->to_double();
    double divisor = rhs->as_float()->to_double();
    if (divisor == 0.0) {
        return FloatValue::nan(env);
    }
    return new FloatValue { env, dividend / divisor };
}

Value *Float_abs(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    FloatValue *self = self_value->as_float();
    auto number = self->to_double();
    if (number < 0.0) {
        return new FloatValue { env, -1 * number };
    } else {
        return self;
    }
}

#define NAT_DEFINE_FLOAT_COMPARISON_METHOD(name, op)                                                             \
    Value *name(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {                               \
        NAT_ASSERT_ARGC(1);                                                                                      \
                                                                                                                 \
        Value *lhs = self;                                                                                       \
        Value *rhs = args[0];                                                                                    \
                                                                                                                 \
        if (!rhs->is_float()) {                                                                                  \
            auto coerced = coerce(env, rhs, lhs);                                                                \
            lhs = coerced.first;                                                                                 \
            rhs = coerced.second;                                                                                \
        }                                                                                                        \
                                                                                                                 \
        if (!lhs->is_float()) return lhs->send(env, NAT_QUOTE(op), 1, &rhs);                                     \
        if (!rhs->is_float()) {                                                                                  \
            NAT_RAISE(env, "ArgumentError", "comparison of Float with %s failed", args[0]->klass->class_name()); \
        }                                                                                                        \
                                                                                                                 \
        if (lhs->as_float()->is_nan() || rhs->as_float()->is_nan()) {                                            \
            return NAT_NIL;                                                                                      \
        }                                                                                                        \
                                                                                                                 \
        double lhs_d = lhs->as_float()->to_double();                                                             \
        double rhs_d = rhs->as_float()->to_double();                                                             \
                                                                                                                 \
        if (lhs_d op rhs_d) {                                                                                    \
            return NAT_TRUE;                                                                                     \
        } else {                                                                                                 \
            return NAT_FALSE;                                                                                    \
        }                                                                                                        \
    }

NAT_DEFINE_FLOAT_COMPARISON_METHOD(Float_lt, <)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(Float_lte, <=)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(Float_gt, >)
NAT_DEFINE_FLOAT_COMPARISON_METHOD(Float_gte, >=)

}