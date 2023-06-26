package main

import "core:fmt"
import "core:mem"
import "core:runtime"
import "vendor:wasm/js"

import "alloc"
import "host"
import "undo"

// allocator state
global_arena := alloc.Arena{}
temp_arena := alloc.Arena{}

global_allocator: mem.Allocator
temp_allocator: mem.Allocator

wasm_context := runtime.default_context()

main :: proc() {
	ONE_GB_PAGES :: 1 * 1024 * 1024 * 1024 / js.PAGE_SIZE
	ONE_MB_PAGES :: 1 * 1024 * 1024 / js.PAGE_SIZE
	temp_data, _ := js.page_alloc(ONE_MB_PAGES * 20)

	alloc.arena_init(&temp_arena, temp_data)

	// This must be init last, because it grows infinitely.
	// We don't want it accidentally growing into anything useful.
	alloc.growing_arena_init(&global_arena)

	temp_allocator = alloc.arena_allocator(&temp_arena)
	global_allocator = alloc.growing_arena_allocator(&global_arena)

	wasm_context.allocator = global_allocator
	wasm_context.temp_allocator = temp_allocator

	context = wasm_context
}

@export
foo :: proc "contextless" () {
    context = wasm_context

    fmt.println("foo")
    host.bar(4, "haldo")
}
