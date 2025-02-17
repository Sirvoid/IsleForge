![Image](https://i.imgur.com/4Ku3xak.png)
[![Chat](https://img.shields.io/discord/908871478576033832?label=%20chat%20on%20discord)](https://discord.gg/tZthSbpUcV)

Midless is a free and open-source voxel game made in C.

## Controls

| Input                        | Action                |
|-------------------------------|----------------------|
| W A S D             | Move                           |
| Space               | Jump                           |
| F                   | Toggle Flight                  |
| Left Click          | Break block                    |
| Right Click         | Place block                    |
| Mouse wheel         | Block Selection                |
| T                   | Open Chat                      |
| ESC                 | Open menu                      |

## Dependencies

| Dependency    | Version | Type      | Used By|
|---------------|---------|-----------|--------|
| [Raylib](https://github.com/raysan5/raylib/)        | [4.5](https://github.com/raysan5/raylib/releases/tag/4.5.0)     | Single-File | Client / Server
| [Raygui](https://github.com/raysan5/raygui/)        | [3.5](https://github.com/raysan5/raygui/releases/tag/3.5)     | Single-File | Client
| [Zpl-c/ENet](https://github.com/zpl-c/enet)    | [2.3.6](https://github.com/zpl-c/enet/releases/tag/v2.3.6)   | Single-File | Client / Server
| [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) | -       | Single-File | Client / Server
| [stb_ds](https://github.com/nothings/stb/blob/master/stb_ds.h) | -       | Single-File | Client / Server
| [MiniLua](https://github.com/edubart/minilua) | -       | Single-File | Server
| For Optional Server's Websocket Support:
| [mongoose](https://github.com/cesanta/mongoose/) | [7.8](https://github.com/cesanta/mongoose/releases/tag/7.8)       | Single-Files (.c, .h) | Server
| [OpenSSL](https://github.com/openssl/openssl) | -       | Linked | Server


## Compiling for Windows using MinGW

1. [Download and Build Raylib](https://github.com/raysan5/raylib/wiki/Working-on-Windows)
2. Place single-files dependencies inside /libs
4. Edit the makefile's properties if needed
3. Run mingw32-make inside the Midless folder where the MakeFile is located. 

Make arguments:
```
BUILD_SERVER=TRUE       - Build Midless Server (Doesn't build the client)
SERVER_HEADLESS=TRUE    - Compile server without graphics
SERVER_WEB_SUPPORT=TRUE - Compile server with websocket support

DEBUG=TRUE              - Debug build

PLATFORM=PLATFORM_WEB   - Build for the web (Client only)
```


## License

All code in this repository is licensed under the [MIT License](https://github.com/Sirvoid/Midless/blob/main/LICENSE).