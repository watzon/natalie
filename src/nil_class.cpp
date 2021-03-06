#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *NilClass_to_s(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new StringValue { env, "" };
}

Value *NilClass_to_a(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new ArrayValue { env };
}

Value *NilClass_to_i(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new IntegerValue { env, 0 };
}

Value *NilClass_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return new StringValue { env, "nil" };
}

Value *NilClass_is_nil(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return NAT_TRUE;
}

}
