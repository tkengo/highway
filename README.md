# Highway

A fast pattern matching tool like `pt` and `ag`.

## Features

- It can search the PATTERN from your source code very fast.
- It can search also the PATTERN as a regular expression.
- It supports UTF-8 and some Japanese encodings (Because I'm Japanese :yum: EUC-JP and Shift_JIS is supported).
- It ignores file patterns form your `.gitiginore` by default.

## Installation

### For OS X

```shell
$ brew tap tkengo/highway
$ brew install highway
```

### For Fedora Core

```shell
$ sudo vi /etc/yum.repos.d/highway.repo
[repos.highway]
name=highway
baseurl=http://tkengo.github.io/highway/fedora
enabled=0
gpgcheck=0

$ sudo yum install highway --enablerepo="repos.highway"
```

## Usage

```shell
# You can search PATTERN from current directory recursively.
$ hw PATTERN

# You can specify options and some paths.
$ hw OPTIONs PATTERN PATHs

# You can check options
$ hw -h
```

## Building from source

1. Building dependencies:

- gperftools
- autoconf
- automake

2. Run follow commands:

```shell
$ git clone git@github.com:tkengo/highway.git
$ cd highway
$ ./tools/build.sh
```
## License

The MIT License (MIT)

Copyright (c) 2015 Kengo Tateishi
