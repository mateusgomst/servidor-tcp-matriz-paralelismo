#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

#define CMD_QUIT     0
#define CMD_CALCULATE 1
#define MAX_DIM      100

#define MODO_ALEATORIO 1
#define MODO_MANUAL    2
#define MODO_PADRAO    3

// Estrutura para retornar o resultado de um processo filho via pipe
typedef struct {
    int i;
    int j;
    int valor;
} ResultadoElemento;

// Função para reportar erros e terminar o programa
void erro(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// Função executada por cada processo filho para calcular um único elemento C[i][j]
void calculaElemento(int n, int A[MAX_DIM][MAX_DIM], int B[MAX_DIM][MAX_DIM], int i, int j, int pipe_fd[2]) {
    close(pipe_fd[0]); // O filho não lê do pipe, apenas escreve

    ResultadoElemento res;
    res.i = i;
    res.j = j;
    res.valor = 0;

    for (int k = 0; k < n; k++) {
        res.valor += A[i][k] * B[k][j];
    }

    // Envia o resultado (i, j, valor) para o processo pai através do pipe
    write(pipe_fd[1], &res, sizeof(ResultadoElemento));
    close(pipe_fd[1]);
    exit(EXIT_SUCCESS);
}

// Função principal para gerenciar a comunicação com um cliente conectado
void atenderCliente(int socket_cliente, struct sockaddr_in cli_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &cli_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(cli_addr.sin_port);

    // Arrays para rastrear processos filhos ativos
    pid_t processos_ativos[MAX_DIM * MAX_DIM];
    int num_processos_ativos = 0;

    printf("[LOG] Cliente conectado: %s:%d\n", client_ip, client_port);

    bool continuar_conexao = true;
    while (continuar_conexao) {
        int comando;
        int bytes_lidos = read(socket_cliente, &comando, sizeof(int));

        if (bytes_lidos <= 0) {
            printf("[LOG] Cliente %s:%d desconectado abruptamente.\n", client_ip, client_port);
            // Encerra processos filhos se cliente desconectou abruptamente
            if (num_processos_ativos > 0) {
                printf("[LOG] Encerrando %d processos órfãos devido à desconexão abrupta\n", num_processos_ativos);
                for (int i = 0; i < num_processos_ativos; i++) {
                    kill(processos_ativos[i], SIGTERM);
                }
            }
            continuar_conexao = false;
        } else {
            if (comando == CMD_QUIT) {
                printf("[LOG] Cliente %s:%d solicitou encerramento.\n", client_ip, client_port);
                continuar_conexao = false;
            } else if (comando == CMD_CALCULATE) {
                int modo;
                read(socket_cliente, &modo, sizeof(int));

                int n;
                read(socket_cliente, &n, sizeof(int));

                int A[MAX_DIM][MAX_DIM], B[MAX_DIM][MAX_DIM], C[MAX_DIM][MAX_DIM];

                if (modo == MODO_ALEATORIO) {
                    printf("[LOG] Cliente %s:%d - Modo ALEATÓRIO - Matriz %dx%d\n", client_ip, client_port, n, n);
                    // Gera matrizes A e B com valores aleatórios
                    srand(time(NULL) ^ getpid());
                    for (int i = 0; i < n; i++) {
                        for (int j = 0; j < n; j++) {
                            A[i][j] = rand() % 9 + 1;
                            B[i][j] = rand() % 9 + 1;
                        }
                    }
                } else if (modo == MODO_MANUAL) {
                    printf("[LOG] Cliente %s:%d - Modo MANUAL - Matriz %dx%d\n", client_ip, client_port, n, n);
                    // Recebe as matrizes A e B do cliente
                    read(socket_cliente, A, sizeof(int) * n * n);
                    read(socket_cliente, B, sizeof(int) * n * n);
                } else if (modo == MODO_PADRAO) {
                    printf("[LOG] Cliente %s:%d - Modo PADRÃO - Matriz 10x10 (valor 2)\n", client_ip, client_port);
                    // Matrizes padrão: 10x10 com todos elementos = 2
                    for (int i = 0; i < n; i++) {
                        for (int j = 0; j < n; j++) {
                            A[i][j] = 2;
                            B[i][j] = 2;
                        }
                    }
                }

                pid_t pids[MAX_DIM * MAX_DIM];
                int pipes[MAX_DIM * MAX_DIM][2];
                int total_processos_criados = 0;

                // Limpa lista de processos ativos antes de criar novos
                num_processos_ativos = 0;

                // Cria um processo filho para calcular cada elemento de C
                for (int i = 0; i < n; i++) {
                    for (int j = 0; j < n; j++) {
                        if (pipe(pipes[total_processos_criados]) < 0) continue;

                        pid_t pid = fork();
                        if (pid == 0) { // Processo filho
                            calculaElemento(n, A, B, i, j, pipes[total_processos_criados]);
                        } else if (pid > 0) { // Processo pai
                            pids[total_processos_criados] = pid;
                            processos_ativos[num_processos_ativos] = pid;
                            num_processos_ativos++;
                            close(pipes[total_processos_criados][1]); // Pai não escreve
                            total_processos_criados++;
                        }
                    }
                }
                
                printf("[LOG] Cliente %s:%d - Criados %d processos para o cálculo.\n", client_ip, client_port, total_processos_criados);

                // Coleta os resultados dos filhos via pipes
                for (int i = 0; i < total_processos_criados; i++) {
                    ResultadoElemento res;
                    read(pipes[i][0], &res, sizeof(ResultadoElemento));
                    C[res.i][res.j] = res.valor;
                    close(pipes[i][0]);
                    waitpid(pids[i], NULL, 0); // Aguarda o término do filho
                }
                
                // Limpa lista de processos ativos após conclusão
                num_processos_ativos = 0;
                
                printf("[LOG] Cliente %s:%d - Cálculo concluído. Enviando resultado.\n", client_ip, client_port);

                // Envia a matriz resultante C para o cliente, linha por linha
                bool erro_envio = false;
                for (int i = 0; i < n && !erro_envio; i++) {
                    int bytes_linha = sizeof(int) * n;
                    if (write(socket_cliente, C[i], bytes_linha) != bytes_linha) {
                        fprintf(stderr, "[ERRO] Falha ao enviar linha %d para o cliente %s:%d\n", i, client_ip, client_port);
                        erro_envio = true;
                    }
                }
            }
        }
    }
    
    // Antes de encerrar, mata todos os processos filhos ativos (se houver)
    if (num_processos_ativos > 0) {
        printf("[LOG] Encerrando %d processos filhos ativos do cliente %s:%d\n", 
               num_processos_ativos, client_ip, client_port);
        for (int i = 0; i < num_processos_ativos; i++) {
            kill(processos_ativos[i], SIGTERM);
            waitpid(processos_ativos[i], NULL, WNOHANG); // Não bloqueia se já terminou
        }
    }
    
    close(socket_cliente);
    printf("[LOG] Conexão com %s:%d encerrada.\n", client_ip, client_port);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) erro("Erro ao criar socket");

    // Permite reutilizar o endereço da porta imediatamente após o servidor fechar
    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        erro("Erro no bind");

    listen(sockfd, 5);
    printf("Servidor TCP aguardando conexões na porta %s...\n", argv[1]);

    // O sinal SIGCHLD é enviado quando um processo filho termina.
    // SIG_IGN instrui o sistema a ignorar o sinal, o que efetivamente
    // faz com que os processos filhos "zumbis" sejam limpos automaticamente.
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            perror("Erro no accept");
            continue;
        }

        // Cria um processo filho para lidar com o novo cliente.
        // O processo pai volta imediatamente a esperar por novas conexões.
        pid_t pid_cliente = fork();
        if (pid_cliente == 0) { // Processo filho do cliente
            close(sockfd); // O filho não precisa do socket de escuta
            atenderCliente(newsockfd, cli_addr);
        } else if (pid_cliente > 0) { // Processo pai
            close(newsockfd); // O pai não precisa do socket de comunicação
        }
    }

    close(sockfd);
    return 0;
}