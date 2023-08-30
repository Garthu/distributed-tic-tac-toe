Compilação:
gcc -o server server.c
gcc -o client client.c

Para a execução, o servidor e cada um dos Clientes devem ser executados em terminais distintos:
./server
./client

Dentro do server não existem funções a serem escritas pelo usuário, apenas dentro do Cliente,
estas são:

register
scores
match
exit

Todas as funções devem ser escritas sozinhas e então deve-se pressionar o enter, caso elas precisem
de um complemento, este deve ser adicionado na próxima linha, como o caso do register

Exemplo de escrita:
register
Samuel

OBSERVAÇÃO: Na linha 3 do client.c deve ser completada a definicação macro SERVER_IPV6 para que a
definição correspondente ao endereço ipv6 da máquina onde está sendo executada o servidor
