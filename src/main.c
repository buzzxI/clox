#include <stdlib.h>
#include <stdio.h>
#include "common.h"
#include "vm/vm.h"

int main(int argc, const char* argv[]) {
    VM vm;
    init_vm(&vm);
    Chunk chunk;
    init_chunk(&chunk);
    // append 1.2
    write_chunk(&chunk, OP_CONSTANT, 123);
    int idx = append_constant(&chunk, 1.2);
    write_chunk(&chunk, idx, 123);
    // append 3.4
    write_chunk(&chunk, OP_CONSTANT, 123);
    idx = append_constant(&chunk, 3.4);
    write_chunk(&chunk, idx, 123);
    // append OP_ADD
    write_chunk(&chunk, OP_ADD, 123);
    // append 5.6
    write_chunk(&chunk, OP_CONSTANT, 123);
    idx = append_constant(&chunk, 5.6);
    write_chunk(&chunk, idx, 123);
    // append OP_DIVIDE
    write_chunk(&chunk, OP_DIVIDE, 123);
    // append OP_NEGATE
    write_chunk(&chunk, OP_NEGATE, 123);
    // append OP_RETUNR
    write_chunk(&chunk, OP_RETURN, 123); 
    InterpreterResult rst = interpret(&vm, &chunk);
    printf("running rst: %d\n", rst);
    free_chunk(&chunk);
    free_vm(&vm);
    return 0;
}