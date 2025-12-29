#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <math.h>
#include <ctype.h>

typedef struct Node {
    char token[32];       // store token (operand or operator) as string
    struct Node* next;
} Node;

typedef struct Stack {
    Node* top;
    int size;
} Stack;

void init_stack(Stack* s) {
    s->top = NULL;
    s->size = 0;
}

int is_empty(Stack* s) {
    return s->top == NULL;
}

void push(Stack* s, const char* token) {
    Node* node = (Node*)malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        endwin();
        exit(EXIT_FAILURE);
    }
    strncpy(node->token, token, 31);
    node->token[31] = '\0';
    node->next = s->top;
    s->top = node;
    s->size++;
}

char* pop(Stack* s, int* error) {
    if (is_empty(s)) {
        *error = 1;
        return NULL;
    }
    Node* temp = s->top;
    char* val = strdup(temp->token);
    s->top = temp->next;
    free(temp);
    s->size--;
    *error = 0;
    return val;
}

char* peek(Stack* s, int* error) {
    if (is_empty(s)) {
        *error = 1;
        return NULL;
    }
    *error = 0;
    return s->top->token;
}

int is_operator_char(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '^');
}

int precedence(char c) {
    if (c == '^') return 3;
    if (c == '*' || c == '/') return 2;
    if (c == '+' || c == '-') return 1;
    return 0;
}

// Append a token to postfix with space, ensuring no overflow
void append_postfix(char* postfix, const char* token, int max_len) {
    if (strlen(postfix) + strlen(token) + 2 < (size_t)max_len) {
        strcat(postfix, token);
        strcat(postfix, " ");
    }
}

// Helper to print stack contents as space-separated tokens for infix->postfix trace display
void print_stack_content(Stack* s, char* buffer, int max_len) {
    buffer[0] = '\0';
    Node* curr = s->top;
    // Collect tokens from bottom to top for display to show stack bottom first
    // So first push tokens onto array
    #define MAX_STACK_DISPLAY 64
    char* tokens[MAX_STACK_DISPLAY];
    int count = 0;
    while (curr && count < MAX_STACK_DISPLAY) {
        tokens[count++] = curr->token;
        curr = curr->next;
    }
    // Print in reverse (bottom to top)
    for (int i = count - 1; i >= 0; i--) {
        if (strlen(buffer) + strlen(tokens[i]) + 2 > (size_t)max_len) break;
        strcat(buffer, tokens[i]);
        strcat(buffer, " ");
    }
}

// Display infix to postfix conversion stepwise in message window.
// User presses a key to proceed through each step.
void infix_to_postfix_stepwise(const char* infix, WINDOW* msg_win) {
    Stack op_stack;
    init_stack(&op_stack);

    char postfix[256] = "";
    int i = 0;
    int len = strlen(infix);

    char stackbuf[128];

    werase(msg_win);
    box(msg_win, 0, 0);
    wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
    int width = getmaxx(msg_win);
    mvwprintw(msg_win, 0, (width - 24)/2, " Infix to Postfix Trace ");
    wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);

    mvwprintw(msg_win, 1, 2, "Input infix: %s", infix);
    mvwprintw(msg_win, 3, 2, "Press any key to step through conversion.");
    wrefresh(msg_win);
    wgetch(msg_win);

    while (i < len) {
        // skip spaces
        while (i < len && isspace(infix[i])) i++;
        if (i >= len) break;

        // Determine token: operand (multi-char variable or number) or operator / parenthesis
        char token[32];
        int tlen = 0;

        if (isalpha(infix[i]) || isdigit(infix[i])) {
            // operand: accumulate letters/digits/dot (for float support)
            while (i < len && (isalnum(infix[i]) || infix[i] == '.')) {
                if (tlen < 31) token[tlen++] = infix[i];
                i++;
            }
            token[tlen] = '\0';
            // Append operand to postfix
            append_postfix(postfix, token, 255);

            // Update display
            werase(msg_win);
            box(msg_win, 0, 0);
            wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
            mvwprintw(msg_win, 0, (width - 24)/2, " Infix to Postfix Trace ");
            wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);

            mvwprintw(msg_win, 1, 2, "Read operand: %s", token);
            mvwprintw(msg_win, 2, 2, "Current postfix: %s", postfix);
            print_stack_content(&op_stack, stackbuf, sizeof(stackbuf));
            mvwprintw(msg_win, 3, 2, "Operator stack top-> %s", stackbuf);
            mvwprintw(msg_win, 5, 2, "Press any key to continue...");
            wrefresh(msg_win);
            wgetch(msg_win);
        } else {
            // operator or parenthesis
            char op = infix[i];
            i++;
            token[0] = op;
            token[1] = '\0';

            if (op == '(') {
                push(&op_stack, token);

                werase(msg_win);
                box(msg_win, 0, 0);
                wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(msg_win, 0, (width - 24)/2, " Infix to Postfix Trace ");
                wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(msg_win, 1, 2, "Push '(' onto operator stack.");
                print_stack_content(&op_stack, stackbuf, sizeof(stackbuf));
                mvwprintw(msg_win, 2, 2, "Operator stack top-> %s", stackbuf);
                mvwprintw(msg_win, 3, 2, "Current postfix: %s", postfix);
                mvwprintw(msg_win, 5, 2, "Press any key to continue...");
                wrefresh(msg_win);
                wgetch(msg_win);
            } else if (op == ')') {
                // Pop till '('
                int error;
                char* top_op = peek(&op_stack, &error);
                while (!error && top_op[0] != '(') {
                    char* popped = pop(&op_stack, &error);
                    append_postfix(postfix, popped, 255);
                    free(popped);
                    top_op = peek(&op_stack, &error);
                }

                if (!error && top_op[0] == '(') {
                    char* discarded = pop(&op_stack, &error);
                    free(discarded);
                } else {
                    // mismatch error
                    werase(msg_win);
                    box(msg_win, 0, 0);
                    wattron(msg_win, COLOR_PAIR(4) | A_BOLD);
                    mvwprintw(msg_win, 1, 2, "Error: mismatched parentheses detected.");
                    wattroff(msg_win, COLOR_PAIR(4) | A_BOLD);
                    wrefresh(msg_win);
                    wgetch(msg_win);
                    return;
                }

                werase(msg_win);
                box(msg_win, 0, 0);
                wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(msg_win, 0, (width - 24)/2, " Infix to Postfix Trace ");
                wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(msg_win, 1, 2, "Pop operators until '(' found and discard it.");
                print_stack_content(&op_stack, stackbuf, sizeof(stackbuf));
                mvwprintw(msg_win, 2, 2, "Operator stack top-> %s", stackbuf);
                mvwprintw(msg_win, 3, 2, "Current postfix: %s", postfix);
                mvwprintw(msg_win, 5, 2, "Press any key to continue...");
                wrefresh(msg_win);
                wgetch(msg_win);
            } else if (is_operator_char(op)) {
                int error;
                char* top_op = peek(&op_stack, &error);
                while (!error && top_op[0] != '(' &&
                      ((precedence(top_op[0]) > precedence(op)) ||
                      (precedence(top_op[0]) == precedence(op) && op != '^'))) {
                    char* popped = pop(&op_stack, &error);
                    append_postfix(postfix, popped, 255);
                    free(popped);
                    top_op = peek(&op_stack, &error);
                }
                push(&op_stack, token);

                werase(msg_win);
                box(msg_win, 0, 0);
                wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(msg_win, 0, (width - 24)/2, " Infix to Postfix Trace ");
                wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);
                mvwprintw(msg_win, 1, 2, "Push operator '%c' onto stack.", op);
                print_stack_content(&op_stack, stackbuf, sizeof(stackbuf));
                mvwprintw(msg_win, 2, 2, "Operator stack top-> %s", stackbuf);
                mvwprintw(msg_win, 3, 2, "Current postfix: %s", postfix);
                mvwprintw(msg_win, 5, 2, "Press any key to continue...");
                wrefresh(msg_win);
                wgetch(msg_win);
            } else {
                // Unknown token
                werase(msg_win);
                box(msg_win, 0, 0);
                wattron(msg_win, COLOR_PAIR(4) | A_BOLD);
                mvwprintw(msg_win, 1, 2, "Unknown token encountered: %c", op);
                wattroff(msg_win, COLOR_PAIR(4) | A_BOLD);
                wrefresh(msg_win);
                wgetch(msg_win);
            }
        }
    }

    // Pop remaining operators
    int error;
    while (!is_empty(&op_stack)) {
        char* popped = pop(&op_stack, &error);
        if (popped[0] == '(' || popped[0] == ')') {
            werase(msg_win);
            box(msg_win, 0, 0);
            wattron(msg_win, COLOR_PAIR(4) | A_BOLD);
            mvwprintw(msg_win, 1, 2, "Error: mismatched parentheses detected.");
            wattroff(msg_win, COLOR_PAIR(4) | A_BOLD);
            wrefresh(msg_win);
            free(popped);
            wgetch(msg_win);
            return;
        }
        append_postfix(postfix, popped, 255);
        free(popped);

        werase(msg_win);
        box(msg_win, 0, 0);
        wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(msg_win, 0, (width - 24)/2, " Infix to Postfix Trace ");
        wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);
        mvwprintw(msg_win, 1, 2, "Pop remaining operators:");
        print_stack_content(&op_stack, stackbuf, sizeof(stackbuf));
        mvwprintw(msg_win, 2, 2, "Operator stack top-> %s", stackbuf);
        mvwprintw(msg_win, 3, 2, "Current postfix: %s", postfix);
        mvwprintw(msg_win, 5, 2, "Press any key to continue...");
        wrefresh(msg_win);
        wgetch(msg_win);
    }

    // Final postfix expression display
    werase(msg_win);
    box(msg_win, 0, 0);
    wattron(msg_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(msg_win, 2, 2, "Final Postfix Expression:");
    wattroff(msg_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(msg_win, 3, 2, "%s", postfix);
    mvwprintw(msg_win, 5, 2, "Press any key to return to menu...");
    wrefresh(msg_win);
    wgetch(msg_win);
}

// Converts postfix to infix with stepwise display in msg_win
void postfix_to_infix_stepwise(WINDOW *msg_win) {
    Stack s;
    init_stack(&s);

    char input[256];
    char token[32];
    char stackbuf[128];
    int error;
    int i = 0, len;

    // Clear and prompt input
    werase(msg_win);
    box(msg_win, 0, 0);
    wattron(msg_win, COLOR_PAIR(3));
    mvwprintw(msg_win, 1, 2, "Enter postfix expression (tokens separated by spaces): ");
    wattroff(msg_win, COLOR_PAIR(3));
    wrefresh(msg_win);

    echo();
    wmove(msg_win, 2, 2);
    wclrtoeol(msg_win);
    wrefresh(msg_win);
    wgetnstr(msg_win, input, 255);
    noecho();

    len = strlen(input);

    i = 0;
    int step = 0;

    while (i < len) {
        // skip spaces
        while (i < len && isspace(input[i])) i++;
        if (i >= len) break;

        int tlen = 0;
        // read token (operand or operator)
        if (isalnum(input[i]) || input[i] == '.') {
            // operand token (letters/digits/dot)
            while (i < len && (isalnum(input[i]) || input[i] == '.')) {
                if (tlen < 31) token[tlen++] = input[i];
                i++;
            }
            token[tlen] = '\0';

            push(&s, token);
        } else {
            // operator token (assumed single char)
            token[0] = input[i];
            token[1] = '\0';
            i++;

            if (s.size < 2) {
                werase(msg_win);
                box(msg_win, 0, 0);
                wattron(msg_win, COLOR_PAIR(4) | A_BOLD);
                mvwprintw(msg_win, 1, 2, "Error: insufficient operands for operator '%s'", token);
                wattroff(msg_win, COLOR_PAIR(4) | A_BOLD);
                wrefresh(msg_win);
                wgetch(msg_win);
                // Clear stack and return
                while (!is_empty(&s)) { char* p = pop(&s, &error); if(p) free(p);}
                return;
            }

            // Pop two operands
            char* op2 = pop(&s, &error);
            char* op1 = pop(&s, &error);

            // Form new infix string "(op1 operator op2)"
            char newexpr[128];
            snprintf(newexpr, sizeof(newexpr), "(%s %s %s)", op1, token, op2);

            free(op1);
            free(op2);

            push(&s, newexpr);
        }

        step++;

        // Display step info
        werase(msg_win);
        box(msg_win, 0, 0);
        wattron(msg_win, COLOR_PAIR(5) | A_BOLD);
        int width = getmaxx(msg_win);
        mvwprintw(msg_win, 0, (width - 19)/2, " Postfix to Infix Trace ");
        wattroff(msg_win, COLOR_PAIR(5) | A_BOLD);

        mvwprintw(msg_win, 1, 2, "Step %d: Processed token '%s'", step, token);
        print_stack_content(&s, stackbuf, sizeof(stackbuf));
        mvwprintw(msg_win, 2, 2, "Stack top-> %s", stackbuf);
        mvwprintw(msg_win, 4, 2, "Press any key to continue...");
        wrefresh(msg_win);
        wgetch(msg_win);
    }

    if (s.size != 1) {
        werase(msg_win);
        box(msg_win, 0, 0);
        wattron(msg_win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(msg_win, 1, 2, "Error: invalid postfix expression, stack size not 1");
        wattroff(msg_win, COLOR_PAIR(4) | A_BOLD);
        wrefresh(msg_win);
        wgetch(msg_win);
        while (!is_empty(&s)) { char* p = pop(&s, &error); if(p) free(p);}
        return;
    }

    char* infix = pop(&s, &error);

    werase(msg_win);
    box(msg_win, 0, 0);
    wattron(msg_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(msg_win, 1, 2, "Postfix to Infix Conversion Complete");
    mvwprintw(msg_win, 3, 2, "Infix expression: %s", infix);
    wattroff(msg_win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(msg_win, 5, 2, "Press any key to return to menu...");
    wrefresh(msg_win);
    wgetch(msg_win);
    free(infix);
}

// Evaluate postfix expression with numeric tokens only
// return 1 on success, result filled, else 0
int evaluate_postfix_numeric(const char* postfix, double* result) {
    Stack s;
    init_stack(&s);

    int error;
    int len = strlen(postfix);
    int i = 0;

    while (i < len) {
        // skip spaces
        while (i < len && postfix[i] == ' ')
            i++;
        if (i >= len)
            break;

        char token[32];
        int tlen = 0;

        // read operand (number)
        if (isdigit(postfix[i]) || postfix[i] == '.') {
            while (i < len && (isdigit(postfix[i]) || postfix[i] == '.')) {
                if (tlen < 31) token[tlen++] = postfix[i];
                i++;
            }
            token[tlen] = '\0';
            push(&s, token);
        } else {
            // operator
            char op = postfix[i];
            i++;

            if (s.size < 2) {
                // insufficient operands
                return 0;
            }

            char* val2_s = pop(&s, &error);
            char* val1_s = pop(&s, &error);

            if (error) {
                free(val2_s);
                free(val1_s);
                return 0;
            }

            double val1 = atof(val1_s);
            double val2 = atof(val2_s);
            free(val1_s);
            free(val2_s);

            double res = 0;
            switch (op) {
                case '+': res = val1 + val2; break;
                case '-': res = val1 - val2; break;
                case '*': res = val1 * val2; break;
                case '/': if(val2 == 0) return 0; res = val1 / val2; break;
                case '^': res = pow(val1, val2); break;
                default: return 0;
            }

            char buffer[32];
            snprintf(buffer, 32, "%lf", res);
            push(&s, buffer);
        }
    }

    if (s.size != 1) return 0;

    char* res_s = pop(&s, &error);
    *result = atof(res_s);
    free(res_s);
    return 1;
}

// Display stack contents in UI stack window, supports tokens (strings)
void display_stack(Stack* s, WINDOW* win, int y_start, int x_start) {
    werase(win);
    box(win, 0, 0);
    wattron(win, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win, 0, 2, " Stack Contents ");
    wattroff(win, COLOR_PAIR(5) | A_BOLD);

    Node* curr = s->top;
    int y = y_start;
    int index = s->size;

    while(curr != NULL && y < getmaxy(win) - 1) {
        if ((index % 2) == 0) {
            wattron(win, COLOR_PAIR(8));
        } else {
            wattron(win, COLOR_PAIR(3));
        }
        if (index == s->size) {
            wattron(win, A_BOLD | A_UNDERLINE);
            mvwprintw(win, y, x_start, "#%d: %s  <- Top", index, curr->token);
            wattroff(win, A_BOLD | A_UNDERLINE);
        } else {
            mvwprintw(win, y, x_start, "#%d: %s", index, curr->token);
        }
        wattroff(win, COLOR_PAIR(8) | COLOR_PAIR(3));
        y++;
        curr = curr->next;
        index--;
    }
    wrefresh(win);
}

void draw_menu(WINDOW* win) {
    werase(win);
    box(win, 0, 0);
    int width = getmaxx(win);
    wattron(win, COLOR_PAIR(1) | A_BOLD | A_UNDERLINE);
    mvwprintw(win, 1, (width - 24) / 2, " Stack Machine Menu ");
    wattroff(win, COLOR_PAIR(1) | A_BOLD | A_UNDERLINE);

    wattron(win, COLOR_PAIR(2));
    mvwprintw(win, 3, 2, "1. Push Token (Number or Char)");
    mvwprintw(win, 4, 2, "2. Pop Token");
    mvwprintw(win, 5, 2, "3. Add (Top two)");
    mvwprintw(win, 6, 2, "4. Subtract (Top two)");
    mvwprintw(win, 7, 2, "5. Multiply (Top two)");
    mvwprintw(win, 8, 2, "6. Divide (Top two)");
    mvwprintw(win, 9, 2, "7. Infix to Postfix Conversion (Stepwise)");
    mvwprintw(win, 10, 2, "8. Evaluate Postfix (numeric only)");
    mvwprintw(win, 11, 2, "9. Postfix to Infix Conversion (Stepwise)");
    mvwprintw(win, 12, 2, "10. Exit");
    wattroff(win, COLOR_PAIR(2));

    wattron(win, A_BOLD | COLOR_PAIR(4));
    mvwhline(win, 14, 1, 0, width - 2);
    wattroff(win, A_BOLD | COLOR_PAIR(4));

    wattron(win, A_BOLD);
    mvwprintw(win, 15, 2, "Choose option (1-10): ");
    wclrtoeol(win);
    wattroff(win, A_BOLD);

    wrefresh(win);
}

void draw_msg_box(WINDOW* win) {
    werase(win);
    box(win, 0, 0);
    wattron(win, COLOR_PAIR(5) | A_BOLD);
    int width = getmaxx(win);
    mvwprintw(win, 0, (width - 10) / 2, " Messages ");
    wattroff(win, COLOR_PAIR(5) | A_BOLD);
    wrefresh(win);
}

void print_centered(WINDOW* win, int y, const char* str, int color_pair) {
    int width = getmaxx(win);
    int len = strlen(str);
    int x = (width - len) / 2;
    wattron(win, COLOR_PAIR(color_pair) | A_BOLD);
    mvwprintw(win, y, x, "%s", str);
    wattroff(win, COLOR_PAIR(color_pair) | A_BOLD);
    wrefresh(win);
}

void handle_user_option(Stack* stack, int option, WINDOW* msg_win, WINDOW* stack_win, char* input, char* postfix) {
    int error;
    double res;

    draw_msg_box(msg_win);

    switch(option) {
        case 1: // Push Token (number or char)
            werase(msg_win);
            box(msg_win, 0, 0);
            wattron(msg_win, COLOR_PAIR(3));
            mvwprintw(msg_win, 1, 2, "Enter number or character token to push: ");
            wattroff(msg_win, COLOR_PAIR(3));
            wrefresh(msg_win);

            wmove(msg_win, 2, 2);
            wclrtoeol(msg_win);
            wrefresh(msg_win);

            echo();
            wgetnstr(msg_win, input, 31);
            noecho();

            if (strlen(input) == 0) {
                wattron(msg_win, COLOR_PAIR(4));
                print_centered(msg_win, 3, "Empty input! Nothing pushed.", 4);
                wattroff(msg_win, COLOR_PAIR(4));
                wrefresh(msg_win);
                break;
            }

            int valid = 1;
            for (int i = 0; input[i]; i++) {
                if (!isalnum(input[i]) && input[i] != '.') {
                    valid = 0;
                    break;
                }
            }
            if (!valid) {
                wattron(msg_win, COLOR_PAIR(4));
                print_centered(msg_win, 3, "Invalid token! Use alphanumeric chars only.", 4);
                wattroff(msg_win, COLOR_PAIR(4));
                wrefresh(msg_win);
                break;
            }

            push(stack, input);
            wattron(msg_win, COLOR_PAIR(3));
            mvwprintw(msg_win, 3, 2, "Successfully pushed: %s", input);
            wattroff(msg_win, COLOR_PAIR(3));
            wrefresh(msg_win);
            break;

        case 2: // Pop Token
            {
                char* val = pop(stack, &error);
                if (error) {
                    wattron(msg_win, COLOR_PAIR(4));
                    print_centered(msg_win, 2, "Stack is empty. Cannot pop.", 4);
                    wattroff(msg_win, COLOR_PAIR(4));
                } else {
                    wattron(msg_win, COLOR_PAIR(3));
                    mvwprintw(msg_win, 2, 2, "Popped from stack: %s", val);
                    wattroff(msg_win, COLOR_PAIR(3));
                }
                wrefresh(msg_win);
                if(val) free(val);
            }
            break;

        case 3: // Add (Top two)
        case 4: // Subtract (Top two)
        case 5: // Multiply (Top two)
        case 6: // Divide (Top two)
            {
                if (stack->size < 2) {
                    wattron(msg_win, COLOR_PAIR(4));
                    print_centered(msg_win, 2, "Need at least 2 elements in stack!", 4);
                    wattroff(msg_win, COLOR_PAIR(4));
                    wrefresh(msg_win);
                    break;
                }
                char* a = pop(stack, &error);
                char* b = pop(stack, &error);
                if (error) {
                    wattron(msg_win, COLOR_PAIR(4));
                    print_centered(msg_win, 2, "Error performing operation.", 4);
                    wattroff(msg_win, COLOR_PAIR(4));
                    wrefresh(msg_win);
                    if(a) free(a);
                    if(b) free(b);
                    break;
                }
                double valA = atof(a);
                double valB = atof(b);
                int aIsNum = (strlen(a) > 0 && (isdigit(a[0]) || a[0] == '.'));
                int bIsNum = (strlen(b) > 0 && (isdigit(b[0]) || b[0] == '.'));

                if (!aIsNum || !bIsNum) {
                    char resStr[64];
                    switch(option) {
                        case 3: snprintf(resStr, sizeof(resStr), "(%s+%s)", b, a); break;
                        case 4: snprintf(resStr, sizeof(resStr), "(%s-%s)", b, a); break;
                        case 5: snprintf(resStr, sizeof(resStr), "(%s*%s)", b, a); break;
                        case 6: snprintf(resStr, sizeof(resStr), "(%s/%s)", b, a); break;
                        default: strcpy(resStr, "UNK"); break;
                    }
                    push(stack, resStr);
                    wattron(msg_win, COLOR_PAIR(3));
                    mvwprintw(msg_win, 2, 2, "Symbolic operation result: %s", resStr);
                    wattroff(msg_win, COLOR_PAIR(3));
                } else {
                    double res = 0;
                    switch(option) {
                        case 3: res = valB + valA; break;
                        case 4: res = valB - valA; break;
                        case 5: res = valB * valA; break;
                        case 6:
                            if(valA == 0) {
                                wattron(msg_win, COLOR_PAIR(4));
                                print_centered(msg_win, 2, "Error: Division by zero!", 4);
                                wattroff(msg_win, COLOR_PAIR(4));
                                push(stack, b);
                                push(stack, a);
                                free(a); free(b);
                                wrefresh(msg_win);
                                return;
                            }
                            res = valB / valA;
                            break;
                    }
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%.2lf", res);
                    push(stack, buf);
                    wattron(msg_win, COLOR_PAIR(3));
                    mvwprintw(msg_win, 2, 2, "Operation result: %.2lf", res);
                    wattroff(msg_win, COLOR_PAIR(3));
                }
                free(a); free(b);
                wrefresh(msg_win);
            }
            break;

        case 7: // Infix to Postfix Conversion with stepwise trace
            werase(msg_win);
            box(msg_win, 0, 0);
            wattron(msg_win, COLOR_PAIR(3));
            mvwprintw(msg_win, 1, 2, "Enter infix expression (e.g. A+B*C): ");
            wattroff(msg_win, COLOR_PAIR(3));
            wrefresh(msg_win);

            wmove(msg_win, 2, 2);
            wclrtoeol(msg_win);
            wrefresh(msg_win);

            echo();
            wgetnstr(msg_win, input, 255);
            noecho();

            werase(msg_win);
            wrefresh(msg_win);
            infix_to_postfix_stepwise(input, msg_win);
            break;

        case 8: // Evaluate Postfix Numeric Only
            werase(msg_win);
            box(msg_win, 0, 0);
            wattron(msg_win, COLOR_PAIR(3));
            mvwprintw(msg_win, 1, 2, "Enter postfix expression (numbers only): ");
            wattroff(msg_win, COLOR_PAIR(3));
            wrefresh(msg_win);

            wmove(msg_win, 2, 2);
            wclrtoeol(msg_win);
            wrefresh(msg_win);

            echo();
            wgetnstr(msg_win, input, 255);
            noecho();

            double eval_res;
            if (evaluate_postfix_numeric(input, &eval_res)) {
                wattron(msg_win, COLOR_PAIR(3));
                mvwprintw(msg_win, 3, 2, "Evaluation result: %.2lf", eval_res);
                wattroff(msg_win, COLOR_PAIR(3));
            } else {
                wattron(msg_win, COLOR_PAIR(4));
                print_centered(msg_win, 3, "Invalid expression or contains variables.", 4);
                wattroff(msg_win, COLOR_PAIR(4));
            }
            wrefresh(msg_win);
            break;

        case 9: // Postfix to Infix Stepwise
            postfix_to_infix_stepwise(msg_win);
            break;

        case 10: // Exit
            endwin();
            exit(0);

        default:
            wattron(msg_win, COLOR_PAIR(4));
            print_centered(msg_win, 2, "Invalid option! Select (1-10).", 4);
            wattroff(msg_win, COLOR_PAIR(4));
            wrefresh(msg_win);
            break;
    }
    display_stack(stack, stack_win, 1, 2);
    wrefresh(msg_win);
}

int main() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (has_colors() == FALSE) {
        endwin();
        printf("Your terminal does not support color\n");
        exit(EXIT_FAILURE);
    }
    start_color();

    // Define color pairs
    init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Menu title
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Info text
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Success text
    init_pair(4, COLOR_RED, COLOR_BLACK);     // Error text
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK); // Section titles
    init_pair(6, COLOR_BLUE, COLOR_BLACK);    // Top stack element highlight
    init_pair(8, COLOR_BLACK, COLOR_GREEN);   // Alternate stack rows

    Stack stack;
    init_stack(&stack);

    char input[256], postfix[256];

    // Windows sizes setup
    int menu_height = 17, menu_width = 45;
    int stack_height = 17, stack_width = 27;
    int msg_height = 16, msg_width = 74;

    WINDOW* menu_win = newwin(menu_height, menu_width, 1, 1);
    WINDOW* stack_win = newwin(stack_height, stack_width, 1, menu_width + 2);
    WINDOW* msg_win = newwin(msg_height, msg_width, menu_height + 2, 1);

    while (1) {
        draw_menu(menu_win);
        display_stack(&stack, stack_win, 1, 2);

        wmove(menu_win, 15, 22);
        wclrtoeol(menu_win);
        wrefresh(menu_win);
        int option = wgetch(menu_win) - '0';

        // Support two digit input for 10
        if (option == 1) {
            int c2 = wgetch(menu_win);
            if (c2 == '0') {
                option = 10;
            } else {
                ungetch(c2);
            }
        }

        handle_user_option(&stack, option, msg_win, stack_win, input, postfix);
    }

    endwin();
    return 0;
}

