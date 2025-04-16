.text
    MOVZ X0, #0                 // X0 = 0
    MOVZ X1, #1                 // X1 = 1
    CBZ X0, zero_label          // Saltar si X0 == 0
    ADD X2, X1, #1              // No debería ejecutarse si salta

zero_label:
    CBNZ X0, nonzero_label      // No saltar si X0 == 0
    ADD X3, X1, #2              // Debería ejecutarse

nonzero_label:
    ADDS X4, X0, X0             // Poner flag Z en 1
    B.EQ eq_label               // Saltar si Z == 1

    ADD X5, X1, #3              // No debería ejecutarse

eq_label:
    // B incondicional
    B end_label                 // Saltar al final

    ADD X6, X1, #4              // No debería ejecutarse

end_label:
    HLT 0