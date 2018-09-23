# Qlock

Qlock is a command-line time tracker.

## Usage

Create a new project with

```bash
$ qlock new p
```

Switch between created projects with

```bash
$ qlock switch <name>
```

Add a task to the project with

```bash
$ qlock new t
```

View a list of created projects or tasks with

```bash
$ qlock list p
```

or

```bash
$ qlock list t
```

Clock in and out of a task `N` with

```bash
$ qlock in N
$ qlock out N
```

To view the total tracked time for a task `N` use

```bash
$ qlock elapsed N
```

To get a list of currently active tasks use

```bash
$ qlock active
```


## Building

Just run `make` to build the release version, `make debug` to build the debug version, and `make test` to build the tests.
