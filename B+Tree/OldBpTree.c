#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

//é nessesario excluir arquivos antigos quando alterar o define!!!!
#define order 5 //significa ordem 6, levando em conta o 0 como elemento
#define maxSize 192
#define filmesFile "filmes.dat"
#define treeFile "tree.dat"
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
    int degree;

}BpTree;

//====================================================== Funçoes Arvore =======================================================//

//Cria a arvore B+
BpTree* createTree(int degree)
{
    BpTree* tree = (BpTree*) malloc(sizeof(BpTree));
    tree->root = -1;
    tree->degree = degree-1;
    tree->pageCount = 0;

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
                fseek(fp, 1 + (RNN * sizeof(No)), SEEK_SET); //+1 por causa do flag
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
            fseek(fp, 1 + (RNN * sizeof(No)), SEEK_SET);
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
void changeRoot(int rootRNN)
{
    FILE *fp = fopen(treeFile, "r+b"); //abre p arquivo em binario
    
    if(fp)
    {
        fseek(fp, 0, SEEK_SET);
        fprintf(fp, "%d", rootRNN);
    }
    else
    {
        printf("ERRO> Arquivo não encontrado!\n");
    }

    fclose(fp);
}


No* search(No *root, char key[6])
{
    No *no = loadNode(root->RNN);
    int found = 0;

    while(!no->isLeaf)
    {
        for(int i = 0; (i < no->numKeys) && (found != 0); i++)
        {
            if(i == no->numKeys)
            {
                no = loadNode(no->child[i+1]);
            }

            if(strcmp(key, no->keys[i]) <= 0)
            {
                no = loadNode(no->child[i]);
                found = 1;
            }

        }
    }
    
    if(no == NULL){
        printf("Não encontrado!/n");
    }

    return no; //Null se não achar

}

//insere em algum ponto da arvore
void insertInsideTree(BpTree *tree, Filme fil, char curKey[6], No *parent, No *child){
    int i, j;
    
    No *new;
    No *temp;
    char tempKeys[tree->degree + 1][6]; //carrega keys com extra
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
                parent->child[j] = parent->child[j - 1];
            }
            parent->child[j+1] = parent->child[j];
            
            strcpy(parent->keys[i], curKey);
            parent->numKeys++;
            parent->child[i + 1] = child->RNN;
            
            child->parent = parent->RNN; //troca o pai do inserido

            /*liga filhos
            if(child->isLeaf == 1)
            {
                temp = loadNode(parent->child[i]);
                if(temp != NULL){
                    temp->next_node = child->RNN;
                    overwriteNode(temp->RNN, temp);
                    free(temp);
                }

                temp = loadNode(parent->child[i+2]);         
                 if(temp != NULL){
                    child->next_node = temp->RNN;
                    overwriteNode(temp->RNN, temp);
                    free(temp);
                }
            }
            */

            overwriteNode(parent->RNN, parent); //aplica a nova alteração no pai
            overwriteNode(child->RNN, child); //aplica a alteraçao no filho (cumulativo da anterior)
            free(parent);
        }
        else //split
        {
            printf("Resplit\n");
            new = createNode(tree);
            
            for (i = 0; i < tree->degree ; i++) //copy old data
            {
                strcpy(tempKeys[i], parent->keys[i]);
                ptrs[i] = parent->child[i];
            }
            ptrs[i+1] = parent->child[i+1];
            
            i = 0;
            while(strcmp(curKey, parent->keys[i]) > 0 && i < tree->degree)
                i++;
                
            for (j = tree->degree + 1; j > i; j--) //distribui chaves e ponteiros
            {
                strcpy(parent->keys[j], parent->keys[j-1]);
                parent->child[j] = parent->child[j - 1];
            }
            parent->child[j+1] = parent->child[j];
            
            //adiciona chave
            strcpy(tempKeys[i], curKey);
            parent->child[i+1] = child->RNN;

            new->isLeaf = 0; //deixa de ser folha
            parent->numKeys = (tree->degree + 1) / 2; //recebe metade dos dados
            new->numKeys = tree->degree - ((tree->degree + 1) / 2);
            
            //reparte
            for (i = 0, j = parent->numKeys + 1; i < new->numKeys; i++, j++)
                strcpy(new->keys[i], tempKeys[j]);

            for (i = 0, j = parent->numKeys + 1; i < new->numKeys + 1; i++, j++)
                new->child[i] = ptrs[j];
            
            if(parent->RNN == tree->root) //se o pai era a root da arvore
            {
                No* tempRoot, *tempParent, *tempChild[2];
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
                changeRoot(tree->root); //deu split, novo root

                //limpa
                free(new);
                free(parent);
                free(tempRoot);
                free(tempParent);
            }
            else
            {
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
void insertTree(BpTree *tree, Filme fil)
{
    int i, j;
    int inserted = 0;
    printf("Start!\n");

    No *root = loadNode(tree->root);
    No *parent = NULL, *new = NULL;
    char keys[order+2][6]; //extra pra split
    No *temp;
    
    if(tree->root == -1) //primeiro elemento da arvore
    { 
        temp = createNode(tree);
        strcpy(temp->keys[0], fil.key);
        temp->isLeaf  = 1;
        temp->numKeys++;
        tree->root = temp->RNN;

        changeRoot(temp->RNN);
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
                if (strcmp(fil.key , root->keys[i]) < 0)
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
            while (strcmp(fil.key , root->keys[i]) > 0 && i < root->numKeys)
                i++;
            
            for (j = root->numKeys; j > i; j--)
                strcpy(root->keys[j], root->keys[j - 1]);
            
            //adiciona ordenado
            strcpy(root->keys[i], fil.key);
            root->numKeys++;
            root->child[root->numKeys] = root->child[root->numKeys - 1];
            root->child[root->numKeys - 1] = -1; //n esquecer de adicionar o novo ponteiro
            
            overwriteNode(root->RNN, root);

            free(root);
        }
        else //requer split
        {
            printf("Split!\n");
            new = createNode(tree);
            
            for (i = 0; i < tree->degree; i++)
                strcpy(keys[i], root->keys[i]);
            
            i = 0;
            while (strcmp(fil.key, keys[i]) > 0 && i < tree->degree)
                i++;
    
            for (j = tree->degree + 1; j > i; j--) //abre espaco para key inserida
                strcpy(keys[j], keys[j - 1]);

            //insere a key
            strcpy(keys[i], fil.key);
            new->isLeaf = 1;
            root->numKeys = ceil((tree->degree + 1) / 2);
            new->numKeys = ceil(tree->degree + 1 - (tree->degree + 1) / 2);
            //new->parent = root->parent;
            
            root->child[root->numKeys] = new->RNN;
            new->child[new->numKeys] = root->child[tree->degree];
            root->child[tree->degree] = -1;

            for (i = 0; i < root->numKeys; i++)
                strcpy(root->keys[i], keys[i]);

            for (i = 0, j = root->numKeys; i < new->numKeys; i++, j++)
                strcpy(new->keys[i], keys[j]);

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
                changeRoot(tree->root);

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

//funçao comparadora dos titulos -> usado para busca binaria
int cmpStringTitle(const void *a, const void *b) //compara String
{
    SecondaryKey sa , sb;
    sa = *(SecondaryKey*)a;
    sb = *(SecondaryKey*)b;

    return strcmp(sa.title, sb.title) ;
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

//busca filme pelo titulo da chave secundaria
void buscarFilmeTitulo();

//modifica a nota de um filme
void modificarNota(int memPos, int nota)
{
    int pos = memPos * maxSize;
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
            printf("Modificado!\n");
        } 

        fclose(dados);
    } 

}

//======================================================= Funçoes do Sistema ======================================================//

//modifica a nota do filme
void changeNota(char key[6], int nota, BpTree *tree)
{
    No* no = search(loadNode(tree->root), key);

    if(no)
    {
        for(int i = 0; i < order; i++) //olha todas as keys da pagina
        {
            if(strcmp(no->keys[i], key) == 0)
                modificarNota(no->dataRNN[i], nota);
        }
    }
    else
    {
        printf("Não foi possivel carregar a pagina raiz!\n");
    }

}


//checa se todos os arquivos necessarios existem
void checkFiles(){
    FILE *temp;

    if((temp = fopen(treeFile, "r")) == NULL)
    {
        temp = fopen(treeFile, "w");
        fclose(temp);
    }

    if((temp = fopen(filmesFile, "r")) == NULL)
    {
        temp = fopen(filmesFile, "w");
        fclose(temp);
    }

    if((temp = fopen(titleFile, "r")) == NULL)
    {
        temp = fopen(titleFile, "w");
        fclose(temp);
    }
}

int main(int argc, char const *argv[]){

    BpTree *tree = createTree(order);
    Filme fil, fil2, fil3, fil4, fil5, fil6, fil7, fil8, fil9, fil10, fil11, fil12, fil13;
    No *temp;

    strcpy(fil.key, "CCCCC");
    strcpy(fil2.key, "BBBBB");
    strcpy(fil3.key, "AAAAA");
    strcpy(fil4.key, "FFFFF");
    strcpy(fil5.key, "DDDDD");
    strcpy(fil6.key, "XXXXX");

    strcpy(fil7.key, "EEEEE");
    strcpy(fil8.key, "SSSSS");
    strcpy(fil9.key, "GGGGG");
    strcpy(fil10.key, "HHHHH");
    strcpy(fil11.key, "YYYYY");
    strcpy(fil12.key, "TTTTT");
    strcpy(fil13.key, "WWWWW");
    

    checkFiles(); //verifica integridade dos arquivos
    
    insertTree(tree, fil);
    insertTree(tree, fil2);
    insertTree(tree, fil3);
    insertTree(tree, fil4);
    insertTree(tree, fil5);
    insertTree(tree, fil6);
    insertTree(tree, fil7);
    insertTree(tree, fil8);
    insertTree(tree, fil9);
    insertTree(tree, fil10);
    insertTree(tree, fil11);
    insertTree(tree, fil12);
    insertTree(tree, fil13);
    
    temp = loadNode(tree->root);
    printf(" pageCount: %d\n", tree->pageCount);
    printf(" Root-%d--%s\n", tree->root, temp->keys[0]);

    printTreeFrom(temp);
    printf("\nFinalizado!\n");

    return 0;
}

/*problemas secundarios
1) nós nao limpao chaves antigas quando sao repartidas



void printTreeFrom(No *root) 
{
    No *temp;
    int i = 0;
    int j;

    if(root != NULL)
    {
        // iterate the node element
        while(i < root->numKeys)
        {
            if(root->isLeaf==0)
            {
                // When node is not leaf
                printTreeFrom(loadNode(root->child[i]));
            }
            else
            {
                // Print the left node value
                printf("%s ",root->keys[i]); 
            }
            i++;
        }
        
        if(root != NULL && root->isLeaf == 0)
        {
            printTreeFrom(loadNode(root->child[i]));
        }
    }
    
}

*/