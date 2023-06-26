package main

import "core:intrinsics"

foreign import "host"

@(default_calling_convention="contextless")
foreign host {
    bar :: proc(x: i32, y: string) ---
    set_error_code :: proc(code: int) ---
}

trap :: proc "contextless" () -> ! {
	intrinsics.trap()
}

CrashError :: enum int {
    NoError = 0,
    OutOfMemory = 1,
}

die :: proc(code: CrashError) -> ! {
    set_error_code(int(code))
    trap()
}
