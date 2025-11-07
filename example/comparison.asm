.NVM0

push byte 15
push byte 10
cmp           ; save 15 и 10

; Result of comparison on the stack:
; -1 if a < b, 0 if a == b, 1 if a > b

dup
push byte 1
eq            ; check whether 1 (a > b)
jnz greater

dup
push byte -1
eq           ; check whether -1 (a < b)
jnz less

; a == b
push byte 0
jmp end

greater:
    push byte 1
    jmp end

less:
    push byte -1

end:
syscall exit
hlt