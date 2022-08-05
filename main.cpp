#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <cmath>
#include <iostream>
#include <chrono>

const int MAP_WIDTH = 10;
const int MAP_HEIGHT = 10;
const int WIDTH = 640; 
const int HEIGHT = 480;
const double PIXELS_PER_CELL = 70; 
const double PI = 3.1415926535897932384626; 
SDL_Surface* wall_surface; 
SDL_Surface* floor_surface;

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
        void move(double delta_t);

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

// Move the player. delta_t is the time in seconds since the last update
void Player::move(double delta_t) {
    double new_x = x + speed*delta_t*cos(angle*PI/180);
    double new_y = y + speed*delta_t*sin(angle*PI/180);

    // Only move the player if the new spot is unoccupied
    if (MAP[int(floor(new_y))][int(floor(new_x))] == false) {
        x = new_x;
        y = new_y;
    }
    angle = angle + angular_velocity*delta_t;
}

// TODO: These two methods are super sketchy, and might crash for images other than the
//       ones we are using...
void get_pixel(SDL_Surface* surface, int x, int y, Uint8 &r, Uint8 &g, Uint8 &b) {
    r = ((Uint8*)(surface->pixels))[3*x + y*surface->pitch];
    g = ((Uint8*)(surface->pixels))[3*x + y*surface->pitch + 1];
    b = ((Uint8*)(surface->pixels))[3*x + y*surface->pitch + 2];
}

void set_pixel(SDL_Surface* surface, int x, int y, Uint8 r, Uint8 g, Uint8 b) {
    Uint32 color = SDL_MapRGB(surface->format, r, g, b);
    ((Uint32*)(surface->pixels))[x + y*surface->w] = color;
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
    if (abs(sin_theta) < 1e-8) {
        return 1e8;
    }

    return ((y_line - y)/sin_theta);
}

double intersectWithVerticalLine(double x, double y, double angle, double x_line) {
    double cos_theta = cos(angle*PI/180.0);
    if (abs(cos_theta) < 1e-8) {
        return 1e8;
    }

    return ((x_line - x)/cos_theta);
}

double shootRay(double x_start, double y_start, double angle, bool& hit_horizontal) {
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

void renderTopDownMap(SDL_Window* window, SDL_Renderer* renderer, Player& player) {
    SDL_RenderClear(renderer);
    SDL_Rect rect; 

    // Draw background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, NULL);

    // Draw the occupied cells 
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

    // Draw horizontal lines
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
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end); 

    // Draw the player field of view
    x_end = x_start + (player.angle_visualizer_length*cos((player.angle-player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    y_end = y_start + (player.angle_visualizer_length*sin((player.angle-player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end); 
    x_end = x_start + (player.angle_visualizer_length*cos((player.angle+player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    y_end = y_start + (player.angle_visualizer_length*sin((player.angle+player.fov/2.0)*PI/180))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end);

    // Visualize some of the rays being cast
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
    double focal_length = WIDTH/(2.0*tan(player.fov/2.0*PI/180.0));
    double depth = 0.0, angle = 0.0; 
    bool hit_horizontal = false;
    for (int pixel_col = 0; pixel_col < WIDTH; pixel_col += 8) {
        angle = player.angle + atan((pixel_col - WIDTH/2.0)/focal_length)*180.0/PI; 
        depth = shootRay(player.x, player.y, angle, hit_horizontal);
        x_end = x_start + (depth*cos((angle)*PI/180))*PIXELS_PER_CELL;
        y_end = y_start + (depth*sin((angle)*PI/180))*PIXELS_PER_CELL;
        SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end);
    }

    // Show the freshly drawn frame!
    SDL_RenderPresent(renderer); 
}

void renderRayCasterWindow(SDL_Window* window, SDL_Surface* surface, Player& player) {
    const double BLOCK_HEIGHT = 1.0;

    // Draw roof and floor
    SDL_Rect floorRect, ceilingRect;
    ceilingRect.x = 0.0;
    ceilingRect.y = 0.0;
    ceilingRect.w = WIDTH;
    ceilingRect.h = HEIGHT/2.0;
    SDL_FillRect(surface, &ceilingRect, SDL_MapRGB(surface->format, 50, 50, 50));
 
    // Perform raycasting
    SDL_Rect srcRect, dstRect;
    double focal_length = WIDTH/(2.0*tan(player.fov/2.0*PI/180.0));
    double depth = 0.0, angle = 0.0, height = 0.0, local_angle = 0.0, color = 200.0; 
    double x_start, x_end, y_start, y_end, fraction, x_hit, y_hit;
    bool hit_horizontal = false;
    int y_dst, x_src, y_src;
    Uint8 r, g, b;
    for (int pixel_col = 0; pixel_col < WIDTH; pixel_col++) {
        local_angle = atan((pixel_col - WIDTH/2.0)/focal_length)*180.0/PI;  // local angle of ray in FOV,
                                                                            //zero being straight ahead

        angle = player.angle + local_angle; // angle of ray in global coordinates
        depth = shootRay(player.x, player.y, angle, hit_horizontal); // distance to hit

        // We must distinguish between hits along horizontal or vertical walls to properly compute texture coordinates
        if (hit_horizontal == true) {
            x_hit = player.x + depth*cos(angle*PI/180.0);
            fraction = x_hit - floor(x_hit);
        } else {
            y_hit = player.y + depth*sin(angle*PI/180.0);
            fraction = y_hit - floor(y_hit);
        }
        x_src = (int)(900*fraction);
        height = focal_length/cos(local_angle*PI/180.0)*BLOCK_HEIGHT/depth; // height of wall in pixels along this column 
        
        for (int y_dst = HEIGHT/2.0 - height/2.0; y_dst < HEIGHT/2.0 + height/2.0; y_dst++) {
            y_src = int((y_dst-HEIGHT/2.0+height/2.0)/height*900);
            if (y_dst < 0) {
                y_dst = -1;
                continue;
            } else if (y_dst >= HEIGHT) {
                break;
            }
            get_pixel(wall_surface, x_src, y_src, b, g, r);
            Uint8 color_scaling = 255 - int((depth/20.0)*255);
            set_pixel(surface, pixel_col, y_dst, r, g, b);
        }

        // Draw the floor
        double distance, xx, yy;
        for (int y_dst = HEIGHT/2.0 + height/2.0; y_dst < HEIGHT; y_dst++) {
            if (y_dst < 0) {
                continue;
            }
            else if (y_dst >= HEIGHT) {
                break;
            }
            distance = BLOCK_HEIGHT/2.0*focal_length/((y_dst - HEIGHT/2.0)*cos(local_angle*PI/180.0));
            xx = player.x + distance*cos(angle*PI/180.0);
            yy = player.y + distance*sin(angle*PI/180.0);
            x_src = (int)((xx - floor(xx))*1200);
            y_src = (int)((yy - floor(yy))*1200);
            get_pixel(floor_surface, x_src, y_src, b, g, r);
            set_pixel(surface, pixel_col, y_dst, r, g, b);
        }
    }

    SDL_UpdateWindowSurface(window);
}

int main(int argc, char * argv[]) {
    const int TOP_DOWN_WINDOW_WIDTH = PIXELS_PER_CELL*MAP_WIDTH;
    const int TOP_DOWN_WINDOW_HEIGHT = PIXELS_PER_CELL*MAP_HEIGHT;

    SDL_Init(SDL_INIT_EVERYTHING);

    printMap(); 

    // Create windows
    SDL_Window* window_topdown = NULL;
    SDL_Renderer* renderer_topdown = NULL; 
    SDL_Window* window_3dview = NULL;
    SDL_Surface* surface_3dview = NULL;

    SDL_CreateWindowAndRenderer(TOP_DOWN_WINDOW_WIDTH, TOP_DOWN_WINDOW_HEIGHT, 0, &window_topdown, &renderer_topdown); 
    window_3dview = SDL_CreateWindow("Raycaster", 100, 100, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    surface_3dview = SDL_GetWindowSurface(window_3dview);

    Player player;

    // Load textures
    wall_surface = SDL_LoadBMP("images/wall.bmp");
    floor_surface = SDL_LoadBMP("images/floor.bmp");
    if (wall_surface == NULL) {
        std::cout << "FAILED TO LOAD WALL TEXTURE\n";
        
    }
    if (floor_surface == NULL) {
        std::cout << "FAILED TO LOAD FLOOR TEXTURE\n";
        
    }

    // Main loop 
    auto previous_time = std::chrono::system_clock::now();
    SDL_Event event; 
    bool quit = false;
    while (quit == false) {
        // Check for player inputs
        while (SDL_PollEvent(&event) != 0) {
            if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    quit = true;
                }
            }

            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_UP) {
                    player.speed = 2.0; 
                }
                else if (event.key.keysym.sym == SDLK_DOWN) {
                    player.speed = -2.0; 
                }
                else if (event.key.keysym.sym == SDLK_LEFT) {
                    player.angular_velocity = -120; 
                }
                else if (event.key.keysym.sym == SDLK_RIGHT) {
                    player.angular_velocity = 120;
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
        auto current_time = std::chrono::system_clock::now();
        double delta_t = std::chrono::duration_cast<std::chrono::microseconds>(current_time - previous_time).count();
        delta_t = delta_t / 1e6;
        previous_time = current_time;

        player.move(delta_t);

        // Render the current frame
        renderTopDownMap(window_topdown, renderer_topdown, player);
        renderRayCasterWindow(window_3dview, surface_3dview, player);
    }

    // Quit
    SDL_DestroyWindow(window_topdown);
    SDL_DestroyWindow(window_3dview);
    SDL_Quit(); 

    return 0; 
}
