#include "config.h"

/* Constants */
#pragma region Constants

const char* programName = "Particle Life";

const std::array<int, 2> aspectRatio = {(int)(1600*1), (int)(900*1)}; // Width, height

const double clusterFactor = 1; // Increases motivation to form clusters

const double interactRange = .03 * clusterFactor;
const double repelRange = .013 * clusterFactor;

const double interactForce = 50;
const double repelForce = 5;

const double timeMultiplier = 1;
double timeSpeed = .1;

const double radius = .01;

const int count = 0;
unsigned int variety = 4; // Total number of different particle types to use; not marked as const as it is changed if too large in main()

const bool punishClusters = false;

const bool side = 0; // Left = 0; right = 1
const float length = .6f;

const std::string fontPath = "res/fonts/Uroob-Regular.ttf";

#pragma endregion

// Color palette size and gradient helpers
constexpr int colorsCount = 50;

// Interactions matrix
// Can only be as large as the number of possible colors, but can be smaller 
std::vector<std::vector<float>> interactions(colorsCount, std::vector<float>(colorsCount));

/* Particle Class */
// Extention of body class; more particle specific information
class Particle : public body::Body
{
    // Radius of the circle
    float radius;
    int colorIndex;

    public:
        /* Color lookup */
        // Returns color's rgb value based on a given color index and the current variety
        static std::array<float, 4> GetColor(int colorIndex, float a, int varietyCount = variety)
        {
            const int effectiveVariety = std::max(1, varietyCount);
            const int clampedIndex = std::clamp(colorIndex, 0, effectiveVariety - 1);

            // Calculate rgb from hsv, which is calulated based on the color's index
            const auto hsvToRgb = [](float h, float s, float v) {
                const float hue = std::fmod(h, 360.0f);
                const float c = v * s;
                const float x = c * (1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
                const float m = v - c;

                float r = 0.0f;
                float g = 0.0f;
                float b = 0.0f;

                if (hue < 60.0f) {
                    r = c; g = x; b = 0.0f;
                } else if (hue < 120.0f) {
                    r = x; g = c; b = 0.0f;
                } else if (hue < 180.0f) {
                    r = 0.0f; g = c; b = x;
                } else if (hue < 240.0f) {
                    r = 0.0f; g = x; b = c;
                } else if (hue < 300.0f) {
                    r = x; g = 0.0f; b = c;
                } else {
                    r = c; g = 0.0f; b = x;
                }

                return std::array<float, 3>{r + m, g + m, b + m};
            };

            float hue = 0.0f;
            if (effectiveVariety == 1)
            {
                hue = 0.0f;
            }
            else if (effectiveVariety == 2)
            {
                hue = clampedIndex == 0 ? 0.0f : 180.0f;
            }
            else
            {
                hue = 360.0f * static_cast<float>(clampedIndex) / static_cast<float>(effectiveVariety);
            }

            const auto rgb = hsvToRgb(hue, 0.85f, 0.95f);
            return {rgb[0], rgb[1], rgb[2], a};
        }

        /* Color Name Lookup */
        // Returns color's display name based on a given color index
        static std::string GetColorName(int colorIndex, int varietyCount = variety)
        {
            const int effectiveVariety = std::max(1, varietyCount);
            const int clampedIndex = std::clamp(colorIndex, 0, effectiveVariety - 1);

            static const std::array<std::string, 50> hueNames = {
                // 0°–36°
                "Red",
                "Scarlet",
                "Vermilion",
                "Orange-Red",
                "Red-Orange",

                // 36°–72°
                "Orange",
                "Tangerine",
                "Amber",
                "Golden",
                "Yellow-Orange",

                // 72°–108°
                "Yellow",
                "Lemon",
                "Chartreuse",
                "Yellow-Green",
                "Lime",

                // 108°–144°
                "Lime Green",
                "Spring Green",
                "Green",
                "Emerald",
                "Sea Green",

                // 144°–180°
                "Turquoise",
                "Teal",
                "Robin Egg Blue",
                "Cyan",
                "Aqua",

                // 180°–216°
                "Sky Blue",
                "Azure",
                "Cerulean",
                "Dodger Blue",
                "Blue",

                // 216°–252°
                "Cobalt",
                "Sapphire",
                "Indigo",
                "Blue-Violet",
                "Violet",

                // 252°–288°
                "Purple",
                "Amethyst",
                "Orchid",
                "Medium Orchid",
                "Magenta",

                // 288°–324°
                "Fuchsia",
                "Deep Pink",
                "Hot Pink",
                "Rose",
                "Cerise",

                // 324°–360°
                "Crimson",
                "Ruby",
                "Raspberry",
                "Cherry",
                "Red"
            };
            
            if (effectiveVariety == 1)
                return "Red";

            if (effectiveVariety == 2)
                return clampedIndex == 0 ? "Red" : "Cyan";

            const int hueIndex = static_cast<int>(std::lround(static_cast<float>(clampedIndex) / static_cast<float>(effectiveVariety - 1) * static_cast<float>(hueNames.size() - 1)));
            return hueNames[std::clamp(hueIndex, 0, static_cast<int>(hueNames.size() - 1))];
        }

        // Particle constructor; takes a gfx::Circle
        Particle(const gfx::Circle pos, int colorIndex)
        {
            SetPosition(pos.x, pos.y);
            radius = pos.radius;
            this->colorIndex = colorIndex;

            stable = !punishClusters;
        }

        /* Particle Properties */
        // Returns particle as a gfx::circle
        const gfx::Circle GetProperties()
        {
            return (gfx::Circle) {(float)position[0], (float)position[1], radius, GetColor(colorIndex, 1)};
        }

        const int getColorID() const
        {
            return colorIndex;
        }
};

/* Random Matrix Assignment */
// Chooses a random value between -1 and 1 to set how particles should interact with one another
void RandomizeInteractions(std::vector<std::vector<float>> &matrix)
{
    std::random_device rd; // Seed for the generator
    std::mt19937 gen(rd()); // Standard Mersenne Twister engine
    std::uniform_real_distribution<float> distr(-1, 1); // Distribution between -1 and 1

    for(int x = 0; x < colorsCount; x++)
    {
        for(int y = 0; y < colorsCount; y++)
        {
            matrix[x][y] = distr(gen);
        }
    }
}

/* Initial Particle Creation*/
// Spawns particles randomly around the map
std::vector<Particle> InitializeParticles(int count, int variety)
{
    std::vector<Particle> particles;

    for (int i = 0; i < variety; i++) // i = number of different particle types
    {
        for(int j = 0; j < count/variety; j++) // j = index of particle created in a given particle type
        {
            // Random positioning for particles(between -1 and 1)
            float x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            float y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;

            // Adds new particle to list
            particles.emplace_back(gfx::Circle{x, y, radius, Particle::GetColor(i, 1.0f, variety)}, i);
        }
    }

    return particles;
}

/* Functions for Graphics Rendering */
#pragma region Graphics Functions

/* Shader Compiler*/
// Compiles GLSL source code and returns the OpenGL shader object ID
static unsigned int CompileShader(const std::string& source, unsigned int type)
{
    // Creates a shader of given type to load shader onto
    unsigned int id = glCreateShader(type);

    // Sets the source of the shader
    const char* src = source.c_str();

    glShaderSource(id, 1, &src, nullptr);

    // Compiles shader
    glCompileShader(id);

    // Checks if shader actually compiled
    int result;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);

    // Retrieve and print the shader compilation log
    if (!result)
    {
        // Message length
        int length;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        
        // Actual message (in CString form)
        char* message = new char[length];
        glGetShaderInfoLog(id, length, &length, message);

        // Prints shader
        std::cout << "Shader compile error:\n" << message << std::endl;
        delete[] message;

        // Ends process of trying to get shader
        glDeleteShader(id);
        return 0;
    }

    return id;
}

/* Shader Initialization */
// Links shader to program then removes the intermidiate compiled shader
static unsigned int CreateShader(const std::string& vertex, const std::string& fragment)
{
    // Creates program to create shader under
    unsigned int program = glCreateProgram();

    // Compiles both shaders and gets their id
    unsigned int vs = CompileShader(vertex, GL_VERTEX_SHADER);
    unsigned int fs = CompileShader(fragment, GL_FRAGMENT_SHADER);

    // Attaches shader to state machine
    glAttachShader(program, vs);
    glAttachShader(program, fs);

    // Connects shader to program
    glLinkProgram(program);
    glValidateProgram(program);

    // Removes intermediates
    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}

/* Shader Loader */
// Turns shader file of given path to a string
static std::string LoadShaderFile(const std::string& path)
{
    // Gets file contents in form of a string
    std::ifstream file(path);
    std::stringstream stream;
    stream << file.rdbuf();

    return stream.str();
}

/* Shader Initialization */
// Creates shader and mounts it to state machine
static unsigned int InitializeShader(const std::string path)
{
    // Shader source: change path if moving or renaming basic.shader
    std::string shaderSrc = LoadShaderFile(path);

    const std::string vertexMarker = "#shader vertex";
    const std::string fragmentMarker = "#shader fragment";

    const std::size_t vertexPos = shaderSrc.find(vertexMarker);
    const std::size_t fragmentPos = shaderSrc.find(fragmentMarker);

    // Gets vertex shader from shader file
    std::string vs = shaderSrc.substr(vertexPos + vertexMarker.size());
    vs = vs.substr(0, fragmentPos - (vertexPos + vertexMarker.size()));

    // Gets fragment shader from shader file
    std::string fs = shaderSrc.substr(fragmentPos + fragmentMarker.size());

    // Initializes shader from vertex and fragment shader
    unsigned int shader = CreateShader(vs, fs);

    // Tells state machine to use that shader
    glUseProgram(shader);

    return shader;
}

#pragma endregion

int main()
{
    #pragma region Initialization

    srand(time(NULL));

    RandomizeInteractions(interactions);

    // Clamp the number of particle types to the number of available colors
    if(variety > colorsCount)
        variety = colorsCount;

    // Tries to initialize GLFW
    // Ends program if initialization failed
    if (!glfwInit())
    {
        std::cout << "GLFW failed to start.\n";
        return -1;
    }

    // Creates window and sets it as current window attached to state machine
    GLFWwindow* window = glfwCreateWindow(aspectRatio[0], aspectRatio[1], programName, nullptr, nullptr);
    glfwMakeContextCurrent(window);

    // Tries to initialize GLEW
    // Ends program if initialization failed
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW failed to start.\n";
        return -1;
    }

    // Resolve project root from the executable location so shader assets are found reliably
    std::filesystem::path projectRoot;

    std::error_code error;
    std::filesystem::path executablePath = std::filesystem::canonical("/proc/self/exe", error);

    projectRoot = executablePath.parent_path();

    if(projectRoot.filename() == "build")
        projectRoot = projectRoot.parent_path();

    GLuint worldShader = InitializeShader((projectRoot / "res/shaders/world.shader").string());
    GLuint uiShader = InitializeShader((projectRoot / "res/shaders/UI.shader").string());
    GLuint textShader = InitializeShader((projectRoot / "res/shaders/text.shader").string());

    // Initializes new CircleRenderer object for rendering particles
    gfx::CircleRenderer renderer;
    renderer.CreateBuffer(worldShader, 6);

    // Spawns particles at random normalized positions and adds them to particle list
    std::vector<Particle> particles = InitializeParticles(count, variety);

    // Prevents circles from looking like ovals
    float aspect = aspectRatio[0] / (float) aspectRatio[1];

    // List of rendered particles
    std::vector<gfx::Circle> circles;
    circles.resize(particles.size()); // Preallocate the output array so we can write particle properties directly without growing it each frame

    // Cell size calculation
    float maxQueryRadius = static_cast<float>(std::max(interactRange, repelRange)); // Use the larger of the two ranges so the grid covers all interactions
    const float spatialCellSize = std::max((float)radius * 2, maxQueryRadius * 0.5f); // Each cell must be at least one particle diameter, and also small enough to keep neighbor checks local

    // How many columns and rows fit across the world space from -1 to 1
    const int gridCols = static_cast<int>(std::ceil(2.0f / spatialCellSize));
    const int gridRows = gridCols;

    // Total number of grid buckets in the spatial hash
    const int gridCellCount = gridCols * gridRows;

    // How many cells away we must check for neighbors
    const int maxCellOffset = static_cast<int>(std::ceil(maxQueryRadius / spatialCellSize));

    // Create grid storage: each cell holds indices of particles inside it
    std::vector<std::vector<int>> gridCells(gridCellCount);

    int reserveAmount = static_cast<int>(count / gridCellCount) + 1; // Estimate how many particles will go into each cell
    if (reserveAmount < 8) reserveAmount = 8; // Keep the bucket capacity reasonable even when the grid is sparse
    for (auto &cell : gridCells)
        cell.reserve(reserveAmount); // Reserve capacity once to avoid repeated allocations when filling the grid

    #pragma endregion

    #pragma region UI

    // Panel creation
    UI::Panel panel{side, length, {1, 1, 1, .5f}};
    auto &world = panel.GetWorld();

    UI::element grid = panel.AddGrid(0, .1f, .7, variety, &interactions, true, aspect);
    UI::element timeText = panel.AddTextElement(35, 100, {-.99, .95}, fontPath, true, "Time Speed: 10%", false);
    UI::element timeSlider = panel.AddSlider(.1f, .947f, .2f, .04f, timeSpeed);

    UI::element varietyText = panel.AddTextElement(35, 1000, {-.99, .85}, fontPath, true, std::string("Color Variety: ") + std::to_string(variety), false);
    UI::element varietySlider = panel.AddSlider(.1f, .85f, .2f, .04f, 3.0f/colorsCount);    
    UI::element rerandomizeButton = panel.AddButton(0, .75f, .16f, .055f, {.35f, .35f, .35f, .9f}, textShader, "Randomize", fontPath, 20);

    panel.LinkElements(timeText, timeSlider);
    panel.LinkElements(varietyText, varietySlider);

    std::vector<std::string> colorNames;
    std::vector<std::array<float, 4>> colorOptions;
    colorNames.reserve(variety);
    colorOptions.reserve(variety);

    for (unsigned int i = 0; i < variety; i++)
    {
        colorNames.push_back(Particle::GetColorName(static_cast<int>(i), variety));
        colorOptions.push_back(Particle::GetColor(static_cast<int>(i), .9f, variety));
    }

    UI::SubPanel subPanel(.5f, .01f, {.16f, .16f, .18f, .75f});
    UI::element amountText = subPanel.AddTextElement(18, 100, -.2f, .18f, fontPath, true, "Number of Particles: 100", false);
    UI::element amountSlider = subPanel.AddSlider(.1f, .18f, .2f, .04f, 99.0f / 9999.0f);
    UI::element spawnButton = subPanel.AddButton(.12f, .1f, .1f, .06f, {.35f, .65f, .9f, .9f}, textShader, "Spawn", fontPath, 22);
    UI::element deleteButton = subPanel.AddButton(0.0f, 0.0f, .22f, .055f, {.7f, .25f, .25f, .9f}, textShader, "Delete", fontPath, 22);
    UI::element colorDropdown = subPanel.AddDropdownButton(-.1f, .1f, .22f, .06f, colorNames, colorOptions, textShader, fontPath, 20);
    subPanel.SetActive(false);

    // Initialize everything
    panel.Init(uiShader, textShader, window);
    subPanel.Init(uiShader, textShader);

    #pragma endregion

    int lastDropdownVariety = variety;

    // Variable for storing delta time
    std::chrono::time_point lastTime = std::chrono::steady_clock::now();
    std::chrono::time_point currentTime = std::chrono::steady_clock::now();

    std::chrono::duration<float> elapsed;

    // Read the UI interaction matrix once, not repeatedly inside the particle loop
    auto gridValues = panel.GetGridValues(grid);
    if (!gridValues)
    {
        std::cout << "Warning: failed to read grid values for interactions." << std::endl;
    }

    /* Main Loop */
    while (!glfwWindowShouldClose(window))
    {
        // Read the current simulation speed from the UI slider
        const float timeSliderValue = panel.GetSliderValue(timeSlider);
        timeSpeed = timeSliderValue;

        // Update displayed simulation speed
        const int timePercent = static_cast<int>(std::lround(timeSliderValue * 100.0f));
        panel.SetText(timeText, "Time Speed: " + std::to_string(timePercent) + "%");

        // Update variety
        const int varietyCount = 1 + static_cast<int>(std::lround(panel.GetSliderValue(varietySlider) * static_cast<float>(colorsCount - 1)));

        // Update current displayed color variety
        variety = static_cast<unsigned int>(std::clamp(varietyCount, 1, colorsCount));
        panel.SetText(varietyText, "Color Variety: " + std::to_string(variety));
        panel.SetGridBoxCount(grid, variety);

        if (variety != lastDropdownVariety)
        {
            std::vector<std::string> colorNames;
            std::vector<std::array<float, 4>> colorOptions;
            colorNames.reserve(variety);
            colorOptions.reserve(variety);

            for (unsigned int i = 0; i < variety; ++i)
            {
                colorNames.push_back(Particle::GetColorName(static_cast<int>(i), variety));
                colorOptions.push_back(Particle::GetColor(static_cast<int>(i), .9f, variety));
            }

            subPanel.SetDropdownOptions(colorDropdown, colorNames, colorOptions);
            lastDropdownVariety = variety;
        }

        // Get current time
        currentTime = std::chrono::steady_clock::now();

        // Calculates time distance between current and previous frame
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        float deltaTime = elapsed.count();
        float simDelta = deltaTime * timeMultiplier * timeSpeed; // scale the simulation timestep by user speed

        lastTime = currentTime;

        // Build spatial grid for neighbor lookups
        for (auto &cell : gridCells)
            cell.clear(); // empty the buckets before refilling them this frame

        for (int i = 0; i < static_cast<int>(particles.size()); ++i)
        {
            const auto &pos = particles[i].GetPosition();

            int cx = static_cast<int>(((pos[0] + 1.0f) * 0.5f) * gridCols);
            int cy = static_cast<int>(((pos[1] + 1.0f) * 0.5f) * gridRows);

            if (cx < 0) cx = 0; else if (cx >= gridCols) cx = gridCols - 1;
            if (cy < 0) cy = 0; else if (cy >= gridRows) cy = gridRows - 1;

            // Add this particle index into the correct cell bucket
            gridCells[cy * gridCols + cx].push_back(i);
        }

        // Pick whether to use the UI interaction override or the default interactions matrix
        const auto &interactionMatrix = gridValues ? *gridValues : interactions;

        // Go thru each particle for interactions
        for (int i = 0; i < static_cast<int>(particles.size()); ++i)
        {
            const auto &pos = particles[i].GetPosition(); // Find where particle is

            // Figure out particle's position
            int cx = static_cast<int>(((pos[0] + 1.0f) * 0.5f) * gridCols);
            int cy = static_cast<int>(((pos[1] + 1.0f) * 0.5f) * gridRows);

            if (cx < 0) cx = 0; else if (cx >= gridCols) cx = gridCols - 1;
            if (cy < 0) cy = 0; else if (cy >= gridRows) cy = gridRows - 1;

            // Cache the row for this particle's color so the inner loop only does one vector lookup
            const auto &colorRow = interactionMatrix[particles[i].getColorID()];

            // Only check neighboring cells within the possible interaction radius
            // We do not compare all particles to all other particles, only those in nearby grid buckets.
            for (int oy = -maxCellOffset; oy <= maxCellOffset; ++oy)
            {
                int ncy = cy + oy;
                if (ncy < 0 || ncy >= gridRows) continue; // skip cells outside the grid

                for (int ox = -maxCellOffset; ox <= maxCellOffset; ++ox)
                {
                    int ncx = cx + ox;
                    if (ncx < 0 || ncx >= gridCols) continue; // skip cells outside the grid

                    const auto &cell = gridCells[ncy * gridCols + ncx];
                    // Each neighboring bucket contains particle indices that may interact with this particle
                    for (int j : cell)
                    {
                        if (j == i) continue; // skip self
                        particles[i].Interact(particles[j], interactRange, repelRange,
                            interactForce * colorRow[particles[j].getColorID()], repelForce, simDelta);
                    }
                }
            }

            // Update the current particle after applying all nearby forces for this frame
            particles[i].Update(simDelta);
        }

        // Keep the render buffer synchronized with the current particle set so removed particles do not linger on screen
        circles.resize(particles.size());
        for (size_t i = 0; i < particles.size(); ++i)
        {
            circles[i] = particles[i].GetProperties();
        }

        // Remove any selected entries that no longer correspond to live particles
        std::vector<void*> prunedSelection;
        std::vector<float> prunedMarkerSizes;
        prunedSelection.reserve(world.selected.size());
        prunedMarkerSizes.reserve(world.selected.size());

        for (size_t i = 0; i < world.selected.size(); ++i)
        {
            void *entry = world.selected[i];
            if (!entry)
                continue;

            const auto *candidate = static_cast<const Particle*>(entry);
            const bool stillLive = std::any_of(particles.begin(), particles.end(), [candidate](const Particle &particle)
            {
                return &particle == candidate;
            });

            if (stillLive)
            {
                prunedSelection.push_back(entry);
                prunedMarkerSizes.push_back(world.selectedMarkerSizes[i]);
            }
        }

        world.selected = std::move(prunedSelection);
        world.selectedMarkerSizes = std::move(prunedMarkerSizes);

        // Clears screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Draws all circles
        renderer.DrawBatch(circles, worldShader, aspect);

        world.DisplaySelection(uiShader);
        world.DisplayAreaSelection(uiShader);

        glfwPollEvents();

        panel.Update(uiShader, textShader, window);

        const bool worldspaceSelected = UI::Events::rightSelectionActive;
        const bool particlesSelected = !world.selected.empty();
        const float amountSliderValue = std::max(0.0f, subPanel.GetSliderValue(amountSlider));
        const int particlesToSpawn = 1 + static_cast<int>(std::round(amountSliderValue * 9999.0f));
        subPanel.SetText(amountText, "Number of Particles: " + std::to_string(particlesToSpawn));
        subPanel.SetActive(particlesSelected || worldspaceSelected);
        subPanel.SetElementActive(amountText, worldspaceSelected);
        subPanel.SetElementActive(amountSlider, worldspaceSelected);
        subPanel.SetElementActive(spawnButton, worldspaceSelected);
        subPanel.SetElementActive(colorDropdown, worldspaceSelected);
        subPanel.SetElementActive(deleteButton, particlesSelected);
        subPanel.Update(uiShader, textShader, window);

        // Do particles need to be selected
        if (UI::Events::selectionRequested)
        {

            // "Accept" request
            UI::Events::selectionRequested = false;
            world.selected.clear();
            world.selectedMarkerSizes.clear();

            // Make sure event didn't start on panel
            if (!UI::Events::leftMouseDownStartedOnPanel)
            {
                // Get current mouse position
                double xCurrent, yCurrent;
                glfwGetCursorPos(window, &xCurrent, &yCurrent);

                // Get current window aspect ratio
                int w, h;
                glfwGetWindowSize(window, &w, &h);

                // Update cursor pos based on aspect ratio
                const double currentX = (xCurrent / w) * 2.0 - 1.0;
                const double currentY = -((yCurrent / h) * 2.0 - 1.0);

                // Get where select event began
                const double startX = UI::Events::selectionStartPos[0];
                const double startY = UI::Events::selectionStartPos[1];

                // Make sure drag isn't too small
                const bool dragSelection = std::hypot(currentX - startX, currentY - startY) > 0.001;

                // Get start and end positions of selection
                const double minX = std::min(startX, currentX);
                const double maxX = std::max(startX, currentX);
                const double minY = std::min(startY, currentY);
                const double maxY = std::max(startY, currentY);

                // Loop through each particle
                for (size_t i = 0; i < particles.size(); ++i)
                {
                    // Get it's position
                    const auto &pos = particles[i].GetPosition();

                    // Decide if it is inside the selection
                    const bool inside = dragSelection
                        ? (minX <= pos[0] && pos[0] <= maxX && minY <= pos[1] && pos[1] <= maxY)
                        : (std::fabs(pos[0] - currentX) <= radius * 2.0 && std::fabs(pos[1] - currentY) <= radius * 2.0);

                    // Add to list
                    if (inside)
                    {
                        world.selected.push_back(&particles[i]);
                        world.selectedMarkerSizes.push_back(static_cast<float>(radius * 1.15));
                    }
                }
            }
        }

        // Delete all selected particles when button is pressed
        if(particlesSelected && subPanel.IsButtonDown(deleteButton))
        {
            std::vector<size_t> indicesToRemove;
            indicesToRemove.reserve(world.selected.size());

            for(void *ptr : world.selected)
            {
                // Get particle from ptr
                Particle *particle = static_cast<Particle*>(ptr);
                auto it = std::find_if(particles.begin(), particles.end(), [particle](const Particle &candidate)
                {
                    return &candidate == particle;
                });

                // Record the particle's index so it can be removed later
                if (it != particles.end())
                    indicesToRemove.push_back(static_cast<size_t>(std::distance(particles.begin(), it)));
            }

            std::sort(indicesToRemove.begin(), indicesToRemove.end(), std::greater<size_t>());
            for (size_t index : indicesToRemove)
                particles.erase(particles.begin() + index);

            world.selected.clear();
            world.selectedMarkerSizes.clear();
            subPanel.ResetButton(deleteButton);
        }

        // Spawn selected color particles randomly inside the selected worldspace when button is pressed
        if(subPanel.IsButtonDown(spawnButton))
        {
            if (UI::Events::rightSelectionActive)
            {
                // Store bounds for the selected worldspace
                const double minX = std::min(UI::Events::rightSelectionStartPos[0], UI::Events::rightSelectionCurrentPos[0]);
                const double maxX = std::max(UI::Events::rightSelectionStartPos[0], UI::Events::rightSelectionCurrentPos[0]);
                const double minY = std::min(UI::Events::rightSelectionStartPos[1], UI::Events::rightSelectionCurrentPos[1]);
                const double maxY = std::max(UI::Events::rightSelectionStartPos[1], UI::Events::rightSelectionCurrentPos[1]);

                // Read the dropdown's currently selected color index
                int selectedColorIndex = subPanel.GetDropdownSelectedIndex(colorDropdown);
                if (selectedColorIndex < 0)
                    selectedColorIndex = 0;

                // Add the requested amount of particles scattered uniformly inside the selected worldspace
                for (int i = 0; i < particlesToSpawn; i++)
                {
                    const float x = static_cast<float>(minX + (rand() / (float)RAND_MAX) * (maxX - minX));
                    const float y = static_cast<float>(minY + (rand() / (float)RAND_MAX) * (maxY - minY));

                    particles.emplace_back(gfx::Circle{x, y, radius, Particle::GetColor(selectedColorIndex, 1.0f, variety)}, selectedColorIndex);
                }
            }

            subPanel.ResetButton(spawnButton);
        }

        // Reupdate interactions and rerender grid when rerandomize button is clicked
        if (panel.IsButtonDown(rerandomizeButton))
        {
            RandomizeInteractions(interactions);
            panel.SetGridBoxCount(grid, variety);
            panel.ResetButton(rerandomizeButton);
        }

        // Swaps visible and write buffers
        glfwSwapBuffers(window);

        // Get current cursor position
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);
    }

    // Ends program and removes all OpenGL related stuff
    glDeleteProgram(worldShader);
    glDeleteProgram(uiShader);
    glDeleteProgram(textShader);
    glfwTerminate();
    return 0;
}
