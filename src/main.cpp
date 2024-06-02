#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <pspdebug.h>
#include <pspkernel.h>

PSP_MODULE_INFO("SDL-Pong", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
 
int exitCallback(int arg1, int arg2, void* common) {

    sceKernelExitGame();
    return 0;
}

int callbackThread(SceSize args, void* argp) {

    int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();

    return 0;
}

int setupCallbacks(void) {

    int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }

    return thid;
}

enum {
  SCREEN_WIDTH  = 480,
  SCREEN_HEIGHT = 272
};

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
SDL_GameController* controller = NULL;

SDL_Rect player1 = {8, SCREEN_HEIGHT / 2 - 42, 8, 42};
SDL_Rect player2 = {SCREEN_WIDTH - 16, SCREEN_HEIGHT / 2 - 42, 8, 42};
SDL_Rect ball = {SCREEN_WIDTH / 2 - 16, SCREEN_HEIGHT / 2 - 16, 12, 12};

int playerSpeed = 400;
int ballVelocityX = 200;
int ballVelocityY = 200;

bool isAutoPlayMode = true;

void quitGame() {
    
    SDL_GameControllerClose(controller);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void handleEvents() {

    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_QUIT) {
            
            quitGame();
            exit(0);
        }
    }
}

bool hasCollision(SDL_Rect player, SDL_Rect ball) {

    return player.x < ball.x + ball.w && player.x + player.w > ball.x &&
            player.y < ball.y + ball.h && player.y + player.h > ball.y;
}
 
void update(float deltaTime) {

    SDL_GameControllerUpdate();

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP) && player1.y > 0) {
        player1.y -= playerSpeed * deltaTime;
    }

    else if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN) && player1.y < SCREEN_HEIGHT - player1.h) {
        player1.y += playerSpeed * deltaTime;
    }

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_START)) {
        isAutoPlayMode = !isAutoPlayMode;
    }

    if (isAutoPlayMode && ball.y < SCREEN_HEIGHT - player2.h) {
        player2.y = ball.y;
    }

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_Y) && player2.y > 0) {
        player2.y -= playerSpeed * deltaTime;
    }

    if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_A) && player2.y < SCREEN_HEIGHT - player2.h) {
        player2.y += playerSpeed * deltaTime;
    }

    if (ball.x > SCREEN_WIDTH + ball.w || ball.x < -ball.w) {

        ball.x = SCREEN_WIDTH / 2 - ball.w;
        ball.y = SCREEN_HEIGHT / 2 - ball.h;

        ballVelocityX *= -1;
        ballVelocityY *= -1;
    }

    if (ball.y < 0 || ball.y > SCREEN_HEIGHT - ball.h) {
        ballVelocityY *= -1;
    }

    if (hasCollision(player1, ball) || hasCollision(player2, ball)) {
        ballVelocityX *= -1;
    }
    
    ball.x += ballVelocityX * deltaTime;
    ball.y += ballVelocityY * deltaTime;
}

void render() {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    SDL_RenderFillRect(renderer, &player1);

    SDL_RenderDrawLine(renderer, SCREEN_WIDTH / 2, 0, SCREEN_WIDTH / 2, SCREEN_HEIGHT);

    SDL_RenderFillRect(renderer, &ball);
    SDL_RenderFillRect(renderer, &player2);

    SDL_RenderPresent(renderer);
}

int main()
{
    setupCallbacks();
    SDL_SetMainReady();
    pspDebugScreenInit();

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        return -1;
    }

    if ((window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0)) == NULL) {
        return -1;
    }

    if ((renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) == NULL) {
        return -1;
    }

    if (SDL_NumJoysticks() < 1) {
        pspDebugScreenPrintf("no game controller");
        return -1;
    } 
    
    else {

        controller = SDL_GameControllerOpen(0);
        if (controller == NULL) {
            pspDebugScreenPrintf("unable to open game controller");
            return -1;
        }
    }

    Uint32 previousFrameTime = SDL_GetTicks();
    Uint32 currentFrameTime;
    float deltaTime;

    while (true) {

        currentFrameTime = SDL_GetTicks();
        deltaTime = (currentFrameTime - previousFrameTime) / 1000.0f;
        previousFrameTime = currentFrameTime;

        handleEvents();
        update(deltaTime);
        render();
    }

    quitGame();
}