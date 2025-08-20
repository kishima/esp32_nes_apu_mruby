# Test

mruby/c uses picoruby-picotest of PicoRuby for unit testing.

https://github.com/picoruby/picoruby/blob/master/mrbgems/picoruby-picotest/README.md

## Prerequisite

- Docker: https://docs.docker.com/get-docker/

## Setup

```
make docker_build
```

## Test

### Test for host under no MRBC_LIBC_ALLOC defined

```
make test
```

### Test all combinations of build options and target platforms

```
make test_full
```
