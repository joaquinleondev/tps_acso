#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include <stdbool.h>

#define FLAG_C 0
#define FLAG_V 0

/* Máscaras y desplazamientos para campos de instrucción */
#define RD_MASK 0x0000001F
#define RN_MASK 0x000003E0
#define RM_MASK 0x001F0000
#define IMM12_MASK 0x003FFC00
#define SHIFT1_MASK 0x00400000
#define IMM6_MASK 0x0000FC00
#define SHIFT_TYPE_MASK 0x00C00000
#define IMM9_MASK 0x001FF000
#define IMM16_MASK 0x007FFF80
#define HW_MASK 0x0060000
#define IMM26_MASK 0x03FFFFFF
#define COND_MASK 0x0000000F
#define IMM19_MASK 0x00FFFFE0
#define IMM3_MASK 0x1C00
#define OPTION_MASK 0xE000
#define IMMR_MASK 0x003F0000
#define IMMS_MASK 0x0000FC00

/* Desplazamientos */
#define RN_SHIFT 5
#define RM_SHIFT 16
#define IMM12_SHIFT 10
#define SHIFT1_SHIFT 22
#define IMM6_SHIFT 10
#define SHIFT_TYPE_SHIFT 22
#define IMM9_SHIFT 12
#define IMM16_SHIFT 5
#define HW_SHIFT 21
#define IMMR_SHIFT 16
#define IMMS_SHIFT 10

/* Opcodes */
#define OP_HALT 0x6A2
#define OP_ADDS_EXT 0x558
#define OP_ADDS_IMM 0x588
#define OP_SUBS_EXT 0x758
#define OP_SUBS_IMM 0x788
#define OP_ANDS_SHIFT 0x750
#define OP_EOR_SHIFT 0x650
#define OP_ORR_SHIFT 0x550
#define B_TARGET 0x140
#define OP_STUR 0x7C0
#define OP_STURB 0x1C0
#define OP_STURH 0x3C0
#define OP_LDUR 0x7C2
#define OP_LDURB 0x1C2
#define OP_LDURH 0x3C2
#define OP_MOVZ 0x1A5
#define OP_LSL 0x1A6
#define OP_MUL 0x4D8

/* Mascaras opcodes por categoria */
#define OP_MASK 0xFFE00000
#define OP_MASK_BRANCH 0xFC000000
#define OP_MASK_BR 0xFFFFFC00
#define OP_MASK_BCOND 0xFF000000
#define OP_MASK_LSL 0xFF800000
#define OP_MASK_ADD 0xFF100000
#define OP_MASK_MOVZ 0xFF000000

/* Patrones */
#define OP_BRANCH 0x5
#define OP_BR 0x3587C0
#define OP_BCOND 0x54
#define OP_CBZ 0xB4
#define OP_CBNZ 0xB5

#define INSTRUCTION_TABLE_SIZE (sizeof(instruction_table) / sizeof(instruction_table[0]))

typedef struct
{
    uint32_t mask;
    uint32_t pattern;
    const char *name;
    void (*handler)(uint32_t);
} instruction_t;

typedef enum
{
    SHIFT_LSL = 0,
    SHIFT_LSR = 1,
    SHIFT_ASR = 2,
    SHIFT_ROR = 3
} ShiftType;

/* Declaración adelantada de funciones de manejo de instrucciones */
static void handle_halt(uint32_t instruction);
static void handle_add_ext(uint32_t instruction);
static void handle_adds_ext(uint32_t instruction);
static void handle_adds_imm(uint32_t instruction);
static void handle_subs_ext(uint32_t instruction);
static void handle_subs_imm(uint32_t instruction);
static void handle_ands_shift(uint32_t instruction);
static void handle_eor_shift(uint32_t instruction);
static void handle_orr_shift(uint32_t instruction);
static void handle_stur(uint32_t instruction);
static void handle_sturb(uint32_t instruction);
static void handle_sturh(uint32_t instruction);
static void handle_ldur(uint32_t instruction);
static void handle_ldurb(uint32_t instruction);
static void handle_ldurh(uint32_t instruction);
static void handle_movz(uint32_t instruction);
static void handle_branch(uint32_t instruction);
static void handle_br(uint32_t instruction);
static void handle_bcond(uint32_t instruction);
static void handle_lsl_lsr(uint32_t instruction);
static void handle_mul(uint32_t instruction);
static void handle_cbz(uint32_t instruction);
static void handle_cbnz(uint32_t instruction);
static void handle_add_imm(uint32_t instruction);

/* Declaración de funciones helper */
static inline uint32_t extract_field(uint32_t instruction, uint32_t mask, uint8_t shift);
static uint64_t apply_shift(uint64_t value, ShiftType type, uint32_t amount);
static void set_flags_z_n(CPU_State *state, uint64_t result);
static const char *shift_type_to_str(ShiftType type);
static bool check_condition(uint8_t cond);