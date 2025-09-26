# Projeto 1: Software de Processamento de Imagens (SDL3)

### Descrição
Projeto desenvolvido em linguagem C utilizando as bibliotecas `SDL3`, `SDL3_image` e `SDL3_ttf` para processamento básico de imagens.

### Membros do projeto:

- Arthur Cezar da Silveira Lima - RA:10409172
- Gabriel Morgado Nogueira - RA:10409493
- Guillermo Kuznietz - RA:10410134
- Marcos Arambasic Rebelo da Silva - RA:10443260

### Contribuições do grupo
**Arthur** 
- Implementação da interface gráfica de usuário (GUI) com duas janelas
- Equalização do histograma  
- Tratamento de erros

**Gabriel**
- Funcionalidade de salvamento da imagem, incluindo ao apertar a tecla `S`
- Análise e exibição do histograma
- Limpeza e manutenção do código
- Testes finais de funcionamento

**Guillermo**
- Criação inicial do repositório e estrutura inicial
- Leitura e salvamento da imagem
- Verificação e conversão da imagem para escala de cinza
- Análise e exibição do histograma

**Marcos**
- Organização do README
- Implementação do botão interativo.
- Implementação da biblioteca `SDL_ttf` e exibição dos textos

## Processo de funcionamento do projeto
O programa começa carregando a imagem, permitindo nos formatos PNG, JPG e BMP. Em sequência, verifica e converte a imagem para escala de cinza, caso não esteja.

Gera duas janelas de exibição, uma contendo a imagem na escala de cinza, e o outro contendo o histograma calculado da imagem, mostrando estatísticas de intensidade e contraste. 

Possui também o botão de equalização do histograma, permitindo salvar a imagem ao pressionar a tecla `S`.


## Estrutura do projeto
O projeto segue a seguinte estrutura:
```
software-de-processamento-de-imagens/
│── libs/
│   └── SDL3/
│       ├── include/
│       └── lib/
|       └── bin/
│── main.c
│── SDL3.dll
│── SDL3_image.dll
│── SDL3_ttf.dll
│── README.md
│── makefile
│── OpenSans-Regular.ttf
```

### Pré-requisitos

- **GCC** instalado e configurado no PATH.
- Imagem de entrada (exemplo: `teste.png`) salva no diretório do projeto.

### Como compilar e executar

1. Abra o terminal na pasta do projeto.
2. Compile o programa com:

```bash
gcc main.c -IC:{caminho_do_projeto}\software-de-processamento-de-imagens\libs\SDL3\include -o executavel -LC:{caminho_do_projeto}\software-de-processamento-de-imagens\libs\SDL3\lib -lSDL3 -lSDL3_image -lSDL3_ttf
```


**Aviso 1:** Substitua `{caminho_do_projeto}` pelo diretório completo onde você salvou o projeto.

**Aviso 2:** Salve a imagem que realizará o teste no diretório completo onde você salvou o projeto.

**Dica:** Caso tenha MinGW instalado, tente compilar usando `mingw32-make`

3. Execute o programa passando a imagem de entrada:

Exemplo:
```
./executavel caminho-para-imagem.png
```
