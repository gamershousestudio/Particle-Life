#include "config.h"

/* Constants */
#pragma region Constants

const char* programName = "Particle Life";

const std::array<int, 2> aspectRatio = {1600, 900}; // Width, height

const double clusterFactor = 1; // Increases motivation to form clusters

const double interactRange = .03 * clusterFactor;
const double repelRange = .013 * clusterFactor;

const double interactForce = 50;
const double repelForce = 5;

const double programSpeed = .5;

const double radius = .01;

const int count = 10000;
unsigned int variety = 2; // Total number of different particle types to use; not marked as const as it is changed if too large in main()

const bool punishClusters = false;

const bool side = 0; // Left = 0; right = 1
const float length = .6f;

#pragma endregion

// Custom enum for colors
enum class Color { Red, Green, Blue, Orange, Yellow, Pink, Purple, White, Count };
const int colorsCount = static_cast<int>(Color::Count);

// Interactions matrix
// Can only be as large as the number of possible colors, but can be smaller 
std::vector<std::vector<float>> interactions(colorsCount, std::vector<float>(colorsCount));

/* Particle Class */
// Extention of body class; more particle specific information
class Particle : public body::Body
{
    // Radius of the circle
    float radius;
    Color color;

    public:
        /* Color lookup */
        // Returns color's rgb value based on a given color
        static std::array<float, 4> GetColor(Color name, float a)
        {
            switch(name)
            {
                case Color::Red: return {1, 0, 0, a};
                case Color::Green: return {0, 1, 0, a};
                case Color::Blue: return {0, 0, 1, a};
                case Color::Orange: return {1, .647, 0, a};
                case Color::Yellow: return {1, 1, 0, a};
                case Color::Pink: return {1, .753, .796, a};
                case Color::Purple: return {.502, 0, .502, a};
                case Color::White: return {1, 1, 1, a};

                default: return {0, 0, 0, a};
            }
        }

        // Particle constructor; takes a gfx::Circle
        Particle(const gfx::Circle pos, const Color &color)
        {
            SetPosition(pos.x, pos.y);
            radius = pos.radius;
            this->color = color;

            stable = !punishClusters;
        }

        /* Particle Properties */
        // Returns particle as a gfx::circle
        const gfx::Circle GetProperties()
        {
            return (gfx::Circle) {(float)position[0], (float)position[1], radius, GetColor(color, 1)};
        }

        const int getColorID()
        {
            return static_cast<int>(color);
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
            // Random positioning for particles(between 0 and 1)
            float x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            float y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;

            // Adds new particle to list
            particles.emplace_back(gfx::Circle{x, y, radius, Particle::GetColor(static_cast<Color>(i), 1)}, static_cast<Color>(i));
        }
    }

    return particles;
}

/* Functions for Graphics Rendering */
#pragma region Graphics Functions

/* Shader Compiler*/
// Returns shader as string from basic.shader
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

    // Gets shader compile error if shader failed to load
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

    // Allows only so many colors
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
    renderer.CreateBuffer(worldShader, 16);

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

    // TEXT STUFF
    std::array<float, 2> position = {-1, 0};
    UI::element text = panel.AddTextElement(20, 0, position, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", false, "", false);


    // Interactions matrix creation
    const float buffer = .05;
    const float size = .25;

    const float yCenter = .5;

    // Center of drawing area calculation
    float sideVal = (!side) ? -1 : 1;

    float xStart = sideVal-(sideVal*buffer);
    float xEnd = sideVal*(1-(length))+(sideVal*buffer);

    float center = (xStart+xEnd)/2;

    // Where to place interactions matrix
    float x0 = center-size;
    float x1 = center+size;
    float y0 = yCenter+size;
    float y1 = yCenter-size;

    // Top left & bottom right corners
    std::array<float, 4> positions = {x0, y0, x1, y1};

    UI::element grid = panel.AddGrid(positions, variety, &interactions, true, aspect);

    // Initialize everything
    panel.Init(uiShader, window);

    #pragma endregion

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
        currentTime = std::chrono::steady_clock::now();

        // Calculates time distance between current and previous frame
        std::chrono::duration<float> elapsed = currentTime - lastTime;
        float deltaTime = elapsed.count();
        float simDelta = deltaTime * programSpeed; // scale the simulation timestep by user speed

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

        // Gets the circle properties from each particle that needs to be rendered
        for (size_t i = 0; i < particles.size(); ++i)
            circles[i] = particles[i].GetProperties();

        // Clears screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Draws all circles
        renderer.DrawBatch(circles, worldShader, aspect);

        panel.Draw(uiShader);

        // Clears user events buffer
        glfwPollEvents();

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