# GraphLab C++ Coding Standards & Architecture Guide

This document defines the official code style, naming conventions, and architectural guidelines for the **GraphLab** project.

---

## 1. Directory & File Layout

- **`src/main.cpp`**: Minimal application entry point (~15-20 lines).
- **`src/core/`**: Core application lifecycle, window management, and engine infrastructure (`app.h`, `app.cpp`).
- **`src/ui/`**: User interface components (`main_layer`, `menu_bar`, `sidebar`, `canvas`).
- **`src/math/`**: Mathematical evaluation engines, expression parsers, and function models.
- **`thirdparty/`**: External third-party dependencies (ImGui, GLFW, STB).

---

## 2. Naming Conventions

| Category | Naming Style | Prefix / Suffix | Examples |
| :--- | :--- | :--- | :--- |
| **Directory Names** | `lowercase` | None | `src/`, `core/`, `ui/`, `math/` |
| **File Names** | `snake_case` | `.h` / `.cpp` | `app.h`, `main_layer.cpp`, `color_utils.h` |
| **Namespaces** | `PascalCase` | None | `namespace GraphLab { ... }` |
| **Classes & Structs** | `PascalCase` | None | `class App`, `struct AppSpecification` |
| **Functions & Methods** | `PascalCase` | None | `Init()`, `Run()`, `OnRenderUI()` |
| **Member Variables (Private)** | `PascalCase` | `m_` | `m_Window`, `m_Running`, `m_Specification` |
| **Static Member Variables** | `PascalCase` | `s_` | `s_Instance` |
| **Local Variables & Parameters** | `camelCase` | None | `mainScale`, `clearColor`, `spec` |
| **Constants & Enums** | `ALL_CAPS` / `PascalCase` | None | `MAX_EQUATIONS = 50`, `enum class GraphStyle` |

---

## 3. Architecture Rules

1. **RAII & Non-Copyable**:
   - Main engine classes like `GraphLab::App` manage unique system resources.
   - Always delete copy constructor and copy assignment operators:
     ```cpp
     App(const App&) = delete;
     App& operator=(const App&) = delete;
     ```

2. **Singleton Access**:
   - Provide static global access to the active application instance via `GraphLab::App::Get()`.

3. **Const Correctness**:
   - Mark all read-only getters with `const` (e.g., `GLFWwindow* GetNativeWindow() const;`).

4. **Forward Declarations**:
   - Use forward declarations in header files (e.g., `struct GLFWwindow;`) to avoid heavy `#include` dependencies and optimize build speeds.
