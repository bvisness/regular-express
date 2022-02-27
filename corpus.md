# js-regex "business logic"

```
(SH|RE|MF)-((?:197[1-9]|19[89]\d|[2-9]\d{3})-(?:0[1-9]|1[012])-(?:0[1-9]|[12]\d|3[01]))-((?!0{5})\d{5})
```

This makes fairly heavy use of anonymous groups, and numbered repetition.

# email

```
(?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\])
```

I don't think I support this kind of capture.

# named capture group reference

```
(?<title>\w+), yes \k<title>
```

I didn't even know this was a thing.
