global task_spinup

section .text

bits 32

task_spinup:
    mov esp, dword [esp+4]

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
    iretd
