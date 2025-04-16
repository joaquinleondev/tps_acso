.text
    // Inicializar registros
    MOVZ X0, #0x1234            // X0 = 0x1234
    MOVZ X1, #0x5678            // X1 = 0x5678

    // ADDS con registros
    ADDS X2, X0, X1             // X2 = X0 + X1 = 0x68AC, probar flags Z y N

    // SUBS con registros
    SUBS X3, X1, X0             // X3 = X1 - X0 = 0x4444, probar flags

    // MUL
    MUL X4, X0, X1              // X4 = X0 * X1 = 0x626FB60

    // ADDS con inmediato
    ADDS X5, X0, #0x100         // X5 = X0 + 0x100 = 0x1334


    // ADD con inmediato
    ADD X7, X0, #0x1000         // X7 = X0 + 0x1000 = 0x2234

    // ANDS con shift
    ANDS X9, X0, X1     // X9 = X0 & X1 , probar flags

    HLT 0                        // Fin del programa