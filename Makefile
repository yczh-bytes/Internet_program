# 编译器
CC = gcc
CXX = g++

# 编译选项
CFLAGS = -Wall
CXXFLAGS = -Wall -std=c++14

# 源码目录
SRC = src
MAIN_SRC = src/main

# 可执行文件目录
BIN = bin

# 最终生成文件
TARGETS = \
	$(BIN)/userserve \
	$(BIN)/clientserve \
	$(BIN)/client_fork_serve \
	$(BIN)/client_pthread \
	$(BIN)/httpserve

# 默认目标
all: $(TARGETS)

# userserve
$(BIN)/userserve: $(SRC)/userserve.c
	$(CC) $(CFLAGS) $^ -o $@

# clientserve
$(BIN)/clientserve: $(SRC)/clientserve.c
	$(CC) $(CFLAGS) $^ -o $@

# client_fork_serve
$(BIN)/client_fork_serve: $(SRC)/client_fork_serve.c
	$(CC) $(CFLAGS) $^ -o $@

# client_pthread
$(BIN)/client_pthread: $(SRC)/client_pthread.c
	$(CC) $(CFLAGS) $^ -o $@ -lpthread

# httpserve (main项目 - HTTP服务器)
$(BIN)/httpserve: $(MAIN_SRC)/main.cpp $(MAIN_SRC)/http_conn.cpp $(MAIN_SRC)/lock.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

# 清理
clean:
	rm -f $(BIN)/*

# 重新编译
rebuild: clean all
