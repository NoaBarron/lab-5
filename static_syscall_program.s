.section .data
arg_msg:
    .asciz "Argument: "
arg_msg_len = . - arg_msg
newline:
    .asciz "\n"
newline_len = . - newline

.section .text
.global _start

_start:
    # Get the number of arguments
    pop %eax              # Get argc
    pop %ebx              # Discard program name (argv[0])
    dec %eax             # Decrease argc by 1 (to account for program name)
    mov %eax, %esi       # Save argc in %esi
    mov %esp, %edi       # Save argv pointer in %edi

print_loop:
    # Check if we're done
    test %esi, %esi      # Check if counter is zero
    jz exit              # If zero, exit program
    
    # Print "Argument: " prefix
    mov $4, %eax         # sys_write
    mov $1, %ebx         # stdout
    mov $arg_msg, %ecx   # message
    mov $arg_msg_len, %edx # length
    int $0x80
    
    # Get and print the current argument
    mov (%edi), %ecx     # Get current argument string pointer
    push %edi            # Save argv pointer
    
    # Calculate string length
    mov %ecx, %edi       # Move string pointer to %edi for scanning
    mov $0, %edx         # Initialize counter
strlen_loop:
    cmpb $0, (%edi)      # Check for null terminator
    je strlen_done
    inc %edx             # Increment length
    inc %edi             # Move to next character
    jmp strlen_loop
strlen_done:
    
    # Print the argument
    mov $4, %eax         # sys_write
    mov $1, %ebx         # stdout
    pop %edi             # Restore argv pointer
    int $0x80
    
    # Print newline
    mov $4, %eax         # sys_write
    mov $1, %ebx         # stdout
    mov $newline, %ecx   # newline character
    mov $newline_len, %edx # length
    int $0x80
    
    # Move to next argument
    add $4, %edi         # Point to next argument
    dec %esi             # Decrease counter
    jmp print_loop       # Repeat for next argument

exit:
    mov $1, %eax         # sys_exit
    xor %ebx, %ebx       # status = 0
    int $0x80