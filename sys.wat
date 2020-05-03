(module $sys
	(memory (import "env" "memory") 2)
	(func (export "abort") unreachable)
	(func (export "memset")
		(param $ptr i32) (param $value i32) (param $num i32)
		(result i32)

		(local $currentPtr i32)
		get_local $ptr
		set_local $currentPtr

		loop
			;; store value
			get_local $currentPtr
			get_local $value
			i32.store8

			;; increment currentPtr
			get_local $currentPtr
			i32.const 1
			i32.add
			set_local $currentPtr

			;; decrement num
			get_local $num
			i32.const 1
			i32.sub
			tee_local $num

			br_if 0
		end

		get_local $ptr
	)
	(func (export "memcpy")
		(param $dst i32) (param $src i32) (param $count i32)
		(result i32)

		(local $currentSrc i32)
		(local $currentDst i32)

		get_local $src
		set_local $currentSrc
		get_local $dst
		set_local $currentDst

		loop
			;; copy value
			get_local $currentDst
			get_local $currentSrc
			i32.load8_u
			i32.store8

			;; increment ptrs
			get_local $currentSrc
			i32.const 1
			i32.add
			set_local $currentSrc
			get_local $currentDst
			i32.const 1
			i32.add
			set_local $currentDst

			;; decrement count
			get_local $count
			i32.const 1
			i32.sub
			tee_local $count

			br_if 0
		end

		get_local $dst
	)
)
