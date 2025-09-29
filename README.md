# Servidor Concorrente TCP - Multiplicação de Matrizes

Este projeto implementa um **servidor concorrente** utilizando **paralelismo de tarefas** para realizar multiplicação de matrizes através do protocolo TCP. O servidor cria uma tarefa (processo filho) para calcular cada elemento da matriz resultante, garantindo máximo paralelismo e eficiência.

## 📋 Características do Projeto

### Servidor Concorrente Multi-Tarefa
- **Socket TCP** com arquitetura cliente-servidor
- **Paralelismo por processos** usando `fork()` 
- **Uma tarefa por elemento**: Para matriz NxN, cria N² processos filhos
- **Comunicação via pipes** entre processos pai e filhos
- **Gerenciamento automático** de processos órfãos
- **Suporte a múltiplos clientes** simultâneos

### Protocolo da Aplicação
- **Mensagens de controle**: 
  - `CMD_CALCULATE (1)`: Solicita cálculo de matriz
  - `CMD_QUIT (0)`: Encerra conexão
- **Mensagens com dados**:
  - Dimensão da matriz (int)
  - Matriz resultante (linha por linha)
- **Tratamento de falhas**:
  - Validação de dimensões (10-100)
  - Detecção de desconexão abrupta
  - Limpeza automática de processos

### Algoritmo Concorrente
Para cada elemento C[i][j] da matriz resultante:
```
C[i][j] = Σ(k=0 até n-1) A[i][k] * B[k][j]
```
- **Processo filho independente** para cada elemento
- **Comunicação via pipe** para retornar resultado
- **Coleta sincronizada** dos resultados pelo processo pai

## 🚀 Como Executar

### 1. Compilação
```bash
# Compila ambos os programas
make

# Ou compile manualmente:
gcc servidor.c -o servidor
gcc cliente.c -o cliente
```

### 2. Executar o Servidor
```bash
./servidor <porta>

# Exemplo:
./servidor 3132
```

### 3. Executar o Cliente
```bash
./cliente <host> <porta>

# Exemplo:
./cliente 127.0.0.1 3132
# ou
./cliente localhost 3132
```

## 💡 Exemplo de Uso

### Servidor:
```
Servidor TCP aguardando conexões na porta 3132...
[LOG] Cliente conectado: 127.0.0.1:49548
[LOG] Cliente 127.0.0.1:49548 solicitou cálculo de matriz 10x10.
[LOG] Cliente 127.0.0.1:49548 - Criados 100 processos para o cálculo.
[LOG] Cliente 127.0.0.1:49548 - Cálculo concluído. Enviando resultado.
[LOG] Cliente 127.0.0.1:49548 solicitou encerramento.
[LOG] Conexão com 127.0.0.1:49548 encerrada.
```

### Cliente:
```
Conectado ao servidor 127.0.0.1:3132

Deseja realizar uma multiplicação de matrizes? (S/N): S
Digite o tamanho da matriz (entre 10 e 100): 10
Aguardando resultado do servidor...

Matriz resultante C (10x10):
   245   198   267   ...
   312   156   234   ...
   ...   ...   ...   ...

Deseja realizar uma multiplicação de matrizes? (S/N): N
Encerrando conexão.
```

## 🏗️ Arquitetura Técnica

### Fluxo do Servidor:
1. **Socket()** - Cria socket TCP
2. **Bind()** - Vincula à porta especificada  
3. **Listen()** - Define backlog de conexões
4. **Accept()** - Aguarda clientes (loop infinito)
5. **Fork()** - Cria processo filho para cada cliente
6. **Recv()** - Recebe mensagens de controle
7. **Serviço** - Multiplica matrizes (N² processos filhos)
8. **Send()** - Envia resultado ao cliente
9. **Close()** - Encerra conexão e processo do cliente

### Recursos Implementados:
-  Matriz mínima 10x10 (até 100x100)
-  Protocolo de aplicação definido
-  Mensagens de controle, operação e falhas
-  Mensagens com dados estruturados
-  Paralelismo real (uma tarefa por elemento)
-  Servidor multi-cliente concorrente
-  Gerenciamento robusto de processos
