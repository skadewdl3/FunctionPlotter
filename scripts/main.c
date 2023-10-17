#include <SDL.h>
#include <stdbool.h>
#include <math.h>

#define WIDTH 800
#define HEIGHT 800
#define STEP_X 50
#define STEP_Y 50
#define ORIGIN_X WIDTH / 2
#define ORIGIN_Y HEIGHT / 2

// Forward declaration for Quad struct
typedef struct QuadStruct Quad;
typedef struct ContourStruct Contour;

// Enum of directions - ne, nw, se, sw
typedef enum {
    nw = 0,
    ne = 1,
    sw = 2,
    se = 3,
    nodir = 4
} Direction;

// Enum of types of quads - tree, rant or none (catch-all type)
typedef enum {
    none = 0,
    tree = 1,
    rant = 2,
} QuadType;

// Struct for the quadtree
typedef struct {
    double x;
    double y;
    double width;
    double height;

    // Children of the quadtree can be either a quadtree or a quadrant
    // Hence used discriminated union Quad
    Quad* ne;
    Quad* nw;
    Quad* se;
    Quad* sw;
} QuadTree;

// Struct for the quadrant
// Quadrant cannot have any children. Quadrant is always a leaf node.
typedef struct {
    double x;
    double y;
    double width;
    double height;
    Contour* contour;
} Quadrant;

// Discriminated union for the quadtree and the quadrant
struct QuadStruct {
    union {
        QuadTree* tree;
        Quadrant* rant;
    } quad;
    QuadType type;
    // Put direction here as it is common to both quadtree and quadrant
    Direction dir;
};

// Contains contour coordinates
// basically, defines a line
struct ContourStruct {
    double x1;
    double y1;
    double x2;
    double y2;
};

// creates an empty quad
Quad* create_quad(Direction direction) {
    Quad* quad = (Quad*)malloc(sizeof(Quad));
    quad->quad.tree = NULL;
    quad->quad.rant = NULL;
    quad->type = none;
    quad->dir = direction;
    return quad;
}

void destroy_quad (Quad* quad) {
    if (quad == NULL) return;
    if (quad->type == tree) {
        destroy_quad(quad->quad.tree->ne);
        destroy_quad(quad->quad.tree->se);
        destroy_quad(quad->quad.tree->nw);
        destroy_quad(quad->quad.tree->sw);
        free(quad->quad.tree);
    }
    else if (quad->type == rant) {
        free(quad->quad.rant->contour);
        free(quad->quad.rant);
    }
}

// converts quad to quadtree by setting type = tree
// and allocating memory for the quadtree
QuadTree* to_quadtree (Quad* quad, double x, double y, double width, double height) {
    if (quad == NULL) {
        perror("\nExpected Quad* but got NULL.");
    }
    quad->type = tree;
    quad->quad.tree = (QuadTree*)malloc(sizeof(QuadTree));
    QuadTree* tree = quad->quad.tree;
    tree->x = x;
    tree->y = y;
    tree->width = width;
    tree->height = height;
    tree->ne = NULL;
    tree->se = NULL;
    tree->nw = NULL;
    tree->sw = NULL;
    return tree;
}

// converts quad to quadrant by setting type = rant
// and allocating memory for the quadrant
Quadrant* to_quadrant (Quad* quad, double x, double y, double width, double height) {
    if (quad == NULL) {
        perror("\nExpected Quad* but got NULL.");
    }
    quad->type = rant;
    quad->quad.tree = (QuadTree*)malloc(sizeof(Quadrant));
    Quadrant* qd = quad->quad.rant;
    qd->x = x;
    qd->y = y;
    qd->width = width;
    qd->height = height;
    qd->contour = NULL;
    return qd;
}

// traverses quadtree and draws out only the quadrants
// which contain the contours
void draw_quadrant (SDL_Renderer* renderer, Quad* quad) {
    if (quad == NULL) return;
    if (quad->type == tree) {
        draw_quadrant(renderer, quad->quad.tree->ne);
        draw_quadrant(renderer, quad->quad.tree->se);
        draw_quadrant(renderer, quad->quad.tree->nw);
        draw_quadrant(renderer, quad->quad.tree->sw);
    }
    else if (quad->type == rant) {
        Quadrant* qd = quad->quad.rant;
        if (!qd->contour) return;

        // Code for drawing contours

//        SDL_RenderDrawLineF(renderer, qd->contour->x1, qd->contour->y1, qd->contour->x2, qd->contour->y2);


        // Code for visualising the quadtree

        SDL_FRect rect;
        rect.x = (float)((qd->x) + 400);
        rect.y = (float)(-400 - (qd->y));
        rect.x = (float)qd->x;
        rect.y = (float)qd->y;
        rect.w = (float)qd->width;
        rect.h = (float)qd->height;
        SDL_RenderDrawRectF(renderer, &rect);
    }
}

// draws the quadtree recursively
void draw (SDL_Renderer* renderer, Quad* quad) {
    draw_quadrant(renderer, quad);
}

// equation to be graphed
double equation (double x, double y) {


    // scaled and translates the coordinates
    // according to the origin and step size
    x = (x - ORIGIN_X) / STEP_X;
    y = (ORIGIN_Y - y) / STEP_Y;

    // simply change this line to change the equation
    return x*sin(x) - y*cos(y);
}


// converts the binary value of the quadrant to a decimal value
short binary (double tl, double tr, double br, double bl) {
    return 8  * (tl > 0 ? 0 : 1) +
           4  * (tr > 0 ? 0 : 1) +
           2  * (br > 0 ? 0 : 1) +
           1  * (bl > 0 ? 0 :  1);
}


// returns the contour of the quadrant as per marching squares lookup table
Contour* get_contour(short bin, double x, double y, double width, double height) {
    Contour* contour = (Contour*)malloc(sizeof(Contour));
    // use marching squares lookup table to get contour
    switch (bin) {
        case 1:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width / 2;
            contour->y2 = y + height;
            break;
        case 2:
            contour->x1 = x + width / 2;
            contour->y1 = y + height;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 3:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 4:
            contour->x1 = x + width / 2;
            contour->y1 = y;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 5:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 6:
            contour->x1 = x + width / 2;
            contour->y1 = y;
            contour->x2 = x + width / 2;
            contour->y2 = y + height;
            break;
        case 7:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width / 2;
            contour->y2 = y + height;
            break;
        case 8:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width / 2;
            contour->y2 = y;
            break;
        case 9:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 10:
            contour->x1 = x + width / 2;
            contour->y1 = y;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 11:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        case 12:
            contour->x1 = x + width / 2;
            contour->y1 = y;
            contour->x2 = x + width / 2;
            contour->y2 = y + height;
            break;
        case 13:
            contour->x1 = x;
            contour->y1 = y + height / 2;
            contour->x2 = x + width / 2;
            contour->y2 = y + height;
            break;
        case 14:
            contour->x1 = x + width / 2;
            contour->y1 = y;
            contour->x2 = x + width;
            contour->y2 = y + height / 2;
            break;
        default:
            free(contour);
            contour = NULL;
    }
    return contour;
}


// Recursively subdivides the tree
// upto minimum depth min_depth
// and maximum depth max_depth
void divide_tree (Quad* quad, int depth, int min_depth, int max_depth) {

    if (max_depth < min_depth) {
        perror("\nMaximum depth cannot be less than minimum depth.");
        return;
    }

    // Get equation value at topleft,topright, bottomright, bottomleft points of the quadrant
    double tl = equation(quad->quad.tree->x, quad->quad.tree->y);
    double tr = equation(quad->quad.tree->x + quad->quad.tree->width, quad->quad.tree->y);
    double br = equation(quad->quad.tree->x + quad->quad.tree->width, quad->quad.tree->y + quad->quad.tree->height);
    double bl = equation(quad->quad.tree->x, quad->quad.tree->y + quad->quad.tree->height);

    // Get binary values of the quadrant from the equation values
    short bin = binary(tl, tr, br, bl);

    // Log them out
    printf("\n\nTL: %lf TR: %lf BR: %lf BL: %lf BIN: %d", tl, tr, br, bl, bin);
    fflush(stdout);

    // If depth has exceeded max_depth, create quadrants
    if (depth >= max_depth) {
        // create quadrants
        quad->quad.tree->nw = create_quad(nw);
        to_quadrant(quad->quad.tree->nw, quad->quad.tree->x, quad->quad.tree->y, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        Quadrant* nw = quad->quad.tree->nw->quad.rant;
        nw->contour = get_contour(bin, nw->x, nw->y, nw->width, nw->height);

        quad->quad.tree->ne = create_quad(ne);
        to_quadrant(quad->quad.tree->ne, quad->quad.tree->x + quad->quad.tree->width / 2, quad->quad.tree->y, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        Quadrant* ne = quad->quad.tree->ne->quad.rant;
        ne->contour = get_contour(bin, ne->x, ne->y, ne->width, ne->height);

        quad->quad.tree->se = create_quad(se);
        to_quadrant(quad->quad.tree->se, quad->quad.tree->x + quad->quad.tree->width / 2, quad->quad.tree->y + quad->quad.tree->height / 2, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        Quadrant* se = quad->quad.tree->se->quad.rant;
        se->contour = get_contour(bin, se->x, se->y, se->width, se->height);

        quad->quad.tree->sw = create_quad(sw);
        to_quadrant(quad->quad.tree->sw, quad->quad.tree->x, quad->quad.tree->y + quad->quad.tree->height / 2, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        Quadrant* sw = quad->quad.tree->sw->quad.rant;
        sw->contour = get_contour(bin, sw->x, sw->y, sw->width, sw->height);

        printf("\nNW coordinates: %lf %lf %lf %lf", nw->x, nw->y, nw->width, nw->height);
        printf("\nNE coordinates: %lf %lf %lf %lf", ne->x, ne->y, ne->width, ne->height);
        printf("\nSE coordinates: %lf %lf %lf %lf", se->x, se->y, se->width, se->height);
        printf("\nSW coordinates: %lf %lf %lf %lf", sw->x, sw->y, sw->width, sw->height);
    }

    // If depth has not reached min_depth, create quadtrees without
    // ignoring empty quadrants
    else if (depth < min_depth) {
        // create quadtrees
        quad->quad.tree->nw = create_quad(nw);
        to_quadtree(quad->quad.tree->nw, quad->quad.tree->x, quad->quad.tree->y, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        quad->quad.tree->ne = create_quad(ne);
        to_quadtree(quad->quad.tree->ne, quad->quad.tree->x + quad->quad.tree->width / 2, quad->quad.tree->y, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        quad->quad.tree->se = create_quad(sw);
        to_quadtree(quad->quad.tree->se, quad->quad.tree->x + quad->quad.tree->width / 2, quad->quad.tree->y + quad->quad.tree->height / 2, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        quad->quad.tree->sw = create_quad(se);
        to_quadtree(quad->quad.tree->sw, quad->quad.tree->x, quad->quad.tree->y + quad->quad.tree->height / 2, quad->quad.tree->width / 2, quad->quad.tree->height / 2);

        divide_tree(quad->quad.tree->ne, depth + 1, min_depth, max_depth);
        divide_tree(quad->quad.tree->nw, depth + 1, min_depth, max_depth);
        divide_tree(quad->quad.tree->se, depth + 1, min_depth, max_depth);
        divide_tree(quad->quad.tree->sw, depth + 1, min_depth, max_depth);
    }

    // If min_depth < depth < max_depth, create quadtrees
    // and ignore empty quadrants
    else {
        // create quadtrees if necessary as per binary value
        if (bin == 0 || bin == 15) return;
        quad->quad.tree->nw = create_quad(nw);
        to_quadtree(quad->quad.tree->nw, quad->quad.tree->x, quad->quad.tree->y, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        quad->quad.tree->ne = create_quad(ne);
        to_quadtree(quad->quad.tree->ne, quad->quad.tree->x + quad->quad.tree->width / 2, quad->quad.tree->y, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        quad->quad.tree->se = create_quad(sw);
        to_quadtree(quad->quad.tree->se, quad->quad.tree->x + quad->quad.tree->width / 2, quad->quad.tree->y + quad->quad.tree->height / 2, quad->quad.tree->width / 2, quad->quad.tree->height / 2);
        quad->quad.tree->sw = create_quad(se);
        to_quadtree(quad->quad.tree->sw, quad->quad.tree->x, quad->quad.tree->y + quad->quad.tree->height / 2, quad->quad.tree->width / 2, quad->quad.tree->height / 2);

        divide_tree(quad->quad.tree->ne, depth + 1, min_depth, max_depth);
        divide_tree(quad->quad.tree->nw, depth + 1, min_depth, max_depth);
        divide_tree(quad->quad.tree->se, depth + 1, min_depth, max_depth);
        divide_tree(quad->quad.tree->sw, depth + 1, min_depth, max_depth);
    }
}

int main(int argc, char* args []) {

    // Initialise SDL stuff
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Function Plotter", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        SDL_Log("Failed to create window: %s", SDL_GetError());
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_Log("Failed to create renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    bool running = true;
    SDL_Event event;


    // Create the base quadtree struct
    Quad* tree = create_quad(nodir);

    // Assert that it is a quadtree, so it'll be subdivided
    to_quadtree(tree, 0, 0, WIDTH, HEIGHT);

    // Recursively subdivide it
    // This only needs to be done once for any particular isovalue
    divide_tree(tree, 0, 5, 10);

    // Start the draw loop
    while (running) {
        // Close window with any input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }
        }

        // Set the background color to purple
        SDL_SetRenderDrawColor(renderer, 100, 100, 180, 255);
        // Clear the screen
        SDL_RenderClear(renderer);

        // draw x, y axes
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawLineF(renderer, 0, ORIGIN_Y, WIDTH, ORIGIN_Y);
        SDL_RenderDrawLineF(renderer, ORIGIN_X, 0, ORIGIN_X, HEIGHT);

        // Make draw color purple
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

        // Draw the quadtree (recursively)
        draw(renderer, tree);

        // Present the renderer
        SDL_RenderPresent(renderer);
    }


    // Clean up the quadtree
    destroy_quad(tree);


    // Clean up SDL Stuff
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
