#include "vm.hpp"
#include "array.hpp"
#include "var.hpp"
#include "global.hpp"
#include "eval.hpp"
#include "bytecode_fn.hpp"
#include "error.hpp"
#include "util.hpp"
#include "multimethod.hpp"
#include "cons.hpp"
#include "util.hpp"

namespace cleo
{
namespace vm
{

static_assert(std::is_same<std::int8_t, signed char>::value, "bytes must be 8-bit, 2's complement");

namespace
{

std::uint16_t read_u16(const Byte *p)
{
    return std::uint8_t(p[0]) | std::uint16_t(std::uint8_t(p[1])) << 8;
}

std::int16_t read_i16(const Byte *p)
{
    return std::int16_t(read_u16(p));
}

const Byte *br(const Byte *p)
{
    return p + (3 + read_i16(p + 1));
}

std::pair<Value, Int64> find_bytecode_fn_body(Value fn, std::uint8_t arity)
{
    auto body = bytecode_fn_find_body(fn, arity);
    if (!body.first)
        throw_arity_error(get_bytecode_fn_name(fn), arity);
    return body;
}

Int64 get_max_arity(Value fn)
{
    auto n = get_bytecode_fn_size(fn);
    if (n == 0)
        return 0;
    return get_bytecode_fn_arity(fn, n - 1);
}

void invoke_body(Value body)
{
    auto consts = get_bytecode_fn_body_consts(body);
    auto vars = get_bytecode_fn_body_vars(body);
    auto exception_table = get_bytecode_fn_body_exception_table(body);
    auto locals_size = get_bytecode_fn_body_locals_size(body);
    auto bytes = get_bytecode_fn_body_bytes(body);
    auto bytes_size = get_bytecode_fn_body_bytes_size(body);
    stack_reserve(locals_size);
    vm::eval_bytecode(consts, vars, locals_size, exception_table, bytes, bytes_size);
}

void call_bytecode_fn(std::uint32_t n)
{
    auto elems = &stack[stack.size() - n];
    auto fn = elems[0];
    auto body_and_arity = find_bytecode_fn_body(fn, n - 1);
    auto body = body_and_arity.first;
    auto arity = body_and_arity.second;
    StackGuard guard(n - 1);
    if (arity < 0)
    {
        auto rest = ~arity + 1;
        if (rest < n)
        {
            elems[rest] = create_array(elems + rest, n - rest).value();
            stack_pop(n - rest - 1);
            stack.back() = array_seq(stack.back()).value();
        }
        else
            stack_push(nil);
    }
    invoke_body(body);
    elems[0] = stack.back();
}

void apply_bytecode_fn(std::uint32_t n)
{
    StackGuard guard(n - 1);
    auto& first = stack[stack.size() - n];
    auto fn = first;
    auto fixed_len = n - 2;
    auto max_arity = get_max_arity(fn);

    Root s{stack.back()};
    s = call_multimethod1(*rt::seq, *s);
    stack_pop();

    if (max_arity < 0)
    {
        auto va_arity = ~max_arity;

        if (fixed_len > va_arity)
        {
            if (*s)
                while (fixed_len > va_arity)
                {
                    s = cons_conj(*s, stack.back());
                    stack_pop();
                    --fixed_len;
                }
            else
            {
                s = create_array(&first + 1 + va_arity, fixed_len - va_arity);
                s = array_seq(*s);
                stack_pop(fixed_len - va_arity);
                fixed_len = va_arity;
            }
        }

        auto len = fixed_len;
        for (; len < va_arity && *s; s = call_multimethod1(*rt::next, *s))
        {
            stack_push(call_multimethod1(*rt::first, *s));
            ++len;
        }
        if (len == va_arity && *s)
        {
            stack_push(*s);
            invoke_body(get_bytecode_fn_body(fn, get_bytecode_fn_size(fn) - 1));
        }
        else
        {
            auto body_and_arity = find_bytecode_fn_body(fn, len);
            if (body_and_arity.second < 0)
                stack_push(*s);
            invoke_body(body_and_arity.first);
        }
    }
    else
    {
        auto len = fixed_len;
        for (; len < max_arity && *s; s = call_multimethod1(*rt::next, *s))
        {
            stack_push(call_multimethod1(*rt::first, *s));
            ++len;
        }
        if (*s)
            throw_call_error("Too many args (" + std::to_string(len + 1) + " or more) passed to: " + to_string(get_bytecode_fn_name(fn)));

        auto body_and_arity = find_bytecode_fn_body(fn, len);
        invoke_body(body_and_arity.first);
    }

    first = stack.back();
}

}

void eval_bytecode(Value constants, Value vars, std::uint32_t locals_size, Value exception_table, const Byte *bytecode, std::uint32_t size)
{
    auto p = bytecode;
    auto endp = p + size;
    auto stack_base = stack.size() - locals_size;
    auto find_exception_handler = [=](const Byte *p, Value ex)
        {
            return exception_table ?
            bytecode_fn_find_exception_handler(exception_table, p - bytecode, get_value_type(ex)) :
            bytecode_fn_exception_handler{-1, -1};
        };
    auto handle_exception = [=](auto handler, const Byte *p, Value ex)
        {
            int_stack.clear();
            stack_pop(stack.size() - stack_base - locals_size - handler.stack_size);
            stack_push(ex);
            return bytecode + handler.offset;
        };
    auto maybe_throw_exception = [=](const Byte *p, Value ex)
        {
            auto handler = find_exception_handler(p, ex);
            if (handler.offset < 0)
                throw_exception(ex);
            return handle_exception(handler, p, ex);
        };
    auto maybe_throw_integer_overflow = [=](const Byte *p)
        {
            Root s{create_string("Integer overflow")};
            Root ex{new_arithmetic_exception(*s)};
            return maybe_throw_exception(p, *ex);
        };

    while (p != endp)
    {
        switch (*p)
        {
        case LDC:
            stack_push(get_array_elem_unchecked(constants, read_u16(p + 1)));
            p += 3;
            break;
        case LDL:
            stack_push(stack[stack_base + read_i16(p + 1)]);
            p += 3;
            break;
        case LDDV:
            stack_push(get_var_value(get_array_elem_unchecked(vars, read_u16(p + 1))));
            p += 3;
            break;
        case LDV:
            stack_push(get_var_root_value(get_array_elem_unchecked(vars, read_u16(p + 1))));
            p += 3;
            break;
        case LDDF:
        {
            auto obj = stack[stack.size() - 2];
            auto type = get_value_type(obj);
            auto field = stack[stack.size() - 1];
            auto index = get_object_field_index(type, field);
            if (index < 0)
            {
                Root msg{create_string("No matching field found: " + to_string(field) + " for type: " + to_string(type))};
                Root ex{new_illegal_argument(*msg)};
                p = maybe_throw_exception(p, *ex);
                break;
            }
            stack[stack.size() - 2] =
                is_object_dynamic(obj) ?
                get_dynamic_object_element(obj, index) :
                (is_object_element_value(obj, index) ?
                 get_static_object_element(obj, index) :
                 create_int64(get_static_object_int(obj, index)).value());
            stack_pop();
            ++p;
            break;
        }
        case STL:
        {
            stack[stack_base + read_i16(p + 1)] = stack.back();
            stack_pop();
            p += 3;
            break;
        }
        case STVV:
        {
            auto var = stack[stack.size() - 2];
            set_var_root_value(var, stack.back());
            stack_pop();
            ++p;
            break;
        }
        case STVM:
        {
            auto var = stack[stack.size() - 2];
            set_var_meta(var, stack.back());
            stack_pop();
            ++p;
            break;
        }
        case POP:
            stack_pop();
            ++p;
            break;
        case BNIL:
        {
            p = stack.back() ? p + 3 : br(p);
            stack_pop();
            break;
        }
        case BNNIL:
        {
            p = stack.back() ? br(p) : p + 3;
            stack_pop();
            break;
        }
        case BR:
            p = br(p);
            break;
        case CALL:
        {
            auto n = std::uint8_t(p[1]) + 1;
            auto& first = stack[stack.size() - n];
            try
            {
                if (get_value_type(first).is(*type::BytecodeFn))
                    call_bytecode_fn(n);
                else
                {
                    first = call(&first, n).value();
                    stack_pop(n - 1);
                }
                p += 2;
            }
            catch (cleo::Exception const& )
            {
                auto handler = find_exception_handler(p, *current_exception);
                if (handler.offset < 0)
                    throw;
                Root ex{catch_exception()};
                p = handle_exception(handler, p, *ex);
            }
            break;
        }
        case APPLY:
        {
            auto n = std::uint8_t(p[1]) + 2;
            auto& first = stack[stack.size() - n];
            try
            {
                if (get_value_type(first).is(*type::BytecodeFn))
                    apply_bytecode_fn(n);
                else
                {
                    first = apply(&first, n).value();
                    stack_pop(n - 1);
                }
                p += 2;
            }
            catch (cleo::Exception const& )
            {
                auto handler = find_exception_handler(p, *current_exception);
                if (handler.offset < 0)
                    throw;
                Root ex{catch_exception()};
                p = handle_exception(handler, p, *ex);
            }
            break;
        }
        case CNIL:
            stack_push(nil);
            ++p;
            break;
        case IFN:
        {
            if (auto n = p[1])
            {
                auto fn = stack[stack.size() - n - 1];
                auto consts = &stack[stack.size() - n];
                stack[stack.size() - n - 1] = bytecode_fn_replace_consts(fn, consts, n).value();
                stack_pop(n);
            }
            p += 2;
            break;
        }
        case THROW:
        {
            p = maybe_throw_exception(p, stack.back());
            break;
        }
        case BXI64:
        {
            stack_push(create_int64(int_stack.back()));
            int_stack_pop();
            ++p;
            break;
        }
        case UBXI64:
        {
            auto val = stack.back();
            if (get_value_tag(val) != tag::INT64)
            {
                Root msg{create_string("Cannot unbox " + to_string(get_value_type(val)) + " as Int64")};
                Root ex{new_illegal_argument(*msg)};
                p = maybe_throw_exception(p, *ex);
                break;
            }
            int_stack_push(get_int64_value(val));
            stack_pop();
            ++p;
            break;
        }
        case ADDI64:
        {
            auto x = std::uint64_t(int_stack.back());
            int_stack_pop();
            auto y = std::uint64_t(int_stack.back());
            auto r = x + y;
            if (std::int64_t((x ^ r) & (y ^ r)) < 0)
            {
                p = maybe_throw_integer_overflow(p);
                break;
            }
            int_stack.back() = r;
            ++p;
        }
        }
    }
}

}
}
