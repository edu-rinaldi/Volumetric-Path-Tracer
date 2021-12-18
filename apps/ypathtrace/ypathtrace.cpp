//
// LICENSE:
//
// Copyright (c) 2016 -- 2021 Fabio Pellacini
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//

#include <yocto/yocto_cli.h>
#include <yocto/yocto_math.h>
#include <yocto/yocto_parallel.h>
#include <yocto/yocto_scene.h>
#include <yocto/yocto_sceneio.h>
#include <yocto/yocto_shape.h>
#include <yocto_gui/yocto_glview.h>
#include <yocto_pathtrace/yocto_pathtrace.h>
using namespace yocto;

// render scene offline
void run_offline(const string& filename, const string& output,
    const pathtrace_params& params_) {
  // copy params
  auto params = params_;

  print_progress_begin("load scene");
  auto error = string{};
  auto scene = scene_data{};
  if (!load_scene(filename, scene, error)) print_fatal(error);
  print_progress_end();

  // camera
  // params.camera = find_camera(scene, params.camname);

  // tesselate subdivs
  print_progress_begin("tesselate surfaces");
  tesselate_surfaces(scene);
  print_progress_end();

  // build bvh
  print_progress_begin("build bvh");
  auto bvh = make_bvh(scene, params);
  print_progress_end();

  // build bvh
  print_progress_begin("init lights");
  auto lights = make_lights(scene, params);
  print_progress_end();

  // state
  print_progress_begin("init state");
  auto state = make_state(scene, params);
  print_progress_end();

  // render
  print_progress_begin("render image", params.samples);
  for (auto sample = 0; sample < params.samples; sample++) {
    pathtrace_samples(state, scene, bvh, lights, params);
    print_progress_next();
  }

  // save image
  print_progress_begin("save image");
  if (!save_image(output, get_render(state), error)) print_fatal(error);
  print_progress_end();
}

// render scene interactively
void run_interactive(const string& filename, const string& output,
    const pathtrace_params& params_) {
  // copy params
  auto params = params_;

  print_progress_begin("load scene");
  auto error = string{};
  auto scene = scene_data{};
  if (!load_scene(filename, scene, error)) print_fatal(error);
  print_progress_end();

  // camera
  // params.camera = find_camera(scene, params.camname);

  // tesselate subdivs
  print_progress_begin("tesselate subdivs");
  tesselate_surfaces(scene);
  print_progress_end();

  // build bvh
  print_progress_begin("build bvh");
  auto bvh = make_bvh(scene, params);
  print_progress_end();

  // build lights
  print_progress_begin("init lights");
  auto lights = make_lights(scene, params);
  print_progress_end();

  // init state
  print_progress_begin("init state");
  auto state   = make_state(scene, params);
  auto image   = make_image(state.width, state.height, true);
  auto display = make_image(state.width, state.height, false);
  auto render  = make_image(state.width, state.height, true);
  print_progress_end();

  // opengl image
  auto glimage  = glimage_state{};
  auto glparams = glimage_params{};

  // top level combo
  auto names    = vector<string>{filename};
  auto selected = 0;

  // camera names
  auto camera_names = scene.camera_names;

  // renderer update
  auto render_update  = std::atomic<bool>{};
  auto render_current = std::atomic<int>{};
  auto render_mutex   = std::mutex{};
  auto render_worker  = future<void>{};
  auto render_stop    = atomic<bool>{};
  auto reset_display  = [&]() {
    // stop render
    render_stop = true;
    if (render_worker.valid()) render_worker.get();

    state   = make_state(scene, params);
    image   = make_image(state.width, state.height, true);
    display = make_image(state.width, state.height, false);
    render  = make_image(state.width, state.height, true);

    render_worker = {};
    render_stop   = false;

    // preview
    auto pparams = params;
    pparams.resolution /= params.pratio;
    pparams.samples = 1;
    auto pstate     = make_state(scene, pparams);
    pathtrace_samples(pstate, scene, bvh, lights, pparams);
    auto preview = get_render(pstate);
    for (auto idx = 0; idx < state.width * state.height; idx++) {
      auto i = idx % render.width, j = idx / render.width;
      auto pi            = clamp(i / params.pratio, 0, preview.width - 1),
           pj            = clamp(j / params.pratio, 0, preview.height - 1);
      render.pixels[idx] = preview.pixels[pj * preview.width + pi];
    }
    // if (current > 0) return;
    {
      auto lock      = std::lock_guard{render_mutex};
      render_current = 0;
      image          = render;
      tonemap_image_mt(display, image, params.exposure, params.filmic);
      render_update = true;
    }

    // start renderer
    render_worker = std::async(std::launch::async, [&]() {
      for (auto sample = 0; sample < params.samples; sample += 1) {
        if (render_stop) return;
        pathtrace_samples(state, scene, bvh, lights, params);
        if (!render_stop) {
          auto lock      = std::lock_guard{render_mutex};
          render_current = state.samples;
          get_render(render, state);
          image = render;
          tonemap_image_mt(display, image, params.exposure, params.filmic);
          render_update = true;
        }
      }
    });
  };

  // stop render
  auto stop_render = [&]() {
    render_stop = true;
    if (render_worker.valid()) render_worker.get();
  };

  // start rendering
  reset_display();

  // callbacks
  auto callbacks    = glwindow_callbacks{};
  callbacks.init_cb = [&](const glinput_state& input) {
    auto lock = std::lock_guard{render_mutex};
    init_image(glimage);
    set_image(glimage, display);
  };
  callbacks.clear_cb = [&](const glinput_state& input) {
    clear_image(glimage);
  };
  callbacks.draw_cb = [&](const glinput_state& input) {
    // update image
    if (render_update) {
      auto lock = std::lock_guard{render_mutex};
      set_image(glimage, display);
      render_update = false;
    }
    glparams.window                           = input.window_size;
    glparams.framebuffer                      = input.framebuffer_viewport;
    std::tie(glparams.center, glparams.scale) = camera_imview(glparams.center,
        glparams.scale, {image.width, image.height}, glparams.window,
        glparams.fit);
    draw_image(glimage, glparams);
  };
  callbacks.widgets_cb = [&](const glinput_state& input) {
    auto edited = 0;
    draw_glcombobox("name", selected, names);
    auto current = (int)render_current;
    draw_glprogressbar("sample", current, params.samples);
    if (begin_glheader("render")) {
      auto edited  = 0;
      auto tparams = params;
      edited += draw_glcombobox("camera", tparams.camera, camera_names);
      edited += draw_glslider("resolution", tparams.resolution, 180, 4096);
      edited += draw_glslider("samples", tparams.samples, 16, 4096);
      edited += draw_glcombobox(
          "shader", (int&)tparams.shader, pathtrace_shader_names);
      edited += draw_glslider("bounces", tparams.bounces, 1, 128);
      continue_glline();
      edited += draw_glslider("pratio", tparams.pratio, 1, 64);
      end_glheader();
      if (edited) {
        stop_render();
        params = tparams;
        reset_display();
      }
    }
    if (begin_glheader("tonemap")) {
      edited += draw_glslider("exposure", params.exposure, -5, 5);
      edited += draw_glcheckbox("filmic", params.filmic);
      end_glheader();
      if (edited) {
        tonemap_image_mt(display, image, params.exposure, params.filmic);
        set_image(glimage, display);
      }
    }
  };
  callbacks.uiupdate_cb = [&](const glinput_state& input) {
    auto edited = false;
    auto camera = scene.cameras[params.camera];
    if (input.mouse_left && input.modifier_alt && !input.widgets_active) {
      auto dolly  = 0.0f;
      auto pan    = zero2f;
      auto rotate = zero2f;
      if (input.modifier_shift) {
        pan   = (input.mouse_pos - input.mouse_last) * camera.focus / 200.0f;
        pan.x = -pan.x;
      } else if (input.modifier_ctrl) {
        dolly = (input.mouse_pos.y - input.mouse_last.y) / 100.0f;
      } else {
        rotate = (input.mouse_pos - input.mouse_last) / 100.0f;
      }
      auto [frame, focus] = camera_turntable(
          camera.frame, camera.focus, rotate, dolly, pan);
      if (camera.frame != frame || camera.focus != focus) {
        camera.frame = frame;
        camera.focus = focus;
        edited       = true;
      }
    }
    if (edited) {
      stop_render();
      scene.cameras[params.camera] = camera;
      reset_display();
    }
  };

  // run ui
  run_ui({1280 + 320, 720}, "yraytrace", callbacks);

  // done
  stop_render();
}

// Run
void run(const vector<string>& args) {
  // command line parameters
  auto params      = pathtrace_params{};
  auto filename    = "scene.json"s;
  auto output      = "image.png"s;
  auto interactive = false;

  // command line parsing
  auto cli = make_cli("ypathtrace", "Raytrace scenes.");
  add_option(cli, "scene", filename, "Scene filename.");
  add_option(cli, "output", output, "Output filename.");
  add_option(cli, "interactive", interactive, "Run interactively.");
  add_option(
      cli, "resolution", params.resolution, "Image resolution.", {1, 4096});
  add_option(
      cli, "shader", params.shader, "Shader type.", pathtrace_shader_names);
  add_option(cli, "samples", params.samples, "Number of samples.", {1, 4096});
  add_option(cli, "bounces", params.bounces, "Number of bounces.", {1, 128});
  add_option(cli, "noparallel", params.noparallel, "Disable threading.");
  parse_cli(cli, args);

  // run
  if (!interactive) {
    run_offline(filename, output, params);
  } else {
    run_interactive(filename, output, params);
  }
}

int main(int argc, const char* argv[]) {
  handle_errors(run, make_cli_args(argc, argv));
}
