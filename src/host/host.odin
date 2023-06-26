package host

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

die :: proc(code: int) -> ! {
    set_error_code(code)
    trap()
}
