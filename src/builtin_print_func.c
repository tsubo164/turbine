#include "builtin_print_func.h"
#include "parser_type.h"
#include "runtime_vec.h"
#include "runtime_map.h"
#include "runtime_set.h"
#include "runtime_stack.h"
#include "runtime_queue.h"
#include "runtime_string.h"
#include "runtime_struct.h"
#include "runtime_value.h"
#include "vm_cpu.h"

#include <assert.h>
#include <stdio.h>
#include <math.h>

static void print_value(const struct vm_cpu *vm, struct runtime_value val, struct parser_typelist_iterator *it)
{
    switch (it->kind) {

    case TYP_NIL:
        printf("nil");
        return;

    case TYP_BOOL:
        if (val.inum)
            printf("true");
        else
            printf("false");
        return;

    case TYP_INT:
        printf("%" PRIival, val.inum);
        return;

    case TYP_FLOAT:
        printf("%g", val.fpnum);
        if (fmod(val.fpnum, 1.0) == 0.0)
            printf(".0");
        return;

    case TYP_STRING:
        printf("%s", runtime_string_get_cstr(val.string));
        return;

    case TYP_VEC:
        {
            struct parser_typelist_iterator elem_it;
            int len = runtime_vec_len(val.vec);

            parser_typelist_next(it);
            elem_it = *it;

            printf("{");
            for (int i = 0; i < len; i++) {
                *it = elem_it;
                print_value(vm, runtime_vec_get(val.vec, i), it);
                if (i < len - 1)
                    printf(", ");
            }
            printf("}");

            if (len == 0)
                parser_typelist_skip_next(it);
        }
        return;

    case TYP_MAP:
        {
            struct parser_typelist_iterator elem_it;

            parser_typelist_next(it);
            elem_it = *it;

            printf("{");
            struct runtime_map_entry *ent;
            for (ent = runtime_map_entry_begin(val.map);
                    ent; ent = runtime_map_entry_next(ent)) {

                *it = elem_it;
                printf("%s:", runtime_string_get_cstr(ent->key.string));
                print_value(vm, ent->val, it);

                if (runtime_map_entry_next(ent))
                    printf(", ");
            }
            printf("}");

            if (runtime_map_len(val.map) == 0)
                parser_typelist_skip_next(it);
        }
        return;

    case TYP_SET:
        {
            struct parser_typelist_iterator elem_it;

            parser_typelist_next(it);
            elem_it = *it;

            printf("{");
            struct runtime_set_node *node;
            for (node = runtime_set_node_begin(val.set);
                    node; node = runtime_set_node_next(node)) {

                *it = elem_it;
                print_value(vm, node->val, it);

                if (runtime_set_node_next(node))
                    printf(", ");
            }
            printf("}");

            if (runtime_set_len(val.set) == 0)
                parser_typelist_skip_next(it);
        }
        return;

    case TYP_STACK:
        {
            struct parser_typelist_iterator elem_it;
            int len = runtime_stack_len(val.stack);

            parser_typelist_next(it);
            elem_it = *it;

            printf("{");
            for (int i = 0; i < len; i++) {
                *it = elem_it;
                print_value(vm, runtime_stack_get(val.stack, i), it);
                if (i < len - 1)
                    printf(", ");
            }
            printf("}");

            if (runtime_stack_len(val.stack) == 0)
                parser_typelist_skip_next(it);
        }
        return;

    case TYP_QUEUE:
        {
            struct parser_typelist_iterator elem_it;
            int len = runtime_queue_len(val.queue);

            parser_typelist_next(it);
            elem_it = *it;

            printf("{");
            for (int i = 0; i < len; i++) {
                *it = elem_it;
                print_value(vm, runtime_queue_get(val.queue, i), it);
                if (i < len - 1)
                    printf(", ");
            }
            printf("}");

            if (runtime_queue_len(val.queue) == 0)
                parser_typelist_skip_next(it);
        }
        return;

    case TYP_STRUCT:
        {
            int len = runtime_struct_field_count(val.strct);
            parser_typelist_next(it);

            printf("{");
            for (int i = 0; i < len; i++) {
                print_value(vm, runtime_struct_get(val.strct, i), it);
                if (i < len - 1)
                    printf(", ");
                parser_typelist_next(it);
            }
            printf("}");
            assert(parser_typelist_struct_end(it));
        }
        return;

    case TYP_ENUM:
        {
            struct runtime_value field = vm_get_enum_field(vm, val.inum);
            printf("%s", runtime_string_get_cstr(field.string));
        }
        return;

    default:
        assert(!"variadic argument error");
        return;
    }
}

void builtin_print_func(const struct vm_cpu *vm, const struct runtime_value *args, const char *typelist)
{
    const struct runtime_value *arg;
    struct parser_typelist_iterator it;

    parser_typelist_begin(&it, typelist);
    arg = args;

    while (!parser_typelist_end(&it)) {
        print_value(vm, *arg++, &it);
        parser_typelist_next(&it);

        if (!parser_typelist_end(&it))
            printf(" ");
    }
    printf("\n");
}
