#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../include/apidisk.h"
#include "../include/t2fs.h"


#define ERRO -1
#define SUCESSO 0

/** Item on the open files list */
typedef struct OPENFILE_s{
	int ocupado;					//0 = closed, 1 = open
	BYTE type;						//0 = invalid, 1 = file, 2 = directory
	DWORD firstCluster;				//Number of the first cluster used
	int cp;							//Current pointer (points to the byte "in use")
	int size;						//File size in bytes
    DWORD clusterPai;                //numero do cluster do pai
    char name[MAX_FILE_NAME_SIZE];   //nome do arquivo
} OPENFILE_t;

typedef struct DirEntry_s{
	DIRENT2 *hDir;
	int ocupado;  // se ==0, está livre; se !=0, está ocupado
	DWORD firstCluster;
}DIRENTRY_t;


// globais
int iniciar_fs = 0;
char* diretorio_corrente;
char raiz = "/";
struct t2fs_superbloco* superbloco;
DWORD cluster_diretorio_corrente;

DIRENTRY_t* diretorios[MAX_DIR];
OPENFILE_t arquivos[MAX_ARQ];






//funcao inicializa FS.
int iniciar ()
{
    int i;
    int j;
    BYTE buffer[SECTOR_SIZE];

    if (iniciar_fs == 0)
    {
        // ler superbloco.
        if(read_sector(0,(unsigned char *) &buffer) != 0)
            return ERRO;
        else
        {
            superbloco = malloc(sizeof(unsigned char)*32);
            memcpy(superbloco,buffer,32);
        }

        // reserva tamanho do diretorio_corrente na memória.
        diretorio_corrente = malloc((sizeof(char)*MAX_FILE_NAME_SIZE));

        // diretorio_corrente recebe endereço da raiz.
        strcpy(diretorio_corrente, raiz);

        // diretório corrente recebe p número do cluster.
        cluster_diretorio_corrente = superbloco->RootDirCluster;



		for (i = 0; i <MAX_DIR ; ++i) {
			if(diretorios[i] == NULL){
				diretorios[i] = malloc(sizeof(struct DirEntry_s));
				diretorios[i]->ocupado =0;
			}
		}

		for (j = 0; j < MAX_FILE; ++j) {
			if(arquivos[j] == NULL){
				arquivos[j] = malloc(sizeof(OPENFILE_t));
				arquivos[j]->ocupado =0;
            }
		}


    }
    iniciar_fs = 1;
    return SUCESSO;
}


/*-----------------------------------------------------------------------------
Função: Usada para identificar os desenvolvedores do T2FS.
	Essa função copia um string de identificação para o ponteiro indicado por "name".
	Essa cópia não pode exceder o tamanho do buffer, informado pelo parâmetro "size".
	O string deve ser formado apenas por caracteres ASCII (Valores entre 0x20 e 0x7A) e terminado por ‘\0’.
	O string deve conter o nome e número do cartão dos participantes do grupo.

Entra:	name -> buffer onde colocar o string de identificação.
	size -> tamanho do buffer "name" (número máximo de bytes a serem copiados).

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int identify2 (char *name, int size)
{
    char names[] = "Vinicius Roratto Carvalho - 00160094\n"
                    "Augusto Timm do Espirito Santo - 00113887";

    if(size<sizeof(names))
        return ERRO;
    else
        strcpy(name,names);

    return SUCESSO;
}

int getHandleArq()
{
	int i, max;
	max = MAX_ARQ;
	for (i=0; i<max; ++i) {
		if(arquivos[i]->ocupado == 0){
			return i;
		}
	}
	return ERRO;
}


void filenameParaArray(char *str,char *array[])
{
	int index = 0;
	array[index] = strtok(str,"/");
	while(array[index]!=NULL){
		array[++index] = strtok(NULL,"/");
	}
}

DWORD buscaPrimeiroSetor(DWORD nro_cluster){
	DWORD n = nro_cluster*superbloco->SectorsPerCluster + superbloco->DataSectorStart;
	return n;
}


struct t2fs_record* busca_entrada_arquivo(DWORD nro_cluster, char *nomeArquivo){

	BYTE buffer[SECTOR_SIZE];
	DWORD primeiroSetor = buscaPrimeiroSetor(nro_cluster);
	struct t2fs_record* t2fsRecord = malloc(sizeof(struct t2fs_record)*4);
	int entradaDir =0;


	int i,j;
	for (i = 0; i < superbloco->SectorsPerCluster; ++i) {

		read_sector(primeiroSetor+i,(unsigned char *)buffer);
		memcpy(t2fsRecord,&buffer[0],SECTOR_SIZE);

		for (j = 0; j < 4; ++j) {
			//printf("    Entrada de diretorio %d: %s\n",entradaDir,t2fsRecord[j].name);
			if(strcmp(t2fsRecord[j].name,nomeArquivo)==0 && t2fsRecord[j].TypeVal==0x01){
				//printf("Encontrou arquivo com nome: %s\n",nomeArquivo);

				struct t2fs_record* retorno = malloc(sizeof(struct t2fs_record));
				memcpy(retorno,&t2fsRecord[j], sizeof(struct t2fs_record));
				return retorno;
			}
			entradaDir++;
		}
	}
	return NULL;
}

int gravarFAT(int entry, int value){
	int sector, sectorOffset, byteOffset;
	BYTE sectorBuffer[SECTOR_SIZE];

	sectorOffset = entry/(SECTOR_SIZE/4);
	sector = superbloco->pFATSectorStart + sectorOffset;

	if(read_sector(sector, sectorBuffer) != 0){
		return ERRO;
	}

	byteOffset = (entry%(SECTOR_SIZE/4))*4;	//Calculates where in the sector is the entry
	memcpy(&sectorBuffer[byteOffset], &value, 4);	//Changes the entry

	//Writes the updated sector back to disk
	if(write_sector(sector, sectorBuffer) != 0){
		//printf("Error writing FAT.\n");
		return ERRO;
	}

	return SUCESSO;
}



DWORD lerFAT(int entry){
	int sector, sectorOffset, byteOffset;
	BYTE sectorBuffer[SECTOR_SIZE];
	int content;

	sectorOffset = entry/(SECTOR_SIZE/4);
	sector = superbloco->pFATSectorStart + sectorOffset;

	read_sector(sector, sectorBuffer);

	byteOffset = (entry%(SECTOR_SIZE/4))*4;
	memcpy(&content, &sectorBuffer[byteOffset], 4);
	return content;
}


DWORD gravarRegistro(DWORD nro_cluster, struct t2fs_record *record){
	BYTE buffer[SECTOR_SIZE];
	DWORD primeiroSetor = getFirstSectorOfCluster(nro_cluster);
	struct t2fs_record* t2fsRecord = malloc(sizeof(struct t2fs_record)*4);
	int entradaDir =0;


	int i;
	int j;
	for (i = 0; i < superbloco->SectorsPerCluster; ++i) {

		read_sector(primeiroSetor+i,(unsigned char *)buffer);
		memcpy(t2fsRecord,&buffer[0],SECTOR_SIZE);

		for (j = 0; j < 4; ++j) {
			if(t2fsRecord[j].TypeVal==0x00){

				memcpy(&t2fsRecord[j],record, sizeof(struct t2fs_record));

				memcpy(&buffer[0],t2fsRecord,SECTOR_SIZE);

				if(write_sector(primeiroSetor+i,(unsigned char*)buffer)!=0){

					return ERRO;
				}
				return SUCESSO;
			}
			entradaDir++;
		}
	}
	return ERRO;
}
}



DWORD existDir(DWORD dirClusterNumber,char *filename)
{
	BYTE buffer[SECTOR_SIZE];
	DWORD primeiroSetor = buscaPrimeiroSetor(dirClusterNumber);
	struct t2fs_record* t2fsRecord = malloc(sizeof(struct t2fs_record)*4);
	int entradaDir =0;

	//printf("Verificando se exite diretorio %s no cluster: 0x%08X:\n",filename,dirClusterNumber);

	int i,j;
	for (i = 0; i < superbloco->SectorsPerCluster; ++i) {

		read_sector(primeiroSetor+i,(unsigned char *)buffer);
		memcpy(t2fsRecord,&buffer[0],SECTOR_SIZE);

		for (j = 0; j < 4; ++j) {
			//printf("    Entrada de diretorio %d: %s\n",entradaDir,t2fsRecord[j].name);
			if(strcmp(t2fsRecord[j].name,filename)==0 && t2fsRecord[j].TypeVal==0x02){

				/*printf("Achou diretorio chamado %s dentro do cluster 0x%08X\n",
					   t2fsRecord[j].name,
					   dirClusterNumber);*/

				//printf("Retornando record->firstCluster: 0x%08X\n\n",t2fsRecord[j].firstCluster);
				return t2fsRecord[j].firstCluster;
			}
			entradaDir++;
		}
	}
	return -1;

}

int gravarFAT(int entry, int value){
	int sector, sectorOffset, byteOffset;
	BYTE sectorBuffer[SECTOR_SIZE];

	sectorOffset = entry/(SECTOR_SIZE/4);
	sector = superbloco->pFATSectorStart + sectorOffset;

	if(read_sector(sector, sectorBuffer) != 0){
		return ERRO;
	}

	byteOffset = (entry%(SECTOR_SIZE/4))*4;	//Calculates where in the sector is the entry
	memcpy(&sectorBuffer[byteOffset], &value, 4);	//Changes the entry

	//Writes the updated sector back to disk
	if(write_sector(sector, sectorBuffer) != 0){
		//printf("Error writing FAT.\n");
		return ERRO;
	}

	return SUCESSO;
}



DWORD lerFAT(int entry){
	int sector, sectorOffset, byteOffset;
	BYTE sectorBuffer[SECTOR_SIZE];
	int content;

	sectorOffset = entry/(SECTOR_SIZE/4);
	sector = superbloco->pFATSectorStart + sectorOffset;

	read_sector(sector, sectorBuffer);

	byteOffset = (entry%(SECTOR_SIZE/4))*4;
	memcpy(&content, &sectorBuffer[byteOffset], 4);
	return content;
}


/*-----------------------------------------------------------------------------
Função: Criar um novo arquivo.
	O nome desse novo arquivo é aquele informado pelo parâmetro "filename".
	O contador de posição do arquivo (current pointer) deve ser colocado na posição zero.
	Caso já exista um arquivo ou diretório com o mesmo nome, a função deverá retornar um erro de criação.
	A função deve retornar o identificador (handle) do arquivo.
	Esse handle será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do arquivo criado.

Entra:	filename -> nome do arquivo a ser criado.

Saída:	Se a operação foi realizada com sucesso, a função retorna o handle do arquivo (número positivo).
	Em caso de erro, deve ser retornado um valor negativo.
-----------------------------------------------------------------------------*/
FILE2 create2 (char *filename)
{
    if(iniciar()==SUCESSO){

    	char* componente, path;
        char* buffer_path = malloc(sizeof(char)*strlen(filename));
        int handle = getHandleArq();
        int qtd = 0;
        DWORD nro_cluster;
        DWORD diretorio_pai;
        struct t2fs_record record = {0};

        strcpy(buffer_path, filename);

	// caso de arquivo nulo ou vazio.
        if (filename = NULL || strlen(filename)==0)
            return ERRO;

        if(handle < 0)
            return ERRO;

	while((componente = strsep(&path, "/")) != NULL)
	{
		if (strcmp(componente, "") == 0)
		}
			if (path == NULL)
				return ERRO;
			continue;
		}
		qtd++;

		if (strlen(componente) > MAX_FILE_NAME_SIZE)
			return ERRO;
		else
			memcpy(record.name, componente, MAX_FILE_NAME_SIZE);



	char *pathArray[qtd];
	filenameParaArray(filename, pathArray);

	nro_cluster = getNextFreeFatId();
	arquivos[handle]->ocupado=1;
	arquivos[handle]->cp=0;
	arquivos[handle]->type=0x01;
	arquivos[handle]->size=0;
	arquivos[handle]->firstCluster = nro_cluster;


	strcpy(arquivos[handle]->name, record.name);
	record.TypeVal-0x01;


	if (filename[0] == '/')
		diretorio_pai = superbloco->RootDirCluster;
	else
		diretorio_pai = cwdCluster;

	for(i=0; i < qtd-1; i++)
	{
		diretorio_pai = existDir(diretorio_pai, pathArray[i]);
		if(diretorio_pai==-1)
			return ERRO;
	}

	arquivos[handle]->clusterPai = diretorio_pai;

	if(busca_entrada_arquivo(diretorio_pai, pathArray[qtd -1])!= NULL)
		return ERRO;

	else
	{
		if(gravarRegistro(diretorio_pai, &record) == -1)
			return ERRO;
		lerFAT(nro_cluster);
		gravarFAT(clusterNunber, 0xFFFFFFFF);
		lerFAT(nro_cluster);
	}
    return handle;

    }else
        return ERRO;
}


/*-----------------------------------------------------------------------------
Função:	Apagar um arquivo do disco.
	O nome do arquivo a ser apagado é aquele informado pelo parâmetro "filename".

Entra:	filename -> nome do arquivo a ser apagado.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int delete2 (char *filename)
{
    if(iniciar()==SUCESSO)
    {

    DWORD nro_cluster, ultimo_cluster;
	char* componente;
	char* path;
	int qtd=0;

	path = malloc(sizeof(char)*strlen(filename));
	strcpy(path, filename);

	while ((componente = strsep(&path, "/")) != NULL)
    {
        if (strcmp(componente,"") == 0)
        {
            if(path == NULL)
                return ERRO;

        continue;
        }


        qtd++;
    }

    char *pathArray[qtd];
    nomeParaArray(filename,pathArray);

    if(qtd>0)
    {
        if(filename[0]=='/')
            nro_cluster = superbloco->RootDirCluster;
        else
            nro_cluster = cluster_diretorio_corrente;

        ultimo_cluster = nro_cluster;
		int i;
		for (i = 0; i < qtd-1; ++i)
        {
			nro_cluster = existDir(ultimo_cluster,pathArray[i]);

			if(nro_cluster!=-1)
				ultimo_cluster = nro_cluster;
			else
				return ERRO;
        }

		struct t2fs_record* registroArquivo = buscarEntrada(ultimo_cluster, pathArray[qtd - 1]);

		if(registroArquivo!=NULL && registroArquivo->TypeVal!=0x00)
        {

			DWORD entrada_FAT = registroArquivo->firstCluster;
			DWORD fatEntryDel;

			//deletando entrada na FAT relacionado ao arquivo
			// transformar isso em funcao
			while(entrada_FAT >= 0x00000002 && entrada_FAT<=0xFFFFFFFD){
				//printf("FAT entry: 0x%08X\n",entrada_FAT);
				fatEntryDel = entrada_FAT;
				entrada_FAT = lerFAT(fatEntryDel);
				gravarFAT(fatEntryDel, 0x00000000);
				lerFAT(fatEntryDel);
			}

			deletarEntrada(ultimoClusterEncontrado,registroArquivo->name);

			return 0;
		}
    }
    else
			return ERRO; // Erro: arquivo não existe.

    }
}

int deletarEntrada(DWORD clusterNumber,char* nomeArquivo){

	BYTE buffer[SECTOR_SIZE];
	DWORD firstClusterSector = getFirstSectorOfCluster(clusterNumber);
	struct t2fs_record* t2fsRecord = malloc(sizeof(struct t2fs_record)*4);
	int entradaDir =0;

	//printf("deleteFileEntry, Verificando se existe arquivo chamado: %s no cluster: 0x%08X\n",nomeArquivo,clusterNumber);

	int i,j;
	for (i = 0; i < superbloco->SectorsPerCluster; ++i) {

		read_sector(firstClusterSector+i,(unsigned char *)buffer);
		memcpy(t2fsRecord,&buffer[0],SECTOR_SIZE);

		for (j = 0; j < 4; ++j) {
			//printf("    Entrada de diretorio %d: %s\n",entradaDir,t2fsRecord[j].name);
			if(strcmp(t2fsRecord[j].name,nomeArquivo)==0 && t2fsRecord[j].TypeVal==0x01){
				//printf("Encontrou arquivo com nome: %s\n",nomeArquivo);

				struct t2fs_record* record = malloc(sizeof(struct t2fs_record));
				record->TypeVal=0x00;
				record->bytesFileSize=0;
				record->firstCluster=0xFFFFFFFF;

				memcpy(&t2fsRecord[j],record, sizeof(struct t2fs_record));

				memcpy(&buffer[0],t2fsRecord,SECTOR_SIZE);

				if(write_sector(firstClusterSector+i,(unsigned char*)buffer)!=0){
					//printf("Error deleting entry\n");
					return -2;
				}
				//printf("Deletou arquivo com sucesso!\n");
				return 0;


			}
			entradaDir++;
		}
	}
	return -1;
}

struct t2fs_record* buscarEntrada(DWORD clusterNumber, char *nomeArquivo){

	BYTE buffer[SECTOR_SIZE];
	DWORD firstClusterSector = getFirstSectorOfCluster(clusterNumber);
	struct t2fs_record* t2fsRecord = malloc(sizeof(struct t2fs_record)*4);
	int entradaDir =0;

	//printf("Verificando se existe arquivo chamado: %s no cluster: 0x%08X\n",nomeArquivo,clusterNumber);

	int i,j;
	for (i = 0; i < superbloco->SectorsPerCluster; ++i) {

		read_sector(firstClusterSector+i,(unsigned char *)buffer);
		memcpy(t2fsRecord,&buffer[0],SECTOR_SIZE);

		for (j = 0; j < 4; ++j) {
			//printf("    Entrada de diretorio %d: %s\n",entradaDir,t2fsRecord[j].name);
			if(strcmp(t2fsRecord[j].name,nomeArquivo)==0 && t2fsRecord[j].TypeVal==0x01){
				//printf("Encontrou arquivo com nome: %s\n",nomeArquivo);

				struct t2fs_record* retorno = malloc(sizeof(struct t2fs_record));
				memcpy(retorno,&t2fsRecord[j], sizeof(struct t2fs_record));
				return retorno;
			}
			entradaDir++;
		}
	}
	return NULL;
}

int validaNome(char* nome){

	//printf("size %d\n",strlen(nome));
	if(strlen(nome)>MAX_FILE_NAME_SIZE){
		//printf("Name is too long.\n");
		return ERRO;
	}
	int i;
	for (i = 0; i <strlen(nome) ; ++i) {
		if(nome[i]<0x21||nome[i]>0x7a){
			return ERRO;
		}

	}
	return SUCESSO;
}

void nomeParaArray(char *str,char *array[])
{
	int index = 0;
	array[index] = strtok(str,"/");
	while(array[index]!=NULL)
    {
		array[++index] = strtok(NULL,"/");
	}
}




/*-----------------------------------------------------------------------------
Função:	Abre um arquivo existente no disco.
	O nome desse novo arquivo é aquele informado pelo parâmetro "filename".
	Ao abrir um arquivo, o contador de posição do arquivo (current pointer) deve ser colocado na posição zero.
	A função deve retornar o identificador (handle) do arquivo.
	Esse handle será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do arquivo criado.
	Todos os arquivos abertos por esta chamada são abertos em leitura e em escrita.
	O ponto em que a leitura, ou escrita, será realizada é fornecido pelo valor current_pointer (ver função seek2).

Entra:	filename -> nome do arquivo a ser apagado.

Saída:	Se a operação foi realizada com sucesso, a função retorna o handle do arquivo (número positivo)
	Em caso de erro, deve ser retornado um valor negativo
-----------------------------------------------------------------------------*/
FILE2 open2 (char *filename)
{

}


/*-----------------------------------------------------------------------------
Função:	Fecha o arquivo identificado pelo parâmetro "handle".

Entra:	handle -> identificador do arquivo a ser fechado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int close2 (FILE2 handle);
{
    if(iniciar()==SUCESSO)
    {
        if(handle<0 || handle>MAX_DIR)
            return ERRO;

        if(arquivos[handle]->ocupado == 0)
            return ERRO;

        arquivos[handle]->ocupado=0;
        struct t2fs_record* record = malloc(sizeof(struct t2fs_record));

        record->bytesFileSize = arquivos[handle]->size;
        strcpy(record->name, arquivos[handle]->name);
        record->TypeVal = arquivos[handle]->type;
        record->firstCluster = arquivos[handle]->firstCluster;

        updateEntry(arquivos[handle]->clusterPai,record);

        return SUCESSO;


    }
    else
        return ERRO;
}


/*





    struct t2fs_record* record = malloc(sizeof(struct t2fs_record));

    record->bytesFileSize = fileList[handle]->size;
    strcpy(record->name, fileList[handle]->name);
    record->TypeVal = fileList[handle]->type;
    record->firstCluster = fileList[handle]->firstCluster;

    updateEntry(fileList[handle]->clusterPai,record);

	return 0;
}
*/


/*-----------------------------------------------------------------------------
Função:	Realiza a leitura de "size" bytes do arquivo identificado por "handle".
	Os bytes lidos são colocados na área apontada por "buffer".
	Após a leitura, o contador de posição (current pointer) deve ser ajustado para o byte seguinte ao último lido.

Entra:	handle -> identificador do arquivo a ser lido
	buffer -> buffer onde colocar os bytes lidos do arquivo
	size -> número de bytes a serem lidos

Saída:	Se a operação foi realizada com sucesso, a função retorna o número de bytes lidos.
	Se o valor retornado for menor do que "size", então o contador de posição atingiu o final do arquivo.
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
int read2 (FILE2 handle, char *buffer, int size);


/*-----------------------------------------------------------------------------
Função:	Realiza a escrita de "size" bytes no arquivo identificado por "handle".
	Os bytes a serem escritos estão na área apontada por "buffer".
	Após a escrita, o contador de posição (current pointer) deve ser ajustado para o byte seguinte ao último escrito.

Entra:	handle -> identificador do arquivo a ser escrito
	buffer -> buffer de onde pegar os bytes a serem escritos no arquivo
	size -> número de bytes a serem escritos

Saída:	Se a operação foi realizada com sucesso, a função retorna o número de bytes efetivamente escritos.
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
int write2 (FILE2 handle, char *buffer, int size);


/*-----------------------------------------------------------------------------
Função:	Função usada para truncar um arquivo.
	Remove do arquivo todos os bytes a partir da posição atual do contador de posição (CP)
	Todos os bytes a partir da posição CP (inclusive) serão removidos do arquivo.
	Após a operação, o arquivo deverá contar com CP bytes e o ponteiro estará no final do arquivo

Entra:	handle -> identificador do arquivo a ser truncado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int truncate2 (FILE2 handle);


/*-----------------------------------------------------------------------------
Função:	Reposiciona o contador de posições (current pointer) do arquivo identificado por "handle".
	A nova posição é determinada pelo parâmetro "offset".
	O parâmetro "offset" corresponde ao deslocamento, em bytes, contados a partir do início do arquivo.
	Se o valor de "offset" for "-1", o current_pointer deverá ser posicionado no byte seguinte ao final do arquivo,
		Isso é útil para permitir que novos dados sejam adicionados no final de um arquivo já existente.

Entra:	handle -> identificador do arquivo a ser escrito
	offset -> deslocamento, em bytes, onde posicionar o "current pointer".

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int seek2 (FILE2 handle, DWORD offset);


/*-----------------------------------------------------------------------------
Função:	Criar um novo diretório.
	O caminho desse novo diretório é aquele informado pelo parâmetro "pathname".
		O caminho pode ser ser absoluto ou relativo.
	São considerados erros de criação quaisquer situações em que o diretório não possa ser criado.
		Isso inclui a existência de um arquivo ou diretório com o mesmo "pathname".

Entra:	pathname -> caminho do diretório a ser criado

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int mkdir2 (char *pathname);


/*-----------------------------------------------------------------------------
Função:	Apagar um subdiretório do disco.
	O caminho do diretório a ser apagado é aquele informado pelo parâmetro "pathname".
	São considerados erros quaisquer situações que impeçam a operação.
		Isso inclui:
			(a) o diretório a ser removido não está vazio;
			(b) "pathname" não existente;
			(c) algum dos componentes do "pathname" não existe (caminho inválido);
			(d) o "pathname" indicado não é um diretório;

Entra:	pathname -> caminho do diretório a ser removido

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int rmdir2 (char *pathname);


/*-----------------------------------------------------------------------------
Função:	Altera o diretório atual de trabalho (working directory).
		O caminho desse diretório é informado no parâmetro "pathname".
		São considerados erros:
			(a) qualquer situação que impeça a realização da operação
			(b) não existência do "pathname" informado.

Entra:	pathname -> caminho do novo diretório de trabalho.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
		Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int chdir2 (char *pathname);


/*-----------------------------------------------------------------------------
Função:	Informa o diretório atual de trabalho.
		O "pathname" do diretório de trabalho deve ser copiado para o buffer indicado por "pathname".
			Essa cópia não pode exceder o tamanho do buffer, informado pelo parâmetro "size".
		São considerados erros:
			(a) quaisquer situações que impeçam a realização da operação
			(b) espaço insuficiente no buffer "pathname", cujo tamanho está informado por "size".

Entra:	pathname -> buffer para onde copiar o pathname do diretório de trabalho
		size -> tamanho do buffer pathname

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
		Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int getcwd2 (char *pathname, int size);


/*-----------------------------------------------------------------------------
Função:	Abre um diretório existente no disco.
	O caminho desse diretório é aquele informado pelo parâmetro "pathname".
	Se a operação foi realizada com sucesso, a função:
		(a) deve retornar o identificador (handle) do diretório
		(b) deve posicionar o ponteiro de entradas (current entry) na primeira posição válida do diretório "pathname".
	O handle retornado será usado em chamadas posteriores do sistema de arquivo para fins de manipulação do diretório.

Entra:	pathname -> caminho do diretório a ser aberto

Saída:	Se a operação foi realizada com sucesso, a função retorna o identificador do diretório (handle).
	Em caso de erro, será retornado um valor negativo.
-----------------------------------------------------------------------------*/
DIR2 opendir2 (char *pathname);


/*-----------------------------------------------------------------------------
Função:	Realiza a leitura das entradas do diretório identificado por "handle".
	A cada chamada da função é lida a entrada seguinte do diretório representado pelo identificador "handle".
	Algumas das informações dessas entradas serão colocadas no parâmetro "dentry".
	Após realizada a leitura de uma entrada, o ponteiro de entradas (current entry) deve ser ajustado para a próxima entrada válida, seguinte à última lida.
	São considerados erros:
		(a) qualquer situação que impeça a realização da operação
		(b) término das entradas válidas do diretório identificado por "handle".

Entra:	handle -> identificador do diretório cujas entradas deseja-se ler.
	dentry -> estrutura de dados onde a função coloca as informações da entrada lida.

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero ( e "dentry" não será válido)
-----------------------------------------------------------------------------*/
int readdir2 (DIR2 handle, DIRENT2 *dentry);


/*-----------------------------------------------------------------------------
Função:	Fecha o diretório identificado pelo parâmetro "handle".

Entra:	handle -> identificador do diretório que se deseja fechar (encerrar a operação).

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int closedir2 (DIR2 handle);


/*-----------------------------------------------------------------------------
Função:	Função usada para criar um caminho alternativo (softlink) com o nome dado por linkname (relativo ou absoluto) para um arquivo ou diretório fornecido por filename.

Entra:	linkname -> nome do link a ser criado
	filename -> nome do arquivo ou diretório apontado pelo link

Saída:	Se a operação foi realizada com sucesso, a função retorna "0" (zero).
	Em caso de erro, será retornado um valor diferente de zero.
-----------------------------------------------------------------------------*/
int ln2(char *linkname, char *filename);
