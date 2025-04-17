Joaquín León Alderete (jleonalderete@udesa.edu.ar)

## Fase 1

**Qué hacía el código**: La función `phase_1` compara el input del usuario con una cadena específica almacenada en la dirección `0x4c9a58`. Si el input no coincide exactamente, la bomba explota.

**Cómo se resolvió**: El input correcto es la cadena `"Los hermanos sean unidos porque esa es la ley primera porque si entre ellos se pelean los devoran los de ajuera"`, obtenida inspeccionando la memoria en `0x4c9a58` y verificando que la función `strings_not_equal` espera una coincidencia exacta.

## Fase 2

**Qué hacía el código**: La función `phase_2` parsea el input en tres números enteros separados por espacios, realiza un XOR entre los dos primeros números, lo divide por 2, y compara el resultado con el tercer número. Además, el tercer número debe ser negativo para pasar la función `misterio`.

**Cómo se resolvió**: El input correcto es `1 -1 -1`. Se encontró que 1 XOR -1 = -2, y -2/2 = -1, que coincide con el tercer número. El tercer número -1 es negativo, satisfaciendo la función `misterio`. Cualquier otro trio de números que satisfaga esas dos condiciones:
- input `a b c` tal que `(a xor b) / 2 = c` y que `c < 0`   


## Fase 3

**Qué hacía el código**:
- Parseaba una cadena (`input_string`) y un entero (`num1`) desde la entrada con `sscanf` usando el formato `"%s %d"`.
- Llamaba a `readlines` para leer 10784 líneas de un archivo `palabras.txt`, que contenía palabras ordenadas lexicográficamente.
- Usaba la función `cuenta` para realizar una búsqueda binaria, comparando `input_string` con las palabras y contando las iteraciones.
- Verificaba que el contador de iteraciones fuera igual a `num1`, mayor que 6, y menor o igual a 11; de lo contrario, explotaba.

**Cómo se resolvió**:
- Se identificó que `num1` debía ser el número de iteraciones de la búsqueda binaria (7, 8, 9, 10, o 11).
- Con 10784 líneas, se calculó que la palabra en el índice 83 (línea 84) requería exactamente 7 iteraciones.
- Se extrajo la palabra en la línea 84 de `palabras.txt` usando `sed -n '84p' palabras.txt` (`aboquillar`).
- Se ingresó la entrada `aboquillar 7`, que pasó todas las verificaciones.

## Fase 4

**Qué hacía el código**:
- Verificaba que la entrada fuera una cadena de exactamente 6 caracteres mediante una llamada a `string_length`.
- Para cada carácter, calculaba un índice (`c_i & 0xf`) y sumaba el valor correspondiente del arreglo `array.0`.
- Requería que la suma de los 6 valores fuera 59 (`0x3b`); si no, explotaba.

**Cómo se resolvió**:
- Se inspeccionó el arreglo `array.0` en `gdb`, obteniendo los valores: `[2, 13, 7, 14, 5, 10, 6, 15, 1, 12, 3, 4, 11, 8, 16, 9]`.
- Se encontraron 6 índices `[1, 1, 9, 12, 15, 8]` cuya suma en `array.0` era `13 + 13 + 12 + 11 + 9 + 1 = 59`.
- Se construyó la cadena `AAILOH` con caracteres que producían esos índices (`'A'` para 1, `'I'` para 9, `'L'` para 12, `'O'` para 15, `'H'` para 8).
- Se ingresó `AAILOH`, desactivando la fase con éxito.

## Fase secreta

**Cómo acceder a ella**:
Analizando la función `phase_defused` pude observar que en el flujo de ejecución, chequeaba la cantidad de inputs. Cuando esa cantidad llegaba a 4, se hacia un `scanf`, que verificando lo que contenía la direccion que cargaban en `$rsi` pude observar que esperaba `%s %d %s`. Por lo que estaba esperando un input para parsear con el formato `string int string`. Luego comprobé el registro que contenia el segundo parámetro de la función, `$rdi` y pude observar el input que habia pasado en la fase 3, que además, era consistente, con el formato que tenía los dos primeros miembros del input. Luego se utilizaba la función `strings_not_equal` y viendo en la dirección del primer parametro, encontre que estaba intentando coincidir `"abrete_sesamo"`.

**Qué hacía el código**:
- Parseaba la entrada a un entero y verificaba que `entrada - 1 < 1001`.
- Utilizaba la entrada como input para una función `fun7` que realizaba una busqueda de árbol binaria comenzando con el nodo en la dirección `0x4f91f0`.
- La función a su vez, era recursiva y los casos base eran que el puntero al nodo sea null y retornaba `-1`, que el nodo contenga el valor que recibió por parametro la función y retornaba `0`, si buscaba para la izquierda multiplicaba por dos, y si buscaba en la derecha multiplicaba por dos y sumaba ´1´.
- Chequeaba que el retorno de esta función sea igual a `6`.

**Cómo se resolvió**:
- Para que la función retorne `6`, debia recorrer desde el nodo raíz, `root -> left -> right -> right` y hallar el valor.
- Con gdb seguí el camino nodo por nodo con las direcciones hasta dar con el nodo destino, donde verifiqué que su valor era de `35`.
- Se ingresó `35`, desactivando la fase con éxito.
