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

        // Draw with corners
        glUniform1i(glGetUniformLocation(shader, "u_Shape"), 0);
        glUniform1f(glGetUniformLocation(shader, "u_CornerRadius"), 0.0f);

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
    void World::DisplaySelection(GLuint shader, const body::Body *bodies, size_t count)
    {
        // If nothing is selected, don't display anything
        if (selected.empty())
            return;

        // Bind shader
        glUseProgram(shader);
        glBindVertexArray(vao);

        // How many stuff to loop for
        const size_t selectionCount = std::min(selected.size(), selectedMarkerSizes.size());

        // Loop through each selected index
        for (size_t i = 0; i < selectionCount; ++i)
        {
            int idx = selected[i];
            if (idx < 0 || static_cast<size_t>(idx) >= count) // out-of-range selection
                continue;

            const auto &position = bodies[idx].GetPosition();

            // How big each box should be
            const float markerRadius = std::max(0.001f, selectedMarkerSizes[i]);
            const float boxSize = markerRadius * 2.0f;
            const float boxWidth = aspect > 0.0f ? boxSize / aspect : boxSize;

            // Draw radial borders
            glUniform1i(glGetUniformLocation(shader, "u_Shape"), 0);
            glUniform1f(glGetUniformLocation(shader, "u_CornerRadius"), 0.0f);

            // Bind stuff to draw
            glUniform2f(posLoc, position[0], position[1]);
            glUniform2f(scaleLoc, boxWidth, boxSize);
            glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

            // Draw square
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        }

        glBindVertexArray(0);
    }

    // Renders selection boxes using SoA position buffers to avoid syncing AoS
    void World::DisplaySelectionSoA(GLuint shader, const std::vector<float> &posX, const std::vector<float> &posY, size_t count)
    {
        if (selected.empty()) return;
        glUseProgram(shader);
        glBindVertexArray(vao);

        const size_t selectionCount = std::min(selected.size(), selectedMarkerSizes.size());
        for (size_t i = 0; i < selectionCount; ++i)
        {
            int idx = selected[i];
            if (idx < 0 || static_cast<size_t>(idx) >= count) continue;

            float px = posX[idx];
            float py = posY[idx];

            const float markerRadius = std::max(0.001f, selectedMarkerSizes[i]);
            const float boxSize = markerRadius * 2.0f;
            const float boxWidth = aspect > 0.0f ? boxSize / aspect : boxSize;

            glUniform1i(glGetUniformLocation(shader, "u_Shape"), 0);
            glUniform1f(glGetUniformLocation(shader, "u_CornerRadius"), 0.0f);
            glUniform2f(posLoc, px, py);
            glUniform2f(scaleLoc, boxWidth, boxSize);
            glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);
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

        glUniform1i(glGetUniformLocation(shader, "u_Shape"), 0);
        glUniform1f(glGetUniformLocation(shader, "u_CornerRadius"), 0.0f);

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

    /* Slider Creation */
    // Creates a new slider under the panel
    element Panel::AddSlider(float xOffset, float yCenter, float length, float totalHeight, float defaultValue)
    {
        // Create new slider
        Slider slider(xOffset, yCenter, length, totalHeight, defaultValue);

        // Update it's panel center
        if(side == -1)
            slider.panelCenter = -1 + this->length / 2.0f;
        else
            slider.panelCenter = 1 - this->length / 2.0f;

            // Tell it to calculate where it should be
        slider.RecalculatePosition();

        // Add to elements
        elements.push_back(ElementHandle{std::make_unique<Slider>(std::move(slider)), element{nextId}, true});

        // Update Ids
        element assignedId = elements.back().id;
        nextId++;

        if(nextId == 0)
            std::cout << "ERROR: To many elements have been created under panel!" << std::endl;

        return assignedId;
    }

    /* Button Creation */
    // Creates a new button under the panel
    element Panel::AddButton(float xOffset, float yCenter, float width, float height, std::array<float, 4> color, GLuint textShader, std::string text, std::string font, int fontSize, bool autoShrink)
    {
        // Create new button
        Button button(xOffset, yCenter, width, height, color, textShader, text, font, fontSize, autoShrink);

        // Update it's panel center
        if(side == -1)
            button.panelCenter = -1 + this->length / 2.0f;
        else
            button.panelCenter = 1 - this->length / 2.0f;

        // Tell it to calculate where it should be
        button.RecalculatePosition();

        // Add to elements
        elements.push_back(ElementHandle{std::make_unique<Button>(std::move(button)), element{nextId}, true});

        // Update Ids
        element assignedId = elements.back().id;
        nextId++;

        if(nextId == 0)
            std::cout << "ERROR: To many elements have been created under panel!" << std::endl;

        return assignedId;
    }

    /* Link Elements */
    // Link two elements so they move and scale together anchored on their median
    void Panel::LinkElements(element first, element second)
    {
        // Saves a lambda expression for finding values by id
        auto findIndex = [&](element id) -> int {
            for (int i = 0; i < static_cast<int>(elements.size()); ++i)
            {
                if (elements[i].id == id)
                    return i;
            }
            return -1;
        };

        // Finds elements by id
        int firstIndex = findIndex(first);
        int secondIndex = findIndex(second);

        // Makes sure values aren't invalid
        if (firstIndex < 0 || secondIndex < 0 || firstIndex == secondIndex)
            return;

        // Get pointers to each element
        Element *firstPtr = elements[firstIndex].ptr.get();
        Element *secondPtr = elements[secondIndex].ptr.get();

        // Get the center of each element
        auto getElementCenter = [&](Element *elem) -> float
        {
            // If it is text, use it's center instead of manually calculating it
            if (auto *textArea = dynamic_cast<TextArea*>(elem))
                return textArea->center[0];
            return (elem->pos[0] + elem->pos[2]) * 0.5f;
        };

        // Calculate the current centers of the two objects together and the center of the panel
        float firstCenter = getElementCenter(firstPtr);
        float secondCenter = getElementCenter(secondPtr);
        float median = (firstCenter + secondCenter) * 0.5f;
        float panelCenter = (side == -1) ? -1.0f + length / 2.0f : 1.0f - length / 2.0f;

        // Calculate the relative center of the objects
        const float firstOffset = firstCenter - median;
        const float secondOffset = secondCenter - median;
        const float medianOffsetFromPanelCenter = median - panelCenter;

        // Save element pair
        linkedGroups.push_back(LinkedGroup{first, second, medianOffsetFromPanelCenter, firstOffset, secondOffset});
    }

    /* Panel Graphics Initialization */
    // Loads all necessary info to draw panel and elements
    void Panel::Init(GLuint uiShader, GLuint textShader, GLFWwindow *window)
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
        glUseProgram(uiShader);
        glBindVertexArray(vao);

        posLoc = glGetUniformLocation(uiShader, "u_Position");
        scaleLoc = glGetUniformLocation(uiShader, "u_Scale");
        colorLoc = glGetUniformLocation(uiShader, "desiredColor");

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
            {
                // Assign shader to each element based on it's type -> what kind it needs
                if (auto *textArea = dynamic_cast<TextArea*>(e.ptr.get()))
                    e.ptr->Init(textShader);
                else if (auto *button = dynamic_cast<Button*>(e.ptr.get()))
                {
                    button->textShader = textShader;
                    button->Init(uiShader);
                }
                else
                    e.ptr->Init(uiShader);
            }
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
    void Panel::Draw(GLuint uiShader, GLuint textShader)
    {
        // What shader and vertex array OpenGL should use to render
        glUseProgram(uiShader);
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

        glUniform1i(glGetUniformLocation(uiShader, "u_Shape"), 0);
        glUniform1f(glGetUniformLocation(uiShader, "u_CornerRadius"), 0.0f);

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
            {
                // Runs element's Draw() function (actual Element class's is overriden)
                // Check element's type -> use the right shader
                if(auto *textArea = dynamic_cast<TextArea*>(e.ptr.get()))
                    e.ptr->Draw(textShader); // Runs element's draw function
                else
                    e.ptr->Draw(uiShader);
            }
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
                // Make sure scroll occurred somewhere on the panel(x/w)*2-1 or subpanel
                else if(((panel->side == -1 && x < (-1+panel->length)) || (panel->side == 1 && x > (1-panel->length))) || SubPanel::AnyActivePanelContains(static_cast<float>(x), static_cast<float>(y))) // Only continue if click area was within UI
                {
                    Events::leftMouseDownStartedOnPanel = true;
                }
                // Is in the world
                else
                {
                    // Disable the blue area-selection overlay only when a new left-click world selection starts
                    Events::rightSelectionActive = false;
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
                if(((panel->side == -1 && x < (-1+panel->length)) || (panel->side == 1 && x > (1-panel->length))) || SubPanel::AnyActivePanelContains(static_cast<float>(x), static_cast<float>(y)))
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

                // Update events
                if (!Events::rightMouseDownStartedOnPanel)
                {
                    const double dragX = Events::rightSelectionCurrentPos[0] - Events::rightSelectionStartPos[0];
                    const double dragY = Events::rightSelectionCurrentPos[1] - Events::rightSelectionStartPos[1];
                    const bool dragSelection = std::hypot(dragX, dragY) > 0.001;

                    if (!dragSelection)
                        Events::rightSelectionActive = false;
                }
            }
        }

    }

    /* Panel Text Update */
    // Updates the text displayed by a text area
    void Panel::SetText(element textElement, std::string text)
    {
        // Loop through each element
        for (auto &entry : elements)
        {
            // Find the one with the right id
            if (entry.id == textElement.id)
            {
                // If it is a text element, update its text
                if (auto *textArea = dynamic_cast<TextArea*>(entry.ptr.get()))
                    textArea->SetText(text);

                return;
            }
        }
    }

    /* Grid Box Count Update */
    // Updates the visible size of a grid element
    void Panel::SetGridBoxCount(element grid, unsigned int count)
    {
        for (auto &entry : elements)
        {
            if (entry.id == grid.id)
            {
                if (auto *gridElement = dynamic_cast<Grid*>(entry.ptr.get()))
                {
                    gridElement->boxesCount = static_cast<int>(std::max<unsigned int>(1u, count));
                    gridElement->RecalculateSquares();
                }

                return;
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
            //  If element is slider, update slider
            else if (auto *slider = dynamic_cast<Slider*>(element.ptr.get()))
            {
                // Value calculation
                const float desiredCenter = (side == -1 ? -1.0f + length / 2.0f : 1.0f - length / 2.0f) + slider->xOffset;
                const float halfWidth = slider->length * 0.5f;
                const float minCenter = panelStart + halfWidth;
                const float maxCenter = panelEnd - halfWidth;
                const float clampedCenter = std::max(minCenter, std::min(maxCenter, desiredCenter));

                // Update center
                slider->panelCenter = clampedCenter - slider->xOffset;

                // Recalculate slider positioning
                slider->RecalculatePosition();
            }
            // If element is a button, update using its panel-relative center and width
            else if (auto *button = dynamic_cast<Button*>(element.ptr.get()))
            {
                // Calculate where it's center should be
                const float desiredCenter = (side == -1 ? -1.0f + length / 2.0f : 1.0f - length / 2.0f) + button->xOffset;
                const float halfWidth = button->width * 0.5f;
                const float minCenter = panelStart + halfWidth;
                const float maxCenter = panelEnd - halfWidth;
                const float clampedCenter = std::max(minCenter, std::min(maxCenter, desiredCenter));

                // Update it's center
                button->panelCenter = clampedCenter - button->xOffset;
                button->RecalculatePosition();
            }
            // If element is a text area, update its center now that pos has moved
            else if (auto *textArea = dynamic_cast<TextArea*>(element.ptr.get()))
            {
                textArea->RecalculatePosition();
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

                // Update it's center
                textArea->center = {(element.ptr->pos[0] + element.ptr->pos[2]) * 0.5f,
                                    (element.ptr->pos[1] + element.ptr->pos[3]) * 0.5f};
                textArea->RecalculatePosition();
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

        // Move linked groups together around their shared median
        const float newPanelCenter = (side == -1) ? -1.0f + length / 2.0f : 1.0f - length / 2.0f;

        for (auto &group : linkedGroups)
        {
            const float median = newPanelCenter + group.medianOffsetFromPanelCenter;

            for (const auto &pair : {std::pair<element, float>{group.first, group.firstOffsetFromMedian}, std::pair<element, float>{group.second, group.secondOffsetFromMedian}})
            {
                int index = -1;
                for (int i = 0; i < static_cast<int>(elements.size()); ++i)
                {
                    if (elements[i].id == pair.first)
                    {
                        index = i;
                        break;
                    }
                }

                if (index < 0)
                    continue;

                Element *linkedElement = elements[index].ptr.get();
                float width = linkedElement->pos[2] - linkedElement->pos[0];
                float center = median + pair.second;
                float left = center - width / 2.0f;
                float right = center + width / 2.0f;

                linkedElement->pos[0] = left;
                linkedElement->pos[2] = right;

                if (auto *grid = dynamic_cast<Grid*>(linkedElement))
                {
                    grid->panelCenter = center - grid->xOffset;
                    grid->RecalculateSquares();
                }
                else if (auto *slider = dynamic_cast<Slider*>(linkedElement))
                {
                    slider->panelCenter = center - slider->xOffset;
                    slider->RecalculatePosition();
                }
                else if (auto *textArea = dynamic_cast<TextArea*>(linkedElement))
                {
                    textArea->center = {center, (textArea->pos[1] + textArea->pos[3]) * 0.5f};
                    textArea->RecalculatePosition();
                }
                else if (auto *textArea = dynamic_cast<TextArea*>(linkedElement))
                {
                    textArea->center = {center,
                                        (textArea->pos[1] + textArea->pos[3]) * 0.5f};
                    textArea->RecalculatePosition();
                }
            }
        }

    }

    /* Panel Update Function */
    // Updates everything related to the panel
    void Panel::Update(GLuint uiShader, GLuint textShader, GLFWwindow *window)
    {
        UpdateCursor(window);
 
        for (auto &e : elements)
        {
            if (e.active)
                e.ptr->HandleInput(window);
        }

        // If right mouse is down and didn't start on panel -> do right mouse drag
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

        Draw(uiShader, textShader);

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
                world.LeftDrag(window, uiShader);
        
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

    /* Slider Value Retrieving */
    // Called by user; finds given slider and return's its normalized value
    float Panel::GetSliderValue(element slider)
    {
        // Loop through each element attached to panel
        for(int i = 0; i < elements.size(); i++)
        {
            // Check if element is right one
            if(elements[i].id == slider.id)
            {
                // Ensure element is a slider
                if(auto *sliderPtr = dynamic_cast<Slider*>(elements[i].ptr.get()))
                    return sliderPtr->normalizedValue;
                else
                    return -1;
            }
        }

        return -1;
    }

    bool Panel::IsButtonDown(element button)
    {
        // Loop through each element attached to panel
        for(int i = 0; i < elements.size(); i++)
        {
            // Check if element is right one
            if(elements[i].id == button.id)
            {
                // Ensure element is a button
                if(auto *buttonPtr = dynamic_cast<Button*>(elements[i].ptr.get()))
                    return buttonPtr->pressed;
            }
        }

        return false;
    }

    void Panel::ResetButton(element button)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == button.id)
            {
                if(auto *buttonPtr = dynamic_cast<Button*>(elements[i].ptr.get()))
                    buttonPtr->pressed = false;
                return;
            }
        }
    }

    void Panel::SetElementActive(element elementId, bool active)
    {
        for (auto &entry : elements)
        {
            if (entry.id == elementId)
            {
                entry.active = active;
                return;
            }
        }
    }

    int Panel::GetDropdownSelectedIndex(element dropdown)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == dropdown.id)
            {
                if(auto *dropdownPtr = dynamic_cast<DropdownButton*>(elements[i].ptr.get()))
                    return dropdownPtr->GetSelectedIndex();
            }
        }

        return -1;
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

    /* Grid Drawing */
    // Draws every square in the grid and colors them based on their values
    void Grid::Draw(GLuint shader)
    {
        RecalculateSquares();

        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);

        glUniform1i(glGetUniformLocation(shader, "u_Shape"), 0);
        glUniform1f(glGetUniformLocation(shader, "u_CornerRadius"), 0.0f);

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
        Element({0.0f, 0.0f, 0.0f, 0.0f}),
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
        auto error = FT_Init_FreeType(&library); // Creates a new library

        if (error) // FT_Init_FreeType returns error code, or 0 if there was no error
        {
            std::cout << "ERROR: Freetype failed to initialize." << std::endl;
            return;
        }

        // Initialization of font
        error = FT_New_Face(
            library,
            font.c_str(),
            0,
            &face
        );

        // Error stuff for erroring
        if (error == FT_Err_Unknown_File_Format) // Format could not be read
        {
            std::cout << "ERROR: Font file format could not be read." << std::endl;
            FT_Done_FreeType(library);
            return;
        }
        else if (error) // Some other kind of error
        {
            std::cout << "ERROR: Issue accessing font file, please verify its existence and accessability" << std::endl;
            FT_Done_FreeType(library);
            return;
        }

        // Sets the font height in pixels
        FT_Set_Pixel_Sizes(face, 0, fontSize);

        // Checks if glyphs can be loaded
        if (FT_Load_Char(face, 'X', FT_LOAD_RENDER)) // Loads glyph for X from face, FT_LOAD_RENDER creates an 8-bit gray fontSize bitmap for said glyph
        {
            std::cout << "ERROR: Freetype failed to load glyphs!" << std::endl;
            FT_Done_Face(face);
            FT_Done_FreeType(library);
            return;
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
        RecalculatePosition();

        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(viewport[2]), 0.0f, static_cast<float>(viewport[3]));

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

    /* Position Recalculation */
    // Updates the Element position for the TextArea based on panel position and positions assigned to it
    void TextArea::RecalculatePosition()
    {
        // How big text field is
        float width = 0.0f;
        for (char c : text)
        {
            Character ch = Characters[c];
            width += (ch.advance >> 6);
        }

        // How tall text field is
        float height = static_cast<float>(fontSize);

        // Get aspect ratio from viewport
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const float viewportWidth = static_cast<float>(viewport[2]);
        const float viewportHeight = static_cast<float>(viewport[3]);

        // Calculate center
        float x0 = center[0] - (startDrawingTextAtCenter ? (width * 0.5f) / viewportWidth * 2.0f : 0.0f);
        float x1 = center[0] + (width * 0.5f) / viewportWidth * 2.0f;
        float y0 = center[1] - (height * 0.5f) / viewportHeight * 2.0f;
        float y1 = center[1] + (height * 0.5f) / viewportHeight * 2.0f;

        // Update Element center
        pos = {x0, y0, x1, y1};
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
        float ascent = 0.0f;
        float descent = 0.0f;

        for (char c : text)
        {
            Character ch = Characters[c];
            width += (ch.advance >> 6);
            ascent = std::max(ascent, static_cast<float>(ch.bearing.y));
            descent = std::max(descent, static_cast<float>(ch.size.y - ch.bearing.y));
        }

        // Converts normalized device coords [-1,1] into screen pixels (so units are constant, as all other elements use normalized values)
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const float viewportWidth = static_cast<float>(viewport[2]);
        const float viewportHeight = static_cast<float>(viewport[3]);

        float screenX = (center[0] + 1.0f) * 0.5f * viewportWidth;
        float screenY = (center[1] + 1.0f) * 0.5f * viewportHeight;

        // Center text (if needed)
        float x;
        if (startDrawingTextAtCenter)
            x = screenX - width * 0.5f;
        else
            x = screenX;

        float y = screenY - (ascent - descent) * 0.5f;

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

    /* Update Center */
    // Updates the text rendering center position
    void TextArea::SetCenter(std::array<float, 2> center)
    {
        this->center = center;
        RecalculatePosition();
    }

    #pragma endregion

    // Slider class functions
    #pragma region Slider

    Slider* Slider::activeSlider = nullptr;

    /* Default Slider Constructor */
    // Just creates a slider, nothin special about it...
    Slider::Slider(): normalizedValue(0.5f), isDragging(false), length(0.0f), height(0.0f), xOffset(0.0f), yCenter(0.0f), panelCenter(0.0f) {}

    Slider::Slider(float xOffset, float yCenter, float length, float totalHeight, float defaultValue):
        xOffset(xOffset), yCenter(yCenter), length(length), height(totalHeight), normalizedValue(std::clamp(defaultValue, 0.0f, 1.0f)), isDragging(false), panelCenter(0.0f) {}

    /* Recenter Slider */
    // Recalculate the stored center of the slider
    void Slider::RecalculatePosition()
    {
        const float x0 = panelCenter + xOffset - length / 2.0f;
        const float x1 = panelCenter + xOffset + length / 2.0f;
        const float y0 = yCenter - height / 2.0f;
        const float y1 = yCenter + height / 2.0f;

        this->pos = {x0, y0, x1, y1};
    }


    /* Slider Drawing Initialization */
    // Prepare buffers for slider to be drawn
    void Slider::Init(GLuint shader)
    {
        // Start by calculating the center
        RecalculatePosition();

        // Bind buffers
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Prepare buffers for drawing
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW);

        // Point to what is needed where
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

        // Prepare buffers for drawing
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

        // Unbind intermediates
        glEnableVertexAttribArray(0);

        // Find location of needed inputs
        glUseProgram(shader);
        glBindVertexArray(vao);

        posLoc = glGetUniformLocation(shader, "u_Position");
        scaleLoc = glGetUniformLocation(shader, "u_Scale");
        colorLoc = glGetUniformLocation(shader, "desiredColor");
        shapeLoc = glGetUniformLocation(shader, "u_Shape");
        radiusLoc = glGetUniformLocation(shader, "u_CornerRadius");

        if (posLoc == -1 || scaleLoc == -1 || colorLoc == -1)
            std::cout << "Warning: slider shader uniform not found." << std::endl;

        // Unbind intermediates
        glBindVertexArray(0);
    }

    /* Slider Drawing */
    // Draws the slider base and handle based on current normalized value
    void Slider::Draw(GLuint shader)
    {
        // Bind the slider's shader and vertex data
        glUseProgram(shader);
        glBindVertexArray(vao);

        // Get viewport dimensions to keep the slider shape correct on different aspect ratios
        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const float aspect = static_cast<float>(viewport[2]) / std::max(1, viewport[3]);

        // Calculate the slider's track and handle sizes
        const float barRadius = std::min(height * 0.5f, 0.02f);
        const float knobRadius = std::min(height * 0.6f, std::max(0.01f, (pos[2] - pos[0]) * 0.5f));
        const float trackWidth = std::max(0.0f, pos[2] - pos[0]);
        const float effectiveMinX = pos[0] + knobRadius;
        const float effectiveMaxX = pos[2] - knobRadius;
        const float knobCenterX = std::clamp(pos[0] + trackWidth * normalizedValue, effectiveMinX, effectiveMaxX);
        const float knobCenterY = (pos[1] + pos[3]) * 0.5f;

        // Draw the rounded track of the slider
        glUniform1i(shapeLoc, 1);
        glUniform1f(radiusLoc, barRadius);
        glUniform1f(glGetUniformLocation(shader, "u_Aspect"), aspect);
        glUniform2f(posLoc, (pos[0] + pos[2]) * 0.5f, (pos[1] + pos[3]) * 0.5f);
        glUniform2f(scaleLoc, pos[2] - pos[0], pos[3] - pos[1]);
        glUniform4f(colorLoc, 0.22f, 0.24f, 0.28f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Draw the handle at the current normalized value as a rectangle
        glUniform1i(shapeLoc, 0);
        glUniform1f(radiusLoc, 0.0f);
        glUniform1f(glGetUniformLocation(shader, "u_Aspect"), aspect);
        glUniform2f(posLoc, knobCenterX, knobCenterY);
        glUniform2f(scaleLoc, knobRadius, knobRadius);
        glUniform4f(colorLoc, 0.92f, 0.92f, 0.92f, 1.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Unbind intermediates
        glBindVertexArray(0);
    }

    /* Input Response */
    // Updates visual and value based on new position, decided from mouse position
    void Slider::HandleInput(GLFWwindow *window)
    {
        // Get cursor position
        double cursorX, cursorY;
        glfwGetCursorPos(window, &cursorX, &cursorY);

        // Get window aspect
        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        // Recalculate position to account for window aspect ratio
        const float x = static_cast<float>((cursorX / windowWidth) * 2.0 - 1.0);
        const float y = static_cast<float>(-((cursorY / windowHeight) * 2.0 - 1.0));

        // See if cursor is within range
        const bool hovered = (x >= pos[0] && x <= pos[2] && y >= pos[1] && y <= pos[3]);

        // Left mouse is down
        if (Events::leftMouseDown)
        {
            // Update active slider from Events, and update isDragging from Events
            if (activeSlider == nullptr && hovered)
            {
                activeSlider = this;
                isDragging = true;
            }
            else if (activeSlider == this)
            {
                isDragging = true;
            }
            else
            {
                isDragging = false;
            }

            if (isDragging)
            {
                // Calculate and update knob positioning
                const float trackWidth = std::max(0.0f, pos[2] - pos[0]);
                const float knobRadius = std::min(height * 0.6f, std::max(0.01f, trackWidth * 0.5f));
                const float effectiveMinX = pos[0] + knobRadius;
                const float effectiveMaxX = pos[2] - knobRadius;
                const float clampedX = std::clamp(x, effectiveMinX, effectiveMaxX);
                const float availableWidth = std::max(1e-4f, trackWidth - (knobRadius * 2.0f));
                normalizedValue = (clampedX - pos[0] - knobRadius) / availableWidth;
                normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
            }
        }
        else
        {
            // Reset active slider and isDragging
            if (activeSlider == this)
                activeSlider = nullptr;
            isDragging = false;
        }
    }

    #pragma endregion

    // Button class functions
    # pragma region Button

    /* Default Button Constructor */
    // Initializes everything necessary for a button
    Button::Button():
        pressed(false), candidate(false), width(0.0f), height(0.0f), xOffset(0.0f), yCenter(0.0f), panelCenter(0.0f), Element() {}

    /* Full Button Constructor */
    // Creates a new button with all the information needed to draw it
    Button::Button(float xOffset, float yCenter, float width, float height, std::array<float, 4> color, GLuint textShader, std::string text, std::string font, int fontSize, bool autoShrink):
        pressed(false), candidate(false), width(width), height(height), xOffset(xOffset), yCenter(yCenter), panelCenter(0.0f), Element(std::array<float, 4>{0, 0, 0, 0}, color),
        textShader(textShader), textOverlay(nullptr), textFont(std::move(font)), textFontSize(fontSize), textAutoShrink(autoShrink), buttonText(std::move(text)) {}

    /* Button Initialization */
    // Initializes shaders for the button to render
    void Button::Init(GLuint shader)
    {
        if (vao == 0)
            glGenVertexArrays(1, &vao);
        if (vbo == 0)
            glGenBuffers(1, &vbo);
        if (ebo == 0)
            glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

        glUseProgram(shader);
        glBindVertexArray(vao);
        posLoc = glGetUniformLocation(shader, "u_Position");
        scaleLoc = glGetUniformLocation(shader, "u_Scale");
        colorLoc = glGetUniformLocation(shader, "desiredColor");
        shapeLoc = glGetUniformLocation(shader, "u_Shape");
        radiusLoc = glGetUniformLocation(shader, "u_CornerRadius");
        glBindVertexArray(0);

        InitTextOverlay(shader);
        RecalculatePosition();
    }

    /* Button Drawing */
    // Draws button from position
    void Button::Draw(GLuint shader)
    {
        glUseProgram(shader);
        glBindVertexArray(vao);

        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const float aspect = static_cast<float>(viewport[2]) / std::max(1, viewport[3]);

        glUniform1i(shapeLoc, 1);
        glUniform1f(radiusLoc, std::min(width, height) * 0.18f);
        glUniform1f(glGetUniformLocation(shader, "u_Aspect"), aspect);
        glUniform2f(posLoc, (pos[0] + pos[2]) * 0.5f, (pos[1] + pos[3]) * 0.5f);
        glUniform2f(scaleLoc, pos[2] - pos[0], pos[3] - pos[1]);

        if (candidate)
            glUniform4f(colorLoc, color[0] * 0.8f, color[1] * 0.8f, color[2] * 0.8f, color[3]);
        else
            glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        glBindVertexArray(0);

        if (textOverlay)
            textOverlay->Draw(textShader);
    }

    /* Button Input Handling */
    // Updates hover/press state for the button based on the current cursor and mouse state
    void Button::HandleInput(GLFWwindow *window)
    {
        double cursorX, cursorY;
        glfwGetCursorPos(window, &cursorX, &cursorY);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        const float x = static_cast<float>((cursorX / windowWidth) * 2.0 - 1.0);
        const float y = static_cast<float>(-((cursorY / windowHeight) * 2.0 - 1.0));

        const bool hovered = (x >= pos[0] && x <= pos[2] && y >= pos[1] && y <= pos[3]);
        const bool wasCandidate = candidate;

        if (hovered && Events::leftMouseDown)
            candidate = true;
        else
            candidate = false;

        if (!Events::leftMouseDown && hovered && wasCandidate && !pressed)
            pressed = true;
    }

    /* Button Text Overlay Initialization */
    // Creates and positions the label shown on top of the button
    void Button::InitTextOverlay(GLuint shader)
    {
        if (!textOverlay)
        {
            std::array<float, 2> overlayCenter{0.0f, 0.0f};
            textOverlay = std::make_unique<TextArea>(textFontSize, 64u, overlayCenter, textFont, textAutoShrink, buttonText, true);
            textOverlay->Init(textShader);
        }

        textOverlay->font = textFont;
        textOverlay->fontSize = textFontSize;
        textOverlay->autoShrink = textAutoShrink;
        textOverlay->SetText(buttonText);
        textOverlay->RecalculatePosition();
    }

    /* Button Text Update */
    // Updates the displayed label text for the button
    void Button::SetText(const std::string &text)
    {
        buttonText = text;
        if (textOverlay)
        {
            textOverlay->SetText(text);
            textOverlay->RecalculatePosition();
        }
    }

    /* Button Position Calculation */
    // Recalculates the global position of the button from the relative position
    void Button::RecalculatePosition()
    {
        const float x0 = panelCenter + xOffset - width / 2.0f;
        const float x1 = panelCenter + xOffset + width / 2.0f;
        const float y0 = yCenter - height / 2.0f;
        const float y1 = yCenter + height / 2.0f;
        pos = {x0, y0, x1, y1};

        if (textOverlay)
        {
            const float centerX = (x0 + x1) * 0.5f;
            const float centerY = (y0 + y1) * 0.5f;
            textOverlay->SetCenter({centerX, centerY});
        }
    }

    #pragma endregion

    // DropdownButton class functions
    #pragma region DropdownButton

    /* Default Dropdown Constructor */
    // Initializes everything necessary for an empty dropdown
    DropdownButton::DropdownButton():
        selectedIndex(0), candidateIndex(-1), open(false), mainCandidate(false), mouseWasDown(false), width(0.0f), height(0.0f), xOffset(0.0f), yCenter(0.0f), panelCenter(0.0f), Element() {}

    /* Full Dropdown Constructor */
    // Creates a new dropdown with all the information needed to draw it
    DropdownButton::DropdownButton(float xOffset, float yCenter, float width, float height, std::vector<std::string> labels, std::vector<std::array<float, 4>> optionColors, GLuint textShader, std::string font, int fontSize, bool autoShrink):
        labels(std::move(labels)), optionColors(std::move(optionColors)), selectedIndex(0), candidateIndex(-1), open(false), mainCandidate(false), mouseWasDown(false), width(width), height(height), xOffset(xOffset), yCenter(yCenter), panelCenter(0.0f),
        Element(std::array<float, 4>{0, 0, 0, 0}, std::array<float, 4>{0.7f, 0.7f, 0.7f, 0.9f}), textShader(textShader), textFont(std::move(font)), textFontSize(fontSize), textAutoShrink(autoShrink)
    {
        // If no labels were given, add a fallback option so drawing and selection always have a valid index
        if (this->labels.empty())
            this->labels.push_back("None");

        // Colors are optional, but each visible row needs one color value for its swatch/background
        while (this->optionColors.size() < this->labels.size())
            this->optionColors.push_back({0.7f, 0.7f, 0.7f, 0.9f});
    }

    /* Dropdown Initialization */
    // Initializes shaders for the dropdown to render
    void DropdownButton::Init(GLuint shader)
    {
        if (vao == 0)
            glGenVertexArrays(1, &vao);
        if (vbo == 0)
            glGenBuffers(1, &vbo);
        if (ebo == 0)
            glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

        glUseProgram(shader);
        glBindVertexArray(vao);
        posLoc = glGetUniformLocation(shader, "u_Position");
        scaleLoc = glGetUniformLocation(shader, "u_Scale");
        colorLoc = glGetUniformLocation(shader, "desiredColor");
        shapeLoc = glGetUniformLocation(shader, "u_Shape");
        radiusLoc = glGetUniformLocation(shader, "u_CornerRadius");
        glBindVertexArray(0);

        InitTextOverlays();
        RecalculatePosition();
    }

    /* Dropdown Text Initialization */
    // Creates and positions all labels shown by the dropdown
    void DropdownButton::InitTextOverlays()
    {
        // The selected text overlay is the label drawn on the closed dropdown button
        if (!selectedTextOverlay)
        {
            std::array<float, 2> overlayCenter{0.0f, 0.0f};
            selectedTextOverlay = std::make_unique<TextArea>(textFontSize, 64u, overlayCenter, textFont, textAutoShrink, labels[selectedIndex], true);
            selectedTextOverlay->Init(textShader);
        }

        // The option overlays are separate because the open menu may draw the selected option again in a different row
        while (optionTextOverlays.size() < labels.size())
        {
            std::array<float, 2> overlayCenter{0.0f, 0.0f};
            auto optionText = std::make_unique<TextArea>(textFontSize, 64u, overlayCenter, textFont, textAutoShrink, labels[optionTextOverlays.size()], true);
            optionText->Init(textShader);
            optionTextOverlays.push_back(std::move(optionText));
        }
    }

    /* Dropdown Position Calculation */
    // Recalculates the global position of the dropdown from the relative position
    void DropdownButton::RecalculatePosition()
    {
        const float x0 = panelCenter + xOffset - width / 2.0f;
        const float x1 = panelCenter + xOffset + width / 2.0f;
        const float y0 = yCenter - height / 2.0f;
        const float y1 = yCenter + height / 2.0f;
        pos = {x0, y0, x1, y1};

        if (selectedTextOverlay)
        {
            selectedTextOverlay->SetText(labels[selectedIndex]);
            selectedTextOverlay->SetCenter({(x0 + x1) * 0.5f, (y0 + y1) * 0.5f});
        }

        for (int i = 0; i < static_cast<int>(optionTextOverlays.size()); i++)
        {
            const std::array<float, 4> optionPos = GetOptionPosition(i);
            optionTextOverlays[i]->SetCenter({(optionPos[0] + optionPos[2]) * 0.5f, (optionPos[1] + optionPos[3]) * 0.5f});
        }
    }

    /* Dropdown Option Position */
    // Returns where the indexed option should render when the dropdown is open
    std::array<float, 4> DropdownButton::GetOptionPosition(int index) const
    {
        constexpr int optionColumns = 2;
        const int rowsPerColumn = std::max(1, static_cast<int>(std::ceil(static_cast<float>(labels.size()) / optionColumns)));
        const int row = index % rowsPerColumn;
        const int col = index / rowsPerColumn;
        const float optionWidth = width * 0.5f;
        const float optionTop = pos[1] - height * row;
        const float optionBottom = optionTop - height;
        const float optionLeft = pos[0] + col * optionWidth;
        const float optionRight = optionLeft + optionWidth;
        return {optionLeft, optionBottom, optionRight, optionTop};
    }

    /* Bounds Check */
    // Returns if x/y is inside the given bounds
    bool DropdownButton::ContainsPoint(const std::array<float, 4> &bounds, float x, float y) const
    {
        return bounds[0] <= x && x <= bounds[2] && bounds[1] <= y && y <= bounds[3];
    }

    /* Dropdown Drawing */
    // Draws dropdown from position, and draws all options if it is currently open
    void DropdownButton::Draw(GLuint shader)
    {
        glUseProgram(shader);
        glBindVertexArray(vao);

        int viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);
        const float aspect = static_cast<float>(viewport[2]) / std::max(1, viewport[3]);

        glUniform1i(shapeLoc, 1);
        glUniform1f(radiusLoc, std::min(width, height) * 0.18f);
        glUniform1f(glGetUniformLocation(shader, "u_Aspect"), aspect);

        // Draw the closed button using the currently selected color
        std::array<float, 4> selectedColor = optionColors[selectedIndex];
        if (mainCandidate)
            selectedColor = {selectedColor[0] * 0.8f, selectedColor[1] * 0.8f, selectedColor[2] * 0.8f, selectedColor[3]};

        glUniform2f(posLoc, (pos[0] + pos[2]) * 0.5f, (pos[1] + pos[3]) * 0.5f);
        glUniform2f(scaleLoc, pos[2] - pos[0], pos[3] - pos[1]);
        glUniform4f(colorLoc, selectedColor[0], selectedColor[1], selectedColor[2], selectedColor[3]);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Draw each selectable option beneath the closed button when open
        if (open)
        {
            for (int i = 0; i < static_cast<int>(labels.size()); i++)
            {
                const std::array<float, 4> optionPos = GetOptionPosition(i);
                std::array<float, 4> optionColor = optionColors[i];

                if (candidateIndex == i)
                    optionColor = {optionColor[0] * 0.8f, optionColor[1] * 0.8f, optionColor[2] * 0.8f, optionColor[3]};

                glUniform2f(posLoc, (optionPos[0] + optionPos[2]) * 0.5f, (optionPos[1] + optionPos[3]) * 0.5f);
                glUniform2f(scaleLoc, optionPos[2] - optionPos[0], optionPos[3] - optionPos[1]);
                glUniform4f(colorLoc, optionColor[0], optionColor[1], optionColor[2], optionColor[3]);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
            }
        }

        glBindVertexArray(0);

        if (selectedTextOverlay)
            selectedTextOverlay->Draw(textShader);

        if (open)
        {
            for (auto &textOverlay : optionTextOverlays)
                textOverlay->Draw(textShader);
        }
    }

    /* Update Dropdown Options */
    // Configure the dropdown options from user input
    void DropdownButton::SetOptions(const std::vector<std::string> &newLabels, const std::vector<std::array<float, 4>> &newOptionColors)
    {
        // Ensure labels and colors are not the same
        const bool sameLabels = labels.size() == newLabels.size() && std::equal(labels.begin(), labels.end(), newLabels.begin());
        const bool sameColors = optionColors.size() == newOptionColors.size() && std::equal(optionColors.begin(), optionColors.end(), newOptionColors.begin());
        
        if (sameLabels && sameColors)
            return;

        // Update labels / colors
        labels = newLabels;
        optionColors = newOptionColors;

        // Filter if they don't exist
        if (labels.empty())
            labels.emplace_back("None");

        // Default color
        while (optionColors.size() < labels.size())
            optionColors.push_back({0.7f, 0.7f, 0.7f, 0.9f});

        // Ensure both are the same size
        optionColors.resize(labels.size());

        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(labels.size()))
            selectedIndex = 0;

        // Update everything
        candidateIndex = -1;
        mainCandidate = false;
        optionTextOverlays.clear();
        InitTextOverlays();
        RecalculatePosition();
    }

    /* Dropdown Input Handling */
    // Updates hover/open/selection state for the dropdown based on the current cursor and mouse state
    void DropdownButton::HandleInput(GLFWwindow *window)
    {
        double cursorX, cursorY;
        glfwGetCursorPos(window, &cursorX, &cursorY);

        int windowWidth, windowHeight;
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        const float x = static_cast<float>((cursorX / windowWidth) * 2.0 - 1.0);
        const float y = static_cast<float>(-((cursorY / windowHeight) * 2.0 - 1.0));
        const bool mousePressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        const bool mouseClicked = mousePressed && !mouseWasDown;
        mouseWasDown = mousePressed;

        const bool mainHovered = ContainsPoint(pos, x, y);
        int hoveredOption = -1;

        if (open)
        {
            for (int i = 0; i < static_cast<int>(labels.size()); i++)
            {
                if (ContainsPoint(GetOptionPosition(i), x, y))
                {
                    hoveredOption = i;
                    break;
                }
            }
        }

        mainCandidate = mainHovered && mousePressed;
        candidateIndex = (hoveredOption >= 0) ? hoveredOption : -1;

        if (!open)
        {
            if (mouseClicked && mainHovered)
            {
                open = true;
                RecalculatePosition();
            }
            return;
        }

        if (mouseClicked)
        {
            if (hoveredOption >= 0 && hoveredOption < static_cast<int>(labels.size()))
            {
                selectedIndex = hoveredOption;
                if (selectedTextOverlay)
                    selectedTextOverlay->SetText(labels[selectedIndex]);
            }

            open = false;
            RecalculatePosition();
        }
    }

    #pragma endregion

    // SubPanel class functions
    #pragma region SubPanel

    std::vector<SubPanel*> SubPanel::registeredPanels;

    /* Default SubPanel Constructor */
    // Creates an inactive, empty subpanel
    SubPanel::SubPanel():
        nextId(0), size(0.0f), margin(0.0f), color({1, 1, 1, .5f}), active(false) {}

    /* Full SubPanel Constructor */
    // Creates a top-right square subpanel with a given size, margin, and color
    SubPanel::SubPanel(float size, float margin, std::array<float, 4> color):
        nextId(0), size(size), margin(margin), color(color), active(true) {}

    /* SubPanel Text Area Creation */
    // Creates a new text area under the subpanel
    element SubPanel::AddTextElement(float fontSize, unsigned int charactersPerLine, float xOffset, float yOffset, std::string font, bool autoShrink, std::string text, bool startAtCenter)
    {
        // Calculate the subpanel's center so the text can be positioned relative to it
        const float subPanelCenterX = 1.0f - margin - size / 2.0f;
        const float subPanelCenterY = 1.0f - margin - size / 2.0f;

        // Create new text area
        TextArea textArea(fontSize, charactersPerLine, {subPanelCenterX + xOffset, subPanelCenterY + yOffset}, font, autoShrink, text, startAtCenter);

        // Add to elements
        elements.push_back(ElementHandle{std::make_unique<TextArea>(std::move(textArea)), element{nextId}, true});

        // Update Ids
        element assignedId = elements.back().id;
        nextId++;

        if(nextId == 0)
            std::cout << "ERROR: To many elements have been created under subpanel!" << std::endl;

        return assignedId;
    }

    /* SubPanel Slider Creation */
    // Creates a new slider under the subpanel
    element SubPanel::AddSlider(float xOffset, float yOffset, float length, float totalHeight, float defaultValue)
    {
        // Create new slider
        Slider slider(xOffset, 0.0f, length, totalHeight, defaultValue);

        // Subpanel centers both axes itself, so yOffset is stored as an absolute yCenter before initializing
        slider.panelCenter = 1.0f - margin - size / 2.0f;
        slider.yCenter = 1.0f - margin - size / 2.0f + yOffset;
        slider.RecalculatePosition();

        // Add to elements
        elements.push_back(ElementHandle{std::make_unique<Slider>(std::move(slider)), element{nextId}, true});

        // Update Ids
        element assignedId = elements.back().id;
        nextId++;

        if(nextId == 0)
            std::cout << "ERROR: To many elements have been created under subpanel!" << std::endl;

        return assignedId;
    }

    /* SubPanel Button Creation */
    // Creates a new button under the subpanel
    element SubPanel::AddButton(float xOffset, float yOffset, float width, float height, std::array<float, 4> color, GLuint textShader, std::string text, std::string font, int fontSize, bool autoShrink)
    {
        // Create new button
        Button button(xOffset, 0.0f, width, height, color, textShader, text, font, fontSize, autoShrink);

        // Subpanel centers both axes itself, so yOffset is stored as an absolute yCenter before initializing
        button.panelCenter = 1.0f - margin - size / 2.0f;
        button.yCenter = 1.0f - margin - size / 2.0f + yOffset;
        button.RecalculatePosition();

        // Add to elements
        elements.push_back(ElementHandle{std::make_unique<Button>(std::move(button)), element{nextId}, true});

        // Update Ids
        element assignedId = elements.back().id;
        nextId++;

        if(nextId == 0)
            std::cout << "ERROR: To many elements have been created under subpanel!" << std::endl;

        return assignedId;
    }

    /* SubPanel Dropdown Creation */
    // Creates a new dropdown under the subpanel
    element SubPanel::AddDropdownButton(float xOffset, float yOffset, float width, float height, std::vector<std::string> labels, std::vector<std::array<float, 4>> optionColors, GLuint textShader, std::string font, int fontSize, bool autoShrink)
    {
        // Create new dropdown
        DropdownButton dropdown(xOffset, 0.0f, width, height, labels, optionColors, textShader, font, fontSize, autoShrink);

        // Subpanel centers both axes itself, so yOffset is stored as an absolute yCenter before initializing
        dropdown.panelCenter = 1.0f - margin - size / 2.0f;
        dropdown.yCenter = 1.0f - margin - size / 2.0f + yOffset;
        dropdown.RecalculatePosition();

        // Add to elements
        elements.push_back(ElementHandle{std::make_unique<DropdownButton>(std::move(dropdown)), element{nextId}, true});

        // Update Ids
        element assignedId = elements.back().id;
        nextId++;

        if(nextId == 0)
            std::cout << "ERROR: To many elements have been created under subpanel!" << std::endl;

        return assignedId;
    }

    /* SubPanel Graphics Initialization */
    // Loads all necessary info to draw subpanel and elements
    void SubPanel::Init(GLuint uiShader, GLuint textShader)
    {
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 8 * sizeof(float), vertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float)*2, 0);

        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);

        glUseProgram(uiShader);
        glBindVertexArray(vao);

        posLoc = glGetUniformLocation(uiShader, "u_Position");
        scaleLoc = glGetUniformLocation(uiShader, "u_Scale");
        colorLoc = glGetUniformLocation(uiShader, "desiredColor");

        glBindVertexArray(0);

        for (auto &e : elements)
        {
            if (auto *button = dynamic_cast<Button*>(e.ptr.get()))
            {
                button->textShader = textShader;
                button->Init(uiShader);
            }
            else if (auto *dropdown = dynamic_cast<DropdownButton*>(e.ptr.get()))
            {
                dropdown->textShader = textShader;
                dropdown->Init(uiShader);
            }
            else if (auto *textArea = dynamic_cast<TextArea*>(e.ptr.get()))
                textArea->Init(textShader);
            else
                e.ptr->Init(uiShader);
        }

        registeredPanels.push_back(this);
    }

    /* SubPanel Drawing */
    // Draws subpanel on screen via OpenGL
    void SubPanel::Draw(GLuint uiShader, GLuint textShader)
    {
        if (!active)
            return;

        glUseProgram(uiShader);
        glBindVertexArray(vao);

        const float centerX = 1.0f - margin - size / 2.0f;
        const float centerY = 1.0f - margin - size / 2.0f;

        glUniform1i(glGetUniformLocation(uiShader, "u_Shape"), 1);
        glUniform1f(glGetUniformLocation(uiShader, "u_CornerRadius"), size * 0.04f);
        glUniform2f(posLoc, centerX, centerY);
        glUniform2f(scaleLoc, size, size);
        glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        for (auto &e : elements)
        {
            if (e.active)
            {
                if(auto *textArea = dynamic_cast<TextArea*>(e.ptr.get()))
                    e.ptr->Draw(textShader);
                else
                    e.ptr->Draw(uiShader);
            }
        }

        glBindVertexArray(0);
    }

    /* SubPanel Update Function */
    // Updates everything related to the subpanel
    void SubPanel::Update(GLuint uiShader, GLuint textShader, GLFWwindow *window)
    {
        if (!active)
            return;

        for (auto &e : elements)
        {
            if (e.active)
                e.ptr->HandleInput(window);
        }

        Draw(uiShader, textShader);
    }

    /* SubPanel Slider Value Retrieving */
    // Called by user; finds given slider and return's its normalized value
    float SubPanel::GetSliderValue(element slider)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == slider.id)
            {
                if(auto *sliderPtr = dynamic_cast<Slider*>(elements[i].ptr.get()))
                    return sliderPtr->GetNormalizedValue();
                else
                    return -1;
            }
        }

        return -1;
    }

    /* SubPanel Button Status */
    // Returns if a button is pressed
    bool SubPanel::IsButtonDown(element button)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == button.id)
            {
                if(auto *buttonPtr = dynamic_cast<Button*>(elements[i].ptr.get()))
                    return buttonPtr->pressed;
            }
        }

        return false;
    }

    /* SubPanel Button Reset */
    // Resets a button press after it has been handled
    void SubPanel::ResetButton(element button)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == button.id)
            {
                if(auto *buttonPtr = dynamic_cast<Button*>(elements[i].ptr.get()))
                    buttonPtr->pressed = false;
                return;
            }
        }
    }

    /* SubPanel Dropdown Value */
    // Gets the currently selected index from a dropdown
    int SubPanel::GetDropdownSelectedIndex(element dropdown)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == dropdown.id)
            {
                if(auto *dropdownPtr = dynamic_cast<DropdownButton*>(elements[i].ptr.get()))
                    return dropdownPtr->GetSelectedIndex();
            }
        }

        return -1;
    }

    /* Update Dropdown */
    // Goes through each element -> finds dropdown box -> updates its labels and option colors
    void SubPanel::SetDropdownOptions(element dropdown, const std::vector<std::string> &labels, const std::vector<std::array<float, 4>> &optionColors)
    {
        for (int i = 0; i < static_cast<int>(elements.size()); ++i)
        {
            if (elements[i].id == dropdown.id)
            {
                if (auto *dropdownPtr = dynamic_cast<DropdownButton*>(elements[i].ptr.get()))
                    dropdownPtr->SetOptions(labels, optionColors);
                return;
            }
        }
    }

    /* SubPanel Text Update */
    // Updates the text displayed by a text area
    void SubPanel::SetText(element textElement, std::string text)
    {
        for(int i = 0; i < elements.size(); i++)
        {
            if(elements[i].id == textElement.id)
            {
                if(auto *textArea = dynamic_cast<TextArea*>(elements[i].ptr.get()))
                    textArea->SetText(text);

                return;
            }
        }
    }

    /* SubPanel Element Active */
    // Enables or disables one element from input/rendering
    void SubPanel::SetElementActive(element elementId, bool active)
    {
        for (auto &entry : elements)
        {
            if (entry.id == elementId)
            {
                entry.active = active;
                return;
            }
        }
    }

    /* SubPanel Active */
    // Enables or disables the full subpanel
    void SubPanel::SetActive(bool active)
    {
        this->active = active;
    }

    /* SubPanel Point Check */
    // Returns if a normalized point is inside the subpanel bounds or an open dropdown
    bool SubPanel::ContainsPoint(float x, float y) const
    {
        const float x1 = 1.0f - margin;
        const float x0 = x1 - size;
        const float y1 = 1.0f - margin;
        const float y0 = y1 - size;

        if (!active)
            return false;

        if (x0 <= x && x <= x1 && y0 <= y && y <= y1)
            return true;

        // Dropdown options can render outside of the subpanel's square.
        // Treat those option rows as part of the UI so clicking them does not start a world selection.
        for (const auto &entry : elements)
        {
            if (!entry.active)
                continue;

            auto *dropdown = dynamic_cast<DropdownButton*>(entry.ptr.get());
            if (!dropdown || !dropdown->open)
                continue;

            if (dropdown->ContainsPoint(dropdown->pos, x, y))
                return true;

            for (int i = 0; i < static_cast<int>(dropdown->labels.size()); i++)
            {
                if (dropdown->ContainsPoint(dropdown->GetOptionPosition(i), x, y))
                    return true;
            }
        }

        return false;
    }

    /* Active SubPanel Point Check */
    // Returns if a normalized point is inside any active registered subpanel
    bool SubPanel::AnyActivePanelContains(float x, float y)
    {
        for (SubPanel *panel : registeredPanels)
        {
            if (panel && panel->ContainsPoint(x, y))
                return true;
        }

        return false;
    }

    #pragma endregion
}
