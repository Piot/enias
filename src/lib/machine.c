/*

MIT License

Copyright (c) 2017 Peter Bjorklund

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
#include <enias/machine.h>
#include <zany/loader.h>
#include <zany/run.h>

#if defined ENIAS_PLATFORM_WEBASSEMBLY
#include <emscripten/emscripten.h>
#endif

void enias_machine_init(enias_machine* self)
{
	zany_cpu_init(&self->cpu);
	enias_graphics_chip_init(&self->graphics);
	enias_input_chip_init(&self->input);
	enias_sound_chip_init(&self->sound);
}

void enias_machine_close(enias_machine* self)
{
	//zany_cpu_close(&self->cpu);
	//enias_graphics_chip_close(&self->graphics);
	//enias_input_chip_close(&self->input);
	enias_sound_chip_close(&self->sound);
}

void enias_machine_load_memory(enias_machine* self, const char* filename)
{
	zany_load(&self->cpu, filename);
	self->cpu.memory[ZANY_CONTINUE_VECTOR] = 0x00;
	self->cpu.memory[ZANY_CONTINUE_VECTOR + 1] = 0x02;
}

static int update_frame(enias_machine* self)
{
	zany_cpu_set_continue_vector(&self->cpu);
	int error_code = zany_run(&self->cpu);
	if (error_code) {
		printf("ERR: cpu error code:%d\n", error_code);
		return error_code;
	}
	enias_sound_chip_update(&self->sound, self->cpu.memory);
	int quit = enias_input_chip_update(&self->input, self->cpu.memory);
	enias_graphics_chip_render(&self->graphics, self->cpu.memory);
	return quit;
}

#if defined ENIAS_PLATFORM_WEBASSEMBLY
static void on_frame(void* _self)
{
	enias_machine* self = (enias_machine*) _self;
	update_frame(self);
}
#endif

static int loop(enias_machine* self)
{
	int quit = 0;

	while (!quit) {
		quit = update_frame(self);
	}

	enias_machine_close(self);
	return quit;
}

int enias_machine_go(enias_machine* self)
{
#if defined ENIAS_PLATFORM_WEBASSEMBLY
	emscripten_set_main_loop_arg(on_frame, self, 0, 1);
	return 0;
#else
	return loop(self);
#endif
}
