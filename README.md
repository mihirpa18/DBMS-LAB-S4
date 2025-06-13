# NITCbase – RDBMS Implementation Project

NITCbase is a simplified **Relational Database Management System (RDBMS)** implemented as part of an undergraduate coursework project at **NIT Calicut**. The project is designed to help students gain a deeper understanding of how an RDBMS is built from scratch, layer by layer.

This repository contains my implementation of NITCbase, covering all the essential components of a relational database system including table management, record handling, basic SQL queries, and indexing using B+ Trees.

## 🔍 Project Overview

NITCbase follows an **eight-layer architecture**, guiding the implementation in a structured and modular fashion. Each layer introduces a new capability, building toward a functional and educational RDBMS.

Key learning outcomes include:
- Hands-on experience in system design.
- Understanding data structures and file systems used in databases.
- Implementing parsing, query execution, and indexing from scratch.

## ✨ Features

- **Table Management**: Create, drop, and alter tables.
- **Record Handling**: Insert records with support for dynamic storage allocation.
- **SQL Query Support** (Elementary):
  - `CREATE`, `DROP`, `ALTER`
  - `INSERT`, `SELECT`
  - `PROJECT`, `EQUI-JOIN`
- **Indexing**:
  - `CREATE INDEX` and `DROP INDEX` using **B+ Tree**
- **Disk-Based Storage** with a custom page-based file format.
- **Buffer Management** for handling in-memory page caching.
- **No Concurrency**: This implementation is single-user and single-threaded for simplicity and learning purposes.


> Note: Folder/file structure may vary based on your implementation. Please adjust accordingly.

## 🚀 Getting Started

### Requirements
- Linux environment 


### Build Instructions

```bash
git clone https://github.com/yourusername/nitcbase.git
cd nitcbase
make
./nitcbase
