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

#define MODO_ALEATORIO 1
#define MODO_MANUAL    2
#define MODO_PADRAO    3

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
        printf("\n===========================================\n");
        printf("Deseja realizar uma multiplicação de matrizes? (S/N): ");
        scanf(" %c", &opcao);

        int comando;
        if (opcao == 'S' || opcao == 's') {
            comando = CMD_CALCULATE;
            write(sockfd, &comando, sizeof(int));

            // Menu de escolha do modo
            int modo = 0;
            printf("\nEscolha o modo de operação:\n");
            printf("  1 - Matrizes com valores aleatórios\n");
            printf("  2 - Digitar as matrizes manualmente\n");
            printf("  3 - Usar matrizes padrão (A e B = 10x10 com valor 2 em todas posições)\n");
            printf("Opção: ");
            scanf("%d", &modo);

            while (modo < 1 || modo > 3) {
                printf("Opção inválida. Digite 1, 2 ou 3: ");
                scanf("%d", &modo);
            }

            // Envia o modo escolhido para o servidor
            write(sockfd, &modo, sizeof(int));

            int n = 0;
            int A[MAX_DIM][MAX_DIM], B[MAX_DIM][MAX_DIM];

            if (modo == MODO_PADRAO) {
                // Modo padrão: matrizes 10x10 com valor 2
                n = 10;
                printf("\nUsando matrizes padrão 10x10 (todos elementos = 2)\n");
                
            } else if (modo == MODO_ALEATORIO) {
                // Modo aleatório: solicita apenas o tamanho
                while (n < MIN_DIM || n > MAX_DIM) {
                    printf("\nDigite o tamanho da matriz (entre %d e %d): ", MIN_DIM, MAX_DIM);
                    scanf("%d", &n);
                    if (n < MIN_DIM || n > MAX_DIM) {
                        printf("Tamanho inválido.\n");
                    }
                }
                printf("Matrizes %dx%d serão geradas aleatoriamente pelo servidor.\n", n, n);
                
            } else if (modo == MODO_MANUAL) {
                // Modo manual: solicita tamanho e elementos das matrizes
                while (n < MIN_DIM || n > MAX_DIM) {
                    printf("\nDigite o tamanho da matriz (entre %d e %d): ", MIN_DIM, MAX_DIM);
                    scanf("%d", &n);
                    if (n < MIN_DIM || n > MAX_DIM) {
                        printf("Tamanho inválido.\n");
                    }
                }

                printf("\n--- Digite os elementos da Matriz A (%dx%d) ---\n", n, n);
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        printf("A[%d][%d]: ", i, j);
                        scanf("%d", &A[i][j]);
                    }
                }

                printf("\n--- Digite os elementos da Matriz B (%dx%d) ---\n", n, n);
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        printf("B[%d][%d]: ", i, j);
                        scanf("%d", &B[i][j]);
                    }
                }
            }

            // Envia o tamanho da matriz
            write(sockfd, &n, sizeof(int));

            // Se modo manual, envia as matrizes A e B
            if (modo == MODO_MANUAL) {
                write(sockfd, A, sizeof(int) * n * n);
                write(sockfd, B, sizeof(int) * n * n);
            }

            // Recebe a matriz resultante C
            int C[MAX_DIM][MAX_DIM];
            printf("\nAguardando resultado do servidor...\n");
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
                printf("\n=== Matriz Resultante C (%dx%d) ===\n", n, n);
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        printf("%6d", C[i][j]);
                    }
                    printf("\n");
                }
                printf("===========================================\n");
            } else {
                continuar = false;
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