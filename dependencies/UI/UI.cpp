#include "UI.h"

namespace UI
{
    // All definitions for events struct variables
    #pragma region Events

    bool Events::leftMouseDown = false;
    bool Events::leftMouseDownStartedOnPanel = false;

    std::array<double, 2> Events::pressedPos = {0, 0};
    std::array<double, 2> Events::selectionStartPos = {0, 0};
    bool Events::selectionRequested = false;

    bool Events::rightMouseDown = false;
    bool Events::rightMouseDownStartedOnPanel = false;
    std::array<double, 2> Events::rightSelectionStartPos = {0, 0};
    std::array<double, 2> Events::rightSelectionCurrentPos = {0, 0};
    bool Events::rightSelectionActive = false;

    bool Events::draggingPanel;

    bool Events::defaultCursor = true;
    bool Events::horResizeCursor = false;

    #pragma endregion

    // Element class functions
    #pragma region Element

    Element::Element(std::array<float, 4> pos, std::array<float, 4> color): 
        pos(pos), color(color) {}

    Element::Element(std::array<float, 4> pos):
        pos(pos) {}

    Element::Element() {}

    #pragma endregion

    // World class functions
    #pragma region World

    World::World() {}

    /* Left Drag Click Callback */
    // Called every time is left drag clicked
    void World::LeftDrag(GLFWwindow *window, GLuint shader)
    {
        // Get current cursor position
        double x1, y1;
        glfwGetCursorPos(window, &x1, &y1);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        // Normalize x and y values
        // Divide by width/height, multiply by two, and subtract one
        // Normalizes value to [0, 1600] to [-1, 1]
        x1 = (x1/w)*2-1;
        y1 = -((y1/h)*2-1);

        // Get previous mouse pos
        float x0 = Events::pressedPos[0];
        float y0 = Events::pressedPos[1];

        // Draw rectangle from where cursor was to where it is
        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);

        // Binds shader variables
        glUniform2f(posLoc, (x0+x1)/2, (y0+y1)/2);
        glUniform2f(scaleLoc, fabs(x1-x0), fabs(y1-y0));
        glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

        // Draws circle(aka triangle fan)
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Unbinds intermediates
        glBindVertexArray(0);
    }

    /* Selection Renderer */
    // Renders little boxes around particles within the selected area
    void World::DisplaySelection(GLuint shader)
    {
        // If nothing is selected, don't display anything
        if (selected.empty())
            return;

        // Bind shader
        glUseProgram(shader);
        glBindVertexArray(vao);

        // How many stuff to loop for
        const size_t selectionCount = std::min(selected.size(), selectedMarkerSizes.size());

        // Loop through each particle
        for (size_t i = 0; i < selectionCount; ++i)
        {
            // Get current particle to view
            void *entry = selected[i];

            // If particle is null, move on
            if (!entry)
                continue;

            // Get the body
            body::Body *body = static_cast<body::Body*>(entry);
            const auto &position = body->GetPosition();

            // How big each box should be
            const float markerRadius = std::max(0.001f, selectedMarkerSizes[i]);
            const float boxSize = markerRadius * 2.0f;
            const float boxWidth = aspect > 0.0f ? boxSize / aspect : boxSize;

            // Bind stuff to draw
            glUniform2f(posLoc, position[0], position[1]);
            glUniform2f(scaleLoc, boxWidth, boxSize);
            glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

            // Draw square
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        glBindVertexArray(0);
    }

    /* Display Of Area Selector */
    // Renderers area selector as rectangle
    void World::DisplayAreaSelection(GLuint shader)
    {
        // If right selection not active, no point in being here...
        if (!Events::rightSelectionActive)
            return;

        // Bind shader stuff
        glUseProgram(shader);
        glBindVertexArray(vao);

        // Get positions for triangle
        const double x0 = Events::rightSelectionStartPos[0];
        const double y0 = Events::rightSelectionStartPos[1];
        const double x1 = Events::rightSelectionCurrentPos[0];
        const double y1 = Events::rightSelectionCurrentPos[1];

        // Turn said positions into accurate values
        const double minX = std::min(x0, x1);
        const double maxX = std::max(x0, x1);
        const double minY = std::min(y0, y1);
        const double maxY = std::max(y0, y1);

        // Use positions to get the width, height, and center
        const double width = std::max(0.001, maxX - minX);
        const double height = std::max(0.001, maxY - minY);
        const double centerX = (minX + maxX) * 0.5;
        const double centerY = (minY + maxY) * 0.5;

        // Bind uniforms
        glUniform2f(posLoc, static_cast<float>(centerX), static_cast<float>(centerY));
        glUniform2f(scaleLoc, static_cast<float>(width), static_cast<float>(height));
        glUniform4f(colorLoc, areaSelectionColor[0], areaSelectionColor[1], areaSelectionColor[2], areaSelectionColor[3]);

        // Draw elements
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Unbind intermediates
        glBindVertexArray(0);
    }

    #pragma endregion

    // Panel class functions
    #pragma region Panel

    /* Full Panel Constructor */
    // Creates a new panel with all the information needed to draw it
    Panel::Panel(int side, float length, std::array<float, 4> color): 
        side((!side) ? -1 : 1), length(length), color(color), nextId(0), vao(0), vbo(0), ebo(0), posLoc(-1), colorLoc(-1), scaleLoc(-1) {}

    /* Grid Creation */
    // Creates a new instance of grid under the panel
    element Panel::AddGrid(float xOffset, float yCenter, float length, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect)
    {
        // Create new grid
        Grid grid(xOffset, yCenter, length, numberOfBoxes, values, useInputs, aspect);

        if(side == -1)
            grid.panelCenter = -1 + this->length / 2.0f;
        else
            grid.panelCenter = 1 - this->length / 2.0f;

        // Create element based on reference to object, value, and if it is active(true by default)
        // Smart pointers: delete themselves to make our lives easier
        // std::make_unique: creates a unique_ptr upcast for Element based on the created grid
        // Creates a pointer to the grid to edit/access variables
        elements.push_back(ElementHandle{std::make_unique<Grid>(std::move(grid)), element{nextId}, true});

        // Return the assigned element id before incrementing nextId
        element assignedId = elements.back().id;

        // Increment nextId and throw error if too many ids have been generated
        nextId++;

        if(nextId == 0) // Will loop back around after reaching integer limit
            std::cout << "ERROR: To many elements have been created under panel!" << std::endl;

        return assignedId;
    }

    /* Text Area Creation */
    // Creates a new instance of text area under the panel
    element Panel::AddTextElement(float fontSize, unsigned int charactersPerLine, std::array<float, 2> center, std::string font, bool autoShrink, std::string text, bool startAtCenter)
    {
        // Create new grid
        TextArea textArea(fontSize, charactersPerLine, center, font, autoShrink, text, startAtCenter);

        // Create element based on reference to object, value, and if it is active(true by default)
        // Smart pointers: delete themselves to make our lives easier
        // std::make_unique: creates a unique_ptr upcast for Element based on the created textArea
        // Creates a pointer to the textArea to edit/access variables
        elements.push_back(ElementHandle{std::make_unique<TextArea>(std::move(textArea)), element{nextId}, true});

        // Return the assigned element id before incrementing nextId
        element assignedId = elements.back().id;

        // Increment nextId and throw error if too many ids have been generated
        nextId++;

        if(nextId == 0) // Will loop back around after reaching integer limit
            std::cout << "ERROR: To many elements have been created under panel!" << std::endl;

        return assignedId;
    }

    /* Panel Graphics Initialization */
    // Loads all necessary info to draw panel and elements
    void Panel::Init(GLuint shader, GLFWwindow *window)
    {
        // Create and bind a valid VAO before setting up vertex attributes
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // glGenBuffers(number_of_buffers, pointer_to_buffer_to_edit)
        glGenBuffers(1, &vbo);

        // Selects buffer to edit
        // glBindBuffer(buffer_type, buffer)
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Give data
        // glBufferData(buffer_type, size_in_bytes, pointer_to_data, )
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW); // Draw specifies that it will be drawn

        // glVertexAttribPointer(index, size, type, normalized, stride, pointer)
        // Index = what index in the list refers to what (what the corresponding index actually means)
        // Size = how many of each per vertex
        // Type = type of data
        // Stride = amount of bytes between each vertex (size of each vertex in bytes)
        // Pointer = offset for piece of information in bytes (must be a pointer, can turn number into pointer by casting to (const void) *num)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0); // Called once per attribute

        // Index buffer
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

        // Enables attribute pointer; only parameter is index to enable
        glEnableVertexAttribArray(0);

        // Where each variable is located in the shader so they can be set
        glUseProgram(shader);
        glBindVertexArray(vao);

        posLoc = glGetUniformLocation(shader, "u_Position");
        scaleLoc = glGetUniformLocation(shader, "u_Scale");
        colorLoc = glGetUniformLocation(shader, "desiredColor");

        // Unbinds intermediates
        glBindVertexArray(0);

        // Updates worlds rendering stuff
        world.vbo = vbo;
        world.ebo = ebo;
        world.vao = vao;

        world.posLoc = posLoc;
        world.scaleLoc = scaleLoc;
        world.colorLoc = colorLoc;

        // Update world's aspect ratio
        int width, height;

        // Retrieve the window's current dimensions
        glfwGetWindowSize(window, &width, &height);

        // Calculate the aspect ratio
        world.aspect = (float)width / (float)height;

        // Initializes all other elements
        for (auto &e : elements)
        {
            // Makes sure element is supposed to be initialized
            if (e.active)
                // Runs element's Init() function (actual Element class's is overriden)
                e.ptr->Init(shader); // Runs element's init function
        }

        resizeHorizontalCursor = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);

        // Attach instance of panel with window
        glfwSetWindowUserPointer(window, this);

        // Initialize inputs
        glfwSetScrollCallback(window, ScrollCallback);

        glfwSetMouseButtonCallback(window, MouseButtonCallback);
    }

    /* Panel Drawing */
    // Draws panel on screen via OpenGL
    void Panel::Draw(GLuint shader)
    {
        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);


        // Makes sure shader works correctly
        if (posLoc == -1 || scaleLoc == -1 || colorLoc == -1)
        {
            std::cout << "Warning: shader uniform not found." << std::endl;
        }

        // Calculate edge of panel
        if(side == -1)
            edge = -1.0f + length;
        else
            edge = 1.0f - length;

        // Gets current viewport
        int viewport[4]; // Stores OpenGL viewport dimensions
        glGetIntegerv(GL_VIEWPORT, viewport); // Retrieves said viewport

        // Calculation of panel's left / right boundaries
        const float panelX0 = (side == -1) ? -1.0f : 1.0f - length;
        const float panelX1 = (side == -1) ? -1.0f + length : 1.0f;

        // Converts panel left edge and width into pixel-space cordinates
        const int scissorX = static_cast<int>((panelX0 + 1.0f) * 0.5f * viewport[2]);
        const int scissorWidth = std::max(0, static_cast<int>((panelX1 - panelX0) * 0.5f * viewport[2]));

        // Turns on scissor mode and tells OpenGL to only draw inside viewport bounds
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorX, viewport[1], scissorWidth, viewport[3]);

        // Binds shader variables
        glUniform2f(posLoc, (side + side*(1-length))/2, 0);
        glUniform2f(scaleLoc, fabs(side*(1-length)-side), 2);
        glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

        // Draws rectangle
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Draws all other elements
        for (auto &e : elements)
        {
            // Makes sure element is supposed to be drawn
            if (e.active)
                // Runs element's Draw() function (actual Element class's is overriden)
                e.ptr->Draw(shader); // Runs element's draw function
        }

        // Re-disables scissor mode after everything is rendered
        glDisable(GL_SCISSOR_TEST);

        // Unbinds intermediates
        glBindVertexArray(0);
    }

    /* Scroll Callback */
    // Ran every time user scrolls. Figures out where user scrolled and runs appropriate function
    void Panel::ScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
    {
        // Get panel instance
        Panel* panel = static_cast<Panel*>(glfwGetWindowUserPointer(window));

        // Get current cursor position
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        // Normalize x and y values
        // Divide by width/height, multiply by two, and subtract one
        // Normalizes value to [0, 1600] to [-1, 1]
        x = (x/w)*2-1;
        y = -((y/h)*2-1);

        // Variable to store element
        Element *elementPtr;

        // Make sure scroll occurred somewhere on the panel(x/w)*2-1
        if((panel->side == -1 && x < (-1+panel->length)) || (panel->side == 1 && x > (1-panel->length))) // Only continue if scroll area was within the panel.length
        {
            // Find what object was scrolled over
            // Loop through each object attached to the panel
            for(int i = 0; i < panel->elements.size(); i++)
            {
                // Get current element
                elementPtr = panel->elements[i].ptr.get();

                // Check if mouse is within object's position
                if((elementPtr->pos[0] <= x && x <= elementPtr->pos[2]) && (elementPtr->pos[1] <= y && y <= elementPtr->pos[3]))
                {
                    // Call said object's scroll
                    elementPtr->Scroll(window, xOffset, yOffset);

                    // End function, no point in continuing to look
                    return;
                }
            }
        }
    }

    /* Mouse Input Callback */
    // Ran whenever the state of a mouse button changes
    void Panel::MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        // Get panel instance
        Panel* panel = static_cast<Panel*>(glfwGetWindowUserPointer(window));

        // Get current cursor position
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        // Normalize x and y values
        // Divide by width/height, multiply by two, and subtract one
        // Normalizes value to [0, 1600] to [-1, 1]
        x = (x/w)*2-1;
        y = -((y/h)*2-1);

        // What mouse button was clicked
        if(button == GLFW_MOUSE_BUTTON_LEFT)
        {
            // Was it clicked or released, or has it been down
            if(!Events::leftMouseDown && action == GLFW_PRESS) // Just clicked
            {
                // Disable the blue area-selection overlay when a left-click selection starts
                Events::rightSelectionActive = false;

                // Tell event struct it has been clicked and where
                Events::leftMouseDown = true;
                Events::pressedPos = {x, y};
                Events::selectionStartPos = {x, y};
                Events::selectionRequested = false;

                // If is on border of panel
                if(Events::horResizeCursor)
                {
                    Events::draggingPanel = true;
                }
                // Make sure scroll occurred somewhere on the panel(x/w)*2-1
                else if((panel->side == -1 && x < (-1+panel->length)) || (panel->side == 1 && x > (1-panel->length))) // Only continue if scroll area was within the panel.length
                {
                    Events::leftMouseDownStartedOnPanel = true;
                }
                // Is in the world
                else
                {
                    Events::leftMouseDownStartedOnPanel = false;
                }
            }
            else if(Events::leftMouseDown && action == GLFW_RELEASE) // Has been clicked; just released
            {
                Events::leftMouseDown = false;
                Events::pressedPos = {0, 0};
                Events::draggingPanel = false;
                Events::selectionRequested = !Events::leftMouseDownStartedOnPanel;
            }
        }
        else if(button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            if(action == GLFW_PRESS) // Right mouse was just pressed
            {
                // Update event information
                Events::rightMouseDown = true;
                Events::rightSelectionStartPos = {x, y};
                Events::rightSelectionCurrentPos = {x, y};
                Events::rightSelectionActive = false;

                // Clear the red particle selection so only the blue area-selection can remain active
                panel->world.selected.clear();
                panel->world.selectedMarkerSizes.clear();

                // Make sure click did/didn't start on panel
                if((panel->side == -1 && x < (-1+panel->length)) || (panel->side == 1 && x > (1-panel->length)))
                    Events::rightMouseDownStartedOnPanel = true;
                else
                {
                    Events::rightMouseDownStartedOnPanel = false;
                    Events::rightSelectionActive = true;
                }
            }
            else if(action == GLFW_RELEASE) // Right click was released
            {
                // Update events
                Events::rightMouseDown = false;
                Events::rightSelectionCurrentPos = {x, y};
            }
        }
    }

    /* Grid Value Fetcher */
    // Gets current values for a given grid
    std::vector<std::vector<float>> *Panel::GetGridValues(element &grid)
    {
        // Find grid it's looking for
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == grid)
            {
                // Get grid
                Grid* grid = dynamic_cast<Grid*>(elements[i].ptr.get());

                if (grid)
                {
                    // Access Grid members
                    return grid -> values;
                }
                else
                    return nullptr;
            }
        }

        return nullptr;
    }

    /* Cursor Update Function */
    // Updates cursor that is currently displayed
    void Panel::UpdateCursor(GLFWwindow *window)
    {
        // Get current cursor position
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        // Normalize x and y values
        // Divide by width/height, multiply by two, and subtract one
        // Normalizes value to [0, 1600] to [-1, 1]
        x = (x/w)*2-1;
        y = -((y/h)*2-1);

        // Conditions for when special cursors are used
        if(((edge - .005) <= x) && (x <= edge + .005) && !Events::leftMouseDown) // Is cursor currently within ten pixels of the panel egde -> set to resize panel cursor
        {
            glfwSetCursor(window, resizeHorizontalCursor);
            Events::defaultCursor = false;
            Events::horResizeCursor = true;
        }
        else if(!Events::defaultCursor)
        {
            glfwSetCursor(window, 0);
            Events::defaultCursor = true;
            Events::horResizeCursor = false;
        }
    }

    /* Element Position Recalculation */
    // When rescaling panel, repositions elements to be properly aligned
    void Panel::RecalculateElementPositions(float oldLength)
    {
        // Length neglegible; no need to resize
        if (std::fabs(length - oldLength) < 1e-6f)
            return;

        // Calculate the old and new positions of the panel's edge
        const float oldEdge = (side == -1) ? -1.0f + oldLength : 1.0f - oldLength;
        const float newEdge = (side == -1) ? -1.0f + length : 1.0f - length;

        // Get difference between two
        const float xDelta = newEdge - oldEdge;

        // Defines the start and end of panel
        const float panelStart = (side == -1) ? -1.0f : newEdge;
        const float panelEnd = (side == -1) ? newEdge : 1.0f;

        // Loop through each element
        for (auto &element : elements)
        {
            // If element is grid, recalculate grid's squares and update its center
            if (auto *grid = dynamic_cast<Grid*>(element.ptr.get()))
            {
                // Grid's intended center based on new size
                const float desiredCenter = (side == -1 ? -1.0f + length / 2.0f : 1.0f - length / 2.0f) + grid->xOffset;

                // Converts logical size to visible(aka, includes aspect ratio)
                const float visibleHalfWidth = grid->length / (2.0f * std::max(grid->aspect, 1e-6f));

                // Farthest left/right positions grid can occupy to remain inside of the panel
                const float minCenter = panelStart + visibleHalfWidth;
                const float maxCenter = panelEnd - visibleHalfWidth;

                // Finds the best center based on the goal, if it is possible
                const float clampedCenter = std::max(minCenter, std::min(maxCenter, desiredCenter));

                grid->panelCenter = clampedCenter - grid->xOffset;
                grid->RecalculateSquares();
            }
            // If element is anything else, change its x position based on the panel's position changes
            else
            {
                // Total width of element currently
                const float width = element.ptr->pos[2] - element.ptr->pos[0];

                // Left panel case
                if (side == -1)
                {
                    // Prevents panel moving past right edge
                    const float clampedX1 = std::min(panelEnd, element.ptr->pos[2] + xDelta);

                    // Updates its position
                    element.ptr->pos[0] = clampedX1 - width;
                    element.ptr->pos[2] = clampedX1;
                }
                // Right panel case
                else
                {
                    // Prevents panel moving past left edge
                    const float clampedX0 = std::max(panelStart, element.ptr->pos[0] + xDelta);

                    // Updates its position
                    element.ptr->pos[0] = clampedX0;
                    element.ptr->pos[2] = clampedX0 + width;
                }
            }
        }
    }

    /* Panel Update Function */
    // Updates everything related to the panel
    void Panel::Update(GLuint shader, GLFWwindow *window)
    {
        UpdateCursor(window);

        // If right mouse is down and didnt start on panel -> do right mouse drag
        if (Events::rightMouseDown && !Events::rightMouseDownStartedOnPanel)
        {
            // Get cursor position
            double xCursor, yCursor;
            glfwGetCursorPos(window, &xCursor, &yCursor);

            // Get aspect ration
            int cursorW, cursorH;
            glfwGetWindowSize(window, &cursorW, &cursorH);

            // Scale cursor position by aspect ratio
            const double cursorX = (xCursor / cursorW) * 2.0 - 1.0;
            const double cursorY = -((yCursor / cursorH) * 2.0 - 1.0);

            // Record where trigger started
            Events::rightSelectionCurrentPos = {cursorX, cursorY};
        }

        Draw(shader);

        // If dragging is occuring outside of the panel, call the world's drag function

        // Get current cursor position
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        // Normalize x and y values
        // Divide by width/height, multiply by two, and subtract one
        // Normalizes value to [0, 1600] to [-1, 1]
        x = (x/w)*2-1;
        y = -((y/h)*2-1);

        if(!((side == -1 && x < (-1+length)) || (side == 1 && x > (1-length)))) // Only continue if scroll area was within the panel.length
            if(Events::leftMouseDown && !Events::leftMouseDownStartedOnPanel && !Events::draggingPanel)
                world.LeftDrag(window, shader);
        
        // Should panel be resized
        if(Events::leftMouseDown && Events::draggingPanel)
        {
            const float oldLength = length;

            // Get new panel length based on panel position
            if(side == -1)
                length = x + 1.0f;
            else
                length = 1.0f - x;

            // Restrict length
            if (length < 0.0f)
                length = 0.0f;
            else if (length > 2.0f)
                length = 2.0f;

            // Recalculate the positions of all entities for the new panel
            RecalculateElementPositions(oldLength);
        }
    }

    #pragma endregion

    // Grid class functions
    #pragma region Grid

    /* Full Grid Constructor */
    // Creates a new grid with all information needed to be drawn
    Grid::Grid(float xOffset, float yCenter, float length, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect):
        xOffset(xOffset), yCenter(yCenter), length(length), boxesCount(numberOfBoxes), values(values), Element(), useInputs(useInputs), aspect(aspect) {}

    /* Positioning Calculation */
    // Calculates the position for each square on a grid
    void Grid::RecalculateSquares()
    {
        // Remove all boxes
        boxes.clear();

        // Lock values based on what is expected
        float x0 = panelCenter + xOffset - length/2;
        float x1 = panelCenter + xOffset + length/2;
        float y0 = yCenter - length/2;
        float y1 = yCenter + length/2;

        this->pos = {x0, y0, x1, y1};

        // Get how big each cell should be
        float cellW = (x1 - x0) / boxesCount;
        float cellH = (y1 - y0) / boxesCount;

        // Prepares each square
        std::array<float, 4> color;
        std::array<float, 4> position;

        float centerX = panelCenter + xOffset;

        for(int i = 0; i < boxesCount; i++)
        {
            for(int j = 0; j < boxesCount; j++)
            {
                if((*values)[i][j] < 0)
                {
                    color = {-(*values)[i][j], 0, 0, 1};
                }
                else if((*values)[i][j] > 0)
                {
                    color = {0, (*values)[i][j], 0, 1};
                }
                else
                {
                    color = {.5, .5, .5, 1};
                }

                // Position calculation
                // Calculate where each x is for cell
                position[0] = x0 + i * cellW;
                position[2] = position[0] + cellW;

                position[1] = yCenter + (boxesCount/2.0 - j) * cellH;
                position[3] = position[1] - cellH;

                // Fixes xs to make them more square-ular[i][j], 0,
                position[0] = centerX + (position[0] - centerX) / aspect;
                position[2] = centerX + (position[2] - centerX) / aspect;

                boxes.emplace_back(position, color);
            }
        }
    }

    /* Drawing Initialization */
    // Prepares all parts of the grid to be drawn -- anything that does not need to be ran every frame
    void Grid::Init(GLuint shader)
    {
        RecalculateSquares();

        // Loops thru each square to prepare for rendering
        // Creates vertex array
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // glGenBuffers(number_of_buffers, pointer_to_buffer_to_edit)
        glGenBuffers(1, &vbo);

        // Selects buffer to edit
        // glBindBuffer(buffer_type, buffer)
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Give data
        // glBufferData(buffer_type, size_in_bytes, pointer_to_data, )
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW); // Draw specifies that it will be drawn

        // glVertexAttribPointer(index, size, type, normalized, stride, pointer)
        // Index = what index in the list refers to what (what the corresponding index actually means)
        // Size = how many of each per vertex
        // Type = type of data
        // Stride = amount of bytes between each vertex (size of each vertex in bytes)
        // Pointer = offset for piece of information in bytes (must be a pointer, can turn number into pointer by casting to (const void) *num)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0); // Called once per attribute

        // Index buffer
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);


        // Unbind intermediates
        glEnableVertexAttribArray(0);

        // Where each variable is located in the shader so they can be set
        posLoc = glGetUniformLocation(shader, "u_Position");
        scaleLoc = glGetUniformLocation(shader, "u_Scale");
        colorLoc = glGetUniformLocation(shader, "desiredColor");


        // Makes sure shader works correctly
        if (posLoc == -1 || scaleLoc == -1 || colorLoc == -1)
        {
            std::cout << "Warning: shader uniform not found." << std::endl;
        }
    }

    void Grid::Draw(GLuint shader)
    {
        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);

        // Loops through each rectangle in given rectangles
        for (const Square& s : boxes)
        {
            // Binds shader variables
            glUniform2f(posLoc, (s.pos[0]+s.pos[2])/2, (s.pos[1]+s.pos[3])/2);
            glUniform2f(scaleLoc, fabs(s.pos[2]-s.pos[0]), fabs(s.pos[3]-s.pos[1]));
            glUniform4f(colorLoc, s.color[0], s.color[1], s.color[2], s.color[3]);

            // Draws circle(aka triangle fan)
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        // Unbinds intermediates
        glBindVertexArray(0);
    }

    /* Grid Scrolling Callback */
    // Called when scrolling is detected over grid, updates values based on scroll input
    void Grid::Scroll(GLFWwindow *window, double xOffset, double yOffset)
    {
        // Get current mouse position
        double x, y;
        glfwGetCursorPos(window, &x, &y);

        // Get current window aspect
        int w, h;
        glfwGetWindowSize(window, &w, &h);

        // Normalize x and y values
        // Divide by width/height, multiply by two, and subtract one
        // Normalizes value to [0, 1600] to [-1, 1]
        x = (x/w)*2-1;
        y = -((y/h)*2-1);

        float newValue;

        // Keep track of current index
        size_t totalElements = (boxesCount) * (boxesCount);
        size_t i = 0;

        size_t r, c;

        // Loop through each square
        for(Square square : boxes)
        {
            // Get current row and column
            size_t r = i / (boxesCount); // Row index
            size_t c = i % (boxesCount); // Column index

            // Check if mouse is within square
            if(square.pos[0] <= x && x <= square.pos[2] && square.pos[3] <= y && y <= square.pos[1])
            {
                // Update value
                newValue = (*values)[r][c] - .1*yOffset;

                // Lock value to between -1 and 1 inclusive
                if(newValue < -1)
                    newValue = -1;
                else if(newValue > 1)
                    newValue = 1;

                // Update color to display
                if(newValue < 0)
                {
                    square.color = {-newValue, 0, 0, 1};
                }
                else if(newValue > 0)
                {
                    square.color = {0, newValue, 0, 1};
                }
                else
                {
                    square.color = {.5, .5, .5, 1};
                }

                // Update square & value
                boxes[i] = square;
                (*values)[r][c] = newValue;

                // End function, no point in continuing to look
                return;
            }

            i++;
        }
    }

    #pragma endregion

    // TextArea class functions
    #pragma region TextArea

    /* Full Text Area Constructor */
    // Creates a text area with all needed information
    TextArea::TextArea(float fontSize, unsigned int charactersPerLine, std::array<float, 2> center, std::string font, bool autoShrink, std::string text, bool startAtCenter):
        fontSize(fontSize), maxCharactersPerLine(charactersPerLine), center(center), font(font), autoShrink(autoShrink), text(text), face(NULL), startDrawingTextAtCenter(startAtCenter) {}

    /* Set Text Displayed */
    // Changes the text to be displayed
    void TextArea::SetText(std::string text)
    {
        this->text = text;
    }

    /* Return Text Displayed */
    // Returns the current text that is being displayed as an std::string
    std::string TextArea::GetText()
    {
        return text;
    }

    /* Font Size Changer */
    // Changes the current font size
    void TextArea::SetFontSize(int fontSize)
    {
        if(face != NULL)
        {
            this->fontSize = fontSize;
            FT_Set_Pixel_Sizes(face, 0, fontSize);
        }
        else
            this->fontSize = fontSize;
    }

    /* Font Loader */
    // Loads font based on information already provided
    void TextArea::LoadFont()
    {
        // Initialization of freetype
        auto error = FT_Init_FreeType( &library ); // Creates a new library

        if(error) // FT_Init_FreeType returns error code, or 0 if there was no error
        {
            std::cout << "ERROR: Freetype failed to initialize." << std::endl;
        }

        // Initialization of font
        error = FT_New_Face( library, // Library to be initialized under; FT_New_Face() initializes a new font("face")
                     "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", // Path to font
                     0, // What face to load
                     &face ); // What face to set it to

        // Error stuff for erroring
        if (error == FT_Err_Unknown_File_Format) // Format could not be read
        {
            std::cout << "ERROR: Font file format could not be read." << std::endl;
        }
        else if(error) // Some other kind of error
        {
            std::cout << "ERROR: Issue accessing font file, please verify its existence and accessability" << std::endl;
        }

        // Sets the font height in pixels
        FT_Set_Pixel_Sizes(face, 0, fontSize);  

        // Checks if glyphs can be loaded
        if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) // Loads glyph for X from face, FT_LOAD_RENDER creates an 8-bit gray fontSize bitmap for said glyph
        {
            std::cout << "ERROR: Freetype failed to load glyphs!" << std::endl;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Disables byte-alignment restriction (so bytes are not always multiples of 4)
  
        // Loops through each possible character in standard ASCII
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                // Load attempt did not work
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }

            // Generate texture
            unsigned int texture;

            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);

            // Creation of 2D textured image
            glTexImage2D(
                GL_TEXTURE_2D, // Type of texture to load
                0, // Level of detail(0 is default)
                GL_RED, // Number of color components in texture
                face->glyph->bitmap.width, // Width
                face->glyph->bitmap.rows, // Height
                0, // Must be 0- (allegedly)
                GL_RED, // Format of pixel data
                GL_UNSIGNED_BYTE, // Type of pixel data
                face->glyph->bitmap.buffer // Pointer to where image data is
            );

            // Sets texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // Stores each character as a struct of all of the generated information
            Character character = {
                texture, 
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                face->glyph->advance.x
            };

            Characters.insert(std::pair<char, Character>(c, character));
        }

        // Clear intermediates
        FT_Done_Face(face);
        FT_Done_FreeType(library);
    }

    /* Text Area Drawing Initialization */
    // Initializes text area with all things necessary so it is ready to be drawn
    void TextArea::Init(GLuint shader)
    {
        LoadFont();

        glm::mat4 projection = glm::ortho(0.0f, 1600.0f, 0.0f, 900.0f);

        glUseProgram(shader);
        glUniformMatrix4fv(
            glGetUniformLocation(shader, "projection"),
            1,
            GL_FALSE,
            &projection[0][0]
        );

        // Allows for colors to render properly
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  

        // Generates vertex array/object buffer
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        // Binds to state machine
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Modifies them to draw texts and work with text.shader
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

        // Unbinds intermediates
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);      
    }

    /* Text Drawing */
    // Draws text based on all previously given information
    void TextArea::Draw(GLuint shader)
    {
        glUseProgram(shader);

        // Loads vertex array buffer
        glUniform3f(glGetUniformLocation(shader, "textColor"), color[0], color[1], color[2]);
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(vao);

        // Computation of length of text(so we can find center)
        float width = 0.0f;
        for (char c : text)
        {
            Character ch = Characters[c];
            width += (ch.advance >> 6);
        }

        // Converts normalized device coords [-1,1] into screen pixels (so units are constant, as all other elements use normalized values)
        float screenX = (center[0] + 1.0f) * 0.5f * 1600.0f;
        float screenY = (center[1] + 1.0f) * 0.5f * 900.0f;

        // Center text (if needed)
        float x;
        if(startDrawingTextAtCenter)
            x = screenX - width * 0.5f;
        else
            x =  center[0];

        float y = screenY;

        // Goes through each character in the text
        std::string::const_iterator c;

        for (c = text.begin(); c != text.end(); c++)
        {
            // Loads the information for how to draw that character
            Character ch = Characters[*c];

            // Where to draw it
            float xpos = x + ch.bearing.x;
            float ypos = y - (ch.size.y - ch.bearing.y); // Sometimes parts of glyph may need to be below line(Ex. y, g)

            // Width / height calculation
            float w = ch.size.x;
            float h = ch.size.y;

            // Updates VBO for each character
            float vertices[6][4] = {
                { xpos,     ypos + h,   0.0f, 0.0f },            
                { xpos,     ypos,       0.0f, 1.0f },
                { xpos + w, ypos,       1.0f, 1.0f },

                { xpos,     ypos + h,   0.0f, 0.0f },
                { xpos + w, ypos,       1.0f, 1.0f },
                { xpos + w, ypos + h,   1.0f, 0.0f }           
            };

            // Renders glyph texture over quad
            glBindTexture(GL_TEXTURE_2D, ch.textureID);

            // Updates content of VBO memory
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // Renders actual quad
            glDrawArrays(GL_TRIANGLES, 0, 6);

            // Advances drawing spot for next glyph
            x += (ch.advance >> 6); // bitshift by 6 to get value in pixels (2^6 = 64)
        }

        // Unbinds intermediates
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
        

    #pragma endregion
}