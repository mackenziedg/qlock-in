# Qlock

Qlock is a command-line time tracker.

## Usage

Create a new project with

```bash
$ qlock new p
```

Add a task to the project with

```bash
$ qlock new t
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
