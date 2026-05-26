# Nexus-C Kernel

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
