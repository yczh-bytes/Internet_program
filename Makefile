# 编译器
CC = gcc

# 编译选项
CFLAGS = -Wall

# 源码目录
SRC = src

# 可执行文件目录
BIN = bin

# 最终生成文件
TARGETS = \
	$(BIN)/userserve \
	$(BIN)/clientserve \
	$(BIN)/client_fork_serve \
	$(BIN)/client_pthread

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

# 清理
clean:
	rm -f $(BIN)/*

# 重新编译
rebuild: clean all