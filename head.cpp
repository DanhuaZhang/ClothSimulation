#include "head.h"

bool DEBUG_ON = true;

float randf() {
	return (float)(rand() % 1001) * 0.001f;
}

bool IsInHemisphere(glm::vec3 point, glm::vec3 center, float radius) {
	if (((point.x - center.x)*(point.x - center.x) + (point.y - center.y)*(point.y - center.y) + (point.z - center.z)*(point.z - center.z) < (radius*radius)) && (point.z < radius)) {
		return true;
	}
	else {
		return false;
	}
}

bool IsUnderCone(glm::vec3 point, float radius, float height) {
	if ((point.x*point.x + point.y*point.y) < ((radius*radius / height / height)*(point.z - height)*(point.z - height))\
		&& (point.z<height)) {
		return true;
	}
	else {
		return false;
	}
}

// Create a NULL-terminated string by reading the provided file
static char* readShaderSource(const char* shaderFile) {
	FILE *fp;
	long length;
	char *buffer;

	// open the file containing the text of the shader code
	fp = fopen(shaderFile, "r");

	// check for errors in opening the file
	if (fp == NULL) {
		printf("can't open shader source file %s\n", shaderFile);
		return NULL;
	}

	// determine the file size
	fseek(fp, 0, SEEK_END); // move position indicator to the end of the file;
	length = ftell(fp);  // return the value of the current position

						 // allocate a buffer with the indicated number of bytes, plus one
	buffer = new char[length + 1];

	// read the appropriate number of bytes from the file
	fseek(fp, 0, SEEK_SET);  // move position indicator to the start of the file
	fread(buffer, 1, length, fp); // read all of the bytes

								  // append a NULL character to indicate the end of the string
	buffer[length] = '\0';

	// close the file
	fclose(fp);

	// return the string
	return buffer;
}

// Create a GLSL program object from vertex and fragment shader files
GLuint InitShader(const char* vShaderFileName, const char* fShaderFileName) {
	GLuint vertex_shader, fragment_shader;
	GLchar *vs_text, *fs_text;
	GLuint program;

	// check GLSL version
	printf("GLSL version: %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Create shader handlers
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	// Read source code from shader files
	vs_text = readShaderSource(vShaderFileName);
	fs_text = readShaderSource(fShaderFileName);

	// error check
	if (vs_text == NULL) {
		printf("Failed to read from vertex shader file %s\n", vShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("Vertex Shader:\n=====================\n");
		printf("%s\n", vs_text);
		printf("=====================\n\n");
	}
	if (fs_text == NULL) {
		printf("Failed to read from fragent shader file %s\n", fShaderFileName);
		exit(1);
	}
	else if (DEBUG_ON) {
		printf("\nFragment Shader:\n=====================\n");
		printf("%s\n", fs_text);
		printf("=====================\n\n");
	}

	// Load Vertex Shader
	const char *vv = vs_text;
	glShaderSource(vertex_shader, 1, &vv, NULL);  //Read source
	glCompileShader(vertex_shader); // Compile shaders

									// Check for errors
	GLint  compiled;
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled) {
		printf("Vertex shader failed to compile:\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(vertex_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Load Fragment Shader
	const char *ff = fs_text;
	glShaderSource(fragment_shader, 1, &ff, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &compiled);

	//Check for Errors
	if (!compiled) {
		printf("Fragment shader failed to compile\n");
		if (DEBUG_ON) {
			GLint logMaxSize, logLength;
			glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &logMaxSize);
			printf("printing error message of %d bytes\n", logMaxSize);
			char* logMsg = new char[logMaxSize];
			glGetShaderInfoLog(fragment_shader, logMaxSize, &logLength, logMsg);
			printf("%d bytes retrieved\n", logLength);
			printf("error message: %s\n", logMsg);
			delete[] logMsg;
		}
		exit(1);
	}

	// Create the program
	program = glCreateProgram();

	// Attach shaders to program
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);

	// Link and set program to use
	glLinkProgram(program);

	return program;
}

void Win2PPM(int width, int height) {
	char outdir[10] = "out/"; //Must be defined!
	int i, j;
	FILE* fptr;
	static int counter = 0;
	char fname[32];
	unsigned char *image;

	/* Allocate our buffer for the image */
	image = (unsigned char *)malloc(3 * width*height * sizeof(char));
	if (image == NULL) {
		fprintf(stderr, "ERROR: Failed to allocate memory for image\n");
	}

	/* Open the file */
	sprintf(fname, "%simage_%04d.ppm", outdir, counter);
	if ((fptr = fopen(fname, "w")) == NULL) {
		fprintf(stderr, "ERROR: Failed to open file for window capture\n");
	}

	/* Copy the image into our buffer */
	glReadBuffer(GL_BACK);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, image);

	/* Write the PPM file */
	fprintf(fptr, "P6\n%d %d\n255\n", width, height);
	for (j = height - 1; j >= 0; j--) {
		for (i = 0; i < width; i++) {
			fputc(image[3 * j*width + 3 * i + 0], fptr);
			fputc(image[3 * j*width + 3 * i + 1], fptr);
			fputc(image[3 * j*width + 3 * i + 2], fptr);
		}
	}

	free(image);
	fclose(fptr);
	counter++;
}