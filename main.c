#include <ncursesw/curses.h> // Include the ncurses library for wide characters support
#include <stdlib.h>
#include <unistd.h>
#include <locale.h>
#include <signal.h>
#include <time.h>
#include <omp.h>
#include "logger.h"

#define DELAY 15000
// #define DELAY 0

#define CHAR_LOWER_HALF "▄"
#define CHAR_UPPER_HALF "▀"
#define CHAR_FULL_BLOCK "█"
#define ALIVE_STRING "██"

/*
 * @struct Settings
   * @brief The settings of the game
 * @param pause: if true, the game will not be updated.
 * @param use_two_cells_per_block: if true, use double height -> 2 rows per line (nop colors can be used).
 * @param use_colors: if true, uses colors for cells (only with use_two_cells_per_block=true ).
 * @param show_info: if true, show the info box at bottom.
 * @param show_history: if true, show the history in the info box.
 * @param info_box_height: the height of the info-box at the bottom.
*/
typedef struct { 
    bool pause;  /* @brief if true, the game will not be updated.*/
    bool use_two_cells_per_block;  /* @brief if true, use double height -> 2 rows per line (nop colors can be used). */
    bool use_colors;  /* @brief if true, uses colors for cells (only with use_two_cells_per_block=true ). */
    bool show_info;  /* @brief if true, show the info box at bottom. */
    bool show_history;  /* @brief if true, show the history in the info box. */
    int info_box_height;  /* @brief the height of the info-box at the bottom. */
} Settings;

/*
 * @struct Cell
    * @brief A cell of the game.
* @param alive: if true, the cell is alive.
* @param alive_for_iterations: the count of the iterations the cell is alive.
**/
typedef struct {
    bool alive;  /* if true, the cell is alive. */
    int alive_for_iterations;  /* the count of the iterations the cell is alive. */
} Cell;

/*
 * @struct History
 * @brief The history of the game.
 * @param calc_time_history: List of the last history_size calc times.
 * @param calc_time_history_all: List of all calc_times. Size increases over time.
 * @param history_max_size: Save the max history size for calc_time_history_all.
 * @param history_size: The calc_time_history && the size increase for history_max_size if full.
 * @param info_box_height: The height of the graph will be the info_box_height - 2.
 * @param free_history: Pointer to the free function.
**/
typedef struct History {
    double *calc_time_history;  /* @brief List of the last history_size calc times. */
    double *calc_time_history_all;  /* @brief List of all calc_times. Size increases over time */
    int history_max_size;  /* @brief Save the max history size for calc_time_history_all. */
    int history_size;  /* @brief The calc_time_history && the size increase for history_max_size if full. */
    // the height of the graph will be the info_box_height - 2

    // Functions:
    void (*free_history)(struct History*); /* @brief Pointer to the free function. */
} History;

/*
 * @struct GameOfLife
    * @brief The game of life.
* @param game_window: The window of the game.
* @param info_box: The info box at the bottom.
* @param cells: The cells of the game.
* @param settings: The settings of the game.
* @param history: The history of the game.
* @param width: The width of the game window.
* @param height: The height of the game window.
* @param last_calc_time: The last calculation time.
* @param count_circles: The count of the cicles.
* @param avg_calc_time: The average calculation time.
**/
typedef struct GameOfLife{
    WINDOW *game_window;
    WINDOW *info_box;
    Cell **cells;
    Settings *settings;
    History *history;
    int width;
    int height;
    double last_calc_time;
    int count_circles;
    double avg_calc_time;

    // Functions:
    void (*update_game_x_y)(struct GameOfLife*);  /* @brief Updates the width and height of the game window. */
    void (*free_game)(struct GameOfLife*);  /* @brief Frees the game. */
    void (*update_cells)(struct GameOfLife*);  /* @brief Updates the cells of the game. */
    void (*handle_resize)(struct GameOfLife*);  /* @brief Handles the resize of the game window. */
    void (*draw_game_field)(struct GameOfLife*);  /* @brief Draws the game field. */
    void (*draw_info_box)(struct GameOfLife*);  /* @brief Draws the info box. */
    void (*handle_key_input)(struct GameOfLife*, bool*);  /* @brief Handles the key input. */
    void (*update_history)(struct GameOfLife*);  /* @brief Updates the history. */
} GameOfLife;

/*
 * Updates the width and height of the game window.
 * @param game: the game to update.
**/
void update_game_x_y(GameOfLife *game) {
    if (game == NULL) return;
    getmaxyx(stdscr, game->height, game->width);  // Update the height and width of the game window
    wresize(game->game_window, game->height, game->width);
    wresize(game->info_box, game->settings->info_box_height, game->width);
    mvwin(game->info_box, game->height - game->settings->info_box_height, 0);

    if (game->settings->use_two_cells_per_block == true)
        game->height *= 2;
    else
        game->width /= 2;
}

/*
 * Frees the history.
 * @param history: the history to free.
**/
void free_history(History *history){
    if (history == NULL) return;
    if (history->calc_time_history != NULL) free(history->calc_time_history);
    if (history->calc_time_history_all != NULL) free(history->calc_time_history_all);
    free(history);
}

/*
 * Creates a new history.
 * @param size: the size of the history.
 * @return the new history.
**/
History* create_history(int size) {
    if (size <= 10){
        log_error("History size must be greater than 10");
        return NULL;
    }
    History *history = calloc(1, sizeof(History));
    history->history_size = size;
    history->history_max_size = size;
    history->calc_time_history = calloc(size, sizeof(double));
    history->calc_time_history_all = calloc(size, sizeof(double));
    history->free_history = free_history;
    return history;
}

/*
 * Creates the settings for the game.
 * The settings can be set with the following options:
 * - [-2]: Display two cells per block.
 * - [-nc]: No colors will be used.
 * - [-nh]: Do not show history.
 * - [-ni]: Do not show info at start.
 * - [-h]: Show the help.
 * @param argc: the number of arguments.
 * @param argv: the arguments.
 * @return the settings for the game.
**/
Settings* create_settings(int argc, char *argv[]) {
    Settings *settings = calloc(1, sizeof(Settings));
    // Default settings
    settings->use_colors = true;
    settings->use_two_cells_per_block = false;
    settings->show_history = true;
    settings->show_info = true;
    settings->info_box_height = 10;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-2") == 0) settings->use_two_cells_per_block = true;
        else if (strcmp(argv[i], "-nc") == 0) settings->use_colors = false;
        else if (strcmp(argv[i], "-nh") == 0) settings->show_history = false;
        else if (strcmp(argv[i], "-ni") == 0) settings->show_info = false;
        else if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [-2] [-nc] [-nh] [-ni]\n", argv[0]);
            printf("Options:\n");
            printf("  -2 : Display two cells per block\n");
            printf("  -nc: No colors will be used\n");
            printf("  -nh: Do not show history\n");
            printf("  -ni: Do not show info at start\n");
            exit(0);
        }
        else {
            log_error("Unknown option: %s", argv[i]);
            exit(1);
        }
    }
    return settings;
}

/*
 * Frees the game.
 * @param game: the game to free.
**/
void free_game(GameOfLife *game){
    if (game == NULL) return;
    if (game->game_window != NULL) delwin(game->game_window);
    if (game->info_box != NULL) delwin(game->info_box);
    if (game->settings != NULL) free(game->settings);
    game->history->free_history(game->history);
    for (int i = 0; i < game-> height; i++) 
        free(game->cells[i]);
    free(game->cells);
    free(game);
}

/*
 * Updates the cells of the game.
 * The cells will be updated according to the rules of the game of life.
 * @param game: the game to update the cells for.
**/
void update_cells(GameOfLife *game) {
    if (game == NULL) return;

    // create a bool array to store the old cells state
    bool **old_cells = malloc(sizeof(bool *) * game->height);
    for (int i = 0; i < game->height; i++) {
        old_cells[i] = malloc(sizeof(bool) * game->width);
        for (int j = 0; j < game->width; j++)
            old_cells[i][j] = game->cells[i][j].alive;
    }
    //#pragma omp parallel for num_threads(16)
    for (int i = 0; i < game->height; i++) {
        for (int j = 0; j < game->width; j++) {
            int alive_neighbours = 0;
            for (int x = -1; x <= 1; x++) {
                for (int y = -1; y <= 1; y++) {
                    if (x == 0 && y == 0) continue;

                    int new_x = i + x;
                    int new_y = j + y;
                    if (new_x < 0 || new_x >= game->height || new_y < 0 || new_y >= game->width)
                        continue;

                    if (old_cells[new_x][new_y])
                        alive_neighbours++;
                }
            }
            if (game->cells[i][j].alive) {
                if (alive_neighbours < 2 || alive_neighbours > 3) {
                    game->cells[i][j].alive = false;
                    game->cells[i][j].alive_for_iterations = 0;
                } else {
                    game->cells[i][j].alive_for_iterations += 1;
                }
            }
            else {
                if (alive_neighbours == 3) {
                    game->cells[i][j].alive = true;
                    game->cells[i][j].alive_for_iterations += 1;
                }
            }
        }
    }

    // Free the old cells array
    for (int i = 0; i < game->height; i++)
        free(old_cells[i]);
    free(old_cells);
}

/*
 * Handles the resize of the game window.
 * The cells will be resized and the new cells will be initialized with random values.
 * @param game: the game to handle the resize for.
**/
void handle_resize(GameOfLife *game){
    if (game == NULL) return;
    int old_height = game->height;
    int old_width = game->width;
    update_game_x_y(game);

    // Check if the size has changed
    if (old_height == game->height && old_width == game->width)
        return;

    log_info("Size-update: (%dx%d)->(%dx%d)", old_height, old_width, game->height, game->width);

    // Resize the array of rows
    if (game->height > old_height) {
        game->cells = realloc(game->cells, sizeof(Cell *) * game->height);
        for (int i = old_height; i < game->height; i++) {
            game->cells[i] = malloc(sizeof(Cell) * game->width);
            for (int j = 0; j < game->width; j++) {
                game->cells[i][j].alive = rand() % 2 == 0;
                game->cells[i][j].alive_for_iterations = 0;
            }
        }
    }
    else if (game->height < old_height){
        for (int i = game->height; i < old_height; i++) {
            free(game->cells[i]);
        }
    }

    // Resize each row
    if (game->width > old_width) {
        for (int i = 0; i < game->height; i++) {
            game->cells[i] = realloc(game->cells[i], sizeof(Cell) * game->width);
            for (int j = old_width; j < game->width; j++) {
                game->cells[i][j].alive = rand() % 2 == 0;
                game->cells[i][j].alive_for_iterations = 0;
            }
        }
    }
    else if (game->width < old_width){
        for (int i = 0; i < game->height; i++)
            game->cells[i] = realloc(game->cells[i], sizeof(Cell) * game->width);
    }
}
/*
 * Returns the color of the cell. The color depends on the number of iterations the cell is alive.
 * @param cell: the cell to get the color for.
 * @return the color of the cell.
**/
int get_cell_color(Cell *cell) {
    if (cell == NULL) {
        log_error("Cell is NULL, return color 1.");
        return COLOR_PAIR(1);
    }
    if (cell->alive_for_iterations < 1) return COLOR_PAIR(1);
    else if (cell->alive_for_iterations < 10) return COLOR_PAIR(2);
    else if (cell->alive_for_iterations < 30) return COLOR_PAIR(3);
    else return COLOR_PAIR(4);
}

void draw_game_field(GameOfLife *game) {
    if (game == NULL) return;
    if (game->settings->use_two_cells_per_block == true){
        char *ch = " ";
        for (int i = 0; i < game->height / 2; i++) {
            for (int j = 0; j < game->width; j++) {
                if (!game->cells[i * 2][j].alive && !game->cells[i * 2 + 1][j].alive)
                    continue;

                ch = " ";
                if (game->cells[i * 2][j].alive && game->cells[i * 2 + 1][j].alive)
                    ch = CHAR_FULL_BLOCK;
                else if (game->cells[i * 2][j].alive)
                    ch = CHAR_UPPER_HALF;
                else if (game->cells[i * 2 + 1][j].alive)
                    ch = CHAR_LOWER_HALF;
                mvwprintw(game->game_window, i, j, "%s", ch);
            }
        }
    }
    else {
        int color_pair = 0;
        for (int i = 0; i < game->height; i++) {
            for (int j = 0; j < game->width; j++) {
                if (game->cells[i][j].alive){
                    if (game->settings->use_colors) {
                        color_pair = get_cell_color(&game->cells[i][j]);
                        wattron(game->game_window, color_pair);
                        mvwprintw(game->game_window, i, j * 2, "%s", ALIVE_STRING);
                        wattroff(game->game_window, color_pair);
                    } else
                        mvwprintw(game->game_window, i, j * 2, "%s", ALIVE_STRING);
                }
            }
        }
    }
}

/*
 * Calculates the average of the given array.
 * If one of the values is 0, the function will return 0.
 * @param arr: the array to calculate the average for.
 * @param size: the size of the array.
 * @return the average of the array.
**/
double calculate_average(double arr[], int size) {
  double sum = 0;
  for (int i = 0; i < size; i++) {
    if (arr[i] == 0) return 0; // if one of the values is 0, return 0
    sum += arr[i];
  }
  return sum / size;
}

/*
 * Downsamples the data and aggregates it.
 * The data will be divided into num_buckets buckets and the average of each bucket will be calculated.
 * @param data: the data to downsample and aggregate.
 * @param size: the size of the data.
 * @param num_buckets: the number of buckets to downsample to.
 * @return the aggregated data.
**/
double* downsample_and_aggregate(double data[], int size, int num_buckets) {
    int bucket_size = size / num_buckets;
    double * aggregated_data = calloc(num_buckets, sizeof(double));

    for (int i = 0; i < num_buckets; i++) {
        int start_index = i * bucket_size;
        aggregated_data[i] = calculate_average(data + start_index, bucket_size);
    }
    return aggregated_data;
}

/*
 * Draws the info box at the bottom of the screen.
 * @param game: the game to draw the info box for.
**/
void draw_info_box(GameOfLife *game) {
    if (game == NULL) return;
    box(game->info_box, 0, 0); // Draw a box around the hole info_window
    mvwprintw(game->info_box, 0, 1, "[i]");
    mvwprintw(game->info_box, 1, 1, "Game of Life");
    mvwprintw(game->info_box, 2, 1, "Grid: %dx%d (%d)", game->width, game->height, game->width * game->height);
    mvwprintw(game->info_box, 3, 1, "Last calculation time   : %.6f sec", game->last_calc_time);
    mvwprintw(game->info_box, 4, 1, "Average calculation time: %.6f sec", game->avg_calc_time);
    mvwprintw(game->info_box, 5, 1, "Cicles: %d", game->count_circles);
    mvwprintw(game->info_box, game->settings->info_box_height - 3, 1, "[q]uit [r]eset [p]ause");
    mvwprintw(game->info_box, game->settings->info_box_height - 2, 1, "[c]olors [h]istory [2]mode");


    if (!game->settings->show_history) return; // Do not show the history

    
    double *total_history = downsample_and_aggregate(game->history->calc_time_history_all,
                                                     game->history->history_max_size,
                                                     game->history->history_size);

    double *graph_data[2] = {game->history->calc_time_history, total_history}; // have different index calc in the loop
    int graph_height = game->settings->info_box_height - 2;
    int graph_width = game->history->history_size;
    int j_offset = 40; // The starting offset to the lest of the graphs
    int min_graph_width = 8;  // Min width to show a graph
    for (int k = 0; k < 2; k++){
        // Break if the graph is too wide, 15 is the minimum width of the graph
        if (j_offset + 15 >= getmaxx(stdscr)) break;
        double *data = graph_data[k];

        // Calculate the maximum and minimum calc times
        double max_calc_time = data[0];
        double min_calc_time = data[0];
        for (int i = 1; i < graph_width; i++) {
            if (data[i] > max_calc_time)
                max_calc_time = data[i];
            if (data[i] < min_calc_time)
                min_calc_time = data[i];
        }

        // Calculate the scaling factors for the calc times
        double calc_time_range = max_calc_time - min_calc_time;
        double calc_time_scale = calc_time_range / graph_height;

        // Draw the graph
        for (int i = 0; i < graph_height; i++) {
            // Calculate the time value for the current row
            double time_value = min_calc_time + (graph_height - i - 0.5) * calc_time_scale;
            mvwprintw(game->info_box, i + 1, j_offset, "%.6f", time_value);

            for (int j = 0; j < graph_width; j++) {
                // Break if the graph is too wide
                if (j + j_offset + min_graph_width >= getmaxx(stdscr) - 1) break;

                // Calculate the index of the calc time in the history array
                int index = j;
                if (k == 0) index = (game->count_circles - graph_width + j) % graph_width;

                // Calculate the scaled calc time
                double scaled_calc_time = (data[index] - min_calc_time) / calc_time_scale;

                // Draw a dot if the scaled calc time is within the current row
                if (scaled_calc_time >= graph_height - i - 1 && scaled_calc_time < graph_height - i)
                    mvwprintw(game->info_box, i + 1, j + j_offset + 8, "•");
            }
        }

        j_offset += graph_width + 10; // offset for the next graph
    }
    free(total_history);
}

/*
 * Handles the key input. The following keys are supported:
 * - [q]uit, [p]ause, [i]nfo, [c]olors, [h]istory, [2]mode, [r]eset
 * @param game: the game to handle the input for.
 * @param running: the running flag. if set to false, the game will stop.
**/
void handle_key_input(GameOfLife *game, bool *running) {
    int ch = getch();
    switch (ch) {
        case 'q':
            *running = false;
            break;
        case 'p':
            game->settings->pause = !game->settings->pause;
            break;
        case 'i':
            game->settings->show_info = !game->settings->show_info;
            break;
        case 'c':
            game->settings->use_colors = !game->settings->use_colors;
            break;
        case 'h':
            game->settings->show_history = !game->settings->show_history;
            break;

        case '2':
            game->settings->use_two_cells_per_block = !game->settings->use_two_cells_per_block;
            break;
        case 'r':
            for (int i = 0; i < game->height; i++) {
                for (int j = 0; j < game->width; j++) {
                    game->cells[i][j].alive = rand() % 2 == 0;
                    game->cells[i][j].alive_for_iterations = 0;
                }
            }
            game->count_circles = 0;
            game->last_calc_time = 0;
            game->avg_calc_time = 0;
            // Reset the history
            int old_history_size = game->history->history_size;
            game->history->free_history(game->history);
            game->history = create_history(old_history_size);
            break;
    }
}

/*
 * Updates the history. The history will be updated with the last calculation time.
 * If the history is full, the size will be increased.
 * @param game: the game to update the history for.
**/
void update_history(GameOfLife *game){
    History *h = game->history;
    if (h == NULL) return;

    h->calc_time_history[game->count_circles % h->history_size] = game->last_calc_time;
    h->calc_time_history_all[game->count_circles] = game->last_calc_time;
    if (game->count_circles - h->history_max_size + 1 == 0) {
        h->history_max_size += h->history_size;
        double *new_array = calloc(h->history_max_size, sizeof(double));
        memcpy(new_array, h->calc_time_history_all, (h->history_max_size - h->history_size) * sizeof(double));
        free(h->calc_time_history_all);
        h->calc_time_history_all = new_array;
    }
}

/*
 * Creates a new game of live. The cells will be initialized with random values.
 * The setttings can be NULL, then default settings will be used (created with calloc, so all false).
 * Height and width will be set to the size of the terminal and ajusted to the settings.
 * @param settings: the settings of the game, if NULL, default settings will be used.
 * @return the new game of life.
**/
GameOfLife* create_game(Settings *settings) {
    GameOfLife *game = calloc(1, sizeof(GameOfLife));
    if (settings != NULL) game->settings = settings;
    else game->settings = calloc(1, sizeof(Settings));

    game->game_window = newwin(0, 0, 0, 0);
    game->info_box = newwin(game->settings->info_box_height, 0, 0, 0);

    update_game_x_y(game);

    game->cells = malloc(sizeof(Cell *) * game->height);
    for (int i = 0; i < game->height; i++) {
        game->cells[i] = malloc(game->width *  sizeof(Cell));
        for (int j = 0; j < game->width; j++){
            game->cells[i][j].alive = rand() % 2 == 0;
            game->cells[i][j].alive_for_iterations = 0;
        }
    }
    game->history = create_history(100);

    // Add functions to the game
    game->update_game_x_y = update_game_x_y;
    game->free_game = free_game;
    game->update_cells = update_cells;
    game->handle_resize = handle_resize;
    game->draw_game_field = draw_game_field;
    game->draw_info_box = draw_info_box;
    game->handle_key_input = handle_key_input;
    game->update_history = update_history;
    return game;
}

/*
 * Initializes the color pairs.
 * If the terminal does not support colors, the function will return 0.
 * @return 1 if the color pairs are initialized, 0 otherwise.
**/
int innit_color_pairs(){
    if (has_colors() == FALSE) {
        log_error("Your terminal does not support color!");
        printw("Your terminal does not support color\n");
        return 0;
    }
    start_color();
    // Define the color pairs
    init_pair(1, COLOR_RED, COLOR_WHITE);
    init_pair(2, COLOR_GREEN, COLOR_WHITE);
    init_pair(3, COLOR_BLUE, COLOR_WHITE);
    init_pair(4, COLOR_YELLOW, COLOR_WHITE);
    return 1;
}

int main(int argc, char *argv[]) {
    log_info("[=============| START |=============]");
    Settings *settings = create_settings(argc, argv);
    if (settings->use_two_cells_per_block == true && settings->use_colors == true)
        log_error("Two cells per block cannot display colors.");

    setlocale(LC_CTYPE, "");  // Activate UTF-8 support for the terminal, must be called before initscr()
    WINDOW *win = initscr();  // Initialize the curses library and the standard screen
    nodelay(win, TRUE);  // Makes the getch() non-blocking, getch is used for input
    curs_set(FALSE);  // Don't show the cursor
    noecho();  // Don't show the input

    // Start colors, if terminal does not support colors, exit the main.
    if(!innit_color_pairs()) {
        endwin();
        return EXIT_FAILURE;
    }

    GameOfLife *game = create_game(settings);
    double start_time = 0;
    //for (int i = 0; i < 10; i++) {
    bool running = true;
    while (running) {
        start_time = omp_get_wtime();
        game->handle_resize(game);
        if (!game->settings->pause)
            game->update_cells(game);

        // Draw the game field
        wclear(game->game_window);
        game->draw_game_field(game);
        wrefresh(game->game_window);

        // Draw the info box
        if (game->settings->show_info) {
            wclear(game->info_box);
            game->draw_info_box(game);
            wrefresh(game->info_box);
        }

        // Update the last calculation time
        game->last_calc_time = omp_get_wtime() - start_time;
        if (!game->settings->pause) {
            game->update_history(game);
            game->count_circles++;
            game->avg_calc_time = (game->avg_calc_time * (game->count_circles - 1) + game->last_calc_time) / game->count_circles;
        }

        game->handle_key_input(game, &running);
        
        usleep(DELAY);
    }
    game->free_game(game);
    delwin(win);
    endwin();
    return EXIT_SUCCESS;
}