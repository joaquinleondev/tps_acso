.text
    // Registro XZR (31) en SUBS
    SUBS XZR, X0, X1            // CMP, no escribir en X31

    // Offset máximo en STUR
    MOVZ X0, #0x1000            // Base
    STUR X1, [X0, #255]       // Offset máximo positivo (9 bits sign-extended)

    // Offset negativo en LDUR
    LDUR X2, [X0, #-256]        // Offset negativo máximo

    // Valor máximo en MOVZ
    MOVZ X3, #0xFFFF

    // Multiplicación grande
    MUL X4, X3, X3              // X4 = X3 * X3, probar overflow

    // BCOND con condición siempre falsa
    B.NE never_label            // No saltar
    ADD X5, X0, #1              // Debería ejecutarse

never_label:
    HLT 0                        // Fin del programa