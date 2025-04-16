; /** defines bool y puntero **/
%define NULL 0
%define TRUE 1
%define FALSE 0

section .data

section .text

global string_proc_list_create_asm
global string_proc_node_create_asm
global string_proc_list_add_node_asm
global string_proc_list_concat_asm

; FUNCIONES auxiliares que pueden llegar a necesitar:
extern malloc
extern free
extern str_concat
extern strlen
extern strcpy
extern strcat

%define SIZE_LIST 16
%define TEMP_LIST_PTR rbp-8
%define OFFSET_FIRST 0
%define OFFSET_LAST 8


string_proc_list_create_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 16                      ; reservar espacio en stack

    mov edi, SIZE_LIST               ; malloc(SIZE_LIST)
    call malloc
    mov [TEMP_LIST_PTR], rax         ; guardar puntero devuelto

    cmp qword [TEMP_LIST_PTR], 0     ; si es NULL, saltar a retorno
    je .retornar

    mov rax, [TEMP_LIST_PTR]
    mov qword [rax + OFFSET_FIRST], 0   ; list->first = NULL
    mov qword [rax + OFFSET_LAST], 0    ; list->last = NULL

.retornar:
    mov rax, [TEMP_LIST_PTR]         ; devolver puntero
    leave
    ret

%define OFFSET_NEXT     0
%define OFFSET_PREV     8
%define OFFSET_TYPE     16
%define OFFSET_HASH     24
%define SIZE_NODE       32
%define TMP_HASH_PTR    rbp-32
%define TMP_TYPE_BYTE   rbp-20
%define TMP_NODE_PTR    rbp-8

string_proc_node_create_asm:
    push rbp
    mov rbp, rsp
    sub rsp, SIZE_NODE

    mov eax, edi                 ; guardar `type` en AL
    mov [TMP_HASH_PTR], rsi      ; guardar `hash`
    mov [TMP_TYPE_BYTE], al      ; guardar `type`

    mov edi, SIZE_NODE
    call malloc
    mov [TMP_NODE_PTR], rax      ; guardar node*

    cmp qword [TMP_NODE_PTR], 0
    je .retornar

    ; node->next = NULL
    mov rax, [TMP_NODE_PTR]
    mov qword [rax + OFFSET_NEXT], 0

    ; node->previous = NULL
    mov rax, [TMP_NODE_PTR]
    mov qword [rax + OFFSET_PREV], 0

    ; node->type = type
    mov rax, [TMP_NODE_PTR]
    movzx edx, byte [TMP_TYPE_BYTE]
    mov byte [rax + OFFSET_TYPE], dl

    ; node->hash = hash
    mov rax, [TMP_NODE_PTR]
    mov rdx, [TMP_HASH_PTR]
    mov [rax + OFFSET_HASH], rdx

.retornar:
    mov rax, [TMP_NODE_PTR]
    leave
    ret

; Definiciones de offsets
%define OFFSET_NEXT     0
%define OFFSET_PREV     8
%define OFFSET_TYPE     16
%define OFFSET_HASH     24
%define SIZE_NODE       32

%define OFFSET_FIRST     0
%define OFFSET_LAST      8
%define SIZE_LIST        16

; Variables locales
%define TMP_LIST_PTR     rbp-24
%define TMP_TYPE_BYTE    rbp-28
%define TMP_HASH_PTR     rbp-40
%define TMP_NODE_PTR     rbp-8

string_proc_list_add_node_asm:
    push rbp
    mov rbp, rsp
    sub rsp, 48

    ; Guardar parámetros
    mov [TMP_LIST_PTR], rdi
    mov eax, esi
    mov [TMP_TYPE_BYTE], al
    mov [TMP_HASH_PTR], rdx

    ; Verificar si list es NULL
    cmp qword [TMP_LIST_PTR], 0
    je .retornar

    ; Llamar a string_proc_node_create(type, hash)
    movzx edi, byte [TMP_TYPE_BYTE] ; pasar type en edi
    mov rsi, [TMP_HASH_PTR]         ; pasar hash en rsi
    call string_proc_node_create_asm
    mov [TMP_NODE_PTR], rax         ; guardar new_node

    ; Verificar si el nodo creado es NULL
    cmp qword [TMP_NODE_PTR], 0
    je .retornar

    ; new_node->next = NULL
    mov rax, [TMP_NODE_PTR]
    mov qword [rax + OFFSET_NEXT], 0

    ; new_node->previous = list->last
    mov rax, [TMP_LIST_PTR]
    mov rdx, [rax + OFFSET_LAST]
    mov rax, [TMP_NODE_PTR]
    mov [rax + OFFSET_PREV], rdx

    ; if (list->last != NULL)
    mov rax, [TMP_LIST_PTR]
    mov rax, [rax + OFFSET_LAST]
    test rax, rax
    je .set_first

    ; list->last->next = new_node
    mov rax, [TMP_LIST_PTR]
    mov rax, [rax + OFFSET_LAST]
    mov rdx, [TMP_NODE_PTR]
    mov [rax + OFFSET_NEXT], rdx
    jmp .set_last

.set_first:
    ; list->first = new_node
    mov rax, [TMP_LIST_PTR]
    mov rdx, [TMP_NODE_PTR]
    mov [rax + OFFSET_FIRST], rdx

.set_last:
    ; list->last = new_node
    mov rax, [TMP_LIST_PTR]
    mov rdx, [TMP_NODE_PTR]
    mov [rax + OFFSET_LAST], rdx

.retornar:
    leave
    ret


%define NODE_NEXT      0
%define NODE_PREV      8
%define NODE_TYPE      16
%define NODE_HASH      24

%define LIST_FIRST     0
%define LIST_LAST      8

%define STACK_HASH_PTR     -56    ; char *hash (param)
%define STACK_LIST_PTR     -40    ; string_proc_list *list (param)
%define STACK_TYPE_VAL     -44    ; uint8_t type (param)
%define STACK_TOTAL_LEN    -8     ; size_t total_len
%define STACK_CURRENT      -16    ; string_proc_node *current
%define STACK_RESULT       -24    ; char *result

global string_proc_list_concat_asm
string_proc_list_concat_asm:
    push    rbp
    mov     rbp, rsp
    sub     rsp, 64                    ; reservar espacio local

    ; Guardar parámetros en stack
    mov     [rbp + STACK_LIST_PTR], rdi
    mov     [rbp + STACK_HASH_PTR], rdx
    mov     byte [rbp + STACK_TYPE_VAL], sil

    ; Verificación de punteros nulos: if (list == NULL || hash == NULL)
    cmp     qword [rbp + STACK_LIST_PTR], 0
    je      .return_null
    cmp     qword [rbp + STACK_HASH_PTR], 0
    je      .return_null

    ; total_len = strlen(hash)
    mov     rdi, [rbp + STACK_HASH_PTR]
    call    strlen
    mov     [rbp + STACK_TOTAL_LEN], rax

    ; current = list->first
    mov     rax, [rbp + STACK_LIST_PTR]
    mov     rax, [rax + LIST_FIRST]
    mov     [rbp + STACK_CURRENT], rax

.calculate_total_len_loop:
    cmp     qword [rbp + STACK_CURRENT], 0
    je      .allocate_memory

    mov     rax, [rbp + STACK_CURRENT]
    movzx   eax, byte [rax + NODE_TYPE]
    cmp     al, [rbp + STACK_TYPE_VAL]
    jne     .skip_node_len

    mov     rax, [rbp + STACK_CURRENT]
    mov     rax, [rax + NODE_HASH]
    test    rax, rax
    je      .skip_node_len

    ; total_len += strlen(current->hash)
    mov     rdi, rax
    call    strlen
    add     [rbp + STACK_TOTAL_LEN], rax

.skip_node_len:
    ; current = current->next
    mov     rax, [rbp + STACK_CURRENT]
    mov     rax, [rax + NODE_NEXT]
    mov     [rbp + STACK_CURRENT], rax
    jmp     .calculate_total_len_loop

.allocate_memory:
    mov     rax, [rbp + STACK_TOTAL_LEN]
    add     rax, 1             ; +1 para '\0'
    mov     rdi, rax
    call    malloc
    mov     [rbp + STACK_RESULT], rax

    cmp     rax, 0
    je      .return_null

    ; strcpy(result, hash)
    mov     rsi, [rbp + STACK_HASH_PTR]
    mov     rdi, [rbp + STACK_RESULT]
    call    strcpy

    ; current = list->first (nuevamente)
    mov     rax, [rbp + STACK_LIST_PTR]
    mov     rax, [rax + LIST_FIRST]
    mov     [rbp + STACK_CURRENT], rax

.concat_loop:
    cmp     qword [rbp + STACK_CURRENT], 0
    je      .return_result

    mov     rax, [rbp + STACK_CURRENT]
    movzx   eax, byte [rax + NODE_TYPE]
    cmp     al, [rbp + STACK_TYPE_VAL]
    jne     .skip_node_concat

    mov     rax, [rbp + STACK_CURRENT]
    mov     rax, [rax + NODE_HASH]
    test    rax, rax
    je      .skip_node_concat

    ; strcat(result, current->hash)
    mov     rsi, rax
    mov     rdi, [rbp + STACK_RESULT]
    call    strcat

.skip_node_concat:
    ; current = current->next
    mov     rax, [rbp + STACK_CURRENT]
    mov     rax, [rax + NODE_NEXT]
    mov     [rbp + STACK_CURRENT], rax
    jmp     .concat_loop

.return_result:
    mov     rax, [rbp + STACK_RESULT]
    jmp     .end

.return_null:
    mov     eax, 0

.end:
    leave
    ret
