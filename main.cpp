#include <iostream>
#include "raylib/include/raylib.h"
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <typeinfo>

using namespace std;


int main() {
  const int screenWidth = 800;
  const int screenHeight = 600;
  InitWindow(screenWidth, screenHeight, "Die to progress");
  InitAudioDevice();
  SetTargetFPS(60);
  struct ButtonHUD {
    float x;
    float y;
    string text;
    float length;
    float height;
    ButtonHUD(float x, float y, string text, float length, float height) : x(x), y(y), text(text),length(length), height(height) {};
    bool is_clicked() {
        Vector2 mouse_pos = GetMousePosition();
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if(x < mouse_pos.x && x+length>mouse_pos.x) {
                if(y < mouse_pos.y && y+height > mouse_pos.y) {
                    return true;
                };
            };
        };
        return false;
    };
    void draw() {
        DrawRectangle(x,y,length,height,GREEN);
        Vector2 text_dimension = MeasureTextEx(GetFontDefault(), text.c_str(), 30, 1.0f);
        DrawText(text.c_str(),x+((length-text_dimension.x)/2),y+((height-text_dimension.y)/2),30,BLACK);
    };
  };
  struct MusicPlayer {
    vector<string> musics = {"Audio/Bubblaine.mp3"};
    int index = 0;
    Music select = LoadMusicStream(musics[index].c_str());
    float timeplayed = 0.0f;
    void update() {
        UpdateMusicStream(select);
        timeplayed = GetMusicTimePlayed(select)/GetMusicTimeLength(select);
        if(timeplayed >= 1.0f)  {
            if(index+1 < musics.size()) {
                index++;
                unload();
                select = LoadMusicStream(musics[index].c_str());
            } else {
                unload();
                index = 0;
                select = LoadMusicStream(musics[index].c_str());
            }
        };
    };
    void play() {
        PlayMusicStream(select);
    };
    void unload() {
        UnloadMusicStream(select);
    };
  };
  struct Options {
    float volume = 0.8f;
    bool active = false;
    Color option_background = {145,145,145,200};
    Vector2 volume_cursor = {};
    ButtonHUD back = {(screenWidth-400)/2+150.0f,300.0f,"Back",100.0f,50.0f};
    void update(Music *music) {
        if(IsKeyDown(KEY_RIGHT)) {
            volume += 0.05f;
            if (volume > 1.0f) volume = 1.0f;
            SetMusicVolume(*music,volume);
        } else if(IsKeyDown(KEY_LEFT)) {
            volume -= 0.05f;
            if (volume < 0.0f) volume = 0.0f;
            SetMusicVolume(*music,volume);
        };
        if(back.is_clicked()) {
            active = false;
        };
    };
    void draw() {
        DrawRectangle((screenWidth-400)/2,100.0f,400.0f,300.0f,option_background);
        back.draw();
        DrawText("RIGHT-LEFT for VOLUME CONTROL", 320, 234, 10, DARKGREEN);
        DrawRectangle(300, 260, 200, 12, LIGHTGRAY);
        DrawRectangleLines(300, 260, 200, 12, GRAY);
        DrawRectangle((int)(300 + volume*200 - 5), 252, 10, 28, DARKGRAY);
    };
  };
  struct Menu {
    Options options = {};
    Color background_color = {184, 203, 208,255};
    bool quit = false;
    bool game = false;
    vector<ButtonHUD*> buttons= {
        new ButtonHUD(100.0f, 200.0f, "Play", 250.0f, 75.0f),
        new ButtonHUD(100.0f, 350.0f, "Options", 250.0f, 75.0f),
        new ButtonHUD(100.0f, 500.0f, "Quit", 250.0f, 75.0f)
    };
    void update() {
        for(auto *button  : buttons) {
            if(button->is_clicked() && !options.active) {
                if(button->text == "Play") {
                    game = true;
                } else if(button->text == "Quit") {
                    quit = true;
                } else if(button->text == "Options") {
                    options.active = true;
                };
            };
        };
    };
    void draw() {
        DrawText("Die to progress",110.0f,75.0f,75.0f,BLACK);
        ClearBackground(background_color);
        for(auto *button : buttons) {
            button->draw();
        };
        if(options.active) {
            options.draw();
        };
    };
  };
  struct Entity {
    float x;
    float y;
    float length = 0.0f;
    float height = 0.0f;
    Color color;
    string type = "";
    bool active = false;
    bool visible = true;
    Entity(float x, float y) : x(x), y(y) {};

    virtual void draw() {};
    virtual void moving() {};
  };
  struct Saw : public Entity {
    float radius;
    Color color = {};
    Saw(float x, float y, float saw_radius) : Entity(x, y), radius(saw_radius) {};
    void draw() override {
        DrawCircle(x,y,radius,GRAY);
    };
  };
  struct FireZone : public Entity {
    FireZone(float x, float y) : Entity(x , y) {};
    void draw() override {
        if (active) {
            DrawTriangle({x, y-30.0f}, {x-15.0f, y+15.0f}, {x+15.0f, y+15.0f}, ORANGE);
            DrawTriangle({x, y-15.0f}, {x-10.0f, y+15.0f}, {x+10.0f, y+15.0f}, YELLOW);
        };
    };
  };
  struct Box : public Entity {
    Box(float x ,float y) : Entity(x , y) {};
    void draw() override {
        DrawRectangle(x,y,50.0f,50.0f, MAROON);
    };
  };
  struct AirFlow : public Entity {
    float height;
    float velocity = 1.0f;
    AirFlow(float x , float y, float height) : Entity(x , y), height(height) {};
    void draw() override {
        DrawRectangle(x, y, 60.0f, height, LIGHTGRAY);
    };
  };
  struct Platform : public Entity {
    float length;
    bool is_moving = false;
    float x_start;
    float velocity_x = 1.0f;
    Color color = {112, 156, 167, 255};
    Platform(float x, float y, float play_length) : Entity(x,y) , length(play_length) {}; 
    
    void moving() override {
        if (is_moving) {
            x += velocity_x;
            if (x > x_start+100.0f && velocity_x > 0) {
                velocity_x = -1.0f;
            }  else if (x < x_start && velocity_x < 0){
                velocity_x = 1.0f;
            }; 
        };
    };
    void draw() override {
        DrawRectangle(x,y,length,50.0f, color);
    };
  };
  struct Bomb : public Entity {
    float range;
    float power;
    float gravity = 1.5f;
    Bomb(float x, float y, float range, float power ) : Entity(x, y), range(range), power(power) {
        visible = false;
    };
    void explode() {
        active = true;
        visible = false;
        reset();
    };
    void collide(Platform *platform) {
        if (((x+50.0f > platform->x ) && x < platform->x + platform->length)) {
            if (y+50.0f > platform->y && y+50.0f < platform->y+50.0f) {
                explode();
            };
        };
    };
    void update() {
        if(visible) {
            y += gravity;
        };
    };
    void draw() override {
        DrawCircle(x, y,25.0f,RED);
    };
    void reset() {
        *this = Bomb{0.0f,y, range, power};
    };
  };
  struct Button : public Entity {
    Color color;
    float length = 30.0f;
    Button(float x,float y, Color color) : Entity(x, y), color(color) {};

    void update(Entity *entity) {
        if (ColorIsEqual(color, GREEN)) {
            if (typeid(*entity) == typeid(FireZone)) { 
                entity->active = true;
                entity->visible = true;
            } else if(typeid(*entity) == typeid(Button)) {
                entity->active = true;
                entity->visible = true;
            };
        } else if(ColorIsEqual(color, YELLOW)) {
            if(typeid(*entity) == typeid(AirFlow)) {
                AirFlow* airflow = static_cast<AirFlow*>(entity);
                airflow->velocity += 1.0f;
            };
        } else if(ColorIsEqual(color, RED)) {
            if(typeid(*entity) == typeid(Bomb)) {
                static_cast<Bomb*>(entity)->visible = true;
            };
        } else if(ColorIsEqual(color,BLUE)) {
            if(typeid(*entity) == typeid(Platform)) {
                entity->visible = true;
            };
        };
    };

    void draw() override {
        DrawRectangle(x,y,length,10.0f,color);
    };
  };
  struct Obstacle : public Entity {
    string type;
    Obstacle(float x , float y, string type) : Entity(x,y), type(type) {
        length = 1.0f;
    };
    float obstacle_length = 0.0f;

    void draw() override {
        if (visible) {
            if (type == "spike-v") {
                DrawTriangle({x,y-25.0f},{x-25.0f,y+25.0f},{x+25.0f,y+25.0f}, RED);
            } else if (type == "spike-h") {
                DrawTriangle({x+25.0f,y-25.0f},{x-25.0f, y},{x+25.0f, y+25.0f}, RED);
            } else if (type == "lava") {
                DrawRectangle(x,y,obstacle_length,50.0f,ORANGE);
            };
        };
    };
  };
  struct Wall : public Entity {
    float height;
    Color color = {112,156,167,255};
    Wall(float x, float y, float height) : Entity(x,y), height(height) {};
    void draw() override {
        DrawRectangle(x,y,50.0f, height, color);
    };
  };
  struct Level {
    int id;
    vector<float> player_pos;
    vector<Entity*> entities;  // pointers to avoid object slicing
    int nbr_lock;
    string name = "Level " + to_string(id);
    vector<string> lock = {};

    void update(string &type) {
        if (find(lock.begin(), lock.end(), type) == lock.end()) {
            lock.push_back(type);
        };
    };
    void draw() {
        for (auto *entity : entities) {
            if (entity->visible) {
               entity->draw(); 
            };
        };
        DrawText(name.c_str(), 20, 20, 20, BLACK); 
        string text_remain_lock = "Remaining locks : " + to_string(nbr_lock-lock.size()); 
        DrawText(text_remain_lock.c_str(),20, 50, 20 ,BLACK);
    };
  };
  struct Player : public Entity {
    float velocity_x = 5.0f;
    float velocity_y = 0.0f;
    float gravity = 0.8f;
    bool is_jumping = false;
    float y_start= 525.0f;
    Color player_color = {184, 203, 208,255};
    Player(float x, float y) : Entity(x,y) {};
    //Methods
    void move() {
        if (IsKeyDown(KEY_A)) {
            x -= velocity_x;
        } else if (IsKeyDown(KEY_D)) {
            x += velocity_x;
        };
        if (IsKeyDown(KEY_SPACE)) {
            if (!is_jumping) {
                is_jumping = true;
                velocity_y = 20.0f;
                y_start = y;
            };
        };
        y -= velocity_y;
        velocity_y -= gravity;
    };

    void draw() override {
        DrawRectangle(x,y, 50, 50, player_color);
    };

    void collide_platform(Platform *platform, Level &level) {
        if(platform->visible) {
            if (((x+50.0f > platform->x ) && x < platform->x + platform->length)) {
                if (y+50.0f > platform->y && y+50.0f < platform->y+50.0f) {
                    velocity_y = 0;
                    is_jumping = false;
                    y = platform->y-50.0f;
                }; 
                if (y >= platform->y && y <= platform->y+50.0f) {
                    y = platform->y+50.0f;
                    if(velocity_y >= 30.0f) {
                        string death_type = "Crash"; 
                        update_status(level,death_type);
                    };
                };
            };
        };
    };

    float distance(Entity *entity) {
        return pow((pow(x+25.0f-entity->x, 2) + pow(y+25.0f-entity->y, 2)), 0.5);
    };

    void collide_obstacle(Obstacle *entity, Level &level) {
        if (entity->type == "spike-v" || entity->type == "spike-h") {
            if (distance(entity) < 50.0f) {
                update_status(level, entity->type);
            };
        } else if (entity->type == "lava") {
            if (x+50.0f > entity->x && x < entity->x+entity->obstacle_length) {
                if (y+50.0f > entity->y && y < entity->y+50.0f) {
                    update_status(level, entity->type);
                };
            };
        };
    };

    bool press_button(Button *button) {
        if (x+50.0f > button->x && x < button->x+button->length) {
            if (abs(y+50.0f-button->y) <= 10.0f) {
                y = button->y-50.0f;
                return true;
            };
        };
        return false;
    };

    float collide_circle(Entity *entity) {
        float closestX = max(x, min(entity->x, x+50.0f));
        float closestY = max(y, min(entity->y, y+50.0f));
        float x_vec = entity->x - closestX;
        float y_vec = entity->y - closestY;
        return pow(x_vec*x_vec + y_vec*y_vec, 0.5);
    };

    void collide_saw(Saw *saw, Level &level) {
        float dist = collide_circle(saw);
        if(dist <= saw->radius) {
            string death_type = "Saw";
            update_status(level,death_type);
        };
    };

    void collide_bomb(Bomb *bomb, Level &level) {
        float dist = collide_circle(bomb);
        if(dist <= bomb->range && bomb->power > 5.0f) {
            string death_type = "Explosion";
            update_status(level, death_type);
        };
    };

    void collide_wall(Wall *wall) {
        if (y+50.0f > wall->y && y < wall->y+wall->height) {
            if (x+50.0f > wall->x && x < wall->x+50.0f) {
              if (pow(x-wall->x,2) < pow(x-wall->x+50.0f,2)) {
                x = wall->x+50.0f;
              } else {
                x = wall->x-50.0f;
              };  
            };  
        };
    };

    void collide_airflow(AirFlow *airflow) {
        if(x+50.0f>airflow->x && x<airflow->x+60.0f) {
            if(y+50.0f>airflow->y && y<airflow->y+airflow->height) {
                velocity_y += airflow->velocity;
                is_jumping = true;
            };
        };
    };

    void collide_fire(FireZone *fire, Level &level) {
        if (distance(fire) < 50.0f && fire->active) {
            string death_type = "fire";
            update_status(level, death_type);
        };
    };

    void death_way(Level &level) {
        string death_type = "";
        //Fall damage
        if (y_start < 300.0f && y+50.0f >= level.entities[0]->y) {
            death_type = "fall";
            update_status(level, death_type);
        } else if (x>screenWidth || x+50.0f<0) {
            //Cross border
            death_type = "border";
            update_status(level, death_type);
        } else if(y<-150.0f) {
            //Fly away the up border
            death_type = "fly away";
            update_status(level, death_type);
        };
    };

    void reset(vector<float> pos) {
        *this = Player{pos[0],pos[1]};
    };

    void update_status(Level &level, string &type) {
        reset(level.player_pos);
        level.update(type);
    };
  };

  //Variables
  Menu main_menu = {};
  MusicPlayer main_music = {};
  main_music.play();
  Player player = {100.0f, screenHeight-75.0f};
  player.y_start = player.y;
  Platform floor = {0.0f,screenHeight-25.0f,screenWidth};
  Platform roof = {0.0f,-25.0f,screenWidth};

  //Level 0
  Wall wall3 = {0.0f,0.0f,screenHeight};
  Wall wall4 = {screenWidth-50.0f,0.0f,screenHeight};
  Obstacle pike0 = {400.0f,screenHeight-50.0f,"spike-v"};
  //Level 1
  Platform platform1 = {200.0f,400.0f,300.0f};
  platform1.x = 200.0f;
  platform1.y = 400.0f;
  Platform platform2 = {350.0f,250.0f,300.0f};
  platform2.x = 350.0f;
  platform2.y = 250.0f;
  //Level 2
  Platform platform3 = {200.0f,400.0f,200.0f};
  Platform platform4 = {500.0f,250.0f,150.0f};
  Wall wall1 = {500.0f,300.0f,300.0f};
  Wall wall2 = {200.0f,100.0f,300.0f};
  Obstacle pike1 = {screenWidth-25.0f,75.0f,"spike-h"};
  Obstacle pike2 = {300.0f,screenHeight-50.0f,"spike-v"};
  //Level 3
  Platform movingplatform1 = {200.0f,250.0f,300.0f};
  Obstacle lava1 = {0.0f,screenHeight-50.0f,"lava"};
  lava1.obstacle_length = screenWidth;
  movingplatform1.is_moving = true;
  movingplatform1.x_start = movingplatform1.x;
  Platform platform5 = {40.0f,40.0f,150.0f};
  platform5.color = {74,145,158,255};
  //Level 4
  FireZone fire1 = {300.0f,screenHeight-40.0f};
  FireZone fire2 = {350.0f,screenHeight-40.0f};
  Button button1 = {screenWidth-200.0f,195.0f, GREEN};
  Button button2 = {screenWidth-100.0f,screenHeight-35.0f,YELLOW};
  button2.visible = false;
  Platform platform6 = {screenWidth-300.0f,200.0f,300.0f};
  AirFlow airflow1 = {200.0f, 250.0f,250.0f};
  //Level 5 
  Platform movingplatform2 = {150.0f,150.0f,400.0f};
  Wall wall5 = {300.0f,200.0f,500.0f};
  Button button3 = {500.0f, screenHeight-35.0f,GREEN};
  Button button4 = {400.0f,screenHeight-35.0f,YELLOW};
  button4.visible = false;
  AirFlow airflow2 = {150.0f, 300.0f,150.0f};
  movingplatform2.is_moving = true;
  movingplatform2.x_start = movingplatform2.x;
  FireZone fire3 = {550.0f,screenHeight-40.0f};
  FireZone fire4 = {600.0f,screenHeight-40.0f};
  //Level 6 
  Saw saw = {250.0f,screenHeight-25.0f,50.0f};
  Saw saw1 = {550.0f,screenHeight-25.0f,50.0f};
  Saw saw2 = {450.0f,screenHeight-25.0f,50.0f};
  Saw saw3 = {350.0f,screenHeight-25.0f,50.0f};
  Button button5 = {650.0f,screenHeight-35.0f,BLUE};
  Platform platform7 = {50.0f,150.0f,100.0f};
  Platform platform8 = {350.0f,350.0f, 100.0f};
  platform7.visible = false;
  platform8.color = {74,145,158,255};
  //Level 7
  Bomb bomb = {450.0f,0.0f,10.0f,10.0f};
  Button button6 = {400.0f,screenHeight-35.0f,RED};
  Button button7 = {75.0f,300.0f,BLUE};
  Saw saw4 = {screenWidth-75.0f,250.0f,25.0f};
  Wall wall6 = {500.0f,200.0f,300.0f};
  Obstacle lava2 = {200.0f,screenHeight-30.0f,"lava"};
  lava2.obstacle_length = 100.0f;
  Platform platform9 = {50.0f,300.0f,100.0f};
  Platform platform10 = {550.0f,350.0f,100.0f};
  Platform platform11 = {200.0f,150.0f,200.0f};

  Level level0  = {0,  {200.0f, screenHeight-75.0f}, {&floor,&pike0,&wall3,&wall4}, 1};
  Level level1  = {1,  {100.0f, screenHeight-75.0f}, {&floor,&platform1,&platform2,&wall3,&roof}, 2};
  Level level2  = {2,  {100.0f, screenHeight-75.0f}, {&floor,&platform3,&platform4,&pike1,&pike2,&wall1,&wall2}, 4};
  Level level3  = {3,  {400.0f, 200.0f}, {&movingplatform1,&platform5,&lava1,&wall3,&wall4}, 2};
  Level level4  = {4,  {100.0f, screenHeight-75.0f}, {&floor,&roof,&button1,&button2,&fire1,&fire2,&wall3,&wall4,&platform6,&airflow1}, 3};
  Level level5  = {5,  {100.0f, screenHeight-75.0f}, {&floor,&roof,&movingplatform2,&wall5,&button4,&button3,&airflow2,&wall3,&wall4,&fire3,&fire4}, 3};
  Level level6  = {6,  {100.0f, screenHeight-75.0f}, {&saw,&saw1,&saw2,&saw3,&floor,&wall3,&button5,&platform7,&platform8}, 4};
  Level level7  = {7,  {100.0f, screenHeight-75.0f}, {&floor,&bomb,&button6,&wall3,&wall4,&lava2,&saw4,&button7,&wall6,&platform9,&platform10,&platform11}, 5};
  Level level8  = {8,  {100.0f, screenHeight-75.0f}, {&floor,&roof,&wall3,&wall4}, 0};

  vector<Level> levels = {
    level0, level1, level2, level3, level4,
    level5, level6, level7, level8
  };
  

  int level_id = 0;
  Level* level_selected = &levels[0];
  Color background_color = {74,145,158,255};

  while (!WindowShouldClose()) {
    if(main_menu.quit) {
        break;
    };
    main_music.update();
    //Check collisions
    if(main_menu.game) {
        player.move();
        player.death_way(*level_selected);
        for (auto *entity : level_selected->entities) {
            if (typeid(*entity) == typeid(Platform)) {
                player.collide_platform(static_cast<Platform*>(entity),levels[level_id]);
                entity->moving();
            } else if (typeid(*entity) == typeid(Obstacle)) {
                player.collide_obstacle(static_cast<Obstacle*>(entity), levels[level_id]);
            } else if (typeid(*entity) == typeid(Wall)) {
                player.collide_wall(static_cast<Wall*>(entity));
            } else if (typeid(*entity) == typeid(Button)) {
                Button* btn = static_cast<Button*>(entity);
                bool press = player.press_button(btn);
                if (press && btn->visible) {
                    for (auto *entity2 : level_selected->entities) {
                        btn->update(entity2);
                    };
                };
            } else if (typeid(*entity) == typeid(FireZone)) {
                player.collide_fire(static_cast<FireZone*>(entity),levels[level_id]);
            } else if(typeid(*entity) == typeid(AirFlow)) {
                player.collide_airflow(static_cast<AirFlow*>(entity));
            } else if(typeid(*entity) == typeid(Saw)) {
                player.collide_saw(static_cast<Saw*>(entity),levels[level_id]);
            } else if(typeid(*entity) == typeid(Bomb)) {
                Bomb *bomb = static_cast<Bomb*>(entity);
                bomb->update();
                player.collide_bomb(bomb,levels[level_id]);
                for(auto *entity2 : level_selected->entities)  {
                    if(typeid(*entity2) == typeid(Platform)) {
                        bomb->collide(static_cast<Platform*>(entity2));
                    };
                };
            };
        };
    };

    level_selected = &levels[level_id];

    BeginDrawing();
    ClearBackground(background_color);

    if(!main_menu.game) {
        main_menu.draw();
        main_menu.update();
        if(main_menu.options.active) {
            main_menu.options.update(&main_music.select);
        };
    } else {
        player.draw();
        level_selected->draw();
    };
    //Change the current level
    if (level_selected->nbr_lock <= (int)level_selected->lock.size() && level_id < levels.size()) {
        DrawText("Press the right arrow to access next level", 150, 200, 20, BLACK);
        if (IsKeyPressed(KEY_RIGHT)) {
            level_id += 1;
            player.reset(levels[level_id].player_pos);
        };
    };
    if(level_id+1 >= levels.size()) {
        DrawText("You have finished the game !",150.0f,200.0f,30.0f,BLACK);
        DrawText("Well Done",250.0f,250.0f,30.0f,BLACK);
    };
    EndDrawing();
  }
  main_music.unload();
  CloseAudioDevice();
  CloseWindow();
  return 0;
}