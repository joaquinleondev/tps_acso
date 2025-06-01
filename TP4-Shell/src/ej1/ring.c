#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "Uso: %s <n_hijos> <valor_inicial> <hijo_inicial_1_indexado>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int n_hijos = atoi(argv[1]);
	int valor_inicial = atoi(argv[2]);
	int hijo_inicial_1_idx = atoi(argv[3]);

	if (n_hijos < 3)
	{
		fprintf(stderr, "El número de procesos para formar un anillo debe ser al menos 3.\n");
		return EXIT_FAILURE;
	}
	if (hijo_inicial_1_idx <= 0 || hijo_inicial_1_idx > n_hijos)
	{
		fprintf(stderr, "El índice del hijo inicial está fuera de rango (1 a %d).\n", n_hijos);
		return EXIT_FAILURE;
	}
	int s0_actual_idx = hijo_inicial_1_idx - 1;

	int all_pipes[n_hijos + 1][2];

	for (int k = 0; k <= n_hijos; k++)
	{
		if (pipe(all_pipes[k]) == -1)
		{
			perror("pipe");
			return EXIT_FAILURE;
		}
	}

	pid_t pids[n_hijos];

	for (int i = 0; i < n_hijos; i++)
	{
		pids[i] = fork();
		if (pids[i] == -1)
		{
			perror("fork");
			return EXIT_FAILURE;
		}

		if (pids[i] == 0)
		{
			int k_seq = (i - s0_actual_idx + n_hijos) % n_hijos;

			int pipe_idx_lectura = k_seq;
			int pipe_idx_escritura = k_seq + 1;

			for (int p_idx = 0; p_idx <= n_hijos; p_idx++)
			{
				if (p_idx == pipe_idx_lectura)
				{
					close(all_pipes[p_idx][1]);
				}
				else if (p_idx == pipe_idx_escritura)
				{
					close(all_pipes[p_idx][0]);
				}
				else
				{
					close(all_pipes[p_idx][0]);
					close(all_pipes[p_idx][1]);
				}
			}

			int mensaje;
			if (read(all_pipes[pipe_idx_lectura][0], &mensaje, sizeof(mensaje)) != sizeof(mensaje))
			{
				perror("hijo read");
				close(all_pipes[pipe_idx_lectura][0]);
				close(all_pipes[pipe_idx_escritura][1]);
				exit(EXIT_FAILURE);
			}

			mensaje++;

			if (write(all_pipes[pipe_idx_escritura][1], &mensaje, sizeof(mensaje)) != sizeof(mensaje))
			{
				perror("hijo write");
				close(all_pipes[pipe_idx_lectura][0]);
				close(all_pipes[pipe_idx_escritura][1]);
				exit(EXIT_FAILURE);
			}

			close(all_pipes[pipe_idx_lectura][0]);
			close(all_pipes[pipe_idx_escritura][1]);
			exit(EXIT_SUCCESS);
		}
	}

	for (int k = 0; k <= n_hijos; k++)
	{
		if (k == 0)
		{
			close(all_pipes[k][0]);
		}
		else if (k == n_hijos)
		{
			close(all_pipes[k][1]);
		}
		else
		{
			close(all_pipes[k][0]);
			close(all_pipes[k][1]);
		}
	}

	if (write(all_pipes[0][1], &valor_inicial, sizeof(valor_inicial)) != sizeof(valor_inicial))
	{
		perror("padre write");
		return EXIT_FAILURE;
	}
	close(all_pipes[0][1]);
	int valor_final;
	if (read(all_pipes[n_hijos][0], &valor_final, sizeof(valor_final)) != sizeof(valor_final))
	{
		perror("padre read");
		return EXIT_FAILURE;
	}
	close(all_pipes[n_hijos][0]);

	printf("El resultado final es %d.\n", valor_final);

	for (int i = 0; i < n_hijos; i++)
	{
		waitpid(pids[i], NULL, 0);
	}

	return EXIT_SUCCESS;
}