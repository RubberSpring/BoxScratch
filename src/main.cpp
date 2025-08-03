#include <nlohmann/json.hpp>
#include "raylib.h"
#include <zip.h>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <istream>

#include <boxScratch/project.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

int main(void) {
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

    int selectProject;
    std::cout << "Select a project from the list (number): ";
    std::cin >> selectProject;

    int err = 0;
    zip* archive = zip_open(projectPaths.at(selectProject).c_str(), 0, &err);
    if (!archive) {
        std::cerr << "Error opening project archive: " << projectPaths.at(selectProject) << std::endl;
        return 1;
    }

    zip_int64_t num_entries = zip_get_num_entries(archive, 0);
    if (num_entries < 0) {
        std::cerr << "Error getting project files." << std::endl;
        zip_close(archive);
        return 1;
    }

    fs::path cachedPath("cached/");
    if (!fs::exists(cachedPath)) {
        fs::create_directory(cachedPath);
    }

    for (zip_int64_t i = 0; i < num_entries; ++i) {
        struct zip_stat st;
        if (zip_stat_index(archive, i, 0, &st) < 0) {
            std::cerr << "Error retrieving file info for index " << i << std::endl;
            continue;
        }

        // Open the file inside the ZIP archive
        zip_file* file = zip_fopen_index(archive, i, 0);
        if (!file) {
            std::cerr << "Error opening file in project archive: " << st.name << std::endl;
            continue;
        }

        // Create an output file to write the extracted data to
        std::string cached = "cached/";
        std::string name(st.name);
        std::ofstream out(cached + name, std::ios::binary);
        if (!out) {
            std::cerr << "Error creating output file: " << st.name << std::endl;
            zip_fclose(file);
            continue;
        }

        // Allocate buffer to read the file contents
        char buffer[8192];  // 8 KB buffer size
        zip_int64_t bytes_read = 0;
        while ((bytes_read = zip_fread(file, buffer, sizeof(buffer))) > 0) {
            out.write(buffer, bytes_read);
        }

        // Check for errors while reading
        if (bytes_read < 0) {
            std::cerr << "Error reading file from project archive: " << st.name << std::endl;
        }

        // Close the file inside the ZIP archive and the output file
        zip_fclose(file);
        out.close();
    }

    zip_close(archive);

    std::cout << "Extraction complete!" << std::endl;

    std::ifstream projectFile("cached/project.json");
    std::stringstream projectBuffer;
    projectBuffer << projectFile.rdbuf();

    std::string projectFileFixed = projectBuffer.str();
    if (projectFileFixed.size() == 0) {
        return 1;
    }

    json projectJson = json::parse(projectFileFixed);
    projectFile.close();

    Project project;

    // Populate project
    for (auto& sprite: projectJson["targets"]) {
        Sprite tempSprite;
        tempSprite.isStage = sprite["isStage"];
        tempSprite.name = sprite["name"];
        // Variables
        for (auto& [key, value]: sprite["variables"].items()) {
            Variable tempVariable;
            tempVariable.name = value.at(0);
            tempVariable.value = to_string(value.at(1));
            project.variableMap[key] = tempVariable;
        }
        std::cout << "Variables deserialized\n";
        // Lists
        for (auto& [key, value]: sprite["lists"].items()) {
            List tempList;
            tempList.name = value.at(0);
            for (auto& listVal: value.at(1)) {
                tempList.value.push_back(listVal);
            }
            project.listMap[key] = tempList;
        }
        std::cout << "Lists deserialized\n";
        // Broadcasts
        for (auto& [id, name]: sprite["broadcasts"].items()) {
            Broadcast tempBroadcast;
            tempBroadcast.name = name;
            project.broadcastMap[id] = tempBroadcast;
        }
        std::cout << "Broadcasts deserialized\n";
        // Blocks
        for (auto& [id, block]: sprite["blocks"].items()) {
            Block tempBlock;
            tempBlock.opcode = block["opcode"];
            tempBlock.next = to_string(block["next"]);
            tempBlock.parent = to_string(block["parent"]);
            for (auto& [arg, val]: block["inputs"].items()) {
                std::cout << arg << "\n";
                if (arg == "SUBSTACK" || arg == "TO") {
                    tempBlock.inputs[arg] = val.at(1);
                } else {
                    tempBlock.inputs[arg] = val.at(1).at(1);
                }
            }
            tempBlock.topLevel = block["topLevel"];
        }
        std::cout << "Blocks deserialized\n";
        // Costumes
        int costumeIndex = 1;
        for (auto& costume: sprite["costumes"]) {
            Costume tempCostume;
            tempCostume.name = costume["name"];
            tempCostume.number = costumeIndex;
            costumeIndex += 1;
            std::ifstream costumeFile(cachedPath.string() + to_string(costume["md5ext"]));
            std::stringstream costumeBuffer;
            costumeBuffer << costumeFile.rdbuf();
            tempCostume.image = costumeBuffer.str();
            costumeFile.close();
        }
        int soundIndex = 1;
        std::cout << "Costumes deserialized\n";
        for (auto& sound: sprite["sounds"]) {
            ScratchSound tempSound;
            tempSound.name = sound["name"];
            tempSound.number = soundIndex;
            soundIndex += 1;
            std::ifstream soundFile(cachedPath.string() + to_string(sound["md5ext"]));
            std::stringstream soundBuffer;
            soundBuffer << soundFile.rdbuf();
            tempSound.sound = soundBuffer.str();
            soundFile.close();
        }
    }
    std::cout << "Project populated!\n";

    const int screenWidth = 480; // Scratch stage dimentions
    const int screenHeight = 360;

    InitWindow(screenWidth, screenHeight, "BoxScratch");

    SetTargetFPS(30); // Scratch max FPS (can break projects when changed)

    // Main loop
    while (!WindowShouldClose())
    {
        BeginDrawing();

            ClearBackground(WHITE);

            DrawText("Congrats! You created your first window!", 0, 0, 20, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}