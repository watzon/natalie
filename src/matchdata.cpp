#include "builtin.hpp"
#include "natalie.hpp"

namespace Natalie {

Value *MatchData_size(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    MatchDataValue *self = self_value->as_match_data();
    assert(self->matchdata_region->num_regs > 0);
    return integer(env, self->matchdata_region->num_regs);
}

Value *MatchData_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    MatchDataValue *self = self_value->as_match_data();
    assert(self->matchdata_region->num_regs > 0);
    const char *str = &self->matchdata_str[self->matchdata_region->beg[0]];
    StringValue *str_obj = string(env, str);
    str_obj->str_len = self->matchdata_region->end[0] - self->matchdata_region->beg[0];
    str_obj->str[str_obj->str_len] = 0;
    return str_obj;
}

Value *MatchData_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    MatchDataValue *self = self_value->as_match_data();
    if (NAT_TYPE(args[0]) == Value::Type::String || NAT_TYPE(args[0]) == Value::Type::Symbol) {
        NAT_NOT_YET_IMPLEMENTED("group name support in Regexp MatchData#[]");
    }
    NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
    int64_t index = NAT_INT_VALUE(args[0]);
    assert(index >= 0); // TODO: accept negative indices
    if (index == 0) {
        return MatchData_to_s(env, self, 0, NULL, NULL);
    } else if (index >= self->matchdata_region->num_regs) {
        return NAT_NIL;
    } else {
        const char *str = &self->matchdata_str[self->matchdata_region->beg[index]];
        StringValue *str_obj = string(env, str);
        str_obj->str_len = self->matchdata_region->end[index] - self->matchdata_region->beg[index];
        str_obj->str[str_obj->str_len] = 0;
        return str_obj;
    }
}

}