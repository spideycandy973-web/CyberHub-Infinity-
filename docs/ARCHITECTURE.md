# Architecture

Folders under app/src/main/cpp: core, ui, gfx, platform. Later: world, physics, audio, game.

Shaders: app/src/main/shaders (GLSL), compiled to SPIR-V by CMake.
Host tests: tests/ (GoogleTest).

Frame (M0): poll events, acquire image, record clear + text, submit, present.
