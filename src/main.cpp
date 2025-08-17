#include <json.hpp>
#include "raylib.h"
#include <zip.h>
#include <lunasvg.h>
#include <ini.h>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <istream>

#include <boxScratch/project.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

int findIndex(std::vector<Costume>& v, Costume val) {
    for (int i = 0; i < v.size(); i++) {
      
      	// When the element is found
        if (v[i].dataName == val.dataName) {
            return i;
        }
    }
  	
  	// When the element is not found
  	return -1;
}

int screenWidth = 480; // Scratch stage dimentions
int screenHeight = 360;
int upscaling = 2;

Vector2 scratchToRaylibVec(int x, int y) {
    Vector2 vec;
    vec.x = x + screenWidth / 2;
    vec.y = y + screenHeight / 2;
    return vec;
}

int main(void) {
    mINI::INIFile file("config.ini");

    mINI::INIStructure ini;
    file.read(ini);

    fs::path projectPath("projects/");
    if (!fs::exists(projectPath)) {
        fs::create_directory(projectPath);
    }

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

    screenHeight = std::stoi(ini["generic"]["screenHeight"]);
    screenWidth = std::stoi(ini["generic"]["screenWidth"]);
    upscaling = std::stoi(ini["generic"]["upscaling"]);

    fs::path projectFilez = projectPaths.at(selectProject);
    if (ini.has(projectFilez.stem().string())) {
        if (ini[projectFilez.stem().string()].has("screenWidth")) {
            screenWidth = std::stoi(ini[projectFilez.stem().string()]["screenWidth"]);
        }
        if (ini[projectFilez.stem().string()].has("screenHeight")) {
            screenHeight = std::stoi(ini[projectFilez.stem().string()]["screenHeight"]);
        }
        if (ini[projectFilez.stem().string()].has("upscaling")) {
            upscaling = std::stoi(ini[projectFilez.stem().string()]["upscaling"]);
        }
    }

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
        tempSprite.costumeIndex = sprite["currentCostume"];
        if (!tempSprite.isStage) {
            tempSprite.x = sprite["x"];
            tempSprite.y = sprite["x"];
        } else {
            tempSprite.x = 0;
            tempSprite.y = 0;
        }
        // Variables
        for (auto& [key, value]: sprite["variables"].items()) {
            Variable tempVariable;
            tempVariable.name = value.at(0);
            tempVariable.value = to_string(value.at(1));
            project.variableMap[key] = tempVariable;
            tempSprite.variables.push_back(tempVariable);
        }
        std::cout << "Variables deserialized [" << tempSprite.name << "]\n";
        // Lists
        for (auto& [key, value]: sprite["lists"].items()) {
            List tempList;
            tempList.name = value.at(0);
            for (auto& listVal: value.at(1)) {
                tempList.value.push_back(listVal);
            }
            project.listMap[key] = tempList;
            tempSprite.lists.push_back(tempList);
        }
        std::cout << "Lists deserialized [" << tempSprite.name << "]\n";
        // Broadcasts
        for (auto& [id, name]: sprite["broadcasts"].items()) {
            Broadcast tempBroadcast;
            tempBroadcast.name = name;
            project.broadcastMap[id] = tempBroadcast;
            tempSprite.broadcasts.push_back(tempBroadcast);
        }
        std::cout << "Broadcasts deserialized [" << tempSprite.name << "]\n";
        // Blocks
        for (auto& [id, block]: sprite["blocks"].items()) {
            Block tempBlock;
            tempBlock.opcode = block["opcode"];
            tempBlock.next = to_string(block["next"]);
            tempBlock.parent = to_string(block["parent"]);
            for (auto& [arg, val]: block["inputs"].items()) {
                if (arg == "SUBSTACK" || arg == "TO") {
                    tempBlock.inputs[arg] = val.at(1);
                } else {
                    tempBlock.inputs[arg] = val.at(1).at(1);
                }
            }
            tempBlock.topLevel = block["topLevel"];
            tempBlock.owner = tempSprite.name;
            project.blockMap[id] = tempBlock;
        }
        std::cout << "Blocks deserialized [" << tempSprite.name << "]\n";
        // Costumes
        tempSprite.costumeIndex = sprite["currentCostume"];
        for (auto& costume: sprite["costumes"]) {
            Costume tempCostume;
            tempCostume.name = costume["name"];
            const std::string dot = ".";
            tempCostume.dataName = costume["assetId"];
            std::string dataFormat = to_string(costume["dataFormat"]);
            if (dataFormat.size() > 1) {
                tempCostume.dataFormat = dot + dataFormat.substr(1, dataFormat.size() - 2);
            }
            tempSprite.costumes.push_back(tempCostume);
            project.costumes.push_back(tempCostume);
        }
        std::cout << "Costumes deserialized [" << tempSprite.name << "]\n";
        // Sounds
        int soundIndex = 1;
        for (auto& sound: sprite["sounds"]) {
            ScratchSound tempSound;
            tempSound.name = sound["name"];
            tempSound.number = soundIndex;
            soundIndex += 1;
            tempSprite.sounds.push_back(tempSound);
            project.sounds.push_back(tempSound);
        }
        project.sprites.push_back(tempSprite);  
    }
    std::cout << "Project populated!\n";    

    InitWindow(screenWidth, screenHeight, "BoxScratch");

    SetTargetFPS(30); // Scratch max FPS (can break projects when changed)

    for (auto& costume: project.costumes) {
        std::string cachedFolder = "cached/";
        auto oldCostume = costume;
        std::vector<Costume> v = project.costumes;
        std::string filePath = cachedFolder + costume.dataName + costume.dataFormat;
        if (costume.dataFormat == ".png") {
            Image tempImage = LoadImage((cachedFolder + costume.dataName + ".png").c_str());
            costume.width = tempImage.width;
            costume.height = tempImage.height;
            costume.ogImage = ImageCopy(tempImage);
            costume.cpuImage = ImageCopy(tempImage);
            costume.gpuImage = LoadTextureFromImage(tempImage);
            UnloadImage(tempImage);
        } else {
            auto document = lunasvg::Document::loadFromFile(filePath);
            if (document == nullptr) {
                return -1;
            }
            auto bitmap = document->renderToBitmap();
            if (bitmap.isNull()) {
                return -1;
            }
            costume.width = bitmap.width();
            costume.height = bitmap.height();
            bitmap.writeToPng(cachedFolder + costume.dataName + ".png");
            Image tempImage = LoadImage((cachedFolder + costume.dataName + ".png").c_str());
            costume.ogImage = ImageCopy(tempImage);
            costume.cpuImage = ImageCopy(tempImage);
            costume.gpuImage = LoadTextureFromImage(tempImage);
            UnloadImage(tempImage);
        }
        for (auto &i : v) {
            if (i.dataName == costume.dataName) {
                project.costumes.at(findIndex(v, i)) = costume;
            }
        }
    }

    // Main loop
    while (!WindowShouldClose())
    {
        BeginDrawing();

            ClearBackground(WHITE);

            // Block execution goes here

            // Render
            for (auto& sprite: project.sprites) {
                for (auto &i : project.costumes) {
                    if (sprite.costumes.at(sprite.costumeIndex).dataName == i.dataName) {
                        if (sprite.isStage) {
                            DrawTexture(i.gpuImage, 0, 0, WHITE);   
                        }
                        Vector2 pos = scratchToRaylibVec(sprite.x, sprite.y);
                        pos.x = pos.x - i.width / 2;
                        pos.y = pos.y - i.height / 2;
                        DrawTextureV(i.gpuImage, pos, WHITE);
                    }
                }
            }

        EndDrawing();
    }

    CloseWindow();

    fs::remove_all("cached/");

    return 0;
}