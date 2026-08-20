#include "config.h"


// Forward declare Particle so helper functions may appear before its full definition.
class Particle;

// RunLoop implementation moved to `src/RunLoop.cpp` to keep `main.cpp` concise.
// See `src/RunLoop.h` for the public signature of `RunMainLoop`.
#include "RunLoop.h"

// Simulation constants and particle types are defined in headers to keep main concise
#include "SimConfig.h"
#include "Particle.h"

static unsigned int CompileShader(const std::string& source, unsigned int type)
{
    unsigned int id = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int result = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        int length = 0;
        glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
        std::string message(length, '\0');
        glGetShaderInfoLog(id, length, &length, message.data());
        std::cerr << "Shader compile error:\n" << message << std::endl;
        glDeleteShader(id);
        return 0;
    }
    return id;
}

/* Shader Initialization */
// Links shader to program then removes the intermediate compiled shader
static unsigned int CreateShader(const std::string& vertex, const std::string& fragment)
{
    unsigned int program = glCreateProgram();
    unsigned int vs = CompileShader(vertex, GL_VERTEX_SHADER);
    unsigned int fs = CompileShader(fragment, GL_FRAGMENT_SHADER);
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glValidateProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

/* Shader Loader */
// Turns shader file of given path to a string
static std::string LoadShaderFile(const std::string& path)
{
    std::ifstream file(path);
    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

/* Shader Initialization */
// Creates shader and mounts it to state machine
static unsigned int InitializeShader(const std::string path)
{
    std::string shaderSrc = LoadShaderFile(path);

    const std::string vertexMarker = "#shader vertex";
    const std::string fragmentMarker = "#shader fragment";

    const std::size_t vertexPos = shaderSrc.find(vertexMarker);
    const std::size_t fragmentPos = shaderSrc.find(fragmentMarker);

    std::string vs = shaderSrc.substr(vertexPos + vertexMarker.size());
    vs = vs.substr(0, fragmentPos - (vertexPos + vertexMarker.size()));

    std::string fs = shaderSrc.substr(fragmentPos + fragmentMarker.size());

    unsigned int shader = CreateShader(vs, fs);
    glUseProgram(shader);
    return shader;

}

// RunMainLoop is implemented in src/RunLoop.cpp

int main(int argc, char** argv)
{
    #pragma region Initialization

    srand((unsigned int)time(NULL));

    RandomizeInteractions(interactions);

    // If run with --bench, advise and exit (headless benchmark implementation
    // is omitted from this TU to keep files concise).
    if (argc > 1 && std::strcmp(argv[1], "--bench") == 0)
    {
        std::cout << "Benchmark mode not available in this build.\n";
        return 0;
    }

    // Clamp the number of particle types to the number of available colors
    if (variety > colorsCount) variety = colorsCount;

    // Initialize GLFW
    if (!glfwInit()) { std::cerr << "GLFW failed to start." << std::endl; return -1; }

    // Create window and context
    GLFWwindow* window = glfwCreateWindow(aspectRatio[0], aspectRatio[1], programName, nullptr, nullptr);
    if (!window) { std::cerr << "glfwCreateWindow failed." << std::endl; return -1; }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) { std::cerr << "GLEW failed to start." << std::endl; return -1; }

    // Resolve project root for assets. Avoid crashing when the process is
    // launched from an environment with no valid current working directory.
    std::filesystem::path projectRoot;
    try { projectRoot = std::filesystem::canonical("/proc/self/exe").parent_path(); }
    catch (...) {
        try { projectRoot = std::filesystem::current_path(); }
        catch (...) { projectRoot = std::filesystem::path("/home/arobertson/Documents/GitHub/Particle-Life"); }
    }
    if (projectRoot.filename() == "build") projectRoot = projectRoot.parent_path();
    if (projectRoot.empty() || !std::filesystem::exists(projectRoot))
        projectRoot = std::filesystem::path("/home/arobertson/Documents/GitHub/Particle-Life");

    // Initialize shaders
    GLuint worldShader = InitializeShader((projectRoot / "res/shaders/world.shader").string());
    GLuint uiShader = InitializeShader((projectRoot / "res/shaders/UI.shader").string());
    GLuint textShader = InitializeShader((projectRoot / "res/shaders/text.shader").string());

    // Renderer and particle buffers
    gfx::CircleRenderer renderer;
    renderer.CreateBuffer(worldShader, 6);

    std::vector<Particle> particles = InitializeParticles(count, static_cast<int>(variety));
    std::vector<gfx::Circle> circles; circles.resize(particles.size());

    std::vector<float> posX, posY, velX, velY; std::vector<int> colorIDs;
    SyncParticlesToSoA(particles, posX, posY, velX, velY, colorIDs);

    float aspect = aspectRatio[0] / (float)aspectRatio[1];

    // UI setup
    UI::Panel panel{side, length, {1,1,1,.5f}};

    UI::element grid = panel.AddGrid(0, .3f, .7f, variety, &interactions, true, aspect);

    UI::element timeText = panel.AddTextElement(35, 100, {-.99f, .95f}, fontPath, true, "Time Speed: 10%", false);
    UI::element timeSlider = panel.AddSlider(.1f, .947f, .2f, .04f, timeSpeed);

    UI::element varietyText = panel.AddTextElement(35, 1000, {-.99, .85}, fontPath, true, std::string("Color Variety: ") + std::to_string(variety), false);
    UI::element varietySlider = panel.AddSlider(.1f, .85f, .2f, .04f, 3.0f/colorsCount);

    UI::element rerandomizeButton = panel.AddButton(0, .7f, .16f, .055f, {.35f, .35f, .35f, .9f}, textShader, "Randomize", fontPath, 20);
    UI::element clearAllButton = panel.AddButton(0, -.9f, .18f, .06f, {.75f, .25f, .25f, .9f}, textShader, "Clear All", fontPath, 20);

    UI::element interactForceSlider = panel.AddSlider(.1f, -.2f, .2f, .04f, interactForce);
    UI::element repelForceSlider = panel.AddSlider(.1f, -.3f, .2f, .04f, repelForce);
    UI::element interactRangeSlider = panel.AddSlider(.1f, -.4f, .2f, .04f, interactRange);
    UI::element repelRangeSlider = panel.AddSlider(.1f, -.5f, .2f, .04f, repelRange);

    UI::element interactForceText = panel.AddTextElement(35, 1000, {-.99, -.2 + .003}, fontPath, true, std::string("Interact Force: ") + std::to_string(std::llround(interactForce*100)), false);
    UI::element repelForceText = panel.AddTextElement(35, 1000, {-.99, -.3 + .003}, fontPath, true, std::string("Repel Force: ") + std::to_string(std::llround(repelForce*100)), false);
    UI::element interactRangeText = panel.AddTextElement(35, 1000, {-.99, -.4 + .003}, fontPath, true, std::string("Interact Range: ") + std::to_string(std::llround(interactRange*100)), false);
    UI::element repelRangeText = panel.AddTextElement(35, 1000, {-.99, -.5 + .003}, fontPath, true, std::string("Repel Range: ") + std::to_string(std::llround(repelRange*100)), false);

    panel.LinkElements(timeText, timeSlider);
    panel.LinkElements(varietyText, varietySlider);
    panel.LinkElements(interactForceText, interactForceSlider);
    panel.LinkElements(repelForceText, repelForceSlider);
    panel.LinkElements(interactRangeText, interactRangeSlider);
    panel.LinkElements(repelRangeText, repelRangeSlider);

    std::vector<std::string> colorNames; std::vector<std::array<float,4>> colorOptions;
    colorNames.reserve(variety); colorOptions.reserve(variety);
    for (unsigned int i = 0; i < variety; ++i) { colorNames.push_back(Particle::GetColorName(static_cast<int>(i), variety)); colorOptions.push_back(Particle::GetColor(static_cast<int>(i), .9f, variety)); }

    UI::SubPanel subPanel(.5f, .01f, {.16f, .16f, .18f, .75f});
    UI::element amountText = subPanel.AddTextElement(18, 100, -.2f, .18f, fontPath, true, "Number of Particles: 100", false);
    UI::element amountSlider = subPanel.AddSlider(.1f, .18f, .2f, .04f, 99.0f / 9999.0f);
    UI::element spawnButton = subPanel.AddButton(.12f, .1f, .1f, .06f, {.35f, .65f, .9f, .9f}, textShader, "Spawn", fontPath, 22);
    UI::element deleteButton = subPanel.AddButton(0.0f, 0.0f, .22f, .055f, {.7f, .25f, .25f, .9f}, textShader, "Delete", fontPath, 22);
    UI::element colorDropdown = subPanel.AddDropdownButton(-.1f, .1f, .22f, .06f, colorNames, colorOptions, textShader, fontPath, 20);
    subPanel.SetActive(false);
    panel.Init(uiShader, textShader, window);
    subPanel.Init(uiShader, textShader);

    // Initialize optional GPU compute
    ParticleCompute compute;
    bool gpuAvailable = false;
    try { gpuAvailable = compute.Init((projectRoot / "dependencies/ParticleCompute/compute.comp").string()); } catch (...) { gpuAvailable = false; }

    RunMainLoop(
        window, panel, subPanel, grid, timeText, timeSlider, varietyText, varietySlider,
        rerandomizeButton, clearAllButton,
        interactForceSlider, repelForceSlider, interactRangeSlider, repelRangeSlider,
        interactForceText, repelForceText, interactRangeText, repelRangeText,
        amountText, amountSlider, spawnButton, deleteButton, colorDropdown,
        renderer, worldShader, uiShader, textShader,
        particles, posX, posY, velX, velY, colorIDs, circles,
        aspect, gpuAvailable ? &compute : nullptr, gpuAvailable
    );

    return 0;
}
