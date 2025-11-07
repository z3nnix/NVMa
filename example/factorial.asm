.NVM0

; Calcute factorial  5
push byte 5    ; n = 5
push byte 1    ; result = 1

loop:
    dup         ; duplicate n
    push byte 1
    gt          ; n > 1?
    jz done     ; if no - exit
    
    dup         ; duplcate n
    mul         ; result *= n
    
    push byte 1
    sub         ; n -= 1
    
    jmp loop

done:
    pop         ; delete n
    ; result on the stack (120)

syscall exit
hlt