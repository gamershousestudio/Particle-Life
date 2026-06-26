#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <array>
#include <iostream>
#include <cmath>
#include <map>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace UI
{
    struct Events
    {
        static bool leftMouseDown;
        static bool leftMouseDownStartedOnPanel;
        static std::array<double, 2> pressedPos;

        static bool draggingPanel;

        static bool defaultCursor;
        static bool horResizeCursor;
    };

    // Structure: element definition
    struct element
    {
        friend class Panel;
        friend class Element;

        private:
            unsigned int id;

        public:
            element() = default;
            element(unsigned int id) : id(id) {} // ID cannot be used after it is created

            bool operator==(const element& other) const
            {
                return id == other.id;
            }
    };

    // Class: UI element
    class Element
    {
        protected:
            // UI / rendering variables
            GLuint vao = 0;
            GLuint vbo = 0;
            GLuint ebo = 0;

            GLuint posLoc;
            GLuint scaleLoc;
            GLuint colorLoc;

            // Attributes of all objects
            std::array<float, 4> color;

            element id;

            unsigned int indices[6] = { // What indices from the positions to use to render each triangle (instead of buffering extra positions)
                    0, 1, 2,
                    2, 3, 0
                };

            float vertices[8] = {
                    -0.5f, -0.5f,
                    0.5f, -0.5f,
                    0.5f,  0.5f,
                    -0.5f,  0.5f
                };

        public:
            virtual void Init(GLuint shader) {}
            virtual void Draw(GLuint shader) {}
            virtual void Scroll(GLFWwindow *window, double xOffset, double yOffset) {}

            Element(std::array<float, 4> pos, std::array<float, 4> color);
            Element(std::array<float, 4> pos);
            Element();

            virtual ~Element() = default; // If object it is attached to gets deleted, it will also delete

            std::array<float, 4> pos; // Public: can be accessed via static event handlers
    };

    // Structure: everything each element needs
    struct ElementHandle
    {
        std::unique_ptr<Element> ptr; // Pointer to each element
        element id; // Every elements' id
        bool active; // If each element is active, true by default
    };

    class World 
    {
        friend class Panel;

        protected:
            // UI / rendering variables
            GLuint vao = 0;
            GLuint vbo = 0;
            GLuint ebo = 0;

            GLuint posLoc;
            GLuint scaleLoc;
            GLuint colorLoc;

            float aspect;

            // Red
            std::array<float, 4> color = {1, .05, .05, .5};

        public:
            World(); // Default world constructor
            void LeftDrag(GLFWwindow *window, GLuint shader); // Drag callback

    };

    class Panel
    {
        unsigned int nextId;

        std::vector<ElementHandle> elements; // Every UI attached to panels

        int side; // Side on which the panel will appear; 0 = left, 1 = right
        float length; // How long the panel should be
        std::array<float, 4> color;

        GLuint vao, vbo, ebo;
        GLuint posLoc, colorLoc, scaleLoc;

        UI::World world; // Instance of world, defines outside of panel

        Events events; // Defines all current events;

        void RecalculateElementPositions(float oldLength);

        unsigned int indices[6] = { // What indices from the positions to use to render each triangle (instead of buffering extra positions)
                0, 1, 2,
                2, 3, 0
            };

        float vertices[8] = {
                -0.5f, -0.5f,
                0.5f, -0.5f,
                0.5f,  0.5f,
                -0.5f,  0.5f
            };
        
        GLFWcursor *resizeHorizontalCursor;

        float edge; // Edge of panel as drawn

        static void ScrollCallback(GLFWwindow *window, double xOffset, double yOffset); // Function ran when scroll input is detected
        static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods); // Function ran when mouse button input is detected

        public:
            Panel(); // Default constructor; defines empty panel
            Panel(int side, float length, std::array<float, 4> color); // Full constructor; defines panel with given size from given side of the screen and a given color
            void AddElement(Element element); // Adds new element to panel
            void Init(GLuint shader, GLFWwindow *window); // Loads panel and all elements to be drawn
            void Draw(GLuint shader); // Draws panel and  all attached elements
            void UpdateCursor(GLFWwindow *window); // Updates the current displayed cursor

            void Update(GLuint shader, GLFWwindow *window); // Panel update function; ran every frame

            std::vector<std::vector<float>> *GetGridValues(element &grid); // Return the current interactions matrix for a grid

            // Constructors for other elements
            element AddGrid(float xOffset, float yCenter, float length, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect); // Create a grid
            element AddTextElement(float fontSize, unsigned int charactersPerLine, std::array<float, 2> center, std::string font, bool autoShrink, std::string text="", bool startAtCenter=true); // Create a panel
    };

    // Class: Grid
    // Used to store info for each square(so making vector is easier)
    struct Square
    {
        std::array<float, 4> pos; // {x0, y0, x1, y1}
        std::array<float, 4> color;
    };

    class Grid : public Element
    {
        // Private functions can only be accessed by Panel instances
        friend class Panel;

        std::vector<std::vector<float>> *values;
        std::vector<Square> boxes;
        int boxesCount; // Number of boxes per row/column (not total area)
        bool useInputs; // If grid spaces should look for inputs
        float aspect; // Aspect ratio of scene; prevents morphing

        float panelCenter;

        float xOffset;
        float yCenter;
        float length;

        static void Interact(); // Checks for mouse input to update grid spaces if any is detected

        Grid(); // Default constructor; defines empty grid
        Grid(float xOffset, float yCenter, float length, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect); // Full constructor;

        void Init(GLuint shader) override; // Loads grid squares to be drawn
        void Draw(GLuint shader) override; // Draws all grid spaces
        void RecalculateSquares(); // Recalculates the position for each square
        void Scroll(GLFWwindow *window, double xOffset, double yOffset) override; // Callback when mouse scrolls over element
    };

    struct Character 
    {
        unsigned int textureID; // ID handle of the glyph texture
        glm::ivec2 size; // Size of glyph
        glm::ivec2 bearing; // Offset from baseline to left/top of glyph
        FT_Pos advance; // Offset to advance to next glyph
    };

    // Class: Text Area (later add ability to change text)
    class TextArea : public Element
    {
        // Private functions can only be accessed by Panel instances
        friend class Panel;

        std::string text;
        std::string font;
        float fontSize;

        int maxCharactersPerLine;
        bool autoShrink; // Shrink font size once it reaches maxCharactersPerLine

        std::map<char, Character> Characters; // List of variables that define each possible character

        std::array<float, 2> center;
        std::array<float, 3> color = {1, 1, 1};

        FT_Library library; // Freetype library instance
        FT_Face face; // Freetype font instance

        bool startDrawingTextAtCenter; // If center should be the center of text or the leftmost portion/where drawing starts

        TextArea(); // Default constructor; defines empty text area
        TextArea(float fontSize, unsigned int charactersPerLine, std::array<float, 2> center, std::string font, bool autoShrink, std::string text, bool startAtCenter);

        void SetText(std::string text); // Change what the text says
        std::string GetText(); // See the current text saved to this instance
        void SetFontSize(int size); // Sets font height in pixels
        void LoadFont(); // Loads a set of characters based in info such as font, size, etc.
        void Init(GLuint shader) override; // Initializes text to be drawn along with other necessary freetype information
        void Draw(GLuint shader) override; // Renders text
    };

    // Class: Slider

}

#endif