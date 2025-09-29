#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdbool.h>

#define CMD_QUIT     0
#define CMD_CALCULATE 1
#define MIN_DIM      10  
#define MAX_DIM      100

// Função para reportar erros e terminar o programa
void erro(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <host> <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) erro("Erro ao criar socket");

    struct hostent *server = gethostbyname(argv[1]);
    if (!server) {
        fprintf(stderr, "Host não encontrado\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        erro("Erro ao conectar");

    printf("Conectado ao servidor %s:%s\n", argv[1], argv[2]);

    bool continuar = true;
    while (continuar) {
        char opcao;
        printf("\nDeseja realizar uma multiplicação de matrizes? (S/N): ");
        scanf(" %c", &opcao);

        int comando;
        if (opcao == 'S' || opcao == 's') {
            comando = CMD_CALCULATE;
            write(sockfd, &comando, sizeof(int));

            int n = 0;
            // Valida a entrada do usuário no lado do cliente
            while (n < MIN_DIM || n > MAX_DIM) {
                printf("Digite o tamanho da matriz (entre %d e %d): ", MIN_DIM, MAX_DIM);
                scanf("%d", &n);
                if (n < MIN_DIM || n > MAX_DIM) {
                    printf("Tamanho inválido.\n");
                }
            }
            write(sockfd, &n, sizeof(int));

            int C[MAX_DIM][MAX_DIM];

            // Recebe a matriz resultante, linha por linha, para garantir a integridade da estrutura de dados
            printf("Aguardando resultado do servidor...\n");
            bool erro_recebimento = false;
            for (int i = 0; i < n && !erro_recebimento; i++) {
                int bytes_linha = sizeof(int) * n;
                int bytes_recebidos = 0;
                char *ptr_linha = (char *)C[i];

                while (bytes_recebidos < bytes_linha && !erro_recebimento) {
                    int b = read(sockfd, ptr_linha + bytes_recebidos, bytes_linha - bytes_recebidos);
                    if (b <= 0) {
                        fprintf(stderr, "Erro: O servidor encerrou a conexão durante a transmissão.\n");
                        erro_recebimento = true;
                    } else {
                        bytes_recebidos += b;
                    }
                }
            }

            if (!erro_recebimento) {
                printf("\nMatriz resultante C (%dx%d):\n", n, n);
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        printf("%6d", C[i][j]);
                    }
                    printf("\n");
                }
            } else {
                continuar = false; // Se houve erro, encerra o cliente
            }

        } else {
            comando = CMD_QUIT;
            write(sockfd, &comando, sizeof(int));
            printf("Encerrando conexão.\n");
            continuar = false;
        }
    }

    close(sockfd);
    return 0;
}