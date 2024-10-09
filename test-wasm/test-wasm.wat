(module
  (memory (export "memory") 1)
  (global (export "__heap_base") i32 (i32.const 1024))
  (func (export "test") (param i32 i32) (result i64)
    (memory.fill (i32.const 0) (i32.const 1) (i32.const 4))
    (i64.const 0)
  )
)
