#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <time.h>
#include <deque>
#include <map>
#include <iostream>
using namespace sf;
// using namespace std;

int N=30,M=20;
int size=16;
int w = size*N;
int h = size*M;

int dir,num=4;

// leaderboard
std::vector<std::pair<std::string, int>> top3 = {
    {"Player1", -999},
    {"Player2", -999},
    {"Player3", -999},
};

std::deque<std::pair<int,int>> snake;

// snake cases
const int NORMAL = 0;
const int INVINCIBLE = 1; // when eats the invicibility fruit
const int POISONED = 2; // when eats the poisoned fruit

const int MAP_OFFSET_X = size * 8;

int snake_state = NORMAL;

struct Fruit { 
    int x,y;
    // 
    int type; // 0 normal fruit, 1 invicibility fruit, 2 poison, 3 anti-poison
} f;

const int LEFT = 1;
const int RIGHT = 2;
const int UP = 3;
const int DOWN = 0;

// a number between 0 and 1. 50% only for testing purposes
const double INVINCIBILITY_FRUIT_CHANCE = 50;
const double POISONED_FRUIT_CHANCE = 25;

const int NORMAL_FRUIT = 0;
const int INVICIBILITY_FRUIT = 1;
const int POISON_FRUIT = 2;
const int ANTI_POISON_FRUIT = 3;


// pause timer 
const double PAUSE_TIMER = 1;

// how long does invicibility lasts
const double INVINCIBILITY_DURATION = 5;

// how long does it take for the snake to die with the poison
const double POISON_DURATION = 5;

// 0: normal, 1: pause
int game_state = 1;

// when have we eaten the invincibility fruit
std::time_t invincibility_timestamp;

// when have we eaten the poison fruit
std::time_t poisoned_timestamp;

// relative positions for each move: UP -> {0, -1} | RIGHT -> {+1, 0}
std::map<int, std::pair<int, int>> dv;

// counts invincibility seconds or duration of being poisoned
int counter = 0;

// invincibility is 20% faster than normal delay
const double INVINCIBILITY_RATE = 0.8;

// frame delay when snake is in normal state
const double NORMAL_DELAY = 0.1;

std::string read_name() {
    // quits the game and saves the best results
    sf::RenderWindow window({ 500 , 100}, "Congratz on entering the leaderboard! Please type in your name!");

    std::string input_text;
    sf::Font font;
    font.loadFromFile("assets/fonts/Pacifico-Regular.ttf");
    sf::Text text("", font);
    Text welcome("Congratz on entering the leaderboard! Please type in your name!", font);
    sf::Clock clock;
    window.draw(welcome);
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::TextEntered) {
                if (std::isprint(event.text.unicode))
                    input_text += event.text.unicode;
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::BackSpace) {
                    if (!input_text.empty())
                        input_text.pop_back();
                }
                if (event.key.code == sf::Keyboard::Return) {
                        return input_text;
                }
            }
        }

        static sf::Time text_effect_time;
        static bool show_cursor;

        text_effect_time += clock.restart();

        if (text_effect_time >= sf::seconds(0.5f))
        {
            show_cursor = !show_cursor;
            text_effect_time = sf::Time::Zero;
        }

        text.setString(input_text + (show_cursor ? '_' : ' '));

        window.clear();
        window.draw(text);
        window.display();
    }
    return input_text;
}

void reset_game() {
    snake.clear();
    snake_state = NORMAL;
    snake.push_back({0, 0});
    snake.push_back({0, 1});
    snake.push_back({0, 2});
    f.type = NORMAL;
    // generate position of next fruit
    // make sure it doesn't overlap with a snake cell
    int x, y;
    while (true) {
        x = rand() % N;
        y = rand() % M;
        bool overlap = false;
        for (auto p : snake) {
            overlap |= (p.first == x && p.second == y);
        }
        if (!overlap)
            break;
    }
    // reset game state to paused
    game_state = 1;
}

void quit_game(RenderWindow& main_window, Sound &s_invincibility, Sound &s_poison) {
    int current_score = snake.size();
    s_invincibility.stop();
    s_poison.stop();
    // current score deserves to be on the leaderboard
    if (current_score > top3.back().second) {
        // add name and score to the leaderboard
        // launches new window to read new name
        std::string name = read_name();
        std::cout << "New typed name " << name << "\n";
        std::vector<std::pair<std::string, int>>::iterator it = top3.begin();
        while (it != top3.end() and it->second > current_score)
            ++it;
        top3.insert(it, std::make_pair(name, current_score));
        // it->first = name;
        // it->second = current_score;
        top3.pop_back();
    }
    reset_game();
}

void Tick(Sound &s_invincibility, Sound &s_bite, Sound &s_poison, RenderWindow& main_window)
 {

    snake.push_front(snake.front());

    // change position of head according to direction
    snake.front().first += dv[dir].first;
    snake.front().second += dv[dir].second;

    // eat fruit: snake deque
    if ((snake.front().first == f.x) && (snake.front().second == f.y)) {
        // update the state of the snake
        switch (f.type) {
            case INVICIBILITY_FRUIT:
                snake_state = INVINCIBLE;
                std::cout << "eating invincibility fruit!\n";
                // save invincibility timer
                invincibility_timestamp = std::time(nullptr);
                s_invincibility.play();
                break;
            case POISON_FRUIT:
                // if snake is invincible poison has no effect
                if (snake_state == INVINCIBLE)
                    break;
                snake_state = POISONED;
                // save the time of eating the poison
                poisoned_timestamp = std::time(nullptr);
                s_poison.play();
                break;
            case ANTI_POISON_FRUIT:
                s_poison.stop();
                snake_state = NORMAL;
                break;
            case NORMAL_FRUIT:
                s_bite.play();
                break;
            default:
                break;
        }
        // generate position of next fruit
        // make sure it doesn't overlap with a snake cell
        int x, y;
        while (true) {
            x = rand() % N;
            y = rand() % M;
            bool overlap = false;
            for (auto p : snake) {
                overlap |= (p.first == x && p.second == y);
            }
            if (!overlap)
                break;
        }
        double fruit_chance = (rand() % 100);
        if (snake_state == POISONED) {
            f.type = ANTI_POISON_FRUIT;
        } else if ((snake_state != INVINCIBLE) and (fruit_chance >= INVINCIBILITY_FRUIT_CHANCE)) {
            f.type = INVICIBILITY_FRUIT;
        } else if (fruit_chance >= POISONED_FRUIT_CHANCE) {
            f.type = POISON_FRUIT;
        } else {
            f.type = NORMAL_FRUIT;
        }
        f.x = x;
        f.y = y;
        // std::cout << "fruit found\n";
    } else {
        // std::cout << "popping cuz no fruit\n";
        snake.pop_back();
    }
    if (snake_state == INVINCIBLE) {
        // if invincible the snake can cross walls
        if (snake.front().first > N)
            snake.front().first = 0;
        if (snake.front().first < 0)
            snake.front().first = N - 1;
        if (snake.front().second > M)
            snake.front().second = 0;
        if (snake.front().second < 0)
            snake.front().second = M - 1;
    } else {
        // snake dies if it hits walls when not invincible
        if (snake.front().first > N)
            quit_game(main_window, s_invincibility, s_poison);
        if (snake.front().first < 0)
            quit_game(main_window, s_invincibility, s_poison);
        if (snake.front().second > M)
            quit_game(main_window, s_invincibility, s_poison);
        if (snake.front().second < 0)
            quit_game(main_window, s_invincibility, s_poison);
    }

    if (snake.size() == 3)
        return;
    bool skipped_once = false;

    // skip collision detection when the snake is inviNcible
    if (snake_state == INVINCIBLE)
        return;

    // collision detection
    for (auto p : snake) {
        // std::cout << p.first << " " << p.second << std::endl;
        // skip head
        if (p.first == snake.front().first && p.second == snake.front().second) {
            if (skipped_once) {
                std::cout << "Collision!!" << std::endl;
                quit_game(main_window, s_invincibility, s_poison);
            }
            skipped_once = true;
            // std::cout << "Skipping front\n" << std::endl;
            continue;

        }
    }
 }

int main()
{  
    // relative vector movements
    dv[LEFT] = {-1, 0};
    dv[RIGHT] = {1, 0};
    dv[UP] = {0, -1};
    dv[DOWN] = {0, 1};



    Sound s_invincibility, s_bite, s_poison;

    Clock clock, pause_clock;

    srand(time(0));
    snake.push_back({0, 0});
    snake.push_back({0, 1});
    snake.push_back({0, 2});
    RenderWindow main_window(VideoMode(w, MAP_OFFSET_X + h), "Snake Game!");
    // map key press with relative movement vector
    
    // set the texts
    Text t_start, t_pause, t_exit, t_score;
    Text p1, p2, p3;
    Text leaderboard;
    Text t_counter;
    // top3 of the leaderboard
    std::vector<Text> t_top3 = {p1, p2, p3};
    Font font;

    // load font
    font.loadFromFile("assets/fonts/Pacifico-Regular.ttf");
    
    // set font to texts
    t_start.setFont(font);
    t_pause.setFont(font);
    t_exit.setFont(font);
    t_score.setFont(font);
    leaderboard.setFont(font);
    t_counter.setFont(font);

    for (Text &p : t_top3) {
        p.setFont(font);
    }


    Texture t1,t2, t3, t4;
    // fruits
    Texture t_normal_fruit, t_invincibility, t_anti_poison, t_poison;
    SoundBuffer sb_invicibility, sb_bite, sb_poison;
    t1.loadFromFile("assets/images/background.png");
    t2.loadFromFile("assets/images/red.png");
    t3.loadFromFile("assets/images/green.png");
    t4.loadFromFile("assets/images/poisoned.png");

    t_anti_poison.loadFromFile("assets/images/curefruit.png");
    t_normal_fruit.loadFromFile("assets/images/normalfruit.png");
    t_invincibility.loadFromFile("assets/images/invinciblefruit.png");
    t_poison.loadFromFile("assets/images/poisonfruit.png");

    sb_invicibility.loadFromFile("assets/sounds/invincible.wav");
    sb_bite.loadFromFile("assets/sounds/bite.wav");
    sb_poison.loadFromFile("assets/sounds/poison.wav");
    s_invincibility.setBuffer(sb_invicibility);
    s_bite.setBuffer(sb_bite);
    s_poison.setBuffer(sb_poison);

    Sprite sprite1(t1);
    Sprite sprite2(t2);
    Sprite invicibility_fruit(t3);
    Sprite poisoned_snake(t4);
    Sprite spr_anti_poison(t_anti_poison);
    Sprite spr_invincibility(t_invincibility);
    Sprite spr_normalfruit(t_normal_fruit);
    Sprite spr_poison(t_poison);

    float timer=0;
    
    f.x=10;
    f.y=10; 
    
    // game loop
    while (main_window.isOpen())
    {

        double delay = NORMAL_DELAY - (0.01 * (snake.size() / 5));

        // std::cout << "snake size " << snake.size() << std::endl;
        float time = clock.getElapsedTime().asSeconds();
        clock.restart();
        timer += time;

        Event e;
        while (main_window.pollEvent(e))
        {
            if (e.type == Event::Closed)      
                main_window.close();
            // you can play by pressing any key
            if (e.type == Event::KeyPressed)
                game_state = 0; 
            
        }

        // track timer for invincibility
        if (snake_state == INVINCIBLE) {
            s_poison.stop();
            double invincibility_time = std::time(nullptr) - invincibility_timestamp;
            counter = INVINCIBILITY_DURATION - invincibility_time;
            std::cout << "Invincibility time " << invincibility_time << std::endl;
            // make the snake normal again after being inviNcible for a certain time
            if (invincibility_time >= INVINCIBILITY_DURATION) {
                snake_state = NORMAL;
                s_invincibility.stop();
            }
        }
        if (snake_state == POISONED) {
            s_invincibility.stop();
            double poisoned_time = std::time(nullptr) - poisoned_timestamp;
            counter = POISON_DURATION - poisoned_time;

            std::cout << "Poisoned time " << poisoned_time << std::endl;
            if (poisoned_time >= POISON_DURATION) {
                std::cout << "You lost! You haven't eaten the antipoison in time!\n";
                quit_game(main_window, s_invincibility, s_poison);
            }
        }

        if (Keyboard::isKeyPressed(Keyboard::Left)) dir=1;   
        if (Keyboard::isKeyPressed(Keyboard::Right)) dir=2;    
        if (Keyboard::isKeyPressed(Keyboard::Up)) dir=3;
        if (Keyboard::isKeyPressed(Keyboard::Down)) dir=0;

        if (timer > delay and Keyboard::isKeyPressed(Keyboard::P)) {
            std::cout << "Button p is clicked\n";
            if (pause_clock.getElapsedTime().asSeconds() >= PAUSE_TIMER) {

                pause_clock.restart();
                // change the state of the game between paused and playing
                game_state = !game_state; 
            }
        }
        if (snake_state == INVINCIBLE) {
            if (timer >= (INVINCIBILITY_RATE * delay)) {
                timer = 0;
                // if game is not paused
                if (game_state == 0) {
                    Tick(s_invincibility, s_bite, s_poison, main_window);
                }
            }
        } else {
            if (timer >= delay) {
                timer = 0; 
                // if game is not paused
                if (game_state == 0) {
                    Tick(s_invincibility, s_bite, s_poison, main_window);
                }
            }
        }

        // clear main_window to draw new frame
        main_window.clear(Color(200,200,200,200));

            
        ///// draw texts //////

        // draw leaderboard //
        leaderboard.setString("Leaderboard");
        leaderboard.setPosition(10, 0);
        
        for (int i = 0; i < (int) top3.size(); ++i) {
            std::string cur = std::to_string(i + 1) + " - " + top3[i].first;
            cur += ": " + std::to_string(top3[i].second);
            t_top3[i].setString(cur);
            t_top3[i].setPosition(10, 30 * i);
            main_window.draw(t_top3[i]);
        }
        // t_start.setFillColor(Color::Red);
        // t_start.setPosition(0, 0);
        // t_start.setString(game_state == 0 ? "Playing" : "Paused");
        // t_pause.setString("Pause");
        // t_pause.setPosition(0, 30);
        // t_exit.setString("Exit");
        // t_exit.setPosition(0, 60);
        t_score.setString(std::to_string(snake.size()));
        t_score.setStyle(Text::Bold);
        t_score.setCharacterSize(100);
        t_score.setPosition(370, 0);
        t_counter.setCharacterSize(60);
        if (snake_state == INVINCIBLE) {
            t_counter.setFillColor(Color::Green);
        } else if (snake_state == POISONED) {
            t_counter.setFillColor(Color::Red);
        } else {
            t_counter.setFillColor(Color::White);
        }
        t_counter.setPosition(280, 30);
        t_counter.setString(std::to_string(counter));
        main_window.draw(t_start);
        main_window.draw(t_pause);
        main_window.draw(t_exit);
        main_window.draw(t_score);
        main_window.draw(t_counter);

        // draw white blocks that form the background
        for (int i=0; i<N; i++) {
            for (int j=0; j<M; j++)  { 
                sprite1.setPosition(i*size, MAP_OFFSET_X + j*size);  
                main_window.draw(sprite1); 
            }
        }
        // draw snake
        for (auto pos : snake) {
            // std::cout << pos.first << " "  << pos.second << std::endl;
            if (snake_state == INVINCIBLE) {
                invicibility_fruit.setPosition(pos.first * size, MAP_OFFSET_X + pos.second * size);
                main_window.draw(invicibility_fruit);
            } else if (snake_state == POISONED) {
                poisoned_snake.setPosition(pos.first * size, MAP_OFFSET_X +  pos.second * size);
                main_window.draw(poisoned_snake);
            } else if (snake_state == NORMAL) {
                sprite2.setPosition(pos.first * size, MAP_OFFSET_X + pos.second * size);
                main_window.draw(sprite2);
            }
        }
        // for (int i=0;i<num;i++) { 
        //     std::cout << s[i].x << " "  << s[i].y << std::endl;
        //     sprite2.setPosition(s[i].x*size, s[i].y*size);  
        //     main_window.draw(sprite2); 
        // }

        // draw fruit
        if (f.type == INVICIBILITY_FRUIT) {
            spr_invincibility.setPosition(f.x * size, MAP_OFFSET_X + f.y * size);
            main_window.draw(spr_invincibility);
        } else if (f.type == POISON_FRUIT) {
            spr_poison.setPosition(f.x*size, MAP_OFFSET_X + f.y*size);  
            main_window.draw(spr_poison);   
        } else if (f.type == ANTI_POISON_FRUIT) {
            spr_anti_poison.setPosition(f.x*size, MAP_OFFSET_X + f.y*size);  
            main_window.draw(spr_anti_poison);   
        } else {
            spr_normalfruit.setPosition(f.x*size, MAP_OFFSET_X + f.y*size);  
            main_window.draw(spr_normalfruit);   
        }

        // display newly drawn main_window
        main_window.display();
    }

    return 0;
}
