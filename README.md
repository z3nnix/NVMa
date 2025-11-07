# NVMa
> The [Novaria](https://github.com/z3nnix/NovariaOS) Virtual Machine bytecode assember

## Deps:
- _any C compiler_
- _any libc_
- [chorus](https://github.com/z3nnix/chorus)

## Build
```
chorus all
```

## Example
```asm
.NVM0

; Calculate 5 + 3
push byte 5
push byte 3
add

; Result (8) now on stack

syscall exit ; result 8 will be use like exitcode
hlt ; if syscall doesn't work - halt the procces
```

## Looks for more examples?
See example/[*](https://github.com/z3nnix/NVMa/tree/main/example)!