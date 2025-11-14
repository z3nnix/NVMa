.NVM0
; Extended memory operations

; Initialize an array in local variables
; locals[0] = array size (3)
; locals[1-3] = array elements

push 3
store 0         ; array size = 3

; Initialize array values
push 10
store 1         ; array[0] = 10

push 20  
store 2         ; array[1] = 20

push 30
store 3         ; array[2] = 30

; Calculate sum of array elements
push 0          ; Initialize sum = 0
store 4         ; Store sum in local 4

push 0          ; Initialize index = 0
store 5         ; Store index in local 5

sum_loop:
    load 5          ; Load current index
    load 0          ; Load array size
    lt              ; Check if index < size
    jz sum_done
    
    ; Calculate array element address: 1 + index
    load 5
    push 1
    add
    store 6         ; Store address in local 6
    
    ; Load array element using the address
    load 6          ; This loads the ADDRESS, not the value!
    ; We can't do indirect addressing, so we'll use a different approach
    
    ; Instead, let's unroll the loop for our fixed-size array
    jmp sum_unrolled

sum_unrolled:
    ; Directly access each element
    load 1          ; array[0]
    load 2          ; array[1]  
    add
    load 3          ; array[2]
    add
    store 4         ; sum = 10 + 20 + 30 = 60
    
    jmp sum_done

sum_done:
    load 4          ; Load sum (should be 60)
    break
    
    ; Demonstrate variable swapping
    load 1          ; array[0] = 10
    load 3          ; array[2] = 30
    store 1         ; array[0] = 30
    store 3         ; array[2] = 10
    
    ; Verify swap
    load 1          ; Should be 30 now
    break
    load 3          ; Should be 10 now  
    break

hlt