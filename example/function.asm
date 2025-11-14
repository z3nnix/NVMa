.NVM0
; Function demonstration

; Call function with parameter 5
push 5
call multiply_by_3

; Result should be on stack - print it
break

; Call function with different parameter
push 10
call multiply_by_3
break

hlt

; Function: multiply_by_3
; Input: number on stack
; Output: number * 3 on stack
multiply_by_3:
    dup             ; Duplicate input (now we have: input, input)
    dup             ; Triple duplicate (input, input, input)
    add             ; input + input = 2*input
    add             ; 2*input + input = 3*input
    ret