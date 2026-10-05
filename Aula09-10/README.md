# Aula 09-10: Bucket Sort

O projeto contém uma versão sequencial (`bucket_sort.c`) e uma versão com pthreads (`thread_bucket_sort.c`). Ambas leem um arquivo no formato fornecido para o problema 1: `N` na primeira linha e, em seguida, exatamente `N` inteiros. O bucket sort aceita valores inteiros negativos, positivos e repetidos.

## Compilar (WSL/Linux)

Execute a partir desta pasta:

```bash
gcc -Wall -Wextra -O2 bucket_sort.c -o bucket_sort
gcc -Wall -Wextra -O2 thread_bucket_sort.c -o thread_bucket_sort -pthread
```

Use a mesma opção de otimização para as duas versões.

### Executar pelo VS Code no Windows

Este projeto usa `pthread` e `sysconf`, então deve ser aberto e compilado em Linux/WSL, não com um compilador Windows nativo. No terminal da sua distribuição Ubuntu no WSL, instale as ferramentas uma vez:

```bash
sudo apt update
sudo apt install build-essential gdb
```

Depois, entre nesta pasta dentro do WSL e abra o VS Code conectado ao WSL:

```bash
cd "/mnt/c/Programação/Atividade Sistemas Operacionais/SistemasOperacionais/Aula09-10"
code .
```

Instale a extensão **C/C++** da Microsoft no ambiente WSL se o VS Code solicitar. Pressione `F5` e escolha uma configuração `Bucket Sort ...` na lista. O VS Code compila o programa antes de executá-lo. Também é possível compilar com `Ctrl+Shift+B` escolhendo a tarefa desejada.

## Executar

Execute os comandos a partir da pasta `Aula09-10`. A versão sequencial representa **1 fluxo de execução**. Para comparar corretamente as configurações, use o mesmo arquivo de entrada em todas as execuções:

```bash
# Sequencial: 1 fluxo
./bucket_sort entradas/pequena.txt

# Paralelo: 2, 4, 8 e máximo de CPUs lógicas online
./thread_bucket_sort entradas/pequena.txt 2
./thread_bucket_sort entradas/pequena.txt 4
./thread_bucket_sort entradas/pequena.txt 8
./thread_bucket_sort entradas/pequena.txt max
```

Repita o conjunto substituindo `pequena.txt` por `media.txt` e depois por `grande.txt`. Assim, para cada tamanho, a versão sequencial e todas as configurações paralelas recebem exatamente os mesmos dados. O segundo argumento da versão paralela define o número de threads; `max` consulta `sysconf(_SC_NPROCESSORS_ONLN)`. Se omitido, o padrão também é `max`. Os caminhos relativos são resolvidos a partir desta pasta. Sem caminho informado, ambos usam `entradas/pequena.txt`.

A versão paralela mede também a busca do menor/maior valor, a criação e junção das threads e a coleta dos baldes. Ela compara o resultado com uma ordenação sequencial e encerra com erro se houver divergência. Essa validação sequencial ocorre fora do intervalo cronometrado.

## Comparação experimental

Para cada uma das três entradas, execute cada configuração pelo menos três vezes e registre cada tempo individual. Calcule a média e, para cada configuração paralela, `speedup = média_sequencial / média_paralela` e `eficiência = speedup / número_de_threads`. Registre também a quantidade de CPUs lógicas, o compilador, as opções usadas e as características da máquina. Preserve resultados em que aumentar as threads piorar o tempo. Mesmo que `max` seja igual a 2, 4 ou 8, execute e registre essa configuração como o máximo online observado.

Os arquivos `entradas/` devem corresponder às entradas de Bucket Sort do material do professor ou a entradas próprias documentadas com os mesmos tamanhos. Use exatamente o mesmo arquivo nas execuções sequencial e paralela.


