#include <json.hpp>
#include <zip.h>
#include <lunasvg.h>
#include <ini.h>
#include <SDL.h>
#include <SDL_image.h>

#include <iostream>
#include <filesystem>
#include <fstream>
#include <istream>
#include <cstdio>
#include <stdexcept>

#include <boxScratch/blocks/motion.h>
#include <boxScratch/scratchEnum.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

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
    if (block.opcode == "motion_pointindirection") {
        return BlockExecutors::pointInDirection(block, sprite);
    } else if (block.opcode == "motion_gotoxy") {
        return BlockExecutors::goToXY(block, sprite);
    }
    
    return sprite;
}

bool init() {
    bool success = true;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("SDL failed to initialize. SDL_Error: %s\n", SDL_GetError());
		success = false;
	}
	else {
		window = SDL_CreateWindow("BoxScratch", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screenWidth, screenHeight, SDL_WINDOW_SHOWN);
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

SDL_Texture* loadTexture(SDL_Surface* surface) {
    SDL_Texture* texture = NULL;

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == NULL) {
        printf("failed to create texture. SDL error: %s\n", SDL_GetError());
    }

    return texture;
}

int main(int argc, char** argv) {
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
            Block tempBlock;
            tempBlock.opcode = scratchBlock["opcode"];
            tempBlock.next = to_string(scratchBlock["next"]);
            tempBlock.parent = to_string(scratchBlock["parent"]);
            tempBlock.topLevel = scratchBlock["topLevel"];
            tempBlock.owner = tempSprite.name;
            project.blockMap[id] = tempBlock;
            for (auto& [arg, val]: scratchBlock["inputs"].items()) {
                    if (val.at(0) == 1) {
                        Block tempShadow;
                        if (val.at(1).at(0) == SHADOW_NUMBER) {
                            tempBlock.shadowType = SHADOW_NUMBER;
                        } else if (val.at(1).at(0) == SHADOW_POSITIVE_NUM) {
                           tempBlock.shadowType = SHADOW_POSITIVE_NUM;
                        } else if (val.at(1).at(0) == SHADOW_POSITIVE_INT) {
                           tempBlock.shadowType = SHADOW_POSITIVE_INT;
                        } else if (val.at(1).at(0) == SHADOW_INT) {
                           tempBlock.shadowType = SHADOW_INT;
                        } else if (val.at(1).at(0) == SHADOW_ANGLE) {
                           tempBlock.shadowType = SHADOW_ANGLE;
                        } else if (val.at(1).at(0) == SHADOW_COLOR) {
                           tempBlock.shadowType = SHADOW_COLOR;
                        } else if (val.at(1).at(0) == SHADOW_STRING) {
                           tempBlock.shadowType = SHADOW_STRING;
                        } else if (val.at(1).at(0) == SHADOW_BROADCAST) {
                           tempBlock.shadowType = SHADOW_BROADCAST;
                        } else if (val.at(1).at(0) == SHADOW_VARIABLE) {
                           tempBlock.shadowType = SHADOW_VARIABLE;
                        } else if (val.at(1).at(0) == SHADOW_LIST) {
                           tempBlock.shadowType = SHADOW_LIST;
                        }
                        tempShadow.shadowValue = val.at(1).at(1);
                        tempBlock.inputs[arg] = tempShadow;
                    } else {
                        Block tempReporter = project.blockMap[val.at(1)];
                        tempBlock.inputs[arg] = tempReporter;
                    }
                }
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

    init();

    for (auto& costume: project.costumes) {
        std::string cachedFolder = "cached/";
        auto oldCostume = costume;
        std::vector<Costume> v = project.costumes;
        std::string filePath = cachedFolder + costume.dataName + costume.dataFormat;
        if (costume.dataFormat == ".png") {
            SDL_Texture* tempTexture = loadTexture(loadSurface(cachedFolder + costume.dataName + ".png"));
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
            auto bitmap = document->renderToBitmap();
            if (bitmap.isNull()) {
                return -1;
            }
            costume.width = bitmap.width();
            costume.height = bitmap.height();
            bitmap.writeToPng(cachedFolder + costume.dataName + ".png");
            SDL_Texture* tempTexture = loadTexture(loadSurface(cachedFolder + costume.dataName + ".png"));
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
    while (!quit) {
    	while (SDL_PollEvent( &e ) != 0) {
    		if (e.type == SDL_QUIT) {
    			quit = true;
    		}
    	}

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        // Block execution
        for (auto& sprite: project.sprites) {
            for (Stack& stack: sprite.stacks) {
                if (stack.blocks.at(0).opcode == "event_whenflagclicked") {
                    nextBlock = stack.blocks.at(0).next;
                } else {
                    Sprite newSprite = blockExec(project.blockMap[nextBlock], sprite);
                    project.sprites.at(findSpriteIndex(project.sprites, newSprite)) = newSprite;
                    nextBlock = project.blockMap[nextBlock].next;
                }
            }
        }

        // Render
            for (auto& sprite: project.sprites) {
                for (auto &i : project.costumes) {
                    if (sprite.costumes.at(sprite.costumeIndex).dataName == i.dataName) {
                        SDL_Rect rect;
                        rect.x = i.width * 2 / upscaling;
                        rect.y = i.height * 2 / upscaling;
                        SDL_Rect* rectPtr = &rect;
                        if (sprite.isStage) {
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