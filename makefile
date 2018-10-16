#
# Makefile ESQUELETO
#
# DEVE ter uma regra "all" para geração da biblioteca
# regra "clean" para remover todos os objetos gerados.
#
# NECESSARIO adaptar este esqueleto de makefile para suas necessidades.
#
# 

CC=gcc
LIB_DIR=./lib
INC_DIR=./include
BIN_DIR=./bin
SRC_DIR=./src

all: t2fs.o libt2fs.a t2fstst

t2fs.o: $(SRC_DIR)/t2fs.c
	$(CC) -c $(SRC_DIR)/t2fs.c -o $(BIN_DIR)/t2fs.o -Wall
	
libt2fs.a: t2fs.o
	ar crs $(LIB_DIR)/libt2fs.a $(BIN_DIR)/t2fs.o $(LIB_DIR)/apidisk.o

t2fstst: libt2fs.a
	$(CC) -o $(EXP_DIR)/t2fstst $(EXP_DIR)/t2fstst.c -L$(LIB_DIR) -lt2fs -Wall

clean:
rm -rf $(EXP_DIR)/t2fstst $(LIB_DIR)/*.a $(BIN_DIR)/*.o $(SRC_DIR)/*~ $(INC_DIR)/*~ *~

