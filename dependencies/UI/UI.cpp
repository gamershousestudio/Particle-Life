#include "UI.h"

namespace UI
{
    // Element class functions
    #pragma region Element

    Element::Element(std::array<float, 4> pos, std::array<float, 4> color): 
        pos(pos), color(color) {}

    Element::Element(std::array<float, 4> pos):
        pos(pos) {}

    Element::Element() {}

    #pragma endregion


    // Panel class functions
    #pragma region Panel

    /* Full Panel Constructor */
    // Creates a new panel with all the information needed to draw it
    Panel::Panel(int side, float length, std::array<float, 4> color): side((!side) ? -1 : 1), length(length), color(color), nextId(0), vao(0), vbo(0), ebo(0), posLoc(-1), colorLoc(-1), scaleLoc(-1) {}

    /* Grid Creation */
    // Creates a new instance of grid under the panel
    element Panel::AddGrid(std::array<float, 4> position, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect)
    {
        // Create new grid
        Grid grid(position, numberOfBoxes, values, useInputs, aspect);

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

        // Initializes all other elements
        for (auto &e : elements)
        {
            // Makes sure element is supposed to be initialized
            if (e.active)
                // Runs element's Init() function (actual Element class's is overriden)
                e.ptr->Init(shader); // Runs element's init function
        }

        // Attach instance of panel with window
        glfwSetWindowUserPointer(window, this);

        // Initialize inputs
        glfwSetScrollCallback(window, ScrollCallback);
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

        // Binds shader variables
        glUniform2f(posLoc, (side + side*(1-length))/2, 0);
        glUniform2f(scaleLoc, fabs(side*(1-length)-side), 2);
        glUniform4f(colorLoc, color[0], color[1], color[2], color[3]);

        // Draws rectangle
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        // Unbinds intermediates
        glBindVertexArray(0);
        

        // Draws all other elements
        for (auto &e : elements)
        {
            // Makes sure element is supposed to be drawn
            if (e.active)
                // Runs element's Draw() function (actual Element class's is overriden)
                e.ptr->Draw(shader); // Runs element's draw function
        }
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
                if((elementPtr->pos[0] <= x && x <= elementPtr->pos[2]) && (elementPtr->pos[3] <= y && y <= elementPtr->pos[1]))
                {
                    // Call said object's scroll
                    elementPtr->Scroll(window, xOffset, yOffset);

                    // End function, no point in continuing to look
                    return;
                }
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

    #pragma endregion

    // Grid class functions
    #pragma region Grid

    /* Full Grid Constructor */
    // Creates a new grid with all information needed to be drawn
    Grid::Grid(std::array<float, 4> position, unsigned int numberOfBoxes, std::vector<std::vector<float>> *values, bool useInputs, float aspect):
        boxesCount(numberOfBoxes), values(values), Element(position), useInputs(useInputs), aspect(aspect) {}

    /* Drawing Initialization */
    // Prepares all parts of the grid to be drawn -- anything that does not need to be ran every frame
    void Grid::Init(GLuint shader)
    {
        // Lock values based on what is expected
        float x0 = std::min(pos[0], pos[2]);
        float x1 = std::max(pos[0], pos[2]);
        float y0 = std::max(pos[1], pos[3]);
        float y1 = std::min(pos[1], pos[3]);

        // Get how big each cell should be
        float cellW = (x1 - x0) / boxesCount;
        float cellH = (y1 - y0) / boxesCount;

        // Get where the y axis is centered around
        float centerY = (y0 + y1) * 0.5f;

        // Prepares each square
        std::array<float, 4> color;
        std::array<float, 4> position;

        float centerX;

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

                position[1] = centerY + (boxesCount/2.0 - j) * cellH;
                position[3] = position[1] - cellH;

                // Fixes xs to make them more square-ular[i][j], 0,
                centerX = (pos[0] + pos[2])/2.0;
                position[0] = centerX + (position[0] - centerX) / aspect;
                position[2] = centerX + (position[2] - centerX) / aspect;

                boxes.emplace_back(position, color);
            }
        }

        // Update element's position
        pos = {centerX+(x0-centerX)/aspect, y0, centerX+(x1-centerX)/aspect, y1};

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


        // Enables attribute pointer; only parameter is index to enable
        glEnableVertexAttribArray(0);
    }

    void Grid::Draw(GLuint shader)
    {
        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);

        // Where each variable is located in the shader so they can be set
        GLint posLoc = glGetUniformLocation(shader, "u_Position");
        GLint scaleLoc = glGetUniformLocation(shader, "u_Scale");
        GLint colorLoc = glGetUniformLocation(shader, "desiredColor");


        // Makes sure shader works correctly
        if (posLoc == -1 || scaleLoc == -1 || colorLoc == -1)
        {
            std::cout << "Warning: shader uniform not found." << std::endl;
        }

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
            if(square.pos[0] <= x && x <= square.pos[2] && square.pos[1] <= y && y <= square.pos[3])
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