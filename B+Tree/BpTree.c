#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

//é nessesario excluir arquivos antigos quando alterar o define!!!!
#define order 5 //significa ordem 6, levando em conta o 0 como elemento
#define maxSize 192
#define filmesFile "filmes.dat"
#define treeFile "ibtree.dat"
#define titleFile "ititle.idx"

//usando '!' para verificar key vazia

typedef struct Filme{
    char key[6]; //3 iniciais do sobrenome do diretor, 2 ultimos digitos do ano do filme
    char *titulo;
    char *titulo_ori;
    char *diretor;
    char ano[4];
    char *pais;
    int nota;
    int somaBytes;
}Filme;

typedef struct SecondaryKey
{
    char *title;
    char key[6];

} SecondaryKey;

typedef struct No { //No/Pagina da arvore
    //Metadados
    int RNN; // RNN do próprio nó no arquivo de indice (arvore B+)
    int isLeaf; // Verifica se o nó/página é folha

    //dados
    char keys[order][6]; // vetor com valores das chaves (order-1)==bucketSize*
    int dataRNN[order]; // RRNs de dados associados a cada chave (somente em folha)

    //ponteiros RNN
    int child[order + 1]; // Ponteiros** (RNNs dos nós no arquivo) para os filhos
    int parent; // referência para o pai
    int next_node; // referência para o próximo nó/página da lista (se o nó é folha)

    int numKeys; // número de chaves inseridas na página

} No;
//keys...#dataRnn...#child...#parent#next#isLeaf#numKeys

typedef struct BpTree{ //Struct da arvore
    int root; //RNN da root
    int pageCount;
    int dataCount;
    int degree;

}BpTree;

//====================================================== Funçoes Arvore =======================================================//

//Cria a arvore B+
BpTree* createTree(int degree)
{
    BpTree* tree = (BpTree*) malloc(sizeof(BpTree));
    tree->root = -1;
    tree->degree = degree - 1;
    tree->pageCount = 0;
    tree->dataCount = 0;

    return tree;
}

//carrega a pagina a partir de um RNN
No* loadNode(int RNN)
{
    FILE *fp = fopen(treeFile, "rb"); //abre p arquivo em binario
    No *load;
    
    if(RNN != -1)
    {
        if(fp)
        {
            load = (No*) malloc(sizeof(No));
            if(load != NULL)
            {
                fseek(fp, 9 + (RNN * sizeof(No)), SEEK_SET); //+9 por causa do flag
                fread(load, sizeof(No), 1, fp);
                
                fclose(fp);
                return load;
            }
            else
            {
                printf("ERRO > Alocação de elemento\n");
            }

        }
        else
        {
            printf("ERRO> Arquivo não encontrado!\n");
        }
    }

    fclose(fp);
    
    return NULL;
}

//Cria o nó vazio
No *createNode(BpTree *tree)
{
    int i;

    No* no = (No*) malloc(sizeof(No));
    if(no != NULL){
        no->RNN = tree->pageCount;
        no->numKeys = 0;
        no->isLeaf = 0;
        no->parent = -1;
        no->next_node = -1;
    
        for(i = 0; i < tree->degree; i++){
            strcpy(no->keys[i], "!");
            no->dataRNN[i] = -1;
            no->child[i] = -1;
        }
        no->child[i+1] = -1; //ultimo valor que sobrou sem definir -> reduzir loop

        tree->pageCount++;
    }

    return no;
}

//Sobrescreve o nó de um RNN com um novo nó editado
void overwriteNode(int RNN, No *new)
{
    FILE *fp = fopen(treeFile, "r+b"); //abre p arquivo em binario
    
    if(fp && RNN != -1)
    {
        if(new != NULL)
        {
            fseek(fp, 9 + (RNN * sizeof(No)), SEEK_SET); //8 é offset dos dados da arvore no geral +1 pra começar na 9 posiçao
            fwrite(new, sizeof(No), 1, fp);
        }
        else
        {
            printf("ERRO > Elemento NULL!\n");
        }

    }
    else
    {
        printf("ERRO> Arquivo não encontrado!\n");
    }

    fclose(fp);
}


//troca o RNN da raiz no arquivo
void changeRoot(BpTree *tree)
{
    FILE *fp = fopen(treeFile, "r+b"); //abre p arquivo em binario
    
    if(fp)
    {
        fseek(fp, 0, SEEK_SET);
        fprintf(fp, "%d/", tree->root);
        fprintf(fp, "%d/", tree->degree);
        fprintf(fp, "%d/", tree->pageCount);
        fprintf(fp, "%d/", tree->dataCount);
    }
    else
    {
        printf("ERRO> Arquivo não encontrado!\n");
    }

    fclose(fp);
}

//save or load tree to file
void loadTree(BpTree *tree)
{
    FILE *fp = fopen(treeFile, "rb");
    int data[4];

    if(fp != NULL)
    {
        for(int i = 0; i < 4; i++)
        {
            fscanf(fp, "%d", &data[i]);
            fgetc(fp); //gasta delimitador
        }

        tree->root = data[0];
        tree->degree = data[1];
        tree->pageCount = data[2];
        tree->dataCount = data[3];

        fclose(fp);
    }

}

//procura uma pagina na memoria -> nem sempre a chave esta nela, apenas carrega a pagina mais provavel
No* searchPage(No *no, char key[6])
{
    No *temp = no;
    int found;
    int i;

    if(no != NULL)
    {
        if(no->isLeaf == 1){
            //printf("page: %d,é folha %s\n", no->RNN, no->keys[0]);
            return no;
        }

        while(no->isLeaf == 0)
        {
            found = 0;
            for(i = 0; (i <= no->numKeys) && (found == 0); i++)
            {
                if(i == no->numKeys)
                {
                    if(no->child[i] != -1)
                    {
                        temp = no;
                        no = loadNode(no->child[i]);
                        free(temp);
                        found = 1; //necessita pra sair de um loop infinito

                    }else{
                        printf("Não encontrado\n");
                        return NULL;
                    }
                }

                if(strcmp(key, no->keys[i]) < 0)
                {
                    if(no->child[i] != -1)
                    {
                        temp = no;
                        no = loadNode(no->child[i]);
                        free(temp);
                    }

                    found = 1;
                }

            }
        }
    }
    else
    {
        printf("Arvore vazia!\n");
    }

    return no; //Null se não achar

}

void saveFilme(Filme *entry){
    FILE *dados = fopen(filmesFile, "a");
    int i;

    if(!dados) //se nao existir eu crio
    {
        dados = fopen(filmesFile, "w");
    }
    else
    {
        fprintf(dados, "%s@%s@%s@%s@%s@%s@%d", entry->key, entry->titulo, entry->titulo_ori, entry->diretor, entry->ano, entry->pais, entry->nota);
        for(i = 0; i < (maxSize - (entry->somaBytes)); i++) fprintf(dados,"#"); //preenche de dados
        fprintf(dados, "\n");
    }
    fclose(dados);
}

void printFilme(Filme *entry)
{
    printf("Chave: %s  Titulo: %s  Original: %s  Diretor %s  Ano: %s  País: %s  Nota: %d\n", 
            entry->key, entry->titulo, entry->titulo_ori, entry->diretor, entry->ano, entry->pais, entry->nota);
}

//insere em algum ponto da arvore
void insertInsideTree(BpTree *tree, Filme *fil, char curKey[6], No *parent, No *child){
    int i, j;
    
    No *new;
    No *temp;
    char tempKeys[tree->degree + 1][6]; //carrega keys com extra
    int datas[tree->degree+1];
    int ptrs[tree->degree + 2]; //carrega ponteiros RNN extra
    
    if(parent != NULL){
        if(parent->numKeys < tree->degree)
        {
            i=0;
            while(strcmp(curKey, parent->keys[i]) > 0 && i < parent->numKeys )
                i++;
                
            for (j = parent->numKeys; j > i; j--)
            {
                strcpy(parent->keys[j], parent->keys[j-1]);
                parent->dataRNN[j] = parent->dataRNN[j-1];
            }
            
            for (j = parent->numKeys+1; j > i+1; j--)
            {
                parent->child[j] = parent->child[j - 1];
            }
            
            strcpy(parent->keys[i], curKey); //salva o filme
            parent->dataRNN[i] = tree->dataCount;
            
            parent->numKeys++;
            parent->child[i + 1] = child->RNN;
            
            child->parent = parent->RNN; //troca o pai do inserido

            overwriteNode(parent->RNN, parent); //aplica a nova alteração no pai
            overwriteNode(child->RNN, child); //aplica a alteraçao no filho (cumulativo da anterior)
            free(parent);
        }
        else //split
        {
            new = createNode(tree);
            
            for (i = 0; i < tree->degree+1; i++) //copy old data
            {
                strcpy(tempKeys[i], parent->keys[i]);
                datas[i] = parent->dataRNN[i];
                ptrs[i] = parent->child[i];
            }
            ptrs[i+1] = parent->child[i+1];
            
            i = 0;
            while(strcmp(curKey, tempKeys[i]) > 0 && i < tree->degree)
                i++;
                
            for (j = tree->degree + 1; j > i; j--) //distribui chaves e ponteiros
            {
                strcpy(tempKeys[j], tempKeys[j-1]);
                datas[j] = datas[j-1];
            }
            
            for (j = tree->degree + 2; j > i+1; j--) //distribui chaves e ponteiros
            {
                ptrs[j] = ptrs[j - 1];
            }
            
            //adiciona chave
            strcpy(tempKeys[i], curKey);
            datas[i] = tree->dataCount;
            ptrs[i+1] = child->RNN;

            new->isLeaf = 0; //não é folha
            parent->numKeys = (tree->degree + 1) / 2; //recebe metade dos dados
            new->numKeys = tree->degree - ((tree->degree + 1) / 2);
            
            //reparte
            for (i = 0, j = parent->numKeys + 1; i < new->numKeys; i++, j++){
                strcpy(new->keys[i], tempKeys[j]);
                new->dataRNN[i] = datas[j];
            }

            for (i = 0, j = parent->numKeys + 1; i < new->numKeys + 1; i++, j++)
                new->child[i] = ptrs[j];
            
            if(parent->RNN == tree->root) //se o pai era a root da arvore
            {
                No* tempRoot, *tempParent;
                tempRoot = createNode(tree);
                tempParent = loadNode(parent->RNN);
                
                strcpy(tempRoot->keys[0], parent->keys[parent->numKeys]); //maior da esquerda
                tempRoot->child[0] = parent->RNN;
                tempRoot->child[1] = new->RNN;

                new->parent = tempRoot->RNN; //troca referencia do pai
                tempParent->parent = tempRoot->RNN;

                tempRoot->isLeaf = 0;
                tempRoot->numKeys++;             
                tree->root = tempRoot->RNN;

                //salva new e parent em arquivo
                overwriteNode(parent->RNN, tempParent);
                overwriteNode(new->RNN, new);
                overwriteNode(tempRoot->RNN, tempRoot);
                changeRoot(tree); //deu split, novo root

                //limpa
                free(new);
                free(parent);
                free(tempRoot);
                free(tempParent);
            }
            else
            {
                overwriteNode(parent->RNN, parent);
                overwriteNode(new->RNN, new);
                insertInsideTree(tree, fil, parent->keys[parent->numKeys], loadNode(parent->parent), new);
                //search vai ser usado somente para achar a pagina
            }
            
        }
    }
    else{
        printf("Parent é NULL!\n");
    }
    
}

//função principal de inserção
void insertTree(BpTree *tree, Filme *fil)
{
    int i, j;
    int inserted = 0;

    No *root = loadNode(tree->root);
    No *parent = NULL, *new = NULL;
    char keys[order+2][6]; //extra pra split
    int datas[order+2];
    No *temp;
    
    if(tree->root == -1) //primeiro elemento da arvore
    { 
        temp = createNode(tree);
        strcpy(temp->keys[0], fil->key);
        temp->isLeaf = 1;
        temp->numKeys++;
        tree->root = temp->RNN;

        //insere data
        saveFilme(fil); //write file
        temp->dataRNN[0] = tree->dataCount;
        tree->dataCount++;

        changeRoot(tree);
        overwriteNode(temp->RNN, temp);
        free(temp);
    }
    else
    {
        
        while(root->isLeaf == 0 && inserted != 1)
        {
            //parent = root; //salva o pai
            
            for (i = 0; (i < root->numKeys) && (inserted != 1); i++)
            {

                if(strcmp(fil->key , root->keys[i]) == 0) //se ja existir chave cadastrada
                {
                    printf("Erro - Chave já existe!");
                    return;
                }

                if (strcmp(fil->key , root->keys[i]) < 0)
                {
                    temp = root;
                    root = loadNode(root->child[i]);
                    free(temp); //libera pagina antiga
                    inserted = 1;
                }
                
                if (i == root->numKeys - 1) //ultimo elemento
                {
                    temp = root;
                    root = loadNode(root->child[i+1]);
                    free(temp);
                    inserted = 1;
                }
            }
        }
            
        if (root->numKeys < tree->degree) //se houver espaco
        {
            i = 0;
            while (strcmp(fil->key , root->keys[i]) > 0 && i < root->numKeys)
                i++;
            
            for (j = root->numKeys; j > i; j--){
                strcpy(root->keys[j], root->keys[j - 1]);
                root->dataRNN[j] = root->dataRNN[j-1];
            }

            //adiciona ordenado
            strcpy(root->keys[i], fil->key);
            saveFilme(fil);
            root->dataRNN[i] = tree->dataCount;
            tree->dataCount++;

            root->numKeys++;
            root->child[root->numKeys] = root->child[root->numKeys - 1];
            root->child[root->numKeys - 1] = -1; //n esquecer de adicionar o novo ponteiro
            
            overwriteNode(root->RNN, root);

            free(root);
        }
        else //requer split
        {
            new = createNode(tree);
            
            for (i = 0; i < tree->degree; i++){
                strcpy(keys[i], root->keys[i]);
                datas[i] = root->dataRNN[i];
            }
            
            i = 0;
            while (strcmp(fil->key, keys[i]) > 0 && i < tree->degree)
                i++;
    
            for (j = tree->degree + 1; j > i; j--){ //abre espaco para key inserida em meio virtual
                strcpy(keys[j], keys[j - 1]);
                datas[j] = datas[j-1];
            }

            //insere o filme
            strcpy(keys[i], fil->key);
            saveFilme(fil);
            datas[i] = tree->dataCount;
            tree->dataCount++;

            new->isLeaf = 1;
            root->isLeaf = 1;
            root->numKeys = (tree->degree + 1) / 2;
            new->numKeys = tree->degree + 1 - (tree->degree + 1) / 2;
            //new->parent = root->parent;
            
            root->child[root->numKeys] = new->RNN;
            new->child[new->numKeys] = root->child[tree->degree];
            root->child[tree->degree] = -1;

            for (i = 0; i < root->numKeys; i++){
                strcpy(root->keys[i], keys[i]);
                root->dataRNN[i] = datas[i];
            }

            for (i = 0, j = root->numKeys; i < new->numKeys; i++, j++){
                strcpy(new->keys[i], keys[j]);
                new->dataRNN[i] = datas[j];
            }

            //liga as folhas
            root->next_node = new->RNN;

            if (root->RNN == tree->root) //se for split na root da arvore
            {
                No *tempRoot = createNode(tree);
                
                strcpy(tempRoot->keys[0], new->keys[0]); //aumenta o grau da arvore
                tempRoot->child[0] = root->RNN; //aponta pros filhos
                tempRoot->child[1] = new->RNN;

                new->parent = tempRoot->RNN; //troca referencia do pai
                root->parent = tempRoot->RNN;

                tempRoot->isLeaf = 0;
                tempRoot->numKeys++;
                tree->root = tempRoot->RNN;

                //root->next_node = new->RNN; //liga as folhas (new e root sao folhas)

                overwriteNode(root->RNN, root); //reajusta os nós no arquivo
                overwriteNode(new->RNN, new); //adiciona a nova folha
                overwriteNode(tempRoot->RNN, tempRoot); //adiciona a nova raiz
                changeRoot(tree);

                //limpa
                free(new);
                free(root);
                free(tempRoot);
            }
            else
            {
                printf("NonLeaf!??\n");
                overwriteNode(root->RNN, root); //aplica alteração da root antes de prossegur
                overwriteNode(new->RNN, new); 
                insertInsideTree(tree, fil, new->keys[0], loadNode(root->parent), new);
                //é necessario passar o valor da key a ser promovida para ordenar direito e evitar bugs
            }
        }
    }
}

//printa os nos a partir de um ponto
void printTreeFrom(No *root) 
{
    No *temp;
    int i = 0, j;
    int isLast;

    if(root != NULL)
    {
        // iterate the node element
        while(root->isLeaf == 0)
        {
            temp = root;
            root = loadNode(root->child[0]);
            free(temp);
        }
        
        if(root->isLeaf == 1) //cheguei na folhas -> lista de folhas
        {
            isLast = 0;
            while(isLast != 1)
            {
                printf("-->");
                if(root->next_node == -1) //ultima lista a ser printada
                    isLast = 1;

                for(i = 0; i < root->numKeys; i++)
                {
                    printf("%s ", root->keys[i]);
                }

                temp = root;
                root = loadNode(root->next_node);
                free(temp);
                
            }
        }
    }
    
}

//================================================================= Funçoes dos Dados =================================================//

char* criaKey(char* diretor, char ano[4])
{
    char *key = malloc(sizeof(char) * 6);
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

//funçao para passar uma string inteira para lowcase
char *strlwr(char *str)
{
  unsigned char *p = (unsigned char *)str;

  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }

  return str;
}

//funçao comparadora dos titulos -> usado para busca binaria
int cmpStringTitle(const void *a, const void *b) //compara String
{
    SecondaryKey sa , sb;
    char ta[100], tb[100];
    sa = *(SecondaryKey*)a;
    sb = *(SecondaryKey*)b;
    strcpy(ta, strlwr(sa.title));
    strcpy(tb, strlwr(sb.title));

    return strcmp(ta, tb) ;
}


Filme* criaFilme(char *key, char *titulo, char *titulo_ori, char *diretor, char ano[4], char *pais, int nota)
{
    Filme *e; 

    int soma = strlen(titulo) + strlen(diretor) + strlen(titulo_ori) + strlen(pais) + 5 + 4 + 1;

    if(soma <= 175)
    {
        e = malloc(sizeof(Filme));

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

Filme* criaFilmePorPosicao(int index)
{
    char *token;
    char string[maxSize];
    char split[7][200];
    int i;

    FILE *dados = fopen(filmesFile, "r");;
    Filme *fil = NULL;

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

        //cria o filme na memoria
        fil = criaFilme(split[0],split[1],split[2],split[3],split[4],split[5],atoi(split[6]));
        fclose(dados);

        return fil;
    }
    
    return NULL;

}

int buscarFilme(BpTree *tree, char key[6], int doPrint)
{
    No *no;

    no = searchPage(loadNode(tree->root), key);
    if(no)
    {
        for(int i = 0; i <= no->numKeys; i++) //olha chaves da pagina
        {
            if(i == no->numKeys && strcmp(key, no->keys[i+1]) == 0)
            {
                if(doPrint == 1) printFilme(criaFilmePorPosicao(no->dataRNN[i+1]));
                return no->dataRNN[i+1];
            }

            if(strcmp(key, no->keys[i]) == 0)
            {
                if(doPrint == 1) printFilme(criaFilmePorPosicao(no->dataRNN[i]));
                return no->dataRNN[i];
            }
        }
    }
    else
    {
        printf("Pagina/Chave não encontrada\n");
    }

    return -1;
}

//busca filme pelo titulo da chave secundaria
int buscarFilmeTitulo(BpTree *tree, SecondaryKey **sArr, char title[100], int doPrint)
{
    SecondaryKey *sKey = (SecondaryKey*) bsearch(&title, *sArr, tree->dataCount, sizeof(SecondaryKey), &cmpStringTitle);

    if(sKey)
    {
        return buscarFilme(tree, sKey->key, doPrint);
    }
    else
    {
        printf("Titulo não encontrado!");
    }
}

//modifica a nota de um filme
void modificarNota(int RNN, int nota)
{
    int pos = RNN * maxSize;
    int counter = 0;
    char c;

    FILE *dados = fopen(filmesFile, "r+");    
    fseek(dados, pos, SEEK_SET);
    if(dados)
    {
        while(counter < 6 && (c=fgetc(dados)) != EOF ) //procura até achar a nota
        {
            if(c == '@') counter++;
        }

        if(counter == 6)
        {
            fprintf(dados, "%d", nota); 
            printf("Nota Modificada!\n");
        } 

        fclose(dados);
    } 

}

//======================================================= Funçoes do Sistema ======================================================//

//modifica a nota do filme
void changeNota(char key[6], int nota, BpTree *tree)
{
    No* no = searchPage(loadNode(tree->root), key);
    int i;

    if(no)
    {
        for(i = 0; i <= no->numKeys; i++) //olha todas as keys da pagina
        {
            if(i == no->numKeys && strcmp(no->keys[i-1], key) == 0)
            {
                modificarNota(no->dataRNN[i], nota);
            }

            if(strcmp(no->keys[i], key) == 0)
            {
                modificarNota(no->dataRNN[i], nota);
            }
        }
    }
    else
    {
        printf("Não foi possivel carregar a pagina raiz!\n");
    }

}

void OpAdicionarFilme(BpTree *tree, SecondaryKey **sArr)
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

    //cria elemento de filme
    Filme *fil = criaFilme(criaKey(diretor, ano), titulo, titulo_ori, diretor, ano, pais, nota);

    if(fil)
    {
        if(sArr != NULL)
        {
            *sArr = realloc(*sArr, ((tree->dataCount)+1) * sizeof(SecondaryKey));
        }
        else
        {
            *sArr = malloc(sizeof(SecondaryKey));
        }

        //passa dados para array
        (*sArr)[tree->dataCount].title = malloc((strlen(fil->titulo)+1) * sizeof(char));
        strcpy((*sArr)[tree->dataCount].title, fil->titulo);
        strcpy((*sArr)[tree->dataCount].key, fil->key);
        qsort(*sArr, tree->dataCount, sizeof(SecondaryKey), &cmpStringTitle);

        //insere o filme na arvore
        insertTree(tree, fil);
        free(fil);
    }
    else
    {
        printf("Nao foi possivel criar a instancia do filme! Cheque os valores inseridos!\n");
    }
    

}

//retorna se necessario o RNN dos dados
int opBuscarFilme(BpTree *tree, SecondaryKey **sArr)
{
    int op = -1;
    char key[6], titulo[100];
    

    while(op != 0)
    {   
        printf("Digite o tipo de busca:\n<1> - Busca por Chave\n<2>- Busca por Titulo\n<0> - Cancelar\n");
        scanf(" %d", &op);

        switch(op){        

            case 1: 
                    printf("Digite a Chave do Filme:\n");
                    scanf(" %s", key);
                    return buscarFilme(tree, key, 1); 
                    break;

            case 2: printf("Digite o Titulo do Filme:\n");
                    scanf(" %[^\n]s", titulo);
                    return buscarFilmeTitulo(tree, sArr, titulo, 1); 
                    break;

            case 0: return -1; break;
            default: printf("Opçao invalida!\n"); break;
        }
    }

    return -1;
}

//lista todos os filmes na arvore
void opListarFilmes(BpTree *tree)
{

    No *temp;
    No *root = loadNode(tree->root);
    Filme *fil;
    int i = 0, j;
    int isLast;

    if(root != NULL)
    {
        // iterate the node element
        while(root->isLeaf == 0)
        {
            temp = root;
            root = loadNode(root->child[0]);
            free(temp);
        }
        
        if(root->isLeaf == 1) //cheguei na folhas -> lista de folhas
        {
            isLast = 0;
            while(isLast != 1)
            {
                if(root->next_node == -1) //ultima lista a ser printada
                    isLast = 1;

                for(i = 0; i < root->numKeys; i++)
                {
                    fil = criaFilmePorPosicao(root->dataRNN[i]);
                    printFilme(fil);
                }

                temp = root;
                root = loadNode(root->next_node);
                free(temp);
                
            }
        }
    }
    

}

//modifica a nota do filme
void opModificarFilme(BpTree *tree)
{
    char key[6];
    int nota;
    printf("Digite a Chave do Filme:\n");
    scanf(" %s", key);
    printf("Digite a nova Nota do Filme:\n");
    scanf(" %d", &nota);

    changeNota(key, nota, tree);
}

//checa se todos os arquivos necessarios existem
void checkFiles(){
    FILE *temp;

    if((temp = fopen(treeFile, "r")) == NULL)
    {
        temp = fopen(treeFile, "w");
        fclose(temp);
    }
    //temp = fopen(treeFile, "w"); //para debug
    //fclose(temp);

    if((temp = fopen(filmesFile, "r")) == NULL)
    {
        temp = fopen(filmesFile, "w");
        fclose(temp);
    }
    //temp = fopen(filmesFile, "w"); //para debug
    //fclose(temp);

    if((temp = fopen(titleFile, "r")) == NULL)
    {
        temp = fopen(titleFile, "w");
        fclose(temp);
    }
}

int main(int argc, char const *argv[]){

    BpTree *tree = createTree(order);
    No *temp;
    int op = -1;
    int i; 

    FILE *sFile;
    SecondaryKey *sArr = NULL;
    char tempString[100];
    char *tempTitle, tempKey[6];

    loadTree(tree); //carrega se houver arquivo

    checkFiles(); //verifica integridade dos arquivos

    //carrega lista secundaria do arquivo
    sFile = fopen(titleFile, "r");
    if(sFile)
    {
        sArr = malloc(sizeof(SecondaryKey) * tree->dataCount); //aloca o tamanho necessario

        while(fscanf(sFile, " %[^\n]s", tempString) != EOF)
        {
           int tamanho = strlen(tempString); //tamanho da palavras

            tempTitle = malloc(sizeof(char) * (tamanho-6)); //-5 pelo espaço da Key
            strncpy(tempTitle, tempString, tamanho-5);
            tempTitle[tamanho-6] = '\0'; //n é adicionado automaticamente o \0

            for(int j = 0; j < 5; j++) //copia key
            {
                tempKey[j] = tempString[tamanho+j-5];
            }
            tempKey[5] = '\0';
            
            sArr[i].title = malloc(sizeof(char) * (tamanho-4));
            strcpy(sArr[i].title, tempTitle);
            strcpy(sArr[i].key, tempKey);

            i++;
        }

        fclose(sFile);
    }

    //operaçoes
    while(op != 0)
    {
        printf("Digite o que deseja fazer:\n<1> - Inserir Filme\n<2> - Buscar Filme\n<3> - Modificar Nota\n<4> - Listar Todos\n<0> - Sair do programa\n");
        scanf("%d", &op);

        switch(op)
        {
            case 1: OpAdicionarFilme(tree, &sArr); break;
            case 2: opBuscarFilme(tree, &sArr); break;
            case 3: opModificarFilme(tree); break;
            case 4: opListarFilmes(tree); break;
            case 0: changeRoot(tree); break; //saida
            default: printf("Opçao invalida!\n"); break;
        }
    }

    //salva array de Chaves secundarias
    sFile = fopen(titleFile, "w");
    if(sFile && sArr != NULL)
    {
        for(i = 0; i < tree->dataCount; i++)
        {
            fprintf(sFile, "%s %s", sArr[i].title, sArr[i].key);
            fprintf(sFile, "\n");
        }
        fclose(sFile);
    }

    //para exibir os dados nas folhas
    //temp = loadNode(tree->root);
    //printTreeFrom(temp);

    printf("\nFinalizado!\n");

    return 0;
}