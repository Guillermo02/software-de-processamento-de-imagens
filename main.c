#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

static int eh_cinza(Uint8 r, Uint8 g, Uint8 b) {
    return (r == g && g == b);
}

// Converte superfície para escala de cinza. Retornando nova superfície ou NULL em caso de erro.
SDL_Surface* converte_para_cinza(SDL_Surface *orig) {
    if (!orig) return NULL;

    // Criar nova surface com o mesmo formato e dimensões
    SDL_PixelFormat fmt = orig->format;
    SDL_Surface *cinza = SDL_CreateSurface(orig->w, orig->h, orig->format);
    if (!cinza) {
        fprintf(stderr, "Erro ao criar surface cinza: %s\n", SDL_GetError());
        return NULL;
    }

    // Travar surfaces se necessário
    if (SDL_MUSTLOCK(orig)) {
        if (!SDL_LockSurface(orig)) {
            fprintf(stderr, "Erro ao realizar lock na surface original: %s\n", SDL_GetError());
            SDL_DestroySurface(cinza);
            return NULL;
        }
    }
    if (SDL_MUSTLOCK(cinza)) {
        if (!SDL_LockSurface(cinza)) {
            fprintf(stderr, "Erro ao realizar lock na surface cinza: %s\n", SDL_GetError());
            if (SDL_MUSTLOCK(orig)) SDL_UnlockSurface(orig);
            SDL_DestroySurface(cinza);
            return NULL;
        }
    }

    // Loop em cada pixel
    for (int y = 0; y < orig->h; y++) {
        for (int x = 0; x < orig->w; x++) {
            Uint8 r, g, b, a;
            if (!SDL_ReadSurfacePixel(orig, x, y, &r, &g, &b, &a)) {
                fprintf(stderr, "Erro ao ler pixel (%d,%d): %s\n", x, y, SDL_GetError());
            }

            // Se já estiver em cinza, apenas copia
            Uint8 valor;
            if (eh_cinza(r, g, b)) {
                valor = r;  // r == g == b
            } else {
                double y_val = 0.2125 * r + 0.7154 * g + 0.0721 * b;
                if (y_val < 0) y_val = 0;
                if (y_val > 255) y_val = 255;
                valor = (Uint8)(y_val + 0.5);  // arredondamento para melhorar a precisão, mas da para seguir com truncamento simples
            }


            const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(cinza->format);
            int bpp = SDL_BYTESPERPIXEL(cinza->format);

            Uint32 pixel_cinza = SDL_MapRGBA(fmt, NULL, valor, valor, valor, a);
            Uint8 *p = (Uint8 *)cinza->pixels + y * cinza->pitch + x * bpp;

            switch (bpp) {
                case 1: 
                    *p = (Uint8)pixel_cinza; 
                    break;
                case 2: 
                    (Uint16)p = (Uint16)pixel_cinza; 
                    break;
                case 3:
                    if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                        p[0] = (pixel_cinza >> 16) & 0xFF;
                        p[1] = (pixel_cinza >> 8) & 0xFF;
                        p[2] = pixel_cinza & 0xFF;
                    } else {
                        p[0] = pixel_cinza & 0xFF;
                        p[1] = (pixel_cinza >> 8) & 0xFF;
                        p[2] = (pixel_cinza >> 16) & 0xFF;
                    }
                    break;
                case 4: 
                    *(Uint32 *)p = pixel_cinza; 
                    break;
                default:
                    break;
            }
        }
    }

    // Destrava surfaces
    if (SDL_MUSTLOCK(cinza)) SDL_UnlockSurface(cinza);
    if (SDL_MUSTLOCK(orig)) SDL_UnlockSurface(orig);

    return cinza;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s caminho_da_imagem.ext\n", argv[0]);
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Erro ao inicializar o SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface* imagem = IMG_Load(argv[1]);
    if (!imagem) {
        printf("Erro ao carregar a imagem: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    printf("Imagem carregada com sucesso!\n");
    printf("Dimensões: %dx%d pixels, Formato de pixel: %s\n", imagem->w, imagem->h, SDL_GetPixelFormatName(imagem->format));

    // ################################# Verificação de Escala de Cinza ###################################
    // Verifica se a imagem já é escala de cinza
    int todos_cinza = 1;
    for (int y = 0; y < imagem->h && todos_cinza; y++) {
        for (int x = 0; x < imagem->w; x++) {
            Uint8 r, g, b, a;
            SDL_ReadSurfacePixel(imagem, x, y, &r, &g, &b, &a);
            if (!eh_cinza(r, g, b)) {
                todos_cinza = 0;
                break;
            }
        }
    }

    SDL_Surface *img_cinza;
    if (todos_cinza) {
        printf("A imagem já está em escala de cinza.\n");
        img_cinza = SDL_DuplicateSurface(imagem);
        if (!img_cinza) {
            fprintf(stderr, "Erro ao duplicar a surface: %s\n", SDL_GetError());
            SDL_DestroySurface(imagem);
            SDL_Quit();
            return 1;
        }
    } else {
        printf("Convertendo imagem para escala de cinza...\n");
        img_cinza = converte_para_cinza(imagem);
        if (!img_cinza) {
            fprintf(stderr, "Falha na conversão para escala de cinza.\n");
            SDL_DestroySurface(imagem);
            SDL_Quit();
            return 1;
        }
    }

    // ############################# Campo de Exibição da janela principal #################################
    // Tamanho fixo da janela
    int larguraP = img_cinza->w;
    int alturaP = img_cinza->h;

    // Obtém as dimensões da tela principal
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    SDL_Rect area;
    if (!SDL_GetDisplayBounds(displayID, &area)) {
        SDL_Log("Erro ao obter as dimensões da tela: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // Cria janela principal, com tamanho da imagem
    SDL_Window *win_main = SDL_CreateWindow("Janela Principal", larguraP, alturaP, 0);
    if (!win_main) {
        fprintf(stderr, "Erro ao criar a janela principal: %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_Quit();
        return 1;
    }
    // Cria renderer para a janela principal
    SDL_Renderer *rend_main = SDL_CreateRenderer(win_main, NULL);
    if (!rend_main) {
        fprintf(stderr, "Erro ao criar o renderer principal: %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_DestroyWindow(win_main);
        SDL_Quit();
        return 1;
    }

    // Converte a surface em texture para renderizar
    SDL_Texture *tex = SDL_CreateTextureFromSurface(rend_main, img_cinza);
    if (!tex) {
        fprintf(stderr, "Erro ao criar textura da imagem: %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_DestroyWindow(win_main);
        SDL_DestroyRenderer(rend_main);
        SDL_Quit();
        return 1;
    }

    // ############################# Campo de Exibição da janela secundária #################################
    // Cria janela secundária (popup) ao lado da principal
    int offset_x = larguraP + 10;
    int offset_y = 0;
    int larguraS = 400;
    int alturaS = 300;

    SDL_Window *win_sec = SDL_CreatePopupWindow(
        win_main,
        offset_x, offset_y,
        larguraS, alturaS,
        SDL_WINDOW_POPUP_MENU
    );
    if (!win_sec) {
        fprintf(stderr, "Erro ao criar a janela secundária (popup): %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_DestroyWindow(win_main);
        SDL_DestroyRenderer(rend_main);
        SDL_DestroyTexture(tex);
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *rend_sec = SDL_CreateRenderer(win_sec, NULL);
    if (!rend_sec) {
        fprintf(stderr, "Erro ao criar renderer secundário: %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_DestroyWindow(win_main);
        SDL_DestroyRenderer(rend_main);
        SDL_DestroyTexture(tex);
        SDL_DestroyWindow(win_sec);
        SDL_Quit();
        return 1;
    }

    // ############################### Loop de eventos & renderização ####################################
    bool quit = false;
    SDL_Event event;

    while (!quit) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }

        // Render da janela principal
        float aspect = (float)imagem->w / (float)imagem->h;
        SDL_FRect rect = { 0.0f, 0.0f, (float)larguraP, (float)alturaP };

        SDL_SetRenderDrawColor(rend_main, 0, 0, 0, 255);
        SDL_RenderClear(rend_main);

        if (!SDL_RenderTexture(rend_main, tex, NULL, &rect)) {
            fprintf(stderr, "Erro ao renderizar a textura: %s\n", SDL_GetError());
        }
        SDL_RenderPresent(rend_main);

        // Render da janela secundária ##### por enquanto só limpar com uma cor e desenhar um retângulo "botão"
        SDL_SetRenderDrawColor(rend_sec, 240, 240, 240, 255);
        SDL_RenderClear(rend_sec);
        SDL_FRect button = { 50.0f, 200.0f, 300.0f, 60.0f };
        SDL_SetRenderDrawColor(rend_sec, 0, 120, 255, 255);
        if (!SDL_RenderFillRect(rend_sec, &button)) {
            fprintf(stderr, "Erro ao desenhar o botão: %s\n", SDL_GetError());
        }
        SDL_RenderPresent(rend_sec);

        SDL_Delay(16);  // ~60 FPS
    }

    // ################################### Campo de Saída ############################################
    // Salva a imagem convertida para escala de cinza como png
    if (!IMG_SavePNG(img_cinza, "saida.png")) {
        fprintf(stderr, "Erro ao salvar PNG: %s\n", SDL_GetError());
    } else {
        printf("Imagem em escala de cinza salva como 'saida.png'\n");
    }


    // Limpeza
    SDL_DestroySurface(imagem);
    SDL_DestroySurface(img_cinza);
    SDL_DestroyWindow(win_main);
    SDL_DestroyRenderer(rend_main);
    SDL_DestroyTexture(tex);
    SDL_DestroyWindow(win_sec);
    SDL_DestroyRenderer(rend_sec);

    SDL_Quit();
    return 0;
}