# **Trabalho Prático de Sistemas Operativos** 

## **Título:** Orquestrador de Tarefas 

* **Objetivo:** Demonstrar a compreensão dos conceitos de comunicação entre processos, escalonamento de tarefas e gerenciamento de recursos em sistemas operacionais.

* **Sistema Operativo:** Linux 
* **Linguagem:** C



Mais informações no enunciado do projeto

--------------------------------------------------------------------------------------------------------------------------------------------


## **TO DO**

#### Erros:

* ❗Exec está a usar a shell! Alterar! ✔️
* Status não mostra o tempo nas tasks completed. -> acontece porque o tempo está apenas a ser calculado no processo filho
* Se o primeiro pedido for um execute, o comando é recebido de forma desformatada ✔️

* ./client sem args => segmentation fault ✔️
* Status vai mostrar o tempo de inicio em vez de mostrar o tempo que demorou a executar.  ✔️ -> Added to struct
* Os ficheiros dos projetos executados devem ser enviados para uma pasta vinda do input. ✔️ -> pasta de testes: file_saver
* Quando são enviados 2 pedidos de clientes ao mesmo tempo, o id só é devolvido ao cliente do segundo pedido após o pedido do primeiro cliente terminar ✔️
* Por vezes o input é recebido de forma desformatada no servidor ✔️
* Status não é recebido corretamente no cliente, ajustar! ✔️
* Status das tasks não está a ser atualizado corretamente ✔️
* Resposta do status não deve conter a palavra terminal "END". ✔️
* FCFS é na verdade LIFO. Rever código e atualizar ✔️


#### Falta:

* Pipelines no orchestrator ✔️

* Implementar sjf - comparar resultados com o FCFS ✔️ -> implementado. Comparar no relatorio os resultados que observamos
* Orchestrator tem que receber os argumentos ✔️ -> parallel tasks e sched_policy não estão a ser usadas
* Mostrar id da tarefa após enviar o pedido ✔️
* Orchestrator nao bloquear quando recebe 2 progs ✔️
* Orchs nao pode morrer ✔️
* Enviar stdout para txt com o id da tarefa ✔️


#### Dúvidas:
* Se fifo nao existir, devemos criá-lo? ou é desnecessário? -> criar : TODO!
* Ficheiros temporários devem ir para o tmp? e devem ser apagados no final?

* <s>Localizacao dos tests, alterar ou deixar?</s>
* <s>Posso usar realloc nas pipelines?</s> -> sim
* <s>Como fechar o orquestrador quando já não quiser usar mais? será que mandar um pedido do cliente ./client shutdown seria aceitável?</s> -> "é completamente aceitável". -> Implementado ✔️
* <s>Proibido usar printf para mostrar ao cliente?</s> -> Deve ser usado write com a função sprintf em conjunção. -> Feito ✔️
