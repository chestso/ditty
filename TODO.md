# Ditty TODO

## Error Handling

### unwind-protect + tail-call optimization

`unwind-protect` has a similar issue to the `condition-case` tail-call bug. The cleanup forms may not run if an error occurs during a tail-recursive call inside the protected body.

**Investigation needed:**

- Verify if `unwind-protect` cleanup runs correctly when an error occurs in a tail-recursive loop
- If not, apply similar handler_stack mechanism or use a separate cleanup_stack
- The cleanup must run even if the body becomes a tail call

**Test case:**

```lisp
(define cleanup-ran #f)
(unwind-protect
  (let loop ()
    (error 'test-error)
    (loop))
  (set! cleanup-ran #t))
;; cleanup-ran should be #t
```

**Related:** See `ditty-bug-condition-case-tail-call.md` for the fix pattern used for `condition-case`.
