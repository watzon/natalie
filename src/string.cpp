#include <ctype.h>
#include <stdlib.h>

#include "natalie.hpp"
#include "natalie/builtin.hpp"

namespace Natalie {

Value *String_new(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    Value *obj = new StringValue { env, self->as_class() };
    return obj->initialize(env, argc, args, block);
}

Value *String_initialize(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(0, 1);
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], Value::Type::String, "String");
        StringValue *arg = args[0]->as_string();
        self->set_str(arg->c_str());
    }
    return self;
}

Value *String_to_s(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(0);
    return self;
}

Value *String_ltlt(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_NOT_FROZEN(self);
    Value *arg = args[0];
    if (arg->is_string()) {
        self->append_string(env, arg->as_string());
    } else {
        Value *str_obj = arg->send(env, "to_s");
        NAT_ASSERT_TYPE(str_obj, Value::Type::String, "String");
        self->append_string(env, str_obj->as_string());
    }
    return self;
}

Value *String_inspect(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->as_string()->inspect(env);
}

Value *String_add(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    const char *str;
    if (args[0]->is_string()) {
        str = args[0]->as_string()->c_str();
    } else {
        StringValue *str_obj = args[0]->send(env, "to_s")->as_string();
        NAT_ASSERT_TYPE(str_obj, Value::Type::String, "String");
        str = str_obj->c_str();
    }
    StringValue *new_string = new StringValue { env, self->c_str() };
    new_string->append(env, str);
    return new_string;
}

Value *String_mul(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    NAT_ASSERT_TYPE(arg, Value::Type::Integer, "Integer");
    StringValue *new_string = new StringValue { env, "" };
    for (int64_t i = 0; i < arg->as_integer()->to_int64_t(); i++) {
        new_string->append_string(env, self);
    }
    return new_string;
}

Value *String_eqeq(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    if (*self->as_string() == *args[0]) {
        return NAT_TRUE;
    } else {
        return NAT_FALSE;
    }
}

Value *String_cmp(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    StringValue *self = self_value->as_string();
    NAT_ASSERT_ARGC(1);
    Value *arg = args[0];
    if (NAT_TYPE(arg) != Value::Type::String) return NAT_NIL;
    int diff = strcmp(self->c_str(), arg->as_string()->c_str());
    int result;
    if (diff < 0) {
        result = -1;
    } else if (diff > 0) {
        result = 1;
    } else {
        result = 0;
    }
    return new IntegerValue { env, result };
}

Value *String_eqtilde(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::Regexp, "Regexp");
    return Regexp_eqtilde(env, args[0], 1, &self_value, block);
}

Value *String_match(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    NAT_ASSERT_TYPE(args[0], Value::Type::Regexp, "Regexp");
    return Regexp_match(env, args[0], 1, &self_value, block);
}

Value *String_succ(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->as_string()->successive(env);
}

Value *String_ord(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    ArrayValue *chars = self->as_string()->chars(env);
    if (chars->size() == 0) {
        NAT_RAISE(env, "ArgumentError", "empty string");
    }
    StringValue *c = (*chars)[0]->as_string();
    assert(c->length() > 0);
    unsigned int code;
    const char *str = c->c_str();
    switch (c->length()) {
    case 0:
        NAT_UNREACHABLE();
    case 1:
        code = (unsigned char)str[0];
        break;
    case 2:
        code = (((unsigned char)str[0] ^ 0xC0) << 6) + (((unsigned char)str[1] ^ 0x80) << 0);
        break;
    case 3:
        code = (((unsigned char)str[0] ^ 0xE0) << 12) + (((unsigned char)str[1] ^ 0x80) << 6) + (((unsigned char)str[2] ^ 0x80) << 0);
        break;
    case 4:
        code = (((unsigned char)str[0] ^ 0xF0) << 18) + (((unsigned char)str[1] ^ 0x80) << 12) + (((unsigned char)str[2] ^ 0x80) << 6) + (((unsigned char)str[3] ^ 0x80) << 0);
        break;
    default:
        NAT_UNREACHABLE();
    }
    return new IntegerValue { env, code };
}

Value *String_bytes(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    StringValue *self = self_value->as_string();
    ArrayValue *ary = new ArrayValue { env };
    ssize_t length = self->length();
    const char *str = self->c_str();
    for (ssize_t i = 0; i < length; i++) {
        ary->push(new IntegerValue { env, str[i] });
    }
    return ary;
}

Value *String_chars(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    return self->as_string()->chars(env);
}

Value *String_size(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ArrayValue *chars = self->as_string()->chars(env);
    return new IntegerValue { env, chars->size() };
}

Value *String_encoding(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0);
    ClassValue *Encoding = NAT_OBJECT->const_get(env, "Encoding", true)->as_class();
    switch (self->as_string()->encoding()) {
    case Encoding::ASCII_8BIT:
        return Encoding->const_get(env, "ASCII_8BIT", true);
    case Encoding::UTF_8:
        return Encoding->const_get(env, "UTF_8", true);
    }
    NAT_UNREACHABLE();
}

static char *lcase_string(const char *str) {
    char *lcase_str = strdup(str);
    for (int i = 0; lcase_str[i]; i++) {
        lcase_str[i] = tolower(lcase_str[i]);
    }
    return lcase_str;
}

static EncodingValue *find_encoding_by_name(Env *env, const char *name) {
    char *lcase_name = lcase_string(name);
    ArrayValue *list = Encoding_list(env, NAT_OBJECT->const_get(env, "Encoding", true), 0, nullptr, nullptr)->as_array();
    for (ssize_t i = 0; i < list->size(); i++) {
        EncodingValue *encoding = (*list)[i]->as_encoding();
        ArrayValue *names = encoding->names(env);
        for (ssize_t n = 0; n < names->size(); n++) {
            StringValue *name_obj = (*names)[n]->as_string();
            char *name = lcase_string(name_obj->c_str());
            if (strcmp(name, lcase_name) == 0) {
                free(name);
                free(lcase_name);
                return encoding;
            }
            free(name);
        }
    }
    free(lcase_name);
    NAT_RAISE(env, "ArgumentError", "unknown encoding name - %s", name);
}

Value *String_encode(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Encoding orig_encoding = self->encoding();
    StringValue *copy = self->dup(env)->as_string();
    String_force_encoding(env, copy, argc, args, nullptr);
    ClassValue *Encoding = NAT_OBJECT->const_get(env, "Encoding", true)->as_class();
    if (orig_encoding == copy->encoding()) {
        return copy;
    } else if (orig_encoding == Encoding::UTF_8 && copy->encoding() == Encoding::ASCII_8BIT) {
        ArrayValue *chars = self->chars(env);
        for (ssize_t i = 0; i < chars->size(); i++) {
            StringValue *char_obj = (*chars)[i]->as_string();
            if (char_obj->length() > 1) {
                Value *ord = String_ord(env, char_obj, 0, nullptr, nullptr);
                Value *message = StringValue::sprintf(env, "U+%X from UTF-8 to ASCII-8BIT", ord->as_integer()->to_int64_t());
                StringValue zero_x { env, "0X" };
                StringValue blank { env, "" };
                Value *sub_args[2] = { &zero_x, &blank };
                message = String_sub(env, message, 2, sub_args, nullptr);
                env->raise(Encoding->const_get(env, "UndefinedConversionError", true)->as_class(), "%S", message);
                abort();
            }
        }
        return copy;
    } else if (orig_encoding == Encoding::ASCII_8BIT && copy->encoding() == Encoding::UTF_8) {
        return copy;
    } else {
        env->raise(Encoding->const_get(env, "ConverterNotFoundError", true)->as_class(), "code converter not found");
        abort();
    }
}

Value *String_force_encoding(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *encoding = args[0];
    switch (encoding->type) {
    case Value::Type::Encoding:
        self->set_encoding(encoding->as_encoding()->num());
        break;
    case Value::Type::String:
        self->set_encoding(find_encoding_by_name(env, encoding->as_string()->c_str())->num());
        break;
    default:
        NAT_RAISE(env, "TypeError", "no implicit conversion of %s into String", NAT_OBJ_CLASS(encoding)->class_name());
    }
    return self;
}

Value *String_ref(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *index_obj = args[0];

    // not sure how we'd handle a string that big anyway
    assert(self->length() < INT64_MAX);

    if (index_obj->is_integer()) {
        int64_t index = index_obj->as_integer()->to_int64_t();

        ArrayValue *chars = self->chars(env);
        if (index < 0) {
            index = chars->size() + index;
        }

        if (index < 0 || index >= (int64_t)chars->size()) {
            return NAT_NIL;
        }
        return (*chars)[index];
    } else if (index_obj->is_range()) {
        RangeValue *range = index_obj->as_range();

        NAT_ASSERT_TYPE(range->begin(), Value::Type::Integer, "Integer");
        NAT_ASSERT_TYPE(range->end(), Value::Type::Integer, "Integer");

        int64_t begin = range->begin()->as_integer()->to_int64_t();
        int64_t end = range->end()->as_integer()->to_int64_t();

        ArrayValue *chars = self->chars(env);

        if (begin < 0) begin = chars->size() + begin;
        if (end < 0) end = chars->size() + end;

        if (begin < 0 || end < 0) return NAT_NIL;
        if (begin >= chars->size()) return NAT_NIL;

        if (!range->exclude_end()) end++;

        ssize_t max = chars->size();
        end = end > max ? max : end;
        StringValue *result = new StringValue { env };
        for (int64_t i = begin; i < end; i++) {
            result->append_string(env, (*chars)[i]);
        }

        return result;
    }
    NAT_ASSERT_TYPE(index_obj, Value::Type::Integer, "Integer");
    abort();
}

Value *String_index(Env *env, Value *self, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    Value *find = args[0];
    NAT_ASSERT_TYPE(find, Value::Type::String, "String");
    ssize_t index = self->as_string()->index(env, find->as_string());
    if (index == -1) {
        return NAT_NIL;
    }
    return new IntegerValue { env, index };
}

Value *String_sub(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(2);
    StringValue *self = self_value->as_string();
    Value *sub = args[0];
    Value *repl = args[1];
    NAT_ASSERT_TYPE(repl, Value::Type::String, "String");
    if (sub->is_string()) {
        ssize_t index = self->index(env, sub->as_string());
        if (index == -1) {
            return self->dup(env);
        }
        StringValue *out = new StringValue { env, self->c_str(), index };
        out->append_string(env, repl->as_string());
        out->append(env, &self->c_str()[index + sub->as_string()->length()]);
        return out;
    } else if (sub->is_regexp()) {
        Value *match = Regexp_match(env, sub, 1, &self_value, nullptr);
        if (match == NAT_NIL) {
            return self->dup(env);
        }
        size_t length = match->as_match_data()->group(env, 0)->as_string()->length();
        int64_t index = match->as_match_data()->index(0);
        StringValue *out = new StringValue { env, self->c_str(), index };
        out->append_string(env, repl->as_string());
        out->append(env, &self->c_str()[index + length]);
        return out;
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Regexp)", NAT_OBJ_CLASS(sub)->class_name());
    }
}

Value *String_to_i(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(0, 1);
    StringValue *self = self_value->as_string();
    int base = 10;
    if (argc == 1) {
        NAT_ASSERT_TYPE(args[0], Value::Type::Integer, "Integer");
        base = args[0]->as_integer()->to_int64_t();
    }
    int64_t number = strtoll(self->c_str(), nullptr, base);
    return new IntegerValue { env, number };
}

Value *String_split(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1);
    StringValue *self = self_value->as_string();
    Value *splitter = args[0];
    ArrayValue *ary = new ArrayValue { env };
    if (self->length() == 0) {
        return ary;
    } else if (splitter->is_regexp()) {
        ssize_t last_index = 0;
        ssize_t index, len;
        OnigRegion *region = onig_region_new();
        int result = splitter->as_regexp()->search(self->c_str(), region, ONIG_OPTION_NONE);
        if (result == ONIG_MISMATCH) {
            ary->push(self->dup(env));
        } else {
            do {
                index = region->beg[0];
                len = region->end[0] - region->beg[0];
                ary->push(new StringValue { env, &self->c_str()[last_index], index - last_index });
                last_index = index + len;
                result = splitter->as_regexp()->search(self->c_str(), last_index, region, ONIG_OPTION_NONE);
            } while (result != ONIG_MISMATCH);
            ary->push(new StringValue { env, &self->c_str()[last_index] });
        }
        onig_region_free(region, true);
        return ary;
    } else if (splitter->is_string()) {
        ssize_t last_index = 0;
        ssize_t index = self->index(env, splitter->as_string(), 0);
        if (index == -1) {
            ary->push(self->dup(env));
        } else {
            do {
                ary->push(new StringValue { env, &self->c_str()[last_index], index - last_index });
                last_index = index + splitter->as_string()->length();
                index = self->index(env, splitter->as_string(), last_index);
            } while (index != -1);
            ary->push(new StringValue { env, &self->c_str()[last_index] });
        }
        return ary;
    } else {
        NAT_RAISE(env, "TypeError", "wrong argument type %s (expected Regexp))", NAT_OBJ_CLASS(splitter)->class_name());
    }
}

Value *String_ljust(Env *env, Value *self_value, ssize_t argc, Value **args, Block *block) {
    NAT_ASSERT_ARGC(1, 2);
    StringValue *self = self_value->as_string();
    Value *length_obj = args[0];
    NAT_ASSERT_TYPE(length_obj, Value::Type::Integer, "Integer");
    ssize_t length = length_obj->as_integer()->to_int64_t() < 0 ? 0 : length_obj->as_integer()->to_int64_t();
    StringValue *padstr;
    if (argc > 1) {
        NAT_ASSERT_TYPE(args[1], Value::Type::String, "String");
        padstr = args[1]->as_string();
    } else {
        padstr = new StringValue { env, " " };
    }
    StringValue *copy = self->dup(env)->as_string();
    while (copy->length() < length) {
        bool truncate = copy->length() + padstr->length() > length;
        copy->append_string(env, padstr);
        if (truncate) {
            copy->truncate(length);
        }
    }
    return copy;
}

}
