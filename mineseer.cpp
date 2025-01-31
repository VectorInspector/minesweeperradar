#define SDL_MAIN_HANDLED

#include <lodepng.h>
#include <iostream>
#include <windows.h>
// #include <algorithm>
#include <sdl2/sdl.h>
#include <string>
#include <random>
#include <unordered_map>

// Struct declarations.
struct PngFile;
struct AppData;
struct AppLoop;
struct StateData;
struct PlayFuncs;
struct InputEvents;
struct SdlTexture;
struct SdlDrawFuncs;
struct WinMineInfo;
class AppState;
class AppSeeState;

int main (int argc, char* argv []);

// Data and function declarations.
struct WinMineInfo {
	// Finding minesweeper! (Windows-dependent code)
	static constexpr auto x_size_offset	= 0x01005334;	// Found using a debugger running WINMINE.exe
	static constexpr auto y_size_offset	= 0x01005338;	// it's ok, these never change because
	static constexpr auto grid_offset		= 0x01005361;	// minesweeper runs mostly on stack memory.
	static constexpr auto empty_tile	= 0x0f;
	static constexpr auto clicked_tile	= 0x40;
	static constexpr auto bomb_tile		= 0x8f;
	static constexpr auto row_size		= 32;
	static constexpr auto max_x_size	= 32;
	static constexpr auto max_y_size	= 24;
	static constexpr auto buff_size		= max_x_size * max_y_size;
	
	HWND		window;
	HANDLE		process;
	DWORD		process_id;
	
	int				x_size;
	int				y_size;
	unsigned char	game [buff_size];
	
	int cursor_x;
	int cursor_y;
	int cursor_x_tile;
	int cursor_y_tile;
	
	int active_timer;
	
	void Tick () {
		
		// Just reset / lost process
		if(active_timer < 0) {
			active_timer = 0;
		}
		
		// Look for process
		if(0 == active_timer) {
			window = FindWindow(nullptr, "Minesweeper");
			
			if(window) {
				GetWindowThreadProcessId(window, &process_id);
				process = OpenProcess(
					PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					false,
					process_id);
				
				// Process found.
				if(process) {
					active_timer = 1;
				}
			}
			
			cursor_x = 0;
			cursor_y = 0;
			cursor_x_tile = 0;
			cursor_y_tile = 0;
		}
		
		// As long as the active timer is positive, memory reads are valid.
		// Check if the process still exists.
		if(0 < active_timer) {
			DWORD exit_code = 0;
			GetExitCodeProcess(process, &exit_code);
			
			if(STILL_ACTIVE != exit_code) {
				active_timer = -1;
			}
		}
		
		// Read the grid.
		if(0 < active_timer) {
			
			// Memory
			ReadProcessMemory(process, (void*)x_size_offset, &x_size, sizeof(x_size), nullptr);
			ReadProcessMemory(process, (void*)y_size_offset, &y_size, sizeof(y_size), nullptr);
			ReadProcessMemory(process, (void*)grid_offset, &game, buff_size, nullptr);
			
			// Cursor position
			POINT cursor_pos;
			GetCursorPos(&cursor_pos);
			ScreenToClient(window, &cursor_pos);
			
			// Subtracting the borders of the game window.
			cursor_x = cursor_pos.x - 12;
			cursor_y = cursor_pos.y - 55;
			
			cursor_x_tile = 1 + cursor_x / 16;
			cursor_y_tile = 1 + cursor_y / 16;
			
			active_timer++;
		}
	}
};

struct SdlDrawFuncs {
	SdlDrawFuncs (AppData& app_data);
	
	void Make ();
	void DrawTile (int x_tile, int y_tile, int x, int y);
	void DrawText (std::string text, int x, int y);
	void FontChara (char c);
	
	int font_x_tile;
	int font_y_tile;
	
	int text_x;
	int text_y;
	int cursor_x;
	int cursor_y;
	
	AppData& a;
};

struct SdlTexture {
	SdlTexture ();
	~SdlTexture ();
	bool LoadFromPng (SDL_Renderer* renderer, std::string path);
	
	SDL_Surface* surface;
	SDL_Texture* texture;
	int x_size;
	int y_size;
};

struct PngFile {
	PngFile (std::string path);
	~PngFile ();
	
	unsigned char* data;
	unsigned x_size;
	unsigned y_size;
};

struct InputEvents {
	void Tick (std::string key, bool cond);
	bool IsPressed (std::string key);
	bool IsReleased (std::string key);
	bool IsDown (std::string key);
	bool IsUp (std::string key);
	
	std::unordered_map <std::string, int> timers;
};

struct PlayFuncs {
	PlayFuncs (AppData& app_data, StateData& state_data);
	std::string DirName (int dir) const;
	int Random (int u, int v);
	
	int new_dir;
	int new_x;
	int new_y;
	int x_offset;
	int y_offset;
	
	AppData& a;
	StateData& d;
};

class AppState {
public:
	AppState (AppData& app_data, StateData& state_data);
	virtual void OnFirstTick () {};
	virtual void OnTick () {};
	virtual void OnLastTick () {};
	virtual void OnDraw () {};
	
	AppData& a;
	StateData& d;
};

class AppSeeState : public AppState {
public:
	using AppState::AppState;
	virtual void OnFirstTick () override;
	virtual void OnTick () override;
	virtual void OnDraw () override;
};

struct StateData {
	StateData (AppData& app_data);
	
	int tile_size;
	int timer;
	
	PlayFuncs play_funcs;
};

struct AppLoop {
	AppLoop (AppData& app_data);
	
	int MainLoop ();
	void OnTick ();
	void OnDraw ();
	
	AppData& a;
};

struct AppData {
	AppData ();
	~AppData ();
	
	void DefaultSettings ();
	
	AppState* current_state;

	InputEvents inputs;
	int window_x_size;
	int window_y_size;
	int canvas_x_size;
	int canvas_y_size;
	int exit_clicked;
	std::mt19937 random;
	WinMineInfo mine_info;
	
	int cmd_arg_count;
	char** cmd_args;
	
	SDL_Window*					window;
	SDL_Renderer*				renderer;
	SDL_Event					event;
	SDL_Texture*				target;
	SdlTexture					sprite;
	SdlDrawFuncs				draw_funcs;
	const unsigned char*		keyboard;
};

// Functions go here.
SdlDrawFuncs::SdlDrawFuncs (AppData& app_data) : a(app_data) {
}

void SdlDrawFuncs::Make () {
}

void SdlDrawFuncs::DrawTile (int x_tile, int y_tile, int x, int y) {
	auto tile_size = 8;
	y_tile += 4;
	
	SDL_Rect src { x_tile * tile_size, y_tile * tile_size, tile_size, tile_size };
	SDL_Rect dest { x * tile_size, y * tile_size, tile_size, tile_size };
	SDL_RenderCopy(a.renderer, a.sprite.texture, &src, &dest);
}

void SdlDrawFuncs::DrawText (std::string text, int x, int y) {
	auto font_tile_size = 7;
	
	SDL_Rect src { 0, 0, font_tile_size, font_tile_size };
	SDL_Rect dest { x, y, font_tile_size, font_tile_size };
	auto& texture = a.sprite;
	
	for(auto c: text) {
		FontChara(c);
		src.x = font_tile_size * font_x_tile;
		src.y = font_tile_size * font_y_tile;
		SDL_RenderCopy(a.renderer, texture.texture, &src, &dest);
		dest.x += font_tile_size;
	}
}

void SdlDrawFuncs::FontChara (char c) {
	if('A' <= c && c <= 'Z') {
		font_x_tile = c - 'A';
		font_y_tile = 0;
	}
	
	else if('a' <= c && c <= 'z') {
		font_x_tile = c - 'a';
		font_y_tile = 1;
	}
	
	else if('0' <= c && c <= '9') {
		font_x_tile = c - '0';
		font_y_tile = 2;
	}
	
	else {
		font_y_tile = 2;
		switch(c) {
		case ' ':
			font_x_tile = 26;
			font_y_tile = 0;
			break;
		case '-': font_x_tile = 10; break;
		case '+': font_x_tile = 11; break;
		case '.': font_x_tile = 12; break;
		case ',': font_x_tile = 13; break;
		case '/': font_x_tile = 14; break;
		case '\\': font_x_tile = 15; break;
		case '(': font_x_tile = 16; break;
		case ')': font_x_tile = 17; break;
		case '[': font_x_tile = 18; break;
		case ']': font_x_tile = 19; break;
		case '<': font_x_tile = 20; break;
		case '>': font_x_tile = 21; break;
		case '*': font_x_tile = 22; break;
		case '_': font_x_tile = 23; break;
		case '!': font_x_tile = 24; break;
		case '?': font_x_tile = 25; break;
		case '\'': font_x_tile = 26; break;
		case '&': font_x_tile = 27; break;
		case '%': font_x_tile = 28; break;
		case '{': font_x_tile = 29; break;
		case '}': font_x_tile = 30; break;
		case ':': font_x_tile = 31; break;
		}
	}
}

PngFile::PngFile (std::string path) {
	data = nullptr;
	x_size = 0;
	y_size = 0;
	
	lodepng_decode32_file(&data, &x_size, &y_size, path.c_str());
}

PngFile::~PngFile () {
	if(data) {
		delete [] data;
	}
}

StateData::StateData (AppData& app_data) : play_funcs(app_data, *this) {
	timer = 0;
}

AppState::AppState (AppData& app_data, StateData& state_data) : a(app_data), d(state_data) {
}

void InputEvents::Tick (std::string key, bool cond) {
	if(cond) {
		if(timers [key] <= 0) {
			timers [key] = 1;
		}
		
		else {
			timers [key]++;
		}
	}
	
	else {
		if(0 < timers [key]) {
			timers [key] = 0;
		}
		
		else {
			timers [key]--;
		}
	}
}

bool InputEvents::IsPressed (std::string key) {
	return 1 == timers [key];
}

bool InputEvents::IsReleased (std::string key) {
	return 0 == timers [key];
}

bool InputEvents::IsDown (std::string key)  {
	return 0 < timers [key];
}

bool InputEvents::IsUp (std::string key) {
	return timers [key] <= 0;
}

PlayFuncs::PlayFuncs (AppData& app_data, StateData& state_data) : a(app_data), d(state_data) {
}

int PlayFuncs::Random (int u, int v) {
	return a.random() % (v - u) + u;
}

SdlTexture::SdlTexture () {
	texture		= nullptr;
	surface		= nullptr;
	x_size		= 0;
	y_size		= 0;
}

SdlTexture::~SdlTexture () {
	if(texture) {
		SDL_DestroyTexture(texture);
	}
	
	if(surface) {
		SDL_FreeSurface(surface);
	}
}

bool SdlTexture::LoadFromPng (SDL_Renderer* renderer, std::string path) {
	PngFile png(path);
	surface = SDL_CreateRGBSurfaceFrom(
		png.data,
		png.x_size,
		png.y_size,
		32,
		4 * png.x_size,
		0x000000ff,
		0x0000ff00,
		0x00ff0000,
		0xff000000);
	
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	x_size = png.x_size;
	y_size = png.y_size;
	return texture != nullptr;
}

void AppSeeState::OnFirstTick () {
	a.mine_info.active_timer = -1;
}

void AppSeeState::OnTick () {
	a.mine_info.Tick();
}

void AppSeeState::OnDraw () {
	auto is_active = 0 < a.mine_info.active_timer;
	auto ticker_x = a.canvas_x_size - 7 * ((d.timer / 6) % 50);
	
	a.draw_funcs.DrawText(is_active ? "Found" : "Not found", ticker_x, 0);
	
	if(is_active) {
		
		// Field
		auto tile_x = (a.mine_info.max_x_size - a.mine_info.x_size) / 2;
		auto tile_y = (a.mine_info.max_y_size - a.mine_info.y_size) / 2;
		
		for(int x = 0; x < a.mine_info.x_size; x++) {
			for(int y = 0; y < a.mine_info.y_size; y++) {
				
				// Select the appropriate grid sprite.
				int sprite = 0;
				int val = a.mine_info.game [y * a.mine_info.row_size + x];
				
				if(val & 0x40) {
					int num = val & (~0x40);
					
					if(0 == num) {
						sprite = 3;
					}
					
					else {
						sprite = 5 + num;
					}
				}
				
				else if(val == 0x0f) {
					sprite = 5;
				}
				
				else if(val == 0x8f) {
					sprite = 0;
				}
				
				a.draw_funcs.DrawTile(sprite, 0, 1 + x + tile_x, 1 + y + tile_y);
			}
		}
	
		// Cursor
		a.draw_funcs.DrawTile(4, 0, tile_x + a.mine_info.cursor_x_tile, tile_y + a.mine_info.cursor_y_tile);
	}
}

AppLoop::AppLoop (AppData& app_data) : a(app_data) {}

// SDL App loop.
int AppLoop::MainLoop () {
	SDL_Init(SDL_INIT_EVERYTHING);
	
	a.window = SDL_CreateWindow(
		"Minesweeper Seer",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		a.window_x_size, a.window_y_size,
		SDL_WINDOW_RESIZABLE);
	a.renderer = SDL_CreateRenderer(a.window, -1, 0);
	a.keyboard = SDL_GetKeyboardState(nullptr);
	
	StateData state_data(a);
	state_data.timer = 0;
	
	AppSeeState see_state(a, state_data);
	a.current_state = &see_state;
	
	long long previous_tick = 0;
	long long ticks_per_frame = 1000.0 / 60;
	
	auto canvas_x_size = a.canvas_x_size;
	auto canvas_y_size = a.canvas_y_size;
	a.target = SDL_CreateTexture(a.renderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, canvas_x_size, canvas_y_size);
	
	a.sprite.LoadFromPng(a.renderer, "sprites.png");
	
	while(a.exit_clicked < 1) {
		
		// Limit frames per second.
		if(SDL_GetTicks() < previous_tick + ticks_per_frame) {
			continue;
		}
		
		// Let SDL handle window, keys, OS specific events.
		while(SDL_PollEvent(&a.event)) {
			switch(a.event.type) {
			case SDL_QUIT:
				a.exit_clicked++;
				break;
			}
		}
		
		// Get window sizes and calculate canvas offsets.
		SDL_GetWindowSize(a.window, &a.window_x_size, &a.window_y_size);
		
		auto canvas_scale = std::max(1, std::min(a.window_x_size / canvas_x_size, a.window_y_size / canvas_y_size));
		auto canvas_scaled_x = canvas_scale * canvas_x_size;
		auto canvas_scaled_y = canvas_scale * canvas_y_size;
		auto canvas_x = 0.5 * (a.window_x_size - canvas_scaled_x);
		auto canvas_y = 0.5 * (a.window_y_size - canvas_scaled_y);
		
		// Count all inputs.
		a.inputs.Tick("up", a.keyboard [SDL_SCANCODE_UP] || a.keyboard [SDL_SCANCODE_W]);
		a.inputs.Tick("down", a.keyboard [SDL_SCANCODE_DOWN] || a.keyboard [SDL_SCANCODE_S]);
		a.inputs.Tick("left", a.keyboard [SDL_SCANCODE_LEFT] || a.keyboard [SDL_SCANCODE_A]);
		a.inputs.Tick("right", a.keyboard [SDL_SCANCODE_RIGHT] || a.keyboard [SDL_SCANCODE_D]);
		a.inputs.Tick("confirm", a.keyboard [SDL_SCANCODE_Y] || a.keyboard [SDL_SCANCODE_Z]);
		a.inputs.Tick("cancel", a.keyboard [SDL_SCANCODE_X]);
		
		// Game state handling.
		OnTick();
		
		// Renderer handling.
		SDL_SetRenderDrawColor(a.renderer, 0x1a, 0x36, 0x2e, 0xff);
		SDL_RenderClear(a.renderer);
		
		SDL_SetRenderTarget(a.renderer, a.target);
		SDL_SetRenderDrawColor(a.renderer, 0x0a, 0x26, 0x1e, 0xff);
		SDL_RenderClear(a.renderer);
		
		// Draw the game state to the smaller canvas. It will be scaled up.
		OnDraw();
		
		SDL_Rect canvas_rect = {
			(int)canvas_x, (int)canvas_y,
			canvas_scaled_x, canvas_scaled_y
		};
		
		SDL_SetRenderTarget(a.renderer, nullptr);
		SDL_RenderCopy(a.renderer, a.target, nullptr, &canvas_rect);
		SDL_RenderPresent(a.renderer);
		previous_tick = SDL_GetTicks();
	}
	
	SDL_DestroyTexture(a.target);
	SDL_Quit();
	return 0;
}

void AppLoop::OnTick () {
	if(0 == a.current_state->d.timer) {
		a.current_state->OnFirstTick();
	}
	
	a.current_state->OnTick();
	a.current_state->d.timer++;
}

void AppLoop::OnDraw () {
	a.current_state->OnDraw();
}

AppData::AppData () : draw_funcs(*this) {
	DefaultSettings();
}

AppData::~AppData () {
}

void AppData::DefaultSettings () {
	window_x_size = 800;
	window_y_size = 600;
	canvas_x_size = 8 * WinMineInfo::max_x_size + 16;
	canvas_y_size = 8 * WinMineInfo::max_y_size + 16;
	exit_clicked = 0;
}

int main (int argc, char* argv []) {
	AppData app_data;
	AppLoop app_loop(app_data);
	
	app_data.cmd_arg_count = argc;
	app_data.cmd_args = argv;
	return app_loop.MainLoop();
}
