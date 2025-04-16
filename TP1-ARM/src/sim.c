#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "shell.h"
#include "sim.h"

instruction_t instruction_table[] = {
    {OP_MASK, OP_HALT << 21, "HALT", handle_halt},
    {OP_MASK, OP_ADDS_EXT << 21, "ADDS_EXT", handle_adds_ext},
    {OP_MASK, OP_ADDS_IMM << 21, "ADDS_IMM", handle_adds_imm},
    {OP_MASK, OP_SUBS_EXT << 21, "SUBS_EXT", handle_subs_ext},
    {0xFF000000, OP_SUBS_IMM << 21, "SUBS_IMM", handle_subs_imm},
    {OP_MASK, 0x458 << 21, "ADD_EXT", handle_add_ext},
    {OP_MASK_ADD, 0x122 << 23, "ADD_IMM", handle_add_imm},
    {OP_MASK, OP_ANDS_SHIFT << 21, "ANDS_SHIFT", handle_ands_shift},
    {OP_MASK, OP_EOR_SHIFT << 21, "EOR_SHIFT", handle_eor_shift},
    {OP_MASK, OP_ORR_SHIFT << 21, "ORR_SHIFT", handle_orr_shift},
    {OP_MASK, OP_STUR << 21, "STUR", handle_stur},
    {OP_MASK, OP_STURB << 21, "STURB", handle_sturb},
    {OP_MASK, OP_STURH << 21, "STURH", handle_sturh},
    {OP_MASK, OP_LDUR << 21, "LDUR", handle_ldur},
    {OP_MASK, OP_LDURB << 21, "LDURB", handle_ldurb},
    {OP_MASK, OP_LDURH << 21, "LDURH", handle_ldurh},
    {OP_MASK, OP_MOVZ << 23, "MOVZ", handle_movz},
    {OP_MASK_BRANCH, OP_BRANCH << 26, "BRANCH", handle_branch},
    {OP_MASK_BR, OP_BR << 10, "BR", handle_br},
    {OP_MASK_BCOND, OP_BCOND << 24, "BCOND", handle_bcond},
    {OP_MASK_LSL, OP_LSL << 23, "LSL", handle_lsl_lsr},
    {OP_MASK, OP_MUL << 21, "MUL", handle_mul},
    {OP_MASK_BCOND, OP_CBZ << 24, "CBZ", handle_cbz},
    {OP_MASK_BCOND, OP_CBNZ << 24, "CBNZ", handle_cbnz},
};

void process_instruction()
{
    uint32_t instruction = mem_read_32(CURRENT_STATE.PC);

    for (size_t i = 0; i < INSTRUCTION_TABLE_SIZE; i++)
    {
        instruction_t *entry = &instruction_table[i];
        if ((instruction & entry->mask) == entry->pattern)
        {
            entry->handler(instruction);
            return;
        }
    }

    printf("Unknown instruction: 0x%08x\n", instruction);
    NEXT_STATE.PC += 4;
}

/*
    HANDLERS
*/

static void handle_halt(uint32_t instruction)
{
    printf("[HALT]\n");
    RUN_BIT = 0;
    NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}

static void handle_adds_ext(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);

    uint64_t result = CURRENT_STATE.REGS[rn] + CURRENT_STATE.REGS[rm];
    NEXT_STATE.REGS[rd] = result;
    set_flags_z_n(&NEXT_STATE, result);

    NEXT_STATE.PC += 4;
    printf("[ADDS] X%d, X%d, X%d => 0x%016lx\n", rd, rn, rm, result);
}

static void handle_adds_imm(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint64_t imm = extract_field(instruction, IMM12_MASK, IMM12_SHIFT);
    uint8_t shift = extract_field(instruction, SHIFT1_MASK, SHIFT1_SHIFT);

    if (shift)
        imm <<= 12;

    uint64_t result = CURRENT_STATE.REGS[rn] + imm;
    NEXT_STATE.REGS[rd] = result;
    set_flags_z_n(&NEXT_STATE, result);

    NEXT_STATE.PC += 4;
    printf("[ADDS IMM] X%d, X%d, #0x%lx (shift: %d) => 0x%016lx\n",
           rd, rn, imm, shift, result);
}

static void handle_subs_ext(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);

    uint64_t result = CURRENT_STATE.REGS[rn] - CURRENT_STATE.REGS[rm];
    if (rd != 31)
        NEXT_STATE.REGS[rd] = result;
    set_flags_z_n(&NEXT_STATE, result);

    NEXT_STATE.PC += 4;
    printf("[SUBS/CMP] X%d, X%d, X%d => 0x%016lx\n", rd, rn, rm, result);
}

static void handle_subs_imm(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint64_t imm = extract_field(instruction, IMM12_MASK, IMM12_SHIFT);
    uint8_t shift = extract_field(instruction, SHIFT1_MASK, SHIFT1_SHIFT);

    if (shift)
        imm <<= 12;

    uint64_t result = CURRENT_STATE.REGS[rn] - imm;
    if (rd != 31)
        NEXT_STATE.REGS[rd] = result;
    set_flags_z_n(&NEXT_STATE, result);

    NEXT_STATE.PC += 4;
    printf("[SUBS/CMP IMM] X%d, X%d, #0x%lx (shift: %d) => 0x%016lx\n",
           rd, rn, imm, shift, result);
}

static void handle_ands_shift(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);
    ShiftType shift_type = extract_field(instruction, SHIFT_TYPE_MASK, SHIFT_TYPE_SHIFT);
    uint32_t shift_amount = extract_field(instruction, IMM6_MASK, IMM6_SHIFT);

    uint64_t shifted = apply_shift(CURRENT_STATE.REGS[rm], shift_type, shift_amount);
    uint64_t result = CURRENT_STATE.REGS[rn] & shifted;

    NEXT_STATE.REGS[rd] = result;
    set_flags_z_n(&NEXT_STATE, result);

    NEXT_STATE.PC += 4;
    printf("[ANDS] X%d, X%d, X%d %s #%d => 0x%016lx\n",
           rd, rn, rm, shift_type_to_str(shift_type), shift_amount, result);
}

static void handle_eor_shift(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);
    ShiftType shift_type = extract_field(instruction, SHIFT_TYPE_MASK, SHIFT_TYPE_SHIFT);
    uint32_t shift_amount = extract_field(instruction, IMM6_MASK, IMM6_SHIFT);

    uint64_t shifted = apply_shift(CURRENT_STATE.REGS[rm], shift_type, shift_amount);
    uint64_t result = CURRENT_STATE.REGS[rn] ^ shifted;

    NEXT_STATE.REGS[rd] = result;
    NEXT_STATE.PC += 4;
    printf("[EOR] X%d, X%d, X%d %s #%d => 0x%016lx\n",
           rd, rn, rm, shift_type_to_str(shift_type), shift_amount, result);
}

static void handle_orr_shift(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);
    ShiftType shift_type = extract_field(instruction, SHIFT_TYPE_MASK, SHIFT_TYPE_SHIFT);
    uint32_t shift_amount = extract_field(instruction, IMM6_MASK, IMM6_SHIFT);

    uint64_t shifted = apply_shift(CURRENT_STATE.REGS[rm], shift_type, shift_amount);
    uint64_t result = CURRENT_STATE.REGS[rn] | shifted;

    NEXT_STATE.REGS[rd] = result;
    NEXT_STATE.PC += 4;
    printf("[ORR] X%d, X%d, X%d %s #%d => 0x%016lx\n",
           rd, rn, rm, shift_type_to_str(shift_type), shift_amount, result);
}

static void handle_stur(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint64_t offset = extract_field(instruction, IMM9_MASK, IMM9_SHIFT);

    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    uint64_t aligned_addr = address & ~0x3;
    int byte_offset = address % 4;
    uint64_t value = CURRENT_STATE.REGS[rt];

    uint32_t low_word = value & 0xFFFFFFFF;
    uint32_t high_word = (value >> 32) & 0xFFFFFFFF;

    if (byte_offset == 0)
    {
        // Alineado: escribir directamente
        mem_write_32(aligned_addr, low_word);
        mem_write_32(aligned_addr + 4, high_word);
    }
    else
    {
        // No alineado: leer-modificar-escribir
        uint32_t orig_low = mem_read_32(aligned_addr);
        uint32_t orig_high = mem_read_32(aligned_addr + 4);
        uint32_t next_word = mem_read_32(aligned_addr + 8);

        uint64_t shifted_value = value << (byte_offset * 8);
        uint32_t new_low = (orig_low & ~(0xFFFFFFFF << (byte_offset * 8))) | (shifted_value & 0xFFFFFFFF);
        uint32_t new_high = (shifted_value >> 32) | (next_word & (0xFFFFFFFF << (4 - byte_offset) * 8));

        mem_write_32(aligned_addr, new_low);
        mem_write_32(aligned_addr + 4, new_high);
        if (byte_offset < 4)
        {
            mem_write_32(aligned_addr + 8, next_word & ~(0xFFFFFFFF >> ((4 - byte_offset) * 8)));
        }
    }

    NEXT_STATE.PC += 4;
    printf("[STUR] X%d, [X%d, #0x%lx] => 0x%016lx\n", rt, rn, offset, value);
}

static void handle_sturb(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    int64_t offset = (int64_t)(int16_t)(extract_field(instruction, IMM9_MASK, IMM9_SHIFT) << 7) >> 7;

    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    uint64_t aligned_addr = address & ~0x3;
    int byte_offset = address % 4;
    uint8_t data = CURRENT_STATE.REGS[rt] & 0xFF;

    uint32_t word = mem_read_32(aligned_addr);
    uint32_t mask = ~(0xFF << (byte_offset * 8));
    uint32_t new_value = (word & mask) | ((uint32_t)data << (byte_offset * 8));

    mem_write_32(aligned_addr, new_value);

    NEXT_STATE.PC += 4;
    printf("[STURB] X%d, [X%d, #0x%lx] => 0x%02x\n", rt, rn, offset, data);
}

static void handle_sturh(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint64_t offset = extract_field(instruction, IMM9_MASK, IMM9_SHIFT);

    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    uint64_t aligned_addr = address & ~0x3;
    int byte_offset = address % 4;
    uint16_t data = CURRENT_STATE.REGS[rt] & 0xFFFF;

    uint32_t word = mem_read_32(aligned_addr);
    uint32_t mask = ~(0xFFFF << (byte_offset * 8));
    uint32_t new_value = (word & mask) | ((uint32_t)data << (byte_offset * 8));

    mem_write_32(aligned_addr, new_value);

    // Si cruza el límite de 32 bits
    if (byte_offset > 2)
    {
        uint32_t next_word = mem_read_32(aligned_addr + 4);
        uint32_t next_mask = ~(0xFFFF >> ((4 - byte_offset) * 8));
        uint32_t next_value = (next_word & next_mask) | ((data >> (16 - byte_offset * 8)) & 0xFFFF);
        mem_write_32(aligned_addr + 4, next_value);
    }

    NEXT_STATE.PC += 4;
    printf("[STURH] X%d, [X%d, #0x%lx] => 0x%04x\n", rt, rn, offset, data);
}

static void handle_ldur(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    int64_t offset = (int64_t)(int16_t)(extract_field(instruction, IMM9_MASK, IMM9_SHIFT) << 7) >> 7;

    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    // Alinear a 4 bytes
    uint64_t aligned_addr = address & ~0x3;
    int byte_offset = address % 4;

    // Leer dos palabras de 32 bits para formar 64 bits
    uint32_t low_word = mem_read_32(aligned_addr);
    uint32_t high_word = mem_read_32(aligned_addr + 4);
    uint64_t data = ((uint64_t)high_word << 32) | low_word;

    // Ajustar si la dirección no está alineada
    if (byte_offset)
    {
        uint32_t next_word = mem_read_32(aligned_addr + 8);
        data = (data >> (byte_offset * 8)) | ((uint64_t)next_word << (64 - byte_offset * 8));
    }

    NEXT_STATE.REGS[rt] = data;
    NEXT_STATE.PC += 4;
    printf("[LDUR] X%d, [X%d, #0x%lx] => 0x%016lx\n", rt, rn, offset, data);
}

static void handle_ldurb(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint64_t offset = extract_field(instruction, IMM9_MASK, IMM9_SHIFT);

    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    uint64_t aligned_addr = address & ~0x3;
    int byte_offset = address % 4;

    uint32_t word = mem_read_32(aligned_addr);
    uint8_t data = (word >> (byte_offset * 8)) & 0xFF;

    NEXT_STATE.REGS[rt] = (uint64_t)data;
    NEXT_STATE.PC += 4;
    printf("[LDURB] X%d, [X%d, #0x%lx] => 0x%02x\n", rt, rn, offset, data);
}

static void handle_ldurh(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint64_t offset = extract_field(instruction, IMM9_MASK, IMM9_SHIFT);

    uint64_t address = CURRENT_STATE.REGS[rn] + offset;
    uint64_t aligned_addr = address & ~0x3;
    int byte_offset = address % 4;

    uint32_t word = mem_read_32(aligned_addr);
    uint16_t data = (word >> (byte_offset * 8)) & 0xFFFF;

    // Si el halfword cruza el límite de 32 bits
    if (byte_offset > 2)
    {
        uint32_t next_word = mem_read_32(aligned_addr + 4);
        data = (word >> (byte_offset * 8)) | ((next_word << (16 - byte_offset * 8)) & 0xFFFF);
    }

    NEXT_STATE.REGS[rt] = (uint64_t)data;
    NEXT_STATE.PC += 4;
    printf("[LDURH] X%d, [X%d, #0x%lx] => 0x%04x\n", rt, rn, offset, data);
}

static void handle_movz(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint64_t imm = extract_field(instruction, 0x1FFFE0, 5);
    uint8_t shift = extract_field(instruction, 0x200000, 21);

    if (shift == 0)
    {
        NEXT_STATE.REGS[rd] = imm;
        printf("[MOVZ] X%d, #0x%lx => 0x%016lx\n", rd, imm, imm);
    }

    NEXT_STATE.PC += 4;
}

static void handle_branch(uint32_t instruction)
{
    uint32_t imm26 = extract_field(instruction, IMM26_MASK, 0);
    uint64_t offset = ((int64_t)(imm26 << 6)) >> 4;

    NEXT_STATE.PC = CURRENT_STATE.PC + offset;
    printf("[BRANCH] #%ld\n", offset);
}

static void handle_br(uint32_t instruction)
{
    uint8_t rm = extract_field(instruction, 0xFC0, 5);
    uint64_t offset = CURRENT_STATE.REGS[rm];
    NEXT_STATE.PC = CURRENT_STATE.PC + offset;
    printf("[BR] X%d => #%ld\n", rm, offset);
}

static void handle_bcond(uint32_t instruction)
{
    uint8_t cond = extract_field(instruction, COND_MASK, 0);
    uint32_t imm19 = extract_field(instruction, IMM19_MASK, 5);
    uint64_t offset = ((int32_t)(imm19 << 13)) >> 11;

    if (check_condition(cond))
    {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        printf("[BCOND] #%ld\n", offset);
    }
    else
    {
        NEXT_STATE.PC += 4;
        printf("[BCOND] Not taken\n");
    }
}

static void handle_lsl_lsr(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t immr = extract_field(instruction, IMMR_MASK, IMMR_SHIFT);
    uint8_t imms = extract_field(instruction, IMMS_MASK, IMMS_SHIFT);

    //    printf("IMMR: %d, IMMS: %d, RN: %d, RD:  %d\n", immr, imms, rn, rd);

    uint64_t value = CURRENT_STATE.REGS[rn];
    uint64_t result;
    char *shift_type;

    if (imms == 0x3F)
    {
        shift_type = "LSR";
        result = apply_shift(value, SHIFT_LSR, immr);
    }
    else
    {
        shift_type = "LSL";
        result = apply_shift(value, SHIFT_LSL, 64 - immr);
    }

    NEXT_STATE.REGS[rd] = result;
    NEXT_STATE.PC += 4;
    printf("[%s] X%d, X%d, #%d, #%d => 0x%016lx\n", shift_type, rd, rn, immr, imms, result);
}

static void handle_mul(uint32_t instruction)
{
    uint8_t rd = extract_field(instruction, RD_MASK, 0);
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);

    uint64_t result = CURRENT_STATE.REGS[rn] * CURRENT_STATE.REGS[rm];
    NEXT_STATE.REGS[rd] = result;
    set_flags_z_n(&NEXT_STATE, result);

    NEXT_STATE.PC += 4;
    printf("[MUL] X%d, X%d, X%d => 0x%016lx\n", rd, rn, rm, result);
}

static void handle_cbz(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint32_t imm19 = extract_field(instruction, IMM19_MASK, 5);

    // Correct sign extension and word alignment
    int64_t offset = ((int32_t)(imm19 << 13)) >> 13; // Sign extend imm19 and shift correctly
    offset *= 4;                                     // Ensure offset is word-aligned

    if (CURRENT_STATE.REGS[rt] == 0)
    {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        printf("[CBZ] X%d == 0, Branch to 0x%lx\n", rt, NEXT_STATE.PC);
    }
    else
    {
        NEXT_STATE.PC += 4;
        printf("[CBZ] X%d != 0, No branch\n", rt);
    }
}

static void handle_cbnz(uint32_t instruction)
{
    uint8_t rt = extract_field(instruction, RD_MASK, 0);
    uint32_t imm19 = extract_field(instruction, IMM19_MASK, 5);

    // Correct sign extension and word alignment
    int64_t offset = ((int32_t)(imm19 << 13)) >> 13; // Sign extend imm19 and shift correctly
    offset *= 4;                                     // Ensure offset is word-aligned

    if (CURRENT_STATE.REGS[rt] != 0)
    {
        NEXT_STATE.PC = CURRENT_STATE.PC + offset;
        printf("[CBNZ] X%d != 0, Branch to 0x%lx\n", rt, NEXT_STATE.PC);
    }
    else
    {
        NEXT_STATE.PC += 4;
        printf("[CBNZ] X%d == 0, No branch\n", rt);
    }
}

static void handle_add_imm(uint32_t instruction)
{
    // Extraer campos
    uint8_t rd = extract_field(instruction, RD_MASK, 0);                  // Registro destino (Xd)
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);           // Registro base (Xn)
    uint64_t imm12 = extract_field(instruction, IMM12_MASK, IMM12_SHIFT); // Inmediato de 12 bits
    uint8_t sh = extract_field(instruction, SHIFT1_MASK, SHIFT1_SHIFT);   // Bit de shift (0 o 1)

    int datasize = 64;

    uint64_t imm;
    if (sh == 0)
    {
        imm = imm12; // Sin desplazamiento
    }
    else
    {
        imm = imm12 << 12; // Desplazado 12 bits a la izquierda
    }

    // Obtener operando 1 (Xn o SP)
    uint64_t operand1;
    operand1 = CURRENT_STATE.REGS[rn];

    // Operando 2: Extender el inmediato a datasize
    uint64_t operand2 = imm;

    // Realizar la suma
    uint64_t result = operand1 + operand2;

    // Almacenar el resultado
    NEXT_STATE.REGS[rd] = result;

    // Avanzar el PC
    NEXT_STATE.PC += 4;

    // Impresión de depuración
    printf("[ADD] %s%d, %s%d, #0x%lx (sh=%d) => 0x%016lx\n",
           (rd == 31) ? "SP" : "X", rd,
           (rn == 31) ? "SP" : "X", rn,
           imm, sh, result);
}

static void handle_add_ext(uint32_t instruction)
{
    // Extraer campos
    uint8_t rd = extract_field(instruction, RD_MASK, 0);          // Registro destino (Xd)
    uint8_t rn = extract_field(instruction, RN_MASK, RN_SHIFT);   // Registro base (Xn)
    uint8_t rm = extract_field(instruction, RM_MASK, RM_SHIFT);   // Registro a extender (Xm)
    uint8_t imm3 = extract_field(instruction, IMM3_MASK, 10);      // Shift (0-4)
    uint8_t option = extract_field(instruction, OPTION_MASK, 13); // Tipo de extensión
    uint8_t sf = 1;   // Extraer el valor de SF

    // Validar imm3 (shift)
    if (imm3 >= 5)
    {
        printf("[ADD EXT] Undefined instruction: imm3 = %d\n", imm3);
        NEXT_STATE.PC += 4;
        return;
    }
    uint8_t shift = imm3;

    printf("Shift: %d\n", shift);

    // Determinar tamaño de datos
    int datasize = 32 << sf; // 32 si sf = 0, 64 si sf = 1

    // Obtener operando 1 (Xn o SP)
    uint64_t operand1 = CURRENT_STATE.REGS[rn];
    if (datasize == 32)
        operand1 &= 0xFFFFFFFF;

    // Obtener operando 2 (extender Rm)
    uint64_t operand2 = CURRENT_STATE.REGS[rm];
    printf("Option: %d\n", option);

    operand2 = (shift == 0) ? operand2 : (operand2 << shift);
    printf("operand1: %lx, operand2: %lx\n", operand1, operand2); // Debugging
    if (datasize == 32)
        operand2 &= 0xFFFFFFFF;

    // Realizar la suma
    uint64_t result = operand1 + operand2;
    if (datasize == 32)
        result &= 0xFFFFFFFF;

    NEXT_STATE.REGS[rd] = result;

    // Avanzar el PC
    NEXT_STATE.PC += 4;

    // Impresión de depuración
    const char *extend_str[] = {"UXTB", "UXTH", "UXTW", "UXTX", "SXTB", "SXTH", "SXTW", "SXTX"};
    printf("[ADD EXT] %s%d, %s%d, X%d, %s #%d (sf=%d) => 0x%016lx\n",
           (rd == 31) ? "SP" : "X", rd,
           (rn == 31) ? "SP" : "X", rn,
           rm, extend_str[option], shift, sf, result);
}


/*
    HELPER FUNCTIONS
*/

static inline uint32_t extract_field(uint32_t instruction, uint32_t mask, uint8_t shift)
{
    return (instruction & mask) >> shift;
}

static uint64_t apply_shift(uint64_t value, ShiftType type, uint32_t amount)
{
    if (amount == 0)
        return value; // No shift needed

    switch (type)
    {
    case SHIFT_LSL:
        return value << amount;
    case SHIFT_LSR:
        return value >> amount;
    case SHIFT_ASR:
        return (int64_t)value >> amount;
    case SHIFT_ROR:
        return (value >> amount) | (value << (64 - amount));
    default:
        assert(0 && "Invalid shift type");
        return value;
    }
}

static void set_flags_z_n(CPU_State *state, uint64_t result)
{
    state->FLAG_Z = (result == 0) ? 1 : 0;
    state->FLAG_N = (result >> 63) & 1; // Usamos el bit más significativo
}

static const char *shift_type_to_str(ShiftType type)
{
    static const char *names[] = {"LSL", "LSR", "ASR", "ROR"};
    return names[type];
}

static bool check_condition(uint8_t cond)
{
    switch (cond)
    {
    case 0x0: // BEQ
        return CURRENT_STATE.FLAG_Z == 1;
    case 0x1: // BNE
        return CURRENT_STATE.FLAG_Z == 0;
    case 0x2: // BCS
        return FLAG_C == 1;
    case 0x3: // BCC
        return FLAG_C == 0;
    case 0x4: // BMI
        return CURRENT_STATE.FLAG_N == 1;
    case 0x5: // BPL
        return CURRENT_STATE.FLAG_N == 0;
    case 0x6: // BVS
        return FLAG_V == 1;
    case 0x7: // BVC
        return FLAG_V == 0;
    case 0x8: // BHI
        return (FLAG_C == 1 && CURRENT_STATE.FLAG_Z == 0);
    case 0x9: // BLS
        return !(FLAG_C == 1 && CURRENT_STATE.FLAG_Z == 0);
    case 0xA: // BGE
        return (CURRENT_STATE.FLAG_N == FLAG_V);
    case 0xB: // BLT
        return (CURRENT_STATE.FLAG_N != FLAG_V);
    case 0xC: // BGT
        return (CURRENT_STATE.FLAG_Z == 0 && (CURRENT_STATE.FLAG_N == FLAG_V));
    case 0xD: // BLE
        return (CURRENT_STATE.FLAG_Z == 0 && (CURRENT_STATE.FLAG_N != FLAG_V));
    case 0xE: // Undefined
        return false;
    case 0xF: // Always
        return true;

    default:
        assert(0 && "Invalid condition code");
        return false;
    }
}