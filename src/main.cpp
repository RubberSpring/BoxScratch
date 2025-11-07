#include <json.hpp>
#include <zip.h>
#include <lunasvg.h>
#include <ini.h>
#include <SDL.h>
#include <SDL_image.h>
#ifdef _WIN32
    #include <windows.h>
#endif

#include <iostream>
#include <filesystem>
#include <fstream>
#include <istream>
#include <cstdio>
#include <stdexcept>
#include <algorithm>

#include <boxScratch/blocks/motion.h>
#include <boxScratch/scratchEnum.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

bool isASCII (const std::string& s) {
    return !std::any_of(s.begin(), s.end(), [](char c) { 
        return static_cast<unsigned char>(c) > 127; 
    });
}

int findIndex(std::vector<Costume>& v, Costume val) {
    for (int i = 0; i < (int)v.size(); i++) {
      
          // When the element is found
        if (v[i].dataName == val.dataName) {
            return i;
        }
    }
      
      // When the element is not found
      return -1;
}

int findSpriteIndex(std::vector<Sprite>& v, Sprite val) {
    for (int i = 0; i < (int)v.size(); i++) {
      
          // When the element is found
        if (v[i].name == val.name) {
            return i;
        }
    }
      
      // When the element is not found
      return -1;
}
static std::string stripOuterQuotes(const std::string &s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

#ifdef _WIN32
    static void create_console(void) {
        AllocConsole();
        freopen("CONIN$", "r", stdin);
        freopen("CONOUT$", "w", stdout);
        freopen("CONOUT$", "w", stderr);
    }
#endif

int screenWidth = 480; // Scratch stage dimentions
int screenHeight = 360;
int upscaling = 2;
int targetFPS = 30;
int offsetX = 0;
int offsetY = 0;
SDL_Window* window = NULL;
SDL_Surface* screen = NULL;
SDL_Renderer* renderer = NULL;

Sprite blockExec(Block block, Sprite sprite) {
    printf("Executing block: %s\n", block.opcode.c_str()); // Add debug log
    
    if (block.opcode == "motion_pointindirection") {
        return BlockExecutors::pointInDirection(block, sprite);
    } else if (block.opcode == "motion_gotoxy") {
        return BlockExecutors::goToXY(block, sprite);
    } else if (block.opcode == "motion_turnright") {
        return BlockExecutors::turnRight(block, sprite);
    }
    
    printf("Warning: Unknown block type: %s\n", block.opcode.c_str()); // Add warning for unknown blocks
    return sprite;
}

bool init() {
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL failed to initialize. SDL_Error: %s\n", SDL_GetError());
        success = false;
    }
    else {
        window = SDL_CreateWindow("BoxScratch", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth * upscaling, screenHeight * upscaling, SDL_WINDOW_SHOWN);
        if (window == NULL) {
            printf("Failed to create window. SDL_Error: %s\n", SDL_GetError());
            success = false;
        }
        else {
            renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
            if (renderer == NULL) {
                printf("failed to create renderer. SDL error: %s\n", SDL_GetError());
                success = false;
            }
            else {
                int imgFlags = IMG_INIT_PNG;
                if (!(IMG_Init(imgFlags) & imgFlags)) {
                    printf("SDL_image failed to initialize. SDL_image error: %s\n", IMG_GetError());
                    success = false;
                }
                else {
                    screen = SDL_GetWindowSurface(window);
                }
            }
        }
    }

    return success;
}

SDL_Surface* loadSurface(std::string path) {
    SDL_Surface* optimized = NULL;

    SDL_Surface* loadedSurface = IMG_Load(path.c_str());
    if (loadedSurface == NULL) {
        printf("failed to load image %s. SDL_image error: %s\n", path.c_str(), IMG_GetError());
    }
    else {
        optimized = SDL_ConvertSurface(loadedSurface, screen->format, 0);
        if (optimized == NULL)
        {
            printf("failed to optimize image %s. SDL error: %s\n", path.c_str(), SDL_GetError());
        }

        SDL_FreeSurface(loadedSurface);
    }

    return optimized;
}

SDL_Texture* loadTexture(std::string path) {
    SDL_Texture* texture = NULL;

    texture = IMG_LoadTexture(renderer, path.c_str());
    if (texture == NULL) {
        printf("failed to create texture. SDL error: %s\n", SDL_GetError());
    }

    return texture;
}

// Add this struct before main():
struct StackState {
    bool isStarted = false;
    bool isForever = false;
    std::string nextBlock;
    std::string foreverStart;
};

int main(int argc, char** argv) {
    #ifdef _WIN32
        create_console();
    #endif

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

    freopen("output.log", "w", stdout);
    freopen("error.log", "w", stderr); 

    try {
        screenHeight = std::stoi(ini["generic"]["screenHeight"]);
        screenWidth = std::stoi(ini["generic"]["screenWidth"]);
        upscaling = std::stoi(ini["generic"]["upscaling"]);
        targetFPS = std::stoi(ini["generic"]["targetFPS"]);
        offsetX = std::stoi(ini["generic"]["offsetX"]);
        offsetY = std::stoi(ini["generic"]["offsetY"]);
    }
    catch (...) {
        std::cerr << "ERROR: Configs missing on generic section of config.ini\n";
        return -1;
    }

    if (selectProject > (int)projectPaths.size()) {
        selectProject = 0;
    }

    fs::path projectFilez(projectPaths.at(selectProject));
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
        if (ini[projectFilez.stem().string()].has("targetFPS")) {
            targetFPS = std::stoi(ini[projectFilez.stem().string()]["targetFPS"]);
        }
        if (ini[projectFilez.stem().string()].has("offsetX")) {
            offsetX = std::stoi(ini[projectFilez.stem().string()]["offsetX"]);
        }
        if (ini[projectFilez.stem().string()].has("offsetY")) {
            offsetY = std::stoi(ini[projectFilez.stem().string()]["offsetY"]);
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
        if (!tempSprite.isStage) {
            tempSprite.direction = sprite["direction"];
        } else {
            tempSprite.direction = 0;
        }
        if (!tempSprite.isStage) {
            tempSprite.size = sprite["size"];
        } else {
            tempSprite.size = 100;
        }
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
        for (auto& [id, scratchBlock]: sprite["blocks"].items()) {
            if (scratchBlock["opcode"] == "") {
                continue;
            }
            Block tempBlock;
            tempBlock.opcode = scratchBlock["opcode"];
            tempBlock.id = id;
            if (scratchBlock["next"].is_null()) {
                tempBlock.hasNext = false;
                tempBlock.next = "";
            } else {
                tempBlock.hasNext = true;
                tempBlock.next = stripOuterQuotes(to_string(scratchBlock["next"]));
            }
            tempBlock.parent = to_string(scratchBlock["parent"]);
            tempBlock.topLevel = scratchBlock["topLevel"];
            tempBlock.owner = tempSprite.name;
            // leave inputs empty for now — resolve in second pass
            project.blockMap[id] = tempBlock;
        }

        // Second pass: resolve inputs, now that all blocks exist in project.blockMap
        for (auto& [id, scratchBlock]: sprite["blocks"].items()) {
            if (scratchBlock["opcode"] == "") continue;
            Block &tempBlock = project.blockMap.at(id);
            for (auto& [arg, val]: scratchBlock["inputs"].items()) {
                if (val.at(0) == 1) {
                    Block tempShadow;
                    int st = val.at(1).at(0);
                    // store shadow type/value on the input block object
                    tempShadow.shadowType = st;
                    tempShadow.shadowValue = val.at(1).at(1);
                    tempShadow.isShadow = true;
                    tempBlock.inputs[arg] = tempShadow;
                } else {
                    std::string refId = val.at(1);
                    tempBlock.inputs[arg] = project.blockMap.at(refId);
                }
            }
        }
        std::cout << "Blocks deserialized [" << tempSprite.name << "]\n";
        // Stacks
        for (auto& [id, block] : project.blockMap) {
            if (block.owner != tempSprite.name) continue;
            if (!block.topLevel) continue;
            Stack tempStack;
            tempStack.blocks.push_back(block);
            std::string nextBlockId = block.next;
            while (!nextBlockId.empty()) {
                auto it = project.blockMap.find(nextBlockId);
                if (it == project.blockMap.end()) {
                    printf("Warning: missing block %s referenced in stack for sprite %s\n", nextBlockId.c_str(), tempSprite.name.c_str());
                    break;
                }
                tempStack.blocks.push_back(it->second);
                nextBlockId = it->second.next;
            }
            tempSprite.stacks.push_back(tempStack);
        }
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

    init();

    for (auto& costume: project.costumes) {
        std::string cachedFolder = "cached/";
        auto oldCostume = costume;
        std::vector<Costume> v = project.costumes;
        std::string filePath = cachedFolder + costume.dataName + costume.dataFormat;
        if (costume.dataFormat == ".png") {
            SDL_Texture* tempTexture = loadTexture(cachedFolder + costume.dataName + ".png");
            SDL_Point size;
            SDL_QueryTexture(tempTexture, NULL, NULL, &size.x, &size.y);
            costume.width = size.x;
            costume.height = size.y;
            costume.image = NULL;
            costume.texture = tempTexture;
        } else {
            auto document = lunasvg::Document::loadFromFile(filePath);
            if (document == nullptr) {
                return -1;
            }
            auto bitmap = document->renderToBitmap(document->width() * upscaling, document->height() * upscaling);
            if (bitmap.isNull()) {
                return -1;
            }
            costume.width = bitmap.width();
            costume.height = bitmap.height();
            bitmap.writeToPng(cachedFolder + costume.dataName + ".png");
            SDL_Texture* tempTexture = loadTexture(cachedFolder + costume.dataName + ".png");
            SDL_Point size;
            SDL_QueryTexture(tempTexture, NULL, NULL, &size.x, &size.y);
            costume.width = size.x;
            costume.height = size.y;
            costume.image = NULL;
            costume.texture = tempTexture;
        }
        for (auto &i : v) {
            if (i.dataName == costume.dataName) {
                project.costumes.at(findIndex(v, i)) = costume;
            }
        }
    }
    bool quit = false;

    SDL_Event e;  

    std::string nextBlock;

    bool isForever = false;
    std::string foreverStart;
    // In main(), before the game loop, add:
    std::map<std::string, StackState> stackStates;

    while (!quit) {
        SDL_PollEvent( &e );
        if (e.type == SDL_QUIT) {
            quit = true;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Block execution
        for (auto& sprite : project.sprites) {
            for (Stack& stack : sprite.stacks) {
                if (stack.blocks.size() == 0) continue;
                
                // Get or create state for this stack
                std::string stackId = stack.blocks[0].id;
                auto& state = stackStates[stackId];
                
                // Only start new stacks that haven't been started
                if (!state.isStarted && stack.blocks[0].opcode == "event_whenflagclicked") {
                    state.isStarted = true;
                    state.nextBlock = stack.blocks[0].next;
                }

                // Continue executing this stack if it has a next block
                if (!state.nextBlock.empty()) {
                    auto itBlock = project.blockMap.find(state.nextBlock);
                    if (itBlock == project.blockMap.end()) {
                        printf("Warning: referenced block %s not found\n", state.nextBlock.c_str());
                        state.nextBlock.clear();
                        continue;
                    }
                    Block currentBlock = itBlock->second;

                    if (!isASCII(currentBlock.opcode) || currentBlock.opcode.empty()) {
                        printf("Warning: Corrupt block %s\n", currentBlock.id.c_str());
                        project.blockMap.erase(itBlock);
                        state.nextBlock.clear();
                        continue;
                    }

                    if (currentBlock.opcode == "control_forever") {
                        state.isForever = true;
                        auto itInput = currentBlock.inputs.find("SUBSTACK");
                        if (itInput == currentBlock.inputs.end()) {
                            printf("Warning: forever block missing SUBSTACK\n");
                            state.nextBlock.clear();
                            continue;
                        }
                        state.foreverStart = itInput->second.id;
                        state.nextBlock = state.foreverStart;
                    } else {
                        sprite = blockExec(currentBlock, sprite);
                        int idx = findSpriteIndex(project.sprites, sprite);
                        if (idx >= 0) project.sprites.at(idx) = sprite;

                        if (!currentBlock.hasNext && state.isForever) {
                            state.nextBlock = state.foreverStart;
                            break; // Break for frame render
                        } else {
                            state.nextBlock = currentBlock.next;
                        }
                    }
                }
            }
        }

        // Render
            for (auto& sprite: project.sprites) {
                for (auto &i : project.costumes) {
                    if (sprite.costumes.at(sprite.costumeIndex).dataName == i.dataName) {
                        SDL_Rect rect;
                        rect.x = (screenWidth * upscaling) / 2 - (i.width / 2);
                        rect.y = (screenHeight * upscaling) / 2 - (i.height / 2);
                        rect.h = i.height;
                        rect.w = i.width;
                        SDL_Rect* rectPtr = &rect;
                        if (sprite.isStage) {
                            rect.x = 0;
                            rect.y = 0;
                            SDL_RenderCopy(renderer, i.texture, NULL, rectPtr);
                        } else {
                            SDL_RenderCopyEx(renderer, i.texture, NULL, rectPtr, sprite.direction - 90, NULL, SDL_FLIP_NONE);
                        }
                    }
                }
            }

        SDL_RenderPresent(renderer);
    }

    fs::remove_all("cached/");

    return 0;

}