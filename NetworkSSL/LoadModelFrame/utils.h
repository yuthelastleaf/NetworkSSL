#pragma once

struct GLFWwindow;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void printUsage();

extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;
