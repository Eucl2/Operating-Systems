# **Trabalho Prático de Sistemas Operativos** 

## **Título:** Orquestrador de Tarefas 

* **Objetivo:** Demonstrar a compreensão dos conceitos de comunicação entre processos, escalonamento de tarefas e gerenciamento de recursos em sistemas operacionais.

* **Sistema Operativo:** Linux 
* **Linguagem:** C



Mais informações no enunciado do projeto

--------------------------------------------------------------------------------------------------------------------------------------------


## **TO DO**

#### Erros:

* ./client sem args => segmentation fault


#### Falta:

* Orchestrator tem que receber os argumentos
* Mostrar id da tarefa após enviar o pedido
* Pipelines no orchestrator
* Implementar sjf - comparar resultados com o FCFS
* Orchestrator nao bloquear quando recebe 2 progs
* Orchs nao pode morrer


#### Dúvidas:
* Se fifo nao existir, devemos criá-lo? ou é desnecessário?
* Localizacao dos tests, alterar ou deixar?
* Posso usar realloc nas pipelines? -> sim
* Como fechar o orquestrador quando já não quiser usar mais? será que mandar um pedido do cliente ./client shutdown seria aceitável?

