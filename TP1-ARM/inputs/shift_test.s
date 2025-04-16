.text
    MOVZ X0, #0x8000            // X0 = 0x8000

    // LSL
    LSL X1, X0, #4              // X1 = X0 << 4 = 0x80000

    // LSR
    LSR X2, X0, #2              // X2 = X0 >> 2 = 0x2000

    // LSL con valor grande
    LSL X4, X3, #8              // X4 = X3 << 8 = 0x100000000000000

    HLT 0                        // Fin del programa