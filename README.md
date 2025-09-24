# Software de Processamento de Imagens (SDL3 em C)

Projeto desenvolvido em C utilizando a biblioteca **SDL3** e **SDL3_image** para processamento básico de imagens.

Membros do projeto:

- Arthur Cezar da Silveira Lima - RA:10409172
- Gabriel Morgado Nogueira - RA:10409493
- Guillermo Kuznietz - RA:10410134
- Marcos Arambasic Rebelo da Silva - RA:10443260

## Estrutura do projeto
O projeto segue a seguinte estrutura:
```
software-de-processamento-de-imagens/
│── libs/
│   └── SDL3/
│       ├── include/
│       └── lib/
│── main.c
│── teste.png
│── SDL3.dll
│── SDL3_image.dll
│── README.md
```

## Pré-requisitos

- **GCC** instalado e configurado no PATH.
- Imagem de entrada (exemplo: `teste.png`) salva no diretório do projeto.

## Como compilar e executar

1. Abra o terminal na pasta do projeto.
2. Compile o programa com:

```bash
gcc main.c -IC:{caminho_do_projeto}\software-de-processamento-de-imagens\libs\SDL3\include -o executavel -LC:{caminho_do_projeto}\software-de-processamento-de-imagens\libs\SDL3\lib -lSDL3 -lSDL3_image
```
> ⚠️ **Aviso:** Substitua `{caminho_do_projeto}` pelo diretório completo onde você salvou o projeto.


3. Execute o programa passando a imagem de entrada:
```
./executavel teste.png
```
