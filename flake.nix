{
  description = "A Modern Cross-Platform Low-Level 3D Graphics Library and Rendering Framework";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    self.submodules = true;
  };

  outputs =
    { self, nixpkgs, ... }:
    let
      forAllSystems =
        function:
        nixpkgs.lib.genAttrs [
          "x86_64-linux"
          "aarch64-linux"
          "x86_64-darwin"
          "aarch64-darwin"
        ] (system: function nixpkgs.legacyPackages.${system});
    in
    {
      packages = forAllSystems (pkgs: {
        default = pkgs.clangStdenv.mkDerivation {
          pname = "diligent";
          version = "2.5.6";
          src = ./.;

          cmakeFlags = [
            "-DDILIGENT_BUILD_SAMPLES=OFF"
            "-DILIGENT_NO_FORMAT_VALIDATION=ON"
          ];

          nativeBuildInputs = [
            pkgs.cmake
            pkgs.autoPatchelfHook
          ];

          patches = [
            (pkgs.writeText "libs.patch" ''
              diff --git a/DiligentTools/RenderStateNotation/CMakeLists.txt b/DiligentTools/RenderStateNotation/CMakeLists.txt
              index 726e376..492d2e3 100644
              --- a/DiligentTools/RenderStateNotation/CMakeLists.txt
              +++ b/DiligentTools/RenderStateNotation/CMakeLists.txt

              @@ -26,26 +26,6 @@ file(COPY ../.clang-format DESTINATION "''${RSN_PARSER_GENERATED_HEADERS_DIR}")

               find_package(Python3 REQUIRED)

              -set(LIBCLANG_INSTALL_CMD ''${Python3_EXECUTABLE} -m pip install libclang==16.0.6)
              -set(JINJA2_INSTALL_CMD ''${Python3_EXECUTABLE} -m pip install jinja2)
              -
              -if(''${Python3_VERSION} VERSION_GREATER_EQUAL "3.11")
              -    set(LIBCLANG_INSTALL_CMD ''${LIBCLANG_INSTALL_CMD} --break-system-packages)
              -    set(JINJA2_INSTALL_CMD ''${JINJA2_INSTALL_CMD} --break-system-packages)
              -endif()
              -
              -execute_process(COMMAND ''${LIBCLANG_INSTALL_CMD}
              -                RESULT_VARIABLE PYTHON_PIP_LIBCLANG_RESULT)
              -if(NOT PYTHON_PIP_LIBCLANG_RESULT EQUAL "0")
              -    message(FATAL_ERROR "Command ''\'''${LIBCLANG_INSTALL_CMD}' failed with error code ''${PYTHON_PIP_LIBCLANG_RESULT}")
              -endif()
              -
              -execute_process(COMMAND ''${JINJA2_INSTALL_CMD}
              -                RESULT_VARIABLE PYTHON_PIP_JINJIA_RESULT)
              -if(NOT PYTHON_PIP_JINJIA_RESULT EQUAL "0")
              -    message(FATAL_ERROR "Command ''\'''${JINJA2_INSTALL_CMD}' failed with error code ''${PYTHON_PIP_JINJIA_RESULT}")
              -endif()
              -
               file(GLOB INCLUDE    include/*)
               file(GLOB INTERFACE  interface/*)
               file(GLOB SOURCE     src/*)
            '')
          ];

          buildInputs = [
            pkgs.python313
            pkgs.xorg.libX11
            pkgs.libGL
            pkgs.xorg.libXrandr
            pkgs.xorg.libXinerama
            pkgs.xorg.libXcursor
            pkgs.xorg.libXi
            pkgs.libxcb
            pkgs.python313Packages.libclang
            pkgs.python313Packages.jinja2

            pkgs.glslang
            pkgs.vulkan-headers
            pkgs.vulkan-loader
            pkgs.vulkan-validation-layers
          ];

          meta = {
            description = "A Modern Cross-Platform Low-Level 3D Graphics Library and Rendering Framework";
            longDescription = "Diligent Engine is a lightweight, high-performance graphics API abstraction layer and rendering framework for cross-platform development. It leverages the power of modern graphics APIs such as Direct3D12, Vulkan, Metal, and WebGPU, while also maintaining robust support for legacy platforms through Direct3D11, OpenGL, OpenGLES, and WebGL. Providing a consistent front-end API, Diligent Engine uses HLSL as its universal shading language and also supports platform-specific shader formats (GLSL, MSL, DirectX bytecode, SPIR-V) for optimized performance. Ideal for game engines, interactive simulations, and 3D visualization applications, Diligent Engine is open-source and distributed under the permissive Apache 2.0 license.";
            homepage = "https://diligentgraphics.com/diligent-engine/";
            license = pkgs.lib.licenses.asl20;
            platforms = pkgs.lib.platforms.all;

            maintainers = with pkgs.lib.maintainers; [
              TheMostDiligent
            ];
          };

          LD_LIBRARY_PATH = "${pkgs.vulkan-loader}/lib";
        };
      });

      devShells = forAllSystems (pkgs: {
        default = pkgs.mkShell {
          packages = [
            self.packages.${pkgs.system}.default
          ];
        };
      });
    };
}
