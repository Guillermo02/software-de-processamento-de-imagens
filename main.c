/* Universidade Presbiteriana Mackenzie – Computação Visual – Projeto 1 (SDL3)
Alunos:
- Nome: Arthur Cezar da Silveira Lima       - RA: 10409172
- Nome: Gabriel Morgado Nogueira            - RA: 10409493
- Nome: Guillermo Kuznietz                  - RA: 10410134
- Nome: Marcos Arambasic Rebelo da Silva    - RA: 10443260

Compilação:
Caso possua o MinGW instalado:
mingw32-make

Caso não tenha o MinGW instalado:
gcc main.c -IC:{coloque-caminho-até-a-pasta}\software-de-processamento-de-imagens\libs\SDL3\include -o executavel -LC:{coloque-caminho-até-a-pasta}software-de-processamento-de-imagens\libs\SDL3\lib -lSDL3 -lSDL3_image -lSDL3_ttf

Execução:
./executavel caminho-para-imagem.png  
*/

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#define NIVEIS 256

static int eh_cinza(Uint8 r, Uint8 g, Uint8 b)
{
    return (r == g && g == b);
}

// Converte superfície para escala de cinza
SDL_Surface *converte_para_cinza(SDL_Surface *orig)
{
    if (!orig)
        return NULL;

    SDL_Surface *cinza = SDL_CreateSurface(orig->w, orig->h, orig->format);
    if (!cinza)
    {
        fprintf(stderr, "Erro ao criar surface cinza: %s\n", SDL_GetError());
        return NULL;
    }

    if (SDL_MUSTLOCK(orig))
    {
        if (!SDL_LockSurface(orig))
        {
            fprintf(stderr, "Erro ao realizar lock na surface original: %s\n", SDL_GetError());
            SDL_DestroySurface(cinza);
            return NULL;
        }
    }
    if (SDL_MUSTLOCK(cinza))
    {
        if (!SDL_LockSurface(cinza))
        {
            fprintf(stderr, "Erro ao realizar lock na surface cinza: %s\n", SDL_GetError());
            if (SDL_MUSTLOCK(orig))
                SDL_UnlockSurface(orig);
            SDL_DestroySurface(cinza);
            return NULL;
        }
    }

    for (int y = 0; y < orig->h; y++)
    {
        for (int x = 0; x < orig->w; x++)
        {
            Uint8 r, g, b, a;
            if (!SDL_ReadSurfacePixel(orig, x, y, &r, &g, &b, &a))
            {
                fprintf(stderr, "Erro ao ler pixel (%d,%d): %s\n", x, y, SDL_GetError());
            }

            Uint8 valor;
            if (eh_cinza(r, g, b))
            {
                valor = r;
            }
            else
            {
                double y_val = 0.2125 * r + 0.7154 * g + 0.0721 * b;
                if (y_val < 0)
                    y_val = 0;
                if (y_val > 255)
                    y_val = 255;
                valor = (Uint8)(y_val + 0.5);
            }

            const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(cinza->format);
            int bpp = SDL_BYTESPERPIXEL(cinza->format);
            Uint32 pixel_cinza = SDL_MapRGBA(fmt, NULL, valor, valor, valor, a);
            Uint8 *p = (Uint8 *)cinza->pixels + y * cinza->pitch + x * bpp;

            switch (bpp)
            {
            case 1:
                *p = (Uint8)pixel_cinza;
                break;
            case 2:
                *(Uint16 *)p = (Uint16)pixel_cinza;
                break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                {
                    p[0] = (pixel_cinza >> 16) & 0xFF;
                    p[1] = (pixel_cinza >> 8) & 0xFF;
                    p[2] = pixel_cinza & 0xFF;
                }
                else
                {
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

    if (SDL_MUSTLOCK(cinza))
        SDL_UnlockSurface(cinza);
    if (SDL_MUSTLOCK(orig))
        SDL_UnlockSurface(orig);
    return cinza;
}

void calcular_histograma(SDL_Surface *img, int hist[NIVEIS])
{
    for (int i = 0; i < NIVEIS; i++)
        hist[i] = 0;
    for (int y = 0; y < img->h; y++)
    {
        for (int x = 0; x < img->w; x++)
        {
            Uint8 r, g, b, a;
            SDL_ReadSurfacePixel(img, x, y, &r, &g, &b, &a);
            hist[r]++;
        }
    }
}

// --------- gera LUT de equalização pela CDF ----------
void gerar_lut_equalizacao(const int hist[NIVEIS], int total_pixels, int lut[NIVEIS])
{
    int cdf[NIVEIS];
    int soma = 0;
    for (int i = 0; i < NIVEIS; i++)
    {
        soma += hist[i];
        cdf[i] = soma;
    }
    int cdf_min = 0;
    for (int i = 0; i < NIVEIS; i++)
    {
        if (cdf[i] > 0)
        {
            cdf_min = cdf[i];
            break;
        }
    }
    if (total_pixels <= cdf_min)
    {
        for (int i = 0; i < NIVEIS; i++)
            lut[i] = i;
        return;
    }
    for (int i = 0; i < NIVEIS; i++)
    {
        double v = (double)(cdf[i] - cdf_min) / (double)(total_pixels - cdf_min);
        if (v < 0.0)
            v = 0.0;
        if (v > 1.0)
            v = 1.0;
        lut[i] = (int)lrint(v * 255.0);
    }
}

// --------- aplica LUT e retorna nova surface ----------
SDL_Surface *aplicar_lut(SDL_Surface *src, const int lut[NIVEIS])
{
    if (!src)
        return NULL;
    SDL_Surface *dst = SDL_CreateSurface(src->w, src->h, src->format);
    if (!dst)
    {
        fprintf(stderr, "Erro ao criar surface equalizada: %s\n", SDL_GetError());
        return NULL;
    }

    if (SDL_MUSTLOCK(src))
    {
        if (!SDL_LockSurface(src))
        {
            fprintf(stderr, "Erro ao lock na src: %s\n", SDL_GetError());
            SDL_DestroySurface(dst);
            return NULL;
        }
    }
    if (SDL_MUSTLOCK(dst))
    {
        if (!SDL_LockSurface(dst))
        {
            fprintf(stderr, "Erro ao lock na dst: %s\n", SDL_GetError());
            if (SDL_MUSTLOCK(src))
                SDL_UnlockSurface(src);
            SDL_DestroySurface(dst);
            return NULL;
        }
    }

    const SDL_PixelFormatDetails *fmt = SDL_GetPixelFormatDetails(dst->format);
    int bpp = SDL_BYTESPERPIXEL(dst->format);

    for (int y = 0; y < src->h; y++)
    {
        for (int x = 0; x < src->w; x++)
        {
            Uint8 r, g, b, a;
            SDL_ReadSurfacePixel(src, x, y, &r, &g, &b, &a);
            Uint8 val = (Uint8)lut[r];

            Uint32 pixel_eq = SDL_MapRGBA(fmt, NULL, val, val, val, a);
            Uint8 *p = (Uint8 *)dst->pixels + y * dst->pitch + x * bpp;

            switch (bpp)
            {
            case 1:
                *p = (Uint8)pixel_eq;
                break;
            case 2:
                *(Uint16 *)p = (Uint16)pixel_eq;
                break;
            case 3:
                if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                {
                    p[0] = (pixel_eq >> 16) & 0xFF;
                    p[1] = (pixel_eq >> 8) & 0xFF;
                    p[2] = pixel_eq & 0xFF;
                }
                else
                {
                    p[0] = pixel_eq & 0xFF;
                    p[1] = (pixel_eq >> 8) & 0xFF;
                    p[2] = (pixel_eq >> 16) & 0xFF;
                }
                break;
            case 4:
                *(Uint32 *)p = pixel_eq;
                break;
            default:
                break;
            }
        }
    }

    if (SDL_MUSTLOCK(dst))
        SDL_UnlockSurface(dst);
    if (SDL_MUSTLOCK(src))
        SDL_UnlockSurface(src);
    return dst;
}

/* ----------- Histograma desenhado dentro de uma área dedicada ----------- */
void render_histograma(SDL_Renderer *renderer, int *hist, int max_contagem, SDL_FRect area)
{
    const int margem_x = 10;
    const int margem_y = 10;

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderFillRect(renderer, &area);

    float iw = area.w - 2 * margem_x;
    float ih = area.h - 2 * margem_y;
    if (iw <= 0 || ih <= 0 || max_contagem <= 0)
        return;

    int largura_barras = (int)floor(iw / NIVEIS);
    if (largura_barras < 1)
        largura_barras = 1;

    SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
    SDL_FRect inner = {area.x + margem_x, area.y + margem_y, iw, ih};
    SDL_RenderRect(renderer, &inner);

    SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
    for (int i = 0; i < NIVEIS; i++)
    {
        float proporcao = (float)hist[i] / (float)max_contagem;
        int altura_barra = (int)(ih * proporcao);
        float bx = inner.x + (float)i * largura_barras;
        float bw = (float)largura_barras;

        if (bx + bw > inner.x + inner.w)
            bw = (inner.x + inner.w) - bx;
        if (bw <= 0)
            break;

        SDL_FRect barra = {
            bx,
            inner.y + inner.h - altura_barra,
            bw,
            (float)altura_barra};
        SDL_RenderFillRect(renderer, &barra);
    }
}

void render_texto(SDL_Renderer *renderer, const char *texto, float x, float y, TTF_Font *fonte, SDL_Color cor)
{
    SDL_Surface *surfText = TTF_RenderText_Blended(fonte, texto, 0, cor);
    if (surfText)
    {
        SDL_Texture *texText = SDL_CreateTextureFromSurface(renderer, surfText);
        SDL_FRect dst = {x, y, (float)surfText->w, (float)surfText->h};
        SDL_RenderTexture(renderer, texText, NULL, &dst);
        SDL_DestroyTexture(texText);
        SDL_DestroySurface(surfText);
    }
    else
    {
        fprintf(stderr, "Erro ao criar a superfície de texto: %s\n", SDL_GetError());
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s caminho_da_imagem.ext\n", argv[0]);
        return 1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "Erro ao inicializar o SDL: %s\n", SDL_GetError());
        return 1;
    }
    if (!TTF_Init())
    {
        fprintf(stderr, "Erro ao inicializar o SDL_ttf: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *imagem = IMG_Load(argv[1]);
    if (!imagem)
    {
        printf("Erro ao carregar a imagem: %s\n", SDL_GetError());
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    printf("Imagem carregada com sucesso!\n");
    printf("Dimensões: %dx%d pixels, Formato de pixel: %s\n", imagem->w, imagem->h, SDL_GetPixelFormatName(imagem->format));

    // Verifica se a imagem já é cinza
    int todos_cinza = 1;
    for (int y = 0; y < imagem->h && todos_cinza; y++)
    {
        for (int x = 0; x < imagem->w; x++)
        {
            Uint8 r, g, b, a;
            SDL_ReadSurfacePixel(imagem, x, y, &r, &g, &b, &a);
            if (!eh_cinza(r, g, b))
            {
                todos_cinza = 0;
                break;
            }
        }
    }

    SDL_Surface *img_cinza = todos_cinza ? SDL_DuplicateSurface(imagem) : converte_para_cinza(imagem);
    if (!img_cinza)
    {
        fprintf(stderr, "Falha ao obter imagem em cinza.\n");
        SDL_DestroySurface(imagem);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    /* --------------------- Janela principal -------------------- */
    int larguraP = img_cinza->w;
    int alturaP = img_cinza->h;

    SDL_Window *win_main = SDL_CreateWindow("Janela Principal", larguraP, alturaP, 0);
    if (!win_main)
    {
        fprintf(stderr, "Erro janela principal\n");
        goto FIM_ERRO1;
    }
    SDL_Renderer *rend_main = SDL_CreateRenderer(win_main, NULL);
    if (!rend_main)
    {
        fprintf(stderr, "Erro renderer principal\n");
        goto FIM_ERRO2;
    }

    SDL_Texture *tex_original = SDL_CreateTextureFromSurface(rend_main, img_cinza);
    if (!tex_original)
    {
        fprintf(stderr, "Erro textura original\n");
        goto FIM_ERRO3;
    }

    /* --------------------- Janela secundária -------------------- */
    int larguraS = 400, alturaS = 300;
    SDL_Window *win_sec = SDL_CreatePopupWindow(win_main, larguraP + 10, 0, larguraS, alturaS, SDL_WINDOW_POPUP_MENU);
    if (!win_sec)
    {
        fprintf(stderr, "Erro janela secundária\n");
        goto FIM_ERRO4;
    }
    SDL_Renderer *rend_sec = SDL_CreateRenderer(win_sec, NULL);
    if (!rend_sec)
    {
        fprintf(stderr, "Erro renderer secundário\n");
        goto FIM_ERRO5;
    }

    // Fonte
    TTF_Font *fonte = TTF_OpenFont("OpenSans-Regular.ttf", 16);
    if (!fonte)
    {
        fprintf(stderr, "Erro ao abrir fonte: %s\n", SDL_GetError());
        goto FIM_ERRO6;
    }
    SDL_Color cor = {200, 200, 200, 255};

    /* --------------------- Histogramas e equalização -------------------- */
    int hist_orig[NIVEIS], hist_eq[NIVEIS];
    calcular_histograma(img_cinza, hist_orig);
    int max_orig = 0;
    for (int i = 0; i < NIVEIS; i++)
        if (hist_orig[i] > max_orig)
            max_orig = hist_orig[i];

    int lut[NIVEIS];
    gerar_lut_equalizacao(hist_orig, img_cinza->w * img_cinza->h, lut);
    SDL_Surface *img_eq = aplicar_lut(img_cinza, lut);
    if (!img_eq)
    {
        fprintf(stderr, "Erro ao equalizar.\n");
        goto FIM_ERRO7;
    }
    calcular_histograma(img_eq, hist_eq);
    int max_eq = 0;
    for (int i = 0; i < NIVEIS; i++)
        if (hist_eq[i] > max_eq)
            max_eq = hist_eq[i];

    SDL_Texture *tex_equalizada = SDL_CreateTextureFromSurface(rend_main, img_eq);
    if (!tex_equalizada)
    {
        fprintf(stderr, "Erro textura equalizada\n");
        goto FIM_ERRO8;
    }

    bool usando_equalizada = false;

    bool quit = false;
    SDL_Event event;

    // IDs das janelas para checar de onde veio o clique
    SDL_WindowID id_sec = SDL_GetWindowID(win_sec);

    while (!quit)
{
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            quit = true;

        // Salvar imagem ao pressionar 'S'
        if (event.type == SDL_EVENT_KEY_DOWN) {
    if (event.key.key == SDLK_S) {
        SDL_Surface *to_save = usando_equalizada ? img_eq : img_cinza;
        if (IMG_SavePNG(to_save, "output_image.png") == 1)
            printf("Imagem salva como 'output_image.png'\n");
        else
            fprintf(stderr, "Erro ao salvar PNG: %s\n", SDL_GetError());
    }
}

        if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            // Só considera cliques que acontecem na janela secundária
            if (event.button.windowID == id_sec)
            {
                float mx = event.button.x;
                float my = event.button.y;

                const float padding = 10.0f;
                const float tituloAltura = 22.0f;
                const float espacamento = 10.0f;
                const float botaoAltura = 48.0f;

                SDL_FRect areaHist = {
                    padding,
                    padding + tituloAltura + espacamento,
                    larguraS - 2 * padding,
                    alturaS - (padding + tituloAltura + espacamento) - (espacamento + botaoAltura + padding)};
                if (areaHist.h < 40)
                    areaHist.h = 40;

                SDL_FRect button = {padding, areaHist.y + areaHist.h + espacamento, larguraS - 2 * padding, botaoAltura};

                if (mx >= button.x && mx <= (button.x + button.w) &&
                    my >= button.y && my <= (button.y + button.h))
                {
                    usando_equalizada = !usando_equalizada;
                }
            }
        }
    }

    // --- Render principal (imagem) ---
    SDL_SetRenderDrawColor(rend_main, 0, 0, 0, 255);
    SDL_RenderClear(rend_main);
    SDL_FRect rect = {0, 0, (float)larguraP, (float)alturaP};
    SDL_RenderTexture(rend_main, usando_equalizada ? tex_equalizada : tex_original, NULL, &rect);
    SDL_RenderPresent(rend_main);

    // --- Render secundária (UI) ---
    SDL_SetRenderDrawColor(rend_sec, 240, 240, 240, 255);
    SDL_RenderClear(rend_sec);

    const float padding = 10.0f;
    const float tituloAltura = 22.0f;
    const float espacamento = 10.0f;
    const float botaoAltura = 48.0f;

    SDL_FRect areaHist = {
        padding,
        padding + tituloAltura + espacamento,
        larguraS - 2 * padding,
        alturaS - (padding + tituloAltura + espacamento) - (espacamento + botaoAltura + padding)};
    if (areaHist.h < 40)
        areaHist.h = 40;

    if (usando_equalizada)
        render_histograma(rend_sec, hist_eq, max_eq, areaHist);
    else
        render_histograma(rend_sec, hist_orig, max_orig, areaHist);

    render_texto(rend_sec, usando_equalizada ? "Histograma (Equalizada)" : "Histograma (Original)",
                 padding, padding, fonte, cor);

    SDL_FRect button = {padding, areaHist.y + areaHist.h + espacamento, larguraS - 2 * padding, botaoAltura};
    SDL_SetRenderDrawColor(rend_sec, 0, 120, 255, 255);
    SDL_RenderFillRect(rend_sec, &button);
    render_texto(rend_sec, usando_equalizada ? "Voltar para Original" : "Equalizar",
                 button.x + 16, button.y + 12, fonte, (SDL_Color){255, 255, 255, 255});

    SDL_RenderPresent(rend_sec);

    SDL_Delay(16);
}

    // salva a última imagem mostrada (opcional)
    if (!IMG_SavePNG(usando_equalizada ? img_eq : img_cinza, "saida.png"))
    {
        fprintf(stderr, "Erro ao salvar PNG: %s\n", SDL_GetError());
    }
    else
    {
        printf("Imagem salva como 'saida.png'\n");
    }

    // limpeza
    FIM_ERRO8:
        SDL_DestroyTexture(tex_equalizada);
    FIM_ERRO7:
        SDL_DestroySurface(img_eq);
    FIM_ERRO6:
        TTF_CloseFont(fonte);
    FIM_ERRO5:
        SDL_DestroyRenderer(rend_sec);
        SDL_DestroyWindow(win_sec);
    FIM_ERRO4:
        SDL_DestroyTexture(tex_original);
    FIM_ERRO3:
        SDL_DestroyRenderer(rend_main);
    FIM_ERRO2:
        SDL_DestroyWindow(win_main);
    FIM_ERRO1:
    SDL_DestroySurface(img_cinza);
    SDL_DestroySurface(imagem);
    TTF_Quit();
    SDL_Quit();
    return 0;
}
