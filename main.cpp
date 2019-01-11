#include "head.h"

bool saveOutput = false; //Make to true to save out your animation 
int screen_width = 800;
int screen_height = 600;

int num_thread = 4;
bool Eulerian = true; 
bool burn = false;
bool motion = true;

//initialization
float cloth_width = 20.0f; float cloth_height = 20.0f;
int nh = 30; int nv = 30;
float node_disth = cloth_width/(float)(nh-1); 
float node_distv = cloth_height/(float)(nv-1);
float radius = 3.0f; float scale_radius = radius * 2;
glm::vec3 sphere_position = glm::vec3(-10.0f, 10.0f, 0.0f);

int trian_row_num = nv - 1;
int trian_col_num = (nh - 1) * 2;
int trian_num = trian_row_num * trian_col_num;

float ks = 120; float kv = 2;
float restlen = node_disth; float mass = 0.1;
glm::vec3 spring_top = glm::vec3(0.0f, -5.0f, 6.0f);
glm::vec3 gravity = glm::vec3(0.0f, 0.0f, -9.8f);

glm::vec3 ini_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ini_acceleration = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 ini_force = glm::vec3(0.0f, 0.0f, 0.0f);
float dens_air = 1; float cd = 1;
glm::vec3 vel_air = glm::vec3(-1.0f, 0.0f, 0.0f);
glm::vec3 vel_air_incre = vel_air * 2.0f;

glm::vec3 *velocity = (glm::vec3*)malloc(nh*nv * sizeof(glm::vec3));
glm::vec3 *acceleration = (glm::vec3*)malloc(nh*nv * sizeof(glm::vec3));
glm::vec3 *force = (glm::vec3*)malloc(nh*nv * sizeof(glm::vec3));

glm::vec3 *position = (glm::vec3*)malloc(4 * sizeof(glm::vec3));//0-up,1-down,2-left,3-right
glm::vec3 *springF = (glm::vec3*)malloc(4 * sizeof(glm::vec3));//0-up,1-down,2-left,3-right
glm::vec3 *dampF = (glm::vec3*)malloc(4 * sizeof(glm::vec3));//0-up,1-down,2-left,3-right
glm::vec3 *veloc = (glm::vec3*)malloc(4 * sizeof(glm::vec3));//0-up,1-down,2-left,3-right
glm::vec3 pos, dire, vel, r1, r2, r3, n, bounce;
float springforce;

float ***nodes = (float ***)malloc(nv * sizeof(float**));
Triangle *trian = (Triangle*)malloc(trian_num * sizeof(Triangle));

//fire
vector<glm::vec3>particle_position;
vector<glm::vec3>particle_velocity;
vector<glm::vec3>particle_color;
vector<float>particle_lifespan;
float particle_radius = 0.2f;
float floorPos = -1.2;
float fire_radius = 1.0f;
glm::vec3 ini_particle_position = glm::vec3(0.0f, 0.0f, 0.0f);
float maxLifeSpan = 0.8f;
//set attributes for flames on a candle
//used a cone and a semisphere to simulate the shape of the flame
//outside: red, sphere center is (0,0,radius3)
float radius3 = fire_radius;
glm::vec3 c3 = glm::vec3(0.0f, 0.0f, radius3);
float h3 = 2.0f;
//core: white, sphere center is (0,0,radius1)
float radius1 = 1.5f;
glm::vec3 c1 = glm::vec3(0.0f, 0.0f, radius1 - radius3);
//median: yellow, sphere center is (0,0,radius2)
float radius2 = 1.0f;
glm::vec3 c2 = glm::vec3(0.0f, 0.0f, radius2 - radius3);

bool fullscreen = false;
void UpdatePosition(float dt);

// Sphere Shader sources
const GLchar* vertexSource =
"#version 150 core\n"
"in vec3 position;"
//"in vec3 inColor;"
//the color will be changed in the OpenGL part
//change the number of particles
"uniform vec3 inColor;"
"in vec3 inNormal;"
"const vec3 inLightDir = normalize(vec3(0,2,2));"
"out vec3 Color;"
"out vec3 normal;"
"out vec3 lightDir;"
"uniform mat4 model;"
"uniform mat4 view;"
"uniform mat4 proj;"
"void main() {"
"   Color = inColor;"
"   gl_Position = proj * view * model * vec4(position, 1.0);"
"   vec4 norm4 = transpose(inverse(model)) * vec4(inNormal, 1.0);"
"   normal = normalize(norm4.xyz);"
"   lightDir = (view * vec4(inLightDir, 0)).xyz;"
"}";

const GLchar* fragmentSource =
"#version 150 core\n"
"in vec3 Color;"
"in vec3 normal;"
"in vec3 lightDir;"
"out vec4 outColor;"
"const float ambient = .2;"
"void main() {"
"   vec3 diffuseC = Color * max(dot(lightDir, normal), 0);"
"   vec3 ambC = Color * ambient;"
// the alpha channel of all pixels are 1.0, i.e. the opacity is 100%
"   outColor = vec4(diffuseC+ambC, 1.0);"
"}";

//Index of where to model, view, and projection matricies are stored on the GPU
GLint uniModel, uniView, uniProj, uniColor;

float aspect; //aspect ratio (needs to be updated if the window is resized)

int main(int argc, char *argv[]) {
	SDL_Init(SDL_INIT_VIDEO);  //Initialize Graphics (for OpenGL)

	//Ask SDL to get a recent version of OpenGL (3.2 or greater)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);

	//Create a window (offsetx, offsety, width, height, flags)
	SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 50, screen_width, screen_height, SDL_WINDOW_OPENGL);
	aspect = screen_width / (float)screen_height; //aspect ratio (needs to be updated if the window is resized)

	//The above window cannot be resized which makes some code slightly easier.
	//Below show how to make a full screen window or allow resizing
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 0, 0, screen_width, screen_height, SDL_WINDOW_FULLSCREEN|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program", 100, 100, screen_width, screen_height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL);
	//SDL_Window* window = SDL_CreateWindow("My OpenGL Program",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,0,0,SDL_WINDOW_FULLSCREEN_DESKTOP|SDL_WINDOW_OPENGL); //Boarderless window "fake" full screen

	//Create a context to draw in
	SDL_GLContext context = SDL_GL_CreateContext(window);

	if (gladLoadGLLoader(SDL_GL_GetProcAddress)) {
		printf("\nOpenGL loaded\n");
		printf("Vendor:   %s\n", glGetString(GL_VENDOR));
		printf("Renderer: %s\n", glGetString(GL_RENDERER));
		printf("Version:  %s\n\n", glGetString(GL_VERSION));
	}
	else {
		printf("ERROR: Failed to initialize OpenGL context.\n");
		return -1;
	}

	omp_set_num_threads(num_thread);

	//// Allocate Texture 0 (wood) ///////
	SDL_Surface* surface = SDL_LoadBMP("dragon.bmp");
	if (surface == NULL) { //If it failed, print the error
		printf("Error: \"%s\"\n", SDL_GetError()); return 1;
	}
	GLuint tex0;
	glGenTextures(1, &tex0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex0);

	//What to do outside 0-1 range
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load the texture into memory
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_BGR, GL_UNSIGNED_BYTE, surface->pixels);
	glGenerateMipmap(GL_TEXTURE_2D);

	SDL_FreeSurface(surface);
	//// End Allocate Texture ///////

	//--------------------------------------
	//create cloth nodes (nv*nh)
	//the 0th row is the top row, fixed
	//--------------------------------------
	for (int i = 0; i < nv; i++) {
		nodes[i] = (float **)malloc(nh * sizeof(float*));
		for (int j = 0; j < nh; j++) {
			nodes[i][j] = (float *)malloc(8 * sizeof(float));
		}
	}

	for (int i = 0; i < nv; i++) {
		for (int j = 0; j < nh; j++) {
			//set position X, Y, Z
			/*nodes[i][j][0] = 0.0f;
			nodes[i][j][1] = j * 0.1f;
			nodes[i][j][2] = -i * 0.1f;*/

			nodes[i][j][0] = spring_top.x-i * node_distv;
			nodes[i][j][1] = spring_top.y+j * node_disth;
			nodes[i][j][2] = spring_top.z+0.0f;

			//set normal
			nodes[i][j][3] = 1.0f;
			nodes[i][j][4] = 0.0f;
			nodes[i][j][5] = 0.0f;
			
			//set texture cord
			nodes[i][j][6] = (float)j / (float)(nh - 1);
			nodes[i][j][7] = (float)i / (float)(nv - 1);
		}
	}

	//initialize the property of nodes in cloth
	for (int i = 0; i < nv; i++) {
		for (int j = 0; j < nh; j++) {
			velocity[i*nh + j] = ini_velocity;
			acceleration[i*nh + j] = ini_acceleration;
			force[i*nh + j] = ini_force;
		}
	}

	//initialize the elements of triangles
	//each row has (nv-1)*2 triangles
	int id;
	for (int i = 0; i < nv-1; i++) {
		for (int j = 0; j < nh-1; j++) {
			id = 2 * (i + j) + i * 2*(nh-2);
			//lower triangle
			trian[id].index[0][0] = i;
			trian[id].index[0][1] = j;
			trian[id].index[1][0] = i + 1;
			trian[id].index[1][1] = j + 1;
			trian[id].index[2][0] = i + 1;
			trian[id].index[2][1] = j;
			//upper triangle
			trian[id + 1].index[0][0] = i;
			trian[id + 1].index[0][1] = j;
			trian[id + 1].index[1][0] = i + 1;
			trian[id + 1].index[1][1] = j + 1;
			trian[id + 1].index[2][0] = i;
			trian[id + 1].index[2][1] = j + 1;
		}
	}
	
	// create cloth to be loaded to VBO
	float *cloth = (float *)malloc(8 * 3 * trian_num * sizeof(float));
	int hp, vp; //store the current index of nodes
	int cloth_numLines = 3 * trian_num * 8;
	int cloth_numVerts = 3 * trian_num;

	//--------------------------------------
	// load cloth
	//--------------------------------------
	//Build a Vertex Array Object. This stores the VBO and attribute mappings in one object
	GLuint vao_cloth;
	glGenVertexArrays(1, &vao_cloth); //Create a VAO
	glBindVertexArray(vao_cloth); //Bind the above created VAO to the current context

	//Join the vertex and fragment shaders together into one program
	int ClothShaderProgram = InitShader("vertexTex.glsl", "fragmentTex.glsl");
	GLuint vbo_cloth;
	glGenBuffers(1, &vbo_cloth);  //Create 1 buffer called vbo
	glBindVertexArray(0); //Bind the above created VAO to the current context

	//------------------------------
	//load sphere as obstacle
	//------------------------------
	ifstream modelFile;
	modelFile.open("sphere.txt");
	int sphere_numLines = 0;
	modelFile >> sphere_numLines;
	float *sphere = new float[sphere_numLines];
	for (int i = 0; i < sphere_numLines; i++) {
		modelFile >> sphere[i];
	}
	printf("sphere numLines: %d\n", sphere_numLines);
	int sphere_numVerts = sphere_numLines / 8;
	modelFile.close();

	//Allocate memory on the graphics card to store geometry (vertex buffer object)
	GLuint vbo_sphere;
	glGenBuffers(1, &vbo_sphere);  //Create 1 buffer called vbo
	glBindBuffer(GL_ARRAY_BUFFER, vbo_sphere); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
	glBufferData(GL_ARRAY_BUFFER, sphere_numLines * sizeof(float), sphere, GL_STATIC_DRAW); //upload vertices to vbo
	//GL_STATIC_DRAW means we won't change the geometry, GL_DYNAMIC_DRAW = geometry changes infrequently
	//GL_STREAM_DRAW = geom. changes frequently.  This effects which types of GPU memory is used

	GLuint vao_sphere;
	glGenVertexArrays(1, &vao_sphere); //Create a VAO
	glBindVertexArray(vao_sphere); //Bind the above created VAO to the current context
	
	//Load the vertex Shader
	GLuint SphereVertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(SphereVertexShader, 1, &vertexSource, NULL);
	glCompileShader(SphereVertexShader);

	//Let's double check the shader compiled 
	GLint status;
	glGetShaderiv(SphereVertexShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char buffer[512];
		glGetShaderInfoLog(SphereVertexShader, 512, NULL, buffer);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Compilation Error",
			"Failed to Compile: Check Consol Output.",
			NULL);
		printf("Vertex Shader Compile Failed. Info:\n\n%s\n", buffer);
	}

	GLuint SphereFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(SphereFragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(SphereFragmentShader);

	//Double check the shader compiled 
	glGetShaderiv(SphereFragmentShader, GL_COMPILE_STATUS, &status);
	if (!status) {
		char buffer[512];
		glGetShaderInfoLog(SphereFragmentShader, 512, NULL, buffer);
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
			"Compilation Error",
			"Failed to Compile: Check Consol Output.",
			NULL);
		printf("Fragment Shader Compile Failed. Info:\n\n%s\n", buffer);
	}

	//Join the vertex and fragment shaders together into one program
	GLuint SphereShaderProgram = glCreateProgram();
	glAttachShader(SphereShaderProgram, SphereVertexShader);
	glAttachShader(SphereShaderProgram, SphereFragmentShader);
	glBindFragDataLocation(SphereShaderProgram, 0, "outColor"); // set output
	glLinkProgram(SphereShaderProgram); //run the linker
	glUseProgram(SphereShaderProgram); //Set the active shader (only one can be used at a time)

	//Tell OpenGL how to set fragment shader input 
	GLuint posAttrib = glGetAttribLocation(SphereShaderProgram, "position");
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
	//Attribute, vals/attrib., type, normalized?, stride, offset
	//Binds to VBO current GL_ARRAY_BUFFER 
	glEnableVertexAttribArray(posAttrib);

	GLuint nolAttrib = glGetAttribLocation(SphereShaderProgram, "inNormal");
	glVertexAttribPointer(nolAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
	glEnableVertexAttribArray(nolAttrib);

	glBindVertexArray(0); //Unbind the VAO

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Event Loop (Loop forever processing each event as fast as possible)
	SDL_Event windowEvent;
	bool quit = false;
	srand(time(NULL));

	float lastTime = SDL_GetTicks() / 1000.f;
	float dt = 0;

	//set parameters for camera
	float movestep = 0.1;
	float anglestep = 0.1;
	glm::vec3 camera_position = glm::vec3(0.0f, 35.0f, 0.0f);  //Cam Position
	glm::vec3 look_point = glm::vec3(0.0f, 0.0f, 0.0f);  //Look at point
	glm::vec3 up_vector = glm::vec3(0.0f, 0.0f, 1.0f); //Up
	glm::vec3 move_vector = glm::vec3(0.0f, 0.0f, 0.0f);

	glm::mat4 proj = glm::perspective(3.14f / 4, aspect, 0.1f, 10000.0f); //FOV, aspect, near, far

	float timecost = 0.0f;
	int frame = 0;

	while (!quit) {
		while (SDL_PollEvent(&windowEvent)) {
			if (windowEvent.type == SDL_QUIT) quit = true; //Exit event loop
														   //List of keycodes: https://wiki.libsdl.org/SDL_Keycode - You can catch many special keys
														   //Scancode referes to a keyboard position, keycode referes to the letter (e.g., EU keyboards)
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_ESCAPE)
				quit = true; ; //Exit event loop
			if (windowEvent.type == SDL_KEYUP && windowEvent.key.keysym.sym == SDLK_f) //If "f" is pressed
				fullscreen = !fullscreen;
			SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN : 0); //Set to full screen

			//user-controlled camera
			if ((windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_UP) || \
				(windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_w)) {
				//move up
				move_vector = glm::normalize(look_point - camera_position)*movestep;
				camera_position = camera_position + move_vector;
				look_point = look_point + move_vector;
			}
			if ((windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_DOWN) || \
				(windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_s)) {
				//move down
				move_vector = glm::normalize(look_point - camera_position)*movestep;
				camera_position = camera_position - move_vector;
				look_point = look_point - move_vector;
			}
			if ((windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_LEFT) || \
				(windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_a)) {
				//move left
				move_vector = glm::normalize(look_point - camera_position);
				move_vector = glm::cross(up_vector, move_vector)*movestep;
				camera_position = camera_position + move_vector;
				look_point = look_point + move_vector;
			}
			if ((windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_RIGHT) || \
				(windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_d)) {
				//move right
				move_vector = glm::normalize(look_point - camera_position);
				move_vector = glm::cross(up_vector, move_vector)*movestep;
				camera_position = camera_position - move_vector;
				look_point = look_point - move_vector;
			}
			if (windowEvent.type == SDL_MOUSEWHEEL && windowEvent.wheel.y == 1) {// scroll up
				//turn left
				move_vector = glm::rotateZ(look_point - camera_position, anglestep);
				look_point = camera_position + move_vector;
			}
			if (windowEvent.type == SDL_MOUSEWHEEL && windowEvent.wheel.y == -1) {// scroll up
				//turn right
				move_vector = glm::rotateZ(look_point - camera_position, -anglestep);
				look_point = camera_position + move_vector;
			}

			//real-time user interaction
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_F1) {
				//increase wind
				vel_air = vel_air + vel_air_incre;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_F2) {
				//decrease wind
				vel_air = vel_air - vel_air_incre;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_b) {
				//set fire
				burn = true;
			}
			if (windowEvent.type == SDL_KEYDOWN && windowEvent.key.keysym.sym == SDLK_m) {
				//capture motion
				motion = false;
			}
		}
		
		// Clear the screen to default color
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (!saveOutput) dt = (SDL_GetTicks() / 1000.f) - lastTime;
		lastTime = SDL_GetTicks() / 1000.f;
		if (saveOutput) dt += .07; //Fix framerate at 14 FPS
		printf("frame_id:%d\ttimecost:%f\n", frame,timecost);

		//draw the sphere	
		glUseProgram(SphereShaderProgram);
		glm::vec3 inColor = glm::vec3(1.0f,0.0f,0.0f);
		uniProj = glGetUniformLocation(SphereShaderProgram, "inColor");
		glUniform3f(uniColor, inColor.r, inColor.g, inColor.b);

		uniProj = glGetUniformLocation(SphereShaderProgram, "proj");
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

		glm::mat4 view = glm::lookAt(camera_position, look_point, up_vector);
		uniView = glGetUniformLocation(SphereShaderProgram, "view");
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));		

		glm::mat4 sphere_model(1.0f);
		sphere_model = glm::translate(sphere_model, sphere_position);

		sphere_model = glm::scale(sphere_model, glm::vec3(scale_radius));
		uniModel = glGetUniformLocation(SphereShaderProgram, "model");
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(sphere_model));

		glBindVertexArray(vao_sphere);
		glDrawArrays(GL_TRIANGLES, 0, sphere_numVerts); //(Primitives, Which VBO, Number of vertices)
		glBindVertexArray(0);

		//draw cloth
		//transit to VBO
		//#pragma omp parallel for private(hp,vp)
		for (int i = 0; i < trian_num; i++) { // for each triangle
			for (int j = 0; j < 3; j++) { // for each node of a triangle
				for (int k = 0; k < 8; k++) {
					hp = trian[i].index[j][0];
					vp = trian[i].index[j][1];
					cloth[i * 24 + j * 8 + k] = nodes[hp][vp][k];
				}
			}
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cloth); //Set the vbo as the active array buffer (Only one buffer can be active at a time)
		glBufferData(GL_ARRAY_BUFFER, cloth_numLines * sizeof(float), cloth, GL_STATIC_DRAW);
		glBindVertexArray(vao_cloth);
		//Tell OpenGL how to set fragment shader input 
		GLint posAttrib = glGetAttribLocation(ClothShaderProgram, "position");
		glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), 0);
		//Attribute, vals/attrib., type, normalized?, stride, offset
		//Binds to VBO current GL_ARRAY_BUFFER 
		glEnableVertexAttribArray(posAttrib);

		GLint texAttrib = glGetAttribLocation(ClothShaderProgram, "inTexcoord");
		glEnableVertexAttribArray(texAttrib);
		glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

		GLint nolAttrib = glGetAttribLocation(ClothShaderProgram, "inNormal");
		glVertexAttribPointer(nolAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(nolAttrib);

		glBindVertexArray(0); //Unbind the VAO

		glUseProgram(ClothShaderProgram);

		uniProj = glGetUniformLocation(ClothShaderProgram, "proj");
		glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));

		uniView = glGetUniformLocation(ClothShaderProgram, "view");
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

		glm::mat4 cloth_model(1.0f);
		uniModel = glGetUniformLocation(ClothShaderProgram, "model");
		glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(cloth_model));

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex0);
		glUniform1i(glGetUniformLocation(ClothShaderProgram, "tex0"), 0);

		glBindVertexArray(vao_cloth);
		GLint uniTexID = glGetUniformLocation(ClothShaderProgram, "texID");
		glUniform1i(uniTexID, 0); //Set texture ID to use 
		glDrawArrays(GL_TRIANGLES, 0, cloth_numVerts); //(Primitives, Which VBO, Number of vertices)
		//glDrawArrays(GL_POINTS, 0, cloth_numVerts); //(Primitives, Which VBO, Number of vertices)
		glBindVertexArray(0);

		//update the position of nodes
		for (int i = 0; i < 20; i++) {
			UpdatePosition(dt/20);
		}

		//draw fire
		//burn = true;
		if (burn && nv>0) {
			//Partical birthrate
			float numParticles = PARTICLE_NUM;
			//each grid of the last row has segment particles
			int segment = (int)(numParticles / (float)(nh - 1));

			//generate new particles
			for (int i = 0; i < nh - 1; i++) {
				int start = i * segment;
				int end = (i + 1)* segment;
				if (i == nh - 2) end = numParticles;
				for (int j = start; j < end; j++) {
					glm::vec3 pos1 = glm::vec3(nodes[nv - 1][i][0], nodes[nv - 1][i][1], nodes[nv - 1][i][2]);
					glm::vec3 pos2 = glm::vec3(nodes[nv - 1][i + 1][0], nodes[nv - 1][i + 1][1], nodes[nv - 1][i + 1][2]);
					ini_particle_position = pos1 + ((float)j - (float)start) / ((float)end - (float)start)*(pos2 - pos1);
					
					//ini_particle_position = pos1;
					particle_position.push_back(ini_particle_position);
					//choose random particle velocity
					particle_velocity.push_back(glm::vec3(0.0f, 0.0f, 2 * randf()));
					particle_color.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
					particle_lifespan.push_back(maxLifeSpan);
				}
			}

			for (int i = 0; i < particle_position.size(); i++) {
				particle_lifespan[i] -= dt;
				glUseProgram(SphereShaderProgram);
				glm::vec3 inColor = particle_color[i];
				glUniform3f(uniColor, inColor.r, inColor.g, inColor.b);

				glm::mat4 model(1.0f);
				if (particle_lifespan[i] <= 0) {// draw the "alive" particles
					particle_position.erase(particle_position.begin() + i);
					particle_velocity.erase(particle_velocity.begin() + i);
					particle_color.erase(particle_color.begin() + i);
					particle_lifespan.erase(particle_lifespan.begin() + i);
				}

				model = glm::translate(model, particle_position[i]);
				model = glm::scale(model, glm::vec3(particle_radius));
				glUniformMatrix4fv(uniModel, 1, GL_FALSE, glm::value_ptr(model));

				computePhysics(i, dt);

				glBindVertexArray(vao_sphere);
				glDrawArrays(GL_TRIANGLES, 0, sphere_numVerts); //(Primitives, Which VBO, Number of vertices)		
			}
			glBindVertexArray(0);

			//eliminate the burned cloth
			if (frame % 4 == 0) {
				trian_num -= (nh - 1) * 2;
				nv -= 1;
				cloth_numLines = trian_num * 24;
			}
		}

		//motion blur for art
		if (!motion&&frame % 8 == 0) {
			trian_num -= (nh - 1) * 2;
			nv -= 1;
		}

		//compute fps
		frame++;
		timecost += dt;

		if (saveOutput) Win2PPM(screen_width, screen_height);
		
		SDL_GL_SwapWindow(window); //Double buffering
	}

	//Clean Up
	glDeleteProgram(ClothShaderProgram);
	glDeleteProgram(SphereShaderProgram);
	glDeleteShader(SphereFragmentShader);
	glDeleteShader(SphereVertexShader);
	glDeleteBuffers(1, &vbo_cloth);
	glDeleteVertexArrays(1, &vao_cloth);
	glDeleteBuffers(1, &vbo_sphere);
	glDeleteVertexArrays(1, &vao_sphere);

	SDL_GL_DeleteContext(context);
	SDL_Quit();

	free(cloth);
	free(nodes);
	free(sphere);
	free(trian);
	free(velocity);
	free(acceleration);
	free(position);
	free(springF);
	free(dampF);
	free(veloc);
	free(force);

	return 0;
}

void UpdatePosition(float dt) {
	//#pragma omp parallel for private(r1,r2,r3)
	//compute air-resistance for each triangle
	for (int i = 0; i < trian_row_num; i++) {
		for (int j = 0; j < trian_col_num; j++) {
			int id = i * trian_col_num + j;
			r1 = glm::vec3(nodes[trian[id].index[0][0]][trian[id].index[0][1]][0], \
				nodes[trian[id].index[0][0]][trian[id].index[0][1]][1], \
				nodes[trian[id].index[0][0]][trian[id].index[0][1]][2]);
			r2 = glm::vec3(nodes[trian[id].index[1][0]][trian[id].index[1][1]][0], \
				nodes[trian[id].index[1][0]][trian[id].index[1][1]][1], \
				nodes[trian[id].index[1][0]][trian[id].index[1][1]][2]);
			r3 = glm::vec3(nodes[trian[id].index[2][0]][trian[id].index[2][1]][0], \
				nodes[trian[id].index[2][0]][trian[id].index[2][1]][1], \
				nodes[trian[id].index[2][0]][trian[id].index[2][1]][2]);
			trian[id].velocity = velocity[trian[id].index[0][0] * nh + trian[id].index[0][1]] \
				+ velocity[trian[id].index[1][0] * nh + trian[id].index[1][1]] \
				+ velocity[trian[id].index[2][0] * nh + trian[id].index[2][1]];
			trian[id].velocity = trian[id].velocity / 3.0f - vel_air;
			trian[id].normal_star = glm::cross(r2 - r1, r3 - r1);
			trian[id].force_aero = (-0.25f*dens_air*cd / (glm::length(trian[id].normal_star)) \
				*(glm::length(trian[id].velocity))*(glm::dot(trian[id].velocity, trian[id].normal_star))) \
				*trian[id].normal_star;
		}
	}

	//compute resultant force for each node
	for (int i = 1; i < nv; i++) {// from the 2nd row to the last row
		for (int j = 0; j < nh; j++) {// from the 1st column to the last column
			//get current position and velocity
			pos = glm::vec3(nodes[i][j][0], nodes[i][j][1], nodes[i][j][2]);
			vel = velocity[i*nh + j];
			//get the up node
			position[0] = glm::vec3(nodes[i - 1][j][0], nodes[i - 1][j][1], nodes[i - 1][j][2]);
			veloc[0] = velocity[(i - 1)*nh + j];
			//get the down node
			if (i == nv - 1) { //the last row
				position[1] = pos;
				veloc[1] = vel;
			}
			else {
				position[1] = glm::vec3(nodes[i + 1][j][0], nodes[i + 1][j][1], nodes[i + 1][j][2]);
				veloc[1] = velocity[(i + 1)*nh + j];
			}
			//get the left node
			if (j == 0) { //the left most column
				position[2] = pos;
				veloc[2] = vel;
			}
			else {
				position[2] = glm::vec3(nodes[i][j - 1][0], nodes[i][j - 1][1], nodes[i][j - 1][2]);
				veloc[2] = velocity[i*nh + (j - 1)];
			}
			//get the right node
			if (j == nh - 1) { //the right most column
				position[3] = pos;
				veloc[3] = vel;
			}
			else {
				position[3] = glm::vec3(nodes[i][j + 1][0], nodes[i][j + 1][1], nodes[i][j + 1][2]);
				veloc[3] = velocity[i*nh + (j + 1)];
			}

			force[i*nh + j] = glm::vec3(0.0f, 0.0f, 0.0f);
			//add air-resistance
			if (i == nv - 1) { // the last row
				if (j == 0) { //the bottom left corner
					force[i*nh + j] = trian[(i - 1)*trian_col_num + 2 * j].force_aero / 3.0f;
				}
				if (j == nh - 1) { //the bottom right corner
					force[i*nh + j] = (trian[(i - 1)*trian_col_num + 2 * j - 2].force_aero
									 + trian[(i - 1)*trian_col_num + 2 * j - 1].force_aero) / 3.0f;
				}
				else {
					force[i*nh + j] = (trian[(i - 1)*trian_col_num + 2 * j - 2].force_aero
									+ trian[(i - 1)*trian_col_num + 2 * j - 1].force_aero \
									+ trian[(i - 1)*trian_col_num + 2 * j].force_aero) / 3.0f;
				}
			}
			else if (j == 0) { //the left column
				force[i*nh + j] = (trian[(i - 1)*trian_col_num + 2 * j].force_aero \
								+ trian[i*trian_col_num + 2 * j].force_aero \
								+ trian[i*trian_col_num + 2 * j + 1].force_aero) / 3.0f;
			}
			else if (j == nh - 1) { // the right column
				force[i*nh + j] = (trian[(i - 1)*trian_col_num + 2 * j-2].force_aero \
								+ trian[(i-1)*trian_col_num + 2 * j-1].force_aero \
								+ trian[i*trian_col_num + 2 * j - 1].force_aero) / 3.0f;
			}
			else {
				force[i*nh + j] = (trian[(i - 1)*trian_col_num + 2 * j - 2].force_aero \
								+ trian[(i - 1)*trian_col_num + 2 * j - 1].force_aero \
								+ trian[(i - 1)*trian_col_num + 2 * j].force_aero \
								+ trian[i*trian_col_num + 2 * j - 1].force_aero \
								+ trian[i*trian_col_num + 2 * j + 1].force_aero \
								+ trian[i*trian_col_num + 2 * j + 1].force_aero) / 3.0f;
			}

			#pragma omp parallel for private(dire,springforce)
			//compute springforce and dampforce
			for (int k = 0; k < 4; k++) {
				if ((i == nv - 1) && (k == 1)) { // the bottom row no down node
					continue;
				}
				if ((j == 0) && (k == 2)) { // the left column no left node
					continue;
				}
				if ((j == nh - 1) && (k == 3)) { // the right column no right node
					continue;
				}
				dire = position[k] - pos;
				springforce = ks * (sqrt(dire.x*dire.x + dire.y*dire.y + dire.z*dire.z) - restlen);
				dire = glm::normalize(dire);
				springF[k] = dire * springforce; //pointing to the nodes around to the current node
				dampF[k] = -kv * (vel - veloc[k]); //pointing nodes around to the current node
				force[i*nh + j] = force[i*nh + j] + springF[k] +dampF[k];
			}
		}
	}

	//update
	//#pragma omp parallel for private(pos,n,bounce)
	for (int i = 1; i < nv; i++) {
		for (int j = 0; j < nh; j++) {
			acceleration[i*nh + j] = gravity + force[i*nh + j] / mass;
			velocity[i*nh + j] = velocity[i*nh + j] + acceleration[i*nh + j] * dt;
			pos = glm::vec3(nodes[i][j][0], nodes[i][j][1], nodes[i][j][2]);
			if (Eulerian) {
				// Eulerian
				pos = pos + velocity[i*nh + j] * dt;
			}		
			else {
				//Midpoint
				glm::vec3 pos_half = pos + velocity[i*nh + j] * dt*0.5f;
				glm::vec3 vel_half = velocity[i*nh + j] + acceleration[i*nh + j] * dt*0.5f;
				pos =  pos + vel_half * dt;
			}

			// collision detect
			float d = glm::distance(pos, sphere_position);
			if (d < (radius + 0.09f)) {
				n = pos - sphere_position;
				n = glm::normalize(n);
				bounce = glm::dot(velocity[i*nh + j], n)*n;
				velocity[i*nh + j] = velocity[i*nh + j] - 1.2f*bounce;
				pos = pos + (0.11f + radius - d) *n;
				//printf("distance: %f\n", d);
			}			

			nodes[i][j][0] = pos.x;
			nodes[i][j][1] = pos.y;
			nodes[i][j][2] = pos.z;
		}
	}
}

void computePhysics(int i, float dt) {
	//glm::vec3 acceleration = glm::vec3(0.0f,0.0f,-10.0f);//-9.8;
	//velocity[i] = velocity[i] + acceleration * dt;
	particle_position[i] = particle_position[i] + particle_velocity[i] * dt;
	//printf("pos: (%.2f,%.2f,%.2f)\nvel: (%.2f,%.2f,%.2f)\n", position.x, position.y, position.z, velocity.x, velocity.y, velocity.z);
	if ((particle_position[i].z + particle_radius) > 3.0) {
		/*position[i].z = 3.0 + radius;
		velocity[i].z *= -.95;*/
		particle_position.erase(particle_position.begin() + i);
		particle_velocity.erase(particle_velocity.begin() + i);
		particle_color.erase(particle_color.begin() + i);
		particle_lifespan.erase(particle_lifespan.begin() + i);
	}
}