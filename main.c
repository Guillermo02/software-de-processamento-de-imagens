#include <stdio.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <math.h>

#define NIVEIS 256

static int eh_cinza(Uint8 r, Uint8 g, Uint8 b) {
    return (r == g && g == b);
}

// Converte superfície para escala de cinza. Retornando nova superfície ou NULL em caso de erro.
SDL_Surface* converte_para_cinza(SDL_Surface *orig) {
    if (!orig) return NULL;

    // Criar nova surface com o mesmo formato e dimensões
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
                valor = r;
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
                    *(Uint16*)p = (Uint16)pixel_cinza; 
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


// Preenche histograma de uma imagem em escala de cinza
void calcular_histograma(SDL_Surface *img, int hist[NIVEIS]) {
    for (int i = 0; i < NIVEIS; i++) hist[i] = 0;

    for (int y = 0; y < img->h; y++) {
        for (int x = 0; x < img->w; x++) {
            Uint8 r, g, b, a;
            SDL_ReadSurfacePixel(img, x, y, &r, &g, &b, &a);
            hist[r]++;
        }
    }
}

// Calcula média e desvio padrão
void estatisticas_histograma(int hist[NIVEIS], int total_pixels, double *media, double *desvio) {
    // média
    double soma = 0.0;
    for (int i = 0; i < NIVEIS; i++) {
        soma += i * hist[i];
    }
    *media = soma / total_pixels;

    // desvio padrão
    double soma2 = 0.0;
    for (int i = 0; i < NIVEIS; i++) {
        double diff = i - *media;
        soma2 += diff * diff * hist[i];
    }
    *desvio = sqrt(soma2 / total_pixels);
}

// Classificações com base nas métricas
const char* classificar_intensidade(double media) {
    if (media < 85.0) return "escura";
    else if (media < 170.0) return "media";
    else return "clara";
}

const char* classificar_contraste(double desvio) {
    if (desvio < 50.0) return "baixo";
    else if (desvio < 100.0) return "medio";
    else return "alto";
}

void render_histograma(SDL_Renderer *renderer, int *hist, int max_contagem, int largura, int altura) {
    int margem_x = 20, margem_y = 20;
    int largura_barras = (largura - 2 * margem_x) / NIVEIS;

    for (int i = 0; i < NIVEIS; i++) {
        float proporcao = (float)hist[i] / (float)max_contagem;
        int altura_barra = (int)((altura - 2 * margem_y) * proporcao);
        SDL_FRect barra = {
            margem_x + i * largura_barras,
            altura - margem_y - altura_barra,
            (float)largura_barras,
            (float)altura_barra
        };
        SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
        SDL_RenderFillRect(renderer, &barra);
    }
}

void render_texto(SDL_Renderer *renderer, const char *texto, float x, float y, TTF_Font *fonte, SDL_Color cor) {
    SDL_Surface *surfText = TTF_RenderText_Blended(fonte, texto, 0, cor);
    if (surfText) {
        SDL_Texture *texText = SDL_CreateTextureFromSurface(renderer, surfText);
        SDL_FRect dst = { x, y, (float)surfText->w, (float)surfText->h };
        SDL_RenderTexture(renderer, texText, NULL, &dst);
        SDL_DestroyTexture(texText);
        SDL_DestroySurface(surfText);
    } else {
        fprintf(stderr, "Erro ao criar a superfície de texto: %s\n", SDL_GetError());
    }
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

    if (!TTF_Init()) {
        fprintf(stderr, "Erro ao inicializar o SDL_ttf: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface* imagem = IMG_Load(argv[1]);
    if (!imagem) {
        printf("Erro ao carregar a imagem: %s\n", SDL_GetError());
        SDL_Quit();
        TTF_Quit();
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
            TTF_Quit();
            return 1;
        }
    } else {
        printf("Convertendo imagem para escala de cinza...\n");
        img_cinza = converte_para_cinza(imagem);
        if (!img_cinza) {
            fprintf(stderr, "Falha na conversão para escala de cinza.\n");
            SDL_DestroySurface(imagem);
            SDL_Quit();
            TTF_Quit();
            return 1;
        }
    }

    // ############################# Campo de Exibição da janela principal #################################
    // Janela do tamanho da imagem
    int larguraP = img_cinza->w;
    int alturaP = img_cinza->h;

    // Obtém as dimensões da tela principal ##################### checar uso
    SDL_DisplayID displayID = SDL_GetPrimaryDisplay();
    SDL_Rect area;
    if (!SDL_GetDisplayBounds(displayID, &area)) {
        SDL_Log("Erro ao obter as dimensões da tela: %s", SDL_GetError());
        SDL_Quit();
        TTF_Quit();
        return 1;
    }

    // Cria janela principal, com tamanho da imagem
    SDL_Window *win_main = SDL_CreateWindow("Janela Principal", larguraP, alturaP, 0);
    if (!win_main) {
        fprintf(stderr, "Erro ao criar a janela principal: %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_Quit();
        TTF_Quit();
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
        TTF_Quit();
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
        TTF_Quit();
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
        TTF_Quit();
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
        TTF_Quit();
        return 1;
    }

    // ############################ Análise e exibição do Histograma #################################
    TTF_Font *fonte = TTF_OpenFont("OpenSans-Regular.ttf", 16);
    if (!fonte) {
        fprintf(stderr, "Erro ao abrir fonte: %s\n", SDL_GetError());
        SDL_DestroySurface(imagem);
        SDL_DestroySurface(img_cinza);
        SDL_DestroyWindow(win_main);
        SDL_DestroyRenderer(rend_main);
        SDL_DestroyTexture(tex);
        SDL_DestroyWindow(win_sec);
        SDL_DestroyRenderer(rend_sec);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    int hist[NIVEIS];
    calcular_histograma(img_cinza, hist);

    int max_contagem = 0;
    for (int i = 0; i < NIVEIS; i++) {
        if (hist[i] > max_contagem) max_contagem = hist[i];
    }

    // // dimensões da janela secundária
    // int sw, sh;
    // SDL_GetWindowSize(win_sec, &sw, &sh);

    // // margens
    // int margem_x = 20, margem_y = 20;
    // int largura_das_barras = (sw - 2*margem_x) / NIVEIS;

    // // desenha barras
    // for (int i = 0; i < NIVEIS; i++) {
    //     float proporcao = (float)hist[i] / (float)max_contagem;
    //     int altura_barra = (int)((sh - 2*margem_y) * proporcao);
    //     SDL_FRect barra = {
    //         margem_x + i * largura_das_barras,
    //         sh - margem_y - altura_barra,
    //         (float)largura_das_barras,
    //         (float)altura_barra
    //     };
    //     SDL_SetRenderDrawColor(rend_sec, 100, 100, 255, 255);  // azul claro
    //     SDL_RenderFillRect(rend_sec, &barra);
    // }

    // renderiza o texto
    SDL_Color cor = {200, 200, 200, 255};
    // SDL_Surface *surfText = TTF_RenderText_Blended(fonte, "Histograma", 0, cor);
    // if (!surfText) {
    //     fprintf(stderr, "Erro ao criar a superfície de texto: %s\n", SDL_GetError());
    // }
    // SDL_Texture *texText = SDL_CreateTextureFromSurface(rend_sec, surfText);
    // SDL_FRect dst = { 0.0f, 0.0f, (float)surfText->w, (float)surfText->h };
    // SDL_RenderTexture(rend_sec, texText, NULL, &dst);
    // SDL_DestroyTexture(texText);
    // SDL_DestroySurface(surfText);

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

        // Desenhar o histograma
        render_histograma(rend_sec, hist, max_contagem, larguraS-20, alturaS-20);
        // Renderizar o texto
        render_texto(rend_sec, "Histograma", 10, 10, fonte, cor);
        
        SDL_RenderPresent(rend_sec);
        
        // Desenhar o histograma
        // char buffer[128];
        // double media, desvio;
        // estatisticas_histograma(hist, img_cinza->w * img_cinza->h, &media, &desvio);

        // const char *clint = classificar_intensidade(media);
        // const char *clcont = classificar_contraste(desvio);

        // // monta string
        // snprintf(buffer, sizeof(buffer), "Media: %.2f (%s)\nDesvio: %.2f (%s)", media, clint, desvio, clcont);

        // SDL_RenderPresent(rend_sec);
        
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
    TTF_CloseFont(fonte);

    SDL_Quit();
    TTF_Quit();
    return 0;
}
