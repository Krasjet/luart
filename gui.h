#pragma once

#include <GLFW/glfw3.h>
#include <string>

extern GLFWwindow *win;

void glfw_init();
void glfw_finish();
int done();

void gl_init();

void imgui_init();
void imgui_finish();

template<typename T> struct RingBuf;
void plot_rb(std::string name, RingBuf<float> &rb, float height = 120);
