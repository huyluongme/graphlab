#include "core/app.h"

int main(int argc, char** argv)
{
    GraphLab::AppSpec spec;
    spec.Title = "GraphLab - Graphing Calculator";
    spec.Width = 1280;
    spec.Height = 800;
    spec.VSync = true;

    GraphLab::App app(spec);
    return app.Run();
}
