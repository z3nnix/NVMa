.NVM0
; Calculate factorial of 5

push 5          ; Input number
store 0         ; Store in local 0
push 1          ; Initialize result = 1
store 1         ; Store in local 1

factorial_loop:
    load 0          ; Load current number
    push 1
    lt              ; Check if number < 1
    jnz factorial_done
    
    ; result = result * number
    load 1
    load 0
    mul
    store 1
    
    ; number = number - 1
    load 0
    push 1
    sub
    store 0
    
    jmp factorial_loop

factorial_done:
    load 1          ; Load result (should be 120)
    break
    hlt