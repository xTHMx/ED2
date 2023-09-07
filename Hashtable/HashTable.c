#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define maxSize 192
#define PrimaryFile "iprimary.idx"
#define TitleFile "ititle.idx"
#define DataFile "filmes.dat"
#define TempFile "filmes.temp"

int deleteFlag = 0; //verifica se ouve exclusao de filme

//====================Estruturas========================
typedef struct Entry
{
    char key[6]; //3 iniciais do sobrenome do diretor, 2 ultimos digitos do ano do filme
    char *titulo;
    char *titulo_ori;
    char *diretor;
    char ano[4];
    char *pais;
    int nota;
    int somaBytes;

} Entry;

typedef struct PrimaryKey
{
    char key[6];
    int RNN;

} PrimaryKey;

typedef struct SecondaryKey
{
    char *title;
    char key[6];

} SecondaryKey;

//===================funçoes comparadoras para qsort =================
int cmpStringKey(const void *a, const void *b) //compara String
{
    PrimaryKey pa ,pb;
    pa = *(PrimaryKey*)a;
    pb = *(PrimaryKey*)b;

    return strcmp(pa.key, pb.key) ;
}

int cmpStringTitle(const void *a, const void *b) //compara String
{
    SecondaryKey sa , sb;
    sa = *(SecondaryKey*)a;
    sb = *(SecondaryKey*)b;

    return strcmp(sa.title, sb.title) ;
}

//========================Funçoes Principais============================

char* criaKey(char* diretor, char ano[4])
{
    char *key;
    int i;

    for(i = 0; i < 5; i++) //max size
    {
        if(i < 3)
        {
            key[i] = toupper(diretor[i]); //3 primeiras iniciais do sobrenome
        }
        else
        {
            key[i] = ano[i-1]; //2 ultimos digitos do ano
        }
    }

    //adiciona finalizador
    key[5] = '\0';

    return key;
}

//verifica se o arquivo existe
int arquivoExiste(FILE *f){
    if(f) return 1;
    return 0;
}

//verifica se existe a chave primaria
PrimaryKey* existeChavePrimaria(PrimaryKey **arr, int tamanho, char key[5])
{
    PrimaryKey *pKey = (PrimaryKey*) bsearch(key, *arr, tamanho, sizeof(PrimaryKey), &cmpStringKey);
    if(pKey)
    {
        return pKey;
    }

    return NULL;
}

//verifica se existe a chave secundaria
SecondaryKey* existeChaveSecundaria(SecondaryKey **arr, int tamanho, char title[100])
{
    SecondaryKey *sKey = (SecondaryKey*) bsearch(&title, *arr, tamanho, sizeof(SecondaryKey), &cmpStringTitle);
    if(sKey)
    {
        return sKey;
    }

    return NULL;
}

//cria os indices a partir de um arquivo de dados
void criaChavePrimaria(PrimaryKey **arrPrimary, int *tamanhoChaves)
{
    char tempKey[5];
    int tempRNN, i;
    FILE *chavesPrimarias = fopen(PrimaryFile, "r");
    
    //inicializar arquivos na memoria
    if(!chavesPrimarias )
    {
        printf("Sem Chaves primarias cadastradas, arquivo criado!\n");
        chavesPrimarias = fopen(PrimaryFile, "w");
        fclose(chavesPrimarias);

    }else
    {
        i = 0;
        while(fscanf(chavesPrimarias, " %s %d\n", tempKey, &tempRNN) != EOF) i++; //contador do numero de indices

        *arrPrimary =  malloc(i * sizeof(PrimaryKey)); //inicia o array
        fseek(chavesPrimarias, 0, SEEK_SET); //reseta o ponteiro do fluxo

        i = 0;
        while(fscanf(chavesPrimarias, " %s %d\n", tempKey, &tempRNN) != EOF)
        {
           strcpy((*arrPrimary)[i].key, tempKey);
           (*arrPrimary)[i].RNN = tempRNN;

           i++;
           (*tamanhoChaves)++;  
        }
        
        fclose(chavesPrimarias);
    }

}

//cria os indices a partir de um arquivo de dados
void criaChaveSecundaria(SecondaryKey **arrTitle, int *tamanhoChaves)
{
    char tempKey[5], tempString[100];
    char *tempTitle;   
    int i, j;
    FILE *chavesSecundarias = fopen(TitleFile, "r");

    if(!chavesSecundarias )
    {
        printf("Sem Chaves secundarias cadastradas, arquivo criado!\n");
        chavesSecundarias = fopen(TitleFile, "w");
        fclose(chavesSecundarias);

    }else
    {
        int tamanho;

        i = 0;
        if(fgetc(chavesSecundarias) != EOF) //se nao for um arquivo vazio
        {
            fseek(chavesSecundarias, 0, SEEK_SET);//reseta para a leitura
            while(fscanf(chavesSecundarias, " %[^\n]s", tempString) != EOF) i++; //contador do numero de indices

            *arrTitle = malloc(i * sizeof(SecondaryKey)); //inicia o array
            fseek(chavesSecundarias, 0, SEEK_SET); //reseta o ponteiro do fluxo

            i = 0;
            while(fscanf(chavesSecundarias, " %[^\n]s", tempString) != EOF)
            {
                tamanho = strlen(tempString); //tamanho da palavras

                tempTitle = malloc(sizeof(char) * (tamanho-6)); //-5 pelo espaço da Key
                strncpy(tempTitle, tempString, tamanho-5);
                tempTitle[tamanho-6] = '\0'; //n é adicionado automaticamente o \0

                for(j = 0; j < 5; j++) //copia key
                {
                    tempKey[j] = tempString[tamanho+j-5];
                }
                tempKey[5] = '\0';
                
                (*arrTitle)[i].title = malloc(sizeof(char) * (tamanho-4));
                strcpy((*arrTitle)[i].title, tempTitle);
                strcpy((*arrTitle)[i].key, tempKey);
                //printf("%s ",(*arrTitle)[i].key);

                i++;
            }
        }
        fclose(chavesSecundarias);
    }

}

Entry* criaEntry(char *key, char *titulo, char *titulo_ori, char *diretor, char ano[4], char *pais, int nota)
{
    Entry *e; 

    int soma = strlen(titulo) + strlen(diretor) + strlen(titulo_ori) + strlen(pais) + 5 + 4 + 1;

    if(soma <= 175)
    {
        e = malloc(sizeof(Entry));

        strcpy(e->key, key);
        e->titulo = malloc(sizeof(char) * (strlen(titulo) + 1));
        strcpy(e->titulo, titulo);
        e->titulo_ori = malloc(sizeof(char) * (strlen(titulo_ori) + 1));
        strcpy(e->titulo_ori, titulo_ori);
        e->diretor = malloc(sizeof(char) * (strlen(diretor) + 1));
        strcpy(e->diretor, diretor);
        strcpy(e->ano, ano);
        e->ano[4] = '\0';
        e->pais = malloc(sizeof(char) * (strlen(pais) + 1));
        strcpy(e->pais, pais);
        e->nota = nota;
        e->somaBytes = soma + 6; //6 dos separadores

        return e;
    }
    else
    {
        printf("\n Erro! Campos ultrapassam 175 bytes! \n");
    }

    return NULL;
}


//Cria uma entry por posiçao do fluxo
//imagine que o arquivo é um array de filmes
Entry* criaEntryPorPosicao(int index)
{
    char *token;
    char string[maxSize];
    char split[7][200];
    int i;

    FILE *dados = fopen(DataFile, "r");;
    Entry *entry = NULL;

    fseek(dados, index * (maxSize+1) , SEEK_SET); //posiciona no local desejado do fluxo (+1 pois no proximo fluxo ele começa 193 e nao 192 igual ao tamanho)
    if(dados)
    {
        fscanf(dados, " %[^#]s", string); //passa para a variavel o valor encontrado no arquivo

        //reparte a string para ter cada elemento da struct
        token = strtok(string, "@");
        i=0;
        while(token != NULL)
        {
            strcpy(split[i], token);
            token = strtok(NULL, "@");
            i++;
        }

        //cria o entry na memoria
        entry = criaEntry(split[0],split[1],split[2],split[3],split[4],split[5],atoi(split[6]));
        fclose(dados);

        return entry;
    }
    
    return NULL;

}

//Redimenciona os arrays de chaves na memoria, geralmente para apagar algo
void RedimencionaArrayChaves(PrimaryKey **pArr, SecondaryKey **sArr, int *tamanho, int posPrimary, int posSecond)
{
    int i;
    PrimaryKey tempP;
    SecondaryKey tempS;

    for(i = posPrimary; i < (*tamanho)-1; i++) //move o valor a ser removido ao final
    {
        tempP = (*pArr)[i];
        (*pArr)[i] = (*pArr)[i+1];
        (*pArr)[i+1] = tempP;
    }

    for(i = posSecond; i < (*tamanho)-1; i++) //move o valor a ser removido ao final
    {
        tempS = (*sArr)[i];
        (*sArr)[i] = (*sArr)[i+1];
        (*sArr)[i+1] = tempS;
    }

    //redimenciona
    *pArr = realloc(*pArr, sizeof(PrimaryKey)*((*tamanho)-1));
    *sArr = realloc(*sArr, sizeof(SecondaryKey)*((*tamanho)-1));

    (*tamanho)--; //reduz tamanho do array
   
}

//Busca um filme por chave primaria
Entry* buscaFilme(PrimaryKey **pArr, int tamanho, char key[5])
{
    Entry *entry = NULL;
    PrimaryKey *pKey = NULL;

    if(tamanho > 0) //se nao for vazia
    {
        pKey = existeChavePrimaria(pArr, tamanho, key); //busca na memoria
        if(pKey)
        {
            //leitura do dado do arquivo
            entry = criaEntryPorPosicao(pKey->RNN);
        }

        //se alocar corretamente devolvemos o Filme
        if(entry) return entry;
    }
    
    return NULL;
}

//busca filme por titulo
Entry* buscaFilmeTitulo(PrimaryKey **pArr, SecondaryKey **sArr, int tamanho, char *titulo)
{
    Entry* entry = NULL;
    SecondaryKey *sKey;
    int pos;

    if(tamanho > 0) //nao vazia
    {   
        sKey = existeChaveSecundaria(sArr, tamanho, titulo); //busca na memoria
        if(sKey)
        {
            entry = buscaFilme(pArr, tamanho, sKey->key);
        }

        if(entry) return entry; //retorna filme resultante

    }
    return NULL; //nao achou
}

//insere um novo filme
//char *titulo, char *titulo_ori, char *diretor, char ano[4], char *pais, int nota
void inserirFilme(PrimaryKey **pArr, SecondaryKey **sArr, int *tamanho, Entry *entry)
{      
    FILE *dados;
    int i;

    PrimaryKey pKey;
    SecondaryKey sKey;

    //adiciona a listas
    if(pArr && sArr)
    {
        *pArr = realloc(*pArr, ((*tamanho)+1) * sizeof(PrimaryKey));
        *sArr = realloc(*sArr, ((*tamanho)+1) * sizeof(SecondaryKey));
    }
    else
    {
       *pArr = malloc(sizeof(PrimaryKey));
        *sArr = malloc(sizeof(SecondaryKey));
    }

    //passa chaves para respectivos arrays na memoria
    sKey.title = malloc(sizeof(char) * (strlen(entry->titulo)+1));

    strcpy(pKey.key, entry->key);
    pKey.RNN = *tamanho;
    (*pArr)[*tamanho] = pKey;

    strcpy(sKey.title, entry->titulo);
    strcpy(sKey.key, entry->key);
    (*sArr)[*tamanho] = sKey;

    (*tamanho)++; //incrementa tamanho geral

    //adiciona em arquivo
    if(entry)
    {
        dados = fopen(DataFile, "a");
        if(!dados) //se nao existir eu crio
        {
            dados = fopen(DataFile, "w");
        }

        fprintf(dados, "%s@%s@%s@%s@%s@%s@%d", entry->key, entry->titulo, entry->titulo_ori, entry->diretor, entry->ano, entry->pais, entry->nota);
        for(i = 0; i < (maxSize - (entry->somaBytes)); i++) fprintf(dados,"#"); //preenche de dados
        fprintf(dados, "\n");

        fclose(dados);
    }

    free(entry); //desaloca obj temporario

    //ordena o array
    qsort(*pArr, *tamanho, sizeof(PrimaryKey), &cmpStringKey);
    qsort(*sArr, *tamanho, sizeof(SecondaryKey), &cmpStringTitle);

}

//remove um filme
void removerFilme(PrimaryKey **pArr, SecondaryKey **sArr, int *tamanho, char key[5])
{
    FILE *dados;
    PrimaryKey *pKey = existeChavePrimaria(pArr, *tamanho, key);
    int pos, i;
    int index[2]; //index de ambas as keys

    if(pKey)
    {
        //coloca flag para deletação
        dados = fopen(DataFile, "r+");
        pos = pKey->RNN * (maxSize+1);

        fseek(dados, pos, SEEK_SET);
        fprintf(dados, "**"); //marca para ser deletado
        fclose(dados);

        //busca posiçao da chave na memoria
        index[0] = 0;
        for(i = 0; i < (*tamanho); i++)
        {
            if(pKey->RNN == (*pArr)[i].RNN)
            { 
                index[0] = i;
            }
        }

        //busca posiçao da chave secudaria na memoria
        index[1] = 0;
        for(i = 0; i < (*tamanho); i++)
        {
            if(strcmp(pKey->key, (*sArr)[i].key) == 0)
            { 
                index[1] = i;
            }
        }

        //redimenciona o array
        RedimencionaArrayChaves(pArr, sArr, tamanho, index[0], index[1]);
        deleteFlag = 1; //flag ativado

        printf("\nFilme Removido, feche o programa para salva atualizacoes\n");
        
    }
    else
    {
        printf("\nChave nao encontrada!\n");
    }
    


}

//modifica a nota de um filme
void modificarNota(int memPos, int nota)
{
    int pos = memPos * maxSize;
    int counter = 0;
    char c;

    FILE *dados = fopen(DataFile, "r+");

    if(dados)
    {
        while(counter < 6 && (c=fgetc(dados)) != EOF ) //procura até achar a nota
        {
            if(c == '@') counter++;
        }

        if(counter == 6)
        {
        fprintf(dados, "%d", nota); 
        printf("Modificado!\n");
        } 

        fclose(dados);
    }
    

}

//mostra todos os filmes cadastrados
void listarFilmes(int tamanho)
{
    FILE *dados = fopen(DataFile, "r");
    char string[maxSize];
    int i;
    Entry *entry;

    if(dados)
    {
        for(i = 0; i < tamanho; i++)
        {
            entry = criaEntryPorPosicao(i); //cria uma filme(entry) temporario na memoria

            //printa o titulo atual
            printf("Chave: %s Titulo: %s Titulo Original: %s Diretor: %s Ano: %s Pais: %s Nota: %d \n",
                         entry->key,entry->titulo,entry->titulo_ori,entry->diretor,entry->ano, entry->pais, entry->nota);

            free(entry);
        }

        fclose(dados);
    }
    else
    {
        printf("Erro! arquivo de filmes inexistente!");
    }

}

//funçao que possibilita compactar os registros, removendo os excluidos do arquivo de dados
void compactar(PrimaryKey **pArr, int tamanho)
{

    FILE *novoDados = fopen(TempFile, "w");
    Entry *entry;
    int i, j;

    //escrever os dados em outro arquivo, sem os que foram excluidos
    for(i = 0; i < tamanho; i++)
    {
        entry = criaEntryPorPosicao((*pArr)[i].RNN); //cria filme a partir de chave para reescrever arquivo
        (*pArr)[i].RNN = i;

        fprintf(novoDados, "%s@%s@%s@%s@%s@%s@%d", entry->key, entry->titulo, entry->titulo_ori, entry->diretor, entry->ano, entry->pais, entry->nota);
        for(j = 0; j < (maxSize - (entry->somaBytes)); j++) fprintf(novoDados,"#"); //preenche de dados
        fprintf(novoDados, "\n");

    }

    fclose(novoDados);
    
    //trocar nomes dos arquivos temporarios
    remove(DataFile); //deleta
    rename(TempFile, DataFile); //renomeia

    qsort(*pArr, tamanho, sizeof(PrimaryKey), &cmpStringKey); //reorganiza as chaves primarias

}

//=============== Funçoes de operaçao sobre uma opçao selecionada  ====================================================================

void OpAdicionarFilme(PrimaryKey **pArr, SecondaryKey **sArr , int *tamanho)
{
    char titulo[100], titulo_ori[100], diretor[50], ano[4], pais[20];
    int nota;

    printf("\nDigite o Titulo do Filme:\n");
    scanf(" %[^\n]s", titulo);
    printf("Digite o Titulo Original do Filme:\n");
    scanf(" %[^\n]s", titulo_ori);
    printf("Digite o Diretor do Filme: (Siga o padrao: Sobrenome, Nome)\n");
    scanf(" %[^\n]s", diretor);
    printf("Digite o ano do lancamento do Filme:\n");
    scanf(" %s", ano);
    printf("Digite o Pais Original do Filme:\n");
    scanf(" %s", pais);
    printf("Digite a nota do Filme:\n");
    scanf(" %d", &nota);

    char *key = criaKey(diretor, ano);  
    Entry *entry = criaEntry(key, titulo, titulo_ori, diretor, ano, pais, nota);

    if(entry)
    {
        inserirFilme(pArr, sArr, tamanho, entry);
    }
    else
    {
        printf("Nao foi possivel criar a instancia do filme! Cheque os valores inseridos!");
    }
    

}

void OpRemoverFilme(PrimaryKey **pArr, SecondaryKey **sArr , int *tamanho)
{
    char key[6];

    printf("\nDigite a chave do filme que deseja remover:\n");
    scanf(" %s", key);

    removerFilme(pArr, sArr, tamanho, key);
}

//modifica a nota do filme
void OpModificarFilme(PrimaryKey **pArr, int *tamanho)
{
    char key[6];
    int nota;

    PrimaryKey *pKey;

    printf("\nDigite a chave do filme que deseja alterar a nota:\n");
    scanf("%s", key);

    pKey = existeChavePrimaria(pArr, *tamanho, key);
    if(pKey)
    {
        printf("Digite a nova nota:\n");
        scanf(" %d", &nota);
        while(nota < 0 && nota > 10)
        {      
            printf("Nota Invalida, Digite outra entre 0-9:\n");
            scanf(" %d", &nota);
        }   

        modificarNota(pKey->RNN, nota);

    }else{
        printf("Chave nao encontrada!\n");
    }

}

void OpBuscarFilme(PrimaryKey **pArr, SecondaryKey **sArr , int tamanho)
{
    int op = -1;
    char key[5];
    char titulo[100];

    Entry *entry;

    while(op != 0)
    {
        printf("\nQual o metodo de busca?\n");
        printf("1 - Por Chave\n");
        printf("2 - Por Titulo\n");
        printf("0 - Voltar\n");

        scanf(" %d", &op);

        switch(op)
        {
            default: printf("Opçao invalida!"); break;
            case 0: break;
            case 1:
                printf("\nDigite o valor da chave:\n");
                scanf(" %s", key);

                entry = buscaFilme(pArr, tamanho, key);
                if(entry)
                {
                    printf("Chave: %s Titulo: %s Titulo Original: %s Diretor: %s Ano: %s Pais: %s Nota: %d \n",
                         entry->key,entry->titulo,entry->titulo_ori,entry->diretor,entry->ano, entry->pais, entry->nota);
                    free(entry);
                }
                else
                {
                    printf("Não foi possivel encontrar!\n");
                }

                break;

            case 2:
                printf("\nDigite o titulo do filme:\n");
                scanf(" %[^\n]s", titulo);

                entry = buscaFilmeTitulo(pArr, sArr, tamanho, titulo);
                if(entry)
                {
                    printf("Chave: %s Titulo: %s Titulo Original: %s Diretor: %s Ano: %s Pais: %s Nota: %d \n",
                         entry->key,entry->titulo,entry->titulo_ori,entry->diretor,entry->ano, entry->pais, entry->nota);
                    free(entry);
                }
                else
                {
                    printf("Não foi possivel encontrar!\n");
                }

                break;

        }
    }


}

//sai do programa salvando todos os dados em arquivos
void OpSair(PrimaryKey **pArr, SecondaryKey **sArr, int tamanho)
{
    //verifica se o arquivo n esta aberto, e abre ele pra escrita
    FILE *pFile = fopen(PrimaryFile, "w");
    FILE *sFile = fopen(TitleFile, "w");

    PrimaryKey pKey;
    SecondaryKey sKey;

    for(int i = 0; i < tamanho; i++)
    {
        pKey = (*pArr)[i];
        sKey = (*sArr)[i];

        //salva em arquivos
        if(deleteFlag)
        {
            fprintf(pFile, "%s %d\n", pKey.key, i); //reajusta o RNN dos dados de acordo com a novas posiçoes
            fprintf(sFile, "%s %s\n", sKey.title, sKey.key);
        }
        else
        {
            fprintf(pFile, "%s %d\n", pKey.key, pKey.RNN);
            fprintf(sFile, "%s %s\n", sKey.title, sKey.key);
        }            
        
        
    }

    //fecha arquivos
    fclose(sFile);
    fclose(pFile);

    //compacta -> remove filmes excluidos
    if(deleteFlag == 1)
    {
        compactar(pArr, tamanho);
    }

    //libera memoria
    free(*pArr);
    free(*sArr);
    
}

//================================================== Main =============================================================

int main(int argc, char const *argv[])
{
    int op = -1;
    int tamanhoChaves = 0; 

    PrimaryKey *arrPrimary = NULL;
    SecondaryKey *arrTitle = NULL;

    //cria os indices na memoria
    criaChavePrimaria(&arrPrimary,&tamanhoChaves);
    criaChaveSecundaria(&arrTitle,&tamanhoChaves);

    //listarFilmes(tamanhoChaves); //debug

    //Selecionar açao
    while(op != 0)
    {
        printf("\nEscolha o que deseja fazer:\n");
        printf("1 - Adicionar Filme\n");
        printf("2 - Remover Filme\n");
        printf("3 - Modificar nota do Filme\n");
        printf("4 - Buscar Filme\n");
        printf("5 - Listar Filmes\n");
        printf("0 - Sair\n");

        scanf(" %d", &op);

        switch(op)
        {
            case 1: OpAdicionarFilme(&arrPrimary, &arrTitle, &tamanhoChaves); break;
            case 2: OpRemoverFilme(&arrPrimary, &arrTitle, &tamanhoChaves); break;
            case 3: OpModificarFilme(&arrPrimary, &tamanhoChaves); break;
            case 4: OpBuscarFilme(&arrPrimary, &arrTitle, tamanhoChaves); break;
            case 5: listarFilmes(tamanhoChaves); break;
            case 0:  OpSair(&arrPrimary, &arrTitle, tamanhoChaves); break;

            default: printf("Opçao Invalida!"); break;
        }
    }

    
    printf("\nFim\n");
    return 0;
}