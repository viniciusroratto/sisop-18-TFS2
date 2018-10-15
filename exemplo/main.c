
/**

    T2 shell, para teste do T2FS - Sistema de arquivos do trabalho 2 de Sistemas Operacionais I

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "t2fs.h"


void cmdMan(void);

void cmdWho(void);
void cmdLs(void);
void cmdMkdir(void);
void cmdRmdir(void);

void cmdOpen(void);
void cmdRead(void);
void cmdClose(void);

void cmdWrite(void);
void cmdCreate(void);
void cmdDelete(void);
void cmdSeek(void);
void cmdTrunc(void);

void cmdLn(void);

void cmdCp(void);
void cmdFscp(void);

void cmdExit(void);

static void dump(char *buffer, int size) {
    int base, i;
    char c;
    for (base=0; base<size; base+=16) {
        printf ("%04d ", base);
        for (i=0; i<16; ++i) {
            if (base+i<size) printf ("%02X ", buffer[base+i]);
            else printf ("   ");
        }

        printf (" *");

        for (i=0; i<16; ++i) {
            if (base+i<size) c = buffer[base+i];
            else c = ' ';

            if (c<' ' || c>'z' ) c = ' ';
            printf ("%c", c );
        }
        printf ("*\n");
    }
}

#define	CMD_EXIT	0
#define	CMD_MAN		1
#define	CMD_WHO		2
#define	CMD_DIR		3
#define	CMD_MKDIR	4
#define	CMD_RMDIR	5
#define	CMD_OPEN	6
#define	CMD_READ	7
#define	CMD_CLOSE	8
#define	CMD_WRITE	9
#define	CMD_CREATE	10
#define	CMD_DELETE	11
#define	CMD_SEEK	12
#define	CMD_TRUNCATE	13
#define	CMD_LN		14
#define	CMD_COPY	15
#define	CMD_FS_COPY	16

char helpString[][120] = {
	"             -> finish this shell",
	"[comando]    -> command help",
	"             -> shows T2FS authors",
	"[pahname]    -> list files in [pathname]",
	"[dirname]    -> create [dirname] in T2FS",
	"[dirname]    -> deletes [dirname] from T2FS",
	"[file]       -> open [file] from T2FS",
	"[hdl] [siz]  -> read [siz] bytes from file [hdl]",
	"[hdl         -> close [hdl]",
	"[hdl] [str]  -> write [str] bytes to file [hdl]",
	"[file]       -> create new [file] in T2FS",
	"[file]       -> deletes [file] from T2FS",
	"[hdl] [pos]  -> set CP of [hdl] file on [pos]",
	"[hdl] [siz]  -> truncate file [hdl] to [siz] bytes",
	"[src] [dst]  -> copy files: [src] -> [dst]",
	"[lnk] [file] -> create link [lnk] to [file]",
	"\n    fscp -t [src] [dst]  -> copy HostFS to T2FS"
	"\n    fscp -f [src] [dst]  -> copy T2FS   to HostFS"
};

	
struct {
	char name[20];
	void (*f)(void);
	int helpId;
} cmdList[] = {
	{ "exit", cmdExit, CMD_EXIT }, { "x", cmdExit, CMD_EXIT },
	{ "man", cmdMan, CMD_MAN },
	{ "who", cmdWho, CMD_WHO }, { "id", cmdWho, CMD_WHO },
	{ "dir", cmdLs, CMD_DIR }, { "ls", cmdLs, CMD_DIR },
	{ "mkdir", cmdMkdir, CMD_MKDIR }, { "md", cmdMkdir, CMD_MKDIR },
	{ "rmdir", cmdRmdir, CMD_RMDIR }, { "rm", cmdRmdir, CMD_RMDIR },
	
	{ "open", cmdOpen, CMD_OPEN },
	{ "read", cmdRead, CMD_READ }, { "rd", cmdRead, CMD_READ },
	{ "close", cmdClose, CMD_CLOSE }, { "cl", cmdClose, CMD_CLOSE },
	{ "write", cmdWrite, CMD_WRITE }, { "wr", cmdWrite, CMD_WRITE },
	{ "create", cmdCreate, CMD_CREATE }, { "cr", cmdCreate, CMD_CREATE },
	{ "delete", cmdDelete, CMD_DELETE }, { "del", cmdDelete, CMD_DELETE },
	{ "seek", cmdSeek, CMD_SEEK }, { "sk", cmdSeek, CMD_SEEK },
	{ "truncate", cmdTrunc, CMD_TRUNCATE }, { "trunc", cmdTrunc, CMD_TRUNCATE }, { "tk", cmdTrunc, CMD_TRUNCATE },
	
	{ "ln", cmdLn, CMD_LN },
	
	{ "cp", cmdCp, CMD_COPY },
	{ "fscp", cmdFscp, CMD_FS_COPY },
	{ "fim", NULL, -1 }
};



void tst_identify() {
    char name[256];
	int err;
	
	printf ("Teste do identify()\n");
	
    err = identify2(name, 256);
    if (err) {
        printf ("Erro: %d\n", err);
        return;
    }
	
	printf ("Ok!\n\n");
}

void tst_open(char *src) {
    FILE2 hSrc;
	
	printf ("Teste do open() e close()\n");
	
	hSrc = open2 (src);
	
    if (hSrc<0) {
        printf ("Erro: Open %s (handle=%d)\n", src, hSrc);
        return;
    }
	
	if (close2(hSrc)) {
        printf ("Erro: Close (handle=%d)\n", hSrc);
        return;
	}
	
	printf ("Ok!\n\n");
}

void tst_read(char *src) {
	char buffer[256];
    FILE2 hSrc;
	
	printf ("Teste do read()\n");
	
    hSrc = open2 (src);
    if (hSrc<0) {
        printf ("Erro: Open %s (handle=%d)\n", src, hSrc);
        return;
    }
	
    int err = read2(hSrc, buffer, 256);
    if (err<0) {
        printf ("Error: Read %s (handle=%d), err=%d\n", src, hSrc, err);
		close2(hSrc);
        return;
    }	
    if (err==0) {
        printf ("Error: Arquivo vazio %s (handle=%d)\n", src, hSrc);
		close2(hSrc);
        return;
    }

    dump(buffer, err);  
	
	if (close2(hSrc)) {
        printf ("Erro: Close (handle=%d)\n", hSrc);
        return;
	}
	printf ("Ok!\n\n");
}

void tst_list_dir(char *src) {
    DIR2 d;
	int n;
	
	printf ("Teste do opendir(), readdir() e closedir()\n");
	
    // Abre o diretório pedido
    d = opendir2(src);
    if (d<0) {
        printf ("Erro: Opendir %s (handle=%d)\n", src, d);
        return;
    }

    // Coloca diretorio na tela
    DIRENT2 dentry;
    while ( readdir2(d, &dentry) == 0 ) {
        printf ("%c %8u %s\n", (dentry.fileType==0x02?'d':'-'), dentry.fileSize, dentry.name);
    }

    n = closedir2(d);
	if (n) {
        printf ("Erro: Closedir %s (handle=%d)\n", src, d);
        return;
	}
	printf ("Ok!\n\n");
}

void tst_seek(char *src, int seek_pos) {
	char buffer[256];
    FILE2 hSrc;
	int err;
	
	printf ("Teste do seek2()\n");
	
    hSrc = open2 (src);
    if (hSrc<0) {
        printf ("Erro: Open %s (handle=%d)\n", src, hSrc);
        return;
    }
	
    err = seek2(hSrc, seek_pos);
    if (err<0) {
        printf ("Error: Seek %s (handle=%d), err=%d\n", src, hSrc, err);
		close2(hSrc);
        return;
    }
	
    err = read2(hSrc, buffer, 256);
    if (err<0) {
        printf ("Error: Read %s (handle=%d), err=%d\n", src, hSrc, err);
		close2(hSrc);
        return;
    }
    if (err==0) {
        printf ("Error: Arquivo vazio %s (handle=%d)\n", src, hSrc);
		close2(hSrc);
        return;
    }

    dump(buffer, err);  
	
	if (close2(hSrc)) {
        printf ("Erro: Close (handle=%d)\n", hSrc);
        return;
	}
	printf ("Ok!\n\n");
}

void tst_create(char *src) {
    FILE2 hFile;
	int err;
	
	printf ("Teste do create2()\n");

    hFile = create2 (src);
    if (hFile<0) {
        printf ("Error: Create %s, handle=%d\n", src, hFile);
        return;
    }

	err = close2(hFile);
	if (err) {
        printf ("Erro: Close %s, handle=%d, err=%d\n", src, hFile, err);
        return;
	}
	
	printf ("Ok!\n\n");
}

void tst_write(char *src, char *texto) {
    FILE2 handle;
	int err;
	
	printf ("Teste do write2()\n");
	
    handle = open2 (src);
    if (handle<0) {
        printf ("Erro: Open %s, handle=%d (PROVAVEL CAUSA = arquivo nao existe)\n", src, handle);
        return;
    }
	
    err = write2(handle, texto, strlen(texto));
    if (err<0) {
        printf ("Error: Write %s, handle=%d, err=%d\n", src, handle, err);
		close2(handle);
        return;
    }
	
	if (close2(handle)) {
        printf ("Erro: Close %s, handle=%d\n", src, handle);
        return;
	}
	
	printf ("Ok!\n\n");
	
}

void tst_truncate(char *src, int size) {
    FILE2 handle;
	int err;

	printf ("Teste do truncate2()\n");
	
    handle = open2(src);
    if (handle<0) {
        printf ("Erro: Open %s, handle=%d\n", src, handle);
        return;
    }
	
    // posiciona CP na posicao selecionada
    err = seek2(handle, size);
    if (err<0) {
        printf ("Error: Seek %s, handle=%d, pos=%d, err=%d\n", src, handle, size, err);
		close2(handle);
        return;
    }
    
    // trunca
    err = truncate2(handle);
    if (err<0) {
        printf ("Error: Truncate %s, handle=%d, pos=%d, err=%d\n", src, handle, size, err);
		close2(handle);
        return;
    }
	
	if (close2(handle)) {
        printf ("Erro: Close (handle=%d)\n", handle);
        return;
	}
	
	printf ("Ok!\n\n");
}

void tst_delete(char *src) {
    int err;
	
	printf ("Teste do delete2()\n");
	
	err = delete2(src);
	if (err) {
        printf ("Erro: Delete %s, err=%d\n", src, err);
        return;
	}
	
	printf ("Ok!\n\n");
	
}

void tst_create_dir(char *src) {
    int err;
	
	printf ("Teste do mkdir2()\n");
	
    err = mkdir2(src);
    if (err<0) {
        printf ("Error: Mkdir %s, err=%d\n", src, err);
        return;
    }

	printf ("Ok!\n\n");
}

void tst_delete_dir(char *src) {
    int err;
	
	printf ("Teste do rmdir2()\n");
	
    err = rmdir2(src);
    if (err<0) {
        printf ("Error: Del dir %s, err=%d\n", src, err);
        return;
    }

	printf ("Ok!\n\n");
	
}

void tst_getcd() {
    int err;
	char buffer[256];
	
	printf ("Teste do getcwd2()\n");
	
    err = getcwd2(buffer, 256);
    if (err<0) {
        printf ("Error: getcwd, err=%d\n", err);
        return;
    }
	
	printf ("cd = %s\n", buffer);
	printf ("Ok!\n\n");
}

void tst_chdir(char *src) {
    int err;
	
	printf ("Teste do chdir2()\n");
	
    err = mkdir2(src);
    if (err<0) {
        printf ("Error: Mkdir %s, err=%d\n", src, err);
        return;
    }
	
	err=chdir2(src);
    if (err<0) {
        printf ("Error: Chdir %s, err=%d\n", src, err);
        return;
    }

	printf ("Ok!\n\n");	
}



void teste(int tstNumber) {
	if (tstNumber<0) {
		printf (" 1 - identify2()\n");
		printf (" 2 - open        open2,close2          [x.txt]\n");
		printf (" 3 - read        open2,read2,close2    [x.txt]\n");
		printf (" 4 - list_dir                          [.]\n");
		printf (" 5 - seek        open2,seek2,close2    [x.txt; 7]\n");
		
		printf (" 6 - create      create2,close2        [y.txt]\n");
		printf (" 7 - write       open,write,close      [y.txt, abced...]\n");
		printf (" 8 - truncate    open,truncate,close   [y.txt, 11]\n");
		printf (" 9 - delete      delete2               [y.txt]\n");
		
		printf ("10 - create_dir  mkdir2                [ndir]\n");
		printf ("11 - delete_dir  rmdir2                [ndir]\n");
		
		printf ("12 -             getcwd2\n");
		printf ("13 - change cd   chdir2                [n2]\n");
		return;
	}
	switch(tstNumber) {
		case 1:
			tst_identify();
			break;
		case 2:
			tst_open("x.txt");
			break;
		case 3:
			tst_read("x.txt");
			break;
		case 4:
			tst_list_dir(".");
			break;
		case 5:
			tst_seek("x.txt", 7);
			break;
			
		case 6:
			tst_create("y.txt");
			tst_list_dir(".");		// Verificação
			break;
		case 7:
			tst_write("y.txt", "[abcdefghijklmnopqrst]"); 
			tst_read("y.txt");		// Verificação
			break;
		case 8:
			tst_truncate("y.txt", 11);
			tst_read("y.txt");		// Verificação
			break;
		case 9:
			tst_delete("y.txt");
			tst_list_dir(".");		// Verificação
			break;
			
		case 10:
			tst_create_dir("ndir");
			tst_list_dir(".");		// Verificação
			tst_list_dir("ndir");
			break;
		case 11:
			tst_delete_dir("ndir");
			tst_list_dir(".");		// Verificação
			break;
			
		case 12:
			tst_getcd();
			break;
		case 13:
			tst_create_dir("n2");
			tst_chdir("n2");
			tst_getcd();
			tst_chdir("..");
			tst_delete_dir("n2");
			break;
	}
}


int main()
{
    char cmd[256];
    char *token;
    int i,n;
    int flagAchou, flagEncerrar;

    printf ("Testing for T2FS - v 2018.1.2\n");
    //token = strtok("who"," \t");
    strcpy(cmd, "man");
    token = strtok(cmd," \t");
    cmdMan();

    flagEncerrar = 0;
    while (1) {
        printf ("T2FS> ");
        gets(cmd);
        if( (token = strtok(cmd," \t")) != NULL ) {
			// Verifica se é comando de teste
			n = atoi(token);
			if (n) {
				teste(n);
				continue;
			}
			//
			flagAchou = 0;
			for (i=0; strcmp(cmdList[i].name,"fim")!=0; i++) {
				if (strcmp(cmdList[i].name, token)==0) {
					flagAchou = 1;
					cmdList[i].f();
					if (cmdList[i].helpId==CMD_EXIT) {
						flagEncerrar = 1;
						break;
					}
				}
			}
			if (!flagAchou) printf ("???\n");
        }
		if (flagEncerrar) break;
    }
    return 0;
}

/**
Encerra a operação do teste
*/
void cmdExit(void) {
    printf ("bye, bye!\n");
}

/**
Informa os comandos aceitos pelo programa de teste
*/
void cmdMan(void) {
	int i;
	char *token = strtok(NULL," \t");
	
	if (token==NULL) {
		for (i=0; strcmp(cmdList[i].name,"fim")!=0; i++) {
			switch(i%4) {
				case 0: printf ("%s", cmdList[i].name); break;
				case 1:
				case 2: printf (", %s", cmdList[i].name); break;
				default: printf (", %s\n", cmdList[i].name); break;
			}
		}
		printf ("\n");
		return;
	}
	
	for (i=0; strcmp(cmdList[i].name,"fim")!=0; i++) {
		if (strcmp(cmdList[i].name,token)==0) {
			printf ("%-10s %s\n", cmdList[i].name, helpString[cmdList[i].helpId]);
		}
	}
	

}
	
/**
Chama da função identify2 da biblioteca e coloca o string de retorno na tela
*/
void cmdWho(void) {
    char name[256];
    int err = identify2(name, 256);
    if (err) {
        printf ("Erro: %d\n", err);
        return;
    }
    printf ("%s\n", name);
}

/**
Copia arquivo dentro do T2FS
Os parametros são:
    primeiro parametro => arquivo origem
    segundo parametro  => arquivo destino
*/
void cmdCp(void) {

    // Pega os nomes dos arquivos origem e destion
    char *src = strtok(NULL," \t");
    char *dst = strtok(NULL," \t");
    if (src==NULL || dst==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    // Abre o arquivo origem, que deve existir
    FILE2 hSrc = open2 (src);
    if (hSrc<0) {
        printf ("Open source file error: %d\n", hSrc);
        return;
    }
    // Cria o arquivo de destino, que será resetado se existir
    FILE2 hDst = create2 (dst);
    if (hDst<0) {
        close2(hSrc);
        printf ("Create destination file error: %d\n", hDst);
        return;
    }
    // Copia os dados de source para destination
    char buffer[2];
    while( read2(hSrc, buffer, 1) == 1 ) {
        write2(hDst, buffer, 1);
    }
    // Fecha os arquicos
    close2(hSrc);
    close2(hDst);

    printf ("Files successfully copied\n");
}

/**
Copia arquivo de um sistema de arquivos para o outro
Os parametros são:
    primeiro parametro => direção da copia
        -t copiar para o T2FS
        -f copiar para o FS do host
    segundo parametro => arquivo origem
    terceiro parametro  => arquivo destino
*/
void cmdFscp(void) {
    // Pega a direção e os nomes dos arquivos origem e destion
    char *direcao = strtok(NULL, " \t");
    char *src = strtok(NULL," \t");
    char *dst = strtok(NULL," \t");
    if (direcao==NULL || src==NULL || dst==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    // Valida direção
    if (strncmp(direcao, "-t", 2)==0) {
        // src == host
        // dst == T2FS

        // Abre o arquivo origem, que deve existir
        FILE *hSrc = fopen(src, "r+");
        if (hSrc==NULL) {
            printf ("Open source file error\n");
            return;
        }
        // Cria o arquivo de destino, que será resetado se existir
        FILE2 hDst = create2 (dst);
        if (hDst<0) {
            fclose(hSrc);
            printf ("Create destination file error: %d\n", hDst);
            return;
        }
        // Copia os dados de source para destination
        char buffer[2];
        while( fread((void *)buffer, (size_t)1, (size_t)1, hSrc) == 1 ) {
            write2(hDst, buffer, 1);
        }
        // Fecha os arquicos
        fclose(hSrc);
        close2(hDst);
    }
    else if (strncmp(direcao, "-f", 2)==0) {
        // src == T2FS
        // dst == host

        // Abre o arquivo origem, que deve existir
        FILE2 hSrc = open2 (src);
        if (hSrc<0) {
            printf ("Open source file error: %d\n", hSrc);
            return;
        }
        // Cria o arquivo de destino, que será resetado se existir
        FILE *hDst = fopen(dst, "w+");
        if (hDst==NULL) {
            printf ("Open destination file error\n");
            return;
        }
        // Copia os dados de source para destination
        char buffer[2];
        while ( read2(hSrc, buffer, 1) == 1 ) {
            fwrite((void *)buffer, (size_t)1, (size_t)1, hDst);
        }
        // Fecha os arquicos
        close2(hSrc);
        fclose(hDst);
    }
    else {
        printf ("Invalid copy direction\n");
        return;
    }

    printf ("Files successfully copied\n");
}

/**
Cria o arquivo informado no parametro
Retorna eventual sinalização de erro
Retorna o HANDLE do arquivo criado
*/
void cmdCreate(void) {
    FILE2 hFile;

    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }

    hFile = create2 (token);
    if (hFile<0) {
        printf ("Error: %d\n", hFile);
        return;
    }

    printf ("File created with handle %d\n", hFile);
}

/**
Apaga o arquivo informado no parametro
Retorna eventual sinalização de erro
*/
void cmdDelete(void) {

    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }

    int err = delete2(token);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }

    printf ("File %s was deleted\n", token);
}

/**
Abre o arquivo informado no parametro [0]
Retorna sinalização de erro
Retorna HANDLE do arquivo retornado
*/
void cmdOpen(void) {
    FILE2 hFile;

    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }

    hFile = open2 (token);
    if (hFile<0) {
        printf ("Error: %d\n", hFile);
        return;
    }

    printf ("File opened with handle %d\n", hFile);
}

/**
Fecha o arquivo cujo handle é o parametro
Retorna sinalização de erro
Retorna mensagem de operação completada
*/
void cmdClose(void) {
    FILE2 handle;

    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }

    if (sscanf(token, "%d", &handle)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    int err = close2(handle);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }

    printf ("Closed file with handle %d\n", handle);
}


void cmdRead(void) {
    FILE2 handle;
    int size;

    // get first parameter => file handle
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &handle)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    // get second parameter => number of bytes
    token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &size)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    // Alloc buffer for reading file
    char *buffer = malloc(size);
    if (buffer==NULL) {
        printf ("Memory full\n");
        return;
    }

    // get file bytes
    int err = read2(handle, buffer, size);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }
    if (err==0) {
        printf ("Empty file\n");
        return;
    }

    // show bytes read
    dump(buffer, err);
    printf ("%d bytes read from file-handle %d\n", err, handle);
    
    free(buffer);
}


void cmdLn(void) {
	char *linkname;
	int err;

    // get first parameter => link name
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter LINKNAME\n");
        return;
    }
	linkname = token;
	
    // get second parameter => pathname
    token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter PATHNAME\n");
        return;
    }

	// make link
    err = ln2 (linkname, token);
    if (err!=0) {
        printf ("Error: %d\n", err);
        return;
    }

    printf ("Created link %s to file %s\n", linkname, token);

}


void cmdWrite(void) {
    FILE2 handle;
    int size;
    int err;

    // get first parameter => file handle
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &handle)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    // get second parameter => string
    token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    size = strlen(token);

    // get file bytes
    err = write2(handle, token, size);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }
    if (err!=size) {
        printf ("Erro: escritos %d bytes, mas apenas %d foram efetivos\n", size, err);
        return;
    }

    printf ("%d bytes writen to file-handle %d\n", err, handle);
}

/**
Cria um novo diretorio
*/
void cmdMkdir(void) {
    // get first parameter => pathname
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    // change working dir
    int err = mkdir2(token);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }

    printf ("Created new directory\n");
}

/**
Apaga um diretorio
*/
void cmdRmdir(void) {
    // get first parameter => pathname
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    // change working dir
    int err = rmdir2(token);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }

    printf ("Directory was erased\n");
}

void cmdLs(void) {

    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }

    // Abre o diretório pedido
    DIR2 d;
    d = opendir2(token);
    if (d<0) {
        printf ("Open dir error: %d\n", d);
        return;
    }

    // Coloca diretorio na tela
    DIRENT2 dentry;
    while ( readdir2(d, &dentry) == 0 ) {
        printf ("%c %8u %s\n", (dentry.fileType==0x02?'d':'-'), dentry.fileSize, dentry.name);
    }

    closedir2(d);


}


/**
Chama a função truncate2() da biblioteca e coloca o string de retorno na tela
*/
void cmdTrunc(void) {
    FILE2 handle;
    int size;

    // get first parameter => file handle
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &handle)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    // get second parameter => number of bytes
    token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &size)==0) {
        printf ("Invalid parameter\n");
        return;
    }
    
    // posiciona CP na posicao selecionada
    int err = seek2(handle, size);
    if (err<0) {
        printf ("Error seek2: %d\n", err);
        return;
    }
    
    // trunca
    err = truncate2(handle);
    if (err<0) {
        printf ("Error truncate2: %d\n", err);
        return;
    }

    // show bytes read
    printf ("file-handle %d truncated to %d bytes\n", handle, size );
}

void cmdSeek(void) {
    FILE2 handle;
    int size;

    // get first parameter => file handle
    char *token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &handle)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    // get second parameter => number of bytes
    token = strtok(NULL," \t");
    if (token==NULL) {
        printf ("Missing parameter\n");
        return;
    }
    if (sscanf(token, "%d", &size)==0) {
        printf ("Invalid parameter\n");
        return;
    }

    // seek
    int err = seek2(handle, size);
    if (err<0) {
        printf ("Error: %d\n", err);
        return;
    }

    printf ("Seek completado para a posicao %d\n", size);
    
}

