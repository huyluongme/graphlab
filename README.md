# 📈 GraphLab

[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-blue.svg)](#-building-from-source)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![OpenGL](https://img.shields.io/badge/Graphics-OpenGL%203.3-orange.svg)](https://www.opengl.org/)

**GraphLab** is a modern, high-performance 2D graphing and mathematical analysis desktop application built in C++17 with Dear ImGui and OpenGL. Designed for speed, precision, and visual clarity, GraphLab allows real-time plotting of explicit functions, implicit equations, relational inequalities, dynamic parametric sliders, and automatic key-point analysis.

---

## 🎬 Demo & Showcase

https://github.com/user-attachments/assets/c2fbcfa3-c10d-4016-9cca-093b6376165e

---

## ✨ Key Features

- **⚡ Explicit & Implicit Graphing**: Plot explicit functions ($y = f(x)$) and complex implicit equations ($f(x,y) = 0$, e.g. circles, ellipses, algebraic curves).
- **🎨 Desmos-style Inequality Shading**: Full support for relational inequalities (`<`, `>`, `<=`, `>=`, `!=`) with translucent region shading and solid/dashed boundary rendering.
- **🎛️ Dynamic Parameter Sliders**: Define global variables (e.g. `y = a * sin(b * x + c)`) and animate parameters with real-time controls.
- **🔍 Automatic Key Points & Inspection**: Instant detection of Roots ($f(x)=0$), Y-intercepts, Local Extrema (Maxima/Minima), and Curve Intersections with click-to-pin tooltips.
- **🚀 Optimized Rendering Engine**: High-performance grid rendering, Row-Merged Run-Length Shading, and ImGui vertex buffer optimizations for ultra-smooth panning and zooming.
- **💾 Project Import & Export**: Save/Load projects in JSON (`.glab`) format and export high-resolution graph screenshots as PNG images.
- **🎯 Modern Responsive UI**: Clean rounded design system powered by FontAwesome 6 icons, customizable color themes, and an interactive math syntax guide.

---

## 🧮 Math Syntax Reference

| Category | Operators / Functions | Example |
| :--- | :--- | :--- |
| **Basic** | `+`, `-`, `*`, `/`, `^`, `%` | `y = 2*x^2 - 3*x + 1` |
| **Inequalities** | `<`, `>`, `<=`, `>=`, `!=`, `≤`, `≥`, `≠` | `y <= sin(x)` or `x^2 + y^2 < 16` |
| **Trigonometry** | `sin`, `cos`, `tan`, `asin`, `acos`, `atan` | `y = sin(x) * cos(2*x)` |
| **Exponential & Log** | `exp`, `ln`, `log` (base 10), `sqrt`, `abs` | `y = exp(-x^2) * sqrt(x + 5)` |
| **Constants** | `pi`, `e` | `y = a * cos(pi * x)` |

---

## 📥 Download & Installation

Pre-built binaries for Windows and Linux are available under the GitHub Releases page:

👉 **[Download Latest GraphLab Release](../../releases)**

---

## 🛠️ Building from Source

### Prerequisites

- **C++ Compiler**: `g++` with C++17 support
- **Build Tool**: `make`
- **Linux Dependencies** (Ubuntu/Debian):
  ```bash
  sudo apt update
  sudo apt install build-essential libgl1-mesa-dev libglfw3-dev
  ```

### Build Instructions

1. **Clone the repository**:
   ```bash
   git clone https://github.com/huyluongme/graphlab.git
   cd graphlab
   ```

2. **Compile the project**:
   ```bash
   make -j4
   ```

3. **Run GraphLab**:
   - **Linux**: `./build/bin/graphlab`
   - **Windows**: `build\bin\graphlab.exe`

---

## 🎮 Navigation & Controls

| Action | Controls |
| :--- | :--- |
| **Pan Canvas** | Click & Drag (Left Mouse Button) |
| **Zoom In / Out** | Mouse Wheel Scroll (Centered at Cursor) |
| **Inspect Key Point** | Hover over key points or click to pin tooltip |
| **Reset View** | Click **Reset View** button on toolbar |

---

## 📦 Tech Stack & Dependencies

- **Language**: C++17
- **GUI Framework**: [Dear ImGui](https://github.com/ocornut/imgui)
- **Windowing & Input**: [GLFW](https://www.glfw.org/)
- **Graphics API**: OpenGL 3.3
- **Icons**: [FontAwesome 6](https://fontawesome.com/)
- **Image Writing**: [stb_image_write](https://github.com/nothings/stb)

---

## 📄 License

This project is licensed under the [MIT License](LICENSE).
