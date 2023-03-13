/* main.cc: entry point for luart */
#include <atomic>
#include <imgui.h>
#include <implot.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>
#include <imgui_stdlib.h>
#include <GLFW/glfw3.h>
#include <jack/jack.h>
#include <lua.hpp>

#include "gui.h"
#include "ringbuf.h"

/* jack */
static jack_client_t *client = NULL;
static jack_port_t *port_in = NULL, *port_out = NULL;
static float sr;

/* plotting */
static RingBuf<float> plotbuf_in(48000*2);
static RingBuf<float> plotbuf_out(48000*2);

/* lua */
static lua_State *L = NULL;
static std::string lua_src_edit =
"if phs == nil then phs = 0 end\n"
"\n"
"for i=1,nframes do\n"
"  y[i] = 0.5*x[i] + 0.5*math.sin(2*math.pi*phs)\n"
"  phs = phs + 220/sr\n"
"  while phs > 1 do\n"
"    phs = phs - 1\n"
"  end\n"
"end\n"
"\n"
"print('phs: ' .. phs)";
static std::string lua_err = "";
static std::string lua_msg = "";
/* for audio thread */
#define luaL_dobuf(L, buf, n) \
  (luaL_loadbuffer(L, buf, n, "bc") || lua_pcall(L, 0, LUA_MULTRET, 0))
static std::atomic<bool> recompile = true;
static bool lua_compile_ok = false;
static std::string lua_src_run = lua_src_edit;
static std::string lua_bc;

/* writer for bytecode compilation */
static int
bc_writer(lua_State *L, const void *b, size_t size, void *ud)
{
  lua_bc.append((const char *)b, size);
  return 0;
}

static int
on_process(jack_nframes_t nframes, void *arg)
{
  float *out;
  const float *in;
  jack_nframes_t i;

  in = (float *)jack_port_get_buffer(port_in, nframes);
  out = (float *)jack_port_get_buffer(port_out, nframes);

  /* 1. for input plot */
  plotbuf_in.push(in, nframes);

  /* 2. recompile if source updated */
  if (recompile) {
    if (luaL_loadstring(L, lua_src_run.data()) == 0) {
      /* clear error */
      lua_err.clear();
      /* dump compiled bytecode */
      lua_bc.clear();
      lua_dump(L, bc_writer, NULL);
      lua_pop(L, 1);
      lua_compile_ok = true;
    } else {
      /* show error */
      lua_err = lua_tostring(L, -1);
      lua_pop(L, 1);
      lua_compile_ok = false;
    }
    recompile = false;
  }

  if (!lua_compile_ok)
    goto err;

  /* 3. update nframes */
  lua_pushinteger(L, nframes);
  lua_setglobal(L, "nframes");
  /* 4. copy input buffer to x */
  lua_getglobal(L, "x");
  for (i = 0; i < nframes; i++) {
    lua_pushnumber(L, in[i]);
    lua_rawseti(L, -2, i+1);
  }
  lua_pop(L, 1);

  /* 5. run lua */
  lua_msg.clear();
  if (luaL_dobuf(L, lua_bc.data(), lua_bc.size()) != 0) {
    /* show error */
    lua_err = lua_tostring(L, -1);
    lua_pop(L, 1);
    goto err;
  }

  /* 6. copy y to output buffer */
  lua_getglobal(L, "y");
  for (i = 0; i < nframes; i++) {
    lua_rawgeti(L, -1, i+1);
    out[i] = lua_tonumber(L, -1);
    lua_pop(L, 1);
  }
  lua_pop(L, 1);

  /* 7. for output plot */
  plotbuf_out.push(out, nframes);

  return 0;
err:
  /* set output to 0 */
  memset(out, 0, nframes * sizeof(float));
  plotbuf_out.push(out, nframes);
  return 0;
}

static void
imgui_draw()
{
  ImGuiID dock = ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::SetNextWindowDockID(dock, ImGuiCond_FirstUseEver);
  ImGui::Begin("main", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    plot_rb("in", plotbuf_in);
    plot_rb("out", plotbuf_out);

    if (ImGui::Button("recompile")) {
      lua_src_run = lua_src_edit;
      recompile = true; /* not safe, but works for prototype. use ringbuf in production */
    }
    ImGui::InputTextMultiline("##lua_src", &lua_src_edit,
                              ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetTextLineHeight()*12),
                              ImGuiInputTextFlags_AllowTabInput);
    if (lua_err.size() > 0)
      ImGui::TextWrapped("%s", lua_err.data());
    ImGui::TextWrapped("%s", lua_msg.data());
  ImGui::End();
}

/* chore below */

static void
jack_init()
{
  client = jack_client_open("luart", JackNoStartServer, NULL);
  sr = jack_get_sample_rate(client);
  jack_set_process_callback(client, on_process, NULL);
  port_in = jack_port_register(client, "in", JACK_DEFAULT_AUDIO_TYPE,
                               JackPortIsInput, 0);
  port_out = jack_port_register(client, "out", JACK_DEFAULT_AUDIO_TYPE,
                                JackPortIsOutput, 0);
}

static void
jack_finish()
{
  jack_deactivate(client);
  jack_client_close(client);
}

/* overwrite print function in lua (modifed from lua5.1 source) */
static int
lua_print_imgui(lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, "'tostring' must return a string to 'print'");
    if (i>1) lua_msg.push_back('\t');
    lua_msg.append(s);
    lua_pop(L, 1);  /* pop result */
  }
  lua_msg.push_back('\n');
  return 0;
}

static void
lua_init()
{
  size_t i;
  size_t nframes = jack_get_buffer_size(client);

  L = luaL_newstate();
  luaL_openlibs(L);
  /* jit off by default (doesn't benefit much for smaller scripts) */
  luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE|LUAJIT_MODE_OFF);

  /* overwrite print function */
  lua_pushcfunction(L, lua_print_imgui);
  lua_setglobal(L, "print");

  /* init input array (x) */
  lua_createtable(L, nframes, 0);
  for (i = 1; i <= nframes; i++) {
    lua_pushnumber(L, 0);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "x");

  /* init output array (y) */
  lua_createtable(L, nframes, 0);
  for (i = 1; i <= nframes; i++) {
    lua_pushnumber(L, 0);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "y");

  /* make sr a global variable */
  lua_pushnumber(L, sr);
  lua_setglobal(L, "sr");
}

static void
redraw()
{
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  imgui_draw();

  ImGui::Render();

  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

  glfwMakeContextCurrent(win);
  glfwSwapBuffers(win);
}

int
main()
{
  jack_init();
  lua_init();
  glfw_init();
  gl_init();
  imgui_init();

  jack_activate(client);

  while (!done())
    redraw();

  imgui_finish();
  glfw_finish();
  jack_finish();
  lua_close(L);
  return 0;
}
