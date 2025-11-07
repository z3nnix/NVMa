.NVM0

; Calculate 6 * 7
push byte 6
push byte 7
call multiply

; Result (42) on the stack
syscall exit
hlt

; Multiply function
multiply:
    load 0      ; a
    load 1      ; b
    mul         ; a * b
    store 2     ; save result
    
    ; Clear the stack
    pop         ; b
    pop         ; a
    
    ; Return result
    load 2      ; result
    ret