# Nexus-C Kernel
![GitHub repo size](https://img.shields.io/github/repo-size/tymotiii/nexus-c)
![GitHub stars](https://img.shields.io/github/stars/tymotiii/nexus-c?style=social)
![GitHub forks](https://img.shields.io/github/forks/tymotiii/nexus-c?style=social)
![GitHub issues](https://img.shields.io/github/issues/tymotiii/nexus-c)
![GitHub last commit](https://img.shields.io/github/last-commit/tymotiii/nexus-c)

![Language](https://img.shields.io/github/languages/top/tymotiii/nexus-c)
![Languages count](https://img.shields.io/github/languages/count/tymotiii/nexus-c)

![License](https://img.shields.io/github/license/tymotiii/nexus-c)

A simple hobby operating system kernel written in C.

This project is built from scratch with the goal of understanding low-level systems, OS development, and how computers actually work under the hood.

## 🚀 Features

- Multiboot-compliant kernel
- Bootable with GRUB
- VGA text mode output
- Basic memory / hardware interaction (WIP)
- Minimal libc-like functions
- Custom build system (Makefile)

## 🧠 Goals

This project is mainly focused on:

- Learning OS development
- Understanding x86 architecture
- Building everything manually instead of relying on existing frameworks

## 📁 Project Structure

```
.
├── kernel.c
├── linker.ld
├── Makefile
├── iso/
└── *.c / *.h
```

## ⚙️ Requirements

- GCC (with 32-bit support)
- GRUB
- xorriso / grub-mkrescue
- qemu

## 🔨 Build & Run

```bash
make run
```

## ⚠️ Disclaimer

This is a hobby project.
It is unstable, incomplete, and probably broken.

## 📌 Roadmap

- [ ] Memory management
- [ ] Interrupts
- [ ] Keyboard input
- [ ] Scheduler
- [ ] Filesystem

## 🤝 Contributing

See CONTRIBUTING.md

## 📜 License

Add your license here
