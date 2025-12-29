
# 📚 Stack Machine ISA Simulator

This project is a functional simulation of a **Stack Machine Architecture** implemented in C. Unlike traditional architectures that rely on CPU registers, this system processes data stored entirely on a stack (LIFO). It features a terminal-based user interface (TUI) to visualize how expressions are converted and evaluated at a low level.

###  Project Features

* **Zero-Register Computing**: All calculations are performed directly on the stack, following pure Stack Machine principles.
* **Expression Conversion**:
* **Infix to Postfix**: Step-by-step visualization of the Shunting-yard algorithm.
* **Postfix to Infix**: Reconstructing readable expressions from stack-based logic.


* **Postfix Evaluation**: Supports real-time numerical evaluation of postfix expressions.
* **Interactive TUI**: Built with the **ncurses** library to provide a color-coded, multi-window interface showing the Stack, Menu, and Message logs simultaneously.

### 📂 Repository Structure

* **`Project_code-5.c`**: The core source code containing the stack implementation (linked list), conversion algorithms, and ncurses UI logic.
* **`CSE360_Project_Report.pdf`**: Comprehensive technical documentation covering the ISA design, objectives, and testing.
* **`Stack_machine_ISA.pptx`**: Presentation slides illustrating the project flow, logic, and output screenshots.

###  Technical Details

* **Language**: C
* **Data Structure**: Linked List based Stack.
* **UI Library**: `ncurses` (for real-time terminal windowing).
* **Key Instructions**: `PUSH`, `POP`, `ADD`, `SUB`, `MUL`, `DIV`.

### ⚙️ How to Run

To run this simulator on a Linux-based system (or macOS/WSL):

1. **Install ncurses**:
```bash
sudo apt-get install libncurses5-dev libncursesw5-dev

```


2. **Compile**:
```bash
gcc Project_code-5.c -o stack_machine -lncurses -lm

```


3. **Execute**:
```bash
./stack_machine

```



###  How it Works

1. **The Stack**: Represented as a dynamic linked list where each node stores a token (operand or operator).
2. **The Interface**:
* **Left Window**: Operational Menu.
* **Right Window**: Real-time visual of the Stack memory.
* **Bottom Window**: Detailed step-by-step trace of the current operation.



