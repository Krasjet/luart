/* gui.cc: gui component of luart */
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <GLFW/glfw3.h>
#include "ringbuf.h"
#include <string>

constexpr int width = 768, height = 512;
GLFWwindow *win = NULL;

static void
resize(GLFWwindow *win, int width, int height)
{
  glViewport(0, 0, width, height);
}

void glfw_init()
{
  glfwInit();

  glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
  win = glfwCreateWindow(width, height, "luart", NULL, NULL);

  glfwMakeContextCurrent(win);
  glfwSetFramebufferSizeCallback(win, resize);
}

void glfw_finish()
{
  glfwTerminate();
}

int done()
{
  glfwPollEvents();
  return glfwWindowShouldClose(win);
}

void gl_init()
{
  int w, h;
  glfwGetFramebufferSize(win, &w, &h);
  resize(win, w, h);
}

void imgui_init()
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImPlot::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = ".imgui.ini";
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  ImGui::StyleColorsLight();

  ImGui_ImplGlfw_InitForOpenGL(win, true);
  ImGui_ImplOpenGL2_Init();
}

void imgui_finish()
{
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImPlot::DestroyContext();
  ImGui::DestroyContext();
}

void
plot_rb(std::string name, RingBuf<float> &rb, float height)
{
  if (ImPlot::BeginPlot(("##"+name).c_str(), ImVec2(ImGui::GetWindowContentRegionWidth(), height))) {
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickLabels, 0);
    ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, (double)(rb.size-1), ImGuiCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -1.0, 1.0, ImGuiCond_Once);
    ImPlot::PlotLine(name.c_str(), rb.buf.data(), (int)rb.size, 1.0, 0.0, 0, (int)rb.wp);
    ImPlot::EndPlot();
  }
}
