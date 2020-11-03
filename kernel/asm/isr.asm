%macro pusham 0
    cld
    push gs
    push fs
    push es
    push ds
    push ebp
    push edi
    push esi
    push edx
    push ecx
    push ebx
    push eax
%endmacro

%macro popam 0
    pop eax
    pop ebx
    pop ecx
    pop edx
    pop esi
    pop edi
    pop ebp
    pop ds
    pop es
    pop fs
    pop gs
%endmacro

global handler_simple
global handler_code
global handler_irq_pic0
global handler_irq_pic1
global irq0_handler
global keyboard_isr
global syscall

global ts_enable
global read_stat
global write_stat

global exception_thunks
global escalate_priv_isr

extern keyboard_handler
extern task_switch

extern exception_handler

extern set_PIC0_mask
extern get_PIC0_mask

; API calls
extern open
extern close
extern read
extern write
extern lseek
extern getpid
extern signal
extern task_fork
extern task_quit_self
extern alloc
extern free
extern realloc
extern enter_iowait_status
extern enter_iowait_status1
extern enter_ipcwait_status
extern enter_vdevwait_status
extern pwd
extern what_stdin
extern what_stdout
extern what_stderr
extern ipc_send_packet
extern ipc_read_packet
extern ipc_resolve_name
extern ipc_payload_sender
extern ipc_payload_length
extern vfs_cd
extern vfs_read
extern vfs_write
extern vfs_remove
extern vfs_mkdir
extern vfs_create
extern vfs_list
extern vfs_get_metadata
extern general_execute
extern general_execute_block
extern execve
extern register_vdev
extern vdev_in_ready
extern vdev_out_ready
extern get_heap_base
extern get_heap_size
extern resize_heap
extern syscall_log
extern syscall_new_segment

extern escalate_privilege

section .data

ts_enable dd 1
read_stat dd 0
write_stat dd 0

routine_list:
        dd      task_quit_self          ; 0x00
        dd      general_execute         ; 0x01
        dd      0 ;general_execute_block; 0x02 - dummy entry
        dd      execve                  ; 0x03
        dd      syscall_log             ; 0x04
        dd      0 ;task_fork            ; 0x05 - dummy entry
        dd      syscall_new_segment     ; 0x06
        dd      0                       ; 0x07
        dd      ipc_send_packet         ; 0x08
        dd      ipc_read_packet         ; 0x09
        dd      ipc_resolve_name        ; 0x0a
        dd      ipc_payload_sender      ; 0x0b
        dd      ipc_payload_length      ; 0x0c
        dd      0 ;ipc_await              0x0d - dummy entry
        dd      0                       ; 0x0e
        dd      0                       ; 0x0f
        dd      get_heap_base           ; 0x10
        dd      get_heap_size           ; 0x11
        dd      resize_heap             ; 0x12
        dd      0                       ; 0x13
        dd      0                       ; 0x14
        dd      getpid                  ; 0x15
        dd      signal                  ; 0x16
        dd      0                       ; 0x17
        dd      0                       ; 0x18
        dd      0                       ; 0x19
        dd      pwd                     ; 0x1a
        dd      what_stdin              ; 0x1b
        dd      what_stdout             ; 0x1c
        dd      what_stderr             ; 0x1d
        dd      0                       ; 0x1e
        dd      0                       ; 0x1f
        dd      register_vdev           ; 0x20
        dd      vdev_in_ready           ; 0x21
        dd      vdev_out_ready          ; 0x22
        dd      0 ;vdev_await           ; 0x23 - dummy entry
        dd      0                       ; 0x24
        dd      0                       ; 0x25
        dd      0                       ; 0x26
        dd      0                       ; 0x27
        dd      0                       ; 0x28
        dd      0                       ; 0x29
        dd      open                    ; 0x2a
        dd      close                   ; 0x2b
        dd      0 ;read                 ; 0x2c - dummy entry
        dd      0 ;write                ; 0x2d - dummy entry
        dd      lseek                   ; 0x2e
        dd      vfs_cd                  ; 0x2f
        dd      0 ;vfs_read             ; 0x30 - dummy entry
        dd      0 ;vfs_write            ; 0x31 - dummy entry
        dd      vfs_list                ; 0x32
        dd      vfs_get_metadata        ; 0x33
        dd      vfs_remove              ; 0x34
        dd      vfs_mkdir               ; 0x35
        dd      vfs_create              ; 0x36


%macro raise_exception_getaddr 1
dd raise_exception_%1
%endmacro

%macro raise_exception_err_getaddr 1
dd raise_exception_err_%1
%endmacro

global exception_thunks
exception_thunks:
%assign i 0
%rep 32

%if i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 30
    raise_exception_err_getaddr i
%else
    raise_exception_getaddr i
%endif

%assign i i+1
%endrep

section .text

bits 32

%macro raise_exception_err 1
raise_exception_err_%1:
    push dword [esp+5*4]
    push dword [esp+5*4]
    push dword [esp+5*4]
    push dword [esp+5*4]
    push dword [esp+5*4]
    pusham
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push dword [esp+16*4]
    push 1
    push eax
    push %1
    xor ebp, ebp   ; EBP barrier
    call exception_handler
    popam
    iretd
%endmacro

%macro raise_exception 1
raise_exception_%1:
    pusham
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push 0
    push 0
    push eax
    push %1
    xor ebp, ebp   ; EBP barrier
    call exception_handler
    popam
    iretd
%endmacro

%assign i 0
%rep 32
raise_exception i
%assign i i+1
%endrep

%assign i 0
%rep 32
raise_exception_err i
%assign i i+1
%endrep

escalate_priv_isr:
    mov eax, dword [esp]
    mov esp, dword [esp+12]
    jmp eax

irq0_handler:
        push eax
        mov al, 0x20    ; acknowledge interrupt to PIC0
        out 0x20, al
        pop eax
        cmp dword [ss:ts_enable], 0
        je .ts_abort
        ; save task status
        pusham
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        push esp
        xor ebp, ebp   ; EBP barrier
        call task_switch
    .ts_abort:
        iretd

keyboard_isr:
        pusham
        mov ax, 0x10
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        xor eax, eax
        in al, 0x60     ; read from keyboard
        push eax
        xor ebp, ebp   ; EBP barrier
        call keyboard_handler
        add esp, 4
        mov al, 0x20    ; acknowledge interrupt to PIC0
        out 0x20, al
        popam
        iretd

global last_syscall
last_syscall dd -1

; ARGS in EAX (call code), ECX, EDX, EDI, ESI
; return value in EAX/EDX
syscall:
        mov dword [ss:last_syscall], eax
        ; special routines check
        cmp eax, 0x0d
        je ipc_await
        cmp eax, 0x23
        je vdev_await
        cmp eax, 0x05
        je fork_isr
        ; disable task switch, reenable all interrupts
        mov dword [ss:ts_enable], 0
        sti
        ; special routines check
        cmp eax, 0x30
        je vfs_read_isr
        cmp eax, 0x31
        je vfs_write_isr
        cmp eax, 0x2c
        je read_isr
        cmp eax, 0x2d
        je write_isr
        cmp eax, 0x02
        je gen_exec_block_isr
        ; end special routines check
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        push ds
        push es
        push fs
        push gs
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        ; push syscall args, and call
        push esi
        push edi
        push edx
        push ecx
        call [routine_list+eax*4]
        add esp, 16
        ; disable all interrupts, reenable task switch
        cli
        mov dword [ts_enable], 1
        ; return
        pop gs
        pop fs
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx
        iretd

vfs_read_isr:
        ; check if I/O is ready
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        push ds
        push es
        push fs
        push gs
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        push esi
        push edi
        push edx
        push ecx
        xor ebp, ebp   ; EBP barrier
        call vfs_read
        add esp, 16
        ; disable all interrupts, reenable task switch
        cli
        mov dword [ts_enable], 1
        ; done
        pop gs
        pop fs
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx
        cmp eax, -5     ; if I/O is not ready
        je .enter_iowait
        iretd           ; else, just return
    .enter_iowait:
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov ax, 0x21
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        push 0      ; VFS read type
        push esi
        push edi
        push edx
        push ecx
        call enter_iowait_status
        add esp, 20
        call escalate_privilege
        push esp
        call task_switch

vfs_write_isr:
        ; check if I/O is ready
        push ebx
        push ecx
        push esi
        push edi
        push ebp
        push ds
        push es
        push fs
        push gs
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        push esi
        push edi
        push edx
        push ecx
        xor ebp, ebp   ; EBP barrier
        call vfs_write
        add esp, 16
        ; disable all interrupts, reenable task switch
        cli
        mov dword [ts_enable], 1
        ; done
        pop gs
        pop fs
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop ecx
        pop ebx
        cmp eax, -5     ; if I/O is not ready
        je .enter_iowait
        iretd           ; else, just return
    .enter_iowait:
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov ax, 0x21
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        push 1      ; VFS write type
        push esi
        push edi
        push edx
        push ecx
        call enter_iowait_status
        add esp, 20
        call escalate_privilege
        push esp
        call task_switch

read_isr:
        ; check if I/O is ready
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        push ds
        push es
        push fs
        push gs
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        push esi
        push edi
        push edx
        push ecx
        xor ebp, ebp   ; EBP barrier
        call read
        add esp, 16
        ; disable all interrupts, reenable task switch
        cli
        mov dword [ts_enable], 1
        cmp dword [read_stat], 1     ; if I/O is not ready
        ; done
        pop gs
        pop fs
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        je .enter_iowait
        iretd           ; else, just return
    .enter_iowait:
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        push eax
        push 2      ; read type
        push edi
        push edx
        push ecx
        call enter_iowait_status1
        add esp, 20
        call escalate_privilege
        push esp
        call task_switch

write_isr:
        ; check if I/O is ready
        push ebx
        push ecx
        push edx
        push esi
        push edi
        push ebp
        push ds
        push es
        push fs
        push gs
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        push esi
        push edi
        push edx
        push ecx
        xor ebp, ebp   ; EBP barrier
        call write
        add esp, 16
        ; disable all interrupts, reenable task switch
        cli
        mov dword [ts_enable], 1
        cmp dword [write_stat], 1     ; if I/O is not ready
        ; done
        pop gs
        pop fs
        pop es
        pop ds
        pop ebp
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        je .enter_iowait
        iretd           ; else, just return
    .enter_iowait:
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov bx, 0x21
        mov ds, bx
        mov es, bx
        mov fs, bx
        mov gs, bx
        push eax
        push 3      ; write type
        push edi
        push edx
        push ecx
        call enter_iowait_status1
        add esp, 20
        call escalate_privilege
        push esp
        call task_switch

gen_exec_block_isr:
        ; save task status
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov ax, 0x21
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        push esi
        push edi
        push edx
        push ecx
        xor ebp, ebp   ; EBP barrier
        call general_execute_block
        add esp, 16
        ; disable all interrupts, reenable task switch
        cli
        mov dword [ts_enable], 1
        ; done
        cmp eax, -1
        je .abort
        call escalate_privilege
        push esp
        call task_switch
    .abort:
        pop eax
        pop ebx
        pop ecx
        pop edx
        pop esi
        pop edi
        pop ebp
        pop ds
        pop es
        pop fs
        pop gs
        mov eax, -1
        mov edx, -1
        iretd

ipc_await:
        ; save task status
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov ax, 0x21
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        xor ebp, ebp   ; EBP barrier
        call enter_ipcwait_status
        call escalate_privilege
        push esp
        call task_switch

vdev_await:
        ; save task status
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        mov ax, 0x21
        mov ds, ax
        mov es, ax
        mov fs, ax
        mov gs, ax
        xor ebp, ebp   ; EBP barrier
        call enter_vdevwait_status
        call escalate_privilege
        push esp
        call task_switch

fork_isr:
        ; save task status
        push gs
        push fs
        push es
        push ds
        push ebp
        push edi
        push esi
        push edx
        push ecx
        push ebx
        push eax
        call escalate_privilege
        push esp
        xor ebp, ebp   ; EBP barrier
        call task_fork
