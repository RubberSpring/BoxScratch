#include <nlohmann/json.hpp>
#include "raylib.h"
#include <zip.h>

#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

int main(void)
{
    // Collect project files
    std::string path = (fs::current_path() / "projects").string();
    std::vector<std::string> projectPaths;
    for (const auto & entry : fs::directory_iterator(path)) {
        std::vector<fs::path> paths;
        paths.push_back(entry.path());
        for (auto path: paths) {
            projectPaths.push_back(path.string());
        }
    }

    // Show projects
    std::cout << "BoxScratch\n";
    for (std::string prj: projectPaths) {
        fs::path p(prj);
        std::string filename = p.filename().string();
        if (!(filename.substr(filename.size() - 3) == "sb3")) {
            continue;
        }
        int index = find(projectPaths.begin(), projectPaths.end(), prj) - projectPaths.begin();
        std::cout << index << ": " << p.stem() << "\n";
    }

    // Get choosen project
    int selectProject;
    std::cout << "Select a project from the list (number): ";
    std::cin >> selectProject;

    // Open the project archive
    int err = 0;
    zip *z = zip_open(projectPaths.at(selectProject).c_str(), 0, &err);

    // Search for the project.json
    const char *name = "project.json";
    struct zip_stat st;
    zip_stat_init(&st);
    zip_stat(z, name, 0, &st);

    // Alloc memory for project file
    char *projectFile = new char[st.size];

    // Read the compressed file
    zip_file *f = zip_fopen(z, "project.json", 0);
    zip_fread(f, projectFile, st.size);
    zip_fclose(f);

    // And close the archive
    zip_close(z);

    const int screenWidth = 480; // Scratch stage dimentions
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "BoxScratch");

    SetTargetFPS(30); // Scratch max FPS (can break projects when changed)

    // Main loop
    while (!WindowShouldClose())
    {
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Congrats! You created your first window!", 0, 0, 20, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}