#ifndef UI_H
#define UI_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <vector>
#include <array>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <map>
#include <memory>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "body.h"

namespace UI
{
    struct Events
    {
        static bool leftMouseDown;
        static bool leftMouseDownStartedOnPanel;
        static std::array<double, 2> pressedPos;
        static std::array<double, 2> selectionStartPos;
        static bool selectionRequested;

        static bool rightMouseDown;
        static bool rightMouseDownStartedOnPanel;
        static std::array<double, 2> rightSelectionStartPos;
        static std::array<double, 2> rightSelectionCurrentPos;
        static bool rightSelectionActive;

        static bool draggingPanel;

        static bool defaultCursor;
        static bool horResizeCursor;
    };

    class SubPanel;

    // Structure: element definition
    struct element
    {
        friend class Panel;
        friend class SubPanel;
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
            virtual void HandleInput(GLFWwindow *window) {}

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
            std::array<float, 4> areaSelectionColor = {0.15f, 0.35f, 1.0f, 0.25f};

        public:
            std::vector<void*> selected;
            std::vector<float> selectedMarkerSizes;

            World(); // Default world constructor
            void LeftDrag(GLFWwindow *window, GLuint shader); // Drag callback
            void DisplaySelection(GLuint shader); // Render little boxes around selected particles
            void DisplayAreaSelection(GLuint shader); // Render the persistent right-click area selection rectangle

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

        struct LinkedGroup
        {
            element first;
            element second;
            float medianOffsetFromPanelCenter;
            float firstOffsetFromMedian;
            float secondOffsetFromMedian;
        };

        std::vector<LinkedGroup> linkedGroups;

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
            void Init(GLuint uiShader, GLuint textShader, GLFWwindow *window); // Loads panel and all elements to be drawn
            void Draw(GLuint uiShader, GLuint textShader); // Draws panel and  all attached elements
            void UpdateCursor(GLFWwindow *window); // Updates the current displayed cursor

            void Update(GLuint uiShader, GLuint textShader, GLFWwindow *window); // Panel update function; ran every frame

            float GetSliderValue(element slider); // Gets the given slider's normalized value
            bool IsButtonDown(element button); // Returns the current status of the button
            void ResetButton(element button); // Resets the button state after handling it
            void SetText(element textElement, std::string text); // Updates a text element
            void SetGridBoxCount(element grid, unsigned int count); // Updates the visible size of a grid element
            void SetElementActive(element elementId, bool active); // Enables or disables an element from input/rendering
            int GetDropdownSelectedIndex(element dropdown); // Gets currently selected dropdown option
            void SetDropdownOptions(element dropdown, const std::vector<std::string> &labels, const std::vector<std::array<float, 4>> &optionColors); // Updates a dropdown's visible options

            World& GetWorld() { return world; }
            const World& GetWorld() const { return world; }

            std::vector<std::vector<float>> *GetGridValues(element &grid); // Return the current interactions matrix for a grid

            // Constructors for other elements
            element AddGrid(float xOffset, float yCenter, float length, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect); // Create a grid
            element AddTextElement(float fontSize, unsigned int charactersPerLine, std::array<float, 2> center, std::string font, bool autoShrink, std::string text="", bool startAtCenter=true); // Create a text element
            element AddSlider(float xOffset, float yCenter, float length, float totalHeight, float defaultValue); // Create a slider
            element AddButton(float xOffset, float yCenter, float width, float height, std::array<float, 4> color, GLuint textShader, std::string text = "", std::string font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int fontSize = 18, bool autoShrink = true); // Create a button
            void LinkElements(element first, element second); // Link two elements so they scale and move together
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
        Grid(float xOffset, float yCenter, float length, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect); // Full constructor

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
        // Private functions can only be accessed by Panel and Button instances
        friend class Panel;
        friend class Button;

        public:
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
            void SetCenter(std::array<float, 2> center); // Updates the text drawing center
            void RecalculatePosition(); // Updates pos bounds from center and text size
            void LoadFont(); // Loads a set of characters based in info such as font, size, etc.
            void Init(GLuint shader) override; // Initializes text to be drawn along with other necessary freetype information
            void Draw(GLuint shader) override; // Renders text
    };

    // Class: Sliders
    class Slider : public Element
    {
        friend class Panel;
        friend class SubPanel;

        static Slider* activeSlider;

        float normalizedValue;
        bool isDragging;

        float length, height;
        float xOffset, yCenter;
        float panelCenter;

        GLuint shapeLoc = -1;
        GLuint radiusLoc = -1;

        Slider(); // Default constructor; defines empty slider
        Slider(float xOffset, float yCenter, float length, float totalHeight, float defaultValue); // Full constructor

        void Init(GLuint shader) override; // Loads slider rect and circle
        void Draw(GLuint shader) override; // Draws slider on window
        void HandleInput(GLFWwindow *window) override; // Updates the slider value from pointer input
        void RecalculatePosition(); // Repositions the slider relative to the panel

        float GetNormalizedValue() const { return normalizedValue; } // Return the normalized value of the slider
    };

    class Button : public Element
    {
        friend class Panel;
        friend class SubPanel;

        bool pressed;
        bool candidate;
        float width;
        float height;
        float xOffset;
        float yCenter;
        float panelCenter;

        GLuint shapeLoc = -1;
        GLuint radiusLoc = -1;
        GLuint textShader = 0;
        std::unique_ptr<TextArea> textOverlay;
        std::string textFont;
        int textFontSize;
        bool textAutoShrink;
        std::string buttonText;

        Button();
        Button(float xOffset, float yCenter, float width, float height, std::array<float, 4> color, GLuint textShader, std::string text = "", std::string font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int fontSize = 18, bool autoShrink = true);

        void Init(GLuint shader) override; // Loads necessary shader for drawing button
        void Draw(GLuint shader) override; // Draws button and text
        void HandleInput(GLFWwindow *window) override; // Updates if the button is currently being pressed
        void InitTextOverlay(GLuint shader); // Adds text overlay
        void RecalculatePosition(); // Updates position when panel moves

        void SetText(const std::string &text); // Updates displayed text

        bool IsPressed() const { return pressed; } // Returns if button is/isn't pressed
        void ResetPressed() { pressed = false; } // Resets button state
    };

    class DropdownButton : public Element
    {
        friend class Panel;
        friend class SubPanel;

        std::vector<std::string> labels;
        std::vector<std::array<float, 4>> optionColors;
        std::unique_ptr<TextArea> selectedTextOverlay;
        std::vector<std::unique_ptr<TextArea>> optionTextOverlays;

        int selectedIndex;
        int candidateIndex;
        bool open;
        bool mainCandidate;
        bool mouseWasDown;

        float width;
        float height;
        float xOffset;
        float yCenter;
        float panelCenter;

        GLuint shapeLoc = -1;
        GLuint radiusLoc = -1;
        GLuint textShader = 0;

        std::string textFont;
        int textFontSize;
        bool textAutoShrink;

        DropdownButton();
        DropdownButton(float xOffset, float yCenter, float width, float height, std::vector<std::string> labels, std::vector<std::array<float, 4>> optionColors, GLuint textShader, std::string font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int fontSize = 18, bool autoShrink = true);

        void Init(GLuint shader) override; // Loads necessary shader for drawing dropdown
        void Draw(GLuint shader) override; // Draws dropdown and any currently open options
        void HandleInput(GLFWwindow *window) override; // Updates dropdown open/selected state
        void InitTextOverlays(); // Adds text overlays for the selected value and options
        void RecalculatePosition(); // Updates position when owner panel moves
        std::array<float, 4> GetOptionPosition(int index) const; // Gets position for dropdown option
        bool ContainsPoint(const std::array<float, 4> &bounds, float x, float y) const; // Checks if a point is inside given bounds

        void SetOptions(const std::vector<std::string> &newLabels, const std::vector<std::array<float, 4>> &newOptionColors); // Updates visible options and colors

        int GetSelectedIndex() const { return selectedIndex; } // Returns currently selected option
    };

    class SubPanel
    {
        unsigned int nextId;

        std::vector<ElementHandle> elements; // Every UI attached to subpanel

        float size;
        float margin;
        std::array<float, 4> color;
        bool active;

        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;

        GLuint posLoc = -1;
        GLuint scaleLoc = -1;
        GLuint colorLoc = -1;

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

        static std::vector<SubPanel*> registeredPanels;

        public:
            SubPanel(); // Default constructor; defines empty subpanel
            SubPanel(float size, float margin, std::array<float, 4> color); // Full constructor; creates a top-right square subpanel

            void Init(GLuint uiShader, GLuint textShader); // Loads subpanel and attached elements
            void Draw(GLuint uiShader, GLuint textShader); // Draws subpanel and attached elements
            void Update(GLuint uiShader, GLuint textShader, GLFWwindow *window); // Updates all attached elements

            element AddTextElement(float fontSize, unsigned int charactersPerLine, float xOffset, float yOffset, std::string font, bool autoShrink, std::string text="", bool startAtCenter=true); // Create a text element
            element AddSlider(float xOffset, float yOffset, float length, float totalHeight, float defaultValue); // Create a slider
            element AddButton(float xOffset, float yOffset, float width, float height, std::array<float, 4> color, GLuint textShader, std::string text = "", std::string font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int fontSize = 18, bool autoShrink = true); // Create a button
            element AddDropdownButton(float xOffset, float yOffset, float width, float height, std::vector<std::string> labels, std::vector<std::array<float, 4>> optionColors, GLuint textShader, std::string font = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", int fontSize = 18, bool autoShrink = true); // Create a dropdown button

            float GetSliderValue(element slider); // Gets the given slider's normalized value
            bool IsButtonDown(element button); // Returns the current status of the button
            void ResetButton(element button); // Resets the button state after handling it
            int GetDropdownSelectedIndex(element dropdown); // Gets currently selected dropdown option
            void SetDropdownOptions(element dropdown, const std::vector<std::string> &labels, const std::vector<std::array<float, 4>> &optionColors); // Updates a dropdown's visible options
            void SetText(element textElement, std::string text); // Updates a text element
            void SetElementActive(element elementId, bool active); // Enables or disables an element from input/rendering
            void SetActive(bool active); // Shows/hides the entire subpanel
            bool IsActive() const { return active; } // Returns if subpanel is active
            bool ContainsPoint(float x, float y) const; // Returns if point is inside the subpanel

            static bool AnyActivePanelContains(float x, float y); // Returns if point is inside any active subpanel
    };
}

#endif
