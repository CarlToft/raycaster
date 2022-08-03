#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cmath>
#include <iostream>

const int MAP_WIDTH = 10;
const int MAP_HEIGHT = 10;
const int WIDTH = 640; 
const int HEIGHT = 480;
const double PIXELS_PER_CELL = 70; 
const double PI = 3.1415926535897932384626; 
SDL_Surface* wall_surface; 
SDL_Texture* wall_texture;

bool MAP[MAP_HEIGHT][MAP_WIDTH] = {
    {true, true, true, true, true, true, true, true, true, true},
    {true, false, false, true, false, false, false, false, false, true},
    {true, false, false, false, false, false, false, false, false, true},
    {true, false, false, false, false, false, false, false, false, true},
    {true, false, true, true, true, false, false, false, false, true},
    {true, false, false, false, false, false, false, false, false, true},
    {true, false, false, false, false, false, false, false, false, true},
    {true, false, false, false, false, true, true, true, false, true},
    {true, false, false, false, false, false, false, false, false, true},
    {true, true, true, true, true, true, true, true, true, true},
}; 

class Player {
    public:
        Player(); 

        double x;
        double y;
        double speed; 
        double angular_velocity; 
        double angle; // in degrees
        double fov; // in degrees
        double angle_visualizer_length;
};

Player::Player() {
    x = 4.4; 
    y = 5.8; 
    angle = 0.0; // in degrees  
    fov = 90.0; // in degrees 
    speed = 0.0; 
    angular_velocity = 0.0; 
    angle_visualizer_length = 5.0;
}

void initMap() {
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            MAP[row][col] = false;
        }
    }

    for (int row = 0; row < MAP_HEIGHT; row++) {
        MAP[row][0] = true;
        MAP[row][MAP_WIDTH-1] = true;
    }
    for (int col = 0; col < MAP_WIDTH; col++) {
        MAP[0][col] = true;
        MAP[MAP_HEIGHT-1][col] = true;
    }
}

void printMap() {
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            std::cout << MAP[row][col] << " ";
        }
        std::cout << std::endl;
    }
}

double intersectWithHorizontalLine(double x, double y, double angle, double y_line) {
    double sin_theta = sin(angle*PI/180.0);
    //std::cout << "sin: " << sin_theta << " " << abs(sin_theta) << "\n";
    if (abs(sin_theta) < 1e-8) {
        return 1e8;
    }

    return ((y_line - y)/sin_theta);
}

double intersectWithVerticalLine(double x, double y, double angle, double x_line) {
    double cos_theta = cos(angle*PI/180.0);
    //std::cout << "cos: " << cos_theta << " " << abs(cos_theta) << "\n";
    if (abs(cos_theta) < 1e-8) {
        return 1e8;
    }

    return ((x_line - x)/cos_theta);
}

double shootRayOptimized(double x_start, double y_start, double angle, bool& hit_horizontal) {
    int curr_x = int(floor(x_start));
    int curr_y = int(floor(y_start));

    int delta_x = cos(angle*PI/180.0) > 0 ? 1 : -1;
    int delta_y = sin(angle*PI/180.0) > 0 ? 1 : -1;

    double x_line, y_line;
    if (delta_x == -1) {
        x_line = floor(x_start);
    } else {
        x_line = floor(x_start) + delta_x;
    }
    if (delta_y == -1) {
        y_line = floor(y_start);
    } else {
        y_line = floor(y_start) + delta_y;
    }

    double horizontal_line_distance = intersectWithHorizontalLine(x_start, y_start, angle, y_line);
    double vertical_line_distance = intersectWithVerticalLine(x_start, y_start, angle, x_line);

    while (true) {
        if (horizontal_line_distance < vertical_line_distance) {
            // We intersected the horizontal line
            curr_y = curr_y + delta_y;
            if (MAP[curr_y][curr_x] == true) {
                hit_horizontal = true;
                return horizontal_line_distance;
            }

            y_line = y_line + delta_y;
            horizontal_line_distance = intersectWithHorizontalLine(x_start, y_start, angle, y_line);
        } else {
            curr_x = curr_x + delta_x;
            if (MAP[curr_y][curr_x] == true) {
                hit_horizontal = false;
                return vertical_line_distance;
            }

            x_line = x_line + delta_x;
            vertical_line_distance = intersectWithVerticalLine(x_start, y_start, angle, x_line);
        }
    } 
}

double shootRay(double x_start, double y_start, double angle) {
    const double STEP_SIZE = 0.05;
    double curr_x = x_start; 
    double curr_y = y_start;
    double depth = 0; 
    while (true) {
        curr_x = curr_x + STEP_SIZE*cos(angle*PI/180.0); 
        curr_y = curr_y + STEP_SIZE*sin(angle*PI/180.0);
        depth = depth + STEP_SIZE;  
        if (MAP[int(floor(curr_y))][int(floor(curr_x))] == true) {
            return depth; 
        }
    }
}

void renderTopDownMap(SDL_Window* window, SDL_Renderer* renderer, Player& player) {
    SDL_RenderClear(renderer);
    SDL_Rect rect; 

    // Draw background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, NULL);
    // Draw the cells 
    for (int row = 0; row < MAP_HEIGHT; row++) {
        for (int col = 0; col < MAP_WIDTH; col++) {
            if (MAP[row][col] == true) {
                rect.x = col*PIXELS_PER_CELL;
                rect.y = row*PIXELS_PER_CELL;
                rect.h = PIXELS_PER_CELL;
                rect.w = PIXELS_PER_CELL; 

                SDL_SetRenderDrawColor(renderer, 100, 100, 100, SDL_ALPHA_OPAQUE);
                SDL_RenderFillRect(renderer, &rect); 

            }
        }
    }

    // Draw horizontal line
    for (int row = 0; row <= MAP_HEIGHT; row++) {
        rect.w = MAP_WIDTH*PIXELS_PER_CELL;
        rect.h = 2;
        rect.x = 0;
        rect.y = row*PIXELS_PER_CELL - rect.h/2.0; 

        SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &rect); 
    }
    // Draw vertical lines 
    for (int col = 0; col <= MAP_WIDTH; col++) {
        rect.h = MAP_HEIGHT*PIXELS_PER_CELL;
        rect.w = 2; 
        rect.x = col*PIXELS_PER_CELL - rect.w/2.0;
        rect.y = 0;

        SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
        SDL_RenderFillRect(renderer, &rect); 
    }

    // Draw the player
    const double PLAYER_WIDTH = 10.0; 
    const double PLAYER_HEIGHT = 10.0; 
    rect.x = player.x*PIXELS_PER_CELL - PLAYER_WIDTH/2.0; 
    rect.y = player.y*PIXELS_PER_CELL - PLAYER_HEIGHT/2.0; 
    rect.w = PLAYER_WIDTH; 
    rect.h = PLAYER_HEIGHT;
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &rect);

    // Draw the viewing direction
    double x_start = player.x*PIXELS_PER_CELL;
    double y_start = player.y*PIXELS_PER_CELL; 
    double x_end = x_start + (player.angle_visualizer_length*cos(player.angle*PI/180))*PIXELS_PER_CELL;
    double y_end = y_start + (player.angle_visualizer_length*sin(player.angle*PI/180))*PIXELS_PER_CELL;
    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end); 

    // Draw field of view
    x_end = x_start + (player.angle_visualizer_length*cos((player.angle-player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    y_end = y_start + (player.angle_visualizer_length*sin((player.angle-player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end); 
    x_end = x_start + (player.angle_visualizer_length*cos((player.angle+player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    y_end = y_start + (player.angle_visualizer_length*sin((player.angle+player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end);

    // Visualize rays
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
    double focal_length = WIDTH/(2.0*tan(player.fov/2.0*PI/180.0));
    double depth = 0.0, angle = 0.0; 
    bool hit_horizontal = false;
    for (int pixel_col = 0; pixel_col < WIDTH; pixel_col += 8) {
        angle = player.angle + atan((pixel_col - WIDTH/2.0)/focal_length)*180.0/PI; 
        depth = shootRayOptimized(player.x, player.y, angle, hit_horizontal);
        //std::cout << depth << "\n"; 
        x_end = x_start + (depth*cos((angle)*PI/180))*PIXELS_PER_CELL;
        y_end = y_start + (depth*sin((angle)*PI/180))*PIXELS_PER_CELL;
        SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end);
    }

    SDL_RenderPresent(renderer); 
}

void renderRayCasterWindow(SDL_Window* window, SDL_Renderer* renderer, Player& player) {
    const double BLOCK_HEIGHT = 1.0; // TODO: IS THIS CORRECT?
    // Paint background
    SDL_Rect floorRect, ceilingRect;
    ceilingRect.x = 0.0;
    ceilingRect.y = 0.0;
    ceilingRect.w = WIDTH;
    ceilingRect.h = HEIGHT/2.0;
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &ceilingRect);
    floorRect.y = HEIGHT/2.0;
    floorRect.w = WIDTH;
    floorRect.h = HEIGHT/2.0;
    floorRect.x = 0.0;
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &floorRect);
 
    // Visualize rays
    SDL_Rect srcRect, dstRect;
    double focal_length = WIDTH/(2.0*tan(player.fov/2.0*PI/180.0));
    double depth = 0.0, angle = 0.0, height = 0.0, local_angle = 0.0, color = 200.0; 
    double x_start, x_end, y_start, y_end, fraction, x_hit, y_hit;
    bool hit_horizontal = false;
    for (int pixel_col = 0; pixel_col < WIDTH; pixel_col++) {
        local_angle = atan((pixel_col - WIDTH/2.0)/focal_length)*180.0/PI;
        angle = player.angle + local_angle; 
        depth = shootRayOptimized(player.x, player.y, angle, hit_horizontal);
        if (hit_horizontal == true) {
            x_hit = player.x + depth*cos(angle*PI/180.0);
            fraction = x_hit - floor(x_hit);
        } else {
            y_hit = player.y + depth*sin(angle*PI/180.0);
            fraction = y_hit - floor(y_hit);
        }
        height = focal_length/cos(local_angle*PI/180.0)*BLOCK_HEIGHT/depth;
        
        x_start = pixel_col;
        x_end = pixel_col;
        y_start = HEIGHT/2.0 - height/2.0;
        y_end = HEIGHT/2.0 + height/2.0;

        color = depth / 15.0*255.0 + 50.0;
        dstRect.x = x_start;
        dstRect.y = y_start;
        dstRect.w = 1;
        dstRect.h = y_end - y_start + 1;
        srcRect.x = fraction*900;
        srcRect.y = 0.0;
        srcRect.w = 1.0;
        srcRect.h = 900; 
        SDL_SetRenderDrawColor(renderer, color, color, color, SDL_ALPHA_OPAQUE);
        SDL_RenderCopyEx(renderer, wall_texture, &srcRect, &dstRect, 0.0, NULL, SDL_FLIP_NONE);
    }

    SDL_RenderPresent(renderer);
}

int main(int argc, char * argv[]) {
    const int TOP_DOWN_WINDOW_WIDTH = PIXELS_PER_CELL*MAP_WIDTH;
    const int TOP_DOWN_WINDOW_HEIGHT = PIXELS_PER_CELL*MAP_HEIGHT;

    printMap(); 

    // Create windows
    SDL_Window* window_topdown = NULL;
    SDL_Renderer* renderer_topdown = NULL; 
    SDL_Window* window_3dview = NULL;
    SDL_Renderer* renderer_3dview = NULL; 

    SDL_CreateWindowAndRenderer(TOP_DOWN_WINDOW_WIDTH, TOP_DOWN_WINDOW_HEIGHT, 0, &window_topdown, &renderer_topdown); 
    SDL_CreateWindowAndRenderer(WIDTH, HEIGHT, 0, &window_3dview, &renderer_3dview);

    Player player;

    // Load textures
    wall_surface = SDL_LoadBMP("/home/carl/Code/SDL/test/images/wall.bmp");
    if (wall_surface == NULL) {
        std::cout << "MASSIVE FAILURE\n";
        
    }
    wall_texture = SDL_CreateTextureFromSurface(renderer_3dview, wall_surface);

    // Main loop 
    SDL_Event event; 
    bool quit = false;
    while (quit == false) {
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) {
                    player.speed = 0.002; 
                }
                else if (event.key.keysym.sym == SDLK_DOWN) {
                    player.speed = -0.002; 
                }
                else if (event.key.keysym.sym == SDLK_LEFT) {
                    player.angular_velocity = -0.2; 
                }
                else if (event.key.keysym.sym == SDLK_RIGHT) {
                    player.angular_velocity = 0.2;
                }
                else if (event.key.keysym.sym == SDLK_ESCAPE) {
                    // QUIT GAME
                    quit = true; 
                }
            }
            if (event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_UP || event.key.keysym.sym == SDLK_DOWN) {
                    player.speed = 0.0; 
                }
                else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_RIGHT) {
                    player.angular_velocity = 0.0; 
                }
            }


        }

        // Move the player
        double new_x = player.x + player.speed*cos(player.angle*PI/180);
        double new_y = player.y + player.speed*sin(player.angle*PI/180);
        if (MAP[int(floor(new_y))][int(floor(new_x))] == false) {
            player.x = new_x;
            player.y = new_y;
        }
        player.angle = player.angle + player.angular_velocity;

        renderTopDownMap(window_topdown, renderer_topdown, player);
        renderRayCasterWindow(window_3dview, renderer_3dview, player);
    }

    // Quit
    SDL_DestroyWindow(window_topdown);
    SDL_DestroyWindow(window_3dview);
    SDL_Quit(); 

    return 0; 
}
