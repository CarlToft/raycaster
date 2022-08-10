#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <cmath>
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

const int MAP_WIDTH = 10;
const int MAP_HEIGHT = 10;
const int WIDTH = 640; 
const int HEIGHT = 480;
const double PIXELS_PER_CELL = 70; 
constexpr double PI = 3.1415926535897932384626;

using radian = double;

// User define literal type which accepts degrees but evaluates to radians
constexpr radian operator"" _deg_to_rad(long double deg) {
    return deg * PI / 180;
}

// User define literal type which takes a degrees but evaluates to radians
constexpr radian operator"" _deg_to_rad(unsigned long long deg) {
  return deg * PI / 180;
}

const radian YAW_RATE = 120_deg_to_rad;

static const double MOVEMENT_SPEED = 2.0;
SDL_Surface* wall_surface;
SDL_Surface* floor_surface;
SDL_Surface* ceiling_surface;

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
        radian angular_velocity;
        radian angle; // in radians
        double fov; // in radians
        double angle_visualizer_length;
};

Player::Player() {
    x = 4.4; 
    y = 5.8; 
    angle = 0.0_deg_to_rad;
    fov = 90.0_deg_to_rad;
    speed = 0.0; 
    angular_velocity = 0.0; 
    angle_visualizer_length = 5.0;
}

class Vector {
    public:
        double coords[3];
        double norm();
        void normalize();
        Vector(double x, double y, double z);
        Vector add(const Vector &a, const Vector &b);
        Vector subtract(const Vector &a, const Vector &b);
        double dot(const Vector &a);
};

Vector::Vector(double x, double y, double z) {
    coords[0] = x;
    coords[1] = y;
    coords[2] = z;
}

double Vector::norm() {
    return sqrt(coords[0]*coords[0] + coords[1]*coords[1] + coords[2]*coords[2]);
}

void Vector::normalize() {
    double length = this->norm();
    coords[0] = coords[0] / length;
    coords[1] = coords[1] / length;
    coords[2] = coords[2] / length;
}

Vector Vector::add(const Vector &a, const Vector &b) {
    Vector result(a.coords[0] + b.coords[0], a.coords[1] + b.coords[1], a.coords[2] + b.coords[2]);
    return result;
}

Vector Vector::subtract(const Vector &a, const Vector &b) {
    Vector result(a.coords[0] - b.coords[0], a.coords[1] - b.coords[1], a.coords[2] - b.coords[2]);
    return result;
}

double Vector::dot(const Vector &a) {
    return coords[0]*a.coords[0] + coords[1]*a.coords[1] + coords[2]*a.coords[2]; 
}

double min(double a, double b) {
    return a < b ? a : b;
}

double LIGHT_X = 5.5;
double LIGHT_Y = 5.5;
double LIGHT_Z = 0.5;
const double LIGHTWIDTH = 10.0; // size of light (in pixels) in top-down map 

// Move the player. delta_t is the time in seconds since the last update
void Player::move(double delta_t) {

    const double MARGIN = speed > 0 ? 0.05 : -0.05;

    double new_x = x + speed*delta_t*cos(angle);
    double new_y = y + speed*delta_t*sin(angle);



    // Only move the player if the new spot is unoccupied
    if (MAP[int(floor(new_y + MARGIN*sin(angle)))][int(floor(new_x + MARGIN*cos(angle)))] == false) {
        x = new_x;
        y = new_y;
    }
    angle = angle + angular_velocity*delta_t;

    // Keep the player angle in the interval [0, 2*pi]
    if (angle < 0.0) {
        angle = angle + 2*PI;
    }
    if (angle > 2.0*PI) {
        angle = angle - 2.0*PI;
    }
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

double intersectWithHorizontalLine(double x, double y, radian angle, double y_line) {
    double sin_theta = sin(angle);
    if (abs(sin_theta) < 1e-8) {
        return 1e8;
    }

    return ((y_line - y)/sin_theta);
}

double intersectWithVerticalLine(double x, double y, radian angle, double x_line) {
    double cos_theta = cos(angle);
    if (abs(cos_theta) < 1e-8) {
        return 1e8;
    }

    return ((x_line - x)/cos_theta);
}

double shootRay(double x_start, double y_start, radian angle, bool& hit_horizontal) {
    int curr_x = int(floor(x_start));
    int curr_y = int(floor(y_start));

    int delta_x = cos(angle) > 0 ? 1 : -1;
    int delta_y = sin(angle) > 0 ? 1 : -1;

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

    // Draw the light source
    rect.x = LIGHT_X*PIXELS_PER_CELL - LIGHTWIDTH/2.0;
    rect.y = LIGHT_Y*PIXELS_PER_CELL - LIGHTWIDTH/2.0;
    rect.w = LIGHTWIDTH;
    rect.h = LIGHTWIDTH;
    SDL_SetRenderDrawColor(renderer, 255, 0 ,0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &rect); 

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
    double x_end = x_start + (player.angle_visualizer_length*cos(player.angle))*PIXELS_PER_CELL;
    double y_end = y_start + (player.angle_visualizer_length*sin(player.angle))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end); 

    // Draw the player field of view
    x_end = x_start + (player.angle_visualizer_length*cos(player.angle-player.fov/2.0))*PIXELS_PER_CELL;
    y_end = y_start + (player.angle_visualizer_length*sin(player.angle-player.fov/2.0))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end); 
    x_end = x_start + (player.angle_visualizer_length*cos(player.angle+player.fov/2.0))*PIXELS_PER_CELL;
    y_end = y_start + (player.angle_visualizer_length*sin(player.angle+player.fov/2.0))*PIXELS_PER_CELL;
    SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end);

    // Visualize some of the rays being cast
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, SDL_ALPHA_OPAQUE);
    double focal_length = WIDTH/(2.0*tan(player.fov/2.0));
    double depth = 0.0, angle = 0.0; 
    bool hit_horizontal = false;
    for (int pixel_col = 0; pixel_col < WIDTH; pixel_col += 8) {
        angle = player.angle + atan((pixel_col - WIDTH/2.0)/focal_length);
        depth = shootRay(player.x, player.y, angle, hit_horizontal);
        x_end = x_start + (depth*cos(angle))*PIXELS_PER_CELL;
        y_end = y_start + (depth*sin(angle))*PIXELS_PER_CELL;
        SDL_RenderDrawLine(renderer, x_start, y_start, x_end, y_end);
    }

    // Show the freshly drawn frame!
    SDL_RenderPresent(renderer); 
}

void renderRayCasterWindow(SDL_Window* window, SDL_Surface* surface, Player* player, int col_start, int col_stop) {
    const double BLOCK_HEIGHT = 1.0;
 
    // Perform raycasting
    SDL_Rect srcRect, dstRect;
    double focal_length = WIDTH/(2.0*tan(player->fov/2.0));
    double focal_length_prime;
    double depth = 0.0, angle = 0.0, height = 0.0, local_angle = 0.0, color = 200.0; 
    double x_start, x_end, y_start, y_end, fraction, x_hit, y_hit, x_fraction, y_fraction, z_hit, light_intensity, light_distance;
    Vector lightVec(0.0, 0.0, 0.0);
    bool hit_horizontal = false;
    int y_dst, x_src, y_src;
    Uint8 r, g, b;
    Vector surfaceNormal(0.0, 0.0, 0.0);
    for (int pixel_col = col_start; pixel_col < col_stop; pixel_col++) {
        local_angle = atan((pixel_col - WIDTH/2.0)/focal_length);  // local angle of ray in FOV,
                                                                            //zero being straight ahead

        angle = player->angle + local_angle; // angle of ray in global coordinates
        if (angle < 0.0) {
            angle = angle + 2.0*PI;
        } else if (angle > 2.0*PI) {
            angle = angle - 2.0*PI;
        }
        depth = shootRay(player->x, player->y, angle, hit_horizontal); // distance to hit

        // We must distinguish between hits along horizontal or vertical walls to properly compute texture coordinates
        x_hit = player->x + depth*cos(angle);
        y_hit = player->y + depth*sin(angle);
        if (hit_horizontal == true) {
            if (angle < PI) {
                surfaceNormal.coords[0] = 0.0;
                surfaceNormal.coords[1] = -1.0;
                surfaceNormal.coords[2] = 0.0;
            } else {
                surfaceNormal.coords[0] = 0.0;
                surfaceNormal.coords[1] = 1.0;
                surfaceNormal.coords[2] = 0.0;        
            }
            fraction = x_hit - floor(x_hit);
        } else {
            fraction = y_hit - floor(y_hit);
            if (angle < PI/2.0 || (angle > 3.0/2.0*PI)) {
                surfaceNormal.coords[0] = -1.0;
                surfaceNormal.coords[1] = 0.0;
                surfaceNormal.coords[2] = 0.0;
            } else {
                surfaceNormal.coords[0] = 1.0;
                surfaceNormal.coords[1] = 0.0;
                surfaceNormal.coords[2] = 0.0;        
            }
        }
        x_src = (int)(wall_surface->w*fraction);
        focal_length_prime = focal_length/cos(local_angle);
        height = focal_length_prime*BLOCK_HEIGHT/depth; // height of wall in pixels along this column
        
        for (int y_dst = HEIGHT/2.0 - height/2.0; y_dst < HEIGHT/2.0 + height/2.0; y_dst++) {
            // Sample texture RGB value
            y_src = int((y_dst-HEIGHT/2.0+height/2.0)/height*wall_surface->h);
            if (y_dst < 0) {
                y_dst = -1;
                continue;
            } else if (y_dst >= HEIGHT) {
                break;
            }

            // Compute shading
            z_hit = BLOCK_HEIGHT/2.0 - depth*(y_dst - HEIGHT/2-0)/focal_length_prime;
            lightVec.coords[0] = LIGHT_X - x_hit;
            lightVec.coords[1] = LIGHT_Y - y_hit;
            lightVec.coords[2] = LIGHT_Z - z_hit;
            light_distance = lightVec.norm();
            //lightVec.normalize();
            
            get_pixel(wall_surface, x_src, y_src, b, g, r);
            light_intensity = lightVec.dot(surfaceNormal)/(light_distance*light_distance*2);
            if (light_intensity < 0.0) {
                light_intensity = 0.0;
            }
            light_intensity = light_intensity + 0.25;
            //std::cout << light_intensity << "\n"; 
            r = (Uint8)(min(255, r*light_intensity));
            g = (Uint8)(min(255, g*light_intensity));
            b = (Uint8)(min(255, b*light_intensity));
            set_pixel(surface, pixel_col, y_dst, r, g, b);
        }

        // Draw the floor and ceiling
        double distance, xx, yy;
        for (int y_dst = HEIGHT/2.0 + height/2.0; y_dst < HEIGHT; y_dst++) {
            if (y_dst < 0) {
                continue;
            }
            else if (y_dst >= HEIGHT) {
                break;
            }
            distance = BLOCK_HEIGHT/2.0*focal_length/((y_dst - HEIGHT/2.0)*cos(local_angle));
            xx = player->x + distance*cos(angle);
            yy = player->y + distance*sin(angle);
            x_hit = xx;
            y_hit = yy;
            x_fraction = xx - floor(xx);
            y_fraction = yy - floor(yy);

            // Draw the floor
            surfaceNormal.coords[0] = 0.0;
            surfaceNormal.coords[1] = 0.0;
            surfaceNormal.coords[2] = 1.0;
            z_hit = 0.0;
            lightVec.coords[0] = LIGHT_X - x_hit; 
            lightVec.coords[1] = LIGHT_Y - y_hit;
            lightVec.coords[2] = LIGHT_Z - z_hit;
            light_distance = lightVec.norm();
            //lightVec.normalize();
            light_intensity = lightVec.dot(surfaceNormal)/(light_distance*light_distance*2);
            if (light_intensity < 0.0) {
                light_intensity = 0.0;
            }
            light_intensity = light_intensity + 0.25;
            
            x_src = (int)(x_fraction*floor_surface->w);
            y_src = (int)(y_fraction*floor_surface->h);
            get_pixel(floor_surface, x_src, y_src, b, g, r);
            r = (Uint8)(min(255, r*light_intensity));
            g = (Uint8)(min(255, g*light_intensity));
            b = (Uint8)(min(255, b*light_intensity));
            set_pixel(surface, pixel_col, y_dst, r, g, b);

            // Draw the ceiling
            surfaceNormal.coords[2] = -1.0;
            z_hit = 1.0;
            lightVec.coords[2] = LIGHT_Z - z_hit;
            light_distance = lightVec.norm();
            light_intensity = lightVec.dot(surfaceNormal)/(light_distance*light_distance*2);
            if (light_intensity < 0.0) {
                light_intensity = 0.0;
            }
            light_intensity = light_intensity + 0.25;

            x_src = (int)(x_fraction*ceiling_surface->w);
            y_src = (int)(y_fraction*ceiling_surface->h);
            get_pixel(ceiling_surface, x_src, y_src, b, g, r);
            r = (Uint8)(min(255, r*light_intensity));
            g = (Uint8)(min(255, g*light_intensity));
            b = (Uint8)(min(255, b*light_intensity));
            set_pixel(surface, pixel_col, HEIGHT - y_dst - 1, r, g, b);
        }
    }

    //SDL_UpdateWindowSurface(window);
}

int main(int argc, char * argv[]) {
    const int TOP_DOWN_WINDOW_WIDTH = PIXELS_PER_CELL*MAP_WIDTH;
    const int TOP_DOWN_WINDOW_HEIGHT = PIXELS_PER_CELL*MAP_HEIGHT;

    SDL_Init(SDL_INIT_EVERYTHING);
    // Initialize TTF
    if (TTF_Init() == -1) {
        printf("TTF_Init: %s\n", TTF_GetError());
        exit(1);
    }
   std::string font_path = "fonts/arial.ttf";
   TTF_Font* font = TTF_OpenFont(font_path.c_str(), 20);
   
   // Print some help info
   std::cout << "You can add / remove walls by clicking in the top-down view.\n";
   std::cout << "The light is also moveable by clicking on it in the top-down view and dragging it\naround using the mouse.\n";

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
    ceiling_surface = SDL_LoadBMP("images/ceiling.bmp");
    if (wall_surface == NULL) {
        std::cout << "FAILED TO LOAD WALL TEXTURE\n";
    }
    if (floor_surface == NULL) {
        std::cout << "FAILED TO LOAD FLOOR TEXTURE\n";  
    }
    if (ceiling_surface == NULL) {
        std::cout << "FAILED TO LOAD CEILING TEXTURE\n";  
    }

    // Main loop 
    auto previous_time = std::chrono::system_clock::now();
    SDL_Event event; 
    bool quit = false;
    bool is_moving_light = false;
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
                    player.speed = MOVEMENT_SPEED;
                }
                else if (event.key.keysym.sym == SDLK_DOWN) {
                    player.speed = -MOVEMENT_SPEED;
                }
                else if (event.key.keysym.sym == SDLK_LEFT) {
                    player.angular_velocity = -YAW_RATE;
                }
                else if (event.key.keysym.sym == SDLK_RIGHT) {
                    player.angular_velocity = YAW_RATE;
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
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                if (event.button.x >= LIGHT_X*PIXELS_PER_CELL - LIGHTWIDTH/2.0 && event.button.x <= LIGHT_X*PIXELS_PER_CELL + LIGHTWIDTH/2.0) {
                    if (event.button.y >= LIGHT_Y*PIXELS_PER_CELL - LIGHTWIDTH/2.0 && event.button.y <= LIGHT_Y*PIXELS_PER_CELL + LIGHTWIDTH/2.0) {
                        is_moving_light = true;
                    }
                } else {
                    if (event.button.x > PIXELS_PER_CELL && event.button.x < (MAP_WIDTH-1)*PIXELS_PER_CELL) {
                        if (event.button.y > PIXELS_PER_CELL && event.button.y < (MAP_HEIGHT-1)*PIXELS_PER_CELL) {
                            int x_cell = (int)(event.button.x / PIXELS_PER_CELL);
                            int y_cell = (int)(event.button.y / PIXELS_PER_CELL);
                            int player_x_cell = (int)(floor(player.x));
                            int player_y_cell = (int)(floor(player.y));
                            if (x_cell != player_x_cell || y_cell != player_y_cell) {
                                MAP[y_cell][x_cell] = !MAP[y_cell][x_cell];
                            }
                        }
                    }
                }
            }
            if (event.type == SDL_MOUSEBUTTONUP) {
                is_moving_light = false;
            }
            if (event.type == SDL_MOUSEMOTION) {
                if (is_moving_light) {
                    LIGHT_X = event.motion.x / PIXELS_PER_CELL;
                    LIGHT_Y = event.motion.y / PIXELS_PER_CELL;
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
        std::thread thread1(renderRayCasterWindow, window_3dview, surface_3dview, &player, 0, 320);
        std::thread thread2(renderRayCasterWindow, window_3dview, surface_3dview, &player, 320, 640);
        thread1.join();
        thread2.join();

        // Render fps in window
        std::stringstream ss;
        ss << "FPS: " << 1.0/delta_t;

        SDL_Color text_color = {255, 255, 255};
        SDL_Surface* text_surface = TTF_RenderText_Solid(font, ss.str().c_str(), text_color);

        // Blit into the 3d surface
        SDL_Rect text_rect = {0, 0, text_surface->w, text_surface->h};
        SDL_BlitSurface(text_surface, NULL, surface_3dview, &text_rect);

        // Destroy the surface
        SDL_FreeSurface(text_surface);

        // Update window
        SDL_UpdateWindowSurface(window_3dview);
    }

    // Quit
    SDL_DestroyWindow(window_topdown);
    SDL_DestroyWindow(window_3dview);
    SDL_Quit(); 

    return 0; 
}
