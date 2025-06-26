# miniftp
Skeleton for socket programming lectures 2025

# 📡 Minimal FTP Server Project

A progressive FTP server implementation written in C, following [RFC 959](https://www.rfc-editor.org/rfc/rfc959.html).  
This project is built in stages, starting from a simple single-client server and evolving toward full concurrency and protocol support.

## 🗂️ Project Structure

```text
server/
├── iterative/     # Single-client blocking FTP server
├── concurrent/    # Forked or threaded server (multi-client)
└── README.md      # This file
```

---

## 🚦 Iterative Server

📁 `iterative/`

- Single-process, single-threaded  
- Blocking I/O  
- Handles one client at a time  
- Implements minimal required FTP commands:
  - `USER`, `PASS`, `QUIT`, `SYST`, `PORT`, `TYPE`, `RETR`, `STOR`, `NOOP`

✅ **Status**: Functional  
🧠 **Educational focus**: Understand basic FTP command flow and socket API.

---

## 👥 Concurrent Server (planned)

📁 `concurrent/`

- Multi-client support using `fork()` or `pthread`
- One child or thread per client

🚧 **Status**: To be implemented  
🧠 **Educational focus**: Understand process/thread concurrency and client isolation.

---

## 🛠️ Build Instructions

Each version has its own `Makefile`. To compile the current version:

```sh
cd iterative
make
make clean
```

---

## 🧪 Testing

You can test with any FTP client, e.g.:

```text
ftp localhost 2121
```

---

## 📚 RFCs and Standards
- RFC 959 – File Transfer Protocol
- RFC 2228 – FTP Security Extensions

---

# 💡 License

MIT License
