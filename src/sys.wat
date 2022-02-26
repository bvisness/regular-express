(module $sys
	(memory (import "env" "memory") 10)
	(func (export "abort") unreachable)
	(func (export "memset")
		(param $ptr i32) (param $value i32) (param $num i32)
		(result i32)

		local.get $ptr
		local.get $value
		local.get $num
		memory.fill

		local.get $ptr
	)
	(func (export "memcpy")
		(param $dst i32) (param $src i32) (param $count i32)
		(result i32)

		local.get $dst
		local.get $src
		local.get $count
		memory.copy

		local.get $dst
	)
	(func (export "memchr")
		(param $ptr i32) (param $value i32) (param $num i32)
		(result i32)

		(local $currentPtr i32)

		local.get $ptr
		local.set $currentPtr

		loop
			;; check if pointing at occurrence of value
			local.get $currentPtr
			i32.load8_u
			local.get $value
			i32.eq
			if
				local.get $currentPtr
				return
			end

			;; increment currentPtr
			local.get $currentPtr
			i32.const 1
			i32.add
			local.set $currentPtr

			;; decrement num
			local.get $num
			i32.const 1
			i32.sub
			local.tee $num

			br_if 0
		end

		i32.const 0
	)
)
